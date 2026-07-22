#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int main(void);
 *     START.C:102, 167 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s3       short i
 *     reg   $s2       unsigned long * dat
 *     stack sp+24     struct RECT rect
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short SkipFrame;
 * END PSX.SYM */

/*
 * main (0x800162a4, START.C) — the game entry point: one-time subsystem init,
 * then an infinite per-frame loop (pad -> stage sequence -> physics -> draw).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - NO explicit `__main()` call: gcc inserts the C-runtime `__main()` at the
 *    top of `main` automatically, and reorg fills its delay slot with the s0
 *    save. Writing `__main();` compiles a SECOND call (2 bytes over).
 *  - `u8 dead[0xF8]` is a DEAD 248-byte local — the retail frame is 0x118 with
 *    NO stack access beyond the 3 register saves, the "unexplained frame-size
 *    gap = unused aggregate" pattern. Neither Ghidra nor m2c shows it (both
 *    optimise it away); only the frame size proves it. The demo's own locals
 *    (`i`/`dat`/`rect`) were removed in retail.
 *  - CHOSEN_STAGE(+5)/CHOSEN_CHARACTER(+4)/PersistentState[0x5F] are read
 *    through ONE `TLinkInfo *ps = (TLinkInfo *)0x80010000;`
 *    assigned right before CreateStage, so cc1 shares a single transient
 *    `%hi(0x8001)` base (the bare-lui rule) instead of re-materialising it per
 *    global; declaring `ps` at function top instead pins it in a callee-saved
 *    register across the whole init sequence (wrong).
 *  - `CdaStatus.flag` is the byte at CdaStatus+0x13 (0x8008EA53); StageSequence
 *    returns `short` (the `sll 16 / sra 16` sign-extend of its result feeds the
 *    `== 1` / `== -1` tests). The constant 1 is hoisted into a callee-saved
 *    register (`s1`) by loop.c as a loop invariant shared by the `seq == 1`
 *    and `SkipFrame == 1` tests.
 */

extern s32 (*AdtPadRead)(s32 port);
extern s32 GetRealPad(s32 port);
extern TSystemFlag SystemFlag;
extern short Humans;
extern short SkipFrame;
extern u16 D_800976F6;
extern char D_80011004[];

extern void ResetCallback(void);
extern void InitFileSystem(s32 mode);
extern void InitGraphicsSystem(void);
extern void InitAccessInfo(void);
extern void KillHumanoid(Humanoid *h);
extern void InitConflict(void);
extern void InitEffect(void);
extern void InitializeInfoView(void);
extern void InitSoundEffect(void);
extern void DemoPatchInit(void);
extern void InitPersistentState(void);
extern void CreateStage(s32 stage, s32 chr);
extern void FUN_8001b4bc(void);
extern void PadProc(void);
extern short StageSequence(void);
extern void StageEndScreen(void);
extern void start_demo_(void);
extern void ComputeAllConflict(void);
extern void Camera(void);
extern short ControlAllHumanoid(void);
extern void ActivateHumans(void);
extern void DrawConstruction(void);
extern void DrawEffect(void);
extern void DoItemProc(void);
extern void DoInfoViewProc(void);
extern void DoMiscProc(void);
extern void update_something_for_each_visible_enemy_(void);
extern s16 GetPad(s16 no);
extern void *vgetfreesize(void);
extern void *vgetmaxsize(void);

int main(void)
{
    short seq;
    u32 pad;
    TLinkInfo *ps;
    u8 dead[0xF8];

    AdtPadRead = GetRealPad;
    ResetCallback();
    InitPadControl();
    InitFileSystem(2);
    CdaStatus.flag = 1;
    SystemFlag = 0;
    InitGraphicsSystem();
    InitAccessInfo();
    while (Humans != 0)
    {
        KillHumanoid(HumanGroup[0]);
    }
    InitConflict();
    InitEffect();
    InitializeItem();
    InitializeInfoView();
    InitMisc();
    InitSoundEffect();
    DemoPatchInit();
    InitPersistentState();
    ps = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    D_800976F6 = ps->field_0x5f[0];
    CreateStage(ps->StageNo, ps->CharType);
    FUN_8001b4bc();
    do
    {
        PadProc();
        seq = StageSequence();
        if ((SystemFlag & SYSFLAG_DEBUGPRINT) == 0)
        {
            if (seq == 1)
            {
                StageEndScreen();
            }
            else if (seq == -1)
            {
                start_demo_();
            }
        }
        ComputeAllConflict();
        StartDrawing();
        Camera();
        ControlAllHumanoid();
        ActivateHumans();
        DrawConstruction();
        DrawEffect();
        DoItemProc();
        DoInfoViewProc();
        DoMiscProc();
        update_something_for_each_visible_enemy_();
        pad = GetPad(0);
        if ((pad & 0x100) != 0 && SkipFrame == 0)
        {
            FntPrint(D_80011004, vgetfreesize(), vgetmaxsize());
        }
        if (SkipFrame != 1 && (SystemFlag & SYSFLAG_DEBUGPRINT) != 0)
        {
            FntFlush(-1);
        }
        EndDrawing(-2);
    } while (1);
}
