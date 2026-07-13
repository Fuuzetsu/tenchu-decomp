#!/usr/bin/env python3
"""Turn an assembly residual plus our RTL into a mechanical next action.

The target executable has no RTL.  Its assembly is the specification; our
compiler's RTL is the causal trace.  rtlguide aligns the target and candidate
instructions, classifies each residual by the cc1 pass that owns it, recompiles
the candidate with debug NOTE objects, and maps the mismatching instructions
back to concrete C lines and final hard-register locals.

This replaces the usual manual loop of `asmdiff -> guess a pass -> rtldump ->
grep source lines -> regalloc` with one reproducible report.  The report is also
consumed by `autorules.py --guided`, which restricts its search to the relevant
cookbook transformations and can try byte-neutral pure-C enabling edits with a
beam.  It also calls out target-only physical calls and summarizes the
candidate's CALL_INSN result/argument fingerprints: jump2 can distinguish calls
whose final machine instructions look identical.

    tools/rtlguide.py <Name>              build draft + full guided report
    tools/rtlguide.py <Name> --no-build   reuse the current .shake build
    tools/rtlguide.py <Name> --no-rtl     assembly classification only
    tools/rtlguide.py <Name> --json       machine-readable report

Run inside the nix devShell.  The RTL/debug compilation is standalone under
/tmp and never races Shake; only the optional initial ./Build touches .shake.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import difflib
import json
import os
import re
import sys

import asmdiff
from matchlock import MatchToolBusy, matching_tool_lock
import matchdiff
import regalloc
import rtldump

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

REG_ORDER = [
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
]
REGNUM = {name: i for i, name in enumerate(REG_ORDER)}
REGNUM["s8"] = 30
REG_RE = re.compile(
    r"(?<![\w$])\$?(zero|at|v[01]|a[0-3]|t[0-9]|s[0-8]|k[01]|gp|sp|fp|ra)(?!\w)"
)
NUM_RE = re.compile(r"(?<!\w)-?(?:0x[0-9a-f]+|\d+)(?!\w)", re.I)
BRANCH = {
    "b", "beq", "beqz", "bne", "bnez", "bgez", "bgtz", "blez", "bltz",
    "bgezal", "bltzal", "j", "jal", "jalr", "jr",
}
CONDITIONAL_BRANCH = BRANCH - {"b", "j", "jal", "jalr", "jr"}
MOVEISH = {"move", "movn", "movz"}
COMBINE_OPS = {
    "add", "addu", "addiu", "sub", "subu", "and", "andi", "or", "ori",
    "xor", "xori", "nor", "sll", "sllv", "sra", "srav", "srl", "srlv",
    "slt", "slti", "sltu", "sltiu", "mult", "multu", "div", "divu",
    "mfhi", "mflo", "negu", "not",
}

CATEGORY_PASSES = {
    "regalloc": ["lreg", "greg"],
    "cse/coalescing": ["rtl", "cse", "lreg"],
    "jump/cross-jump": ["jump", "jump2"],
    "schedule/delay": ["sched2", "dbr"],
    "combine/expression": ["rtl", "combine"],
    "structure/length": ["rtl", "jump", "jump2"],
}
CATEGORY_RULES = {
    "regalloc": [
        "type-width", "cmp-polarity", "loop-fence", "nested-loop-fence", "paired-loop-fence", "loop-range", "split-chain", "shift-stage", "ptr-base-split",
        "or-inplace", "add-prefix-temp", "flag-arm-assign",
        "identical-arm-fence", "subscript-postinc", "switch-cse-evict",
        "call-arg-pair", "eq-literal-swap", "adjacent-field-store-swap", "assignment-chain",
    ],
    "cse/coalescing": [
        "type-width", "loop-fence", "nested-loop-fence", "paired-loop-fence", "loop-range", "temp-inline", "shift-stage", "ptr-base-split",
        "vector-copy-adjust", "subscript-postinc", "switch-cse-evict", "assignment-chain",
        "pointee-volatile",
    ],
    "jump/cross-jump": ["case-fence", "sparse-eq-switch", "mul-affine-shape", "and-nest", "if-else-invert"],
    "schedule/delay": [
        "type-width", "loop-fence", "nested-loop-fence", "paired-loop-fence", "loop-range", "cmp-swap", "cmp-polarity", "shift-stage", "ptr-base-split",
        "split-chain", "or-inplace", "vector-copy-adjust", "flag-arm-assign",
        "loop-boundary-shift", "identical-arm-fence", "subscript-postinc",
        "call-arg-pair", "eq-literal-swap", "pointee-volatile",
        "adjacent-field-store-swap", "assignment-chain",
    ],
    "combine/expression": [
        "abs-ge", "builtin-abs", "cmp-swap", "cmp-polarity", "min-ternary", "ptr-index-sum",
        "shift16-mul", "plus-group", "add-prefix-temp", "split-chain",
    ],
    "structure/length": [
        "type-width", "and-nest", "temp-inline", "case-fence",
        "vector-copy-adjust", "builtin-abs", "subscript-postinc",
        "call-arg-pair", "if-else-invert",
    ],
}

CALL_OPS = {"jal", "jalr"}
LOAD_OPS = {"lb", "lbu", "lh", "lhu", "lw", "lwl", "lwr"}
STORE_OPS = {"sb", "sh", "sw", "swl", "swr"}

SIGNATURE_HINTS = {
    "adjacent-independent-load-order": (
        "do not assume independence: compare .combine -> .sched -> .sched2, "
        "then inspect LOG_LINKS and nearby LOOP_END notes; try boundary-shift "
        "or an identical-arm fence before parking"
    ),
    "copy-then-inplace-adjust": (
        "target stores a stack field before overwriting it with an adjusted value; "
        "try explicit fieldwise copy followed by an in-place +=/-= adjustment"
    ),
    "post-comparison-flag-definition": (
        "target reuses a comparison register for 0/1 branch-delay definitions; "
        "assign the flag at the end of each arm, after that arm's comparisons"
    ),
    "path-result-copy-join": (
        "target preserves a path-produced value through two distinct pseudos "
        "before testing it, while the candidate coalesces the second copy; "
        "split the path result from the final flag at the CFG join, then inspect "
        ".lreg/.greg before trying a bounded liveness fence"
    ),
    "enclosing-global-field-load": (
        "target keeps distinct base/value registers for a global load while "
        "the candidate folds both into one; query one or more scalar names/addresses with "
        "tools/symnear.py and try the proven nonzero field of an enclosing global"
    ),
    "guard-return-island-layout": (
        "target uses one conditional branch into/out of a shared return island, "
        "while the candidate emits the inverse guard plus nop and skip jump; "
        "try if-else-invert or one explicit shared return label once, then park "
        "if RTL-guided body-layout trials are flat"
    ),
    "builtin-abs-inline": (
        "candidate calls abs but target has bgez/negu inline; spell the site "
        "__builtin_abs(...) because the matching cc1 receives -fno-builtin"
    ),
    "postincrement-working-copy": (
        "target increments through a distinct working register and copies it "
        "back; try merging the adjacent increment into array[index++]"
    ),
    "call-result-argument-pipeline": (
        "target transforms the first result across the second call and uses "
        "that call's delay slot; inline the adjacent producer pair into their "
        "common consumer arguments"
    ),
    "commutative-equality-register-order": (
        "target and candidate differ only by load/literal homes and reversed "
        "beq/bne operands; try eq-literal-swap once, then treat a flat result "
        "as a bounded global-allocation tie"
    ),
}


def mnemonic(insn: str) -> str:
    return insn.strip().split(None, 1)[0] if insn.strip() else ""


def call_count(insns) -> int:
    """Count physical calls in an `(address, asm)` instruction stream."""
    return sum(mnemonic(insn) in CALL_OPS for _addr, insn in insns)


def control_flow_counts(insns) -> dict[str, int]:
    """Physical control-flow inventory for structural-candidate auditing."""
    operations = [mnemonic(insn) for _addr, insn in insns]
    return {
        "conditional_branches": sum(op in CONDITIONAL_BRANCH for op in operations),
        "unconditional_jumps": sum(op in {"b", "j"} for op in operations),
        "calls": sum(op in CALL_OPS for op in operations),
        "returns": sum(op == "jr" for op in operations),
    }


def stack_address_rematerialization_hints(target, candidate) -> list[dict]:
    """Find target stack addresses that the candidate retains instead.

    Repeated ``addiu reg,sp,K`` sites in the target but only one site in the
    candidate are a strong sign that cc1 kept ``&local`` in a saved-register
    allocno across calls or a loop.  That can enlarge the frame and rotate all
    saved registers even when the logical C is otherwise correct.
    """
    pattern = re.compile(
        r"^addiu\s+([a-z0-9]+),sp,(-?(?:0x[0-9a-f]+|[0-9]+))$", re.I
    )

    def sites(insns):
        found = defaultdict(list)
        for _address, text in insns:
            match = pattern.match(text.strip())
            if not match or match.group(1) == "sp":
                continue
            found[int(match.group(2), 0)].append(match.group(1))
        return found

    wanted = sites(target)
    actual = sites(candidate)
    hints = []
    for offset, target_regs in wanted.items():
        candidate_regs = actual.get(offset, [])
        if len(target_regs) < 2 or len(candidate_regs) >= len(target_regs):
            continue
        retained = sorted({reg for reg in candidate_regs
                           if re.fullmatch(r"s[0-7]|s8|fp", reg)})
        hints.append(dict(
            offset=offset,
            target_sites=len(target_regs),
            candidate_sites=len(candidate_regs),
            candidate_saved_registers=retained,
        ))
    return sorted(
        hints,
        key=lambda item: (
            -(item["target_sites"] - item["candidate_sites"]),
            item["offset"],
        ),
    )


def call_symbol(insn: str) -> str | None:
    """Best-effort callee name from objdump's `jal ... <symbol>` spelling."""
    m = re.search(r"<([^>+]+)(?:\+0x[0-9a-f]+)?>", insn, re.I)
    return m.group(1) if m else None


