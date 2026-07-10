#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int CdaReady(void);
 *     OPAUDIO.C:46, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

/*
 * CdaReady (0x8004fbe4, 0x10 bytes) — tests bit5 of CdaStatus.status, the
 * "CD-audio stream primed and playable" flag. Same proven TCdaStatus struct as
 * CdaStop.c/FUN_8004fbf4.c/FUN_8004fc08.c. Returns the masked bit rather than a
 * normalised 0/1, so no extra andi follows the lbu.
 */
typedef struct
{
    s32 StartPos;   /* 0x00 */
    s32 CurPos;     /* 0x04 */
    s32 EndPos;     /* 0x08 */
    s16 mode;       /* 0x0C */
    s16 CheckCount; /* 0x0E */
    u8 status;      /* 0x10 */
    u8 voll;        /* 0x11 */
    u8 volr;        /* 0x12 */
    u8 flag;        /* 0x13 */
    u8 field9_0x14; /* 0x14 */
} TCdaStatus;

extern TCdaStatus CdaStatus;

s32 CdaReady(void)
{
    return CdaStatus.status & 0x20;
}
