#!/usr/bin/env python3
"""Rewrite PS1 GTE command opcodes in splat's asm as `.word`, so `as` can build it.

Our `mipsel-...-as` (binutils, `-march=r3000`) is vanilla MIPS: the COP2 *data
moves* (`lwc2`/`swc2`/`mfc2`/`mtc2`/`cfc2`/`ctc2`) assemble, but the GTE *command*
opcodes (`RTPS`/`RTPT`/`NCLIP`/`DPCS`/`DPCT`/`MVMVA`/…) are `unrecognized opcode`.
splat emits them as mnemonics, so every renderer function's split `.s` failed to
assemble — which is why the whole GTE region was un-splittable, and a function
there could not even be carved as a NON_MATCHING stub.

spimdisasm prints a GTE command's mnemonic in UPPERCASE and every ordinary
instruction in lowercase, which is the detector. We rewrite

    /* 4CA54 8005D254 3000284A */  RTPT
into
    /* 4CA54 8005D254 3000284A */  .word 0x4A280030  /* RTPT */

**The word is NOT the comment column.** That column is the raw little-endian file
bytes in address order (`jr $ra` = 0x03E00008 prints as `0800E003`), while `as`
lays a `.word` down little-endian. So the emitted value is the BYTE-SWAP of the
comment; copying it verbatim would reverse every instruction. Verified against
the original image: RTPT's comment `3000284A` -> `.word 0x4A280030` -> bytes
`30 00 28 4A`.

Idempotent: a line already rewritten is left alone. Byte-exact by construction,
and `./Build check` is the arbiter.

  tools/gte2word.py <dir-or-file>...     rewrite in place, report what changed
  tools/gte2word.py --check <dir>        exit 1 if any GTE mnemonic remains
"""
import argparse, os, re, sys

# Every GTE command opcode spimdisasm can print. Anything uppercase that is NOT
# in here is a surprise we want to hear about rather than silently rewrite.
GTE_CMDS = {
    "RTPS", "RTPT", "NCLIP", "AVSZ3", "AVSZ4", "MVMVA", "SQR", "OP",
    "DPCS", "DPCT", "INTPL", "NCS", "NCT", "NCDS", "NCDT", "NCCS", "NCCT",
    "CDP", "CC", "NCLIP", "GPF", "GPL", "DCPL",
}

# /* <fileoff> <vram> <WORD> */  MNEMONIC [operands]
LINE = re.compile(
    r"^(?P<pre>/\* [0-9A-Fa-f]+ [0-9A-Fa-f]{8} (?P<word>[0-9A-Fa-f]{8}) \*/\s+)"
    r"(?P<mn>[A-Z][A-Za-z0-9]*)(?P<rest>\s*.*)$"
)


def swap32(hexword: str) -> int:
    """The comment column is raw LE bytes in address order -> the real word."""
    return int.from_bytes(bytes.fromhex(hexword), "little")


def rewrite(text):
    out, n, unknown = [], 0, set()
    for line in text.splitlines(keepends=True):
        m = LINE.match(line)
        if not m:
            out.append(line)
            continue
        mn = m.group("mn")
        if mn not in GTE_CMDS:
            unknown.add(mn)
            out.append(line)
            continue
        word = swap32(m.group("word"))
        rest = m.group("rest").strip()
        note = f"{mn} {rest}".strip()
        out.append(f"{m.group('pre')}.word 0x{word:08X}  /* {note} */\n")
        n += 1
    return "".join(out), n, unknown


def files(paths):
    for p in paths:
        if os.path.isfile(p):
            yield p
        else:
            for root, _, names in os.walk(p):
                for f in names:
                    if f.endswith(".s"):
                        yield os.path.join(root, f)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--check", action="store_true",
                    help="don't write; exit 1 if a GTE mnemonic remains")
    args = ap.parse_args()

    total, touched, unknown = 0, 0, set()
    for path in files(args.paths):
        src = open(path).read()
        new, n, unk = rewrite(src)
        unknown |= unk
        if n:
            total += n
            touched += 1
            if not args.check:
                open(path, "w").write(new)
    if unknown:
        print(f"gte2word: WARNING unrecognised uppercase mnemonics: "
              f"{sorted(unknown)}", file=sys.stderr)
    if args.check:
        if total:
            print(f"gte2word: {total} GTE mnemonic(s) still present", file=sys.stderr)
            return 1
        return 0
    if total:
        print(f"gte2word: rewrote {total} GTE opcode(s) as .word in {touched} file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
