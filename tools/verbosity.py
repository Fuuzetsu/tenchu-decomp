#!/usr/bin/env python3
"""Rank functions by how much LONGER our C is than the original source was.

PSX.SYM's TU map records each function's original source line span in the
DEMO build. Retail often grew a function, so comparing raw spans is
misleading (DrawGore: 640 demo bytes -> 2328 retail). Scaling the demo span
by the retail/demo SIZE ratio estimates the original retail source length:

    estimated_original_lines = demo_span * (retail_size / demo_size)

The tree's median ratio of ours-to-estimated is ~1.16, so an outlier at 3x
is a real signal - usually copy-paste where the original had a macro or a
loop. That is how the ReqItem* family's 29-line pasted pool-search preamble
was found (2026-08-31); item.h already had TAKE_ITEM_SLOT() for it.

  tools/verbosity.py            top 20 most verbose vs the estimate
  tools/verbosity.py -n 40      more rows
  tools/verbosity.py --compact  the other tail: ours much SHORTER than the
                                estimate, which can mean over-condensed
                                source (or simply that the demo build's
                                function was a different shape)

Calibration (2026-08-31): after collapsing the real structural repeats -
the ReqItem* pasted preamble and 183 two-line motion requests
(SET_MOTION) - the Act* family still sits near 2.5x. That residual is
STYLE, not structure: src/main.exe/.clang-format pins Allman braces and
AllowShortIfStatementsOnASingleLine=false, so a dense original line like
`if (pad & X) { SET_MOTION(A, 1); return; }` necessarily becomes three
statement lines here. Do not chase the ratio below ~2.5x on state
machines; look for repeated BLOCKS instead, which is what this tool is
good for.

Caveats: the demo span is an earlier build's, so treat a single outlier as
a lead, not proof; functions absent from PSX.SYM are skipped.
"""
import argparse, csv, glob, re, statistics, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)


def load():
    demo = {}
    with open("reference/psxsym-tu-map.tsv") as f:
        for row in csv.reader(f, delimiter="\t"):
            if len(row) >= 10 and row[0] and not row[0].startswith("#"):
                try:
                    demo[row[9]] = (int(row[1]), int(row[5]))
                except ValueError:
                    pass
    retail = {}
    with open("config/functions.main.exe.tsv") as f:
        for row in csv.reader(f, delimiter="\t"):
            if len(row) >= 3 and not row[0].startswith("#"):
                try:
                    retail[row[2]] = int(row[1])
                except ValueError:
                    pass
    return demo, retail


def statement_lines(path, name):
    code = re.sub(r"/\*.*?\*/", " ", open(path).read(), flags=re.S)
    code = re.sub(r"//[^\n]*", " ", code)
    i = code.find("\n%s(" % name)
    body = code[code.find("{", i):] if i >= 0 else code
    return len([l for l in body.split("\n")
                if re.search(r";|\b(if|else|while|for|switch|case|do|return|goto)\b", l)
                and not l.strip().startswith("extern")])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-n", type=int, default=20)
    ap.add_argument("--compact", action="store_true")
    args = ap.parse_args()
    demo, retail = load()
    rows = []
    for p in sorted(glob.glob("src/main.exe/*.c")):
        name = os.path.basename(p)[:-2]
        if name not in demo or name not in retail:
            continue
        dsz, span = demo[name]
        if span < 15 or dsz < 100:
            continue
        est = span * (retail[name] / dsz)
        ours = statement_lines(p, name)
        rows.append((ours / est, ours, round(est), name))
    if not rows:
        return
    med = statistics.median(r for r, _, _, _ in rows)
    print("median ours/estimated-original: %.2f  (n=%d)" % (med, len(rows)))
    rows.sort(reverse=not args.compact)
    print("%-6s %6s %10s  %s" % ("ratio", "ours", "estimated", "function"))
    for r, o, e, n in rows[:args.n]:
        print("%5.2fx %6d %10d  %s" % (r, o, e, n))


main()
