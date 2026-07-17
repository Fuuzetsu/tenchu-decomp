#include "common.h"
#include "main.exe.h"

/*
 * start_demo_ (0x80055d64) — load and run the post-stage demo start screen:
 * fade in its five localized sprites, accept the continue/cancel inputs,
 * then release both archives and dispatch to the selected executable.
 *
 * STATUS: NON_MATCHING — complete guarded pure-C reconstruction. The draft
 * recovers the full five-state jump-table loop, including state 2's shared
 * prompt/input tail with state 3, resource ownership, timed sprite fades,
 * pad-edge detection and both cleanup paths. It has the exact 0x1a0 frame and
 * exact 2188-byte/547-instruction extent; 39 differing bytes (down from 75).
 * Build with `NON_MATCHING=start_demo_ ./Build`.
 *
 * This pass (75 -> 39), four independent levers found via autorules +
 * permuter + regalloc.py/rtlguide, applied and re-measured one at a time:
 *   1. (75 -> 48) The state-3 `do { pad = GetRealPad(0); } while (0);` fence
 *      was actively HARMFUL, not neutral: unwrapping it (autorules
 *      fence-unwrap) let cc1 schedule the whole GsSortSprite(&gov_title,...)
 *      argument setup (a0/a1/a2 materialization) BEFORE `old_pad = pad;
 *      new_press = pad & (pad^previous_pad);` instead of after, which is what
 *      lets reorg sink the final `and` (computing new_press) into the FIRST
 *      GsSortSprite call's jal delay slot exactly as the target does
 *      (0x56444-0x56464 cluster, -27 bytes in one edit).
 *   2. (48 -> 43) A fresh, single-use `s32 clear_b = 0;` at the top of the
 *      function, passed as `ClearImage(&clear_rect, 0, 0, clear_b)` instead
 *      of a literal `0`, closes the FadeOutDirect(0x20,2,8,8,8) shared-constant
 *      cluster (0x55df4-0x55e04) — a global-allocation-priority rebalancing
 *      effect with NO textual connection to FadeOutDirect (found by the
 *      permuter; verified `color` cannot substitute for `clear_b` — reusing
 *      an existing local instead of a fresh one costs a frame slot, LENGTH
 *      MISMATCH +4). See the new cookbook rule below.
 *   3. (43 -> 42, then 42 -> 39 combined with #4) Naming the persistent[]
 *      loop's multiply subterm as `chr_offset = CHOSEN_CHARACTER * 0x20;`
 *      before `persistent[(i + chr_offset) + 0x40c]` closes the `addu`
 *      operand-order tie at 0x55dd8. Verified the ALGEBRA is irrelevant —
 *      `16 * (2 * CHOSEN_CHARACTER)` in place of `CHOSEN_CHARACTER * 0x20`
 *      compiles byte-identical to the unnamed form (cc1 folds the
 *      reassociation); only the fresh NAMED TEMP's presence moves bytes.
 *   4. (42 -> 39 combined) Pre-computing the sprintf 4th argument into
 *      `char *prefix = GOV_RESOURCE_PREFIX_PTRS[language_state[0x5e]];`
 *      (mirroring how `resource_root` is already pre-assigned) shrinks the
 *      sprintf address-materialization cluster from 11 insns/38 bytes to 9
 *      insns/35 bytes and gets the FINAL `lw a3,0(v0)` into the correct `a3`
 *      register (previously the whole chain was 8 insns of pure insert/delete
 *      against target). Order matters: `resource_root` must be assigned
 *      BEFORE `prefix`, not after (swapping costs +6 bytes, 46 vs 40).
 *
 * Ideas tried and REJECTED (measured, not guessed):
 *   - `GOV_RESOURCE_PREFIX_PTRS[CHOSEN_LANGUAGE]` in place of
 *     `[language_state[0x5e]]` at one or both use sites: the disassembly's
 *     `%lo(CHOSEN_LANGUAGE)` label on the FIRST access and raw `0x5E` on the
 *     second is a splat SYMBOL-BY-ADDRESS relabeling artifact, not evidence
 *     of two source spellings — both sites are the same pointer-offset
 *     expression in the original. Using CHOSEN_LANGUAGE at one site only
 *     breaks the two accesses' shared `%hi` (LENGTH MISMATCH, +4); at both
 *     sites it's LENGTH-correct but 130 bytes (badly wrong shape).
 *   - `u8 suffix` instead of `s32 suffix`, and a bare pointer-arithmetic
 *     `*(GOV_RESOURCE_PREFIX_PTRS + language_state[0x5e])` instead of
 *     `GOV_RESOURCE_PREFIX_PTRS[language_state[0x5e]]`: both compile
 *     byte-identical (nullcheck confirms the type change is real codegen,
 *     just not a helpful one; cc1 folds `a[i]` and `*(a+i)` identically).
 *   - Naming `lang_idx = language_state[0x5e];` separately from `prefix`:
 *     no additional effect beyond `chr_offset` alone.
 *
 * Residual 39 bytes, 5 clusters — all confirmed HARD-CONFLICT or previously
 * cost-verified, via regalloc.py/rtlguide on the CURRENT (39-byte) draft:
 *   - suffix lives in t0, target a3 (0x55e7c/0x55ea8, 1 byte each): rtlguide
 *     names this HARD-CONFLICT — pseudo p100 (suffix) conflicts with hard
 *     regs v0,v1,a0-a3 AND the long-lived s3-s6 pseudos simultaneously in
 *     the current decomposition (regalloc.py: `100 conflicts: 80 85 87 92 93
 *     94 100 2 3 4 5 6 7 16 17 29`) — no priority/respelling lever exists;
 *     needs a live-range or source-identity change neither `prefix` nor any
 *     tested variant produced.
 *   - the sprintf archive-path address materialization (0x55ec4-0x55ee8, 9
 *     insns/35 bytes): same HARD-CONFLICT family — a v0-vs-v1 base/index
 *     role swap (rtlguide: candidate v1->target v0 conflicts with p98/p315)
 *     plus suffix's register plus WHEN the resource_root->a2 move schedules
 *     (target moves it early, right after computing D_800137A0; ours delays
 *     it to the jal's delay slot). All three are entangled; fixing one
 *     without the others was not found.
 *   - a v0-vs-v1 stack reload at 0x55fd0 (unchanged from prior rounds): the
 *     volatile `gov_prompt.attribute` dead-read's register identity.
 *   - setup_brightness must stay -0x80 (unchanged from prior rounds): a
 *     plain 0x80 CSEs with full_brightness/shade and collapses the frame
 *     (98 -> 346 in an earlier measurement); the lone `li s0,-128` vs
 *     `li s0,128` at 0x55f64 is the unavoidable cost.
 *
 * NEW COOKBOOK RULE (report for §3.9 / autorules): a fresh, single-use local
 * assigned a constant ONCE and read ONCE at an otherwise-literal call
 * argument (`s32 clear_b = 0; ...; f(a, b, clear_b);` in place of a literal
 * `0`) can close an UNRELATED register-identity cluster elsewhere in the
 * SAME function — pure global-allocation-priority rebalancing from the
 * pseudo's mere existence, no textual/data connection to the site it fixes.
 * The lever requires a GENUINELY FRESH local: substituting an existing
 * same-typed local that is read again later (tested: `color`) costs a frame
 * slot (LENGTH MISMATCH) instead of helping, because its live range now
 * spans from the earlier site to its real later use. Re-verify
 * `matchdiff --clusters` (not just the total) after adopting a permuter
 * candidate of this shape — the win and the candidate's own edit site are
 * often in different clusters.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", start_demo_);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", start_demo___override__prt_80055ee4_68447b59);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_5);

/* jump-table pool @ 0x80013bb0 (5 words; tables at 0x80013bb0) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 start_demo__jtbl[5] = {
    0x800561A8, 0x800561F8, 0x80056438, 0x800564EC,
    0x80056550,
};

#else /* NON_MATCHING */

