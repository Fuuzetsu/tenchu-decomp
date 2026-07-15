#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVAsequence(short sid);
 *     CHRANIM.C:83, 58 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       short sid
 *     reg   $s3       struct SoundEffect * vab
 *     reg   $a0       struct Humanoid * human
 *     reg   $s0       short sound
 *     reg   $s0       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAdata;
 *     extern struct CVAType *CVAnow;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 *     extern struct Humanoid *CameraTarget;
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short ActionHalt;
 *     extern short MotionUpdateMode;
 *     extern struct TCdaStatus CdaStatus;
 *     extern short VoiceMode;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — complete pure-C reconstruction with exact target
 * length (884 bytes / 221 instructions) and exact 0x28 frame.  Only seven
 * instructions (28 raw bytes) differ, in one post-memset scheduling block:
 * retail loads the event cursor into v0, StagePlayer into v1, then interleaves
 * the sound/index setup and the CameraTarget/Humans/cursor updates.  This cc1
 * source emits the same operations as StagePlayer(v0), cursor(v1), and moves
 * the independent Humans load earlier.  `rtlguide` classifies both displayed
 * hunks as structure/scheduling; guided autorules (120-candidate budget),
 * named cursor/player temporaries, a single volatile cursor read, statement
 * ordering, and s16*-versus-struct cursor types did not improve the retained
 * direct-global spelling.
 *
 * Recovered source-shape rules: the event scan must be a hand-rolled goto
 * loop so cc1 does not specialize the already-known first iteration; separate
 * s32 `wanted`/`end_kind` temps create retail's one preheader sign extension
 * and loop-carried -1 register.  The five-entry CVAhuman reset is a for loop;
 * an explicit base plus a separately initialized s32 0x80 class constant
 * gives retail's s2/s0 allocation and hoists the constant.  The motion arm's
 * `(motion = 0, test)` comma expression is load-bearing.  VoiceMode also
 * needed an explicit 0x80097CB4 binding because the generated auto-name had
 * drifted to the adjacent cursor at 0x80097CBC.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CVAsequence", CVAsequence);
#else

#include "item.h"

typedef struct
{
    Humanoid *human;
    s16 loop;
    s16 motid;
} CVASequenceHumanAnim;

typedef struct
{
    s16 kind;
    s16 mode;
    s16 x;
    s16 y;
    s16 z;
    s16 param;
} CVASequenceEvent;

typedef struct
{
    s32 StartPos;
    s32 CurPos;
    s32 EndPos;
    s16 mode;
    s16 CheckCount;
    u8 status;
    u8 voll;
    u8 volr;
    u8 flag;
    u8 field9_0x14;
} CVASequenceCdaStatus;

extern CVASequenceEvent *CVAdata;
extern CVASequenceEvent *CVAnow;
extern CVASequenceHumanAnim CVAhuman[5];
extern Humanoid *StagePlayer;
extern Humanoid *HumanGroup[32];
extern s16 Humans;
extern s16 ActionHalt;
extern s16 MotionUpdateMode;
extern CVASequenceCdaStatus CdaStatus;
extern u8 D_800C2C50[];
extern s16 VoiceMode;
extern Humanoid *CameraTarget;
extern s16 D_80097CC0;
extern s16 D_80097CCC;

extern void *memset(void *s, int c, u32 n);
extern void dispose_weapon_data_of_char_(Humanoid *human, s32 mode);
extern void NowReturnNormal(Humanoid *human);
extern s16 CVAupdate(void);
extern void PadShockAR(s32 port, s32 low, s32 high, s32 time);
extern void PadShock(s32 port, s32 power, s32 time);
extern void PadProc(void);
extern void PlayMusicFormID(s32 id);
extern s32 CdaGetCurrentLength(void);
extern s16 CVArun(void);
extern void SetCameraMode(s32 mode);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void CdaStop(void);

s16 CVAsequence(s16 sid)
{
    CVASequenceEvent *event;
    Humanoid **slot;
    Humanoid *human;
    s16 sound;
    s16 i;
    s16 motion;
    s32 wanted;
    s32 end_kind;
    s32 type_class;
    CVASequenceHumanAnim *anim_base;

    CVAnow = CVAdata;
    if (CVAdata->kind == -1)
        goto return_zero;

    wanted = (s16)sid;
    end_kind = -1;
scan_event:
    event = CVAnow;
    if (event->kind == 0 && event->mode == wanted)
        goto event_found;
    CVAnow = event + 1;
    if (event[1].kind != end_kind)
        goto scan_event;

event_found:

    if (CVAnow->kind == -1)
        goto return_zero;

    memset(CVAhuman, 0, sizeof(CVAhuman));
    sound = CVAnow->param;
    i = 0;
    CameraTarget = StagePlayer;
    CVAnow++;
    D_800C2C50[0] = 0;
    if (Humans > 0)
    {
        do
        {
            slot = &HumanGroup[i];
            human = *slot;
            if (human->status != 0x11 &&
                (human->attribute & 0x80) == 0)
            {
                dispose_weapon_data_of_char_(human, 3);
                NowReturnNormal(*slot);
                *(s16 *)&(*slot)->pad = 0;
            }
            i++;
        } while (i < Humans);
    }

    D_80097CCC = 0;
    if (CVAupdate() != 0)
        goto run_sequence;

return_zero:
    return 0;

run_sequence:
    if (ActionHalt != -1)
        ActionHalt = 1;
    MotionUpdateMode = 1;
    StagePlayer->target = 0;
    PadShockAR(0, 0, 0, 0);
    PadShock(0, 0, 0);
    PadProc();

    if (sound > 0)
    {
        PlayMusicFormID(sound);
        while (CdaStatus.status != 0)
        {
            if (CdaGetCurrentLength() > 0)
                break;
        }
    }

    D_80097CC0 = 0;
    VoiceMode = 1;
    do
    {
    } while (CVArun() != 0);
    VoiceMode = 0;
    if (ActionHalt != -1)
        ActionHalt = 0;
    MotionUpdateMode = 0;
    SetCameraMode(0);

    i = 0;
    anim_base = CVAhuman;
    type_class = 0x80;
    for (; i < 5; i++)
    {
        human = anim_base[i].human;
        if (human != 0 && human->status != 0x11)
        {
            motion = 0x501;
            if ((human->attribute & 0x40) == 0 &&
                (motion = 0, (human->type & 0xf0) == type_class))
                motion = 0x80e;
            SetNowMotion(human, motion, 1);
        }
    }

    if (sound > 0)
        VSync(60);
    CdaStop();
    PadShockAR(0, 0, 0, 0);
    PadShock(0, 0, 0);
    PadProc();
    PadProc();
    return 1;
}

