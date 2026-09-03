#include "common.h"
#include "main.exe.h"
#include "graphics.h"
#include "filesystem.h"
#include "adt.h"
#include "effect.h"
#include "item.h"
#include "padcmd.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>
#include <psxsdk/libsnd.h>

/*
 * Retail START.C emits two routines absent from the demo around the surviving
 * main function. The manifest records the distinct retail and demo orders.
 */

extern char fmt_free_memory[];
extern RECT BriefingVramRect[];

extern void SelectStage(TLinkInfo *ps);
extern void InitFileSystem(file_read_mode mode);
extern void InitAccessInfo(void);
extern void InitConflict(void);
extern void InitializeInfoView(void);
extern void InitSoundEffect(void);
extern void DemoPatchInit(void);
extern void CreateStage(stage_id stage, s32 chr);
extern void clear_pad_send_(void);
/* Retail calls the s32-returning definition through an s16 declaration. */
extern short StageSequence(void);
extern void StageEndScreen(void);
extern void game_over_screen_(void);
extern void Camera(void);
extern void ActivateHumans(void);
extern void DrawConstruction(void);
extern void DoInfoViewProc(void);
extern void DoMiscProc(void);
extern void draw_visible_characters_(void);
extern void BriefingAndInventorySelectionScreen(void);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gNannido;
 *     extern unsigned char gSound;
 *     extern unsigned char gSoundLevel;
 *     extern unsigned char gSELevel;
 *     extern unsigned char gfMemory;
 * END PSX.SYM */

s32 InitPersistentState(void)
{
    TLinkInfo *pg =
        (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    TLinkInfo *ps;
    s32 i;
    u32 magic;
    u8 fill;
    u8 *stockp;

    /* CharType is Rikimaru or Ayame, so any bit outside the playable
     * character index range means the saved slot is corrupt. */
    if ((pg->CharType & ~(N_PLAYABLE_CHARACTERS - 1)) != 0 ||
        pg->StageNo >= N_STAGE_CONFIGS)
    {
        memset((void *)TENCHU_PERSISTENT_STATE_ADDRESS, 0,
               TENCHU_PERSISTENT_STATE_SIZE);
        magic = 0x19981110;

        fill = ITEM_LOCKED;
        i = SAVE_ITEM_SLOTS - 1;
        stockp = (u8 *)(TENCHU_PERSISTENT_STATE_ADDRESS | i);
        ps = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
        ps->magic = magic;
        ps->Nannido = 0;
        ps->Stereo = SOUND_MODE_STEREO;
        ps->SoundLevel = SOUND_VOLUME_MAX;
        ps->SELevel = SOUND_VOLUME_MAX;
        ps->fMemory = 0;
        ps->Anakon = 1;
        ps->StageNoMAX[AYAME_0] = 1;
        ps->StageNoMAX[RIKIMARU_0] = 1;
        do
        {
            stockp[TLINKINFO_BYTE_OFFSET(gItem[0][0])] = fill;
            i--;
            stockp--;
        } while (i >= 0);
        ps->gItem[RIKIMARU_0][ITEM_KAGINAWA] = ITEM_INFINITE;
        ps->gItem[RIKIMARU_0][ITEM_SHURIKEN] = 6;
        ps->gItem[RIKIMARU_0][ITEM_MAKIBISHI] = 6;
        ps->gItem[RIKIMARU_0][ITEM_KUSURI] = 2;
        ps->gItem[RIKIMARU_0][ITEM_FIRE] = 1;
        ps->gItem[RIKIMARU_0][ITEM_SMOKE] = 1;
        ps->gItem[RIKIMARU_0][ITEM_DOKUDANGO] = 3;
        ps->gItem[RIKIMARU_0][ITEM_GOSHIKIMAI] = 5;
        __builtin_memcpy(&ps->gItem[AYAME_0][ITEM_KAGINAWA],
                         &ps->gItem[RIKIMARU_0][ITEM_KAGINAWA],
                         sizeof(ps->gItem) / N_PLAYABLE_CHARACTERS);
        ps->selItem[ITEM_SHURIKEN] = 10;
        ps->selItem[ITEM_KAGINAWA] = ITEM_INFINITE;
        ps->selItem[ITEM_MAKIBISHI] = 5;
        ps->selItem[ITEM_KUSURI] = 2;
        ps->layout = STAGE_LAYOUT_RANDOM;
        if (ps->Stereo != SOUND_MODE_MONO)
        {
            SsSetStereo();
        }
        else
        {
            SsSetMono();
        }
        SelectStage(ps);
        ps->control_scheme = 0;
        return 0;
    }
    return 1;
}

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
        if (SkipFrame != SKIPFRAME_SKIPPED &&
            (SystemFlag & SYSFLAG_DEBUGPRINT) != 0)
        {
            FntFlush(-1);
        }
        EndDrawing(-2);
    } while (1);
}

void DoBriefingAndInventorySelection(void)
{
    RECT r;
    u_long *p;

    r = BriefingVramRect[0];
    p = (u_long *)valloc(0x20000);
    StoreImage2(&r, p);
    ResetInventory();
    BriefingAndInventorySelectionScreen();
    LoadImage2(&r, p);
    vfree(p);
}
