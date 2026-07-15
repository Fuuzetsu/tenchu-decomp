#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayVoice(int id);
 *     IMAGES.C:485, 35 src lines, frame 72 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $s3       int id
 *     reg   $s6       unsigned char * FileName
 *     stack sp+24     struct CdlLOC start
 *     stack sp+32     struct CdlLOC end
 *     reg   $s4       struct TVoiceTable * voice
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *     reg   $s2       struct CdlLOC * loc
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 * END PSX.SYM */

typedef struct
{
    u8 id;      /* 0x0 */
    u8 channel; /* 0x1 */
    u8 min;     /* 0x2 */
    u8 sec;     /* 0x3 */
    u8 endmin;  /* 0x4 */
    u8 endsec;  /* 0x5 */
} TVoiceTable;  /* 0x6 */

typedef struct
{
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
} CdlLOC;

extern u8 CHOSEN_LANGUAGE;
extern u8 D_8001005B;

/* Per-language event-XA filename pointers (Ghidra: PTR_s__TENCHU_XA_EVENT_[EFIJ]_XA...). */
extern u8 *D_80097CA0;
extern u8 *D_80097CA4;
extern u8 *D_80097CA8;
extern u8 *D_80097CAC;
/* Per-language voice tables. */
extern TVoiceTable *D_800134E0;
extern TVoiceTable *D_800134E4;
extern TVoiceTable *D_800134E8;
extern TVoiceTable *D_800134EC;

/* INTRO/TORA voice tables + their filenames (id ranges [100,200)/[200,300)). */
extern TVoiceTable D_8008E82C[];
extern TVoiceTable D_8008E930[];
extern u8 *D_80097C98;
extern u8 *D_80097C9C;

/* Fallback (language/range-independent) voice table. */
extern TVoiceTable D_80012CBC[];

extern char D_800134F0[]; /* "bad voice no %d" */
extern char D_80013500[]; /* "playvoice fail %s  chan %d  id %d" */

extern void AdtMessageBox(char *fmt, ...);
extern void CdaStop(void);
extern void SsSetMVol(int voll, int volr);
extern void FUN_8004fbf4(u8 voll, u8 volr);
extern void *memset(void *s, int c, u32 n);
extern s32 CdPosToInt(CdlLOC *pos);
extern void CdIntToPos(s32 n, CdlLOC *pos);
extern int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode);

