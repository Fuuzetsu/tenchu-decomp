#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SelectCameraOwnerOption(void);
 *     INFOVIEW.C:796, 27 src lines, frame 592 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     stack sp+16     struct TAdtSelect [31] targets
 *     stack sp+264    unsigned char [30][10] msg
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/*
 * SelectCameraOwnerOption (0x8005ba08, 0x158 bytes) — debug menu's "select
 * camera owner" submenu (LayoutEnemyOption dispatch case 7): builds an
 * AdtSelect menu of every live Humanoid's index ("%d") -> pointer pair, lets
 * the user pick one, and re-centers the debug camera (ViewInfo) on the
 * chosen Humanoid's model position.
 *
 * Matching notes:
 *  - The scoped union initializer is the source-level spelling of the
 *    original high-word address construction. A plain `(char *)0x80090000`
 *    gives GCC enough nonzero-bit information to replace `+ 0x7D70` with an
 *    `ori`; initializing the two 16-bit fields preserves the target's
 *    `lui s3,0x8009` followed by `addiu a1,s3,0x7D70`, with no stack object
 *    or emitted initialization overhead.
 *  - Real retail layout differs from PSX.SYM's (earlier-build) recollection:
 *    the raw asm places `msg` at sp+0x130 (304), not PSX.SYM's sp+264, which
 *    means `targets` is 36 entries (0x120 bytes), matching Ghidra's own
 *    TAdtSelect[36] rather than PSX.SYM's [31] — the asm wins (frame
 *    0x2A8 = args/pad + targets 0x120 + msg 0x160 + 5 saved regs + pad).
 *  - `CamState.Owner = (Humanoid *)AdtSelect(...)` writes the SAME address a
 *    different TU's small `CURRENTLY_SELECTED_CHARACTER_STATE_PTR` aliases
 *    (cookbook: cross-TU field aliasing) — write it as CamState.Owner, the
 *    proven shared struct, not a separate symbol.
 *  - The three `CamState.Owner->model->locate.coord.t[i]` reads are kept as
 *    three independent, uncached expressions (no `model` temp): the asm
 *    reloads Owner->model fresh via v0 (Owner) for EACH of the three t[i]
 *    accesses (three separate `lw ?,0x58(v0)`), which only happens when the
 *    source repeats the whole chain rather than caching a `model` pointer.
 *  - The coordinate copy is spelled `vrx`/`vry`/`vrz` = t[0..2] (direct reads)
 *    then `vpx=vrx; vpy=vry-5000; vpz=vrz;` (plain copies) — the OPPOSITE of
 *    Ghidra's naming (which computes into vpx/vry/vpz and copies vrx=vpx,
 *    vrz=vpz). The asm's actual value-to-register binding proves it: `t[0]`
 *    stores to vrx's offset FIRST (needs the full address), and the vpx
 *    store (offset 0, foldable through the address's hi-only register) is
 *    scheduled much later, matching a plain-copy-from-vrx shape, not
 *    Ghidra's reverse. Fixed a 6-instruction register-identity mismatch.
 *  - No %gp_rel symbols in this function (tools/gpsyms.py): Humans/
 *    HumanGroup are another TU's smalls, absolute here.
 *  - The inner do-while (Ghidra's do{...}while(iVar2<Humans)) is a
 *    HAND-ROLLED goto loop in source, not a real do-while: the target
 *    recomputes `&targets[i]` (sp+16+i*8) fresh every iteration rather than
 *    hoisting it to a preheader, which only happens when there are no loop
 *    notes for loop.c to act on (cookbook Loops: a hand-rolled goto loop
 *    "loses hoisting"; same lesson as PutNumber's /10 magic constant).
 *  - Ghidra's `iVar2`/`iVar3` are ONE variable, not two: they hold the same
 *    value at every point they're both live, and writing them as a single
 *    `iVar2` avoids an extra a3<->s0 copy-chain the real do-while's
 *    duplicate-name rendering would otherwise force into a separate pseudo.
 *  - `D_80097D70` (the "%d" format string) is a splat auto-name drifted -8
 *    bytes in this run (a whole D_80097D68..D_80097D90 chain shares the
 *    offset — verified against the .map). Bound a fresh
 *    `D_80097D70 = 0x80097D70;` in config/symbols.main.exe.txt per the
 *    cookbook's drifted-symbol recipe rather than fight the wrong auto-name.
 */
typedef struct
{
    VECTOR TargetVector;         /* 0x00 */
    Humanoid *Owner;             /* 0x10 */
    s32 Mode;                    /* 0x14 */
    s16 DirectionRX;             /* 0x18 */
    s16 DirectionRY;             /* 0x1A */
    s32 OldMode;                 /* 0x1C */
    u8 Valiation;                /* 0x20 */
} TCameraStatus;

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
    s32 rz;
    GsCOORDINATE2 *super;
} GsRVIEW2;

extern TCameraStatus CamState;
extern s16 Humans;
extern Humanoid *HumanGroup[];
extern GsRVIEW2 ViewInfo;
extern char D_80097D70[];   /* "%d" */
extern char D_80014018[];   /* "select camera owner" */

extern s32 AdtSelect(char *title, debug_menu_choice *menu, s32 mode);
extern void sprintf(char *s, char *fmt, ...);

void SelectCameraOwnerOption(void)
{
    Humanoid *pHVar1;
    char *buffer;
    int iVar2;
    Humanoid **ppHVar4;
    debug_menu_choice targets[36];
    char msg[352];

    if (Humans < 0x23)
    {
        iVar2 = 0;
        if (0 < Humans)
        {
            union
            {
                struct
                {
                    u32 low : 16;
                    u32 high : 16;
                } parts;
                char *pointer;
            } hi = {{0, 0x8009}};

            ppHVar4 = HumanGroup;
            buffer = msg;
        inner_loop:
            sprintf(buffer, hi.pointer + 0x7D70, iVar2);
            targets[iVar2].choice_name = buffer;
            pHVar1 = *ppHVar4;
            ppHVar4 = ppHVar4 + 1;
            buffer = buffer + 10;
            targets[iVar2].choice_number = (u32)pHVar1;
            iVar2 = iVar2 + 1;
            if (iVar2 < Humans)
                goto inner_loop;
        }
        targets[iVar2].choice_name = (char *)0;
        CamState.Owner = (Humanoid *)AdtSelect(D_80014018, targets, 0);
        ViewInfo.vrx = CamState.Owner->model->locate.coord.t[0];
        ViewInfo.vry = CamState.Owner->model->locate.coord.t[1];
        ViewInfo.vrz = CamState.Owner->model->locate.coord.t[2];
        ViewInfo.vpx = ViewInfo.vrx;
        ViewInfo.vpy = ViewInfo.vry - 5000;
        ViewInfo.vpz = ViewInfo.vrz;
    }
}