typedef struct BackGround BackGround;
typedef struct
{
    u8 pad[0x68];
    GsSPRITE sprite;
} Sprite3D;

typedef struct
{
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} RECT;

extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_LANGUAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 D_80010048;
extern u8 ITEM_LOADOUT_BACKUP[];
extern u8 SHOP_STOCK_STATE_BY_CHAR[];
extern char D_80013AFC[];
extern char D_80013B24[];
extern char D_800137A0[];
extern char *GOV_RESOURCE_PREFIX_PTRS[];
extern char *GOV_ARCHIVE_PTRS[];
extern s32 GameClock;
extern s16 SkipFrame;
extern GsOT *OTablePt;

extern void SetupAppearance(s16 mode, s16 stage);
extern void PadShockAR(s32 port, s32 duration, s32 strength, s32 delay);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, s32 b);
extern void FUN_80038ce0(void);
extern void ClearImage(RECT *rect, u8 r, u8 g, u8 b);
extern s32 DrawSync(s32 mode);
extern s32 VSync(s32 mode);
extern u_long *FileRead(char *path);
extern Sprite3D *SetupSprite(Sprite3D *original, GsIMAGE *image);
extern int sprintf(char *buffer, char *format, ...);
extern u_long *get_tim_from_archive(u_long *archive, int index);
extern BackGround *FUN_8004f4f8(u_long *tim);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void LoadTIM(u_long *tim);
extern void _PlayMusic(s32 music, s32 mode);
extern void StartDrawing(void);
extern short DrawBG(BackGround *background);
extern s32 GetRealPad(s32 port);
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 priority);
extern void FUN_80056910(Sprite3D *sprite, s16 shade);
extern void vfree(void *ptr);
extern void DisposeBG(BackGround *background);
extern void FUN_8004f6c0(s32 mode);
extern void EndDrawing(s16 mode);

