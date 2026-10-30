#include "common.h"
#include "trial.exe.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"
#include "../main.exe/vmemory.h"

void InitTrialMenuGraphics(void)
{
    TrialMenuCursor = 0;
    TrialMenuBackground = 0;
}

void DisposeTrialMenuGraphics(void)
{
    if (TrialMenuCursor != 0)
        vfree(TrialMenuCursor);
    if (TrialMenuBackground != 0)
        DisposeBG(TrialMenuBackground);
}

void LoadTrialMenuGraphics(void)
{
    /* The original retains an unused 32-byte local in this image loader.
     * GsIMAGE is the inferred workspace type; no field is accessed. */
    GsIMAGE image;

    TrialMenuBackground = load_background_(get_tim_from_archive(TrialImageArchive, 0));
    TrialMenuCursor = load_archive_sprite_(TrialImageArchive, 1);
    TrialMenuCursor->sprite.y = -70;
}

void LoadTrialArchiveImage(ArcFile *archive, int index)
{
    LoadTIM(get_tim_from_archive(archive, index));
}
