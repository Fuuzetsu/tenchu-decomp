#include "common.h"
#include "main.exe.h"

/*
 * Load an executable from CD, waiting first for a successful open and then
 * for the asynchronous read to finish.  The caller supplies the stack fields
 * written into the SDK execution record immediately before the hand-off.
 *
 * The two INCLUDE_ASM pieces were one function: the interior prototype marker
 * sits at the printf call in the retry loop, and its branches cross the split.
 */
extern char D_8001484C[];
extern char D_80014860[];
extern void FUN_8005e948(void);
extern void VSyncCallback(void (*func)(void));
extern int printf(char *fmt, ...);
extern EXEC *CdReadExec(u8 *name);
extern int CdReadSync(s32 mode, u8 *result);
extern void StopCallback(void);
extern void Exec(EXEC *exec, s32 argc, char **argv);

void run_exec_file(u8 *name, u32 stack, u32 size)
{
    EXEC *exec;

    VSyncCallback(FUN_8005e948);
    do {
        do {
            printf(D_8001484C, name);
            exec = CdReadExec(name);
        } while (exec == NULL);
    } while (CdReadSync(0, NULL) != 0);

    VSyncCallback(NULL);
    printf(D_80014860);
    exec->s_addr = stack;
    exec->s_size = size;
    StopCallback();
    Exec(exec, 0, NULL);
}

// triage: MEDIUM — 47 insns, 2 loop, 6 callees, ~0.20 to AddXG4
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005e834(char *param_1,undefined4 param_2,undefined4 param_3)
//
// {
//   EXEC *pEVar1;
//   int iVar2;
//
//   VSyncCallback(FUN_8005e948);
//   do {
//     do {
//       printf("reading exec %s\n",param_1);
//       pEVar1 = CdReadExec(param_1);
//     } while (pEVar1 == (EXEC *)0x0);
//     iVar2 = CdReadSync(0,(u_char *)0x0);
//   } while (iVar2 != 0);
//   VSyncCallback((f *)0x0);
//   printf("exe read ok\n");
//   *(undefined4 *)(pEVar1 + 0x20) = param_2;
//   *(undefined4 *)(pEVar1 + 0x24) = param_3;
//   StopCallback();
//   Exec(pEVar1,0,(char **)0x0);
//   return;
// }
