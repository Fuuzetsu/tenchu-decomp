#!/usr/bin/env python3
"""Stamp each src/main.exe/<Name>.c with the original source's own facts.

tools/matcher-prompt.py shows these at agent-launch time, but the facts belong
next to the code: a human reading a parked NON_MATCHING file, or an agent that
opens the `.c` without a prompt, should see what the authors actually wrote.

The block is a single C comment delimited by BEGIN/END PSX.SYM markers, inserted
after the last top-level #include and regenerated in place. It changes no bytes,
so `./Build check` stays byte-identical -- that is the gate.

  tools/symnote.py --write [--all | <Name> ...]
  tools/symnote.py --check          # non-zero if any block is missing/stale (CI)
  tools/symnote.py --params         # matched files whose parameter names differ
                                    # from the originals: a rename worklist

Everything comes from reference/psxsym-*, which tools/symdump.py regenerates from
disks/demo/PSX.SYM. See docs/psx-sym.md.
"""
from __future__ import annotations

import argparse, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(REPO)
SRC = "src/main.exe"
BEGIN = "/* BEGIN PSX.SYM"
END = "END PSX.SYM */"
BLOCK = re.compile(re.escape(BEGIN) + r".*?" + re.escape(END), re.S)
INCLUDE = re.compile(r"^\s*#\s*include\b.*$", re.M)


def safe(s: str) -> str:
    return s.replace("*/", "* /")


def load():
    protos, tu, locals_, cand = {}, {}, {}, {}
    p = "reference/psxsym-protos.h"
    if os.path.exists(p):
        for line in open(p):
            m = re.match(r"(?:static )?(.*?\b(\w+)\(.*?\);)\s*/\* (.*) \*/", line)
            if m:
                protos[m.group(2)] = (m.group(1), m.group(3), line.startswith("static"))
    p = "reference/psxsym-tu-map.tsv"
    if os.path.exists(p):
        for line in open(p):
            if line.startswith("#"):
                continue
            c = line.rstrip("\n").split("\t")
            if len(c) == 10:
                tu[c[9]] = dict(file=c[2], line=c[3], span=c[5], fsize=c[6],
                                mask=c[7], storage=c[8])
    p = "reference/psxsym-locals.tsv"
    if os.path.exists(p):
        for line in open(p):
            if line.startswith("#"):
                continue
            c = line.rstrip("\n").split("\t")
            if len(c) == 6:
                locals_.setdefault(c[0], []).append(c[2:])
    p = "reference/psxsym-candidates.tsv"
    if os.path.exists(p):
        for line in open(p):
            if line.startswith("#"):
                continue
            c = line.rstrip("\n").split("\t")
            if len(c) >= 6:
                cand.setdefault(c[1], []).append((c[2], c[4], c[5]))
    return protos, tu, locals_, cand


def globals_of(name):
    """Globals this function references, typed from PSX.SYM.

    Ownership is resolved against the DENSE symbol table (config/symbols), not
    against the sparse set of typed globals: a reference is attributed to the
    symbol that actually contains it. Attributing to the nearest preceding *typed*
    global instead lets an unrelated symbol two kilobytes earlier claim the
    reference -- that bug put `Projection` and `HumanGroup` in SetFrame's block,
    neither of which it touches.
    """
    try:
        import datamatch as D
    except Exception:
        return []
    hdr = "reference/psxsym-globals.h"
    if not os.path.exists(hdr):
        return []
    decl = {}
    for line in open(hdr):
        m = re.match(r"extern (.*?([A-Za-z_]\w*)(?:\[.*?\])*(?:\(\))?);\s+/\* (0x[0-9a-f]+)", line)
        if m:
            decl[int(m.group(3), 16)] = m.group(1)
    addr = size = None
    for line in open(".shake/ghidra-export/functions.tsv"):
        c = line.rstrip("\n").split("\t")
        if len(c) == 3 and c[2] == name:
            addr, size = int(c[0], 16), int(c[1])
            break
    if addr is None or not decl:
        return []
    import bisect
    dense = sorted(int(m.group(1), 16) for m in
                   (re.match(r"\s*[A-Za-z_]\w*\s*=\s*(0x[0-9A-Fa-f]+)\s*;", l)
                    for l in open("config/symbols.main.exe.txt")) if m)
    exe = open("disks/tenchu/main.exe", "rb").read()
    refs, _ = D.data_refs(exe[0x800:], 0x80011000, addr, size, D.RETAIL_GP, *D.RETAIL_TEXT)
    hits = []
    for _, y in refs:
        i = bisect.bisect_right(dense, y) - 1
        if i < 0:
            continue
        owner = dense[i]
        if owner not in decl or owner in hits:
            continue
        if y != owner:
            # An interior offset only makes sense for an aggregate. A scalar
            # (`short ConflictObjects;`) cannot own `owner + 0xa` -- that address
            # simply has no symbol, and attributing it here is how FUN_8001b2f4's
            # block claimed a global it never touches.
            d = decl[owner].strip()
            if "[" not in d and not d.startswith(("struct ", "union ")):
                continue
        hits.append(owner)
    return [f"extern {decl[a]};" for a in hits[:12]]


