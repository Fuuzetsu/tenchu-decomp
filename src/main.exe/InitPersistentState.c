#include "common.h"
#include "main.exe.h"
#include "item.h"

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

extern void SsSetMono(void);
extern void SsSetStereo(void);
extern void SelectStage(TLinkInfo *ps);

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
