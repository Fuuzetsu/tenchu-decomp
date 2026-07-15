#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CVAsetup(void);
 *     CHRANIM.C:64, 15 src lines, frame 88 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     stack sp+24     unsigned char [50] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAdata;
 *     extern int StageID;
 *     extern struct POLY_F4 TelopbgP;
 *     extern struct tag_TItem items[30];
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
 * ("<lang-prefix>STAGE<n><A|R>.CAD", the trailing letter selecting the
 * Attract/Retail-ish variant per CHOSEN_CHARACTER), then a fixed
 * TelopbgP POLY_F4 letterbox (r0/g0/b0=1, x0..x3 = -0xA0/0xA0/-0xA0/0xA0 —
 * item.h's proven POLY_F4). Stage 10 (+ CHOSEN_CHARACTER==0) only: loads
 * "tanka.tpd" and populates 6 TENCHU_POSITIONAL_DATA_AREA_ slots (each a
 * Sprite3D immediately followed by an EMBEDDED GsSPRITE — SetupSprite
 * vallocs 0x8C == sizeof(Sprite3D)+sizeof(GsSPRITE) exactly, confirmed
 * against CVArun.c's `+0x68`-as-GsSPRITE finding, and its `attribute`@0x5A
 * bit0/flag use there): each slot's own Sprite3D.attribute gets bit 0 set
 * (visible-but-hidden-until-faded), and the embedded GsSPRITE's x/y are
 * laid out in a fan (`(2-i)*20+10`, `(i%3)*8-4`) — then the LAST slot's
 * embedded sprite is nudged (x -= 8, y = 0x28) before the tpd is freed.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The embedded-GsSPRITE x/y stores go through a FRESH
 *    `TENCHU_POSITIONAL_DATA_AREA_[i]` re-read each time (matching
 *    Ghidra's own `*piVar2` — a dereference of the SLOT ADDRESS, not the
 *    `pSVar1` variable already holding the same value) — only the
 *    `attribute |= 1` update reuses `pSVar1` directly. Same lever as
 *    CVArun's GsSortSprite re-read.
 *  - `uVar3` (the trailing filename letter) is a real `int` local,
 *    computed by a plain if/else BEFORE the sprintf call — writing it
 *    inline would evaluate it at the wrong point relative to the other
 *    vararg materialisation.
 */

extern u32 *CVAdata;
extern char *STAGE_ANIMATION_PREFICES[];
extern char D_80013624[]; /* "%sSTAGE%d%c.CAD" */
extern char D_80013634[]; /* "K:\\WORK\\CDIMAGE\\ANIM\\tanka.tpd" */
extern int StageID;

typedef struct
{
    u_long tag;  /* 0x0 */
    u_char r0;   /* 0x4 */
    u_char g0;   /* 0x5 */
    u_char b0;   /* 0x6 */
    u_char code; /* 0x7 */
    s16 x0;      /* 0x8 */
    s16 y0;      /* 0xA */
    s16 x1;      /* 0xC */
    s16 y1;      /* 0xE */
    s16 x2;      /* 0x10 */
    s16 y2;      /* 0x12 */
    s16 x3;      /* 0x14 */
    s16 y3;      /* 0x16 */
} POLY_F4;

extern POLY_F4 TelopbgP;

typedef struct
{
    Sprite3D sprite; /* 0x00-0x67 */
    GsSPRITE gsp;    /* 0x68-0x8B (embedded — CVArun.c GsSortSprite's it) */
} PositionalEntry;

extern PositionalEntry *TENCHU_POSITIONAL_DATA_AREA_[6];

extern void vfree(void *p);
extern u32 *FileRead(char *path);
extern int sprintf(char *buf, char *fmt, ...);
extern void SetPolyF4(POLY_F4 *p);
extern short GetTIMpackInfo(u32 *adr, GsIMAGE *image, int idx);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern void LoadTIMpackAndFree(u32 *adr);

#define PSTATE ((PersistentState *)0x80010000)

void CVAsetup(void)
{
    s16 i;
    u32 *adr;
    Sprite3D *sprite;
    PositionalEntry *reload;
    int letter;
    u8 name[50];
    GsIMAGE image;

    if (CVAdata != 0)
    {
        vfree(CVAdata);
    }
    letter = 0x41;
    if (PSTATE->chr == 0)
    {
        letter = 0x52;
    }
    sprintf(name, D_80013624, STAGE_ANIMATION_PREFICES[PSTATE->language], StageID + 1, letter);
    CVAdata = FileRead(name);

    SetPolyF4(&TelopbgP);
    TelopbgP.b0 = 1;
    TelopbgP.g0 = 1;
    TelopbgP.r0 = 1;
    TelopbgP.x2 = -0xA0;
    TelopbgP.x0 = -0xA0;
    TelopbgP.x3 = 0xA0;
    TelopbgP.x1 = 0xA0;

    if (StageID == 10 && PSTATE->chr == 0)
    {
        adr = FileRead(D_80013634);
        for (i = 0; i < 6; i++)
        {
            GetTIMpackInfo(adr, &image, i);
            sprite = SetupSprite(0, &image);
            TENCHU_POSITIONAL_DATA_AREA_[i] = (PositionalEntry *)sprite;
            sprite->attribute = sprite->attribute | 1;
            TENCHU_POSITIONAL_DATA_AREA_[i]->gsp.x = (2 - i) * 0x14 + 10;
            TENCHU_POSITIONAL_DATA_AREA_[i]->gsp.y = (i % 3) * 8 - 4;
            reload = TENCHU_POSITIONAL_DATA_AREA_[i];
            reload->gsp.b = 0;
            reload->gsp.g = 0;
            reload->gsp.r = 0;
        }
        TENCHU_POSITIONAL_DATA_AREA_[5]->gsp.x = TENCHU_POSITIONAL_DATA_AREA_[5]->gsp.x - 8;
        TENCHU_POSITIONAL_DATA_AREA_[5]->gsp.y = 0x28;
        LoadTIMpackAndFree(adr);
    }
}
