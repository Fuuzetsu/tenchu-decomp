#include "common.h"
#include "mission_editor.h"
#include "sprite_strip.h"
#include "../main.exe/graphics.h"
#include "../main.exe/timpack.h"

static const char EditorImageArchive[] = "K:\\WORK\\CDIMAGE\\TRIAL\\image\\mapmake.arc";
static const char EditorBackgroundImage[] = "K:\\WORK\\CDIMAGE\\TRIAL\\image\\dragon.tim";
static const char EditorButtonImage[] = "K:\\WORK\\CDIMAGE\\DEMO\\start\\mbuttonj.tim";
static const char EditorCharacterImages[] = "K:\\WORK\\CDIMAGE\\TRIAL\\image\\enemyimg.tpd";

/* MAPMAKE.ARC's ten TIM entries, in archive order. */
enum
{
    EDITOR_IMAGE_COMMAND_BACKGROUND,
    EDITOR_IMAGE_COMMAND_LABELS,
    EDITOR_IMAGE_RESUME_HINT,
    EDITOR_IMAGE_SCROLL_ARROWS,
    EDITOR_IMAGE_MAP_CURSOR,
    EDITOR_IMAGE_PALETTE_HIGHLIGHT,
    EDITOR_IMAGE_COMPASS,
    EDITOR_IMAGE_PATROL_ARROWS,
    EDITOR_IMAGE_WAIT_ARROWS,
    EDITOR_IMAGE_WAIT_DIGITS
};

/* Reloaded after a playtest, which replaces the editor's VRAM resources. */
void LoadTrialEditorGraphics(void)
{
    GsIMAGE image;
    ArcFile *archive;
    u_long *data;
    Sprite3D *sprite;
    BackGround *background;
    short i;

    SetDispMask(0);
    archive = (ArcFile *)FileRead(EditorImageArchive);
    LoadTrialSpriteStrip(archive, &TrialEditorCommandBackground, EDITOR_IMAGE_COMMAND_BACKGROUND, 0);
    TrialEditorCommandBackground.x = -100;
    LoadTrialSpriteStrip(archive, TrialEditorCommandLabels, EDITOR_IMAGE_COMMAND_LABELS,
                         TRIAL_EDITOR_COMMAND_COUNT);
    for (i = 0; i < TRIAL_EDITOR_COMMAND_COUNT; i++)
        TrialEditorCommandLabels[i].x = TrialEditorCommandBackground.x + 2;
    LoadTrialSpriteStrip(archive, &TrialEditorResumeHint, EDITOR_IMAGE_RESUME_HINT, 0);
    TrialEditorResumeHint.x = TrialEditorCommandBackground.x - 22;

    LoadTrialSpriteStrip(archive, TrialEditorScrollArrows, EDITOR_IMAGE_SCROLL_ARROWS, 4);
    TrialEditorScrollArrows[0].x = 144;
    TrialEditorScrollArrows[2].x = -144;
    TrialEditorScrollArrows[1].y = 76;
    TrialEditorScrollArrows[3].y = -104;
    LoadTrialSpriteStrip(archive, &TrialEditorCursors.map, EDITOR_IMAGE_MAP_CURSOR, 0);
    LoadTrialSpriteStrip(archive, &TrialEditorCursors.palette, EDITOR_IMAGE_PALETTE_HIGHLIGHT, 0);
    TrialEditorCursors.palette.scalex = TrialEditorCursors.palette.scaley = 0x1100;
    TrialEditorCursors.palette.y = 99;
    TrialEditorCursors.palette.attribute |= GS_ATTR_SEMITRANS_ADD;
    LoadTrialSpriteStrip(archive, TrialPatrolArrows, EDITOR_IMAGE_PATROL_ARROWS, 4);
    LoadTrialSpriteStrip(archive, &TrialPatrolArrows[4], EDITOR_IMAGE_WAIT_ARROWS, 4);
    LoadTrialSpriteStrip(archive, TrialPatrolWaitDigits, EDITOR_IMAGE_WAIT_DIGITS, 10);

    data = get_tim_from_archive(archive, EDITOR_IMAGE_COMPASS);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    setPolyFT4(&TrialPreviewDirectionQuad);
    setRGB0(&TrialPreviewDirectionQuad, 128, 128, 128);
    TrialPreviewDirectionQuad.clut = getClut(image.cx, image.cy);
    TrialPreviewDirectionQuad.tpage = getTPage(0, 0, image.px, image.py);
    TrialPreviewDirectionQuad.u0 = (short)(image.px % 64) * 4;
    TrialPreviewDirectionQuad.v0 = image.py % 256;
    TrialPreviewDirectionQuad.u1 = TrialPreviewDirectionQuad.u0 + (image.pw * 4 - 1);
    TrialPreviewDirectionQuad.v1 = TrialPreviewDirectionQuad.v0;
    TrialPreviewDirectionQuad.u2 = TrialPreviewDirectionQuad.u0;
    TrialPreviewDirectionQuad.v2 = TrialPreviewDirectionQuad.v0 + (image.ph - 1);
    TrialPreviewDirectionQuad.u3 = TrialPreviewDirectionQuad.u1;
    TrialPreviewDirectionQuad.v3 = TrialPreviewDirectionQuad.v2;
    vfree(archive);

    if (TrialEditorBackground)
        DisposeBG(TrialEditorBackground);
    data = FileRead(EditorBackgroundImage);
    GetTIMInfo(data, &image);
    LoadTIMAndFree(data);
    background = SetupBG(&image, 320, 240);
    TrialEditorBackground = background;
    background->sz = 100;
    for (i = 0; i < background->map.ncellw * background->map.ncellh; i++)
        background->index[i] = i;

    data = FileRead(EditorButtonImage);
    GetTIMInfo(data, &image);
    LoadTIMAndFree(data);
    image.ph /= 2;
    sprite = SetupSprite(NULL, &image);
    TrialEditorConfirmButtons = sprite->sprite;
    vfree(sprite);
    TrialEditorConfirmButtons.y = 60;
    LoadTIMpackAndFree(FileRead(EditorCharacterImages));
    SetDispMask(1);
}
