#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void LoadExecEx(unsigned char *file, unsigned long stack, unsigned long size);
 *     INFOVIEW.C:1173, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * file
 *     param $a1       unsigned long stack
 *     param $a2       unsigned long size
 * END PSX.SYM */

extern void save_pad_analog_(void);
extern void CdaStop(void);
extern void SsEnd(void);
extern void SsQuit(void);
extern void PadStopCom(void);
extern void MemCardStop(void);
extern void MemCardEnd(void);
extern void StopCallback(void);
extern void set_boot_exec_(u8 *file, u32 stack, u32 size);
extern void run_exec_file(u8 *name, u32 stack, u32 size);
extern char path_tenchu_run_exe_1[]; /* \\TENCHU\\RUN.EXE;1 */

void LoadExecEx(u8 *file, u32 stack, u32 size)
{
    save_pad_analog_();
    CdaStop();
    SsEnd();
    SsQuit();
    ResetGraph(3);
    PadStopCom();
    MemCardStop();
    MemCardEnd();
    StopCallback();
    set_boot_exec_(file, stack, size);
    CdInit();
    run_exec_file(path_tenchu_run_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
}
