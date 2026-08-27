#!/usr/bin/env python3
"""Observe Humanoid.attribute transitions in a live retail mission.

A research driver over ``tools/pcsx_gameplay.py``'s machinery: boots the
retail-identical build into a mission, then ``tools/pcsx-attrbits.lua``
steers the player around while logging every attribute/status change on
every live humanoid (with EmergencyNotice and motID context) so the
undocumented attribute bits can be named from observed transitions.

Retail layout only — the Lua probe's polled globals are retail addresses:

    python3 tools/pcsx_attrbits.py --repack \
        --exe .shake/build/tenchu/main.exe \
        --elf .shake/build/tenchu/main.exe.elf

Pass ``TENCHU_ATTR_OBSERVE`` (vsyncs, default 9000) to size the window.
"""

from __future__ import annotations

from pathlib import Path
import sys

try:
    from tools import pcsx_gameplay
except ImportError:
    import pcsx_gameplay  # type: ignore[no-redef]


def run(argv=None) -> int:
    pcsx_gameplay.LUA_PROBE = Path(__file__).with_name("pcsx-attrbits.lua")
    root = Path(__file__).resolve().parent.parent
    args = list(sys.argv[1:] if argv is None else argv)
    if "--exe" not in args:
        args += ["--exe", str(root / ".shake/build/tenchu/main.exe")]
    if "--elf" not in args:
        args += ["--elf", str(root / ".shake/build/tenchu/main.exe.elf")]
    # The observer passes on window completion; the act/cb thresholds only
    # gate how long the menus may take, so keep the defaults.
    return pcsx_gameplay.run(args)


if __name__ == "__main__":
    raise SystemExit(run())
