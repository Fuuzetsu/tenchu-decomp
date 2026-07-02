#!/usr/bin/env python3
"""Build a bootable Tenchu CD image (.bin/.cue) with the decomp's main.exe.

Rebuilds the original disc with mkpsxiso, pointing TENCHU/MAIN.EXE at our build:
  * same-size main.exe (matching build or a same-size mod): the forced-LBA layout
    reproduces the original disc byte-for-byte except main.exe's sectors — it
    boots exactly like the original;
  * grown main.exe (from `./Build mod`): falls back to auto-LBA layout (files
    after main.exe shift). Fine for file-name-based access; streaming assets that
    hardcode LBAs (some movies/XA) may glitch — the game logic still runs.

You must provide the ORIGINAL disc (it's copyrighted). Point TENCHU_CUE at its
.cue, or drop the .cue (+ .bin) under disks/ or ~/tenchu-iso/. The disc is dumped
once (via dumpsxiso) into TENCHU_ISO_WORK (default .shake/iso, ~700MB, cached).

Run in the nix devShell (needs mkpsxiso + dumpsxiso). Load the resulting .cue in
pcsx-redux.
"""
import argparse, glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
WORK = os.environ.get("TENCHU_ISO_WORK", os.path.join(ROOT, ".shake", "iso"))
BUILD = os.path.join(ROOT, ".shake", "build", "tenchu")


def find_cue():
    env = os.environ.get("TENCHU_CUE")
    if env:
        if os.path.exists(env):
            return env
        sys.exit(f"mkiso: TENCHU_CUE={env} not found")
    for d in (os.path.join(ROOT, "disks"), os.path.expanduser("~/tenchu-iso")):
        cues = sorted(glob.glob(os.path.join(d, "*.cue")))
        if cues:
            return cues[0]
    sys.exit("mkiso: original disc .cue not found. Set TENCHU_CUE=/path/to/game.cue\n"
             "       (the original disc is copyrighted — you provide it).")


def ensure_dump(cue):
    xml = os.path.join(WORK, "tenchu.xml")
    disc_main = os.path.join(WORK, "extract", "TENCHU", "MAIN.EXE")
    if not (os.path.exists(xml) and os.path.exists(disc_main)):
        os.makedirs(WORK, exist_ok=True)
        print(f"mkiso: dumping original disc to {WORK} (one-time, ~700MB)…", flush=True)
        subprocess.run(["dumpsxiso", "-l", "-x", os.path.join(WORK, "extract"),
                        "-s", xml, cue], check=True)
    return xml, disc_main


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=os.path.join(BUILD, "main.exe"),
                    help="main.exe to put on the disc (default: the matching build)")
    ap.add_argument("-o", "--out", default=os.path.join(BUILD, "tenchu"),
                    help="output prefix; writes <prefix>.bin and <prefix>.cue")
    args = ap.parse_args()
    exe = os.path.abspath(args.exe)
    if not os.path.exists(exe):
        sys.exit(f"mkiso: {args.exe} missing — run ./Build (or ./Build mod) first")

    xml_path, disc_main = ensure_dump(find_cue())
    orig_size = os.path.getsize(disc_main)
    new_size = os.path.getsize(exe)

    # Point MAIN.EXE at our build (leave the dumped extract pristine).
    xml = open(xml_path).read()
    xml, n = re.subn(r'(<file name="MAIN\.EXE"[^>]*?\bsource=")[^"]*(")',
                     lambda m: m.group(1) + exe + m.group(2), xml)
    if n != 1:
        sys.exit(f"mkiso: expected exactly one MAIN.EXE entry in the XML, found {n}")

    if new_size > orig_size:
        # A grown main.exe can't fit its forced slot — drop all forced LBAs so
        # mkpsxiso auto-packs (files after main.exe shift).
        xml = re.sub(r'\s+offs="\d+"', '', xml)
        print(f"mkiso: main.exe grew ({new_size} > {orig_size}) — using auto-LBA "
              "layout (streaming assets that hardcode LBAs may glitch).")

    build_xml = os.path.join(WORK, "tenchu.build.xml")
    open(build_xml, "w").write(xml)

    out_bin, out_cue = args.out + ".bin", args.out + ".cue"
    os.makedirs(os.path.dirname(out_bin), exist_ok=True)
    subprocess.run(["mkpsxiso", "-y", "-o", out_bin, "-c", out_cue, build_xml], check=True)
    print(f"\nmkiso: wrote {out_bin}\n       {out_cue}")
    print("Load the .cue in pcsx-redux (drag it in, or File > Open Disk Image).")


if __name__ == "__main__":
    main()
