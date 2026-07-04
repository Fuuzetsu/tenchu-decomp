#include "common.h"
#include "main.exe.h"

/* CD-audio status (Ghidra: TCdaStatus). Only voll/volr are touched here. */
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

void FUN_8004fbf4(u8 voll, u8 volr)
{
    CdaStatus.voll = voll;
    CdaStatus.volr = volr;
}
