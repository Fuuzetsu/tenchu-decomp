#include "common.h"
#include "main.exe.h"

/*
 * Pre-mission briefing / item selection screen (0x80052084, 0xE24 bytes).
 *
 * CURRENT(28): 905/905 instructions, 28 differing lines in 15 blocks; all
 * remaining diffs are local register-pairing ties (asmdiff.py view):
 *   1. entry clamp {v1,a0}: T ties the stock-address into n's dying reg
 *      (addu v1,t0,v1) and puts maxStock in a0; ours takes fresh a0 for the
 *      address (the case-1 clamp copy, which uses the fresh-reg shape, is
 *      byte-identical in both) (7 lines).
 *   2. (s16)pad argument of check_for_known_button_combination: ours emits
 *      the sll/sra ext BEFORE the np xor/and chain with a0 as intermediate;
 *      T emits it after, reusing v0 (the dying pad copy) (4 lines).
 *   3. case-0x1F store block {v0,v1}: T loads chr into v1/li 255 into v0,
 *      ours inverted. The fresh chr reload itself is right (join block cuts
 *      cse); only the local qty pairing differs (7 lines).
 *   4. cursor-move exts: T interleaves [sll dx][sll dy][move k=cursor]
 *      [sra dx][bnez][slot: sra dy]; ours keeps each (s16)->int ext pair
 *      together and reorg fills the slot with the k move instead (5 lines).
 *   5. bounce-arm compare: T's (s16)t ext reads the addiu temp (sll v0,v0);
 *      ours reads the coalesced scale (sll v0,s7) -- some equivalence makes
 *      cc1 canonicalize (s16)t to scale's reg; reusing early-born ints
 *      (uid/i) as t shifted other regions, so the temp identity is not it
 *      (3 lines).
 *   6. digit-block entry loads: both lhu's are reload-materialized zexts of
 *      the int t1/t2 temps, but ours orders [160][152] with t2 in v0; T
 *      orders [152][160] with t2 in t5. Source statement order of t1/t2
 *      does NOT flip it (canonicalized somewhere post-expand) (3+2 lines).
 *
 * History: 94 -> 28 this session. The wins, in order:
 *   - digit loop's u0 merged into grid x (multi-def host keeps T's
 *     andi s0,_,0xff alive: an `int c = (u8)<multi-def-var>` zext with two
 *     uses across the call must stay one andi + sb of c's home s0);
 *   - shown-items loop's (s16)j ext runs through grid y (y = (s16)j), so
 *     the y-family (grid conflict with x) lands with jext, freeing s1;
 *   - digit-entry [addiu sp,24][lhu cap][lhu taken][li 34][sh x] shape:
 *     `int t1 = cap, t2 = taken;` read-temps make the loads real zext RTL
 *     insns positioned at their statements (u16 temps get combine-merged
 *     back into the subu; int temps' zero_extends have no subu pattern and
 *     survive) -- this also killed the assembler hazard nop (905 exactly);
 *   - inline index sums `[idx + (ps->chr << 5)]` (shift spelling!): inside
 *     an address expression EXPAND_SUM expands a MULT-BY-CONSTANT first
 *     regardless of source operand order (addu chr32,idx); << 5 skips the
 *     mult special-case and preserves source order (addu idx,chr32). Value
 *     temps (`int n = j + ps->chr * 0x20;`) don't need this (7 sites);
 *   - grid loop must be a real `for` (not goto/do-while): the c==0xFE skip
 *     branch targets the loop bottom = jump.c's NOTE_INSN_LOOP_VTOP, so
 *     reorg's mostly_true_jump predicts it taken and duplicates the j++
 *     addiu into the delay slot (goto-loop: fallthrough fill, one insn
 *     short; invariants stay unhoisted either way because the loop body
 *     has calls);
 *   - do{}while(0) around the cursor-move block (permuter find, round 1);
 *   - digit loop sum written `dsp->u = dsp->u + (s16)rem * dsp->w;` and neg
 *     block `int c = (u8)dsp->u;` -- cse folds the re-reads back to x's
 *     register (same bytes) but the shifted pseudo bookkeeping flips the
 *     global s1/s2 seed: {x+u0, shown+dy}=s1, {y+jext, dsp}=s2 like the
 *     target (permuter find, round 2; the whole ~40-line s1/s2 mirror
 *     traces to which callee-saved reg the first cursor mult temp lands in).
 * Permuter work dirs: scratch/permuter-briefing (r1, base 94-state),
 * scratch/permuter-b2 (r2, base 65-state), scratch/permuter-b3 (r3, base
 * 28-state). tools/permute.py's stock target.o is WRONG for this function
 * (splat splits the switch cases into 8 .s files; concatenate them in
 * address order -- see target.s in the scratch permuter dirs).
 */

