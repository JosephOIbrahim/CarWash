"""Test HdCarWash workflow with DEPTH ONLY (no normal ControlNet)."""
import requests
import json
import time
import base64

COMFYUI_URL = "http://127.0.0.1:8188"

# Create a simple 64x64 grayscale depth image (gradient from black to white)
def create_test_depth_png():
    """Create minimal valid grayscale PNG for testing."""
    import struct
    import zlib

    width, height = 64, 64
    # Create gradient depth data (top=near/dark, bottom=far/light)
    pixels = bytes([int(y * 255 / height) for y in range(height) for _ in range(width)])

    def png_chunk(chunk_type, data):
        chunk_len = struct.pack('>I', len(data))
        chunk_data = chunk_type + data
        crc = struct.pack('>I', zlib.crc32(chunk_data) & 0xffffffff)
        return chunk_len + chunk_data + crc

    # PNG signature
    png = b'\x89PNG\r\n\x1a\n'

    # IHDR chunk
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 0, 0, 0, 0)
    png += png_chunk(b'IHDR', ihdr_data)

    # IDAT chunk (compressed image data)
    raw = b''.join([b'\x00' + pixels[y*width:(y+1)*width] for y in range(height)])
    compressed = zlib.compress(raw)
    png += png_chunk(b'IDAT', compressed)

    # IEND chunk
    png += png_chunk(b'IEND', b'')

    return base64.b64encode(png).decode('ascii')


def main():
    print("=" * 60)
    print("  Test HdCarWash Workflow - DEPTH ONLY (no normal)")
    print("=" * 60)

    depth_b64 = create_test_depth_png()
    print(f"\nCreated test depth image: {len(depth_b64)} chars base64")

    # Build workflow - DEPTH ONLY version (matches HdCarWash when useNormalControl=false)
    cache_buster = str(int(time.time() * 1000))

    workflow = {
        "client_id": "hdcarwash-test",
        "prompt": {
            # Node 1: Checkpoint loader
            "1": {
                "class_type": "CheckpointLoaderSimple",
                "inputs": {
                    "ckpt_name": "sdxl_v10VAEFix.safetensors"
                }
            },
            # Node 2: Positive prompt
            "2": {
                "class_type": "CLIPTextEncode",
                "inputs": {
                    "text": "a beautiful mountain landscape, golden hour lighting, photorealistic",
                    "clip": ["1", 1]
                }
            },
            # Node 3: Negative prompt
            "3": {
                "class_type": "CLIPTextEncode",
                "inputs": {
                    "text": "blurry, low quality, distorted",
                    "clip": ["1", 1]
                }
            },
            # Node 4: Load depth from base64
            "4": {
                "class_type": "easy loadImageBase64",
                "inputs": {
                    "base64_data": depth_b64,
                    "image_output": "Hide",
                    "save_prefix": f"hdcarwash_test_depth_{cache_buster}"
                }
            },
            # Node 5: ControlNet Loader (depth)
            "5": {
                "class_type": "ControlNetLoader",
                "inputs": {
                    "control_net_name": "controlnet-depth-sdxl-1.0.safetensors"
                }
            },
            # Node 6: Apply ControlNet
            "6": {
                "class_type": "ControlNetApplyAdvanced",
                "inputs": {
                    "positive": ["2", 0],
                    "negative": ["3", 0],
                    "control_net": ["5", 0],
                    "image": ["4", 0],
                    "strength": 0.8,
                    "start_percent": 0.0,
                    "end_percent": 1.0
                }
            },
            # Node 7: Empty Latent
            "7": {
                "class_type": "EmptyLatentImage",
                "inputs": {
                    "width": 512,
                    "height": 512,
                    "batch_size": 1
                }
            },
            # Node 8: KSampler (depth only path)
            "8": {
                "class_type": "KSampler",
                "inputs": {
                    "model": ["1", 0],
                    "positive": ["6", 0],
                    "negative": ["6", 1],
                    "latent_image": ["7", 0],
                    "seed": 42,
                    "steps": 20,
                    "cfg": 7.5,
                    "sampler_name": "euler",
                    "scheduler": "normal",
                    "denoise": 1.0
                }
            },
            # Node 9: VAE Decode
            "9": {
                "class_type": "VAEDecode",
                "inputs": {
                    "samples": ["8", 0],
                    "vae": ["1", 2]
                }
            },
            # Node 10: Save Image
            "10": {
                "class_type": "SaveImage",
                "inputs": {
                    "images": ["9", 0],
                    "filename_prefix": f"hdcarwash/test_depth_only_{cache_buster}"
                }
            }
        }
    }

    print("\nSubmitting workflow...")
    try:
        resp = requests.post(f"{COMFYUI_URL}/prompt", json=workflow, timeout=10)
        result = resp.json()
        print(f"Response: {result}")

        if "prompt_id" not in result:
            print("ERROR: No prompt_id in response")
            if "error" in result:
                print(f"Error details: {result['error']}")
            if "node_errors" in result:
                print(f"Node errors: {json.dumps(result['node_errors'], indent=2)}")
            return

        prompt_id = result["prompt_id"]
        print(f"Prompt ID: {prompt_id}")

        # Wait for completion
        print("\nWaiting for completion...")
        for i in range(120):  # 2 minutes max
            time.sleep(1)
            hist_resp = requests.get(f"{COMFYUI_URL}/history/{prompt_id}", timeout=10)
            history = hist_resp.json()

            if prompt_id in history:
                entry = history[prompt_id]
                status = entry.get("status", {})
                outputs = entry.get("outputs", {})

                status_str = status.get("status_str", "unknown")
                print(f"  [{i+1}s] Status: {status_str}, Outputs: {len(outputs)} nodes")

                if status_str == "success" or outputs:
                    print(f"\n--- RESULT ---")
                    print(f"Status: {status_str}")
                    print(f"Outputs: {json.dumps(outputs, indent=2)}")

                    if outputs:
                        print("\nSUCCESS - Image generated!")
                        # Find the filename
                        for node_id, node_output in outputs.items():
                            if "images" in node_output:
                                for img in node_output["images"]:
                                    print(f"  Output image: {img.get('subfolder', '')}/{img.get('filename', '')}")
                    else:
                        print("\nFAILED - Empty outputs despite 'success' status")
                        # Show execution details
                        messages = status.get("messages", [])
                        for msg in messages:
                            print(f"  Message: {msg}")
                    return

                if status_str == "error":
                    print(f"\nERROR during execution!")
                    print(f"Status: {json.dumps(status, indent=2)}")
                    return

        print("\nTimeout waiting for completion")

    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
