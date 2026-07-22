#include "common.h"
#include "main.exe.h"

/*
 * initialise_default_player_cameras_ (0x80031fcc, 0xBC bytes) — reset the
 * player camera's tunable SVECTORs (CAMERA_R1/R2/P1/P2, 4 consecutive
 * 8-byte SVECTORs at 0x80089f30) from a compiled-in defaults table
 * D_80011BC0 (rodata sitting right before the CameraType1/SetCameraMode/
 * Camera switch-table data at 0x80011be0+), then (re)point CAMERA_POINTERS'
 * 4 slots at CAMERA_PTR_ARRAY_START's 4 consecutive 8-byte elements.
 * No frame, no calls — pure data moves.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The whole R1/R2/P1/P2 span is ONE 32-byte struct assignment, not 4
 *    separate SVECTOR globals: only ONE lui+addiu pair appears for each of
 *    the source/dest bases ($t0/$a3), reused via increasing constant
 *    displacement for all 4 elements — 4 independently-named externs would
 *    each need their OWN address materialization (cc1 can't know unrelated
 *    symbols land adjacently). CAMERA_R1's type spans all 4 fields;
 *    CAMERA_R2/P1/P2 stay defined (unreferenced here) via config/symbols
 *    for whichever TU names them individually (AdtFntLoad/AdtQuiet/
 *    AdtFntOpen precedent for one shared global with per-TU-sized views).
 *  - The align-2 (SVECTOR) element type forces the lwl/lwr+swl/swr
 *    block-copy idiom (see UpdateOrnament.c's SVECTOR struct-copy note),
 *    one pair per word, 8 pairs total for the 32 bytes.
 *  - CAMERA_PTR_ARRAY_START is a plain `u8 *` (byte-stride pointer): the
 *    three offsets are raw +8/+0x10/+0x18 additions, not a scaled index.
 */
extern u8 *CAMERA_PTR_ARRAY_START;
extern void *CAMERA_POINTERS[4];

void initialise_default_player_cameras_(void)
{
    CAMERA_R1 = D_80011BC0;
    CAMERA_POINTERS[0] = CAMERA_PTR_ARRAY_START;
    CAMERA_POINTERS[1] = CAMERA_PTR_ARRAY_START + 8;
    CAMERA_POINTERS[2] = CAMERA_PTR_ARRAY_START + 0x10;
    CAMERA_POINTERS[3] = CAMERA_PTR_ARRAY_START + 0x18;
}
