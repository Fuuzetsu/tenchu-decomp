#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * AfsGetHeader (0x8005f1c0, 0xB8 bytes) — reads the AFS volume header (a
 * fixed 0x28-byte block: an 11-char signature string, then two big-endian
 * u32 fields) from the start of the volume file into a stack buffer, and on
 * a signature match ("AFS_VOL_200") unpacks maxElements/posElement into the
 * TAFS handle, clearing fModified. Returns 0 on a match, 1 otherwise. Same
 * proven TAFS/FILE layout as AfsInit.c/AfsClose.c/cd_read.c/cd_seek.c.
 *
 * Matching notes:
 *  - The stack buffer is ONE `u8 buf[0x28]` (cd_read's own destination),
 *    not Ghidra's three overlapping locals (acStack_30/local_24/local_20) —
 *    cc1 2.8 never shares stack slots between sibling scopes (cookbook's
 *    "Stack objects" rule); the string compare and the two byte-packed u32
 *    fields are all views into the same buffer (offsets 0/0xC/0x10).
 *  - Both u32s are LEFT-associated `|` chains (`((b0<<24 | b1<<16) | b2<<8) |
 *    b3`); right-nesting them changes the shift/or order.
 *  - `posElement`'s value goes through a named local computed BEFORE
 *    `fModified = 0`, and only stored after it. That is what frees $v0 for the
 *    first chain's accumulator: `return 0`'s `move $v0,zero` then materializes
 *    between the two stores (right where `fModified = 0` sits) instead of
 *    ahead of the first chain, which would push that accumulator to $v1 and
 *    re-colour both chains. Writing the two field assignments in plain source
 *    order is a 61-byte, same-length miss — instruction-for-instruction
 *    identical, just re-scheduled and re-coloured. The permuter found this.
 */

extern int cd_seek(FILE *f, int offset, int whence);
extern int cd_read(FILE *f, void *buffer, int length);
extern int strcmp(const char *a, const char *b);
extern char D_800148C4[]; /* "AFS_VOL_200" — lives in this TU's unsplit data
                            * blob (splat auto-symbol), same pattern as
                            * AfsInit's D_80014944. */

int AfsGetHeader(TAFS *handle)
{
    u8 buf[0x28];
    u32 pos;

    cd_seek(handle->fpVol, 0, 0);
    cd_read(handle->fpVol, buf, 0x28);
    if (strcmp((char *)buf, D_800148C4) != 0) {
        return 1;
    }
    handle->maxElements = ((u32)buf[0xC] << 24) | ((u32)buf[0xD] << 16) |
          ((u32)buf[0xE] << 8) | (u32)buf[0xF];
    pos = ((u32)buf[0x10] << 24) | ((u32)buf[0x11] << 16) |
          ((u32)buf[0x12] << 8) | (u32)buf[0x13];
    handle->fModified = 0;
    handle->posElement = pos;
    return 0;
}