#endif /* NON_MATCHING */

// triage: HARD — 221 insns, 5 loop, 14 callees, ~0.05 to NowReturnNormal
// likely-relevant cookbook sections:
//   - Loops: 5 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short CVAsequence(short sid)
//
// {
//   short *psVar1;
//   ushort uVar2;
//   short sVar3;
//   int iVar4;
//   Humanoid *human;
//   int *piVar5;
//   int iVar6;
//
//   DAT_80097cbc = DAT_80097cb8;
//   if (*DAT_80097cb8 != -1) {
//     do {
//       if ((*DAT_80097cbc == 0) && (DAT_80097cbc[1] == sid)) break;
//       psVar1 = DAT_80097cbc + 6;
//       DAT_80097cbc = DAT_80097cbc + 6;
//     } while (*psVar1 != -1);
//     if (*DAT_80097cbc != -1) {
//       memset((uchar *)CVAhuman,'\0',0x28);
//       uVar2 = DAT_80097cbc[5];
//       iVar6 = 0;
//       DAT_80097cc4 = StagePlayer;
//       DAT_80097cbc = DAT_80097cbc + 6;
//       DAT_800c2c50 = 0;
//       if (0 < Humans) {
//         iVar4 = 0;
//         do {
//           piVar5 = (int *)((int)HumanGroup + (iVar4 >> 0xe));
//           iVar4 = *piVar5;
//           if ((*(short *)(iVar4 + 2) != 0x11) && ((*(ushort *)(iVar4 + 4) & 0x80) == 0)) {
//             FUN_800270c8(iVar4,3);
//             NowReturnNormal((Humanoid *)*piVar5);
//             *(undefined2 *)(*piVar5 + 0x10) = 0;
//           }
//           iVar6 = iVar6 + 1;
//           iVar4 = iVar6 * 0x10000;
//         } while (iVar6 * 0x10000 >> 0x10 < (int)Humans);
//       }
//       DAT_80097ccc = 0;
//       sVar3 = CVAupdate();
//       if (sVar3 != 0) {
//         if (ActionHalt != -1) {
//           ActionHalt = 1;
//         }
//         MotionUpdateMode = 1;
//         StagePlayer->target = (ModelType *)0x0;
//         PadShockAR(0,0,0,0);
//         PadShock(0,0,0);
//         PadProc();
//         if (0 < (short)uVar2) {
//           PlayMusicFormID();
//           do {
//             if (CdaStatus.status == '\0') break;
//             iVar6 = CdaGetCurrentLength();
//           } while (iVar6 < 1);
//         }
//         DAT_80097cc0 = 0;
//         DAT_80097cb4 = 1;
//         do {
//           sVar3 = CVArun();
//         } while (sVar3 != 0);
//         DAT_80097cb4 = 0;
//         if (ActionHalt != -1) {
//           ActionHalt = 0;
//         }
//         MotionUpdateMode = 0;
//         SetCameraMode(CMODE_NORMAL);
//         iVar4 = 0;
//         iVar6 = 0;
//         do {
//           human = *(Humanoid **)((int)&CVAhuman[0].human + (iVar6 >> 0xd));
//           if ((human != (Humanoid *)0x0) && (human->status != 0x11)) {
//             sVar3 = 0x501;
//             if (((human->attribute & 0x40U) == 0) && (sVar3 = 0, (human->type & 0xf0U) == 0x80)) {
//               sVar3 = 0x80e;
//             }
//             SetNowMotion(human,sVar3,1);
//           }
//           iVar4 = iVar4 + 1;
//           iVar6 = iVar4 * 0x10000;
//         } while (iVar4 * 0x10000 >> 0x10 < 5);
//         if (0 < (int)((uint)uVar2 << 0x10)) {
//           VSync(0x3c);
//         }
//         CdaStop();
//         PadShockAR(0,0,0,0);
//         PadShock(0,0,0);
//         PadProc();
//         PadProc();
//         return 1;
//       }
//     }
//   }
//   return 0;
// }
