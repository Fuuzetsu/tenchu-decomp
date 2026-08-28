#include "common.h"
#include "main.exe.h"

/*
 * initialise_default_player_cameras_ (0x80031fcc, 0xBC bytes) — reset the
 * player camera's tunable SVECTORs (CamPos/R2/P1/P2, 4 consecutive
 * 8-byte SVECTORs at 0x80089f30) from a compiled-in defaults table
 * CamPosDefault (rodata sitting right before the CameraType1/SetCameraMode/
 * Camera switch-table data at 0x80011be0+), then (re)point DEBUG_CAMERA_SLOTS_'
 * 4 slots at DEBUG_CAMERA_BASE_'s 4 consecutive 8-byte elements.
 * No frame, no calls — pure data moves.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The whole R1/R2/P1/P2 span is ONE 32-byte struct assignment, not 4
 *    separate SVECTOR globals: only ONE lui+addiu pair appears for each of
 *    the source/dest bases ($t0/$a3), reused via increasing constant
 *    displacement for all 4 elements — 4 independently-named externs would
 *    each need their OWN address materialization (cc1 can't know unrelated
 *    symbols land adjacently). CamPos's type spans all 4 fields;
 *    CAMERA_R2/P1/P2 stay defined (unreferenced here) via config/symbols
 *    for whichever TU names them individually (AdtFntLoad/AdtQuiet/
 *    AdtFntOpen precedent for one shared global with per-TU-sized views).
 *  - The align-2 (SVECTOR) element type forces the lwl/lwr+swl/swr
 *    block-copy idiom (see UpdateOrnament.c's SVECTOR struct-copy note),
 *    one pair per word, 8 pairs total for the 32 bytes.
 *  - DEBUG_CAMERA_BASE_ is a plain `u8 *` (byte-stride pointer): the
 *    three offsets are raw +8/+0x10/+0x18 additions, not a scaled index.
 */
extern u8 *DEBUG_CAMERA_BASE_;
extern void *DEBUG_CAMERA_SLOTS_[4];

void initialise_default_player_cameras_(void)
{
    CamPos = CamPosDefault;
    DEBUG_CAMERA_SLOTS_[0] = DEBUG_CAMERA_BASE_;
    DEBUG_CAMERA_SLOTS_[1] = DEBUG_CAMERA_BASE_ + 8;
    DEBUG_CAMERA_SLOTS_[2] = DEBUG_CAMERA_BASE_ + 0x10;
    DEBUG_CAMERA_SLOTS_[3] = DEBUG_CAMERA_BASE_ + 0x18;
}
