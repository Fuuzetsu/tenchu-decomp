#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "images.h"
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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *     extern struct TCameraStatus CamState;
 *     extern enum TSystemFlag SystemFlag;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/*
 * Build a mission from scratch: resolve the TStageConfig row for the
 * stage/character pair, run briefing and inventory selection, then load
 * and wire every subsystem — construction, layouts, enemies (and the
 * ninken summon), backgrounds, fonts, images, infoview, sound, CVA
 * cutscenes, and the stage sequence — showing the title card in between.
 */
typedef struct
{
    u8 unused[32];
    u8 *title[N_LANGUAGES];
} CreateStageTitleScratch;

extern s32 DepthPoint;
extern u8 *TITLE_SPRITES_PTRS[N_LANGUAGES];
extern u8 CHOSEN_LANGUAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern char fmt_illigal_stage_id[]; /* illigal stage id %d */
extern char path_stage_con[];       /* STAGE.CON */

extern void SetDepthQ(s32 dqa, s32 dqb);
extern void DestroyTraceLine(TraceLine *trace);
extern void DoBriefingAndInventorySelection(void);
extern GsIMAGE *GetImage(s32 id);
extern BackGround *load_background_(u_long *data);
extern void vfree(void *ptr);
extern void clear_screen_(void);
extern short DrawBG(BackGround *bg);
extern void DisposeBG(BackGround *bg);
extern short LoadConstruction(u_long *data);
extern void initialise_font(void);
extern void InitializeImage(void);
extern void ResetInfoview(s32 stage);
extern void SetupThinkFunction(Humanoid *human, TThinkType type);
extern void create_ninken_character_(s16 type, s32 stage);
extern void load_layout(s32 layout);
extern void CVAsetup(void);
extern void SetupStageSequence(void);

/* Only these two read-modify-writes need a localized volatile view to retain
 * the retail instruction schedule; SystemFlag itself is the ordinary shared
 * object used throughout the game. */
void CreateStage(stage_id StageNo, int CharType)
{
    Humanoid *target;
    POLY_FT4 ply_ten;
    CreateStageTitleScratch scratch;
    TStageConfig *base;
    TStageConfig *stage;
    u_long *dat;
    GsIMAGE *image;
    BackGround *bg;
    Humanoid *human;
    int i;
    s32 px;
    s32 py;
    s32 pz;

    if ((u32)StageNo >= N_STAGE_CONFIGS)
    {
        AdtMessageBox(fmt_illigal_stage_id, StageNo);
        return;
    }

    SetDepthQ(FOG_DQA, FOG_DQB);
    DepthPoint = DEPTH_LIMIT;

    while (1)
    {
        if (Humans <= 0)
            break;
        target = HumanGroup[0];
        DestroyTraceLine(target->trace);
        KillHumanoid(target);
    }

    base = StageConfig;
    stage = &base[StageNo];
    ImagePath = (u8 *)stage->path;
    StageID = StageNo;
    SetupSoundEffect(CharType, STAGE_NUMBER(StageNo));
    DoBriefingAndInventorySelection();

    __builtin_memcpy(scratch.title, TITLE_SPRITES_PTRS,
                     sizeof(scratch.title));
    dat = PathFileRead(ImagePath, scratch.title[CHOSEN_LANGUAGE]);
    image = GetImage(IMG_TEN_LOGO);
    SetupImageToPolyFT4(image, &ply_ten, 0x34, 0x43);
    bg = load_background_(dat);
    vfree(dat);

    clear_screen_();
    StartDrawing();
    GsSortPoly(&ply_ten, OTablePt, 0);
    DrawBG(bg);
    SkipFrame = SKIPFRAME_AFTER_LOAD;
    EndDrawing(0);
    StartDrawing();
    SkipFrame = SKIPFRAME_AFTER_LOAD;
    EndDrawing(0);
    DisposeBG(bg);

    SetupAppearance(CharType, STAGE_NUMBER(StageNo));
    LoadConstruction(PathFileRead(ImagePath, (u8 *)path_stage_con));
    initialise_font();
    InitializeImage();
    ResetInfoview(StageNo);

    human = BreedLife(CharType, 0, 0, 0, 0);
    SetupThinkFunction(human, THINK_MIX_PLAYER);
    human->model->locate.coord.t[0] = stage->px;
    human->model->locate.coord.t[1] = stage->py;
    human->model->locate.coord.t[2] = stage->pz;
    human->model->rotate.vx = 0;
    human->model->rotate.vy = (short)stage->pr;
    human->model->rotate.vz = 0;
    CamState.Owner = human;

    for (i = 0; i < N_LOADOUT_ITEMS; i++)
        human->item[i] =
            ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[i];

    create_ninken_character_(CharType, StageNo);
    if (((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->layout >= N_STAGE_LAYOUTS)
    {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->layout = rand() % N_STAGE_LAYOUTS;
        *(volatile TSystemFlag *)&SystemFlag |= SYSFLAG_RANDOM_LAYOUT;
    }
    else
    {
        *(volatile TSystemFlag *)&SystemFlag &= ~SYSFLAG_RANDOM_LAYOUT;
    }
    load_layout(STAGE_LAYOUT_NUMBER);
    leLayoutEnemy(1);

    px = StageConfig[StageNo].px;
    py = StageConfig[StageNo].py;
    pz = StageConfig[StageNo].pz;
    ViewInfo.vpx = px;
    /* One-shot fences here: byte-required (collapse measured; see cookbook). */
    /* The vpy boundary remains load-bearing. */
    do
    {
        ViewInfo.vpy = py - 10000;
    } while (0);
    /* Folded after flow to retain the old vpz allocation weight. */
    ViewInfo.vpz = ((u32)pz + (u32)pz) - (u32)pz;
    ViewInfo.vrx = px;
    ViewInfo.vry = py;
    ViewInfo.vrz = pz;

    StartDrawing();
    CVAsetup();
    SetupStageSequence();
    PadProc();
    EndDrawing(0);
}
