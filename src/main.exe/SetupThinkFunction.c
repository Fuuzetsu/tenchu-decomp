#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupThinkFunction(struct Humanoid *human, short type);
 *     THINK.C:212, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *     param $a1       short type
 *
 * Globals it touches, as the original declared them:
 *     extern short (*Think1Func[10])();
 *     extern short (*Think2Func[5])();
 *     extern short (*Think3Func[10])();
 *     extern short (*Think4Func[6])();
 * END PSX.SYM */

/*
 * SetupThinkFunction (0x8002f73c, 0xb8 bytes) — installs the four
 * think_settingN behaviour-function-pointer slots of a Humanoid
 * (game_types.h; think_setting0..3 proven @0x60-0x6C) from four lookup
 * tables keyed off nibbles of `type`, then sets or clears bit 4 of
 * some_character_marker_thing depending on whether `type` is one of three
 * "default" sentinels (0, 0x1111, 0x2222).
 *
 * Think1Func/Think4Func are plain arrays — their index is explicitly scaled
 * (`sll ,2`) before being added to the base. Think2Func/Think3Func are
 * indexed through raw BYTE-offset pointer arithmetic instead: the shift
 * amount (18/22) already produces a word-aligned byte offset with no
 * separate scale instruction, matching Ghidra's own
 * `*(T **)((int)Think2Func + (shifted >> 18 & 0x3C))` rendering exactly.
 * The final sentinel check re-derives an int from `type<<16` instead of
 * reusing the s16 parameter — matching Ghidra's separate `iVar2`.
 *
 * `table2`/`table3` (permuter-found, docs/matching-cookbook.md): copying
 * Think2Func/Think3Func into a local pointer BEFORE adding the byte offset
 * — instead of casting the extern array name directly inline — is what
 * makes cc1 schedule each table's base-address materialization (lui/addiu)
 * ahead of the PRECEDING table's load/store (a 2-deep software-pipelined
 * schedule); the inline-cast form computes the shift/mask first and the
 * address after, a stable but non-matching alternative schedule with
 * identical instruction content. Think4Func (last table, no following
 * table to prefetch for) needed no such temp.
 */
extern think_func_ *Think1Func[];
extern think_func_ *Think2Func[];
extern think_func_ *Think3Func[];
extern think_func_ *Think4Func[];

void SetupThinkFunction(Humanoid *human, s16 type)
{
    s32 check;
    think_func_ **table2;
    think_func_ **table3;

    human->think[0] = Think1Func[type & 0xF];
    table2 = Think2Func;
    human->think[1] = *(think_func_ **)((u8 *)table2 + ((((s32)type << 16) >> 18) & 0x3C));
    table3 = Think3Func;
    human->think[2] = *(think_func_ **)((u8 *)table3 + ((((s32)type << 16) >> 22) & 0x3C));
    human->think[3] = Think4Func[(u32)((s32)type << 16) >> 28];
    check = ((s32)type << 16) >> 16;
    if (check == 0 || check == 0x1111 || check == 0x2222) {
        human->attribute &= 0xFFFB;
    } else {
        human->attribute |= 4;
    }
}
