#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromDEVPC(unsigned char *filename);
 *     FILEIO.C:214, 29 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned char * filename
 *
 * Globals it touches, as the original declared them:
 *     extern int TotalIO;
 *     extern int ReadMode;
 *     extern unsigned long *MemoryLoadAddress;
 * END PSX.SYM */

/*
 * LoadFromDEVPC (0x8001952c, 0x100 bytes) — loads a file over the PsyQ
 * host-PC link (PCopen/PClseek/PCread/PCclose, part of the 0x80060xxx
 * precompiled SDK block — see PClseek.c). Bumps TotalIO, opens the file,
 * seeks to the end to get its size; on open or size failure reports via
 * AdtMessageBox and returns NULL. On success, optionally logs (ReadMode &
 * 4), rewinds, takes MemoryLoadAddress as a pre-supplied buffer if set
 * (consuming it — clears it back to NULL) or valloc()s a fresh one, reads
 * the file into it, closes, and returns it.
 *
 * Matching notes: the raw asm shows two SEPARATE early-out branches (`beq
 * fd,-1`, then `blez size`) that share the same error tail, not Ghidra's
 * single `||` condition with a comma-expression inside — write nested ifs
 * (fd != -1) { size = ...; if (size > 0) { ...; return buff; } } falling
 * through to the shared AdtMessageBox/return-0 tail (m2c's `goto block_9`
 * shows the same two-branch shape more literally than Ghidra's `||`).
 * `filename` is never reassigned before the PCopen call (stays live in
 * $a0 straight from entry, like FileWrite's `filename` — PCopen is called
 * with it as the raw incoming register); m2c misses this leading argument
 * (shows `PCopen(0, 0)`), a known m2c under-count.
 *
 * `buff = MemoryLoadAddress;` belongs INSIDE the else-branch, not hoisted
 * before the if the way Ghidra (and m2c) render it: the asm loads
 * MemoryLoadAddress once for the compare and reuses that same value for
 * the assignment only via the branch's delay slot (`bnez v0,L; move
 * s0,v0` — harmless on the then-arm since valloc()'s result overwrites it
 * right after). Writing the hoisted form lets cc1 CSE the compare back
 * onto buff's own register instead (`lw s0,...; bnez s0,...`), a 6-byte
 * pure register-content miss. Another instance of "trust the assembly
 * over Ghidra's statement order" (the hoist is an SSA artifact — buff is
 * unconditionally overwritten on the other arm anyway).
 */
extern int PCopen(char *name, int mode, int share);
extern int PClseek(int fd, int offset, int whence);
extern int PCread(int fd, void *buf, int size);
extern int PCclose(int fd);
extern void *valloc(u32 size);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80011110[]; /* "%$LOAD(PC)\n%d[%s]" */
extern char D_80011124[]; /* "LOAD(PC) ERROR\n%d[%s]" */
extern s32 TotalIO;
extern s32 ReadMode;
extern u_long *MemoryLoadAddress;

u_long *LoadFromDEVPC(u8 *filename)
{
    s32 fd;
    s32 size;
    u_long *buff;

    TotalIO = TotalIO + 1;
    fd = PCopen((char *)filename, 0, 0);
    if (fd != -1) {
        size = PClseek(fd, 0, 2);
        if (size > 0) {
            if (ReadMode & 4) {
                AdtMessageBox(D_80011110, TotalIO, filename);
            }
            PClseek(fd, 0, 0);
            if (MemoryLoadAddress == 0) {
                buff = (u_long *)valloc(size);
            } else {
                buff = MemoryLoadAddress;
                MemoryLoadAddress = 0;
            }
            PCread(fd, buff, size);
            PCclose(fd);
            return buff;
        }
    }
    AdtMessageBox(D_80011124, TotalIO, filename);
    return 0;
}
