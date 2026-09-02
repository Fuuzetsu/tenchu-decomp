#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "sound.h"

#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PauseProc(void);
 *     INFOVIEW.C:1261, 56 src lines, frame 56 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short pad
 *     reg   $s2       short ForbiddenCount
 *     reg   $s1       short push
 *     reg   $v1       short trig
 *     stack sp+24     struct RECT rc
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern struct TCameraStatus CamState;
 *     extern short ActionHalt;
 *     extern short Findenemies;
 * END PSX.SYM */

/* Retail declares s16(s32) here; get_pad_active_ defines u8(s16). */
extern short get_pad_active_(s32 arg);
extern void return_to_menu_(void);
extern short check_cheat_command_(short pad, short trg);
extern void CheckCheatCodes(s16 *rec, int n);
extern void DrawPause(int frame);
extern int VSync(int mode);
extern void SsSetMVol(int voll, int volr);

void PauseProc(void)
{
    s16 pad;
    s16 cur;
    s16 opad;
    s16 trig;
    int com;
    s16 i;
    s16 cnt;
    s16 j;
    s16 buf[0x21];

    pad = GetPad(PAD_CONTROLLER_1);
    i = 0;
    cnt = 0;
    if (((pad & PADstart) && !(SystemFlag & SYSFLAG_PAUSE)) ||
        get_pad_active_(PAD_CONTROLLER_1) == 0)
    {
        SystemFlag = (SystemFlag | SYSFLAG_PAUSE) & ~SYSFLAG_DEBUG_SELECT;
        SoundEx((VECTOR *)0, SE_PAUSE_ENTER);
        VSync(0x14);
    }
    if (!(SystemFlag & SYSFLAG_PAUSE))
        return;
    cur = pad;
    SkipFrame = SKIPFRAME_AFTER_LOAD;
    SsSetMVol(MASTER_VOLUME_MUTE, MASTER_VOLUME_MUTE);
    while (1)
    {
        opad = cur;
        PadProc();
        cur = GetPad(PAD_CONTROLLER_1);
        trig = cur & (cur ^ opad);
        opad = trig;
        if (cur == (PADstart | PADselect))
            return_to_menu_();
        com = check_cheat_command_(cur, trig);
        /* Motion ids above the taunt (0x713) are the stealth-kill
         * finishers — no cheating mid-finisher. */
        if (CamState.Owner->status == STAT_ATTACK && CamState.Owner->motion->mid > MOT_ATTACK_TAUNT)
            com = CHEAT_NONE;
        if (com == CHEAT_REVIVE)
        {
            if (CamState.Owner->status != STAT_DEAD)
            {
                CamState.Owner->life = CamState.Owner->lifemax;
                dispose_weapon_data_of_char_(CamState.Owner,
                                             ATTACK_CANCEL_ALL);
                CamState.Owner->status = STAT_NORMAL;
                ActionHalt = ACTION_HALT_NONE;
                Sound(CamState.Owner, SE_ITEM_USE);
                SetCameraMode(CMODE_NORMAL);
                Findenemies++;
                SystemFlag = SystemFlag & ~SYSFLAG_PAUSE;
                CamState.Owner->pad.data = PADRleft;
                break;
            }
            continue;
        }
        if (com == CHEAT_DEBUG_MENU)
        {
            SystemFlag = SystemFlag | SYSFLAG_DEBUGMODE;
            SoundEx((VECTOR *)0, SE_MENU_CONFIRM);
            break;
        }
        if (opad & PADstart)
        {
            while (1)
            {
                if (!(GetRealPad(PAD_PORT_1) & PADstart))
                    break;
                VSync(2);
            }
            SoundEx((VECTOR *)0, SE_MENU_CONFIRM);
            SystemFlag = SystemFlag & ~SYSFLAG_PAUSE;
            break;
        }
        if ((opad & PADselect) && (SystemFlag & SYSFLAG_DEBUGMODE))
        {
            SystemFlag = SystemFlag | SYSFLAG_DEBUG_SELECT;
            break;
        }
        if (opad != 0)
        {
            if (i < 0x20)
            {
                buf[i] = opad;
                buf[i + 1] = -1;
                j = i + 1;
                i = j;
                CheckCheatCodes(buf, j + 1);
            }
        }
        if ((SystemFlag & (SYSFLAG_DEBUGMODE | SYSFLAG_DEBUG_SELECT)) !=
                (SYSFLAG_DEBUGMODE | SYSFLAG_DEBUG_SELECT) ||
            (pad & PADstart))
            DrawPause(cnt);
        VSync(2);
        cnt++;
    }
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
}
