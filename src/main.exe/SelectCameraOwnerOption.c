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
 *     extern struct TCameraStatus CamState;
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
 *  - The ordinary indexed `for` loop is important.  The demo's debug symbols
 *    list only `i`, `targets`, and `msg`; the apparent `buffer`, HumanGroup
 *    cursor, and target pointer in the disassembly are compiler-created
 *    induction values, not source locals.  With all three expressions written
 *    naturally, cc1 strength-reduces `msg[i]` and `HumanGroup[i]` but leaves
 *    `targets[i]` as the target's fresh `sp+16+i*8` calculation.  Manually
 *    spelling the first two walkers made a real loop reduce the third as well
 *    and led to the misleading hand-written-goto reconstruction.
 *  - That real loop also hoists `%hi(D_80097D70)` into s3 while leaving the
 *    `%lo(D_80097D70)` addiu at the sprintf call.  Thus the same symbolic C
 *    both resolves to the retail `lui s3,0x8009; addiu a1,s3,0x7D70` bytes and
 *    carries the HI16/LO16 relocation pair required by a normal link.
 *  - Real retail layout differs from PSX.SYM's (earlier-build) recollection:
 *    the raw asm places `msg` at sp+0x130 (304), not PSX.SYM's sp+264, which
 *    means `targets` is 36 entries (0x120 bytes), matching Ghidra's own
 *    TAdtSelect[36] rather than PSX.SYM's [31].  `msg[35][10]` occupies 350
 *    bytes and its stack object rounds to 0x160, yielding the observed frame
 *    (0x2A8 = args/pad + targets 0x120 + msg 0x160 + 5 saved regs + pad).
 *  - `CamState.Owner = (Humanoid *)AdtSelect(...)` writes the recovered
 *    shared camera field, not a separate object at its interior address.
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
 *  - `D_80097D70` (the "%d" format string) is a splat auto-name drifted -8
 *    bytes in this run (a whole D_80097D68..D_80097D90 chain shares the
 *    offset — verified against the .map). Bound a fresh
 *    `D_80097D70 = 0x80097D70;` in config/symbols.main.exe.txt per the
 *    cookbook's drifted-symbol recipe rather than fight the wrong auto-name.
 */
extern char D_80097D70[];   /* "%d" */
extern char D_80014018[];   /* "select camera owner" */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void sprintf(char *s, char *fmt, ...);

void SelectCameraOwnerOption(void)
{
    int i;
    TAdtSelect targets[36];
    u8 msg[35][10];

    if (Humans < 0x23)
    {
        for (i = 0; i < Humans; i++)
        {
            sprintf((char *)msg[i], D_80097D70, i);
            targets[i].name = msg[i];
            targets[i].value = (u_long)HumanGroup[i];
        }
        targets[i].name = NULL;
        CamState.Owner = (Humanoid *)AdtSelect(D_80014018, targets, 0);
        ViewInfo.vrx = CamState.Owner->model->locate.coord.t[0];
        ViewInfo.vry = CamState.Owner->model->locate.coord.t[1];
        ViewInfo.vrz = CamState.Owner->model->locate.coord.t[2];
        ViewInfo.vpx = ViewInfo.vrx;
        ViewInfo.vpy = ViewInfo.vry - 5000;
        ViewInfo.vpz = ViewInfo.vrz;
    }
}
