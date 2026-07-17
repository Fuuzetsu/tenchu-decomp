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
 * Residual 39 bytes, 5 clusters. A full escalation round (sched/greg/lreg
 * dumps read insn-by-insn, gcc sources read at line level, 10 measured
 * respellings, one bounded permuter run: 12,492 iterations, best candidate
 * rescored 40 > base 39) reduced ALL FIVE to two proven mechanisms:
 *
 * A. Clusters 0/1/2 (0x55e7c, 0x55ea8, 0x55ec4-0x55ee8; 37 bytes) are ONE
 *    allocation story: retail has suffix in a3, ours t0. mips.h defines no
 *    REG_ALLOC_ORDER, so find_reg probes $2,$3,$4... numerically: suffix
 *    lands on a3 ONLY if v0,v1,a0,a1,a2 ALL conflict. a2's sole birth is the
 *    arg3 load-loop move `a2 <- resource_root` — so that move must sit ABOVE
 *    suffix's death (`sw suffix,16(sp)`) in POST-SCHED1 order, and the
 *    `lw a3` (prefix) must sit BELOW it. No C spelling reaches that state:
 *      - named `prefix` (this draft): expand emits the lw BEFORE the sw;
 *        sched1 cannot swap them back (sp+16 vs pseudo-pointer: 2.8.1 sched.c
 *        memrefs_conflict_p cannot see through a pseudo — no REG_BASE_VALUE
 *        tracking — so emission order is a bidirectional pin). p97 (prefix,
 *        denser allocno) takes a3 first anyway; suffix -> t0.
 *      - inline `GOV_RESOURCE_PREFIX_PTRS[language_state[0x5e]]` as the 4th
 *        arg (MEASURED 42): expand_call evaluates reg-parm ADDRESSES in its
 *        precompute loop but leaves a MEM lazy, so the lw IS emitted after
 *        the sw — the right half. But the a2 move is then the LAST-created
 *        member of the after-call ready group {a0,a1lo,a2} (all priority 1,
 *        all alu => potential_hazard equal, sched.c:1346 is function-unit
 *        blockage only), and gcc 2.8.1 sched.c schedules BACKWARD: the
 *        call-adjacent slot goes to the highest-LUID member = the a2 move.
 *        It lands BELOW the sw; a2 never conflicts; suffix -> a2 (li a2
 *        measured at 0x55e7c). The lw is launch-blocked from the adjacent
 *        slot (load next to a jal that uses the loaded reg), so it cannot
 *        take the slot instead; li-s4 (full_brightness) cannot either: with
 *        the fence it is bump-poisoned (the fence-flush absorber's pick
 *        gives it the persistent 0x7f000001 birthing bump one cycle early,
 *        placing it just BELOW the call), and without any fence the
 *        while(1) loop's LOOP_BEG flush hands the LAST pre-loop call an
 *        ANTI dep on it (sched.c:2277 loop_notes path), it drifts to the
 *        block end, and the whole setup block reallocates (679 bytes,
 *        fade_sprite s7->s6). Also measured: `root_arg = resource_root`
 *        copy (combine folds it, byte-identical); fence moved after
 *        full_brightness (bump fires at T-160, byte-identical 42);
 *        `lang_scaled = language_state[0x5e]*4` (folded, 39 unchanged).
 *    To refute: find a spelling whose post-sched1 stream orders
 *    [a2 birth] < [sw suffix,16(sp)] < [lw a3] — dump with
 *    `tools/rtldump.py start_demo_ --pass sched` and read the last ~15
 *    ready-list lines of the block containing the sprintf call.
 * B. Cluster 4 (0x55fd0, 1 byte): the volatile gov_prompt.attribute
 *    dead-read's destination is a ZERO-LENGTH local-alloc quantity; an empty
 *    interval overlaps nothing, so it always receives the first register in
 *    order (v0). Retail's v1 implies the original value LIVED into the
 *    following quantities' ranges — i.e. the loaded value had a real use.
 *    C cannot fake one: a named probe var, probe-recycled-as-h-temp, and a
 *    self-assign keepalive all compile byte-identically (cc1 interposes a
 *    temp for the volatile load and deletes the dead user-var def).
 * C. Cluster 3 (0x55f64, 1 byte): setup_brightness = -0x80 stays; plain
 *    0x80 re-measured on THIS draft: 340 bytes (CSE collapse, replicating
 *    the older 98->346 result).
 *
 * The T-162 pick (A) is this function's single remaining lever: 37 of 39
 * bytes hang off it, and every fence arrangement trades it against the
 * downstream block (the fence notes also bound cse1's extended blocks —
 * removing them lets cse run 90..658 and drift the whole setup).
 *
 * COOKBOOK RULES already reported from earlier rounds: the fresh single-use
 * constant local (clear_b) global-priority rebalance, and fence-unwrap
 * around GetRealPad. NEW REUSABLE FACTS from this round (report upstream):
 *   1. gcc 2.8.1 sched.c schedules each block BACKWARD from its last insn;
 *      "ready" = all CONSUMERS placed; the printed ready-list order is the
 *      pick order (head first); within an equal-priority group the pick goes
 *      to greater potential_hazard (memory-unit ops only), then HIGHEST
 *      LUID; the 0x7f000001 "birthing bump" persists until picked and beats
 *      every plain priority including a call's.
 *   2. expand_call emission order is fixed: reg-parm VALUES precomputed
 *      (a MEM arg stays lazy — only its address insns are emitted), then
 *      store_one_arg for stack args, then reg moves in a0..a3 order. An
 *      INLINE memory-ref argument is the only way to emit its load AFTER
 *      the stack-arg stores; a named local can never reproduce that order.
 *   3. sched.c cannot disambiguate sp+K from a pseudo-held pointer (no
 *      REG_BASE_VALUE in 2.8.1), so store-vs-load EMISSION order between
 *      them is a permanent scheduling pin in both directions.
 *   4. A do{}while(0) fence's notes also delimit cse1/cse2 extended blocks
 *      (cse "Processing block" boundaries land exactly on them), so a fence
 *      can be load-bearing for VALUE PROPAGATION far downstream, not just
 *      for scheduling: deleting one here moved 679 bytes.
 *   5. The volatile-dead-read register class (B above): dest = zero-length
 *      quantity = first free register in order, unmovable from C.
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
