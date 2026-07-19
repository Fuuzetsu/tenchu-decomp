from __future__ import annotations

import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

from tools import pcsx_smoke as smoke


class PcsxSmokeTests(unittest.TestCase):
    def test_reads_current_entry_from_psx_exe_header(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "game.exe"
            header = bytearray(0x800)
            header[:8] = b"PS-X EXE"
            struct.pack_into("<I", header, 0x10, 0x80023450)
            path.write_bytes(header)
            self.assertEqual(smoke.read_psx_exe_entry(path), 0x80023450)

    def test_rejects_truncated_or_zero_entry_psx_exe(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "game.exe"
            path.write_bytes(b"PS-X EXE")
            with self.assertRaisesRegex(smoke.SmokeError, "complete PS-X EXE"):
                smoke.read_psx_exe_entry(path)

            header = bytearray(0x20)
            header[:8] = b"PS-X EXE"
            path.write_bytes(header)
            with self.assertRaisesRegex(smoke.SmokeError, "entry is zero"):
                smoke.read_psx_exe_entry(path)

    def test_disc_discovery_matches_documented_order(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root = base / "repo"
            home = base / "home"
            (root / "disks").mkdir(parents=True)
            (home / "tenchu-iso").mkdir(parents=True)
            later = root / "disks" / "z.cue"
            first = root / "disks" / "a.cue"
            home_cue = home / "tenchu-iso" / "game.cue"
            later.touch()
            first.touch()
            home_cue.touch()

            self.assertEqual(
                smoke.discover_cue(None, environ={}, root=root, home=home),
                first.resolve(),
            )
            self.assertEqual(
                smoke.discover_cue(
                    None,
                    environ={"TENCHU_CUE": str(home_cue)},
                    root=root,
                    home=home,
                ),
                home_cue.resolve(),
            )

    @mock.patch("tools.pcsx_smoke.shutil.which", return_value=None)
    def test_pcsx_discovery_uses_dynamic_home_fallback(self, which: mock.Mock) -> None:
        del which
        with tempfile.TemporaryDirectory() as directory:
            home = Path(directory)
            fallback = home / "programming" / "pcsx-redux" / "pcsx-redux"
            fallback.parent.mkdir(parents=True)
            fallback.touch()
            fallback.chmod(0o755)
            self.assertEqual(
                smoke.discover_pcsx(None, environ={}, home=home), fallback.resolve()
            )

    def test_resolves_symbols_from_the_selected_elf_not_retail_constants(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            exe = base / "growth.exe"
            elf = base / "growth.elf"
            header = bytearray(0x20)
            header[:8] = b"PS-X EXE"
            struct.pack_into("<I", header, 0x10, 0x80070260)
            exe.write_bytes(header)
            elf.touch()
            metadata = mock.Mock()
            metadata.symbol.side_effect = {"main": 0x800162A4, "PadProc": 0x8002ADAC}.__getitem__
            with mock.patch("tools.pcsx_smoke.psxexe.read_elf_metadata", return_value=metadata):
                addresses = smoke.resolve_probe_addresses(exe, elf)
            self.assertEqual(
                addresses,
                smoke.ProbeAddresses(0x80070260, 0x800162A4, 0x8002ADAC),
            )

    def test_command_is_headless_isolated_and_can_boot_disc_without_loadexe(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            direct = smoke.build_command(
                Path("/pcsx"), Path("/disc.cue"), Path("/game.exe"), temp
            )
            faithful = smoke.build_command(
                Path("/pcsx"), Path("/disc.cue"), Path("/game.exe"), temp,
                load_exe=False,
            )
            self.assertIn("-no-ui", direct)
            self.assertIn("-interpreter", direct)
            self.assertIn("-debugger", direct)
            self.assertIn("-softgpu", direct)
            self.assertIn("-loadexe", direct)
            self.assertNotIn("-loadexe", faithful)
            self.assertIn(str(temp / "memcard1.mcd"), direct)
            self.assertIn(str(temp / "memcard2.mcd"), direct)

    @mock.patch("tools.pcsx_smoke.run_process")
    @mock.patch("tools.pcsx_smoke.shutil.which", return_value="/nix/store/mkpsxiso")
    def test_repack_uses_mkiso_with_explicit_original_cue(
        self, which: mock.Mock, run_process: mock.Mock
    ) -> None:
        del which
        run_process.return_value = smoke.ProcessResult(0, "ok\n")
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            cue, result = smoke.package_disc(
                Path("/growth.exe"), Path("/original.cue"), temp,
                environ={"A": "B"}, timeout=12,
            )
        self.assertEqual(result.returncode, 0)
        self.assertEqual(cue.name, "tenchu-smoke.cue")
        command = run_process.call_args.args[0]
        self.assertIn(str(smoke.ROOT / "tools" / "mkiso.py"), command)
        self.assertIn("/growth.exe", command)
        self.assertEqual(
            run_process.call_args.kwargs["environ"]["TENCHU_CUE"], "/original.cue"
        )

    @unittest.skipUnless(os.name == "posix", "process-group timeout is POSIX-specific")
    def test_timeout_terminates_the_emulator_process_group(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            result = smoke.run_process(
                [sys.executable, "-c", "import time; time.sleep(30)"],
                cwd=Path(directory), environ=os.environ, timeout=0.05,
            )
        self.assertTrue(result.timed_out)
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
