#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005a7a4 (0x8005a7a4) — advance the memory-card save UI state machine.
 *
 * STATUS: NON_MATCHING — 12 of 1024 bytes differ, at the target's exact extent.
 * Build it with `NON_MATCHING=FUN_8005a7a4 ./Build`.
 *
 * The earlier "14 bytes / a0-a1 allocation + store scheduling" verdict was
 * WRONG on both counts and has been retired:
 *   - The register half was a DECOMPOSITION error, not allocation. The target
 *     selects the new state into a register and does ONE store; the old draft
 *     stored next_state then conditionally overwrote it (TWO stores). The
 *     `if (value > 2) next_state = saved_state;` + single store below is the
 *     target's shape. (A ternary is NOT equivalent here: cc1 duplicates the
 *     D_80097D32 store into both arms, +8 bytes.)
 *   - What remained was then a pure register tie, closed exactly as
 *     regalloc.py predicted (`p83 > p117: needs +1 weighted ref`). See the
 *     fence comment below.
 *
 * The whole residual is now ONE instruction: the D_80097D32 store sits before
 * the sll; the target has it between the slti and the bnez:
 *     target:  sll / sra / slti / sh v1,1690(gp) / bnez / nop / move a0,a1
 *     ours:    sh v1,1690(gp) / sll / sra / slti / bnez / nop / move a0,a1
 * This is NOT a scheduling tie that a permuter or a source reorder can reach:
 *   - The shared tail is a LOAD-FREE block, so every insn_cost is 1 and
 *     gcc-2.8.1 sched.c's priority() collapses every priority to 1 ("when all
 *     instructions have a latency of 1 ... no scheduling will be done"). The
 *     emitted order IS the RTL order, so the target's order must come from
 *     source order, not from sched.
 *   - But expand emits the compare and the branch adjacently, and separating
 *     them with a `cond` local does not help: combine merges the compare into
 *     the branch and re-splits it AT the branch, relocating it back below the
 *     store (measured: 13 bytes, and it rewrites sra/slti into lui/slt).
 *   - Moving the store after the `if` puts it in the branch's delay slot
 *     (reorg), which is 4 bytes SHORT (1020) — the wrong length.
 * A 420 s permuter run plateaued at 12; autorules found no improving edit.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", FUN_8005a7a4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_21);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_28);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2d);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_32);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_33);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_37);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_3c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_3);

/* jump-table pool @ 0x80013d34 (63 words; tables at 0x80013d34) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 FUN_8005a7a4_jtbl[63] = {
    0x8005A7FC, 0x8005AAF0, 0x8005AB00, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005A80C, 0x8005AAF0,
    0x8005AB00, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005A81C, 0x8005AB0C, 0x8005A82C, 0x8005A9DC,
    0x8005A9DC, 0x8005A83C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005A884, 0x8005A9DC, 0x8005A9DC, 0x8005A898,
    0x8005A95C, 0x8005A980, 0x8005A990, 0x8005A9CC,
    0x8005AB0C, 0x8005AB0C, 0x8005A9A0, 0x8005A9B0,
    0x8005A9CC, 0x8005A9DC, 0x8005A9DC, 0x8005A9F4,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AAE0, 0x8005AAF0, 0x8005AB00,
};

#else /* NON_MATCHING */
extern char *D_80097D18;
extern s16 CardStateFlag;
extern s16 D_80097D2E;
extern s16 D_80097D30;
extern s16 D_80097D32;
extern u8 D_8001005C;

extern s32 FUN_8005adbc(s16 mode);
extern s16 FUN_8005aba4(u16 *state, s16 *page);
extern s16 FUN_80056e30(char *name);
extern s16 SaveCard(s32 target, u8 *name, void *mem, s32 size, s16 write_data);
extern s32 FUN_8005b17c(s32 page, s32 pad);

