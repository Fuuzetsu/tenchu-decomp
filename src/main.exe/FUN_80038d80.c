#include "common.h"
#include "main.exe.h"

/*
 * FUN_80038d80 (0x80038d80) — pokes 4 fields of an unnamed (likely GPU
 * primitive / OT-tag) struct: byte 0x3 = 1 (top/length byte of the 4-byte
 * tag word at offset 0 — only the length byte is set, matching the OT-tag
 * convention of a single following word), word @0x4 = a DR_TPAGE-style
 * 0xE1000000 draw-mode command with the semi-transparency rate (arg1 & 3)
 * placed in bits 5-6, and two more bytes at 0xB/0xF (likely a second
 * primitive's fields) set to fixed constants 8 and 0x3A. No proven struct
 * name exists for this layout, so plain offset casts (per the cookbook's
 * "no proven view" rule) — same shape as the Ghidra/m2c reference.
 */
void FUN_80038d80(void *arg0, s32 arg1)
{
    *((u8 *)arg0 + 0xB) = 8;
    *((u8 *)arg0 + 0xF) = 0x3A;
    *((u8 *)arg0 + 3) = 1;
    *(u32 *)((u8 *)arg0 + 4) = (arg1 & 3) << 5 | 0xE1000200;
}