def render(name, protos, tu, locals_, cand) -> str | None:
    L = []
    if name in protos:
        proto, where, is_static = protos[name]
        t = tu.get(name, {})
        span = f", {t['span']} src lines" if t.get("span", "?") != "?" else ""
        L.append(f"{'static ' if is_static else ''}{proto}")
        L.append(f"    {where}{span}, frame {t.get('fsize','?')} bytes, "
                 f"saved-reg mask {t.get('mask','?')}")
    if name in locals_:
        L.append("")
        L.append("Original parameters and locals (the demo build's register allocation may")
        L.append("differ from retail, but the COUNT and TYPES drive cc1's codegen and carry")
        L.append("over). A repeated name is a nested-block scope, not a duplicate:")
        for kind, where, ty, vn in locals_[name]:
            L.append(f"    {kind:<5} {where:<9} {ty} {vn}")
    g = globals_of(name)
    if g:
        L.append("")
        L.append("Globals it touches, as the original declared them:")
        for d in g:
            L.append(f"    {d}")
    if name in cand:
        L.append("")
        for c, conf, ctu in cand[name]:
            L.append(f"PSX.SYM suggests this may be `{c}` ({conf} confidence, {ctu}) — NOT")
            L.append("adopted. Corroborate with `tools/callmatch.py --verify` before renaming.")
    if not L:
        return None
    body = "\n".join(" * " + safe(x) if x else " *" for x in L)
    return (f"{BEGIN} — the original source's own facts, from the demo disc's\n"
            f" * debug symbols. Regenerate with `tools/symnote.py --write`; see\n"
            f" * docs/psx-sym.md. Do not hand-edit.\n *\n{body}\n * {END}")


def stamp(path: str, block: str) -> bool:
    txt = open(path).read()
    if BLOCK.search(txt):
        new = BLOCK.sub(lambda _: block, txt, count=1)
    else:
        incs = list(INCLUDE.finditer(txt))
        at = incs[-1].end() if incs else 0
        rest = txt[at:].lstrip("\n")
        sep = "\n\n" if at else ""
        new = txt[:at] + sep + block + "\n\n" + rest
    if new == txt:
        return False
    open(path, "w").write(new)
    return True


def body_span(txt: str, name: str):
    """(sig_start, body_end) of our definition of `name`, brace-balanced."""
    m = re.search(rf"^[A-Za-z_][\w \t*]*\b{re.escape(name)}\s*\([^)]*\)\s*\{{", txt, re.M)
    if not m:
        return None
    i = txt.index("{", m.start())
    depth = 0
    for j in range(i, len(txt)):
        if txt[j] == "{":
            depth += 1
        elif txt[j] == "}":
            depth -= 1
            if depth == 0:
                return m.start(), j + 1
    return None


