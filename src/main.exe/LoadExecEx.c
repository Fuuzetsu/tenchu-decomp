#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void LoadExecEx(unsigned char *file, unsigned long stack, unsigned long size);
 *     INFOVIEW.C:1173, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned char * file
 *     param $a1       unsigned long stack
 *     param $a2       unsigned long size
 * END PSX.SYM */

/*
 * LoadExecEx (0x80019448, 0xac bytes) — shuts down every game subsystem
 * (sound, graphics, pads, memcard, CD callback) before loading and jumping
 * to a fresh executable off the disc. The Ghidra callee `FUN_8005e834` is
 * the already-named `run_exec_file` (config/symbols.main.exe.txt); its
 * second argument is the fixed stack-top constant 0x801ffff0, not an
 * address-of (Ghidra's `&DAT_801ffff0` is a decompiler artifact for a bare
 * absolute literal here, not a real object being pointed to — the asm
 * builds it as a plain `lui/ori` 32-bit constant, never with a
 * `-G8`/`%lo`-style small-symbol relocation).
 */
extern void FUN_8001b2b8(void);
extern void CdaStop(void);
extern void SsEnd(void);
extern void FUN_8006ebe4(void);
extern void ResetGraph(int mode);
extern void PadStopCom(void);
extern void MemCardStop(void);
extern void MemCardEnd(void);
extern void StopCallback(void);
extern void FUN_8005e8f0(u8 *file, u32 stack, u32 size);
extern void CdInit(void);
extern void run_exec_file(u8 *name, u32 topaddr, s32 argc);
extern char D_800111D4[]; /* "\\TENCHU\\RUN.EXE;1" */

void LoadExecEx(u8 *file, u32 stack, u32 size)
{
    FUN_8001b2b8();
    CdaStop();
    SsEnd();
    FUN_8006ebe4();
    ResetGraph(3);
    PadStopCom();
    MemCardStop();
    MemCardEnd();
    StopCallback();
    FUN_8005e8f0(file, stack, size);
    CdInit();
    run_exec_file(D_800111D4, 0x801ffff0, 0);
}
