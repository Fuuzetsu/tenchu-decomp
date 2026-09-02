#include "common.h"
#include "main.exe.h"
#include "filesystem.h"
#include "adt.h"
#include "item.h"
#include "padcmd.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int main(void);
 *     START.C:102, 167 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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

extern control_scheme ControlScheme;
extern char fmt_free_memory[];

extern void ResetCallback(void);
extern void InitFileSystem(file_read_mode mode);
extern void InitGraphicsSystem(void);
extern void InitAccessInfo(void);
extern void InitConflict(void);
extern void InitEffect(void);
extern void InitializeInfoView(void);
extern void InitSoundEffect(void);
extern void DemoPatchInit(void);
/* Retail calls the s32-returning definition through a void declaration. */
extern void InitPersistentState(void);
extern void CreateStage(stage_id stage, s32 chr);
extern void clear_pad_send_(void);
extern void PadProc(void);
/* Retail calls the s32-returning definition through an s16 declaration. */
extern short StageSequence(void);
extern void StageEndScreen(void);
extern void game_over_screen_(void);
extern void Camera(void);
extern void ActivateHumans(void);
extern void DrawConstruction(void);
extern void DrawEffect(void);
extern void DoItemProc(void);
extern void DoInfoViewProc(void);
extern void DoMiscProc(void);
extern void draw_visible_characters_(void);
int main(void)
{
    short seq;
    u32 pad;
    TLinkInfo *ps;
    u8 dead[0xF8];

    AdtPadRead = GetRealPad;
    ResetCallback();
    InitPadControl();
    InitFileSystem(READ_SOURCE_CDROM);
    CdaStatus.flag = CDA_FLAG_ACTIVE;
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
    ControlScheme = ps->control_scheme;
    CreateStage(ps->StageNo, ps->CharType);
    clear_pad_send_();
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
                game_over_screen_();
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
        draw_visible_characters_();
        pad = GetPad(PAD_CONTROLLER_1);
        if ((pad & PADselect) != 0 && SkipFrame == 0)
        {
            FntPrint(fmt_free_memory, vgetfreesize(), vgetmaxsize());
        }
        if (SkipFrame != SKIPFRAME_SKIPPED && (SystemFlag & SYSFLAG_DEBUGPRINT) != 0)
        {
            FntFlush(-1);
        }
        EndDrawing(-2);
    } while (1);
}
