#!/usr/bin/env python3
"""Exercise TRIAL's new-mission flow and terrain cursor in PCSX-Redux.

Boot the selected TRIAL.EXE directly, choose Create, accept the default
settings, pass the memory-card check, and move the map cursor right and left.
Require the observed cursor positions, sustained editor input polling, and
the expected initial player placement. Cards and the executable snapshot live
in a temporary directory. With --save, save the new mission through the editor
and compare its on-card payload with game memory. This does not cover disc
handoff, overwriting/deleting a mission, or playtest.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile

try:
    from tools import pcsx_smoke
except ImportError:
    import pcsx_smoke  # type: ignore[no-redef]


ROOT = Path(__file__).resolve().parent.parent
PROBE = Path(__file__).with_name("pcsx-trial.lua")
PROBE_SYMBOLS = {
    "MAIN": "main",
    "MENU": "ShowTrialMainMenu",
    "SETTINGS": "EditTrialMissionSettings",
    "CARD": "UpdateTrialCardCheck",
    "EDITOR": "EditTrialMission",
    "MAP": "EditTrialTerrain",
    "READPAD": "ReadTrialMenuPad",
    "REALPAD": "GetRealPad",
    "COMMANDS": "UpdateTrialEditorCommands",
    "EDITOR_SAVE": "SaveTrialEditorMission",
    "CARD_SAVE": "UpdateTrialCardSave",
}


def verify_saved_mission(card_path: Path, mission_path: Path) -> str:
    """Check the single-block file created on this probe's temporary card."""
    card = card_path.read_bytes()
    mission = mission_path.read_bytes()
    if len(card) != 128 * 1024 or card[:2] != b"MC" or len(mission) != 0x1d3c:
        raise ValueError("invalid temporary card or mission snapshot")
    checksum, = struct.unpack_from("<I", mission)
    filename = f"BISLPS-01901M{checksum:07x}".encode("ascii")
    entries = [(i, card[i * 128:(i + 1) * 128]) for i in range(1, 16)
               if card[i * 128] == 0x51]
    if len(entries) != 1:
        raise ValueError(f"expected one saved mission, found {len(entries)} files")
    index, entry = entries[0]
    size, next_block = struct.unpack_from("<IH", entry, 4)
    if entry[10:30].split(b"\0")[0] != filename or size != 8192 or next_block != 0xffff:
        raise ValueError("saved mission directory entry does not match its checksum")
    parity = 0
    for value in entry[:127]:
        parity ^= value
    if parity != entry[127]:
        raise ValueError("saved mission directory checksum is invalid")
    block = card[index * 8192:(index + 1) * 8192]
    if block[:4] != b"SC\x13\x01" or block[512:512 + len(mission)] != mission:
        raise ValueError("saved mission payload differs from game memory")
    return filename.decode("ascii")


def run() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=ROOT / ".shake/build/tenchu/trial.exe")
    parser.add_argument("--elf", type=Path, default=ROOT / ".shake/build/tenchu/trial.exe.elf")
    parser.add_argument("--cue", type=Path)
    parser.add_argument("--pcsx", type=Path)
    parser.add_argument("--bios", type=Path)
    parser.add_argument("--timeout", type=float, default=150)
    parser.add_argument("--save", action="store_true",
                        help="save through the editor and verify the temporary card file")
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    try:
        exe = pcsx_smoke._existing_file(args.exe, "TRIAL executable")
        elf = pcsx_smoke._existing_file(args.elf, "TRIAL ELF")
        pcsx = pcsx_smoke.discover_pcsx(args.pcsx)
        cue = pcsx_smoke.discover_cue(args.cue)
        bios = pcsx_smoke._existing_file(args.bios, "BIOS") if args.bios else None
        metadata = pcsx_smoke.psxexe.read_elf_metadata(elf)
        environment = os.environ.copy()
        for key, symbol in PROBE_SYMBOLS.items():
            address = metadata.symbol(symbol)
            environment[f"TENCHU_TRIAL_{key}"] = str(address)
            environment[f"TENCHU_TRIAL_{key}_WORD"] = str(
                pcsx_smoke.read_exe_word(exe, address)
            )
        # The inventory supplies the complete exact-build function span;
        # GetRealPad calls outside the terrain editor cannot satisfy the gate.
        rows = (ROOT / "config/functions.trial.exe.tsv").read_text().splitlines()
        sizes = {
            name: int(size)
            for line in rows if line and not line.startswith("#")
            for _, size, name in [line.split("\t")]
        }
        environment["TENCHU_TRIAL_MAP_END"] = str(
            metadata.symbol("EditTrialTerrain") + sizes["EditTrialTerrain"]
        )
        # This sprite is still owned by the unrecovered terrain graphics code.
        environment["TENCHU_TRIAL_CURSOR"] = str(metadata.symbol("D_800CC6E8"))
        environment["TENCHU_TRIAL_MISSION"] = str(metadata.symbol("TrialMission"))
        environment["TENCHU_TRIAL_SAVE"] = "1" if args.save else "0"
        environment["TENCHU_TRIAL_SAVE_STATE"] = str(metadata.symbol("TrialCardSaveState"))
        environment["TENCHU_TRIAL_COMMAND_SELECTION"] = str(metadata.symbol("TrialEditorCommandSelection"))
        saved_file = None
        with tempfile.TemporaryDirectory(prefix="tenchu-trial-editor-") as directory:
            temp = Path(directory)
            mission_snapshot = temp / "saved-mission.bin"
            environment["TENCHU_TRIAL_SNAPSHOT"] = str(mission_snapshot)
            snapshot = temp / "trial.exe"
            shutil.copyfile(exe, snapshot)
            command = pcsx_smoke.build_command(
                pcsx, cue, snapshot, temp, bios=bios, lua_probe=PROBE
            )
            result = pcsx_smoke.run_process(
                command, cwd=temp, environ=environment, timeout=args.timeout
            )
            if args.save and not result.timed_out and result.returncode == 0:
                saved_file = verify_saved_mission(temp / "memcard1.mcd", mission_snapshot)
    except (OSError, KeyError, ValueError, pcsx_smoke.SmokeError,
            pcsx_smoke.psxexe.PsxExeError) as error:
        print(f"pcsx-trial: {error}", file=sys.stderr)
        return 2
    for line in result.output.splitlines():
        if "TENCHU_TRIAL" in line:
            print(line)
    if saved_file:
        print(f"TENCHU_TRIAL verified card file={saved_file} payload=7484 bytes")
    if result.timed_out:
        print(f"pcsx-trial: timed out after {args.timeout:g} seconds", file=sys.stderr)
        return 124
    if (result.returncode != 0 or "TENCHU_TRIAL FAIL " in result.output
            or "TENCHU_TRIAL PASS " not in result.output):
        print(result.output, file=sys.stderr)
        print(f"pcsx-trial: probe failed (emulator status {result.returncode})", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
