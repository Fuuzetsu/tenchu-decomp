#!/usr/bin/env python3
"""Cookbook guards: generate the mechanised-rule index, and lint the cookbook.

Why this exists (docs/cookbook-audit.md §5.4): the cookbook regrew to ~8,000
unrouted lines chiefly by (a) keeping prose for rules that had been mechanised,
and (b) accumulating mechanical damage from scripted splices (duplicated
headings, duplicated bullets, double-numbered lists). Both classes are
machine-checkable, so they are checked by machine:

  tools/cookbooklint.py gen [--check]   (re)generate docs/autorules-index.md
                                        from autorules.py's own rule tables +
                                        rtlguide.py's signature hints;
                                        --check fails if the file is stale
  tools/cookbooklint.py lint            lint docs/matching-cookbook.md (and
                                        companions) for the regression classes

Lint rules (each exists because the failure occurred):
  L1 forbidden-phrase   "Now mechanized/mechanised" in the cookbook — the
                        generated index owns mechanisation status; prose that
                        says it is mechanised should have been deleted.
  L2 duplicate-heading  the same heading text appearing twice.
  L3 self-dup-heading   heading text duplicated ON one heading line.
  L4 duplicate-line     two identical consecutive non-blank lines.
  L5 numbered-list      a top-level ordered list that repeats or skips a
                        number (the audit found two "4."s; a "6." pair crept
                        in later).
  L6 unknown-rule-key   a backticked dash-name that looks like a rule key but
                        matches no autorules rule and no rtlguide signature —
                        stale references to renamed/removed rules.
  L7 ticket-tracking    every inline `TOOL TICKET (name)` must appear in
                        docs/orchestration.md's backlog.

Exit status: 0 clean, 2 findings (printed one per line, file:line prefixed).
"""

import argparse
import os
import re
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(TOOLS)
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

COOKBOOK = os.path.join(ROOT, "docs", "matching-cookbook.md")
INDEX = os.path.join(ROOT, "docs", "autorules-index.md")
ORCHESTRATION = os.path.join(ROOT, "docs", "orchestration.md")
COMPANIONS = [
    os.path.join(ROOT, "docs", "compiler-facts.md"),
    os.path.join(ROOT, "docs", "shape-zoo.md"),
]

FORBIDDEN = re.compile(r"now\s+mechani[sz]ed", re.IGNORECASE)
HEADING = re.compile(r"^(#{1,4})\s+(.*\S)\s*$")
ORDERED = re.compile(r"^(\d+)\.\s")
BACKTICKED = re.compile(r"`([^`\n]+)`")
KEYLIKE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)+$")
TICKET = re.compile(r"TOOL TICKET \(([A-Za-z0-9_.\- ]+)\)")

# Backticked dash-names that are legitimately not rule keys or signatures.
KEY_ALLOWLIST = {
    "check-all",        # ./Build check-all
    "wt-init",          # tools/wt-init.sh shorthand
    "one-shot",         # prose idiom, occasionally backticked
}


def _rule_tables():
    """autorules' own registry + rtlguide's signature hints, by import."""
    import autorules
    import rtlguide
    default = [(k, d) for k, d, _g in autorules.RULES]
    guided = [(k, d) for k, d, _g in autorules.AGGRESSIVE_RULES]
    hints = dict(getattr(rtlguide, "SIGNATURE_HINTS", {}))
    # Signatures rtlguide can REPORT but forgot to describe: surface them,
    # loudly, rather than silently omitting (tool contract: no silent gaps).
    src = open(os.path.join(TOOLS, "rtlguide.py"), encoding="utf-8").read()
    reported = set(re.findall(r'found\.append\("([a-z0-9-]+)"\)', src))
    for name in sorted(reported):
        hints.setdefault(name, "(reported by rtlguide; no hint text recorded)")
    return default, guided, hints


def generate_index():
    default, guided, hints = _rule_tables()
    out = []
    a = out.append
    a("# Mechanised-rule index (GENERATED — do not edit)")
    a("")
    a("Regenerate with `tools/cookbooklint.py gen`; CI checks freshness with")
    a("`tools/cookbooklint.py gen --check`. Sources of truth: the rule tables")
    a("in `tools/autorules.py` and `SIGNATURE_HINTS` in `tools/rtlguide.py`.")
    a("")
    a("The cookbook never re-explains a mechanised rule: if a key below covers")
    a("your residual, run the tool. `tools/autorules.py <Name>` sweeps the")
    a("default rules; `tools/rtlguide.py <Name>` then")
    a("`tools/autorules.py <Name> --guided` routes the guided/advanced ones.")
    a("")
    def cell(s):
        return str(s).replace("|", "\\|")

    a("## autorules — default rules")
    a("")
    a("| rule | does |")
    a("|---|---|")
    for k, d in default:
        a(f"| `{k}` | {cell(d)} |")
    a("")
    a("## autorules — guided/advanced rules (opt-in)")
    a("")
    a("| rule | does |")
    a("|---|---|")
    for k, d in guided:
        a(f"| `{k}` | {cell(d)} |")
    a("")
    a("## rtlguide — named residual signatures")
    a("")
    a("A signature names a residual SHAPE and the first levers to try; the")
    a("cookbook's router maps each to its family.")
    a("")
    a("| signature | meaning / first move |")
    a("|---|---|")
    for name in sorted(hints):
        text = " ".join(str(hints[name]).split())
        a(f"| `{name}` | {cell(text)} |")
    a("")
    return "\n".join(out)


