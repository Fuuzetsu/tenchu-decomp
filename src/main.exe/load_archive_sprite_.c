#include "common.h"
#include "main.exe.h"
#include "item.h"

extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

void load_archive_sprite_(ArcFile *archive, int idx)
{
    GsIMAGE img;
    u_long *adr;

    adr = get_tim_from_archive(archive, idx);
    GetTIMInfo(adr, &img);
    LoadTIM(adr);
    SetupSprite(0, &img);
}
