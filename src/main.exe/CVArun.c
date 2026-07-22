#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVArun(void);
 *     CHRANIM.C:238, 47 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $a2       struct MotionManager * mmp
 *     reg   $s1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern int StageID;
 *     extern struct GsOT *OTablePt;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct CVAType *CVAnow;
 * END PSX.SYM */

/*
 * CVArun (0x80050e60, 0x214 bytes) — the once-per-frame CVA cutscene body:
 * runs the normal per-frame draw/update pipeline (ComputeAllConflict through
 * update_something_for_each_visible_enemy_, matched — Ghidra's own
 * `FUN_80029368`), then two CVA-specific passes:
 *  1. stage 10 (a specific cutscene stage) + CHOSEN_CHARACTER==0 only:
 *     6-entry TENCHU_POSITIONAL_DATA_AREA_ sprite-fade-and-sort pass — each
 *     each slot is a Sprite3D with a "hidden" `attribute` bit and a
 *     three-channel fade in its embedded GsSPRITE. The shared brightness
 *     increments by 8 while `sprite.r`'s sign bit is clear, then the sprite
 *     is sorted.
 *  2. CVAhuman[5] (proven HumanAnimType: human/loop/motid) reconciliation:
 *     for each live human whose queued motion has already looped enough
 *     (`CVAhuman[i].loop <= human->motion->loop`), either motid==-1 (stop:
 *     clear motion->loop and the human's x/z velocity) or (status != DEAD)
 *     start the queued motid via SetNowMotion and clear the slot.
 * Finally advances the CVA frame counter (D_80097CC0) and, once it reaches
 * the current event's own duration (CVAnow->id,
 * the same field AVCameraSetup dispatches on — the event record doubles as
 * a duration when read this way), advances to the next 12-byte event
 * record and calls CVAupdate for the new one; returns 1 while still
 * running the current event, else CVAupdate's own continue/stop flag.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Both loop counters are `short i` (PSX.SYM), reused across both loops —
 *    the narrow counter suppresses loop.c's strength reduction, matching
 *    the target's recompute-from-base address for both
 *    `TENCHU_POSITIONAL_DATA_AREA_[i]` and `&CVAhuman[i]`.
 *  - The positional-data slot's own pointer is loaded ONCE per iteration
 *    into a named local (`e`) for the flag/fade field accesses, but the
 *    GsSortSprite call re-reads `TENCHU_POSITIONAL_DATA_AREA_[i]` directly
 *    (NOT `e`) — the target reloads the slot a SECOND time right before
 *    the call instead of reusing the cached value, matching Ghidra's own
 *    rendering (`*piVar4 + 0x68`, a fresh dereference of the slot address,
 *    distinct from `iVar3` which is `*piVar4`'s EARLIER read reused for
 *    the flag/fade tests). Reusing `e` here compiles one `lw` short.
 *  - The fade counter's new value is computed once (`c = e->fadeC + 8;`)
 *    and stored to all three bytes from that one register — writing each
 *    store as `+8` inline would reload/recompute three times.
 */

extern Sprite3D *TENCHU_POSITIONAL_DATA_AREA_[6];
extern u8 CHOSEN_CHARACTER;
extern GsOT *OTablePt;

extern HumanAnimType CVAhuman[5];

extern CVAType *CVAnow;
extern s16 D_80097CC0;

extern void ComputeAllConflict(void);
extern void AVCameraControl(void);
extern short ControlAllHumanoid(void);
extern void DrawConstruction(void);
extern void DrawEffect(void);
extern void DoItemProc(void);
extern void DoMiscProc(void);
extern void DrawTelop(void);
extern void update_something_for_each_visible_enemy_(void);
extern short SetNowMotion(Humanoid *human, short mid, short move);
extern short CVAupdate(void);

short CVArun(void)
{
    s16 i;
    Sprite3D *e;
    u8 c;
    MotionManager *mmp;
    Humanoid *human;
    Humanoid *reload;
    s16 motid;

    ComputeAllConflict();
    StartDrawing();
    AVCameraControl();
    ControlAllHumanoid();
    DrawConstruction();
    DrawEffect();
    DoItemProc();
    DoMiscProc();
    DrawTelop();
    update_something_for_each_visible_enemy_();

    if (StageID == 10 && CHOSEN_CHARACTER == 0)
    {
        for (i = 0; i < 6; i++)
        {
            e = TENCHU_POSITIONAL_DATA_AREA_[i];
            if ((e->attribute & 1) == 0)
            {
                if ((s8)e->sprite.r >= 0)
                {
                    c = e->sprite.b + 8;
                    e->sprite.b = c;
                    e->sprite.g = c;
                    e->sprite.r = c;
                }
                GsSortSprite(&TENCHU_POSITIONAL_DATA_AREA_[i]->sprite, OTablePt, 0);
            }
        }
    }

    EndDrawing(-2);

    for (i = 0; i < 5; i++)
    {
        human = CVAhuman[i].human;
        if (human != 0 && CVAhuman[i].loop <= human->motion->loop)
        {
            mmp = human->motion;
            motid = CVAhuman[i].motid;
            if (motid == -1)
            {
                mmp->loop = -1;
                reload = CVAhuman[i].human;
                reload->vector.vz = 0;
                reload->vector.vx = 0;
            }
            else if (human->status != 0x11)
            {
                SetNowMotion(human, motid, 1);
                CVAhuman[i].human = 0;
            }
        }
    }

    D_80097CC0++;
    if (D_80097CC0 >= CVAnow->id)
    {
        CVAnow++;
        return CVAupdate();
    }
    return 1;
}