def known_keys():
    default, guided, hints = _rule_tables()
    keys = {k for k, _ in default} | {k for k, _ in guided} | set(hints)
    # Tool names are legitimate backticked dash-tokens (`sched-deps`,
    # `split-scaffold`): anything shipping in tools/ is known.
    for entry in os.listdir(TOOLS):
        if entry.endswith(".py") or entry.endswith(".sh"):
            keys.add(entry.rsplit(".", 1)[0])
        else:
            keys.add(entry)
    return keys


def lint_text(name, text, keys, orchestration_text):
    findings = []
    lines = text.splitlines()
    headings = {}
    prev = None
    list_prev_num = None
    list_prev_indent = None
    in_code = False
    for i, line in enumerate(lines, 1):
        if line.lstrip().startswith("```"):
            in_code = not in_code
            prev = line
            continue
        if in_code:
            prev = line
            continue
        if FORBIDDEN.search(line):
            findings.append((name, i, "L1 forbidden-phrase: 'Now mechanised' "
                             "belongs to the generated index, not prose"))
        m = HEADING.match(line)
        if m:
            body = m.group(2)
            if body in headings:
                findings.append((name, i, "L2 duplicate-heading: also at line "
                                 f"{headings[body]}: {body!r}"))
            else:
                headings[body] = i
            half = len(body) // 2
            if half >= 8 and body[:half].strip() and \
                    body[:half].strip() == body[half:].strip():
                findings.append((name, i,
                                 f"L3 self-dup-heading: {body!r}"))
            list_prev_num = None
        stripped = line.strip()
        if stripped and stripped == (prev.strip() if prev else None) \
                and len(stripped) > 8 and not stripped.startswith("|"):
            findings.append((name, i, f"L4 duplicate-line: {stripped[:60]!r}"))
        om = ORDERED.match(stripped)
        indent = len(line) - len(line.lstrip())
        if om and indent == 0:
            n = int(om.group(1))
            if list_prev_num is not None and list_prev_indent == indent:
                if n != list_prev_num + 1 and n != 1:
                    findings.append((name, i,
                                     f"L5 numbered-list: {n} follows "
                                     f"{list_prev_num}"))
            list_prev_num = n
            list_prev_indent = indent
        if keys is not None:
            for tok in BACKTICKED.findall(line):
                if not KEYLIKE.match(tok):
                    continue
                if tok in keys or tok in KEY_ALLOWLIST:
                    continue
                findings.append((name, i, f"L6 unknown-rule-key: `{tok}` is "
                                 "neither an autorules rule nor an rtlguide "
                                 "signature"))
        for ticket in TICKET.findall(line):
            if orchestration_text is not None and \
                    ticket.split()[0].rstrip(":") not in orchestration_text:
                findings.append((name, i, f"L7 ticket-tracking: TOOL TICKET "
                                 f"({ticket}) not found in orchestration.md"))
        prev = line
    return findings


def run_lint(paths=None):
    keys = known_keys()
    orch = open(ORCHESTRATION, encoding="utf-8").read() \
        if os.path.exists(ORCHESTRATION) else ""
    findings = []
    for p in paths or [COOKBOOK] + COMPANIONS:
        text = open(p, encoding="utf-8").read()
        rel = os.path.relpath(p, ROOT)
        # Rule-key checking only applies to the cookbook + companions, which
        # must reference live rules; other docs may discuss history.
        findings += lint_text(rel, text, keys, orch)
    return findings


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    g = sub.add_parser("gen", help="write docs/autorules-index.md")
    g.add_argument("--check", action="store_true",
                   help="fail (exit 2) if the on-disk index is stale")
    g.add_argument("--out", default=INDEX)
    li = sub.add_parser("lint", help="lint the cookbook docs")
    li.add_argument("paths", nargs="*", help="override the default doc set")
    args = ap.parse_args()

    if args.cmd == "gen":
        text = generate_index()
        if args.check:
            on_disk = open(args.out, encoding="utf-8").read() \
                if os.path.exists(args.out) else ""
            if on_disk != text:
                print(f"STALE: {os.path.relpath(args.out, ROOT)} does not "
                      "match the rule tables — run tools/cookbooklint.py gen",
                      file=sys.stderr)
                return 2
            print(f"fresh: {os.path.relpath(args.out, ROOT)}")
            return 0
        with open(args.out, "w", encoding="utf-8") as fh:
            fh.write(text + "\n" if not text.endswith("\n") else text)
        print(f"wrote {os.path.relpath(args.out, ROOT)}")
        return 0

    findings = run_lint(args.paths or None)
    for name, line, msg in findings:
        print(f"{name}:{line}: {msg}")
    if findings:
        print(f"{len(findings)} finding(s)", file=sys.stderr)
        return 2
    print("clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
