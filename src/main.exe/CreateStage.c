#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include <psxsdk/libgpu.h>
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CreateStage(int StageNo, int CharType);
 *     WORLD.C:139, 114 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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

typedef struct
{
    char *path[4];
} TitlePathBlock;

typedef struct
{
    u8 unused[32];
    TitlePathBlock title;
} CreateStageTitleScratch;

extern s16 Humans;
extern s32 StageID;
extern u8 *ImagePath;
extern GsOT *OTablePt;
extern s16 SkipFrame;
extern volatile s32 SystemFlag;
extern GsRVIEW2 ViewInfo;
extern s32 DepthPoint;
extern TStageConfig StageConfig[];
extern TitlePathBlock TITLE_SPRITES_PTRS;
extern u8 CHOSEN_LANGUAGE;
extern volatile u8 STAGE_LAYOUT_NUMBER;
extern TCameraStatus CamState;
extern char D_8001204C[];
extern char D_800120A0[];

extern void SetDepthQ(s32 dqa, s32 dqb);
extern void DestroyTraceLine(TraceLine *trace);
extern void KillHumanoid(Humanoid *human);
extern void SetupSoundEffect(s16 character, s16 stage);
extern void DoBriefingAndInventorySelection(void);
extern GsIMAGE *GetImage(s32 id);
extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *poly, s16 x, s16 y);
extern BackGround *FUN_8004f4f8(u_long *data);
extern void vfree(void *ptr);
extern void FUN_80038ce0(void);
extern void StartDrawing(void);
extern short DrawBG(BackGround *bg);
extern void EndDrawing(s32 mode);
extern void DisposeBG(BackGround *bg);
extern short LoadConstruction(u_long *data);
extern void initialise_font(void);
extern void InitializeImage(void);
extern void ResetInfoview(s32 stage);
extern void SetupThinkFunction(Humanoid *human, s16 type);
extern void create_ninken_character_(s16 type, s32 stage);
extern void load_layout(s32 layout);
extern void leLayoutEnemy(s32 mode);
extern void CVAsetup(void);
extern void SetupStageSequence(void);

void CreateStage(int StageNo, int CharType)
{
    Humanoid *target;
    POLY_FT4 poly;
    CreateStageTitleScratch scratch;
    TStageConfig *base;
    TStageConfig *stage;
    short mode;
    short stageNo;
    u_long *data;
    GsIMAGE *image;
    BackGround *bg;
    Humanoid *human;
    int i;
    s32 px;
    s32 py;
    s32 pz;

    if ((u32)StageNo >= 11)
    {
        AdtMessageBox(D_8001204C, StageNo);
        return;
    }

    SetDepthQ(-0x7EF4, 0x2F282E0);
    DepthPoint = 0x4E2;

    while (1)
    {
        if (Humans <= 0)
            break;
        target = HumanGroup[0];
        DestroyTraceLine(target->trace);
        KillHumanoid(target);
    }

    mode = (short)CharType;
    base = StageConfig;
    stage = &base[StageNo];
    stageNo = (short)(StageNo + 1);
    ImagePath = (u8 *)stage->path;
    StageID = StageNo;
    SetupSoundEffect(mode, stageNo);
    DoBriefingAndInventorySelection();

    scratch.title = TITLE_SPRITES_PTRS;
    data = PathFileRead(ImagePath,
                        (u8 *)scratch.title.path[CHOSEN_LANGUAGE]);
    image = GetImage(0x2D);
    SetupImageToPolyFT4(image, &poly, 0x34, 0x43);
    bg = FUN_8004f4f8(data);
    vfree(data);

    FUN_80038ce0();
    StartDrawing();
    GsSortPoly(&poly, OTablePt, 0);
    DrawBG(bg);
    SkipFrame = 2;
    EndDrawing(0);
    StartDrawing();
    SkipFrame = 2;
    EndDrawing(0);
    DisposeBG(bg);

    SetupAppearance(mode, stageNo);
    LoadConstruction(PathFileRead(ImagePath, (u8 *)D_800120A0));
    initialise_font();
    InitializeImage();
    ResetInfoview(StageNo);

    human = BreedLife(mode, 0, 0, 0, 0);
    SetupThinkFunction(human, 0x1111);
    human->model->locate.coord.t[0] = stage->px;
    human->model->locate.coord.t[1] = stage->py;
    human->model->locate.coord.t[2] = stage->pz;
    human->model->rotate.vx = 0;
    human->model->rotate.vy = (short)stage->pr;
    human->model->rotate.vz = 0;
    CamState.Owner = human;

    for (i = 0; i < 20; i++)
        human->item[i] =
            ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->counts[i];

    create_ninken_character_(CharType, StageNo);
    if (((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->layout >= 3)
    {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->layout = rand() % 3;
        SystemFlag |= 8;
    }
    else
    {
        SystemFlag &= ~8;
    }
    load_layout(STAGE_LAYOUT_NUMBER);
    leLayoutEnemy(1);

    px = StageConfig[StageNo].px;
    py = StageConfig[StageNo].py;
    pz = StageConfig[StageNo].pz;
    ViewInfo.vpx = px;
    do {
      ViewInfo.vpy = py - 10000;
    } while (0);
    do {
      ViewInfo.vpz = pz;
    } while (0);
    ViewInfo.vrx = px;
    ViewInfo.vry = py;
    ViewInfo.vrz = pz;

    StartDrawing();
    CVAsetup();
    SetupStageSequence();
    PadProc();
    EndDrawing(0);
}
