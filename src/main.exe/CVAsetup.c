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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 *     extern int StageID;
 *     extern struct POLY_F4 TelopbgP;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 1 instruction too long (135 vs 134), ~318 of 536
 * raw bytes differ as a consequence (everything after the extra
 * instruction shifts). Every block/loop/offset/call here is confirmed
 * correct against the raw .s (including the Sprite3D+embedded-GsSPRITE
 * struct, the fan layout math, and the fresh-vs-cached pointer reuse
 * rule) — the SOLE residual is that the target shares ONE `lui` between
 * two DIFFERENT same-page globals: CHOSEN_LANGUAGE (referenced once, near
 * the top, for STAGE_ANIMATION_PREFICES[CHOSEN_LANGUAGE]) and
 * CHOSEN_CHARACTER's SECOND (late, stage-10 guard) reference reuse the
 * identical `%hi` (both addresses are 0x8001xxxx) via one register (a
 * callee-saved $s1, kept alive across 3 intervening calls: sprintf,
 * FileRead, SetPolyF4) instead of each materialising its own `lui`. This
 * draft's cc1 invocation re-materialises `%hi(CHOSEN_CHARACTER)` fresh at
 * that reference instead. Tried: a named `chosenChar` temp read once and
 * reused for both checks (makes it WORSE — cc1 then caches the VALUE
 * instead of the address, diverging structurally); narrowing `letter` to
 * u8 (no effect, confirming it isn't a register-pressure-from-that-local
 * issue). A bounded `tools/permute.py` run (~4 min, -j4, ~16.7k
 * iterations) plateaued at score 985, never near 0 — parked per the
 * cookbook's early-stop rather than chased further; this looks like a
 * genuine %hi-rematerialisation/CSE decision (a "second same-page
 * reference is worth caching" cost-model choice), not a plain register
 * swap, and no source lever tried moved it.
 */

/*
 * CVAsetup (0x8004ff98, 0x218 bytes) — prepares a CVA cutscene: frees any
 * previous PERSISTENT_EVENT_LIST_THING blob and loads the new one
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

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAsetup", CVAsetup);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAsetup", reload_animations__override__prt_80050014_a9b16ba2);
#else /* NON_MATCHING */

extern u32 *PERSISTENT_EVENT_LIST_THING;
extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_LANGUAGE;
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

void CVAsetup(void)
{
    s16 i;
    u32 *adr;
    Sprite3D *sprite;
    PositionalEntry *reload;
    int letter;
    u8 name[50];
    GsIMAGE image;

    if (PERSISTENT_EVENT_LIST_THING != 0)
    {
        vfree(PERSISTENT_EVENT_LIST_THING);
    }
    letter = 0x41;
    if (CHOSEN_CHARACTER == 0)
    {
        letter = 0x52;
    }
    sprintf(name, D_80013624, STAGE_ANIMATION_PREFICES[CHOSEN_LANGUAGE], StageID + 1, letter);
    PERSISTENT_EVENT_LIST_THING = FileRead(name);

    SetPolyF4(&TelopbgP);
    TelopbgP.b0 = 1;
    TelopbgP.g0 = 1;
    TelopbgP.r0 = 1;
    TelopbgP.x2 = -0xA0;
    TelopbgP.x0 = -0xA0;
    TelopbgP.x3 = 0xA0;
    TelopbgP.x1 = 0xA0;

    if (StageID == 10 && CHOSEN_CHARACTER == 0)
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

#endif /* NON_MATCHING */
