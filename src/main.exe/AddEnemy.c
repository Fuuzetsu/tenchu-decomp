#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void AddEnemy(void);
 *     INFOVIEW.C:695, 58 src lines, frame 2064 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     stack sp+24     unsigned char [70][20] names
 *     stack sp+1424   struct TAdtSelect [70] ItemName
 *     reg   $s4       short i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s7       long type
 *     reg   $s5       long x
 *     reg   $s2       long y
 *     reg   $s0       long z
 *     reg   $s3       short r
 *     reg   $s2       short think
 *     stack sp+1984   struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern struct WeaponModelType WeaponModel[41];
 *     extern int StageID;
 *     extern struct ThinkDBtype ThinkDB[20];
 *     extern struct TCameraStatus CamState;
 *     extern int CurrentEnemyID;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — complete pure-C reconstruction, 1132 bytes / 283
 * instructions versus the 1152-byte / 288-instruction target, with the exact
 * 0x810 frame.  The retail stack plan is also exact: names at sp+0x18,
 * ItemName at sp+0x590, output VECTOR at sp+0x7c0, and the zeroed blood
 * VECTOR at sp+0x7d0.
 *
 * The remaining diff is dominated by one allocator/control-flow family in
 * the nested stage-kind, weapon, and think-menu scans.  Retail holds the
 * HumanData base in fp, assigns the scan/count values to s2-s5, and keeps the
 * CHARACTER_KINDS_PER_STAGE and WeaponModel bases in t1/t0, spilling those
 * two caller-saved bases at sp+0x7e4/sp+0x7e0 around sprintf.  This draft's
 * equivalent natural loops color the two bases into saved registers instead.
 * The AddEnemyBloodScratch tail represents those two retail spill words and
 * preserves the exact frame while the guarded draft retains that coloring.
 *
 * `rtlguide` identifies the residue primarily as regalloc/CSE plus loop
 * structure.  `regalloc`, `.lreg`/`.greg`, guided `autorules` (180-candidate
 * budget), and direct stage/weapon-loop variants found no justified source
 * spelling that closes the remaining five instructions.  The mechanical
 * identical-arm fence did make the length exact by inserting an unrelated
 * branch at the menu terminator, so it was rejected; likewise, narrowing
 * count/names_offset superficially improves byte score but contradicts the
 * target's direct `slti`/`sll` uses and Ghidra's recovered int types.  The
 * retained guarded draft has 173 asmdiff lines in 53 hunks and scores 53.59
 * with `tools/fuzz-score.py`.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AddEnemy", AddEnemy);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AddEnemy", debug_menu_enemy_layout_add__override__prt_8005b740_aee7b64a);
#else

#include "item.h"

typedef struct
{
    s16 type;
    s16 wepid;
    s16 turn;
    s16 life;
    s16 width;
    s16 height;
    MotionRegistType *mtbl;
    u8 *name;
    u32 *model;
} AddEnemyHumanData;

typedef struct
{
    u8 *name;
    s16 wid;
    u32 *model;
} AddEnemyWeaponModel;

typedef struct
{
    u8 *name;
    s16 value;
} AddEnemyThinkDB;

typedef struct
{
    VECTOR TargetVector;
    Humanoid *Owner;
    s32 Mode;
    s16 DirectionRX;
    s16 DirectionRY;
    s32 OldMode;
    u8 Valiation;
} AddEnemyCameraStatus;

/* Retail reserves the two words after the temporary VECTOR as the caller-
 * saved base spills around sprintf.  Keeping them in the same local scratch
 * object preserves the original 0x810-byte frame even while this guarded C
 * draft still colors those bases into callee-saved registers. */
typedef struct
{
    VECTOR vector;
    u32 call_spill[2];
} AddEnemyBloodScratch;

extern AddEnemyHumanData HumanData[63];
extern AddEnemyWeaponModel WeaponModel[41];
extern s16 *CHARACTER_KINDS_PER_STAGE[];
extern s32 StageID;
extern AddEnemyThinkDB ThinkDB[20];
extern AddEnemyCameraStatus CamState;
extern s32 CurrentEnemyID;

extern char D_80013FA8[];
extern char D_80013FB4[];
extern char D_80097D48[];
extern char D_80097D50[];

extern s32 AdtSelect(char *title, debug_menu_choice *menu, s32 mode);
extern int sprintf(char *buf, char *fmt, ...);
extern void *memset(void *s, int c, u32 n);
extern s32 leSetEnemy(s32 type, s16 think, s32 x, s32 y, s32 z, s16 r);
extern Humanoid *BreedLife(s16 type, s32 x, s32 y, s32 z, s32 r);
extern void SetBleeds(VECTOR *pos, short grange, short srange, short n,
                      int time, long col);