/*
 * PlayVoice (0x8004eee4) — look up voice-clip `id` in one of several
 * TVoiceTable arrays and CdaPlayXA it. id < 100: current-language event
 * table (D_800134E0..EC / D_80097CA0..AC, indexed by CHOSEN_LANGUAGE).
 * 100 <= id < 200: the INTRO table (id -= 100). id >= 200: the TORA table
 * (id -= 200). If none of those has the id, falls back to the shared
 * D_80012CBC table (always using the event-table filename D_80097CA0).
 * Each table is a linear array of {id,channel,min,sec,endmin,endsec}
 * records terminated by id==0xff. On a total miss: AdtMessageBox + CdaStop.
 * On a hit: clamp the persisted volume byte (D_8001005B) to 0x7f, reset the
 * CD-XA mix volume, re-apply the persisted volume, build `start`/`end`
 * CdlLOCs from the record's min/sec (the CdPosToInt/CdIntToPos "*2+0x96"
 * read-ahead idiom, exactly _PlayMusic.c's own start/end construction), and
 * CdaPlayXA; a zero return gets its own AdtMessageBox.
 *
 * Matching notes:
 *  - The two search loops are DIFFERENT shapes (confirmed against the raw
 *    target .s and cross-checked with Ghidra's own two independent
 *    reconstructions): the CHOSEN_LANGUAGE-indexed loop re-caches the id
 *    scalar (`entry = voice; if (match) break; voice++; entry = 0;` —
 *    Ghidra's own `uVar3 = *pbVar5;` re-read-after-advance shape), while
 *    the INTRO/TORA loop peeks the NEXT record's id through a SEPARATE
 *    pointer before actually advancing the cursor (`entry = voice; if
 *    (match) goto found; entry = voice + 1; voice++; } while (entry->id
 *    != 0xff);` — Ghidra literally renders this as two assignments,
 *    `pbVar6 = pbVar5 + 6; pbVar5 = pbVar5 + 6;`, that a naive reading
 *    would collapse into one).
 *
 * STATUS: NON_MATCHING — 12 of 756 bytes short (186 vs target 189
 * instructions). The residual traces to the PROLOGUE's per-language table
 * setup, which the target compiles through an unusual ROUNDABOUT COPY:
 * it loads the 4 event-XA filename pointers (D_80097CA0/A4/A8/AC, gp-rel)
 * and stores them to the STACK SLOT THAT WILL LATER HOLD THE VOICE-TABLE
 * ARRAY (sp+0x28), then immediately reloads those same 4 words and
 * stores them into the FILENAME ARRAY's own slot (sp+0x18) — only THEN
 * does it load the 4 real voice-table pointers (D_800134E0/E4/E8/EC,
 * absolute — NOT gp-rel, confirmed too far from _gp) and store those into
 * sp+0x28. A direct `filenames[i] = D_80097CAx; tables[i] = D_800134Ex;`
 * (this draft) stores each value straight to its own final slot with no
 * detour, costing 3 instructions and rippling into a smaller frame (96 vs
 * target's 96 — matches) but a different register/stack-offset assignment
 * for everything downstream (confirmed: the dispatch and both loops'
 * bodies use different specific registers even where the CONTROL FLOW
 * shape is now proven right). Tried and rejected: swapping declaration/
 * assignment order of the two arrays (no effect on the roundabout shape);
 * C89 initializer-list spellings (`u8 *filenames[4] = {A,B,C,D};` /
 * `TVoiceTable *tables[4] = {E,F,G,H};` instead of separate assignment
 * statements) — this made it dramatically WORSE (816 vs target 756, 60
 * bytes over), so cc1 does NOT route non-constant array initializer lists
 * through a shared temp the way the target's roundabout copy would need;
 * whatever produces the target's shape is not simply "how the array gets
 * initialized" at the syntax level.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PlayVoice", PlayVoice);
#else
void PlayVoice(int id)
{
    u8 *FileName;
    TVoiceTable *voice;
    TVoiceTable *entry;
    u8 min;
    u8 sec;
    CdlLOC start[2];
    CdlLOC end[2];
    u8 *filenames[4];
    TVoiceTable *tables[4];

    filenames[0] = D_80097CA0;
    filenames[1] = D_80097CA4;
    filenames[2] = D_80097CA8;
    filenames[3] = D_80097CAC;
    tables[0] = D_800134E0;
    tables[1] = D_800134E4;
    tables[2] = D_800134E8;
    tables[3] = D_800134EC;
    memset(start, 0, 4);
    memset(end, 0, 4);

    if (id < 100)
    {
        voice = tables[CHOSEN_LANGUAGE];
        FileName = filenames[CHOSEN_LANGUAGE];
        entry = 0;
        if (voice->id != 0xff)
        {
            do
            {
                entry = voice;
                if (id == voice->id)
                    break;
                voice++;
                entry = 0;
            } while (voice->id != 0xff);
        }
    }
    else
    {
        if (id < 200)
        {
            voice = D_8008E82C;
            id -= 100;
            FileName = D_80097C98;
        }
        else
        {
            voice = D_8008E930;
            id -= 200;
            FileName = D_80097C9C;
        }
        entry = 0;
        if (voice->id != 0xff)
        {
            do
            {
                entry = voice;
                if (id == voice->id)
                    goto found;
                entry = voice + 1;
                voice++;
            } while (entry->id != 0xff);
            entry = 0;
        }
    }
found:
    if (entry == 0)
    {
        entry = D_80012CBC;
        if (entry->id != 0xff)
        {
            do
            {
                if (id == entry->id)
                    goto found2;
                entry++;
            } while (entry->id != 0xff);
        }
        entry = 0;
    found2:
        FileName = D_80097CA0;
        if (entry == 0)
        {
            AdtMessageBox(D_800134F0, id);
            CdaStop();
            return;
        }
    }

    min = D_8001005B;
    if (min > 0x7e)
        min = 0x7f;
    SsSetMVol(0x7f, 0x7f);
    FUN_8004fbf4(min, min);

    min = entry->min;
    sec = entry->sec;
    memset(start, 0, 4);
    start[0].minute = min;
    start[0].second = sec;
    CdIntToPos(CdPosToInt(start) * 2 + 0x96, start);

    min = entry->endmin;
    sec = entry->endsec;
    memset(end, 0, 4);
    end[0].minute = min;
    end[0].second = sec;
    CdIntToPos(CdPosToInt(end) * 2 + 0x96, end);

    if (CdaPlayXA(FileName, start, end, entry->channel, 0) == 0)
    {
        AdtMessageBox(D_80013500, FileName, entry->channel, id);
    }
}
#endif /* NON_MATCHING */