def registers(insn: str) -> list[str]:
    return [m.group(1) for m in REG_RE.finditer(insn)]


def shape(insn: str, numbers: bool = False) -> str:
    """Instruction shape used for alignment.

    Register names are deliberately erased: a whole register rotation should
    align as a run of tiny differences, not one giant replace block.  Branch
    destinations are also erased because address drift is not a source-shape
    difference.  `numbers` additionally masks constants for standalone-object
    (unrelocated) -> linked-image alignment.
    """
    s = re.sub(r"\s*<[^>]+>", "", insn.lower())
    s = REG_RE.sub("R", s)
    op = mnemonic(s)
    if op in BRANCH:
        parts = s.rsplit(",", 1)
        if len(parts) == 2:
            s = parts[0] + ",ADDR"
        elif op not in {"jr", "jalr"}:
            bits = s.split(None, 1)
            s = bits[0] + (" ADDR" if len(bits) > 1 else "")
    if numbers:
        s = NUM_RE.sub("#", s)
    return re.sub(r"\s+", " ", s).strip()


def _paired_alignment(target, ours):
    """Yield `(target_index|None, ours_index|None)` in structural order."""
    tkeys = [shape(x[1]) for x in target]
    okeys = [shape(x[1]) for x in ours]
    sm = difflib.SequenceMatcher(None, tkeys, okeys, autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            yield from zip(range(i1, i2), range(j1, j2))
            continue
        common = min(i2 - i1, j2 - j1)
        for k in range(common):
            yield i1 + k, j1 + k
        for i in range(i1 + common, i2):
            yield i, None
        for j in range(j1 + common, j2):
            yield None, j


def _different_pair(t, o) -> bool:
    if t is None or o is None:
        return True
    # Keep branch-target-only drift out of the diagnosis.  It is a consequence
    # of an earlier shape/length difference and asmdiff hides it by default too.
    if mnemonic(t) in BRANCH and shape(t) == shape(o) and registers(t) == registers(o):
        return False
    return t != o


def diff_hunks(target, ours):
    """Group adjacent differing aligned pairs into compact diagnostic hunks."""
    pairs = list(_paired_alignment(target, ours))
    bad = []
    for pos, (ti, oi) in enumerate(pairs):
        ts = target[ti][1] if ti is not None else None
        os_ = ours[oi][1] if oi is not None else None
        if _different_pair(ts, os_):
            bad.append((pos, ti, oi))
    groups = []
    for item in bad:
        if not groups or item[0] > groups[-1][-1][0] + 1:
            groups.append([item])
        else:
            groups[-1].append(item)
    # SequenceMatcher represents `A; branch` -> `branch; A` as an insertion and
    # a deletion separated by the equal branch.  It does the same for a combine
    # expression moved across a short expansion (`addiu A,25; rem` versus
    # `rem; addiu B,25`). Join complementary one-sided groups when they are
    # adjacent, or when their register-erased shapes prove that the same short
    # instruction run was relocated. This lets the classifier see the complete
    # reordered expression instead of two false structure/length hunks.
    merged = []
    for group in groups:
        if merged:
            left_t = any(x[1] is not None for x in merged[-1])
            left_o = any(x[2] is not None for x in merged[-1])
            right_t = any(x[1] is not None for x in group)
            right_o = any(x[2] is not None for x in group)
            complementary = ((left_t and not left_o and right_o and not right_t)
                             or (left_o and not left_t and right_t and not right_o))
            gap = group[0][0] - merged[-1][-1][0]
            left_shapes = [shape(target[ti][1]) for _p, ti, _oi in merged[-1]
                           if ti is not None]
            left_shapes += [shape(ours[oi][1]) for _p, _ti, oi in merged[-1]
                            if oi is not None]
            right_shapes = [shape(target[ti][1]) for _p, ti, _oi in group
                            if ti is not None]
            right_shapes += [shape(ours[oi][1]) for _p, _ti, oi in group
                             if oi is not None]
            relocated = (gap <= 32 and len(left_shapes) <= 2 and
                         Counter(left_shapes) == Counter(right_shapes))
            if complementary and (gap <= 2 or relocated):
                start, end = merged[-1][0][0], group[-1][0]
                merged[-1] = [(p, ti, oi) for p, (ti, oi) in
                              enumerate(pairs[start:end + 1], start)]
                continue
        merged.append(group)
    groups = merged
    out = []
    for group in groups:
        tis = [x[1] for x in group if x[1] is not None]
        ois = [x[2] for x in group if x[2] is not None]
        out.append(dict(
            target_indices=tis,
            ours_indices=ois,
            target=[target[i] for i in tis],
            ours=[ours[i] for i in ois],
        ))
    return out, pairs


def classify_hunk(hunk):
    t = [x[1] for x in hunk["target"]]
    o = [x[1] for x in hunk["ours"]]
    tops, oops = [mnemonic(x) for x in t], [mnemonic(x) for x in o]
    same_shape = len(t) == len(o) and all(shape(a) == shape(b) for a, b in zip(t, o))
    if same_shape and any(registers(a) != registers(b) for a, b in zip(t, o)):
        return "regalloc"
    # Compare full instructions, not only mnemonics: two independent `lh`s
    # swapped around each other have identical mnemonic sequences.
    if Counter(t) == Counter(o) and t != o:
        return "schedule/delay"
    if (sum(x in MOVEISH for x in tops) != sum(x in MOVEISH for x in oops)
            or tops.count("nop") != oops.count("nop")):
        return "cse/coalescing"
    if (len(t) != len(o) and
            (any(x in BRANCH for x in tops + oops) or tops.count("jr") != oops.count("jr"))):
        return "jump/cross-jump"
    if len(t) != len(o):
        return "structure/length"
    if any(x in BRANCH for x in tops + oops):
        return "jump/cross-jump"
    if any(x in COMBINE_OPS for x in tops + oops):
        return "combine/expression"
    return "structure/length"


def _store_parts(insn):
    """(op, value-reg, displacement, base-reg) for a simple store."""
    m = re.match(
        r"^(s[bhw])\s+\$?([a-z0-9]+),\s*"
        r"(-?(?:0x[0-9a-f]+|\d+))\(\$?([a-z0-9]+)\)$",
        insn.strip(), re.I,
    )
    if not m:
        return None
    return m.group(1).lower(), m.group(2).lower(), m.group(3).lower(), m.group(4).lower()


def _copy_then_adjust(target, ours):
    """Detect target `store x; x += C; store x` missing from the candidate.

    The narrow stack-store form is the assembly fingerprint of a fieldwise
    aggregate copy followed by an in-place adjustment.  It deliberately does
    not diagnose arbitrary repeated stores through unknown pointers.
    """
    ours_by_addr = {addr: insn for addr, insn in ours}
    for i, (start_addr, first_insn) in enumerate(target):
        first = _store_parts(first_insn)
        if first is None or first[3] not in {"sp", "fp", "s8"}:
            continue
        op, value, displacement, base = first
        for j in range(i + 1, min(i + 5, len(target))):
            arithmetic = target[j][1]
            regs = registers(arithmetic)
            if (mnemonic(arithmetic) not in {"add", "addu", "addiu", "sub", "subu"}
                    or len(regs) < 2 or regs[0] != value or regs[1] != value):
                continue
            for k in range(j + 1, min(j + 4, len(target))):
                second = _store_parts(target[k][1])
                if second != (op, value, displacement, base):
                    continue
                candidate_stores = 0
                for addr, insn in ours_by_addr.items():
                    if start_addr <= addr <= target[k][0]:
                        parts = _store_parts(insn)
                        if parts and (parts[0], parts[2], parts[3]) == \
                                (op, displacement, base):
                            candidate_stores += 1
                if candidate_stores < 2:
                    return True
    return False


def _post_comparison_flag(target, ours):
    """Detect a target condition register reused for 0/1 flag definitions."""
    ours_by_addr = {addr: insn for addr, insn in ours}
    for i in range(1, len(target)):
        addr, target_zero = target[i]
        ours_zero = ours_by_addr.get(addr, "")
        tz, oz = registers(target_zero), registers(ours_zero)
        if (mnemonic(target_zero) != mnemonic(ours_zero) or
                mnemonic(target_zero) != "move" or len(tz) < 2 or len(oz) < 2 or
                tz[1] != oz[1] or tz[1] != "zero" or tz[0] == oz[0]):
            continue
        target_reg, candidate_reg = tz[0], oz[0]
        if (mnemonic(target[i - 1][1]) not in BRANCH or
                target_reg not in registers(target[i - 1][1])):
            continue
        saw_one = saw_use = False
        for addr2, target_insn in target[i + 1:min(i + 12, len(target))]:
            ours_insn = ours_by_addr.get(addr2, "")
            tr, or_ = registers(target_insn), registers(ours_insn)
            if (mnemonic(target_insn) == mnemonic(ours_insn) == "li" and
                    tr[:1] == [target_reg] and or_[:1] == [candidate_reg] and
                    re.search(r"(?:^|,)\s*(?:0x0*1|1)$", target_insn)):
                saw_one = True
            if (saw_one and mnemonic(target_insn) in {"beqz", "bnez"} and
                    mnemonic(target_insn) == mnemonic(ours_insn) and
                    tr[:1] == [target_reg] and or_[:1] == [candidate_reg]):
                saw_use = True
                break
        if saw_one and saw_use:
            return True
    return False


def _builtin_abs_gap(target, ours):
    """Candidate calls ``abs`` while the target expands bgez/copy/negu."""
    candidate_calls_abs = any(
        mnemonic(insn) in CALL_OPS and (call_symbol(insn) or "").split(".")[0] == "abs"
        for _addr, insn in ours
    )
    target_calls_abs = any(
        mnemonic(insn) in CALL_OPS and (call_symbol(insn) or "").split(".")[0] == "abs"
        for _addr, insn in target
    )
    if not candidate_calls_abs or target_calls_abs:
        return False
    for i, (_addr, branch) in enumerate(target):
        branch_regs = registers(branch)
        if mnemonic(branch) != "bgez" or not branch_regs:
            continue
        live = {branch_regs[0]}
        for _addr2, insn in target[i + 1:min(i + 5, len(target))]:
            regs = registers(insn)
            if mnemonic(insn) == "move" and len(regs) >= 2 and regs[1] in live:
                live.add(regs[0])
            if (mnemonic(insn) == "negu" and len(regs) >= 2 and
                    (regs[0] in live or regs[1] in live)):
                return True
    return False


def _postincrement_working_copy(target, ours):
    """Target ``addiu tmp,index,1; move index,tmp`` vs in-place candidate."""
    def addiu_one(insn):
        return (mnemonic(insn) == "addiu" and
                re.search(r",\s*(?:0x0*1|1)$", insn.strip(), re.I))

    ours_inplace = []
    for insn in ours:
        regs = registers(insn)
        if addiu_one(insn) and len(regs) >= 2 and regs[0] == regs[1]:
            ours_inplace.append(regs[0])
    if not ours_inplace:
        return False
    for i, insn in enumerate(target):
        regs = registers(insn)
        if not addiu_one(insn) or len(regs) < 2 or regs[0] == regs[1]:
            continue
        working, index = regs[:2]
        if index not in ours_inplace:
            continue
        for later in target[i + 1:min(i + 4, len(target))]:
            move_regs = registers(later)
            if (mnemonic(later) == "move" and len(move_regs) >= 2 and
                    move_regs[:2] == [index, working]):
                return True
    return False


def _path_result_copy_join(target, ours):
    """Target ``move A,B; move B,A; branch B`` vs coalesced candidate.

    The apparently redundant round trip is the final assembly trace of two
    source pseudos meeting at a control-flow join.  A candidate that has only
    ``move A,B; branch A`` has combined the path result with the final flag.
    Keep this deliberately narrow: both copies must be exact reversals and the
    consumer must be a zero-test branch.
    """
    def roundtrip(stream):
        for i in range(len(stream) - 2):
            first = stream[i][1]
            second = stream[i + 1][1]
            branch = stream[i + 2][1]
            first_regs = registers(first)
            second_regs = registers(second)
            branch_regs = registers(branch)
            if (mnemonic(first) == mnemonic(second) == "move" and
                    len(first_regs) >= 2 and len(second_regs) >= 2 and
                    second_regs[:2] == list(reversed(first_regs[:2])) and
                    mnemonic(branch) in {"beqz", "bnez"} and
                    branch_regs[:1] == second_regs[:1]):
                return tuple(first_regs[:2])
        return None

    pair = roundtrip(target)
    if pair is None or roundtrip(ours) is not None:
        return False
    destination, source = pair
    for i in range(len(ours) - 1):
        move_regs = registers(ours[i][1])
        if mnemonic(ours[i][1]) != "move" or tuple(move_regs[:2]) != pair:
            continue
        for _addr, consumer in ours[i + 1:min(i + 4, len(ours))]:
            consumer_regs = registers(consumer)
            if (mnemonic(consumer) in {"beqz", "bnez"} and
                    consumer_regs[:1] == [destination]):
                return True
            if (mnemonic(consumer) == "move" and
                    consumer_regs[:2] == [source, destination]):
                break
    return False


def _enclosing_global_field_load(target, ours):
    """Target split-base global load vs candidate same-register scalar load."""
    if len(target) != 2 or len(ours) != 2:
        return False
    target_high, target_load = target
    ours_high, ours_load = ours
    target_high_regs = registers(target_high)
    target_load_regs = registers(target_load)
    ours_high_regs = registers(ours_high)
    ours_load_regs = registers(ours_load)
    return (
        mnemonic(target_high) == mnemonic(ours_high) == "lui" and
        mnemonic(target_load) == mnemonic(ours_load) and
        mnemonic(target_load) in LOAD_OPS and
        shape(target_high) == shape(ours_high) and
        shape(target_load) == shape(ours_load) and
        len(target_high_regs) >= 1 and len(target_load_regs) >= 2 and
        len(ours_high_regs) >= 1 and len(ours_load_regs) >= 2 and
        target_load_regs[1] == target_high_regs[0] and
        target_load_regs[0] != target_high_regs[0] and
        ours_load_regs[0] == ours_load_regs[1] == ours_high_regs[0]
    )


def _call_result_argument_pipeline(target, ours):
    """Target keeps first call result across a second call via its delay slot."""
    def has_pipeline(stream):
        for i, (_addr, first_call) in enumerate(stream):
            if mnemonic(first_call) not in CALL_OPS:
                continue
            for j in range(i + 1, min(i + 5, len(stream))):
                transform = stream[j][1]
                regs = registers(transform)
                if (mnemonic(transform) != "andi" or len(regs) < 2 or
                        regs[1] != "v0"):
                    continue
                saved = regs[0]
                for k in range(j + 1, min(j + 4, len(stream) - 1)):
                    if mnemonic(stream[k][1]) not in CALL_OPS:
                        continue
                    delay = stream[k + 1][1]
                    delay_regs = registers(delay)
                    if (mnemonic(delay) == "sll" and len(delay_regs) >= 2 and
                            delay_regs[0] == saved and delay_regs[1] == saved):
                        return True
        return False

    return has_pipeline(target) and not has_pipeline(ours)


def known_residual_signatures(hunks, target_stream=None, ours_stream=None):
    """Name narrow, previously root-caused residual shapes.

    These are early-stop hints, not automatic proof: the report still directs
    the matcher to confirm the owning RTL pass and bounded source experiments.
    """
    found = []
    if target_stream is not None and ours_stream is not None:
        if _copy_then_adjust(target_stream, ours_stream):
            found.append("copy-then-inplace-adjust")
        if _post_comparison_flag(target_stream, ours_stream):
            found.append("post-comparison-flag-definition")
        if _path_result_copy_join(target_stream, ours_stream):
            found.append("path-result-copy-join")
        if _builtin_abs_gap(target_stream, ours_stream):
            found.append("builtin-abs-inline")
        if _call_result_argument_pipeline(target_stream, ours_stream):
            found.append("call-result-argument-pipeline")
    for hunk in hunks:
        target = [x[1] for x in hunk["target"]]
        ours = [x[1] for x in hunk["ours"]]
        if (_postincrement_working_copy(target, ours) and
                "postincrement-working-copy" not in found):
            found.append("postincrement-working-copy")
        if (_enclosing_global_field_load(target, ours) and
                "enclosing-global-field-load" not in found):
            found.append("enclosing-global-field-load")

        # A one-branch target versus inverse-branch/nop/skip-jump candidate is
        # the compact signature of a guard body laid out on the wrong physical
        # side of a shared return island.  Address drift is deliberately
        # ignored; only the branch polarity, tested registers, and nop+j shape
        # matter.
        inverse = {
            "beq": "bne", "bne": "beq", "beqz": "bnez", "bnez": "beqz",
            "bgez": "bltz", "bltz": "bgez", "bgtz": "blez", "blez": "bgtz",
        }
        if len(target) == 1 and len(ours) == 3:
            target_op = mnemonic(target[0])
            ours_op = mnemonic(ours[0])
            if (inverse.get(target_op) == ours_op and
                    registers(target[0]) == registers(ours[0]) and
                    mnemonic(ours[1]) == "nop" and mnemonic(ours[2]) == "j" and
                    "guard-return-island-layout" not in found):
                found.append("guard-return-island-layout")

        # One memory load and one literal feed a commutative equality branch,
        # with their $v0/$v1 homes exchanged as a pair. This is the exact
        # ActJUMP terminal residual; source operand swapping is the only local
        # semantic equivalence worth one bounded trial.
        if len(target) == len(ours) == 3:
            t0, t1, t2 = target
            o0, o1, o2 = ours
            tr0, tr1, tr2 = registers(t0), registers(t1), registers(t2)
            or0, or1, or2 = registers(o0), registers(o1), registers(o2)
            if (mnemonic(t0) == mnemonic(o0) and mnemonic(t0) in LOAD_OPS and
                    shape(t0) == shape(o0) and
                    mnemonic(t1) == mnemonic(o1) == "li" and
                    NUM_RE.findall(t1) == NUM_RE.findall(o1) and
                    mnemonic(t2) == mnemonic(o2) and
                    mnemonic(t2) in {"beq", "bne"} and
                    len(tr0) >= 1 and len(or0) >= 1 and
                    len(tr1) >= 1 and len(or1) >= 1 and
                    len(tr2) >= 2 and len(or2) >= 2 and
                    tr0[0] == or1[0] and tr1[0] == or0[0] and
                    tr2[:2] == list(reversed(or2[:2])) and
                    "commutative-equality-register-order" not in found):
                found.append("commutative-equality-register-order")

        # Cancel the aligned instructions common to both sides. A lone addiu
        # left on each side, with identical immediate/self-add shape but at a
        # different position, is the characteristic gcc fold reassociation
        # residual (`A + C + B` versus `A + (B + C)`).
        remaining_ours = Counter(ours)
        target_only = []
        for insn in target:
            if remaining_ours[insn]:
                remaining_ours[insn] -= 1
            else:
                target_only.append(insn)
        remaining_target = Counter(target)
        ours_only = []
        for insn in ours:
            if remaining_target[insn]:
                remaining_target[insn] -= 1
            else:
                ours_only.append(insn)
        if len(target_only) == len(ours_only) == 1:
            ti, oi = target_only[0], ours_only[0]
            tr, or_ = registers(ti), registers(oi)
            if (mnemonic(ti) == mnemonic(oi) == "addiu" and
                    shape(ti) == shape(oi) and
                    len(tr) == len(or_) == 2 and
                    tr[0] == tr[1] and or_[0] == or_[1] and
                    NUM_RE.findall(ti) == NUM_RE.findall(oi) and
                    target.index(ti) != ours.index(oi)):
                if "constant-add-reassociation" not in found:
                    found.append("constant-add-reassociation")

        if (len(target) == len(ours) == 2 and
                target == list(reversed(ours)) and
                all(mnemonic(x) in LOAD_OPS for x in target)):
            if "adjacent-independent-load-order" not in found:
                found.append("adjacent-independent-load-order")

        if len(target) != 2 or len(ours) != 2:
            continue
        if (mnemonic(target[0]) != "addu" or mnemonic(ours[0]) != "addu" or
                mnemonic(target[1]) not in STORE_OPS or
                mnemonic(ours[1]) != mnemonic(target[1])):
            continue
        tr, or_ = registers(target[0]), registers(ours[0])
        ts, os_ = registers(target[1]), registers(ours[1])
        if (len(tr) >= 3 and len(or_) >= 3 and ts and os_ and
                tr[0] == tr[1] == or_[2] and
                or_[0] == or_[1] == tr[2] and
                ts[0] == tr[0] and os_[0] == or_[0] and
                shape(target[1]) == shape(ours[1])):
            if "commutative-plus-destination" not in found:
                found.append("commutative-plus-destination")
    return found


def _candidate_asm(name):
    addr, size = matchdiff.symbol_slot(name)
    linked = matchdiff.linked_text_size(name)
    target = asmdiff.dis(asmdiff.ORIG, addr, size)
    ours_size = linked if linked is not None else size
    ours = asmdiff.dis(asmdiff.OURS, addr, max(ours_size, 4))
    return addr, size, ours_size, target, ours


def assembly_guide(name):
    """Classify the existing build; fast entry point used by autorules."""
    addr, size, ours_size, target, ours = _candidate_asm(name)
    hunks, pairs = diff_hunks(target, ours)
    counts = Counter()
    missing_moves = Counter()
    target_only_calls = Counter()
    substitutions = Counter()
    for h in hunks:
        category = classify_hunk(h)
        h["category"] = category
        counts[category] += 1
        t_moves = [x[1] for x in h["target"] if mnemonic(x[1]) == "move"]
        o_moves = [x[1] for x in h["ours"] if mnemonic(x[1]) == "move"]
        if len(t_moves) > len(o_moves):
            for insn in t_moves:
                regs = registers(insn)
                if regs:
                    missing_moves[regs[0]] += 1
        t_calls = Counter(
            call_symbol(x[1]) or "?" for x in h["target"]
            if mnemonic(x[1]) in CALL_OPS
        )
        o_calls = Counter(
            call_symbol(x[1]) or "?" for x in h["ours"]
            if mnemonic(x[1]) in CALL_OPS
        )
        target_only_calls.update(t_calls - o_calls)
    for ti, oi in pairs:
        if ti is None or oi is None:
            continue
        t, o = target[ti][1], ours[oi][1]
        if shape(t) != shape(o) or mnemonic(t) != mnemonic(o):
            continue
        tr, or_ = registers(t), registers(o)
        for want, have in zip(tr, or_):
            if want != have:
                substitutions[(have, want)] += 1
    # A target-only move or compact register-only residual is useful evidence
    # about the equality/copy chain to inspect in .lreg/.greg.  It is strictly
    # diagnostic: the matching tool never manufactures inline-asm clobbers.
    if counts.get("regalloc"):
        for (_have, want), _n in substitutions.most_common(2):
            if want not in {"zero", "sp", "gp", "fp", "s8", "ra"}:
                missing_moves[want] += 1
    if ours_size != size:
        counts["structure/length"] += 1
    primary = counts.most_common(1)[0][0] if counts else "matched"
    rules = []
    for category, _ in counts.most_common():
        for rule in CATEGORY_RULES.get(category, []):
            if rule not in rules:
                rules.append(rule)
    return dict(
        name=name, address=addr, target_bytes=size, ours_bytes=ours_size,
        target_instructions=len(target), ours_instructions=len(ours),
        primary=primary, categories=dict(counts), rules=rules,
        passes=CATEGORY_PASSES.get(primary, []),
        missing_move_registers=[r for r, _ in missing_moves.most_common(2)],
        target_only_calls=[
            dict(callee=callee, count=count)
            for callee, count in target_only_calls.most_common()
        ],
        call_tail_hint=call_count(target) > call_count(ours),
        call_counts=dict(target=call_count(target), candidate=call_count(ours)),
        control_flow_counts=dict(
            target=control_flow_counts(target),
            candidate=control_flow_counts(ours),
        ),
        stack_address_rematerializations=
            stack_address_rematerialization_hints(target, ours),
        known_residual_signatures=known_residual_signatures(hunks, target, ours),
        register_substitutions=[
            dict(ours=a, target=b, count=n)
            for (a, b), n in substitutions.most_common()
        ],
        hunks=hunks, target=target, ours=ours,
    )


def parse_line_object(path, name):
    """Instructions from `objdump -drSl`, each carrying its current C line."""
    out = []
    current_line = None
    in_func = False
    label = re.compile(r"^[0-9a-f]+\s+<([^>]+)>:$", re.I)
    src_line = re.compile(r"\.c:(\d+)(?:\s|$)")
    insn = re.compile(r"^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+(.+)$", re.I)
    for raw in open(path, errors="replace"):
        s = raw.rstrip("\n")
        m = label.match(s)
        if m:
            # objdump prints every internal LM/$L label in the same `<...>:`
            # form as a function symbol.  Once the requested function starts,
            # keep consuming; structural alignment naturally ignores any later
            # helper function in the rare multi-function source file.
            if m.group(1) == name:
                in_func = True
                current_line = None
            continue
        if not in_func:
            continue
        m = src_line.search(s)
        if m:
            current_line = int(m.group(1))
            continue
        m = insn.match(s)
        if m:
            text = re.sub(r"\s+", " ", m.group(3).replace("\t", " ")).strip()
            out.append(dict(offset=int(m.group(1), 16), asm=text, line=current_line))
    return out


def map_source_lines(line_insns, ours):
    """Map linked candidate instruction indices to C lines via standalone .o."""
    a = [shape(x["asm"], numbers=True) for x in line_insns]
    b = [shape(x[1], numbers=True) for x in ours]
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    mapped = {}
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            for i, j in zip(range(i1, i2), range(j1, j2)):
                mapped[j] = line_insns[i]["line"]
        elif tag == "replace":
            # A register/immediate normalization corner can still leave an
            # equal-length replacement.  Positional mapping is more useful
            # than dropping every line in that small region.
            for i, j in zip(range(i1, i2), range(j1, j2)):
                mapped[j] = line_insns[i]["line"]
    return mapped


def parse_debug_variables(path, name):
    """Final hard-register homes emitted by cc1's stabs `.def` records."""
    by_reg = defaultdict(list)
    in_func = False
    definition = re.compile(
        r"\.def\s+([A-Za-z_]\w*);\s*\.val\s+(\d+);\s*\.scl\s+(4|17);"
    )
    for raw in open(path, errors="replace"):
        s = raw.strip()
        if s == name + ":":
            in_func = True
            continue
        if in_func and re.search(rf"\.end\s+{re.escape(name)}\b", s):
            break
        if not in_func:
            continue
        m = definition.search(s)
        if not m:
            continue
        num = int(m.group(2))
        if num in regalloc.REGNAME and m.group(1) not in by_reg[regalloc.rn(num)]:
            by_reg[regalloc.rn(num)].append(m.group(1))
    return dict(by_reg)


def _dump_suffix(path):
    return path.rsplit(".", 1)[-1]


def _rtl_function(text, name=None):
    """Restrict a dump to one `;; Function NAME` section when available."""
    if not name:
        return text
    start = re.search(rf"^;; Function\s+{re.escape(name)}(?:\s|$)", text, re.M)
    if not start:
        return text
    end = re.search(r"^;; Function\s+", text[start.end():], re.M)
    stop = start.end() + end.start() if end else len(text)
    return text[start.start():stop]


def pass_stats(paths, name=None):
    stats = {}
    for path in paths:
        suffix = _dump_suffix(path)
        with open(path, errors="replace") as stream:
            text = _rtl_function(stream.read(), name)
        stats[suffix] = dict(
            insns=len(re.findall(r"^\((?:insn|jump_insn|call_insn)\b", text, re.M)),
            calls=len(re.findall(r"^\(call_insn\b", text, re.M)),
            labels=len(re.findall(r"^\(code_label\b", text, re.M)),
            moves=len(re.findall(r"\{mov\w*", text)),
            source_notes=len(re.findall(r'^\(note .*\(".*\.c"\) \d+\)', text, re.M)),
        )
    return stats


def loop_boundary_source_lines(paths, name=None):
    """Source lines whose first RTL insn follows NOTE_INSN_LOOP_END.

    gcc-2.8.1 sched turns that first post-loop instruction into a full pending
    register/memory fence.  This makes an apparent adjacent-load reorder a real
    dependency rather than a scheduler tie (DefaultActionHumanoid).
    """
    result = {}
    source_note = re.compile(r'^\(note .*\(".*\.c"\)\s+(\d+)\)')
    real_insn = re.compile(r"^\((?:insn|jump_insn|call_insn)\b")
    for path in paths:
        suffix = _dump_suffix(path)
        if suffix not in {"sched", "sched2"}:
            continue
        with open(path, errors="replace") as stream:
            text = _rtl_function(stream.read(), name)
        pending = False
        current_line = None
        found = set()
        for raw in text.splitlines():
            if "NOTE_INSN_LOOP_END" in raw:
                pending = True
                current_line = None
                continue
            if not pending:
                continue
            note = source_note.match(raw)
            if note:
                current_line = int(note.group(1))
                continue
            if real_insn.match(raw):
                if current_line is not None:
                    found.add(current_line)
                pending = False
                current_line = None
        if found:
            result[suffix] = sorted(found)
    return result


def _rtl_forms(text, kind):
    """Yield balanced top-level RTL forms beginning with `(<kind>`."""
    for match in re.finditer(rf"^\({re.escape(kind)}\b", text, re.M):
        depth = 0
        quoted = False
        escaped = False
        for end in range(match.start(), len(text)):
            ch = text[end]
            if quoted:
                if escaped:
                    escaped = False
                elif ch == "\\":
                    escaped = True
                elif ch == '"':
                    quoted = False
                continue
            if ch == '"':
                quoted = True
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    yield text[match.start():end + 1]
                    break


def parse_call_rtl(path, name=None):
    """Summarize CALL_INSN result mode and hard-register usage expressions.

    jump.c compares more than the eventual `jal`: a typed result and the
    trailing CALL_INSN_FUNCTION_USAGE expr_list are part of the RTL identity.
    These compact fingerprints expose precisely that otherwise invisible
    distinction without pretending that we possess RTL for the target.
    """
    with open(path, errors="replace") as stream:
        text = _rtl_function(stream.read(), name)
    calls = []
    for block in _rtl_forms(text, "call_insn"):
        callee = re.search(r'\(symbol_ref(?::\w+)?\s+\("([^"]+)"\)\)', block)
        result = re.search(
            r"\(set\s+\(reg(?::([A-Z0-9]+))?\s+\d+(?:\s+[^)]*)?\)\s+\(call\b",
            block, re.S,
        )
        # The final expr_list is CALL_INSN_FUNCTION_USAGE.  Restrict uses to
        # that tail so clobbers/uses embedded in the call pattern are not mixed
        # into the caller-visible argument signature.
        usage_at = block.rfind("\n    (expr_list")
        usage = block[usage_at:] if usage_at >= 0 else ""
        uses = []
        for mode, num, name in re.findall(
                r"\(use\s+\(reg(?::([A-Z0-9]+))?\s+(\d+)(?:\s+([^)\s]+))?\)\)",
                usage):
            if name:
                reg = name
            elif int(num) < len(REG_ORDER):
                reg = REG_ORDER[int(num)]
            else:
                reg = "r" + num
            uses.append(f"{mode or '?'}:${reg}")
        calls.append(dict(
            callee=callee.group(1) if callee else "<indirect>",
            result=result.group(1) if result else "void",
            uses=uses,
        ))
    return calls


def call_rtl_evidence(paths, target_only, name=None):
    """Fingerprint candidate calls implicated by target-only physical calls."""
    by_suffix = {_dump_suffix(path): path for path in paths}
    initial_path = by_suffix.get("rtl")
    final_path = by_suffix.get("jump2") or by_suffix.get("jump")
    if not initial_path:
        return []
    initial = parse_call_rtl(initial_path, name)
    final = parse_call_rtl(final_path, name) if final_path else initial
    wanted = {x["callee"] for x in target_only if x["callee"] != "?"}
    evidence = []
    for callee in sorted(wanted):
        before = [x for x in initial if x["callee"] == callee]
        after = [x for x in final if x["callee"] == callee]
        variants = Counter((x["result"], tuple(x["uses"])) for x in before)
        evidence.append(dict(
            callee=callee,
            expanded_sites=len(before),
            after_jump_sites=len(after),
            fingerprints=[
                dict(result=result, uses=list(uses), count=count)
                for (result, uses), count in variants.most_common()
            ],
        ))
    return evidence


def _attach_lines(guide, mapped):
    all_lines = set()
    for h in guide["hunks"]:
        lines = {mapped[i] for i in h["ours_indices"] if mapped.get(i) is not None}
        if not lines and h["ours_indices"]:
            # Nearest mapped neighbour gives target-only/deleted instructions a
            # useful insertion site rather than no source location at all.
            pivot = h["ours_indices"][0]
            neighbours = sorted(mapped, key=lambda x: abs(x - pivot))
            if neighbours and mapped[neighbours[0]] is not None:
                lines.add(mapped[neighbours[0]])
        h["source_lines"] = sorted(lines)
        all_lines.update(lines)
    guide["source_lines"] = sorted(all_lines)


def _byte_counts(addr, size):
    off = matchdiff.FILE_TEXT_OFF + (addr - matchdiff.TEXT_START)
    orig = open(matchdiff.ORIG, "rb").read()
    ours = open(matchdiff.OURS, "rb").read()
    local = sum(a != b for a, b in zip(orig[off:off + size], ours[off:off + size]))
    total = sum(a != b for a, b in zip(orig, ours)) + abs(len(orig) - len(ours))
    return local, total


def build_candidate(name):
    path = os.path.join("src", "main.exe", name + ".c")
    env = dict(os.environ)
    if os.path.exists(path) and "ifndef NON_MATCHING" in open(path).read():
        env["NON_MATCHING"] = name
    r = matchdiff.run_build(env)
    if r.returncode:
        raise RuntimeError(f"./Build failed (rc={r.returncode}):\n{matchdiff.build_log_tail()}")


def diagnose(name, build=True, rtl=True):
    if build:
        build_candidate(name)
    guide = assembly_guide(name)
    local, total = _byte_counts(guide["address"], guide["target_bytes"])
    guide.update(local_differing_bytes=local, whole_image_differing_bytes=total)
    if not rtl or guide["primary"] == "matched":
        guide["source_lines"] = []
        guide["variables_by_register"] = {}
        guide["pass_stats"] = {}
        guide["call_rtl_evidence"] = []
        guide["loop_boundary_source_lines"] = {}
        return guide

    src = os.path.join("src", "main.exe", name + ".c")
    draft = os.path.exists(src) and "ifndef NON_MATCHING" in open(src).read()
    result = rtldump.compile_rtl(name, ["all"], draft=draft,
                                 debug_lines=True, assemble=True)
    line_insns = parse_line_object(result["objdump"], name)
    mapped = map_source_lines(line_insns, guide["ours"])
    _attach_lines(guide, mapped)
    guide["variables_by_register"] = parse_debug_variables(result["asm"], name)
    guide["pass_stats"] = pass_stats(result["dumps"], name)
    guide["loop_boundary_source_lines"] = loop_boundary_source_lines(
        result["dumps"], name
    )
    guide["call_rtl_evidence"] = call_rtl_evidence(
        result["dumps"], guide["target_only_calls"], name
    )

    greg_path = next((p for p in result["dumps"] if p.endswith(".greg")), None)
    lreg_path = next((p for p in result["dumps"] if p.endswith(".lreg")), None)
    alloc = None
    if greg_path:
        with open(greg_path) as stream:
            funcs = regalloc.split_functions(stream.read())
        if name in funcs:
            if lreg_path:
                with open(lreg_path) as stream:
                    usage_funcs = regalloc.split_functions(stream.read())
            else:
                usage_funcs = {}
            alloc = regalloc.analyse(name, funcs[name], usage_funcs.get(name))
    for sub in guide["register_substitutions"]:
        sub["variables"] = guide["variables_by_register"].get(sub["ours"], [])
        if alloc and sub["ours"] in REGNUM:
            hard = REGNUM[sub["ours"]]
            pseudos = []
            for pseudo, home in sorted(alloc["disp"].items()):
                if home == hard:
                    item = dict(
                        pseudo=pseudo,
                        preferences=[regalloc.rn(x) for x in alloc["preferences"].get(pseudo, [])],
                    )
                    if pseudo in alloc["usage"]:
                        item.update(alloc["usage"][pseudo])
                    pseudos.append(item)
            sub["pseudos"] = pseudos
    return guide


def _jsonable(guide):
    def clean(value):
        if isinstance(value, dict):
            return {k: clean(v) for k, v in value.items()}
        if isinstance(value, tuple):
            return list(value)
        if isinstance(value, list):
            return [clean(x) for x in value]
        return value
    result = dict(guide)
    # The top-level streams duplicate every instruction and are only an
    # internal alignment aid.  Keep each hunk's compact target/ours snippets.
    result.pop("target", None)
    result.pop("ours", None)
    result["hunks"] = [
        {k: v for k, v in h.items()
         if k not in {"target_indices", "ours_indices"}}
        for h in guide.get("hunks", [])
    ]
    return clean(result)


def print_report(g, max_hunks=12):
    print(f"rtlguide {g['name']} @ {g['address']:#x}: "
          f"{g['local_differing_bytes']} differing bytes "
          f"({g['whole_image_differing_bytes']} whole image), "
          f"length {g['ours_bytes']} / {g['target_bytes']}")
    if g["primary"] == "matched":
        print("  MATCH — no residual to diagnose.")
        return
    cats = ", ".join(f"{k}={v}" for k, v in
                     sorted(g["categories"].items(), key=lambda x: (-x[1], x[0])))
    print(f"  owner: {g['primary']}  [{cats}]")
    print(f"  inspect: {', '.join('.' + x for x in g['passes'])}")
    if g.get("known_residual_signatures"):
        print("  known residual signature(s): " +
              ", ".join(g["known_residual_signatures"]))
        for signature in g["known_residual_signatures"]:
            hint = SIGNATURE_HINTS.get(signature)
            if hint:
                print(f"    {signature}: {hint}")
        if "adjacent-independent-load-order" in g["known_residual_signatures"]:
            residual_lines = set(g.get("source_lines", []))
            nearby = {}
            for rtl_pass, lines in g.get("loop_boundary_source_lines", {}).items():
                hits = [line for line in lines if any(
                    abs(line - residual) <= 2 for residual in residual_lines
                )]
                if hits:
                    nearby[rtl_pass] = hits
            if nearby:
                evidence = "  ".join(
                    f".{rtl_pass} L{','.join(map(str, lines))}"
                    for rtl_pass, lines in sorted(nearby.items())
                )
                print("    LOOP_END fence evidence near residual: " + evidence)
        print("    confirm the named RTL pass and bounded source levers before parking")

    for n, h in enumerate(g["hunks"][:max_hunks], 1):
        line = ",".join(map(str, h.get("source_lines", []))) or "?"
        print(f"\n  hunk {n}: {h['category']}  C line {line}")
        for addr, insn in h["target"][:6]:
            print(f"    T {addr:#x}  {insn}")
        for addr, insn in h["ours"][:6]:
            print(f"    O {addr:#x}  {insn}")
        if len(h["target"]) > 6 or len(h["ours"]) > 6:
            print("    ...")
    if len(g["hunks"]) > max_hunks:
        print(f"\n  ... {len(g['hunks']) - max_hunks} more hunks (use --max-hunks)")

    if g["register_substitutions"]:
        print("\n  register goals (candidate -> target):")
        for sub in g["register_substitutions"][:12]:
            vars_ = ",".join(sub.get("variables", [])) or "?"
            prefs = sorted({p for x in sub.get("pseudos", []) for p in x["preferences"]})
            ptext = ("; allocator preferences " + ",".join(prefs)) if prefs else ""
            priorities = sorted({x["priority"] for x in sub.get("pseudos", [])
                                 if "priority" in x}, reverse=True)
            if priorities:
                ptext += "; alloc priorities " + ",".join(map(str, priorities[:4]))
            print(f"    ${sub['ours']} -> ${sub['target']} x{sub['count']}: "
                  f"locals {vars_}{ptext}")

    interesting = ["rtl", "cse", "combine", "lreg", "greg", "sched2", "jump2", "dbr"]
    present = [x for x in interesting if x in g.get("pass_stats", {})]
    if present:
        print("\n  RTL pass trace (top-level insns / labels / moves / calls):")
        print("    " + "  ".join(
            f"{p}={g['pass_stats'][p]['insns']}/{g['pass_stats'][p]['labels']}/"
            f"{g['pass_stats'][p]['moves']}/{g['pass_stats'][p]['calls']}"
            for p in present))

    if g.get("call_tail_hint"):
        counts = g["call_counts"]
        print("\n  call-tail diagnosis:")
        print(f"    target retains {counts['target']} physical calls; candidate has "
              f"{counts['candidate']}.")
        if g.get("target_only_calls"):
            desc = ", ".join(
                f"{x['callee']} x{x['count']}" for x in g["target_only_calls"]
            )
            print(f"    target-only aligned calls: {desc}")
        for item in g.get("call_rtl_evidence", []):
            print(f"    candidate {item['callee']}: {item['expanded_sites']} at expand, "
                  f"{item['after_jump_sites']} after jump")
            for fp in item["fingerprints"]:
                uses = ",".join(fp["uses"]) or "no hard-reg uses"
                print(f"      x{fp['count']} result={fp['result']} uses={uses}")
        print("    Inspect .rtl versus .jump/.jump2: machine-identical calls only stay "
              "separate when result mode or CALL_INSN_FUNCTION_USAGE differs. "
              "Check the caller's original declaration and already-live argument "
              "registers; this is diagnostic, not an automatic prototype rewrite.")

    flow = g.get("control_flow_counts", {})
    if flow and flow.get("target") != flow.get("candidate"):
        target_flow = flow["target"]
        candidate_flow = flow["candidate"]
        print("\n  control-flow audit (physical instructions):")
        print("    " + "  ".join(
            f"{key}={target_flow[key]}/{candidate_flow[key]} target/candidate"
            for key in target_flow
        ))
        if (candidate_flow["conditional_branches"] >
                target_flow["conditional_branches"]):
            print("    WARNING: candidate has target-absent conditional branch(es); "
                  "an exact length or lower byte score is not structural proof.")

    rematerializations = g.get("stack_address_rematerializations", [])
    if rematerializations:
        print("\n  stack-address retention hints:")
        for item in rematerializations[:5]:
            offset = item["offset"]
            saved = item["candidate_saved_registers"]
            suffix = ("; candidate retained via " +
                      ",".join("$" + reg for reg in saved)) if saved else ""
            print(f"    sp+{offset:#x}: target rematerializes "
                  f"{item['target_sites']}x, candidate "
                  f"{item['candidate_sites']}x{suffix}")
        print("    Inspect the stack-address allocno in .lreg/.greg. Try an "
              "expanded-inline pointer-formal barrier for repeated &local calls, "
              "or a hand-rolled loop/scope split when loop.c retains the base.")

    print("\n  mechanical search:")
    print(f"    rules: {', '.join(g['rules']) or '(no local rule; source structure first)'}")
    if g["missing_move_registers"]:
        print("    missing-move register targets (inspect .lreg/.greg; no asm emitted): " +
              ", ".join("$" + x for x in g["missing_move_registers"]))
    if g["primary"] == "structure/length":
        print(f"    tools/stackplan.py {g['name']} -n  # frame/aggregate overlap")
    print(f"    tools/autorules.py {g['name']} --guided")
    print("  Exact bytes remain authoritative; review any partial-score improvement.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--no-build", action="store_true", help="reuse current .shake build")
    ap.add_argument("--no-rtl", action="store_true", help="skip standalone dumps/line map")
    ap.add_argument("--json", action="store_true", help="machine-readable report")
    ap.add_argument("--max-hunks", type=int, default=12)
    args = ap.parse_args()
    try:
        guide = diagnose(args.name, build=not args.no_build, rtl=not args.no_rtl)
    except (FileNotFoundError, ValueError, RuntimeError) as e:
        sys.exit(f"rtlguide: {e}")
    if args.json:
        print(json.dumps(_jsonable(guide), indent=2, sort_keys=True))
    else:
        print_report(guide, args.max_hunks)
    return 0 if guide["primary"] == "matched" else 1


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("rtlguide", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"rtlguide: {e}")
