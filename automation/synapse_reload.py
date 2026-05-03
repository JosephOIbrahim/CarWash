#!/usr/bin/env python
"""Reload the HdCarWash plugin via the Synapse bridge.

When invoked without arguments, queries the Houdini scene for the current LOP
stage. With ``--install``, copies the freshly built plugin DLL into the
Houdini DSO directory (run *after* Houdini has been closed so the file is
unlocked).
"""

from __future__ import annotations

import argparse
import asyncio
import json
import logging
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from synapse import SynapseCommandError, SynapseConnectionError, load_config
from synapse._protocol import connect, send_command
from synapse.paths import carwash_build_dll, carwash_dll_path

log = logging.getLogger("carwash.synapse_reload")


_SCENE_PROBE = """
import hou

result = []
stage = hou.node('/stage')
if stage:
    result.append('Stage: ' + stage.path())
    for child in stage.children():
        result.append('  ' + child.name() + ' (' + child.type().name() + ')')
else:
    result.append('No stage node')

sopimport = hou.node('/stage/sopimport1')
if sopimport:
    result.append('SOP Import found: ' + sopimport.path())

'|'.join(result)
"""


async def reload_plugin() -> None:
    config = load_config()
    async with connect(config) as ws:
        print("Connected to Synapse")
        result = await send_command(
            ws,
            {"type": "execute_python", "payload": {"code": _SCENE_PROBE}},
            timeout=5.0,
        )

        if result.get("success"):
            data = result.get("data", {}).get("result", "")
            if data:
                for line in data.split("|"):
                    print(line)
        else:
            raise SynapseCommandError(
                f"Scene probe failed: {result.get('error')}",
                command={"type": "execute_python"},
                response=result,
            )

        print("\n" + "=" * 50)
        print("To update the plugin, please:")
        print("1. Save your scene (if needed)")
        print("2. Close Houdini")
        print("3. Run: python synapse_reload.py --install")
        print("4. Reopen Houdini")
        print("=" * 50)


def install_dll() -> bool:
    """Copy the built plugin DLL into the Houdini DSO directory."""
    src = carwash_build_dll()
    dst = carwash_dll_path()

    if not src.exists():
        log.error("Source DLL not found: %s", src)
        log.error("Build first, or set CARWASH_BUILD_DLL to its location.")
        return False

    try:
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
    except PermissionError:
        log.exception("DLL is locked. Close Houdini first. (target=%s)", dst)
        return False

    print(f"SUCCESS: Copied {src}")
    print(f"     to: {dst}")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--install",
        action="store_true",
        help="Copy the built DLL into the Houdini DSO directory and exit.",
    )
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")

    if args.install:
        return 0 if install_dll() else 1

    try:
        asyncio.run(reload_plugin())
    except SynapseConnectionError:
        log.exception("Synapse connection failed")
        return 2
    except SynapseCommandError as exc:
        response_repr = json.dumps(exc.response)[:400] if exc.response is not None else "<none>"
        log.exception("Bridge command failed (response=%s)", response_repr)
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
