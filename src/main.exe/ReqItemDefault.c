#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ReqItemDefault(struct Humanoid *user, enum TItemType ItemID);
 *     ITEM.C:3261, 24 src lines, frame 96 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * user
 *     param $a1       enum TItemType ItemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *     stack sp+56     struct VECTOR v
 *     stack sp+72     struct VECTOR v0
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * ReqItemDefault (0x800475f4) — spawn an item thrown/placed in front of a
 * Humanoid (called from the ActXXX/DamageControl/ItemUse family). Builds a
 * PARAM_ITEM_USE: start = user's model position (y raised by 0x4B0), end =
 * start + a rotated toss vector (the fixed D_80012258 vector rotated by
 * either the current view direction, when the camera is looking through
 * this user in CMODE_DIRECTION, or the user's own model rotation otherwise).
 * Forwards to ReqItemUse (the big dispatcher — not in this batch).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `type`/`user` are stored FIRST (matching the asm's chronological order),
 *    not after the rotation-vector fill as Ghidra's rendering shows it —
 *    another instance of "trust the assembly over Ghidra's statement order".
 *  - `user->model->locate.coord.t[0/1/2]` for start.vx/vy/vz are three
 *    INDEPENDENT dereferences (no cached pointer temp): each re-does the
 *    `->model` load, matching three separate `lw ...,0x58(v1)` in the asm
 *    with no intervening call to justify caching.
 *  - `pm = u.user->model;` (used for the if-condition and reused unreloaded
 *    in the else branch) reads through the just-STORED struct field, not the
 *    raw `user` parameter — the asm reloads it from the local's stack slot
 *    (sp+0x14) right before the branch, which only lines up with reading the
 *    field (the function keeps zero callee-saved registers, so `user` has no
 *    other memory home to reload from across the intervening memset() call).
 *  - `D_80012258` is a plain (non-gp) extern VECTOR constant; its address
 *    compiles as a split `lui`/`addiu` into TWO different registers, the
 *    "not -G8-small" tell — declared as an unknown-size array per the
 *    cookbook's respelling rule.
 *  - The GetVectorRotation out-params are read back with `lw` (full word),
 *    so `rx`/`ry` must be `s32` locals (matching Ghidra's `int`), even though
 *    ReqItemLightningBolt's TU declares the same callee's out-params as
 *    `short *` — original TUs disagree with each other's prototype for the
 *    same extern; respelled per-TU here too.
 */

/* Camera status (Ghidra: TCameraStatus, at CamState) — redefined locally
 * like every other TU that touches it (PauseProc/PlayerOption/
 * LayoutEnemyOption/DebugMenuItemSet/DoInfoViewProc); only Owner/Mode used
 * here. */
typedef struct
{
    VECTOR TargetVector;         /* 0x00 */
    Humanoid *Owner;             /* 0x10 */
    s32 Mode;                    /* 0x14 (TCameraMode) */
    s16 DirectionRX;             /* 0x18 */
    s16 DirectionRY;             /* 0x1A */
    s32 OldMode;                 /* 0x1C */
    u8 Valiation;                /* 0x20 */
} TCameraStatus;

/* Current view position + rotation target (Ghidra: ViewInfo, a global with
 * more fields than shown elsewhere); only vpx/vpy/vpz and vrx/vry/vrz are
 * read here, as two overlapping VECTOR views (offset 0 and offset 0xC). */
typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
} TViewInfo;

extern TCameraStatus CamState;
extern TViewInfo ViewInfo;
extern VECTOR D_80012258[];
extern void GetVectorRotation(VECTOR *from, VECTOR *to, s32 *rx, s32 *ry);
extern void RotateVector(VECTOR *v, s32 rx, s32 ry, s32 rz);
extern void ReqItemUse(PARAM_ITEM_USE *p);

void ReqItemDefault(Humanoid *user, s32 ItemID)
{
    PARAM_ITEM_USE u;
    VECTOR rot;
    u8 buf[0x10];
    ModelArchiveType *pm;
    s32 rx;
    s32 ry;
    s32 rz;

    u.type = ItemID;
    u.user = user;
    u.start.vx = user->model->locate.coord.t[0];
    u.start.vy = user->model->locate.coord.t[1] - 0x4B0;
    u.start.vz = user->model->locate.coord.t[2];
    rot = D_80012258[0];
    memset(buf, 0, 0x10);
    pm = u.user->model;
    if (CamState.Owner->model == pm && CamState.Mode == 1)
    {
        GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
        rz = 0;
    }
    else
    {
        rx = pm->rotate.vx;
        rz = pm->rotate.vz;
        ry = pm->rotate.vy;
    }
    RotateVector(&rot, rx, ry, rz);
    u.end.vx = u.start.vx + rot.vx;
    u.end.vy = u.start.vy + rot.vy;
    u.end.vz = u.start.vz + rot.vz;
    ReqItemUse(&u);
}
