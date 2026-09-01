#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * SetupCharacterParameter(short type, struct Humanoid *human);
 *     APPEAR.C:173, 25 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * .type == CHARACTER_KIND_END) and copies the per-type stats
 * (turn/width/height/life) plus
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
 *  - The chained life/lifemax assignment reads `HumanData[idx].life` once
 *    and stores lifemax before life (one `lhu` feeding two `sh`s). Two plain
 *    assignments reload the table field.
 */

Humanoid *SetupCharacterParameter(character_kind type, Humanoid *human)
{
    int idx;
    character_kind *idtbl;

    idx = 0;
    while (HumanData[idx].type != CHARACTER_KIND_END)
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
    human->life = human->lifemax = HumanData[idx].life;

    idx = -1;
    /* (u16): the sltiu range test is in the bytes. */
    if ((u16)type > 1)
    {
        idtbl = StageAppearance[NowStage];
        idx = 0;
        while (idtbl[idx] != type)
        {
            if (idtbl[idx] == CHARACTER_KIND_END)
            {
                break;
            }
            idx++;
        }
    }
    human->sound = (idx + 6) * 0x10 /* SE bank: 0x50 for player/partner (idx -1), 0x60+ per stage */;
    return human;
}
