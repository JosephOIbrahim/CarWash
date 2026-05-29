#!/usr/bin/env python3
"""
CarWash Renderer ComfyUI Connection Test
Tests the basic API workflow without Houdini
"""

import urllib.request
import json
import time
import sys

COMFY_URL = "http://localhost:8188"

def test_connection():
    """Test basic connection to ComfyUI"""
    print("Testing ComfyUI connection...")
    try:
        req = urllib.request.Request(f"{COMFY_URL}/system_stats")
        with urllib.request.urlopen(req, timeout=5) as response:
            data = json.loads(response.read().decode())
            print(f"  [OK] Connected! VRAM: {data.get('system', {}).get('vram_total', 'N/A')}")
            return True
    except Exception as e:
        print(f"  [FAIL] {e}")
        return False

def test_workflow_submission():
    """Test submitting a simple workflow"""
    print("\nTesting workflow submission...")

    # Simple test workflow (no images, just text-to-image)
    workflow = {
        "client_id": "hdcarwash_test",
        "prompt": {
            "1": {
                "class_type": "CheckpointLoaderSimple",
                "inputs": {"ckpt_name": "sdxl_v10VAEFix.safetensors"}
            },
            "2": {
                "class_type": "CLIPTextEncode",
                "inputs": {"text": "a simple test cube, 3D render", "clip": ["1", 1]}
            },
            "3": {
                "class_type": "CLIPTextEncode",
                "inputs": {"text": "blurry, bad", "clip": ["1", 1]}
            },
            "4": {
                "class_type": "EmptyLatentImage",
                "inputs": {"width": 512, "height": 512, "batch_size": 1}
            },
            "5": {
                "class_type": "KSampler",
                "inputs": {
                    "model": ["1", 0], "positive": ["2", 0], "negative": ["3", 0],
                    "latent_image": ["4", 0], "seed": 42, "steps": 8,
                    "cfg": 7.0, "sampler_name": "euler", "scheduler": "normal", "denoise": 1.0
                }
            },
            "6": {
                "class_type": "VAEDecode",
                "inputs": {"samples": ["5", 0], "vae": ["1", 2]}
            },
            "7": {
                "class_type": "SaveImage",
                "inputs": {"images": ["6", 0], "filename_prefix": "hdcarwash_test"}
            }
        }
    }

    try:
        req = urllib.request.Request(
            f"{COMFY_URL}/prompt",
            data=json.dumps(workflow).encode('utf-8'),
            headers={'Content-Type': 'application/json'}
        )
        with urllib.request.urlopen(req, timeout=10) as response:
            result = json.loads(response.read().decode())
            prompt_id = result.get('prompt_id')
            print(f"  [OK] Workflow submitted! Prompt ID: {prompt_id}")
            return prompt_id
    except Exception as e:
        print(f"  [FAIL] Failed: {e}")
        return None

def wait_for_completion(prompt_id, timeout=120):
    """Wait for workflow to complete"""
    print(f"\nWaiting for completion (max {timeout}s)...")
    start = time.time()

    while time.time() - start < timeout:
        try:
            req = urllib.request.Request(f"{COMFY_URL}/history/{prompt_id}")
            with urllib.request.urlopen(req, timeout=5) as response:
                history = json.loads(response.read().decode())

                if prompt_id in history:
                    outputs = history[prompt_id].get('outputs', {})
                    if outputs:
                        print(f"  [OK] Completed in {time.time()-start:.1f}s")
                        # Find output image
                        for node_id, node_out in outputs.items():
                            if 'images' in node_out:
                                for img in node_out['images']:
                                    print(f"  [FILE] Output: {img.get('subfolder', '')}/{img.get('filename')}")
                        return True
        except Exception as e:
            pass

        elapsed = int(time.time() - start)
        print(f"  [...] Processing... ({elapsed}s)", end='\r')
        time.sleep(1)

    print(f"  [FAIL] Timeout after {timeout}s")
    return False

def test_base64_node():
    """Test if easy loadImageBase64 node exists and works"""
    print("\nTesting easy loadImageBase64 node...")
    try:
        req = urllib.request.Request(f"{COMFY_URL}/object_info/easy%20loadImageBase64")
        with urllib.request.urlopen(req, timeout=5) as response:
            data = json.loads(response.read().decode())
            if 'easy loadImageBase64' in data:
                print("  [OK] Node available!")
                inputs = data['easy loadImageBase64'].get('input', {}).get('required', {})
                print(f"  [INFO] Inputs: {list(inputs.keys())}")
                return True
    except Exception as e:
        print(f"  [FAIL] Node not found: {e}")
    return False

def main():
    print("=" * 60)
    print("CarWash Renderer ComfyUI Integration Test")
    print("=" * 60)

    # Test 1: Connection
    if not test_connection():
        print("\n[FAIL] Cannot connect to ComfyUI. Is it running at localhost:8188?")
        return 1

    # Test 2: Base64 node
    test_base64_node()

    # Test 3: Workflow submission
    prompt_id = test_workflow_submission()
    if not prompt_id:
        print("\n[FAIL] Failed to submit workflow")
        return 1

    # Test 4: Wait for completion
    if not wait_for_completion(prompt_id, timeout=120):
        print("\n[FAIL] Workflow did not complete")
        return 1

    print("\n" + "=" * 60)
    print("[OK] All tests passed! CarWash Renderer can communicate with ComfyUI")
    print("=" * 60)
    return 0

if __name__ == "__main__":
    sys.exit(main())
