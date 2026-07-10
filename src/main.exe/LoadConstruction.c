#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short LoadConstruction(unsigned long *data);
 *     WORLD.C:436, 191 src lines, frame 416 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param stack+0   unsigned long * data
 *     stack sp+352    int ObjectID
 *     stack sp+356    unsigned long * MapModel
 *     stack sp+360    struct WorldDataType * wlddt
 *     reg   $s2       struct OrnamentType * model
 *     reg   $s4       long x
 *     reg   $s3       long y
 *     reg   $s0       long z
 *     stack sp+364    long n
 *     reg   $s5       long i
 *     stack sp+32     unsigned char [256] name
 *     reg   $s3       int nModel
 *     reg   $s1       struct OrnamentArchiveType * mad
 *     reg   $s0       int i
 *     reg   $s1       struct OrnamentArchiveType * mad
 *     reg   $s0       int i
 *     reg   $s3       int n
 *     reg   $s0       int i
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     stack sp+288    struct SVECTOR center
 *     stack sp+296    int size
 *     reg   $s3       struct tag_ObjectSlotType ** slot
 *     reg   $s2       struct OrnamentType * model
 *     reg   $s6       short shifty
 *     stack sp+304    struct PARAM_ITEM_STAY param
 *     reg   $s6       int i
 *     stack sp+368    struct ParentingType * ix
 *     reg   $fp       int msize
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $s3       struct tag_ObjectSlotType ** slot
 *     reg   $s1       struct OrnamentType * model
 *
 * Globals it touches, as the original declared them:
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Sprite3D *sprSmoke;
 *     extern unsigned char *ImagePath;
 *     extern struct ObjectSlotManager ModelSlot;
 *     extern int StageID;
 *     extern struct ModelType World;
 * END PSX.SYM */

