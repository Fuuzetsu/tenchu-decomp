#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * SetupCharacterParameter(short type, struct Humanoid *human);
 *     APPEAR.C:173, 25 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       short type
 *     param $s1       struct Humanoid * human
 *     reg   $a0       int idx
 *     reg   $a2       short * idtbl
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern short NowStage;
 *     extern short *StageAppearance[10];
 * END PSX.SYM */

/*
 * SetupCharacterParameter (0x80029ea4, 0x174 bytes) — resolves `type` to its
 * row in the sentinel-terminated HumanData[] table (linear search on
 * .type == -1) and copies the per-type stats (turn/width/height/life) plus
 * motion setup (SetupMotionRegist/SetupMotionManager) into `human`; then
 * resolves a second, unrelated "how manieth non-player kind on this stage"
 * count via StageAppearance[NowStage] (another sentinel-
 * terminated, per-stage short list) into human->sound. item.h's proven
 * Humanoid layout (turn@0x6/life@0x8/lifemax@0xA/width@0xC/height@0xE/
 * model@0x58/motion@0x5C/sound@0xAC) accounts for every field written here.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `int idx` (not `short`) lets loop.c strength-reduce the repeated
 *    `HumanData[idx].type` into a walking pointer INSIDE the first loop
 *    (the target's `addiu $v1,$v1,0x18` stride) while `idx` itself survives
 *    the loop (recomputed via `idx*0x18` afterward, since the giv pointer
 *    dies at loop exit) for the later `HumanData[idx].*` field reads —
 *    exactly the Loops "recompute-from-base" + coexisting biv/giv shape.
 *    A NAMED temp/pointer local for the compared field (Ghidra's own
 *    `sVar1`/`sVar2`/`pHVar4`) is a decompiler SSA artifact here, not real
 *    source — introducing one (tried a `sid`/`val` temp and a `phum`
 *    walking pointer literally matching Ghidra's rendering) makes cc1
 *    hoist/CSE the loop's OWN first-iteration test into an extra branch
 *    right after the entry guard, one instruction too many. Plain re-
 *    indexed `HumanData[idx].type` / `idtbl[idx]` in BOTH the loop
 *    condition and the body's break test is what reproduces the target's
 *    two separate `lh` reloads (one per test) with no extra branch.
 *  - `idtbl` (proven `short *` by PSX.SYM) is indexed (`idtbl[idx]`),
 *    reusing the SAME `idx` from the first search, not walked with its own
 *    increment/pointer arithmetic — mirrors the first loop's shape exactly
 *    (an `int idx` giv lets loop.c strength-reduce this to a walking
 *    pointer too, matching the target's `addiu $v1,$v1,2` stride).
 *  - The two searches are mirror-image sentinel/match loop shapes: the
 *    first loops "while not at the sentinel" (breaking early on a match),
 *    the second loops "while not yet matched" (breaking early on the
 *    sentinel) — each a plain `while`, jump.c duplicating each loop's OWN
 *    continue-condition to the entry.
 *  - `HumanData[idx].life` is read ONCE into a temp and stored to both
 *    lifemax and life (one `lhu` feeding two `sh`s) — a plain two-statement
 *    transcription would reload it twice.
 */

extern HumanDataType HumanData[63];
extern s16 NowStage;
extern s16 *StageAppearance[];

Humanoid *SetupCharacterParameter(s16 type, Humanoid *human)
{
    int idx;
    s16 *idtbl;
    s16 life;

    idx = 0;
    while (HumanData[idx].type != -1)
    {
        if (HumanData[idx].type == type)
        {
            break;
        }
        idx++;
    }
    human->turn = HumanData[idx].turn;
    human->width = HumanData[idx].width;
    human->height = HumanData[idx].height;
    if (HumanData[idx].mtbl->motion == 0)
    {
        SetupMotionRegist(HumanData[idx].mtbl);
    }
    human->motion = SetupMotionManager(human->model, HumanData[idx].mtbl);
    life = HumanData[idx].life;
    human->lifemax = life;
    human->life = life;

    idx = -1;
    if (1 < (u16)type)
    {
        idtbl = StageAppearance[NowStage];
        idx = 0;
        while (idtbl[idx] != type)
        {
            if (idtbl[idx] == -1)
            {
                break;
            }
            idx++;
        }
    }
    human->sound = (idx + 6) * 0x10;
    return human;
}
