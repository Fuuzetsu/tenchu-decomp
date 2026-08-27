#include "common.h"
#include "main.exe.h"

/*
 * AfsFilenameFix (0x8005eee4, 0x50 bytes) — in-place uppercases a
 * NUL-terminated string (an AFS entry name):
 * `while (*path) { *path = toupper(*path); path++; }`.
 *
 * Matching notes: the guard test must read the raw PARAMETER (`path`, $a0)
 * and the loop cursor `p` must be assigned INSIDE the guarded `if`, right
 * before the `do`-loop — not as an unconditional `p = path;` ahead of the
 * `if`. Both spellings are semantically identical (p is unused unless the
 * guard passes), but only the "assign-inside-the-if" placement puts the
 * `move $s0,$a0` copy as the first RTL insn of the fallthrough (guard-taken)
 * block, which reorg then pulls BACKWARD into the guard branch's delay slot
 * (the target's shape: `lbu v0,0(a0); beqz v0,end; [delay] move s0,a0`).
 * Writing the assignment ahead of the `if` (or folding it into the `while`
 * condition, Ghidra-style with a `cVar1` temp) schedules the copy as the
 * very first instruction after the prologue instead — 57 bytes differ
 * (prologue store order flips too: our `sw s0` before `sw ra` vs target's
 * `sw ra` before `sw s0`), even though the instruction COUNT is identical.
 */

extern int toupper(int c);

void AfsFilenameFix(char *path)
{
    char *p;

    if (*path != 0)
    {
        p = path;
        do
        {
            *p = toupper(*p);
            p++;
        } while (*p != 0);
    }
}