s32 FUN_8005a7a4(s32 pad)
{
    u16 saved_state;
    s16 value;
    u16 next_state;
    u16 assigned;
    u16 incremented;

    FUN_8005adbc(0);
    switch ((s16)(D_80097D2E - 10))
    {
    case 0:
        D_80097D30 = 0x18;
        break;
    case 10:
        D_80097D30 = 0x19;
        break;
    case 0x1c:
        D_80097D2E = 0x28;
        break;
    case 0x1e:
        D_80097D2E = 0x2b;
        break;
    case 0x21:
        value = FUN_80056e30(D_80097D18);
        if (value == 0)
            goto probe_missing;
        if (value == 5)
            goto probe_present;
        goto clear_state;
probe_missing:
        D_80097D2E = 0x3c;
        break;
probe_present:
        D_80097D2E = 0x32;
        break;
    case 0x28:
        D_80097D30 = 7;
        D_80097D32 = 0;
        goto increment_state;
    case 0x2b:
        SaveCard(0, (u8 *)D_80097D18, (void *)0x80010000, 0xe70, 0);
        value = SaveCard(0, (u8 *)D_80097D18, (void *)0x80010000, 0xe70, 1);
        if (value == 1)
            goto save_2b_success;
        if (value >= 2)
            goto save_2b_ge2;
        if (value == 0)
            goto save_2b_zero;
        assigned = 0x38;
        goto save_2b_assign;
save_2b_ge2:
        if (value == 4)
            goto save_2b_four;
        if (value == 7)
            goto save_2b_seven;
        assigned = 0x38;
        goto save_2b_assign;
save_2b_zero:
        assigned = 0x36;
        goto save_2b_assign;
save_2b_success:
        assigned = 10;
        goto save_2b_assign;
save_2b_seven:
        assigned = 0x46;
        goto save_2b_assign;
save_2b_four:
        D_80097D2E = 0x1e;
        CardStateFlag = 0;
        goto save_2b_after_assign;
save_2b_assign:
        D_80097D2E = assigned;
save_2b_after_assign:
        if (D_80097D2E == 0x36)
            break;
        next_state = 0x35;
        saved_state = (u16)D_80097D2E;
        value = D_80097D32;
        incremented = value + 1;
        goto update_count;
    case 0x2c:
        if (D_8001005C == 0)
        {
            D_80097D30 = 9;
            break;
        }
    case 0x2d:
        D_80097D2E = 99;
        break;
    case 0x2e:
        D_80097D30 = 0xe;
        break;
    case 0x32:
        D_80097D30 = 0x2c;
        break;
    case 0x33:
        D_80097D30 = 7;
        D_80097D2E = 0x3f;
        D_80097D32 = 0;
        break;
    case 0x2f:
    case 0x34:
        D_80097D2E = 0x5a;
        break;
    case 0x1f:
    case 0x20:
    case 0x29:
    case 0x2a:
    case 0x35:
    case 0x36:
increment_state:
        D_80097D2E++;
        break;
    case 0x37:
        SaveCard(0, (u8 *)D_80097D18, (void *)0x80010000, 0xe70, 0);
        value = SaveCard(0, (u8 *)D_80097D18, (void *)0x80010000, 0xe70, 1);
        if (value == 1)
            goto save_37_success;
        if (value >= 2)
            goto save_37_ge2;
        if (value == 0)
            goto save_37_zero;
        assigned = 0x38;
        goto save_37_assign;
save_37_ge2:
        if (value == 4)
            goto save_37_four;
        if (value == 7)
            goto save_37_seven;
        assigned = 0x38;
        goto save_37_assign;
save_37_zero:
        assigned = 0x36;
        goto save_37_assign;
save_37_success:
        assigned = 10;
        goto save_37_assign;
save_37_seven:
        assigned = 0x46;
        goto save_37_assign;
save_37_four:
        D_80097D2E = 0x1e;
        CardStateFlag = 0;
        goto save_37_after_assign;
save_37_assign:
        D_80097D2E = assigned;
save_37_after_assign:
        if (D_80097D2E == 0x36)
            break;
        next_state = 0x41;
        saved_state = (u16)D_80097D2E;
        value = D_80097D32;
        incremented = value + 1;
update_count:
        D_80097D32 = incremented;
        if (value > 2)
            next_state = saved_state;
        /*
         * Load-bearing fence (autorules fence-unwrap: removing it costs 7
         * bytes, 12 -> 19). It buys next_state ONE loop-depth-weighted ref:
         * reg_n_refs is loop-depth weighted and global.c's priority is
         * floor_log2(refs) * refs/live_length. next_state (p83) scores
         * 2*4/16*10000 = 5000 and loses a0 to p117/p151 at 3/5 -> 6000;
         * doubling this one ref gives 5 weighted refs -> 6250 -> a0, which
         * fixes all six register-swapped instructions at once.
         * The fence must enclose ONLY this next_state read: wrapping the `if`
         * would also double saved_state's refs, and its shorter live range
         * (3/11) would then outrank next_state and take a0 the wrong way.
         */
        do {
            D_80097D2E = next_state;
        } while (0);
        break;
    case 0x3c:
        D_80097D30 = 0x1a;
        break;
    case 1:
    case 0xb:
    case 0x3d:
        D_80097D2E = -1;
        break;
    case 2:
    case 0xc:
    case 0x3e:
clear_state:
        D_80097D2E = 0;
        break;
    default:
        value = FUN_8005aba4((u16 *)&D_80097D2E, &D_80097D30);
        if (value == 0)
        {
            D_80097D30 = 0;
            D_80097D32 = 0;
            FUN_8005adbc(1);
            if (D_80097D2E < 0)
            {
                D_80097D2E = 3;
                return 1;
            }
            D_80097D2E = 3;
            return -1;
        }
        break;
    }

    D_80097D2E += FUN_8005b17c(D_80097D30, pad);
    return 0;
}

#endif /* NON_MATCHING */
