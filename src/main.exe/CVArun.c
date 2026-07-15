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
 *     slot holds a Sprite3D-extended pointer (SetupSprite vallocs 0x8C, not
 *     sizeof(Sprite3D)==0x68 — CVAsetup.c's own SetupSprite call proves the
 *     extra 0x24 bytes) with a "hidden" flag@0x5A (Sprite3D's own
 *     `attribute`, bit 0 tested) and a 3-byte fade counter@0x7C-0x7E (only
 *     the low 3 bytes of a wider field elsewhere) that increments by 8
 *     while its OWN sign bit is clear, then GsSortSprite's the GsSPRITE
 *     embedded at that same pointer + 0x68.
 *  2. CVAhuman[5] (proven HumanAnimType: human/loop/motid) reconciliation:
 *     for each live human whose queued motion has already looped enough
 *     (`CVAhuman[i].loop <= human->motion->loop`), either motid==-1 (stop:
 *     clear motion->loop and the human's x/z velocity) or (status != DEAD)
 *     start the queued motid via SetNowMotion and clear the slot.
 * Finally advances the CVA frame counter (D_80097CC0) and, once it reaches
 * the current event's own duration (CVAnow->mode,
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

/*
 * A Sprite3D allocated by SetupSprite with 0x24 extra trailing bytes
 * (SetupSprite vallocs 0x8C, not sizeof(Sprite3D)==0x68) — an embedded
 * GsSPRITE at +0x68 (GsSortSprite'd directly off this pointer, CVAsetup.c
 * proves `*piVar2 + 0x6c`/`+0x6e` are plain offsets off the SAME pointer,
 * no extra indirection) plus this fade/flag pair. Only the fields this
 * function touches are named; the struct is never itself array-strided
 * (TENCHU_POSITIONAL_DATA_AREA_ is an array of these POINTERS, not of the
 * struct), so truncating it here is safe.
 */
typedef struct
{
    u8 pad0[0x5A];
    u16 flag;    /* 0x5A, bit 0 tested (Sprite3D's own `attribute`) */
    u8 pad1[0x7C - 0x5C];
    s8 fadeA;    /* 0x7C, sign-tested */
    u8 fadeB;    /* 0x7D */
    u8 fadeC;    /* 0x7E */
} PositionalEntry;

extern PositionalEntry *TENCHU_POSITIONAL_DATA_AREA_[6];
extern int StageID;
extern u8 CHOSEN_CHARACTER;
extern GsOT *OTablePt;

typedef struct
{
    Humanoid *human; /* 0x0 */
    s16 loop;        /* 0x4 */
    s16 motid;       /* 0x6 */
} HumanAnimType;

extern HumanAnimType CVAhuman[5];

typedef struct
{
    s16 unk0; /* 0x0 */
    s16 mode; /* 0x2 */
    s16 x;    /* 0x4 */
    s16 y;    /* 0x6 */
    s16 z;    /* 0x8 */
    s16 param; /* 0xA */
} EventListEntry;

extern EventListEntry *CVAnow;
extern s16 D_80097CC0;

extern void ComputeAllConflict(void);
extern void StartDrawing(void);
extern void AVCameraControl(void);
extern short ControlAllHumanoid(void);
extern void DrawConstruction(void);
extern void DrawEffect(void);
extern void DoItemProc(void);
extern void DoMiscProc(void);
extern void DrawTelop(void);
extern void update_something_for_each_visible_enemy_(void);
extern void EndDrawing(s16 sync);
extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, int pri);
extern short SetNowMotion(Humanoid *human, short mid, short move);
extern short CVAupdate(void);

short CVArun(void)
{
    s16 i;
    PositionalEntry *e;
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
            if ((e->flag & 1) == 0)
            {
                if (e->fadeA >= 0)
                {
                    c = e->fadeC + 8;
                    e->fadeC = c;
                    e->fadeB = c;
                    e->fadeA = c;
                }
                GsSortSprite((GsSPRITE *)((u8 *)TENCHU_POSITIONAL_DATA_AREA_[i] + 0x68), OTablePt, 0);
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
    if (D_80097CC0 >= CVAnow->mode)
    {
        CVAnow = (EventListEntry *)((u8 *)CVAnow + 0xC);
        return CVAupdate();
    }
    return 1;
}