static inline void StartDemoInitSprite(u_long *tim, GsIMAGE *image,
                                       GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

void start_demo_(void)
{
    GsIMAGE fade_image;
    GsSPRITE gov_title;
    GsSPRITE archive_line_1;
    GsSPRITE archive_line_2;
    GsSPRITE archive_line_3;
    GsSPRITE gov_prompt;
    RECT clear_rect;
    char archive_path[64];
    GsIMAGE image;
    s16 old_pad;
    BackGround *background;
    u_long *gov_archive;
    u_long *tim;
    u_long *fade_archive;
    Sprite3D *fade_sprite;
    u8 *persistent;
    u8 *language_state;
    char *resource_root;
    u16 pad;
    u16 previous_pad;
    u16 new_press;
    s16 shade;
    s32 state;
    s32 title_brightness;
    s32 full_brightness;
    s32 setup_brightness;
    u32 color;
    s32 increment;
    s32 i;
    s32 suffix;
    s32 clear_b;
    char *prefix;
    s32 chr_offset;

    state = 1;
    shade = 0x80;
    title_brightness = 0;
    old_pad = 0;
    clear_b = 0;
    SetupAppearance(0, -1);
    PadShockAR(0, 0, 0, 0);

    i = 0;
    persistent = (u8 *)0x80010000;
    do {
        chr_offset = CHOSEN_CHARACTER * 0x20;
        persistent[0x27 + i] = persistent[(i + chr_offset) + 0x40c];
        i++;
    } while (i < 0x14);

    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    clear_rect.x = 0;
    clear_rect.y = 0;
    clear_rect.w = 0x400;
    clear_rect.h = 0x200;
    ClearImage(&clear_rect, 0, 0, clear_b);
    DrawSync(0);

    tim = FileRead(D_80013AFC);
    GetTIMInfo(tim, &fade_image);
    LoadTIMAndFree(tim);
    fade_sprite = SetupSprite(0, &fade_image);
    suffix = 'r';
    fade_sprite->sprite.attribute |= 0x60000000;

    language_state = (u8 *)0x80010000;
    if (CHOSEN_CHARACTER != 0)
    {
        suffix = 'a';
    }
    resource_root = D_800137A0;
    prefix = GOV_RESOURCE_PREFIX_PTRS[language_state[0x5e]];
    sprintf(archive_path, D_80013B24, resource_root, prefix, suffix);
    /* Keep the loop brightness in its own pre-resource CSE region. */
    do {
    } while (0);
    full_brightness = 0x80;
    fade_archive = FileRead(archive_path);
    tim = get_tim_from_archive(fade_archive, 0);
    background = FUN_8004f4f8(tim);
    gov_archive = PathFileRead(resource_root,
                               GOV_ARCHIVE_PTRS[language_state[0x5e]]);
    /* +128 held distinct from full_brightness/shade; a plain 0x80 here would
     * CSE with them and collapse the frame, so keep it as -0x80 (narrows to
     * 0x80 in the u8 sprite fields). The lone li s0,-128 vs li s0,128 is the
     * residual cost. */
    setup_brightness = -0x80;

    tim = get_tim_from_archive(gov_archive, 0);
    StartDemoInitSprite(tim, &image, &gov_title);
    gov_title.y = -0x28;
    gov_title.x = 0;
    gov_title.r = setup_brightness;
    gov_title.g = setup_brightness;
    gov_title.b = setup_brightness;
    gov_title.attribute |= 0x50000000;
    gov_title.mx = gov_title.w >> 1;
    gov_title.my = gov_title.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(gov_archive, 1);
    StartDemoInitSprite(tim, &image, &gov_prompt);
    /* Preserve the retail binary's otherwise-dead attribute read. */
    (void)*(volatile u32 *)&gov_prompt.attribute;
    gov_prompt.y = 0x5f;
    gov_prompt.x = 0;
    gov_prompt.r = setup_brightness;
    gov_prompt.g = setup_brightness;
    gov_prompt.b = setup_brightness;
    gov_prompt.mx = gov_prompt.w >> 1;
    gov_prompt.my = gov_prompt.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 1);
    StartDemoInitSprite(tim, &image, &archive_line_1);
    archive_line_1.x = 0;
    archive_line_1.y = 0;
    archive_line_1.r = 0;
    archive_line_1.g = 0;
    archive_line_1.b = 0;
    archive_line_1.attribute |= 0x50000000;
    archive_line_1.mx = archive_line_1.w >> 1;
    archive_line_1.my = archive_line_1.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 2);
    StartDemoInitSprite(tim, &image, &archive_line_2);
    archive_line_2.y = 0x14;
    archive_line_2.x = 0;
    archive_line_2.r = 0;
    archive_line_2.g = 0;
    archive_line_2.b = 0;
    archive_line_2.attribute |= 0x50000000;
    archive_line_2.mx = archive_line_2.w >> 1;
    archive_line_2.my = archive_line_2.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 3);
    StartDemoInitSprite(tim, &image, &archive_line_3);
    archive_line_3.y = 0x28;
    archive_line_3.x = 0;
    archive_line_3.r = 0;
    archive_line_3.g = 0;
    archive_line_3.b = 0;
    archive_line_3.attribute |= 0x50000000;
    archive_line_3.mx = archive_line_3.w >> 1;
    archive_line_3.my = archive_line_3.h >> 1;
    LoadTIM(tim);

    DrawSync(0);
    VSync(0);
    _PlayMusic(0xb, 0);

    while (1)
    {
        StartDrawing();
        DrawBG(background);

        switch (state)
        {
        case 1:
            do {
                do {
                    do {
                        do {
                            shade -= 2;
                        } while (0);
                    } while (0);
                } while (0);
            } while (0);
            if (shade <= 0)
            {
                do {
                    do {
                        do {
                            do {
                                state = 2;
                            } while (0);
                        } while (0);
                    } while (0);
                } while (0);
                shade = 0;
                clear_rect.x = 0x280;
                clear_rect.y = 0x168;
                clear_rect.w = 0x100;
                GameClock = 0;
                clear_rect.h = 0x28;
            }
            FUN_80056910(fade_sprite, shade);
            break;

        case 2:
            previous_pad = old_pad;
            do {
                pad = GetRealPad(0);
            } while (0);
            old_pad = pad;
            new_press = pad & (pad ^ previous_pad);
            if ((new_press & 0x20) != 0 && GameClock < 0x23b)
            {
                state = 3;
                gov_title.r = gov_title.g = gov_title.b = full_brightness;
                archive_line_1.r = archive_line_1.g = archive_line_1.b =
                    full_brightness;
                archive_line_2.r = archive_line_2.g = archive_line_2.b =
                    full_brightness;
                archive_line_3.r = archive_line_3.g = archive_line_3.b =
                    full_brightness;
                GsSortSprite(&gov_title, OTablePt, 0xa);
                GsSortSprite(&archive_line_1, OTablePt, 0x50);
                GsSortSprite(&archive_line_2, OTablePt, 0x50);
                GsSortSprite(&archive_line_3, OTablePt, 0x50);
                GsSortSprite(&gov_prompt, OTablePt, 0x50);
                break;
            }

            if (GameClock >= 0x4c)
            {
                title_brightness++;
                if (title_brightness >= 0x80)
                {
                    title_brightness = 0x80;
                }
                gov_title.r = gov_title.g = gov_title.b = title_brightness;
                GsSortSprite(&gov_title, OTablePt, 0xa);
            }
            if (GameClock >= 0x119)
            {
                increment = archive_line_1.b + 1;
                color = -0x80;
                if (increment < 0x80)
                {
                    color = increment;
                }
                archive_line_1.r = archive_line_1.g = archive_line_1.b = color;
                GsSortSprite(&archive_line_1, OTablePt, 0x50);
            }
            if (GameClock >= 0x15f)
            {
                increment = archive_line_2.b + 1;
                color = -0x80;
                if (increment < 0x80)
                {
                    color = increment;
                }
                archive_line_2.r = archive_line_2.g = archive_line_2.b = color;
                GsSortSprite(&archive_line_2, OTablePt, 0x50);
            }
            if (GameClock >= 0x1a5)
            {
                increment = archive_line_3.b + 1;
                color = -0x80;
                if (increment < 0x80)
                {
                    color = increment;
                }
                archive_line_3.r = archive_line_3.g = archive_line_3.b = color;
                GsSortSprite(&archive_line_3, OTablePt, 0x50);
            }
            if (GameClock < 0x23b)
            {
                break;
            }
            goto sort_prompt_and_handle_input;

        case 3:
            previous_pad = old_pad;
            pad = GetRealPad(0);
            old_pad = pad;
            new_press = pad & (pad ^ previous_pad);
            GsSortSprite(&gov_title, OTablePt, 0xa);
            GsSortSprite(&archive_line_1, OTablePt, 0x50);
            GsSortSprite(&archive_line_2, OTablePt, 0x50);
            GsSortSprite(&archive_line_3, OTablePt, 0x50);
        sort_prompt_and_handle_input:
            GsSortSprite(&gov_prompt, OTablePt, 0x50);
            if ((new_press & 0x20) != 0)
            {
                state = 4;
            }
            if ((new_press & 0x800) != 0 || GameClock >= 0xa8c)
            {
                state = 5;
            }
            break;

        case 4:
            shade += 4;
            if (shade >= 0x80)
            {
                D_80010048 |= 1;
                vfree(fade_archive);
                vfree(gov_archive);
                vfree(fade_sprite);
                DisposeBG(background);
                FUN_8004f6c0(0x11);
            }
            FUN_80056910(fade_sprite, shade);
            break;

        case 5:
            shade += 4;
            if (shade >= 0x80)
            {
                D_80010048 &= 0xfe;
                vfree(fade_archive);
                vfree(gov_archive);
                vfree(fade_sprite);
                DisposeBG(background);
                STAGE_LAYOUT_NUMBER = 0xff;
                FUN_8004f6c0(0x10);
            }
            FUN_80056910(fade_sprite, shade);
            break;
        }

        SkipFrame = 2;
        EndDrawing(0);
    }
}

#endif /* NON_MATCHING */
