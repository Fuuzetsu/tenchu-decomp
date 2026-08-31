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

/*
 * STATUS: MATCHED — exact 536 bytes / 134 instructions.
 *
 * The final one-instruction residual was not a register-allocation quirk:
 * CHOSEN_CHARACTER and CHOSEN_LANGUAGE are fields +4 and +0x5e of the same
 * PersistentState blob at 0x80010000.  Expressing all three reads through
 * PSTATE makes cc1 materialise that common base once, retain it in $s1
 * across sprintf/FileRead/SetPolyF4, and reuse it for the late character
 * guard.  Separate extern globals force a second `lui`; caching only the
 * character VALUE also diverges because the target caches the address base.
 */

/*
 * CVAsetup (0x8004ff98, 0x218 bytes) — prepares a CVA cutscene: frees any
 * previous CVAdata blob and loads the new one
 * ("<lang-prefix>STAGE<n><A|R>.CAD", the trailing letter is the character's
 * initial — 'R' Rikimaru / 'A' Ayame), then a fixed
 * TelopbgP POLY_F4 letterbox (r0/g0/b0=1, x0..x3 = -0xA0/0xA0/-0xA0/0xA0 —
 * the canonical PsyQ SDK POLY_F4). Stage 10 (+ CHOSEN_CHARACTER==0) only: loads
 * "tanka.tpd" and populates 6 TANKA_SPRITES_ Sprite3D slots.
 * Each slot's `attribute` gets MODEL_ATTR_HIDDEN set,
 * and the embedded GsSPRITE's x/y are
 * laid out in a fan (`(2-i)*20+10`, `(i%3)*8-4`) — then the LAST slot's
 * embedded sprite is nudged (x -= 8, y = 40) before the tpd is freed.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The embedded-GsSPRITE x/y stores go through a FRESH
 *    `TANKA_SPRITES_[i]` re-read each time (matching
 *    Ghidra's own `*piVar2` — a dereference of the SLOT ADDRESS, not the
 *    `pSVar1` variable already holding the same value) — only the
 *    `attribute |= 1` update reuses `pSVar1` directly, and the r/g/b
 *    stores go through a third re-read held in `slot`. Same lever as
 *    CVArun's GsSortSprite re-read.
 *  - `letter` (the trailing filename letter) is a real `int` local,
 *    computed by a plain if/else BEFORE the sprintf call — writing it
 *    inline would evaluate it at the wrong point relative to the other
 *    vararg materialisation.
 */

extern char *STAGE_ANIMATION_PREFICES[];
extern char fmt_stage_cad[];       /* %sSTAGE%d%c.CAD */
extern char path_anim_tanka_tpd[]; /* K:\\WORK\\CDIMAGE\\ANIM\\tanka.tpd */

extern Sprite3D *TANKA_SPRITES_[6];

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
            STAGE_ANIMATION_PREFICES[PSTATE->language], StageID + 1, letter);
    CVAdata = (CVAType *)FileRead(name);

    SetPolyF4(&TelopbgP);
    TelopbgP.b0 = 1;
    TelopbgP.g0 = 1;
    TelopbgP.r0 = 1;
    TelopbgP.x2 = -0xA0;
    TelopbgP.x0 = -0xA0;
    TelopbgP.x3 = 0xA0;
    TelopbgP.x1 = 0xA0;

    if (StageID == STAGE_FREE_PRINCESS && PSTATE->CharType == RIKIMARU_0)
    {
        adr = FileRead((u8 *)path_anim_tanka_tpd);
        for (i = 0; i < 6; i++)
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
        TANKA_SPRITES_[5]->sprite.x -= 8;
        TANKA_SPRITES_[5]->sprite.y = 40;
        LoadTIMpackAndFree(adr);
    }
}