void AddEnemy(void)
{
    u8 names[70][20];
    debug_menu_choice ItemName[70];
    VECTOR pos;
    AddEnemyBloodScratch blood;
    s32 count;
    s32 names_offset;
    s16 i;
    s16 j;
    s32 weapon;
    s32 type;
    s32 think;
    s16 category;
    s16 think_index;
    s32 stage;
    AddEnemyWeaponModel *weapon_entry;
    debug_menu_choice *item;
    s16 **kind_base;
    AddEnemyHumanData *human_data;
    AddEnemyWeaponModel *weapon_base;
    char *buffer;
    ModelArchiveType *model;
    Humanoid *human;
    s32 x;
    s32 y;
    s32 z;
    s16 r;

    count = 0;
    i = 0;
    if (HumanData[0].type != -1)
    {
        names_offset = 0;
        human_data = HumanData;
        weapon_base = WeaponModel;
        kind_base = CHARACTER_KINDS_PER_STAGE;
        do
        {
            if (count >= 70)
                break;

            stage = StageID;
#define ADD_ENEMY_STAGE_KINDS \
    (*(s16 **)((u8 *)kind_base + ((stage + 1) * sizeof(s16 *))))
            if (ADD_ENEMY_STAGE_KINDS[0] != -1)
            {
                j = 0;
                do
                {
                    if (ADD_ENEMY_STAGE_KINDS[j] == human_data[i].type)
                        break;
                    j++;
                } while (ADD_ENEMY_STAGE_KINDS[j] != -1);

                if (ADD_ENEMY_STAGE_KINDS[j] != -1)
                {
                    weapon = 0;
                    weapon_entry = weapon_base;
                    if (weapon_entry->wid != -1)
                    {
                        do
                        {
                            if (weapon_entry->wid == human_data[i].wepid)
                                break;
                            weapon_entry++;
                            weapon++;
                        } while (weapon_entry->wid != -1);
                    }

                    buffer = (char *)names + names_offset;
                    names_offset += 20;
                    sprintf(buffer, D_80097D48, human_data[i].name,
                            weapon_base[weapon].name);
                    ItemName[count].choice_name = buffer;
                    ItemName[count].choice_number = human_data[i].type;
                    count++;
                }
            }
            i++;
        } while (human_data[i].type != -1);
#undef ADD_ENEMY_STAGE_KINDS
    }

    item = ItemName;
    item[count].choice_name = D_80097D50;
    item[count].choice_number = -1;
    count++;
    item[count].choice_name = 0;
    type = (s16)AdtSelect(D_80013FA8, item, 0);
    if (type == -1)
        return;

    think = 0;
    category = 0;
    do
    {
        count = 0;
        think_index = 0;
        if (ThinkDB[0].name != 0)
        {
            item = ItemName;
            do
            {
                if (count >= 70)
                    break;
                if ((s16)category + 0x31 == ThinkDB[think_index].name[0])
                {
                    item->choice_name = (char *)ThinkDB[think_index].name;
                    item->choice_number = ThinkDB[think_index].value;
                    item++;
                    count++;
                }
                think_index++;
            } while (ThinkDB[think_index].name != 0);
        }
        ItemName[count].choice_name = 0;
        think |= AdtSelect(D_80013FB4, ItemName, 0);
    } while ((s16)think != 0x1111 && (s16)think != 0x2222 &&
             ++category < 4);

    model = CamState.Owner->model;
    x = model->locate.coord.t[0];
    y = model->locate.coord.t[1];
    r = model->rotate.vy;
    z = model->locate.coord.t[2];
    CurrentEnemyID = leSetEnemy(type, (s16)think, x, y, z, r);
    human = BreedLife(type, x, y, z, 0);
    human->model->rotate.vy = r;
    human->target = (ModelType *)CamState.Owner->model;

    memset(&blood.vector, 0, sizeof(VECTOR));
    blood.vector.vx = human->model->locate.coord.t[0];
    blood.vector.vy = human->model->locate.coord.t[1] - 1200;
    blood.vector.vz = human->model->locate.coord.t[2];
    pos = blood.vector;
    SetBleeds(&pos, 400, 0, 50, 30, 0xffffff);
}

#endif /* NON_MATCHING */

