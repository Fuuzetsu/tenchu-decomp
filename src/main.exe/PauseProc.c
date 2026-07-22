#include "common.h"
#include "main.exe.h"

/*
 * PauseProc (0x8004b4c0) — the in-game pause loop. Entered every frame;
 * Start (or a dead player, FUN_8001b174 == 0) raises the pause flag
 * (`SYSFLAG_PAUSE`), then this spins: polling the pad, feeding new presses
 * to the combo matcher (0x10 = revive cheat, 0x1000 = debug enable) and the
 * cheat-code recorder, Select opening the debug menu when debug is enabled,
 * Start unpausing, and DrawPause/VSync ticking the frame counter.
 *
 * Matching notes (all verified against the original bytes):
 *  - All pad state is s16 (pad/cur/opad/trg): the plain `opad != 0` test
 *    compiles to sll+beqz (sign-extension with the sra dropped for a zero
 *    test) — u16 vars would emit andi 0xffff. Likewise buf must be SIGNED
 *    s16[]: the 0xffff terminator store materializes as the HImode-canonical
 *    `li -1` (addiu) only for a signed element; a u16 element gives
 *    `ori 0xffff` (one byte off).
 *  - trg is computed fresh ($s1, feeds only the combo-call argument) and then
 *    copied into opad (`opad = trg;`, $s2) which the rest of the body reads:
 *    two registers holding one value = an explicit source copy (cc1 never
 *    splits live ranges).
 *  - `cur == (START | SELECT)` and the call argument share one sign-extension
 *    of cur, CSE'd into callee-saved $s0 because it lives across
 *    FUN_800566fc().
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
 *  - `cur = pad;` sits before `SkipFrame = 2;` — its move is what reorg puts
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
 *    while(1) { if (!(GetRealPad(0) & START)) break; VSync(2); }.
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PauseProc(void);
 *     INFOVIEW.C:1261, 56 src lines, frame 56 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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

extern s16 GetPad(s16 no);
extern s32 GetRealPad(s32 port);
extern short FUN_8001b174(s32 arg);
extern void FUN_800566fc(void);
extern short check_for_known_button_combination(short pad, short trg);
extern void CheckCheatCodes(s16 *rec, int n);
extern void DrawPause(int frame);
extern int VSync(int mode);
extern void SsSetMVol(int voll, int volr);
extern short Sound(Humanoid *human, int seid);

extern TSystemFlag SystemFlag;
extern s16 SkipFrame;
extern u16 Findenemies;
extern TCameraStatus CamState;

void PauseProc(void)
{
    enum { START = 2048, SELECT = 256 };
    s16 pad;
    s16 cur;
    s16 opad;
    s16 trg;
    int com;
    s16 i;
    s16 cnt;
    s16 j;
    s16 buf[0x21];

    pad = GetPad(0);
    i = 0;
    cnt = 0;
    if (((pad & START) && !(SystemFlag & SYSFLAG_PAUSE)) || FUN_8001b174(0) == 0)
    {
        SystemFlag = (SystemFlag | SYSFLAG_PAUSE) & ~0x10;
        SoundEx((VECTOR *)0, 9);
        VSync(0x14);
    }
    if (!(SystemFlag & SYSFLAG_PAUSE))
        return;
    cur = pad;
    SkipFrame = 2;
    SsSetMVol(0, 0);
    while (1)
    {
        opad = cur;
        PadProc();
        cur = GetPad(0);
        trg = cur & (cur ^ opad);
        opad = trg;
        if (cur == (START | SELECT))
            FUN_800566fc();
        com = check_for_known_button_combination(cur, trg);
        if (CamState.Owner->status == 7 && 0x713 < CamState.Owner->motion->mid)
            com = 0;
        if (com == 0x10)
        {
            if (CamState.Owner->status != 0x11)
            {
                CamState.Owner->life = CamState.Owner->lifemax;
                dispose_weapon_data_of_char_(CamState.Owner, 3);
                CamState.Owner->status = 0;
                ActionHalt = 0;
                Sound(CamState.Owner, 0x4c);
                SetCameraMode(CMODE_NORMAL);
                Findenemies = Findenemies + 1;
                SystemFlag = SystemFlag & ~SYSFLAG_PAUSE;
                CamState.Owner->pad.data = 0x80;
                break;
            }
            continue;
        }
        if (com == 0x1000)
        {
            SystemFlag = SystemFlag | SYSFLAG_DEBUGMODE;
            SoundEx((VECTOR *)0, 10);
            break;
        }
        if (opad & START)
        {
            while (1)
            {
                if (!(GetRealPad(0) & START))
                    break;
                VSync(2);
            }
            SoundEx((VECTOR *)0, 10);
            SystemFlag = SystemFlag & ~SYSFLAG_PAUSE;
            break;
        }
        if ((opad & SELECT) && (SystemFlag & SYSFLAG_DEBUGMODE))
        {
            SystemFlag = SystemFlag | 0x10;
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
        if ((SystemFlag & (SYSFLAG_DEBUGMODE | 0x10)) !=
                (SYSFLAG_DEBUGMODE | 0x10) ||
            (pad & START))
            DrawPause(cnt);
        VSync(2);
        cnt = cnt + 1;
    }
    SsSetMVol(0x7f, 0x7f);
}