// triage: MEDIUM — 189 insns, 4 loop, 8 callees, ~0.07 to InsertConflict
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void PlayVoice(int id)
//
// {
//   byte bVar1;
//   byte bVar2;
//   uint uVar3;
//   int iVar4;
//   byte *pbVar5;
//   byte *pbVar6;
//   undefined1 uVar7;
//   undefined *puVar8;
//   undefined *local_48 [4];
//   undefined *local_38 [4];
//   CdlLOC local_28 [2];
//   CdlLOC local_20 [2];
//
//   local_48[0] = PTR_s__TENCHU_XA_EVENT_E_XA_1_80097ca0;
//   local_48[1] = PTR_s__TENCHU_XA_EVENT_F_XA_1_80097ca4;
//   local_48[2] = PTR_s__TENCHU_XA_EVENT_I_XA_1_80097ca8;
//   local_48[3] = PTR_s__TENCHU_XA_EVENT_J_XA_1_80097cac;
//   local_38[0] = PTR_DAT_800134e0;
//   local_38[1] = PTR_DAT_800134e4;
//   local_38[2] = PTR_DAT_800134e8;
//   local_38[3] = PTR_DAT_800134ec;
//   memset(&local_28[0].minute,'\0',4);
//   memset(&local_20[0].minute,'\0',4);
//   if (id < 100) {
//     pbVar5 = local_38[(byte)PersistentState._94_1_];
//     puVar8 = local_48[(byte)PersistentState._94_1_];
//     pbVar6 = (byte *)0x0;
//     if (*pbVar5 != 0xff) {
//       uVar3 = (uint)*pbVar5;
//       do {
//         pbVar6 = pbVar5;
//         if (id == uVar3) break;
//         pbVar5 = pbVar5 + 6;
//         uVar3 = (uint)*pbVar5;
//         pbVar6 = (byte *)0x0;
//       } while (uVar3 != 0xff);
//     }
//   }
//   else {
//     if (id < 200) {
//       pbVar5 = &DAT_8008e82c;
//       id = id - 100;
//       puVar8 = PTR_s__TENCHU_XA_INTRO_XA_1_80097c98;
//     }
//     else {
//       pbVar5 = &DAT_8008e930;
//       id = id - 200;
//       puVar8 = PTR_s__TENCHU_XA_TORA_XA_1_80097c9c;
//     }
//     pbVar6 = (byte *)0x0;
//     if (*pbVar5 != 0xff) {
//       do {
//         pbVar6 = pbVar5;
//         if (id == (uint)*pbVar5) goto LAB_8004f070;
//         pbVar6 = pbVar5 + 6;
//         pbVar5 = pbVar5 + 6;
//       } while (*pbVar6 != 0xff);
//       pbVar6 = (byte *)0x0;
//     }
//   }
// LAB_8004f070:
//   if (pbVar6 == (byte *)0x0) {
//     pbVar6 = &DAT_80012cbc;
//     if (DAT_80012cbc != 0xff) {
//       uVar3 = (uint)DAT_80012cbc;
//       do {
//         if (id == uVar3) goto LAB_8004f0bc;
//         pbVar6 = pbVar6 + 6;
//         uVar3 = (uint)*pbVar6;
//       } while (uVar3 != 0xff);
//     }
//     pbVar6 = (byte *)0x0;
// LAB_8004f0bc:
//     puVar8 = PTR_s__TENCHU_XA_EVENT_E_XA_1_80097ca0;
//     if (pbVar6 == (byte *)0x0) {
//       AdtMessageBox("bad voice no %d",id);
//       CdaStop();
//       return;
//     }
//   }
//   uVar7 = PersistentState._91_1_;
//   if (0x7e < (byte)PersistentState._91_1_) {
//     uVar7 = 0x7f;
//   }
//   SsSetMVol(0x7f,0x7f);
//   FUN_8004fbf4(uVar7,uVar7);
//   bVar1 = pbVar6[2];
//   bVar2 = pbVar6[3];
//   memset(&local_28[0].minute,'\0',4);
//   local_28[0].minute = bVar1;
//   local_28[0].second = bVar2;
//   iVar4 = CdPosToInt(local_28);
//   CdIntToPos(iVar4 * 2 + 0x96,local_28);
//   bVar1 = pbVar6[4];
//   bVar2 = pbVar6[5];
//   memset(&local_20[0].minute,'\0',4);
//   local_20[0].minute = bVar1;
//   local_20[0].second = bVar2;
//   iVar4 = CdPosToInt(local_20);
//   CdIntToPos(iVar4 * 2 + 0x96,local_20);
//   iVar4 = CdaPlayXA(puVar8,local_28,local_20,pbVar6[1],0);
//   if (iVar4 == 0) {
//     AdtMessageBox("playvoice fail %s  chan %d  id %d",puVar8,(uint)pbVar6[1],id);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8, s32);                 /* extern */
// ? CdIntToPos(s32, u8 *);                            /* extern */
// s32 CdPosToInt(u8 *);                               /* extern */
// s32 CdaPlayXA(s32, u8 *, u8 *, u8, s32);            /* extern */
// ? CdaStop();                                        /* extern */
// ? FUN_8004fbf4(s32, s32);                           /* extern */
// ? SsSetMVol(?, ?);                                  /* extern */
// ? memset(u8 *, ?, ?, s32);                          /* extern */
// extern u8 CHOSEN_LANGUAGE;
// extern u8 D_8001005B;
// extern u8 D_80012CBC;
// extern ? D_800134E0;
// extern ? D_800134F0;
// extern ? D_80013500;
// extern u8 D_8008E82C;
// extern u8 D_8008E930;
// extern s32 D_80097C98;
// extern s32 D_80097C9C;
// extern u8 *D_80097CA0;
// extern s32 D_80097CA4;
// extern s32 D_80097CA8;
// extern s32 D_80097CAC;
//
// void PlayVoice(s32 arg0) {
//     u8 *sp18;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp24;
//     u8 *sp28;
//     s32 sp2C;
//     s32 sp30;
//     s32 sp34;
//     u8 sp38;
//     u8 sp39;
//     u8 sp40;
//     u8 sp41;
//     s32 temp_a0_2;
//     s32 var_s3;
//     s32 var_s4;
//     u8 *temp_a0;
//     u8 *var_a0;
//     u8 *var_s2;
//     u8 *var_v1;
//     u8 *var_v1_2;
//     u8 temp_s0;
//     u8 temp_s0_2;
//     u8 temp_s1;
//     u8 temp_s1_2;
//     u8 var_s0;
//     u8 var_v0;
//     u8 var_v0_2;
//
//     var_s3 = arg0;
//     sp28 = D_80097CA0;
//     sp2C = D_80097CA4;
//     sp30 = D_80097CA8;
//     sp34 = D_80097CAC;
//     sp18 = sp28;
//     sp1C = sp2C;
//     sp20 = sp30;
//     sp24 = sp34;
//     sp28 = D_800134E0.unk0;
//     sp2C = D_800134E0.unk4;
//     sp30 = D_800134E0.unk8;
//     sp34 = D_800134E0.unkC;
//     memset(&sp38, 0, 4, D_80097CA8);
//     memset(&sp40, 0, 4);
//     if (var_s3 >= 0x64) {
//         if (var_s3 >= 0xC8) {
//             var_a0 = &D_8008E930;
//             var_s4 = D_80097C9C;
//             var_s3 -= 0xC8;
//         } else {
//             var_a0 = &D_8008E82C;
//             var_s4 = D_80097C98;
//             var_s3 -= 0x64;
//         }
//         var_s2 = NULL;
//         if (*var_a0 != 0xFF) {
// loop_6:
//             var_s2 = var_a0;
//             if (var_s3 != var_a0->unk0) {
//                 var_a0 += 6;
//                 if (var_a0->unk6 == 0xFF) {
//                     var_s2 = NULL;
//                 } else {
//                     goto loop_6;
//                 }
//             }
//         }
//     } else {
//         temp_a0 = (&sp28)[CHOSEN_LANGUAGE];
//         var_s4 = (s32) (&sp18)[CHOSEN_LANGUAGE];
//         var_s2 = NULL;
//         if (*temp_a0 != 0xFF) {
//             var_v1 = temp_a0;
//             var_v0 = *var_v1;
// loop_12:
//             var_s2 = var_v1;
//             if (var_s3 != var_v0) {
//                 var_v1 += 6;
//                 var_v0 = *var_v1;
//                 var_s2 = NULL;
//                 if (var_v0 != 0xFF) {
//                     goto loop_12;
//                 }
//             }
//         }
//     }
//     if (var_s2 == NULL) {
//         if (D_80012CBC != 0xFF) {
//             var_v1_2 = &D_80012CBC;
//             var_v0_2 = D_80012CBC;
// loop_17:
//             if (var_s3 != var_v0_2) {
//                 var_v1_2 += 6;
//                 var_v0_2 = *var_v1_2;
//                 if (var_v0_2 == 0xFF) {
//                     goto block_19;
//                 }
//                 goto loop_17;
//             }
//             var_s2 = var_v1_2;
//         } else {
// block_19:
//             var_s2 = NULL;
//         }
//         var_s4 = (s32) D_80097CA0;
//         if (var_s2 == NULL) {
//             AdtMessageBox(&D_800134F0, var_s3);
//             CdaStop();
//             return;
//         }
//         goto block_22;
//     }
// block_22:
//     var_s0 = D_8001005B;
//     if ((s32) var_s0 >= 0x7F) {
//         var_s0 = 0x7F;
//     }
//     SsSetMVol(0x7F, 0x7F);
//     temp_a0_2 = var_s0 & 0xFF;
//     FUN_8004fbf4(temp_a0_2, temp_a0_2);
//     temp_s0 = var_s2->unk2;
//     temp_s1 = var_s2->unk3;
//     memset(&sp38, 0, 4);
//     sp38 = temp_s0;
//     sp39 = temp_s1;
//     CdIntToPos((CdPosToInt(&sp38) * 2) + 0x96, &sp38);
//     temp_s0_2 = var_s2->unk4;
//     temp_s1_2 = var_s2->unk5;
//     memset(&sp40, 0, 4);
//     sp40 = temp_s0_2;
//     sp41 = temp_s1_2;
//     CdIntToPos((CdPosToInt(&sp40) * 2) + 0x96, &sp40);
//     if (CdaPlayXA(var_s4, &sp38, &sp40, var_s2->unk1, 0) == 0) {
//         AdtMessageBox(&D_80013500, var_s4, var_s2->unk1, var_s3);
//     }
// }