/*
 * LoadConstruction (0x8003ab60) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=LoadConstruction
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", LoadConstruction);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", reinitialise_models_and_stuff___override__prt_8003ae78_f3434af);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", reinitialise_models_and_stuff___override__prt_8003aef4_f3434af);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_1);

/* jump-table pool @ 0x8001219c (12 words; tables at 0x8001219c) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 LoadConstruction_jtbl[12] = {
    0x8003AE6C, 0x8003B25C, 0x8003AF1C, 0x8003B17C,
    0x8003B1E8, 0x8003AEE8, 0x8003B25C, 0x8003B25C,
    0x8003B25C, 0x8003B25C, 0x8003B25C, 0x8003B1A4,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
// 
// short LoadConstruction(void)
// 
// {
//   short *psVar1;
//   short *psVar2;
//   short *psVar3;
//   short *psVar4;
//   short *psVar5;
//   int *piVar6;
//   int *piVar7;
//   int *piVar8;
//   int *piVar9;
//   undefined *puVar10;
//   OrnamentArchiveType *pOVar11;
//   OrnamentArchiveType *pOVar12;
//   short sVar13;
//   ulong uVar14;
//   ulong *puVar15;
//   short *psVar16;
//   int iVar17;
//   uint uVar18;
//   uint uVar19;
//   short *in_a0;
//   OrnamentType *pOVar20;
//   int iVar21;
//   uint uVar22;
//   int iVar23;
//   WorldType *pWVar24;
//   int iVar25;
//   uchar auStack_180 [256];
//   SVECTOR SStack_80;
//   PARAM_ITEM_STAY local_78;
//   TItemType local_60;
//   long local_5c;
//   long local_58;
//   long local_54;
//   long local_50;
//   int local_48;
//   int local_44;
//   ulong *local_40;
//   short *local_3c;
//   uint local_38;
//   ulong *local_34;
//   int local_30;
//   
//   local_44 = 0;
//   if (in_a0 == (short *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO CONSTRUCTION DATA");
//   }
//   local_3c = in_a0;
//   memset((uchar *)WorldMap,'\0',0x800);
//   iVar23 = 0;
//   uVar14 = vsize((undefined *)in_a0);
//   iVar25 = 0;
//   local_38 = uVar14 >> 5;
//   psVar16 = in_a0;
//   if (local_38 != 0) {
//     do {
//       if (*psVar16 == 2) {
//         iVar23 = iVar23 + 1;
//       }
//       iVar25 = iVar25 + 1;
//       psVar16 = psVar16 + 0x10;
//     } while (iVar25 < (int)local_38);
//   }
//   DisposeAreaMap(GlobalAreaMap);
//   GlobalAreaMap = (ulong *)0x0;
//   ResetAllMisc();
//   ClearItemLayout();
//   puVar10 = (undefined *)DAT_80097a70;
//   if (DAT_80097a70 != (OrnamentArchiveType *)0x0) {
//     iVar25 = 0;
//     if (0 < *(short *)((int)DAT_80097a70 + 0x5c)) {
//       do {
//         iVar17 = iVar25 * 4;
//         iVar25 = iVar25 + 1;
//         DisposeOrnament(*(OrnamentType **)(iVar17 + *(int *)(puVar10 + 0x60)));
//       } while (iVar25 < *(short *)(puVar10 + 0x5c));
//     }
//     vfree(*(undefined **)(puVar10 + 0x60));
//     vfree(*(undefined **)(puVar10 + 100));
//     vfree(puVar10);
//   }
//   puVar10 = (undefined *)DAT_80097a74;
//   if (DAT_80097a74 != (OrnamentArchiveType *)0x0) {
//     iVar25 = 0;
//     if (0 < *(short *)((int)DAT_80097a74 + 0x5c)) {
//       do {
//         iVar17 = iVar25 * 4;
//         iVar25 = iVar25 + 1;
//         DisposeOrnament(*(OrnamentType **)(iVar17 + *(int *)(puVar10 + 0x60)));
//       } while (iVar25 < *(short *)(puVar10 + 0x5c));
//     }
//     vfree(*(undefined **)(puVar10 + 0x60));
//     vfree(*(undefined **)(puVar10 + 100));
//     vfree(puVar10);
//   }
//   puVar15 = PathFileRead(ImagePath,(uchar *)s_TIM_TPD_80097a78);
//   LoadTIMpackAndFree(puVar15);
//   puVar15 = PathFileRead((uchar *)"K:\\WORK\\CDIMAGE\\IMAGE\\",(uchar *)"COMMON.TPD");
//   LoadTIMpackAndFree(puVar15);
//   local_40 = (ulong *)valloc(0x6b800);
//   puVar15 = PathFileRead(ImagePath,(uchar *)"OBJECTS.MAD");
//   DAT_80097a74 = LoadOrnamentArchive(puVar15,&World);
//   if (0 < ModelSlot.max) {
//     iVar25 = 0;
//     if (0 < ModelSlot.n) {
//       iVar17 = 0;
//       do {
//         pOVar20 = *(OrnamentType **)((int)&(ModelSlot.slot)->model + iVar17);
//         if (((uint)pOVar20 & 0xff000000) == 0x80000000) {
//           DisposeOrnament(pOVar20);
//         }
//         iVar25 = iVar25 + 1;
//         iVar17 = iVar17 + 0xc;
//       } while (iVar25 < ModelSlot.n);
//     }
//     vfree((undefined *)ModelSlot.slot);
//   }
//   ModelSlot.max = iVar23 + 500;
//   ModelSlot.slot = (tag_ObjectSlotType *)valloc((iVar23 + 500) * 0xc);
//   iVar23 = 0;
//   ModelSlot.n = 0;
//   psVar16 = local_3c;
// LAB_8003ae28:
//   if ((int)local_38 <= iVar23) {
//     sprintf((char *)auStack_180,s_map_mad_80097a90);
//     vfree((undefined *)local_40);
//     iVar23 = 0;
//     local_40 = PathFileRead(ImagePath,auStack_180);
//     local_34 = local_40 + 2;
//     DAT_80097a70 = LoadOrnamentArchive(local_40,&World);
//     for (iVar25 = 0; pOVar11 = DAT_80097a70, iVar25 < DAT_80097a70->n; iVar25 = iVar25 + 1) {
//       iVar17 = *(int *)(iVar23 + (int)DAT_80097a70->object);
//       uVar14 = local_34[iVar25 * 4];
//       *(int *)(iVar17 + 0x18) = *(int *)(iVar17 + 0x18) * 10;
//       iVar17 = *(int *)(iVar23 + (int)pOVar11->object);
//       *(int *)(iVar17 + 0x1c) = *(int *)(iVar17 + 0x1c) * 10;
//       iVar17 = *(int *)(iVar23 + (int)pOVar11->object);
//       *(int *)(iVar17 + 0x20) = *(int *)(iVar17 + 0x20) * 10;
//       pOVar12 = DAT_80097a70;
//       iVar17 = *(int *)(*(int *)(iVar23 + (int)pOVar11->object) + 0x18);
//       local_30 = (short)uVar14 * -0xe;
//       if (iVar17 < 0) {
//         uVar18 = iVar17 / 16000 - 1;
//       }
//       else {
//         uVar18 = iVar17 / 16000;
//       }
//       iVar17 = *(int *)(*(int *)(iVar23 + (int)DAT_80097a70->object) + 0x1c);
//       if (iVar17 < 0) {
//         uVar19 = iVar17 / 16000 - 1;
//       }
//       else {
//         uVar19 = iVar17 / 16000;
//       }
//       iVar17 = *(int *)(*(int *)(iVar23 + (int)DAT_80097a70->object) + 0x20);
//       if (iVar17 < 0) {
//         uVar22 = iVar17 / 16000 - 1;
//       }
//       else {
//         uVar22 = iVar17 / 16000;
//       }
//       iVar17 = *(int *)(iVar23 + (int)DAT_80097a70->object);
//       *(uint *)(iVar17 + 0x50) = *(uint *)(iVar17 + 0x50) | 0x400;
//       UpdateOrnament(*(OrnamentType **)(iVar23 + (int)pOVar12->object),0);
//       pWVar24 = WorldMap[uVar18 & 7][uVar19 & 7] + (uVar22 & 7);
//       pOVar20 = *(OrnamentType **)(iVar23 + (int)DAT_80097a70->object);
//       if (ModelSlot.max <= ModelSlot.n) {
//         AdtMessageBox("ModelSlot Overflow");
//       }
//       ModelSlot.slot[ModelSlot.n].model = pOVar20;
//       ModelSlot.slot[ModelSlot.n].next = pWVar24->top;
//       ModelSlot.slot[ModelSlot.n].ModelSize = (short)local_30;
//       ModelSlot.slot[ModelSlot.n].ShiftY = 0;
//       iVar23 = iVar23 + 4;
//       pWVar24->top = ModelSlot.slot + ModelSlot.n;
//       ModelSlot.n = ModelSlot.n + 1;
//     }
//     vfree((undefined *)in_a0);
//     jt_init4();
//     return -1;
//   }
//   switch(*psVar16) {
//   case 0:
//     sprintf((char *)auStack_180,s__s_ACM_80097a80,psVar16 + 2);
//     DisposeAreaMap(GlobalAreaMap);
//     puVar15 = PathFileRead(ImagePath,auStack_180);
//     GlobalAreaMap = LoadAreaMap(puVar15);
//     if (StageID != 4) break;
//     puVar15 = PathFileRead(ImagePath,(uchar *)"BALMER.ACM");
//     DAT_800976e8 = FUN_8001ab64(puVar15);
//     goto LAB_8003b260;
//   case 2:
//     if ((char)psVar16[2] == '\0') {
//       pOVar20 = CreateCloneOrnament(*(OrnamentType **)(local_3c + psVar16[1] * 0x10 + 2));
//     }
//     else {
//       pOVar20 = DAT_80097a74->object[local_44];
//       local_44 = local_44 + 1;
//       *(OrnamentType **)(psVar16 + 2) = pOVar20;
//     }
//     (pOVar20->locate).coord.t[0] = *(long *)(psVar16 + 8);
//     (pOVar20->locate).coord.t[1] = *(long *)(psVar16 + 10);
//     (pOVar20->locate).coord.t[2] = *(long *)(psVar16 + 0xc);
//     UpdateOrnament(pOVar20,psVar16[0xe]);
//     iVar21 = *(int *)(psVar16 + 8);
//     iVar17 = iVar21 >> 0x1f;
//     iVar25 = iVar21 / 16000 + iVar17;
//     if (iVar21 < 0) {
//       uVar18 = (iVar25 - iVar17) - 1;
//     }
//     else {
//       uVar18 = iVar25 - iVar17;
//     }
//     iVar25 = *(int *)(psVar16 + 10);
//     if (iVar25 < 0) {
//       uVar19 = iVar25 / 16000 - 1;
//     }
//     else {
//       uVar19 = iVar25 / 16000;
//     }
//     iVar25 = *(int *)(psVar16 + 0xc);
//     if (iVar25 < 0) {
//       uVar22 = iVar25 / 16000 - 1;
//     }
//     else {
//       uVar22 = iVar25 / 16000;
//     }
//     GetCenterAndSize((pOVar20->object).tmd,&SStack_80,&local_48);
//     sVar13 = SStack_80.vy;
//     pWVar24 = WorldMap[uVar18 & 7][uVar19 & 7] + (uVar22 & 7);
//     iVar25 = local_48 / 2;
//     if (ModelSlot.max <= ModelSlot.n) {
//       AdtMessageBox("ModelSlot Overflow");
//     }
//     ModelSlot.slot[ModelSlot.n].model = pOVar20;
//     ModelSlot.slot[ModelSlot.n].next = pWVar24->top;
//     ModelSlot.slot[ModelSlot.n].ModelSize = (short)iVar25;
//     ModelSlot.slot[ModelSlot.n].ShiftY = sVar13;
//     pWVar24->top = ModelSlot.slot + ModelSlot.n;
//     ModelSlot.n = ModelSlot.n + 1;
//     break;
//   case 3:
//     psVar1 = psVar16 + 0xe;
//     psVar2 = psVar16 + 1;
//     psVar3 = psVar16 + 8;
//     psVar4 = psVar16 + 10;
//     psVar5 = psVar16 + 0xc;
//     psVar16 = psVar16 + 0x10;
//     BreedLife((int)*psVar2,*(undefined4 *)psVar3,*(undefined4 *)psVar4,*(undefined4 *)psVar5,
//               *(undefined4 *)psVar1);
//     iVar23 = iVar23 + 1;
//     goto LAB_8003ae28;
//   case 4:
//     memset((uchar *)&local_60,'\0',0x14);
//     local_78.type = (TItemType)psVar16[1];
//     local_78.locate.vx = *(long *)(psVar16 + 8);
//     local_78.locate.vy = *(long *)(psVar16 + 10);
//     local_78.locate.vz = *(long *)(psVar16 + 0xc);
//     local_78.locate.pad = local_50;
//     local_60 = local_78.type;
//     local_5c = local_78.locate.vx;
//     local_58 = local_78.locate.vy;
//     local_54 = local_78.locate.vz;
//     ReqItemStay(&local_78);
//     break;
//   case 5:
//     sprintf((char *)auStack_180,s__s_TIM_80097a88,psVar16 + 2);
//     puVar15 = PathFileRead((uchar *)"K:\\WORK\\CDIMAGE\\IMAGE\\",auStack_180);
//     LoadTIMAndFree(puVar15);
//     goto LAB_8003b260;
//   case 0xb:
//     goto switchD_8003ae64_caseD_b;
//   }
// LAB_8003b260:
//   psVar16 = psVar16 + 0x10;
//   iVar23 = iVar23 + 1;
//   goto LAB_8003ae28;
// switchD_8003ae64_caseD_b:
//   piVar6 = (int *)(psVar16 + 2);
//   piVar7 = (int *)(psVar16 + 4);
//   piVar8 = (int *)(psVar16 + 6);
//   piVar9 = (int *)(psVar16 + 8);
//   psVar16 = psVar16 + 0x10;
//   AddMisc(*piVar6,*piVar7,*piVar8,*piVar9);
//   iVar23 = iVar23 + 1;
//   goto LAB_8003ae28;
// }

#endif /* NON_MATCHING */
