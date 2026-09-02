#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CVAsetup(void);
 *     CHRANIM.C:64, 15 src lines, frame 88 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     unsigned char [50] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAdata;
 *     extern int StageID;
 *     extern struct POLY_F4 TelopbgP;
 * END PSX.SYM */

extern char *STAGE_ANIMATION_PREFICES[N_LANGUAGES];
extern char fmt_stage_cad[];       /* %sSTAGE%d%c.CAD */
extern char path_anim_tanka_tpd[]; /* K:\\WORK\\CDIMAGE\\ANIM\\tanka.tpd */

extern Sprite3D *TANKA_SPRITES_[N_TANKA_SPRITES];

extern void vfree(void *p);
extern int sprintf(char *buf, char *fmt, ...);
extern short GetTIMpackInfo(unsigned long *adr, GsIMAGE *image, int idx);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern void LoadTIMpackAndFree(u_long *adr);

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

void CVAsetup(void)
{
    s16 i;
    u_long *adr;
    Sprite3D *sprite;
    Sprite3D *slot;
    int letter;
    u8 name[50];
    GsIMAGE image;

    if (CVAdata != 0)
    {
        vfree(CVAdata);
    }
    letter = 'A';
    if (PSTATE->CharType == RIKIMARU_0)
    {
        letter = 'R';
    }
    sprintf((char *)name, fmt_stage_cad,
            STAGE_ANIMATION_PREFICES[PSTATE->language],
            STAGE_NUMBER(StageID), letter);
    CVAdata = (CVAType *)FileRead(name);

    SetPolyF4(&TelopbgP);
    TelopbgP.b0 = 1;
    TelopbgP.g0 = 1;
    TelopbgP.r0 = 1;
    TelopbgP.x2 = -(SCREEN_W / 2);
    TelopbgP.x0 = -(SCREEN_W / 2);
    TelopbgP.x3 = SCREEN_W / 2;
    TelopbgP.x1 = SCREEN_W / 2;

    if (StageID == STAGE_ID_CORRUPT_MINISTER &&
        PSTATE->CharType == RIKIMARU_0)
    {
        adr = FileRead((u8 *)path_anim_tanka_tpd);
        for (i = 0; i < N_TANKA_SPRITES; i++)
        {
            GetTIMpackInfo(adr, &image, i);
            sprite = SetupSprite(0, &image);
            TANKA_SPRITES_[i] = sprite;
            sprite->attribute |= MODEL_ATTR_HIDDEN;
            TANKA_SPRITES_[i]->sprite.x = (2 - i) * 20 + 10;
            TANKA_SPRITES_[i]->sprite.y = (i % 3) * 8 - 4;
            slot = TANKA_SPRITES_[i];
            slot->sprite.b = 0;
            slot->sprite.g = 0;
            slot->sprite.r = 0;
        }
        TANKA_SPRITES_[N_TANKA_SPRITES - 1]->sprite.x -= 8;
        TANKA_SPRITES_[N_TANKA_SPRITES - 1]->sprite.y = 40;
        LoadTIMpackAndFree(adr);
    }
}
