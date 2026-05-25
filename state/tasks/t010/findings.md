# t010 — Current ComfyUI HTTP/WS API contract (verified vs ComfyUI master, May 2026)

Source: ComfyUI master (server.py, execution.py, comfy_execution/progress.py), official docs
(docs.comfy.org comms_routes / comms_messages), script_examples/websockets_api_example.py.

## Default port
8188 for BOTH HTTP and WebSocket (same aiohttp app). There is NO separate WS port.

## HTTP endpoints (http://host:8188)
- POST /prompt        body {"prompt": <api-format graph>, "client_id": "<id>", ["extra_data": {}]}
                      ok  -> {"prompt_id":"<uuid>","number":<float>,"node_errors":{}}
                      err -> HTTP 400 {"error":...,"node_errors":{...}}
- GET  /history/{prompt_id} -> { "<pid>": { "prompt":[...], "outputs": { "<node_id>": {
                      "images":[{"filename","subfolder","type"}], ... } }, "status": {...} } }
- GET  /view?filename=..&subfolder=..&type=output|input|temp -> raw image bytes
- POST /upload/image  multipart/form-data (image field, optional subfolder/type/overwrite)
                      -> {"name","subfolder","type":"input"}
- GET  /system_stats  -> server/device info JSON

## WebSocket
- Path: ws://host:8188/ws?clientId=<id>   (query param spelled clientId, camelCase)
- POST /prompt client_id (snake_case) MUST equal the WS clientId to receive targeted messages.
- Text frames: {"type":<name>, "data":{...}}. Binary frames: preview images (opcode 0x2) — not JSON.

## Current message types (exact)
- status            {"status":{"exec_info":{"queue_remaining":N}}, "sid":"<id>"}
- execution_start   {"prompt_id"}
- execution_cached  {"nodes":[...], "prompt_id"}
- executing         {"node":<id|null>, "display_node":<id>, "prompt_id"}  (node:null = prompt end sentinel)
- progress_state    {"prompt_id", "nodes":{"<id>":{"value","max","state","node_id","display_node_id"}}}  <- CURRENT primary progress
- progress          {"value","max","prompt_id","node"}  (legacy, still emitted)
- executed          {"node","display_node","output":{...}, "prompt_id"}  (only when a node returns UI output, e.g. SaveImage)
- execution_success {"prompt_id"}   <- authoritative whole-prompt done signal (newer; CarWash doesn't handle)
- execution_error   {"prompt_id","node_id","node_type","exception_message", ...}

## Deltas to fix in CarWash client (feeds t021/t024/t030)
1. WS default is wrong: comfyClient.h:79 wsUrl="ws://localhost:9999" and cpp:1960 port=9999.
   Real default ws://127.0.0.1:8188/ws. -> WS never connects today; silently polls /history.
2. Add handling for execution_success (cleanest terminal signal) — cpp:2313-2356 parser lacks it.
3. Add progress_state / progress parsing for real step-level progress (currently none).
4. executing:null is the canonical end sentinel; executed only fires on UI-output nodes — current
   completion logic (relying on executed+SaveImage) is fragile.
5. Handle binary preview frames (opcode 0x2) in the WS receive loop (currently text-only).
6. Hardcoded Sec-WebSocket-Key works (not validated server-side) but is non-spec — PARKED/minor.
Endpoints /prompt,/history,/view,/upload/image,/system_stats request/response shapes used today
are still CORRECT.
