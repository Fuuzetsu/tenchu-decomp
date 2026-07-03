#!/usr/bin/env python3
"""Build a coddog-readable ELF of the original main.exe text segment.

coddog (see decomp.yaml) wants an ELF whose function symbols are STT_FUNC with
real sizes — our build's ELF has NOTYPE/size-0 symbols (cc1 2.8 emits no
.type/.size), and our single-segment map lacks the vrom info its map reader
needs. So: synthesize an ELF from ground truth we already have — the ORIGINAL
binary's bytes plus the Ghidra export's function table (addr/size/name) —
with every function a properly typed, sized, section-relative symbol at its
true VRAM.

  tools/coddog-elf.py        -> .shake/build/tenchu/coddog.elf

Run inside the nix devShell (uses the cross as/ld). Regenerate whenever the
Ghidra export's function list changes.
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
OUT = ".shake/build/tenchu/coddog.elf"
WORK = ".shake/build/coddog"
CROSS = "mipsel-unknown-linux-gnu-"


def main():
    funcs = []
    for line in open(TSV):
        parts = line.rstrip("\n").split("\t")
        if len(parts) == 3:
            addr, size, name = int(parts[0], 16), int(parts[1]), parts[2]
            # skip Ghidra pseudo-labels (switch cases etc. — "foo::caseD_2")
            if not re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", name):
                continue
            if TEXT_START <= addr and addr + size <= TEXT_END and size >= 8:
                funcs.append((addr, size, name))
    funcs.sort()

    os.makedirs(WORK, exist_ok=True)
    data = open(ORIG, "rb").read()
    text = data[FILE_TEXT_OFF:FILE_TEXT_OFF + (TEXT_END - TEXT_START)]
    open(os.path.join(WORK, "text.bin"), "wb").write(text)

    # One .incbin slice per function (plus anonymous filler for the gaps) so
    # every symbol is section-relative with a real size.
    lines = ['.section .text', '.set noreorder']
    pos = 0
    for addr, size, name in funcs:
        off = addr - TEXT_START
        if off < pos:  # overlapping entry in the export; skip
            continue
        if off > pos:
            lines.append(f'.incbin "text.bin", {pos}, {off - pos}')
        lines.append(f'.global {name}')
        lines.append(f'.type {name}, @function')
        lines.append(f'{name}:')
        lines.append(f'.incbin "text.bin", {off}, {size}')
        lines.append(f'.size {name}, {size}')
        pos = off + size
    if pos < len(text):
        lines.append(f'.incbin "text.bin", {pos}, {len(text) - pos}')
    open(os.path.join(WORK, "coddog.s"), "w").write("\n".join(lines) + "\n")

    # Maintain a stems dir mirroring coddog's "unmatched = has a .s file" rule:
    # one empty <name>.s per function that does NOT have real (non-stub) C in
    # src/main.exe — decomp.yaml points asm/nonmatchings here so coddog's
    # "(decompiled)" tag means what it says.
    stems = os.path.join(WORK, "unmatched")
    os.makedirs(stems, exist_ok=True)
    matched = set()
    for f in os.listdir("src/main.exe"):
        if f.endswith(".c"):
            if not re.search(r"^\s*INCLUDE_ASM", open(os.path.join("src/main.exe", f)).read(), re.M):
                matched.add(f[:-2])
    for f in os.listdir(stems):
        os.unlink(os.path.join(stems, f))
    for _, _, name in funcs:
        if name not in matched:
            open(os.path.join(stems, name + ".s"), "w").close()

    subprocess.run([CROSS + "as", "-EL", "-march=r3000", "-o", "coddog.o",
                    "coddog.s"], cwd=WORK, check=True)
    subprocess.run([CROSS + "ld", "-EL", f"-Ttext={TEXT_START:#x}", "-e",
                    f"{TEXT_START:#x}", "-o",
                    os.path.join(ROOT, OUT), "coddog.o"], cwd=WORK, check=True)
    print(f"coddog-elf: wrote {OUT} ({len(funcs)} function symbols)")


if __name__ == "__main__":
    main()
