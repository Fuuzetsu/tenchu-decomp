#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005a7a4 (0x8005a7a4) — advance the memory-card save UI state machine.
 *
 * The target selects the new state into a register and does ONE store. A
 * ternary is NOT equivalent (cc1 duplicates the D_80097D32 store into both
 * arms, +8). Three constructs in the shared `update_count` tail are
 * load-bearing; each is measured, and removing any of them costs bytes.
 *
 * The tail's block is LOAD-FREE, so sched cannot reorder it (every insn_cost
 * is 1, so priority() collapses to 1 and nothing moves) — its order is expand
 * order, and the answer had to be source structure:
 *
 *   - `cond = value < 3;` ahead of the store. cc1 emits a compare and its
 *     branch TOGETHER from the MIPS branch expander (`cmpsi` only records the
 *     operands and emits nothing), so no statement can be parked between them;
 *     hoisting the compare into a local is the only way to emit `slti` before
 *     the store. It MUST be spelled `< 3`, never `> 2`: as a *value*, `> 2`
 *     goes through store_flag, which cannot put the constant in the immediate
 *     and folds to `lui`/`slt` against 2<<16 (measured: 13). `< 3` is
 *     store_flag's natural `slti` — the target's exact instruction.
 *
 *   - the do{}while(0) around the D_80097D32 store. A fence emits CODE_LABELs,
 *     i.e. a basic-block boundary, and that boundary is what pins the result:
 *       * combine will not merge the compare into the branch across it, so the
 *         `slti` stays put instead of being re-split back down at the branch.
 *         Without the fence the `cond` local is entirely byte-neutral (12).
 *       * reorg's backward delay-slot scan stops dead at the label — which is
 *         why the target's `bnez` keeps an empty delay slot even though the
 *         `sh` sits right before it and never touches v0.
 *     Both of the target's oddities come from that single boundary.
 *
 *   - the do{}while(0) around the D_80097D2E store buys next_state one
 *     loop-depth-weighted ref, winning it a0 (regalloc.py: `p83 > p117: needs
 *     +1 weighted ref`). It must enclose ONLY this read: wrapping the `if`
 *     would also double saved_state's refs, and its shorter live range would
 *     then outrank next_state and take a0 the wrong way.
 *
 * Measured, so nobody re-derives them: cond alone 12, fence alone 12, both 0;
 * unwrapping the D_80097D2E fence 7. Moving the store after the `if` lets
 * reorg take it into the delay slot: 1020 bytes, 4 short.
 */

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
    s32 cond;
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
        cond = value < 3;
        do {
            D_80097D32 = incremented;
        } while (0);
        if (!cond)
            next_state = saved_state;
        /*
         * Load-bearing fence (autorules fence-unwrap: unwrapping it costs 7
         * bytes, 0 -> 7). It buys next_state ONE loop-depth-weighted ref:
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
