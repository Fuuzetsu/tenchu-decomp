#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CreateStage(int StageNo, int CharType);
 *     WORLD.C:139, 114 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       int StageNo
 *     param $s5       int CharType
 *     reg   $s0       struct Humanoid * target
 *     reg   $s2       int i
 *     stack sp+24     struct POLY_FT4 ply_ten
 *     stack sp+64     struct POLY_FT4 ply_title1
 *     stack sp+104    struct POLY_FT4 ply_title2
 *     stack sp+144    struct GsIMAGE image
 *     reg   $s1       unsigned long * dat
 *     reg   $s0       struct Humanoid * human
 *     reg   $a0       int i
 *     reg   $s0       void * pBuf
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern unsigned char *ImagePath;
 *     extern struct GsOT *OTablePt;
 *     extern short SkipFrame;
 *     extern enum TSystemFlag SystemFlag;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CreateStage", CreateStage);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CreateStage", initialise_stage_and_character__override__prt_8003a3dc_e0d8bb81);

// triage: HARD — 249 insns, mul/div, 1 loop, 31 callees, ~0.09 to AddMisc
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// void CreateStage(int StageNo,int CharType)
//
// {
//   ulong *pt;
//   GsIMAGE *pGVar1;
//   BackGround *bg;
//   Humanoid *human;
//   int iVar2;
//   Humanoid *pHVar3;
//   int iVar4;
//   short stage;
//   short mode;
//   undefined1 auStack_78 [72];
//   undefined *local_30 [4];
//
//   if ((uint)StageNo < 0xb) {
//     SetDepthQ(0xffff810c,0x2f282e0);
//     _DepthPoint = 0x4e2;
//     while (pHVar3 = HumanGroup[0], mode = (short)CharType, 0 < Humans) {
//       DestroyTraceLine(HumanGroup[0]->trace);
//       KillHumanoid(pHVar3);
//     }
//     ImagePath = StageConfig[StageNo].path;
//     stage = (short)((uint)((StageNo + 1) * 0x10000) >> 0x10);
//     StageID = StageNo;
//     SetupSoundEffect(mode,stage);
//     DoBriefingAndInventorySelection();
//     local_30[0] = PTR_s_title_e_tim_80012090;
//     local_30[1] = PTR_s_title_f_tim_80012094;
//     local_30[2] = PTR_s_title_i_tim_80012098;
//     local_30[3] = PTR_s_title_j_tim_8001209c;
//     pt = PathFileRead(ImagePath,local_30[(byte)PersistentState._94_1_]);
//     pGVar1 = GetImage(0x2d);
//     FUN_8004eaf0(pGVar1,auStack_78,0x34,0x43);
//     bg = (BackGround *)FUN_8004f4f8(pt);
//     vfree((undefined *)pt);
//     FUN_80038ce0();
//     StartDrawing();
//     GsSortPoly(auStack_78,OTablePt,0);
//     DrawBG(bg);
//     SkipFrame = 2;
//     EndDrawing(0);
//     StartDrawing();
//     SkipFrame = 2;
//     EndDrawing(0);
//     DisposeBG(bg);
//     SetupAppearance(mode,stage);
//     PathFileRead(ImagePath,(uchar *)"STAGE.CON");
//     LoadConstruction();
//     FUN_80057698();
//     InitializeImage();
//     ResetInfoview(StageNo);
//     human = (Humanoid *)BreedLife((int)mode,0,0,0,0);
//     SetupThinkFunction(human,0x1111);
//     (human->model->locate).coord.t[0] = StageConfig[StageNo].px;
//     (human->model->locate).coord.t[1] = StageConfig[StageNo].py;
//     (human->model->locate).coord.t[2] = StageConfig[StageNo].pz;
//     (human->model->rotate).vx = 0;
//     (human->model->rotate).vy = (short)StageConfig[StageNo].pr;
//     (human->model->rotate).vz = 0;
//     pHVar3 = human;
//     iVar2 = 0;
//     CamState.Owner = human;
//     do {
//       iVar4 = iVar2 + 1;
//       pHVar3->item[0] = (&PersistentState.field_0x7)[iVar2];
//       pHVar3 = (Humanoid *)((int)(human->pad).stream + iVar2 + -0x17);
//       iVar2 = iVar4;
//     } while (iVar4 < 0x14);
//     FUN_8003d528((int)mode,StageNo);
//     if ((byte)PersistentState._6_1_ < 3) {
//       SystemFlag = SystemFlag & 0xfffffff7;
//     }
//     else {
//       iVar2 = rand();
//       PersistentState._6_1_ =
//            (char)iVar2 +
//            ((char)((ulonglong)((longlong)iVar2 * 0x55555556) >> 0x20) - (char)(iVar2 >> 0x1f)) * -3;
//       SystemFlag = SystemFlag | 8;
//     }
//     FUN_8003cc78(PersistentState._6_1_);
//     leLayoutEnemy(1);
//     ViewInfo.vpx = StageConfig[StageNo].px;
//     ViewInfo.vry = StageConfig[StageNo].py;
//     ViewInfo.vpz = StageConfig[StageNo].pz;
//     ViewInfo.vpy = ViewInfo.vry + -10000;
//     ViewInfo.vrx = ViewInfo.vpx;
//     ViewInfo.vrz = ViewInfo.vpz;
//     StartDrawing();
//     CVAsetup();
//     SetupStageSequence();
//     PadProc();
//     EndDrawing(0);
//   }
//   else {
//     AdtMessageBox("illigal stage id %d",StageNo);
//   }
//   return;
// }
