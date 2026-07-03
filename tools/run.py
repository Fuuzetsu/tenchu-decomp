#!/usr/bin/env python3
"""Launch the decomp's disc image in pcsx-redux.

Boots the .bin/.cue that `./Build iso` produced — the original Tenchu disc with our
main.exe swapped in. We boot the whole disc (not `-loadexe main.exe`) because Tenchu
boots SLPS_019.01 -> ... -> TENCHU/MAIN.EXE, so jumping straight to MAIN.EXE would
skip the launcher. pcsx-redux falls back to OpenBIOS if no real BIOS is configured.

Finds pcsx-redux via $PCSX_REDUX, else `pcsx-redux` on PATH, else
~/programming/pcsx-redux/pcsx-redux. Extra pcsx-redux flags come from
$PCSX_REDUX_ARGS (e.g. `PCSX_REDUX_ARGS='-bios /path/scph.bin' ./Build run`) and
from any args after `--` when run directly.

Usually invoked as `./Build run`; can also be run directly:
  tools/run.py [path/to/tenchu.cue] [-- <extra pcsx-redux args>]
"""
import os
import shlex
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_CUE = os.path.join(ROOT, ".shake", "build", "tenchu", "tenchu.cue")
FALLBACK_PCSX = os.path.expanduser("~/programming/pcsx-redux/pcsx-redux")


def find_pcsx():
    env = os.environ.get("PCSX_REDUX")
    if env:
        if os.path.exists(env):
            return env
        sys.exit(f"run: $PCSX_REDUX={env} does not exist.")
    return shutil.which("pcsx-redux") or (
        FALLBACK_PCSX if os.path.exists(FALLBACK_PCSX) else sys.exit(
            "run: pcsx-redux not found. Set PCSX_REDUX=/path/to/pcsx-redux, put it "
            "on PATH, or build it at ~/programming/pcsx-redux."))


def main():
    args = sys.argv[1:]
    extra = shlex.split(os.environ.get("PCSX_REDUX_ARGS", ""))
    if "--" in args:
        i = args.index("--")
        args, extra = args[:i], extra + args[i + 1:]
    cue = args[0] if args else DEFAULT_CUE

    if not os.path.exists(cue):
        sys.exit(f"run: disc image not found: {cue}\n"
                 f"     Build it first with `./Build iso` (or just `./Build run`).")

    pcsx = find_pcsx()
    # -run: boot immediately; -iso: mount the disc. OpenBIOS is the auto BIOS fallback.
    cmd = [pcsx, "-run", "-iso", os.path.abspath(cue), *extra]
    print("run:", " ".join(cmd))
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == "__main__":
    main()