def rename_params(path: str, name: str, ours: list[str], theirs: list[str]):
    """Rename our parameters to the originals, inside the definition only.

    A parameter rename cannot change codegen, so `./Build check` is the gate. The
    guard here is capture: refuse if the new name already means something else in
    this file (comments and the PSX.SYM block excluded, since they mention it).
    """
    txt = open(path).read()
    span = body_span(txt, name)
    if span is None:
        return None, "no definition found"
    lo, hi = span
    stripped = re.sub(r"/\*.*?\*/", "", txt, flags=re.S)
    stripped = re.sub(r"//.*", "", stripped)
    live = set(re.findall(r"[A-Za-z_]\w*", stripped))
    clash = [n for o, n in zip(ours, theirs) if o != n and n in live]
    if clash:
        return None, f"would capture existing identifier(s): {', '.join(clash)}"
    body = txt[lo:hi]
    # two passes through unique placeholders so a->b, b->c cannot chain
    for k, (o, n) in enumerate(zip(ours, theirs)):
        if o != n:
            body = re.sub(rf"\b{re.escape(o)}\b", f"\x00{k}\x00", body)
    for k, (o, n) in enumerate(zip(ours, theirs)):
        if o != n:
            body = body.replace(f"\x00{k}\x00", n)
    open(path, "w").write(txt[:lo] + body + txt[hi:])
    return True, None


def c_params(path: str, name: str) -> list[str] | None:
    """Parameter names in our definition of `name`, in order."""
    txt = re.sub(r"/\*.*?\*/", "", open(path).read(), flags=re.S)
    m = re.search(rf"^[A-Za-z_][\w \t*]*\b{re.escape(name)}\s*\(([^)]*)\)\s*\{{",
                  txt, re.M)
    if not m:
        return None
    args = m.group(1).strip()
    if not args or args == "void":
        return []
    out = []
    for a in args.split(","):
        w = re.findall(r"[A-Za-z_]\w*", a)
        if w:
            out.append(w[-1])
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("names", nargs="*")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--params", action="store_true")
    ap.add_argument("--rename-params", action="store_true",
                    help="adopt the original parameter names (code only; gate on ./Build check)")
    args = ap.parse_args()

    protos, tu, locals_, cand = load()
    names = args.names or (
        [f[:-2] for f in sorted(os.listdir(SRC)) if f.endswith(".c")]
        if (args.all or args.check or args.params or args.rename_params) else [])
    if not names:
        ap.error("give function names, or --all / --check / --params / --rename-params")

    if args.params or args.rename_params:
        rows = []
        for n in names:
            path = f"{SRC}/{n}.c"
            if n not in protos or not os.path.exists(path):
                continue
            ours = c_params(path, n)
            if ours is None:
                continue
            theirs = [v[3] for v in locals_.get(n, []) if v[0] == "param"]
            if theirs and ours and len(ours) == len(theirs) and ours != theirs:
                rows.append((n, ours, theirs))
        print(f"{len(rows)} matched/drafted functions whose parameter names differ "
              f"from the originals:\n")
        done = skip = 0
        for n, ours, theirs in rows:
            print(f"  {n}")
            for a, b in zip(ours, theirs):
                if a != b:
                    print(f"      {a}  ->  {b}")
            if args.rename_params:
                ok, why = rename_params(f"{SRC}/{n}.c", n, ours, theirs)
                if ok:
                    done += 1
                else:
                    skip += 1
                    print(f"      SKIPPED: {why}")
        if args.rename_params:
            print(f"\nrenamed {done}, skipped {skip}. "
                  f"Now run ./Build check — a parameter rename must be byte-identical.")
            print("NOTE: only code was rewritten; a file's prose comment may still use "
                  "the old name.")
        return

    stale, wrote, skipped = [], 0, 0
    for n in names:
        path = f"{SRC}/{n}.c"
        if not os.path.exists(path):
            continue
        block = render(n, protos, tu, locals_, cand)
        if block is None:
            skipped += 1
            # a previous run may have written a block (e.g. globals that later
            # proved to be misattributed) -- drop it rather than leave it stale
            if args.write:
                txt = open(path).read()
                if BLOCK.search(txt):
                    open(path, "w").write(re.sub(r"\n*" + BLOCK.pattern + r"\n*", "\n\n",
                                                 txt, count=1, flags=re.S))
                    wrote += 1
            continue
        if args.check:
            cur = BLOCK.search(open(path).read())
            if not cur or cur.group(0) != block:
                stale.append(n)
        elif args.write:
            wrote += stamp(path, block)
    if args.check:
        print(f"symnote: {len(stale)} stale/missing blocks" +
              (": " + ", ".join(stale[:10]) if stale else ""))
        sys.exit(1 if stale else 0)
    print(f"symnote: stamped {wrote} files ({skipped} have no PSX.SYM facts).")


if __name__ == "__main__":
    main()