/* The persistent state is accessed three ways in the original, on purpose:
 *  - through short-lived pointer locals (q/ps/r below) -> reg+disp addressing;
 *  - through PSTATE casts in the two entry loops -> one hoisted 0x80010000;
 *  - through plain extern globals -> assembler one-line macro (lui+op pairs).
 * Array-indexing spelling picks the addu operand order: `p->arr[i]` puts the
 * base first, `(&p->arr[0])[i]` puts the index first, the extern-symbol form
 * puts the (hoisted) %hi base first.
 */
#define PSTATE ((PersistentState *)0x80010000)

typedef struct
{
    s16 x;        /* 0x0 grid position */
    s16 y;        /* 0x2 */
    s32 itemIndex; /* 0x4 */
    u8 maxStock;  /* 0x8 */
    u8 pad[3];    /* 0x9 */
} ShopItemDefault;

typedef struct
{
    u8 uid;       /* 0x0 */
    u8 pad[0x1B]; /* 0x1 */
} StageConfigEntry;

typedef struct { char *p[4]; } HelpPathBlock;  /* 0x10 */

extern u8 CHOSEN_CHARACTER;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 D_80010019;
extern u8 D_8001001A;
extern u8 D_80010048;
extern u8 SHOP_STOCK_STATE_BY_CHAR[];
extern ShopItemDefault SHOP_ITEM_DEFAULTS[];
extern StageConfigEntry StageConfig[];
extern char *ITEM_SEL_SPRITE_PTRS[];
extern char NUMBER_TIM_PATH[];
extern char *ITEM_HELP_TIM_PATHS[];
extern s16 CARRY_30_ITEMS_CHEAT_APPLIED; /* gp-relative (TU-local .sdata) */
extern u16 SkipFrame;
extern GsOT *OTablePt;

extern int rand(void);
extern u_long *FileRead(char *path);
extern void vfree(void *p);
extern void *FUN_8004f4f8(u_long *tim);
extern void InitSprite(GsIMAGE *im, GsSPRITE *sp);
extern void LoadTIM(u_long *tim);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, u8 b);
extern void FUN_80038ce0(void);
extern void FUN_8004f6c0(int arg);
extern void SoundEx(VECTOR *loc, int id);
extern void StartDrawing(void);
extern void DrawBG(void *bg);
extern void EndDrawing(int arg);
extern void PutNumber(int x, int y, int n);
extern void PutItemIcon(int id, int x, int y, int scale);
extern void PutItemCursor(int x, int y, int size, int rotdif);
extern void DisposeBG(void *bg);
extern int check_for_known_button_combination(s16 pad, s16 newpress);
extern u_long *get_tim_from_archive(u_long *archive, int idx);
extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, int pri);
extern buttons_held GetRealPad(s32 which);
extern void FUN_800519bc(void);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", BriefingAndInventorySelectionScreen);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", switchD_80052360__switchD);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", switchD_80052360__caseD_0);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", switchD_80052360__caseD_1);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", switchD_80052360__caseD_3);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", switchD_80052360__caseD_1f);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", switchD_80052360__caseD_7);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BriefingAndInventorySelectionScreen", switchD_80052360__caseD_2);

