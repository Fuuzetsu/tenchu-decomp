#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupThinkFunction(struct Humanoid *human, short type);
 *     THINK.C:212, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct Humanoid * human
 *     param $a1       short type
 * END PSX.SYM */

/*
 * SetupThinkFunction (0x8002f73c, 0xb8 bytes) — installs the four
 * think_settingN behaviour-function-pointer slots of a character_state
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
extern some_char_state_function *Think1Func[];
extern some_char_state_function *Think2Func[];
extern some_char_state_function *Think3Func[];
extern some_char_state_function *Think4Func[];

void SetupThinkFunction(character_state *human, s16 type)
{
    s32 check;
    some_char_state_function **table2;
    some_char_state_function **table3;

    human->think_setting0 = Think1Func[type & 0xF];
    table2 = Think2Func;
    human->think_setting1 = *(some_char_state_function **)((u8 *)table2 + ((((s32)type << 16) >> 18) & 0x3C));
    table3 = Think3Func;
    human->think_setting2 = *(some_char_state_function **)((u8 *)table3 + ((((s32)type << 16) >> 22) & 0x3C));
    human->think_setting3 = Think4Func[(u32)((s32)type << 16) >> 28];
    check = ((s32)type << 16) >> 16;
    if (check == 0 || check == 0x1111 || check == 0x2222) {
        human->some_character_marker_thing &= 0xFFFB;
    } else {
        human->some_character_marker_thing |= 4;
    }
}
