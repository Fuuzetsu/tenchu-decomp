#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "sound.h"

/*
 * PauseProc (0x8004b4c0) — the in-game pause loop. Entered every frame;
 * Start (or a dead player, get_pad_active_ == 0) raises the pause flag
 * (`SYSFLAG_PAUSE`), then this spins: polling the pad, feeding new presses
 * to the combo matcher (0x10 = revive cheat, 0x1000 = debug enable) and the
 * cheat-code recorder, Select opening the debug menu when debug is enabled,
 * Start unpausing, and DrawPause/VSync ticking the frame counter.
 *
 * Matching notes (all verified against the original bytes):
 *  - All pad state is s16 (pad/cur/opad/trig): the plain `opad != 0` test
 *    compiles to sll+beqz (sign-extension with the sra dropped for a zero
 *    test) — u16 vars would emit andi 0xffff. Likewise buf must be SIGNED
 *    s16[]: the 0xffff terminator store materializes as the HImode-canonical
 *    `li -1` (addiu) only for a signed element; a u16 element gives
 *    `ori 0xffff` (one byte off).
 *  - trig is computed fresh ($s1, feeds only the combo-call argument) and then
 *    copied into opad (`opad = trig;`, $s2) which the rest of the body reads:
 *    two registers holding one value = an explicit source copy (cc1 never
 *    splits live ranges).
 *  - `cur == (PADstart | PADselect)` and the call argument share one sign-extension
 *    of cur, CSE'd into callee-saved $s0 because it lives across
 *    return_to_menu_().
 *  - com is int, not short: the combo matcher's short return is extended
 *    once at the assignment (sll/sra straight into $a1, before the
 *    status==7/mid>0x713 override) and both == compares then run on the word.
 *    A short com would re-extend at the compares, after the join.
 *  - Every exit path `break`s and SsSetMVol(0x7f,0x7f) is written ONCE after
 *    the loop: the after-loop block is entered by fallthrough and reorg
 *    steals its `li a0,0x7f` into the three break-jumps' empty delay slots
 *    (.L810 pattern); the revive path's slot is taken by the pad.data sh, so
 *    it jumps to the li itself (.L80C). Writing the call in each path
 *    cross-jumps less (the arg li gets scheduled away from the call and the
 *    suffixes stop matching) — +6 instructions.
 *  - `cur = pad;` sits before `SkipFrame = SKIPFRAME_AFTER_LOAD;` — its move is what reorg puts
 *    in the pause-flag beqz delay slot.
 *  - The recorder increments through a short temp BEFORE the call:
 *    `j = i + 1; i = j; CheckCheatCodes(buf, j + 1);` reproduces
 *    `addiu a1,s7,1; addu s7,a1; …` with a1 dead at the call. Ghidra's order
 *    (call, then i = i + 1) sends the shared i+1 pseudo across the call into
 *    a callee-saved reg with a post-call move (the scheduler never hoists
 *    across calls); `i = i + 1;` first + `(short)i + 1` collapses to an
 *    in-place addiu, one insn short. The narrow adds stay raw (HImode) while
 *    buf's indexes reuse the bound-check's sign-extension (v1).
 *  - &CamState and buf are hoisted into $s4/$s5 by loop.c (while(1) keeps
 *    loop notes — no source temps); CamState.Owner is re-read at every use
 *    and the sh through Owner->life kills cse's memory equivalence, which is
 *    exactly the original's reload pattern.
 *  - The unpause wait is the cookbook's top-test shape:
 *    while(1) { if (!(GetRealPad(PAD_PORT_1) & PADstart)) break; VSync(2); }.
 */
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

/* Retail's own prototype drift (def: u8 get_pad_active_(short)) -- byte-required. */
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