/*
 * Stub-state jump table. The splat yaml routes vram 0x80013B30 through this
 * TU's .rodata ([0x3330, .rodata, ...]) so the ACTIVE function's compiled
 * switch table lands at the original address. In the stub state the
 * INCLUDE_ASM pieces emit no .rodata, so provide the original 0x20-entry
 * table verbatim (its absence shifts the whole image by 0x80 bytes).
 * DELETE this array when activating the #if 0 draft below.
 */
static const u32 switchD_80052360_jtbl[32] = {
    0x80052368, 0x8005238C, 0x8005260C, 0x80052490,
    0x8005260C, 0x8005260C, 0x8005260C, 0x8005257C,
    0x8005260C, 0x8005260C, 0x8005260C, 0x8005260C,
    0x8005260C, 0x8005260C, 0x8005260C, 0x8005260C,
    0x8005260C, 0x8005260C, 0x8005260C, 0x8005260C,
    0x8005260C, 0x8005260C, 0x8005260C, 0x8005260C,
    0x8005260C, 0x8005260C, 0x8005260C, 0x8005260C,
    0x8005260C, 0x8005260C, 0x8005260C, 0x800524E4,
};

#if 0
/*
 * The two TIM-sprite setup blocks are inlined static helpers (same mechanism
 * as DoInfoViewProc's menus): the GsIMAGE scratch is the helper's own local,
 * so its address expands from the inlined frame base (bare register -- every
 * call gets a direct addiu into the arg register instead of a CSE'd
 * callee-saved pseudo), and the two inline expansions reuse one freed temp
 * slot after the caller's locals (spr @ +0, hspr @ +0x28, tim @ +0x50).
 * NOTE: keep the helpers inside the guard -- in the stub state cc1 emits
 * unreferenced static inlines as standalone code (+32 insns).
 */
static inline u_long *LoadHelpArchive(PersistentState *q)
{
    char *paths[4];

    *(HelpPathBlock *)paths = *(HelpPathBlock *)ITEM_HELP_TIM_PATHS;
    return FileRead(paths[q->language]);
}

static inline void TimToSprite(u_long *buf, GsSPRITE *sp)
{
    GsIMAGE tim;

    GetTIMInfo(buf, &tim);
    InitSprite(&tim, sp);
}

