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
 * The post-memset cursor is deliberately separate from the scan's `event`.
 * Updating that short-lived carrier in place preserves the source dependency
 * that makes cc1 keep the cursor in v0 while loading StagePlayer into v1.
 *
 * Other load-bearing shapes: the event scan is a hand-rolled goto loop;
 * separate s32 `wanted`/`end_mode` values create the preheader extension and
 * loop-carried -1; and the motion arm's `(motion = 0, test)` comma expression
 * selects the retail allocation and schedule.
 */

#include "item.h"

extern u8 D_800C2C50[];
extern s16 D_80097CC0;
extern s16 D_80097CCC;

extern void *memset(void *s, int c, u32 n);
extern void dispose_weapon_data_of_char_(Humanoid *human, s32 mode);
extern s16 CVAupdate(void);
extern void PadShock(s32 port, s32 power, s32 time);
extern void PadProc(void);
extern void PlayMusicFormID(s32 id);
extern s32 CdaGetCurrentLength(void);
extern s16 CVArun(void);
extern void CdaStop(void);

s16 CVAsequence(s16 sid)
{
    CVAType *event;
    CVAType *cursor;
    Humanoid **slot;
    Humanoid *human;
    s16 sound;
    s16 i;
    s16 motion;
    s32 wanted;
    s32 end_mode;
    s32 type_class;
    HumanAnimType *anim_base;

    CVAnow = CVAdata;
    if (CVAdata->mode == -1)
        goto return_zero;

    wanted = (s16)sid;
    end_mode = -1;
scan_event:
    event = CVAnow;
    if (event->mode == 0 && event->id == wanted)
        goto event_found;
    CVAnow = event + 1;
    if (event[1].mode != end_mode)
        goto scan_event;

event_found:

    if (CVAnow->mode == -1)
        goto return_zero;

    memset(CVAhuman, 0, sizeof(CVAhuman));
    cursor = CVAnow;
    sound = cursor->p;
    i = 0;
    CameraTarget = StagePlayer;
    cursor++;
    CVAnow = cursor;
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
                (*slot)->pad.data = 0;
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
    SetCameraMode(CMODE_NORMAL);

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
