#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR *dtL;
 * END PSX.SYM */

/*
 * FUN_80027304 (0x80027304) — publish the current motion model's ground
 * position into `dtL` (a VECTOR*, vx/vz only — skips vy, same
 * copy-two-of-three-fields shape as GetConflictResult's ConflictDistance)
 * when its id is a valid conflict-pool slot.
 * `Me_MOTION_C->model->object[0]` is the first sub-model of the current
 * character's active ModelArchiveType (item.h); its `id` indexes ConflictObject,
 * the same conflict pool typed in GetConflictResult.c. Array INDEXING (not
 * just a pointer's leading fields) needs the real element stride, so unlike
 * a DisposeBG-style truncated pad view, this local copy must keep the FULL
 * 0x78-byte layout (a truncated 0x14-byte version computed id*0x14 instead
 * of id*0x78 — confirmed by the first matchdiff attempt) even though only
 * `model`/`position` are read here.
 * gp: Me_MOTION_C and dtL are gp-relative in this TU (tools/gpsyms.py) —
 * Me_MOTION_C was already in NowReturnNormal's list (same original TU);
 * dtL added here via `tools/gpsyms.py FUN_80027304 --write`.
 */
extern VECTOR *dtL;
extern Humanoid *Me_MOTION_C;

void FUN_80027304(void)
{
    s32 id;

    id = (*Me_MOTION_C->model->object)->id;
    if (id >= 0)
    {
        dtL->vx = ConflictObject[id].position.vx;
        dtL->vz = ConflictObject[id].position.vz;
    }
}

// triage: TRIVIAL — 26 insns, 0 callees, ~0.10 to load_font_image_into_global
// likely-relevant cookbook sections:
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80027304(void)
//
// {
//   VECTOR *pVVar1;
//   int iVar2;
//
//   pVVar1 = dtL;
//   iVar2 = (int)(*Me_MOTION_C->model->object)->id;
//   if (-1 < iVar2) {
//     dtL->vx = ConflictObject[iVar2].position.vx;
//     pVVar1->vz = ConflictObject[iVar2].position.vz;
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// extern ? ConflictObject;
// extern void *Me_MOTION_C;
// extern void *dtL;
//
// void FUN_80027304(void) {
//     s16 temp_a0;
//     void *temp_v0;
//
//     temp_a0 = (*Me_MOTION_C->unk58->unk68)->unk58;
//     if (temp_a0 >= 0) {
//         temp_v0 = (temp_a0 * 0x78) + &ConflictObject;
//         dtL->unk0 = (s32) temp_v0->unk4;
//         dtL->unk8 = (s32) temp_v0->unkC;
//     }
// }