void BriefingAndInventorySelectionScreen(void)
{
    GsSPRITE spr;
    GsSPRITE hspr;
    s16 bounce;
    union { u16 u; s16 s; } pad;
    u16 cap;
    u16 taken;
    void *bg;
    u_long *harc;
    GsSPRITE *p;
    int help;
    PersistentState *q;
    PersistentState *ps;
    GsSPRITE *dsp;
    u_long *buf;
    int cursor;
    int scale;
    int nsel;
    s16 np;
    int i;        /* entry backup loop */
    s16 j;        /* counts loop, case1/3 loops, grid, shown, cursor dx, epilogue */
    s32 x;        /* grid x, cursor dy */
    s32 y;        /* grid y */
    s16 j7;       /* case 7 loop */
    int ci;       /* clamp loops */
    int si;       /* cursor search */
    s16 shown;
    s16 av;
    int t;
    int uid;
    int id;

    pad.s = -1;
    cap = 0xF;
    cursor = 0;
    taken = 0;
    help = -1;

    for (i = 0; i < 0x14; i++) {
        PSTATE->backup[i] = PSTATE->stock[i + CHOSEN_CHARACTER * 0x20];
    }
    for (j = 0; j < 0x14; j++) {
        PSTATE->counts[j] = 0;
    }
    q = (PersistentState *)0x80010000;
    uid = StageConfig[q->stage].uid;
    q->counts[0] = 0xFF;
    if (uid == 0) {
        q->counts[1] = 5;
        return;
    }
    if ((q->flags48 & 1) == 0) {
        FUN_800519bc();
    }
    bounce = 0;
    scale = 0x1000;
    buf = FileRead(ITEM_SEL_SPRITE_PTRS[q->language]);
    bg = FUN_8004f4f8(buf);
    vfree(buf);
    buf = FileRead(NUMBER_TIM_PATH);
    p = &spr;
    TimToSprite(buf, p);
    spr.attribute |= 0x50000000;
    p->x = -0xA0;
    p->y = -0x78;
    p->r = 0x80;
    p->g = 0x80;
    p->b = 0x80;
    p->mx = p->w >> 1;
    p->my = p->h >> 1;
    spr.mx = 0;
    spr.my = 0;
    LoadTIMAndFree(buf);
    spr.w = 0xC;

    nsel = 1;
    harc = LoadHelpArchive(q);

    {
        PersistentState *cq = (PersistentState *)0x80010000;
        for (ci = 0; ci < 0x13; ci++) {
            int n = SHOP_ITEM_DEFAULTS[ci].itemIndex + CHOSEN_CHARACTER * 0x20;
            u8 c = cq->stock[n];
            s32 mx = SHOP_ITEM_DEFAULTS[ci].maxStock;
            if (c != 0xFE && mx < c) {
                cq->stock[n] = mx;
            }
        }
    }

    ps = (PersistentState *)0x80010000;
    do {
        rand();
        np = pad.u;
        pad.u = GetRealPad(0);
        np = pad.u & (pad.u ^ np);
        id = check_for_known_button_combination(pad.s, np);
        switch ((s16)(id - 1)) {
        case 0:
            if (CARRY_30_ITEMS_CHEAT_APPLIED == 0) {
                CARRY_30_ITEMS_CHEAT_APPLIED = 1;
                cap = 0x1E;
            }
            break;
        case 1:
            for (j = 1; j < 9; j++) {
                int n = j + ps->chr * 0x20;
                if ((&ps->stock[0])[n] == 0xFE) {
                    (&ps->stock[0])[n] = 1;
                } else {
                    (&ps->stock[0])[n] = (&ps->stock[0])[n] + 1;
                }
            }
            for (j = 9; j < 0x14; j++) {
                int n = j + ps->chr * 0x20;
                if ((&ps->stock[0])[n] != 0xFE) {
                    (&ps->stock[0])[n] = (&ps->stock[0])[n] + 1;
                }
            }
            {
                PersistentState *cq = (PersistentState *)0x80010000;
                for (ci = 0; ci < 0x13; ci++) {
                    int n = SHOP_ITEM_DEFAULTS[ci].itemIndex + CHOSEN_CHARACTER * 0x20;
                    u8 c = cq->stock[n];
                    s32 mx = SHOP_ITEM_DEFAULTS[ci].maxStock;
                    if (c != 0xFE && mx < c) {
                        cq->stock[n] = mx;
                    }
                }
            }
            break;
        case 3:
            for (j = 9; j < 0x14; j++) {
                int n = j + ps->chr * 0x20;
                if ((&ps->stock[0])[n] == 0xFE) {
                    (&ps->stock[0])[n] = 1;
                }
            }
            break;
        case 0x1F:
            if (ps->chr != 0) {
                u8 already = ps->counts[0x13];
                if (already != 0 || (&ps->stock[0x13])[ps->chr * 0x20] == 1) {
                    do {
                        if ((s16)nsel < 6) {
                            if (already == 0) {
                                nsel++;
                                taken++;
                            }
                            ps->counts[0x13] = 0xFF;
                            (&ps->stock[0x13])[ps->chr * 0x20] = 0;
                            SoundEx((VECTOR *)0x0, 8);
                        }
                    } while (0);
                }
            }
            break;
        case 7:
            for (j7 = 0; j7 < 0x14; j7++) {
                (&ps->stock[0])[(int)j7 + (CHOSEN_CHARACTER << 5)] = (&ps->backup[0])[j7];
            }
            FadeOutDirect(0x20, 2, 8, 8, 8);
            FUN_80038ce0();
            STAGE_LAYOUT_NUMBER = 0xFF;
            D_80010048 = D_80010048 & 0xFE;
            FUN_8004f6c0(0x10);
            break;
        }
        if (np == 0x800) {
            goto quit;
        }
        StartDrawing();
        DrawBG(bg);
        for (j = 0; j < 0x13; j++) {
            int n = SHOP_ITEM_DEFAULTS[j].itemIndex;
            u8 c = (&ps->stock[0])[n + (ps->chr << 5)];
            if (c != 0xFE) {
                x = SHOP_ITEM_DEFAULTS[j].x;
                y = SHOP_ITEM_DEFAULTS[j].y;
                if (c != 0xFF) {
                    PutNumber(x + 0x1A, y + 8, c);
                }
                PutItemIcon(n, x, y, 0x1000);
            }
        }

        PutItemCursor(SHOP_ITEM_DEFAULTS[cursor].x, SHOP_ITEM_DEFAULTS[cursor].y, 0x1000, -0x3000);
        {
            int ddx, ddy;
            int k;

            do {
            shown = 0x10;
            if ((np & 0x4000) == 0) {
                shown = 0;
                if ((np & 0x1000) != 0) {
                    shown = -0x10;
                }
            }
            j = 0x10;
            if ((np & 0x2000) == 0) {
                j = 0;
                if ((np & 0x8000) != 0) {
                    j = -0x10;
                }
            }
            ddx = j;
            ddy = shown;
            k = cursor;
            if (ddx != 0 || ddy != 0) {
                int best = 0x7FFFFFFF;
                int bi = cursor;
                int tx = SHOP_ITEM_DEFAULTS[bi].x + ddx;
                int ty = SHOP_ITEM_DEFAULTS[bi].y + ddy;
                for (si = 0; si < 0x13; si++) {
                    int ex = SHOP_ITEM_DEFAULTS[si].x - tx;
                    int ey = SHOP_ITEM_DEFAULTS[si].y - ty;
                    int d = ex * ex + ey * ey;
                    if (d < best && 0 <= ex * ddx && 0 <= ey * ddy && si != k) {
                        best = d;
                        bi = si;
                    }
                }
                cursor = bi;
            }
            } while (0);
        }
        if ((np & 0xF000) != 0) {
            SoundEx((VECTOR *)0x0, 0xB);
            help = -1;
        }
        if (np != 0) {
            if ((s16)pad.u == 0x20) {
                np = 0;
                bounce = 1;
                {
                    s16 idx = SHOP_ITEM_DEFAULTS[cursor].itemIndex;
                    scale = 0x200;
                    if ((&ps->stock[0])[idx + (ps->chr << 5)] != 0) {
                        if ((&ps->stock[0])[idx + (ps->chr << 5)] != 0xFE) {
                            if ((s16)taken < cap) {
                                u8 cnt = (&ps->counts[0])[idx];
                                if (cnt == 0) {
                                    nsel++;
                                }
                                if ((s16)nsel < 6) {
                                    if (idx != 0x13 || D_8001001A == 0) {
                                        (&ps->counts[0])[idx] = cnt + 1;
                                        taken++;
                                        (&ps->stock[0])[idx + (ps->chr << 5)]--;
                                    }
                                    SoundEx((VECTOR *)0x0, 0xD);
                                } else {
                                    SoundEx((VECTOR *)0x0, 0xC);
                                    help = 0x14;
                                    nsel--;
                                }
                            } else {
                                SoundEx((VECTOR *)0x0, 0xC);
                                help = 0x13;
                            }
                        }
                    }
                }
            }
            if (np != 0 && (s16)pad.u == 0x40) {
                s16 idx = SHOP_ITEM_DEFAULTS[cursor].itemIndex;
                bounce = 2;
                {
                    u8 c = (&ps->counts[0])[idx];
                    scale = 0x1400;
                    if (c != 0) {
                        if (c == 0xFF) {
                            (&ps->counts[0])[idx] = 0;
                            (&ps->stock[0])[idx + (ps->chr << 5)] = 1;
                            nsel--;
                        } else {
                            (&ps->counts[0])[idx] = c - 1;
                            (&ps->stock[0])[idx + (ps->chr << 5)]++;
                            if ((&ps->counts[0])[idx] == 0) {
                                nsel--;
                            }
                        }
                        taken--;
                        SoundEx((VECTOR *)0x0, 0x1F);
                    }
                }
                help = -1;
            }
        }
        if ((s16)scale < 0x1000) {
            scale += 0xC0;
        }
        if (help == -1) {
            if ((&ps->stock[0])[SHOP_ITEM_DEFAULTS[cursor].itemIndex + (ps->chr << 5)] != 0xFE) {
                help = SHOP_ITEM_DEFAULTS[cursor].itemIndex - 1;
            }
        }
        if (help != -1) {
            buf = get_tim_from_archive(harc, help);
            TimToSprite(buf, &hspr);
            hspr.x = -0xA0;
            hspr.y = -0x78;
            hspr.r = 0x80;
            hspr.g = 0x80;
            hspr.b = 0x80;
            hspr.attribute |= 0x50000000;
            hspr.mx = hspr.w >> 1;
            hspr.my = hspr.h >> 1;
            hspr.mx = 0;
            hspr.my = 0;
            LoadTIM(buf);
            hspr.x = -0x92;
            hspr.y = 0x23;
            GsSortSprite(&hspr, OTablePt, 1);
        }
        if (bounce == 1) {
            t = scale + 0x10;
            scale = t;
            if (0x1400 < (s16)t) {
                bounce ^= 1;
            }
        } else if (bounce == 0) {
            t = scale - 0x10;
            scale = t;
            if ((s16)t < 0x1000) {
                bounce ^= 1;
            }
        } else if (bounce == 2) {
            t = scale - 0x10;
            scale = t;
            if ((s16)t < 0x1000) {
                bounce = 0;
            }
        }
        shown = 0;
        for (j = shown; j < 0x14; j++) {
            u8 c;
            y = (s16)j;
            c = (&ps->counts[0])[y];
            if (c != 0) {
                if (c != 0xFF) {
                    PutNumber(0xA6 - shown * 0x19, 0x62, c);
                }
                PutItemIcon(y, (s16)(0x8C - shown * 0x19), 0x5A, 0x1000);
                shown++;
            }
        }
        {
            int neg;
            int m;
            int tv;
            int rem;
            int quo;
            int d;
            int t1;
            int t2;

            dsp = &spr;
            t1 = cap;
            t2 = taken;
            dsp->x = 0x22;
            dsp->y = -0x32;
            av = t1 - t2;
            tv = (s16)av;
            if (tv < 0) {
                av = -tv;
                neg = 1;
            } else {
                neg = 0;
            }
            do {
                d = av;
                quo = d / 10;
                x = dsp->u;
                rem = d - quo * 10;
                dsp->u = dsp->u + (s16)rem * dsp->w;
                GsSortSprite(dsp, OTablePt, 0);
                dsp->u = x;
                dsp->x -= 0xC;
                av = quo;
            } while ((s16)av != 0);
            if (neg) {
                int c = (u8)dsp->u;
                m = 10;
                dsp->u = c + dsp->w * m;
                GsSortSprite(dsp, OTablePt, 0);
                dsp->u = c;
            }
        }
        SkipFrame = 2;
        EndDrawing(0);
    } while (1);

quit:
    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    if (PSTATE->counts[0x12] != 0) {
        PSTATE->counts[0x12] = 0xFF;
    }
    for (j = 0; j < 9; j++) {
        int n = j + PSTATE->chr * 0x20;
        if (PSTATE->stock[n] == 0) {
            PSTATE->stock[n] = 0xFE;
        }
    }
    vfree(harc);
    DisposeBG(bg);
}
#endif
