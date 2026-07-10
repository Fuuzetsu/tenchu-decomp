#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short ItemUse(void);
 *     THINK_3.C:225, 18 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s0       short id
 *
 * Globals it touches, as the original declared them:
 *     extern short Degree;
 * END PSX.SYM */

/*
 * ItemUse (0x8002dc0c, 0x10c bytes) — think-handler: while idle
 * (something_about_current_animation->frames_since_animation_start == 0,
 * i.e. motion just started) and status == USING_ITEM, auto-uses one of the
 * three "use while idling" items in priority order: kusuri (index 3, when
 * health is under a third of max), then shuriken-ish (index 1, only when
 * roughly facing the target, |Degree| < 100) or fire (index 4, only when
 * |Degree| < 300) triggering SetNowMotion.
 *
 * `Me_THINK_C->item[N]` (Ghidra's flat array) is the SAME memory as
 * game_types.h's `inventory.player_item_counts` union — item.h's Humanoid
 * has the plain array view (`u8 item[0x1A]`), character_state's own proven
 * layout instead nests it under the named union; `pic_by_index[N]` reaches
 * the identical byte.
 *
 * `(s16)Me_THINK_C->current_health`/`(s16)Me_THINK_C->max_health` — both
 * fields are proven `u16` in the shared header, but THIS file's compare
 * reads current_health with a signed `lh` and max_health's div-by-3 uses
 * the SIGNED magic-multiply constant (0x55555556 + a `sra 31` sign
 * correction, not the unsigned form) — same per-TU divergent-signedness
 * cast as DoInfoViewProc's `(s16)CamState.Owner->lifemax`.
 *
 * The ReqItemDefault/SetNowMotion tail calls' return values are NOT
 * returned: after `ReqItemDefault(Me_THINK_C, 3);` the asm jumps straight
 * to the shared epilogue with NO move into $v0 (a bare `return;`, valid
 * -w-suppressed old-style C for a value-returning function); the final
 * `SetNowMotion(Me_THINK_C, id, 1);` is the LAST statement with no
 * following `return` at all, so control falls off the end of the function
 * — in both cases $v0 at the epilogue is simply whatever the callee left
 * there, never explicitly set by ItemUse itself (confirmed: neither call
 * site has the short-result sll/sra pair a genuine `return call();` would
 * need, unlike Think4contact's `return Think4abandon();`).
 *
 * `id`'s assignments (0xE00, 0xF02) each sit textually right before the
 * guard that tests the SAME condition they were just derived from — cc1
 * schedules them into that guard branch's delay slot as an ordinary
 * independent-and-ready instruction (not the "hoist the taken block's
 * first statement" trick; here the assignment genuinely precedes the
 * test in source, per Ghidra's own literal order).
 */
extern void ReqItemDefault(character_state *user, s32 ItemID);
extern s16 SetNowMotion(character_state *human, s16 mid, s16 move);
extern s16 Degree;

static s16 ItemUse(void)
{
    character_state *me;
    s16 id;
    s32 d;

    if (Me_THINK_C->something_about_current_animation->frames_since_animation_start != 0)
    {
        return 5;
    }
    if ((s16)Me_THINK_C->character_status != 5)
    {
        return 5;
    }

    if (Me_THINK_C->inventory.player_item_counts.pic_by_index[3] != 0 &&
        (s16)Me_THINK_C->current_health < (s16)Me_THINK_C->max_health / 3)
    {
        ReqItemDefault(Me_THINK_C, 3);
        return;
    }

    me = Me_THINK_C;
    if (me->inventory.player_item_counts.pic_by_index[1] != 0)
    {
        d = Degree;
        if (d < 0)
        {
            d = -d;
        }
        id = 0xE00;
        if (d < 100)
        {
            goto do_motion;
        }
    }

    if (me->inventory.player_item_counts.pic_by_index[4] == 0)
    {
        goto end;
    }

    d = Degree;
    if (d < 0)
    {
        d = -d;
    }
    id = 0xF02;
    if (d < 300)
    {
        goto do_motion;
    }
    goto end;

do_motion:
    SetNowMotion(me, id, 1);
    return;

end:
    return;
}
