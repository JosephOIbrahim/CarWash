"""Check ComfyUI available nodes - diagnose why HdCarWash workflow has empty outputs."""
import requests
import json

COMFYUI_URL = "http://127.0.0.1:8188"

def main():
    print("=" * 60)
    print("  ComfyUI Node Availability Check")
    print("=" * 60)

    # Get object_info which lists all available nodes
    try:
        resp = requests.get(f"{COMFYUI_URL}/object_info", timeout=10)
        if resp.status_code != 200:
            print(f"ERROR: Got status {resp.status_code}")
            return

        nodes = resp.json()
        print(f"\nTotal available nodes: {len(nodes)}")

        # Check for critical nodes used by HdCarWash workflow
        critical_nodes = [
            "easy loadImageBase64",  # Custom node from ComfyUI-Easy-Use
            "CheckpointLoaderSimple",
            "CLIPTextEncode",
            "ControlNetLoader",
            "ControlNetApplyAdvanced",
            "EmptyLatentImage",
            "KSampler",
            "VAEDecode",
            "SaveImage",
        ]

        print("\n--- Critical Nodes for HdCarWash ---")
        missing = []
        for node_name in critical_nodes:
            if node_name in nodes:
                print(f"  [OK] {node_name}")
            else:
                print(f"  [MISSING] {node_name}")
                missing.append(node_name)

        if missing:
            print(f"\n*** PROBLEM: {len(missing)} required node(s) missing! ***")
            if "easy loadImageBase64" in missing:
                print("\n  The 'easy loadImageBase64' node is from ComfyUI-Easy-Use pack.")
                print("  Install it in ComfyUI Manager or from:")
                print("  https://github.com/yolain/ComfyUI-Easy-Use")
                print("\n  ALTERNATIVE: Use LoadImageFromBase64 from other packs:")
                # Check for alternatives
                alternatives = [
                    "LoadImageFromBase64",
                    "Base64ToImage",
                    "LoadImage",
                    "ETN_LoadImageBase64",
                ]
                print("\n  Checking for alternative base64 nodes:")
                for alt in alternatives:
                    if alt in nodes:
                        print(f"    [FOUND] {alt} - could use this instead")
                    else:
                        print(f"    [NOT FOUND] {alt}")
        else:
            print("\n  All critical nodes available!")

        # Also search for any base64-related nodes
        print("\n--- All Base64-related Nodes ---")
        base64_nodes = [name for name in nodes.keys() if 'base64' in name.lower()]
        if base64_nodes:
            for name in base64_nodes:
                print(f"  - {name}")
        else:
            print("  NONE FOUND! No base64 image loading nodes available.")
            print("\n  This is the problem - HdCarWash needs to send images as base64.")
            print("  Install ComfyUI-Easy-Use or another pack with base64 support.")

        # Check for ControlNet models
        print("\n--- ControlNet Loader Check ---")
        if "ControlNetLoader" in nodes:
            node_info = nodes["ControlNetLoader"]
            if "input" in node_info and "required" in node_info["input"]:
                req = node_info["input"]["required"]
                if "control_net_name" in req:
                    # Get available models
                    models = req["control_net_name"][0] if req["control_net_name"] else []
                    print(f"  Available ControlNet models ({len(models)}):")
                    for model in models[:10]:  # Show first 10
                        marker = " <-- DEPTH" if "depth" in model.lower() else ""
                        marker = " <-- CANNY" if "canny" in model.lower() else marker
                        print(f"    - {model}{marker}")
                    if len(models) > 10:
                        print(f"    ... and {len(models) - 10} more")

                    # Check for required models
                    depth_model = "controlnet-depth-sdxl-1.0.safetensors"
                    canny_model = "controlnet-canny-sdxl-1.0.safetensors"

                    print(f"\n  Required model check:")
                    if depth_model in models:
                        print(f"    [OK] {depth_model}")
                    else:
                        print(f"    [MISSING] {depth_model}")
                        # Find similar
                        depth_alts = [m for m in models if "depth" in m.lower()]
                        if depth_alts:
                            print(f"         Available depth models: {depth_alts[:3]}")

                    if canny_model in models:
                        print(f"    [OK] {canny_model}")
                    else:
                        print(f"    [MISSING] {canny_model}")
                        canny_alts = [m for m in models if "canny" in m.lower()]
                        if canny_alts:
                            print(f"         Available canny models: {canny_alts[:3]}")

        print("\n" + "=" * 60)

    except requests.exceptions.ConnectionError:
        print("ERROR: Cannot connect to ComfyUI at", COMFYUI_URL)
        print("Make sure ComfyUI is running.")
    except Exception as e:
        print(f"ERROR: {e}")

if __name__ == "__main__":
    main()