// triage: HARD — 288 insns, 5 loop, frame 0x810, 6 callees, ~0.06 to GetMotionID
// likely-relevant cookbook sections:
//   - Loops: 5 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x810 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void AddEnemy(void)
//
// {
//   short sVar1;
//   short sVar2;
//   int iVar3;
//   ulong uVar4;
//   ModelArchiveType *pMVar5;
//   WeaponModelType *pWVar6;
//   byte *pbVar7;
//   int iVar8;
//   TAdtSelect *pTVar9;
//   WeaponModelType *pWVar10;
//   undefined *puVar11;
//   long z;
//   char *buffer;
//   uint uVar12;
//   long y;
//   int iVar13;
//   int iVar14;
//   int iVar15;
//   long x;
//   int iVar16;
//   char acStack_7f8 [1400];
//   TAdtSelect local_280;
//   char *local_278 [139];
//   int local_4c;
//   undefined4 local_48;
//   undefined4 local_44;
//   undefined4 local_40;
//   int local_3c;
//   undefined4 local_38;
//   undefined4 local_34;
//   WeaponModelType *local_30;
//   undefined *local_2c;
//
//   iVar14 = 0;
//   sVar2 = 0;
//   if (HumanData[0].type != -1) {
//     puVar11 = &DAT_8008f188;
//     pWVar10 = WeaponModel;
//     iVar16 = 0;
//     iVar13 = iVar14;
//     do {
//       iVar14 = iVar13;
//       if (0x45 < iVar13) break;
//       if (**(short **)(puVar11 + (StageID + 1) * 4) != -1) {
//         iVar15 = 0;
//         do {
//           iVar8 = *(int *)(puVar11 + (StageID + 1) * 4);
//           iVar3 = iVar15 + 1;
//           if (*(short *)(((iVar15 << 0x10) >> 0xf) + iVar8) == HumanData[sVar2].type) break;
//           iVar15 = iVar3;
//         } while (*(short *)((iVar3 * 0x10000 >> 0xf) + iVar8) != -1);
//         if (*(short *)(((iVar15 << 0x10) >> 0xf) + iVar8) != -1) {
//           iVar14 = 0;
//           if (pWVar10->wid != -1) {
//             sVar1 = pWVar10->wid;
//             pWVar6 = pWVar10;
//             do {
//               if (sVar1 == HumanData[sVar2].wepid) break;
//               sVar1 = pWVar6[1].wid;
//               iVar14 = iVar14 + 1;
//               pWVar6 = pWVar6 + 1;
//             } while (sVar1 != -1);
//           }
//           buffer = acStack_7f8 + iVar16;
//           iVar16 = iVar16 + 0x14;
//           local_30 = pWVar10;
//           local_2c = puVar11;
//           sprintf(buffer,s__s__s_80097d48,HumanData[sVar2].name,pWVar10[iVar14].name);
//           local_278[iVar13 * 2 + -2] = buffer;
//           iVar14 = iVar13 + 1;
//           local_278[iVar13 * 2 + -1] = (char *)(int)HumanData[sVar2].type;
//           pWVar10 = local_30;
//           puVar11 = local_2c;
//         }
//       }
//       sVar2 = sVar2 + 1;
//       iVar13 = iVar14;
//     } while (HumanData[sVar2].type != -1);
//   }
//   (&local_280)[iVar14].name = s_cancel_80097d50;
//   local_278[iVar14 * 2 + -1] = (char *)0xffffffff;
//   (&local_280)[iVar14 + 1].name = (char *)0x0;
//   uVar4 = AdtSelect("select type",&local_280,0);
//   iVar14 = (int)(short)uVar4;
//   uVar12 = 0;
//   if (iVar14 != -1) {
//     iVar13 = 0;
//     do {
//       iVar15 = 0;
//       iVar16 = 0;
//       if (ThinkDB[0].name != (uchar *)0x0) {
//         pTVar9 = &local_280;
//         do {
//           if (0x45 < iVar15) break;
//           iVar3 = (iVar16 << 0x10) >> 0xd;
//           pbVar7 = *(byte **)((int)&ThinkDB[0].name + iVar3);
//           if ((int)(short)iVar13 + 0x31U == (uint)*pbVar7) {
//             pTVar9->name = (char *)pbVar7;
//             iVar15 = iVar15 + 1;
//             pTVar9->value = (int)*(short *)((int)&ThinkDB[0].value + iVar3);
//             pTVar9 = pTVar9 + 1;
//           }
//           iVar16 = iVar16 + 1;
//         } while (*(int *)((int)&ThinkDB[0].name + (iVar16 * 0x10000 >> 0xd)) != 0);
//       }
//       (&local_280)[iVar15].name = (char *)0x0;
//       uVar4 = AdtSelect("custom think setting",&local_280,0);
//       uVar12 = uVar12 | uVar4;
//       sVar2 = (short)uVar12;
//     } while (((sVar2 != 0x1111) && (iVar13 = iVar13 + 1, sVar2 != 0x2222)) &&
//             (iVar13 * 0x10000 >> 0x10 < 4));
//     pMVar5 = (CamState.Owner)->model;
//     x = (pMVar5->locate).coord.t[0];
//     y = (pMVar5->locate).coord.t[1];
//     sVar1 = (pMVar5->rotate).vy;
//     z = (pMVar5->locate).coord.t[2];
//     DAT_80097d44 = leSetEnemy(iVar14,sVar2,x,y,z,sVar1);
//     iVar14 = BreedLife(iVar14,x,y,z,0);
//     *(short *)(*(int *)(iVar14 + 0x58) + 0x52) = sVar1;
//     *(ModelArchiveType **)(iVar14 + 0x74) = (CamState.Owner)->model;
//     memset((uchar *)&local_40,'\0',0x10);
//     local_278[0x8a] = *(char **)(*(int *)(iVar14 + 0x58) + 0x18);
//     local_4c = *(int *)(*(int *)(iVar14 + 0x58) + 0x1c) + -0x4b0;
//     local_48 = *(undefined4 *)(*(int *)(iVar14 + 0x58) + 0x20);
//     local_44 = local_34;
//     local_40 = local_278[0x8a];
//     local_3c = local_4c;
//     local_38 = local_48;
//     SetBleeds((short)local_278 + 0x228,400,0);
//   }
//   return;
// }
