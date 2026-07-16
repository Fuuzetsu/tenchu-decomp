#include "common.h"
#include "main.exe.h"
#include "misc.h"

/*
 * Post-mission score/high-score screen (0x80054B48, 0x121C bytes).
 *
 * STATUS: NON_MATCHING -- 433 of 4636 bytes differ (was 437; 476; 498; 525; 1266).
 * Exact target length (1159 instructions), frame 0x1B8, 60/60 calls; the whole
 * function is address-aligned except the LoadTIMAndFree arg copy (9 low) and
 * one +4/-4 skew pair (medal bgez delay nop vs the row draw's sra).  No
 * retail-name record in PSX.SYM/demo export.
 *
 * 2026-07-17 round-4 (Fable) landed identities (do not undo):
 * - The ROW draw is its own macro (DRAW_SCORE_NUMBER_ROW32): s16 signedValue
 *   seeded from i+1 (HImode addiu a0,s4,1 — NO pre-extension of i), u16 value
 *   = signedValue (plain HImode move a1,a0 — no andi), draws through the
 *   pre-loop rowSprite pointer (no drawnSprite copy), and declares its OWN
 *   `negative` (target row uses s2 vs s3 elsewhere: per "one variable = one
 *   hard reg", the row's flag was a distinct local in the original).  This
 *   fixed value->a1 (incl. `move a1,s0` quotient reload), negu a1,a0, the
 *   j+two-arm sign shape, the y-store delay-slot fill, the backedge
 *   `addiu a0,s4,1` delay fill, and beqz s2 — the old rowValue->a0
 *   "hard-conflict p91" dissolved once value became the copy (born second)
 *   instead of the andi owner (born first).  476 -> 437.
 * - rowSprite = &number sits between i = 0 and rankSpriteBase = rankSprites
 *   (target order: move s4,zero / addiu sp,24 / addiu sp,96).  The old
 *   "separate rowNumber pointer spills rankSpriteBase" verdict was an
 *   artifact of the drawnSprite-copy shape; with _ROW32 there is no spill.
 *
 * Proven identities added this session (do not undo):
 * - insertedRank is a PLAIN s32 LOCAL, spilled by reload to sp+392: the
 *   li t0,-1/sw + lw t1/t2/t3 spill-reload sequence is reproduced exactly.
 *   tail shrank to {oldPad, pad, background}.
 * - Both rank-scan hit arms are `insertedRank = i; break;` INSIDE the loop:
 *   cc1 relocates loop-exiting arms to the function end (goNext arms first,
 *   then these, cross-jumped into ONE island `j join; sw a0,392(sp)`).
 *   An explicit source-level island (`goto`+label at the bottom) does NOT
 *   reproduce this (breaks the shared-jal cross-jump; wrong island order).
 * - The insert region uses a one-copy guard (`found = insertedRank`),
 *   block-scoped shift pointer, separate insert-block pointer (fresh lui),
 *   inline i-1 subscripts, and grades[i]=grades[i-1] BEFORE i--.
 * - rankColour = 128 alone at entry; the number-init brightness is
 *   `brightness = rankColour;` behind a do{}while(0) fence (the loop note
 *   stops cse1 from const-folding the copy AND stops sched1 hoisting; both
 *   effects are required to keep `move v0,s3` + the v0 anti-dependence that
 *   pins the lhu w/h loads after the r/g/b stores).
 * - Medal + row brightness are chained `r = g = b =` stores (reverse 22/21/20
 *   order) with brightness a plain (caller-saved) local; row uses if/else.
 * - numberSprite is TWO variables: a block-scoped init pointer and the
 *   loop-phase `numberSprite = &number` set before the main for(;;)
 *   (target re-materializes addiu s6,sp,24 in the preheader).
 * - number-init block has NO do{}while(0) fence before number.mx=0/my=0
 *   (the old "pivot reset boundary" fence was WRONG): removing it merges the
 *   pivot reset into the rgb block and lets the LoadTIMAndFree(tim) arg copy
 *   `move a0,s2` float up (target hoists it to the block top right after
 *   InitSprite).  This closed the ~80B block-rotation to a 9-insn shift and
 *   is the 525->498 win.  The a0=tim copy now stops just past the brightness
 *   fence (line ~c1c); it cannot reach the target's bf8 without deleting the
 *   brightness fence, and deleting THAT regresses (503) — so it is parked at
 *   the 9-insn residual.
 * - DRAW2/DRAW5 (the two draws after colons) pass numberSprite (move s2,s6
 *   class); other draws pass &number (fresh addiu).  The colon x-store is
 *   the OBJECT form `number.x = x_` (sp-direct, pinned at block top).
 * - The char loop keeps retail's dead attribute load via a site-local
 *   volatile read; its w>>1/h>>1 stores go through initSprite while the
 *   zero pivots recompute the bank address (SCORE_SPRITE_AT).
 * - The stock tail is direct indexing (displacement-folded 1036) — no
 *   unlock pointer; statePtr = (PersistentState *)0x80010000 behind a
 *   do{}while(0) fence supplies the s0 base for the layout=0xFF store
 *   (the lui fills the FUN_80038ce0 delay slot).
 *
 * REJECTED this session (do not repeat):
 * - Integer-sum (leSetEnemy) spellings for the rank/char initSprite address:
 *   cse then folds the pivot recomputation into the same register and the
 *   loops SHRINK.  The two coexisting shapes (main index-first, pivot
 *   base-first) remain unsolved; current draft keeps both length-neutral.
 * - `signedValue = expr; value = signedValue;` for DRAW2/row (coalesces,
 *   one short) and pasting the expression twice (cse does NOT fold, grows).
 * - A separate rowNumber pointer for the row draw (spills rankSpriteBase).
 * - Removing the i=0 entry fence (line ~320): regresses to 578.  Removing the
 *   brightness copy fence (line ~334): regresses to 503.  Both fences needed.
 * - Moving `ten = 10;`/`numberSprite = &number;` into the loop body: loop.c
 *   hoists them to the SAME preheader slot (before the /10 magic); no change.
 * - guided autorules on the 525 and 498 residuals: no genuine win (every
 *   candidate regresses or is a LOCAL-SHAPE regression).  Do not rerun broad.
 *
 * INFEASIBLE (measured, do not retry):
 * - ten s7->s8 / rankSpriteBase s8->s7 swap: NOT a steerable priority tie.
 *   regalloc.py: ten=p95 priority 1443, rankSpriteBase=p807 priority 99 (15x
 *   gap, both cross 33 calls).  ten wins s7 by huge margin; no minor lever
 *   flips them.  This also owns every DRAW `w*ten` (mult v0,s7 vs s8).
 *
 * Remaining classes (498 bytes, all hard sub-C ties — owner=regalloc per
 * rtlguide): entry a0/128 dbr delay-slot fill; LoadTIMAndFree arg copy 9 low
 * (brightness-fence-blocked, above); s2->s3 drawnSprite/tim/sprite cascade
 * (prio 79200, no hard-conflict but rotates the whole s-family); rank/char
 * bank-address hoist (addiu v0,sp,K vs sp+i*36 then +K); DRAW2/row value/sv
 * register roles; colon x=0x66 store deferral (li t1 vs li v0); scan/shift
 * addu operand orders; row-loop transient +4 skew; GsSortSprite a2 schedules.
 *
 * The local aggregates reproduce the original stack layout: packed
 * ScoreResult copy at sp+50, sprite banks at sp+60/sp+118, one GsIMAGE
 * scratch at sp+160 reused by all seven sprite initialisations.
 *
 * 2026-07-17 round-4 REJECTED (measured; do not repeat):
 * - Row s32 carrier with `sv = (s16)(i+1) << 16; ... sv >>= 16;` (in-place
 *   sll/sra pair like the target's 559a8/ac): correct-length head but the
 *   pair HOISTS to the block top (its statement's early LUID; the pair is
 *   sched-mobile) and the medal nop stays -> +1 insn overall.  The v2 s16
 *   spelling instead lets combine fold sra+bgez (one short) which exactly
 *   cancels the medal nop; both are the SAME parked ±4 pair.
 * - The medal bgez delay nop (target fills it with the `move a0,s0` arg
 *   copy): a second `medalSprite` pointer is REQUIRED (sbs+call via a0-born
 *   pre-branch), but every spelling that keeps its pseudo alive perturbs the
 *   global conflict web and flips draws 0/2/3's jump1 sign arms (their
 *   drawY-t0 carriers dissolve, j's vanish, -3 insns): initializer form
 *   aliases at expand (2 refs); declare-then-assign dies to cse1
 *   -fcse-skip-blocks (uses re-canoned to medal across the skipped
 *   brightness arm); do{}while(0) around the copy makes loop.c
 *   move_movables SUBSTITUTE it away (reg=reg movable); a dead `medal++`
 *   blocker (straight-line or arm-local, any position) preserves the copy
 *   and fills the slot EXACTLY like the target but always costs the
 *   drawY collateral.  4 candidate spellings measured 4624/4640; parked.
 *
 * 2026-07-16 round-3 closure (flat at 476; do not retry these):
 * - The drawY-carrier lead is CLOSED at the C level: for constant-y draws
 *   (5/7/8, y=-53/-36/-18) copy-prop/constant-rematerialization dissolves
 *   any carrier (seed AND store through it are strict no-ops), so it cannot
 *   pin y live to force v0/t1. The only computed-y site (the row draw) is
 *   separately blocked: rowValue->a0 is a p91 HARD-CONFLICT, which also
 *   produces the self-cancelling +4/-4 skew pair (medal nop / merged j).
 * - Draws 5/7/8's v0-vs-t1 shift is a sched1 y-load sink tied to their s16
 *   (lhu) value loads; jump restructuring regresses the matching sign branch.
 * - The bank-address base-first/base-last swap is cc1 address-selection, not
 *   spelling (subscript and cast-macro forms diverge identically).
 * - Other measured dead ends: rankColour entry fence (no-op), row value as
 *   i+1 (length) or (u16)(i+1) (480), y-seed/x-store fence (+2 insns),
 *   160 guided candidates (no improving path).
 * Remaining residual classes: ten $s7<->$s8 x10 (priority-infeasible,
 * measured round 1), $v0->$a0 x7 hard-conflicts p91, $s2->$s3 x13, and the
 * scheduling islands above — all regalloc/sched ties; park pending a NEW
 * pass-level lever.
 */

typedef struct
{
    u8 stageBosses;
    u8 stageEnemies;
    u8 findEnemies;
    u8 murders;
    u8 criticals;
    u8 friendHits;
    u8 pad[2];
    s32 clock;
} ScoreStats;

typedef struct
{
    u16 field0;
    u16 field2;
    u16 field4;
    u16 field6;
    u16 score;
    s16 grade;
} ScoreResult;

typedef struct
{
    u8 pad_000[0x44C];
    u8 characters[5];
    u8 grades[5];
    u8 pad_456[2];
    s32 scores[5];
} MissionScorePersistent;

typedef struct
{
    u16 oldPad;
    u16 pad;
    void *background;
} MissionScoreTail;

extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_STAGE;
extern u8 CHOSEN_LANGUAGE;
extern u8 STAGE_LAYOUT_NUMBER;

extern char NUMBER_TIM_PATH[];
extern char *RANKS_ARCHIVE_PTRS[];
extern char *TRN_SPRITE_PTRS[];
extern char D_80013AA8[];
extern char D_80013AC0[];
extern u8 D_8001005C;

extern u8 D_8001044C[5];
extern u8 D_80010451[5];
extern s32 D_80010458[5];
extern Sprite3D *ItemImage[];
extern s16 D_8008ED50[];

extern s32 GameClock;
extern s16 SkipFrame;
extern GsOT *OTablePt;

extern void init_score_stats(ScoreStats *stats);
extern ScoreResult *calculate_score(ScoreStats *stats, s32 stage);
extern u_long *FileRead(char *path);
extern void vfree(void *ptr);
extern u_long *get_tim_from_archive(u_long *archive, s32 index);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void LoadTIM(u_long *tim);
extern void _PlayMusic(s32 music, s32 mode);
extern u32 GetRealPad(s32 port);
extern void StartDrawing(void);
extern void DrawBG(void *background);
extern void EndDrawing(s32 arg);
extern void DisposeBG(void *background);
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 priority);
extern void FUN_800515b0(GsSPRITE *number, s32 value, s16 x, s32 y,
                         s32 mode);
extern s32 rcos(s32 angle);
extern s32 rsin(s32 angle);
extern void FadeOutDirect(s16 time, s16 attribute, u8 r, u8 g, u8 b);
extern void FUN_80038ce0(void);
extern void FUN_800514d8(void);
extern void FUN_8004f6c0(s32 screen);

static inline void InitScoreSprite(u_long *tim, GsIMAGE *image,
                                   GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

/* The retail code recomputes each bank address for the final pivot writes. */
#define SCORE_SPRITE_AT(bank_, index_)                                       \
    ((GsSPRITE *)((u8 *)(bank_) + (index_) * sizeof(GsSPRITE)))

/* This is deliberately a macro.  Retail repeats the decimal loop at every
 * call site, with fresh arithmetic identities but shared sign/base-U state. */
#define DRAW_SCORE_Y_SEED_0(carrier_, y_) ((carrier_) = (y_))
#define DRAW_SCORE_Y_SEED_1(carrier_, y_) ((carrier_) = (y_))
#define DRAW_SCORE_Y_SEED_PICK_(jump_, carrier_, y_)                         \
    DRAW_SCORE_Y_SEED_##jump_(carrier_, y_)
#define DRAW_SCORE_Y_SEED_PICK(jump_, carrier_, y_)                          \
    DRAW_SCORE_Y_SEED_PICK_(jump_, carrier_, y_)

#define DRAW_SCORE_Y_STORE_0(sprite_, carrier_, y_) ((sprite_)->y = (y_))
#define DRAW_SCORE_Y_STORE_1(sprite_, carrier_, y_) ((sprite_)->y = (carrier_))
#define DRAW_SCORE_Y_STORE_PICK_(jump_, sprite_, carrier_, y_)               \
    DRAW_SCORE_Y_STORE_##jump_(sprite_, carrier_, y_)
#define DRAW_SCORE_Y_STORE_PICK(jump_, sprite_, carrier_, y_)                \
    DRAW_SCORE_Y_STORE_PICK_(jump_, sprite_, carrier_, y_)

/* VALUE_SEED_V: memory-sourced sites define value first, then narrow.
 * VALUE_SEED_S: computed sites define the signed value first, then copy. */
#define DRAW_SCORE_VALUE_SEED_V(value_, type_)                               \
    (value = (value_), signedValue = (type_)value)
/* Computed sites paste the expression twice; cse turns the second
 * evaluation into the retail value-copy of the signed carrier. */
#define DRAW_SCORE_VALUE_SEED_S(value_, type_)                               \
    (signedValue = (value_), value = (value_))

#define DRAW_SCORE_NUMBER(value_, type_, jump_, label_, sprite_, x_, y_)     \
    do                                                                       \
    {                                                                        \
        GsSPRITE *drawnSprite;                                               \
                                                                             \
        drawnSprite = (sprite_);                                             \
        DRAW_SCORE_NUMBER_(value_, type_, jump_, label_, drawnSprite,        \
                           x_, y_, V);                                       \
    } while (0)
#define DRAW_SCORE_NUMBER_SV(value_, type_, jump_, label_, sprite_, x_, y_)  \
    do                                                                       \
    {                                                                        \
        GsSPRITE *drawnSprite;                                               \
                                                                             \
        drawnSprite = (sprite_);                                             \
        DRAW_SCORE_NUMBER_(value_, type_, jump_, label_, drawnSprite,        \
                           x_, y_, S);                                       \
    } while (0)
/* The subtraction draw's carrier is a plain s32 (u8 - u8 int arithmetic):
 * the subu itself IS signedValue (single def, no sll/sra), and `value` is
 * the paste-twice cse copy, which sched sinks into the sign branch's delay
 * slot.  Everything else matches the jump_=1 shape. */
#define DRAW_SCORE_NUMBER_SV32(value_, label_, sprite_, x_, y_)               \
    do                                                                       \
    {                                                                        \
        GsSPRITE *drawnSprite;                                               \
        s32 dividend;                                                        \
        s32 remainder;                                                       \
        s32 quotient;                                                        \
        u16 value;                                                           \
        s32 signedValue;                                                     \
        s32 drawY;                                                           \
                                                                             \
        drawnSprite = (sprite_);                                             \
        signedValue = (value_);                                              \
        value = signedValue;                                                 \
        drawY = (y_);                                                        \
        drawnSprite->x = (x_);                                               \
        drawnSprite->y = drawY;                                              \
        if (signedValue < 0)                                                 \
        {                                                                    \
            value = -signedValue;                                            \
            negative = 1;                                                    \
            goto label_;                                                     \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            negative = 0;                                                    \
        }                                                                    \
label_:                                                                      \
        do                                                                   \
        {                                                                    \
            dividend = (s16)value;                                           \
            quotient = dividend / 10;                                        \
            remainder = dividend % 10;                                       \
            baseU = drawnSprite->u;                                          \
            drawnSprite->u = baseU + (s16)remainder * drawnSprite->w;        \
            GsSortSprite(drawnSprite, OTablePt, 0);                           \
            drawnSprite->x -= 12;                                            \
            value = quotient;                                                \
            quotient <<= 16;                                                 \
            drawnSprite->u = baseU;                                          \
        } while (quotient != 0);                                             \
        if (negative != 0)                                                   \
        {                                                                    \
            u32 signBaseU;                                                   \
                                                                             \
            signBaseU = drawnSprite->u;                                      \
            drawnSprite->u = signBaseU + drawnSprite->w * ten;               \
            GsSortSprite(drawnSprite, OTablePt, 0);                           \
            drawnSprite->u = signBaseU;                                      \
        }                                                                    \
    } while (0)

/* The row site's carrier is an s16 whose HImode add needs NO pre-extension
 * of i (addiu straight off the promoted register); `value` is u16 so the
 * copy is a plain HImode move (no andi).  The sign test then emits the
 * carrier's sll/sra extension pair right before the branch — the carrier
 * dies there, so the extension result inherits its register.  Draws go
 * through the pre-loop rowSprite pointer directly (no drawnSprite copy). */
#define DRAW_SCORE_NUMBER_ROW32(value_, label_, spr_, x_, y_)                 \
    do                                                                       \
    {                                                                        \
        s32 dividend;                                                        \
        s32 remainder;                                                       \
        s32 quotient;                                                        \
        u16 value;                                                           \
        s16 signedValue;                                                     \
        s32 drawY;                                                           \
        register s32 negative;                                               \
                                                                             \
        signedValue = (value_);                                              \
        value = signedValue;                                                 \
        drawY = (y_);                                                        \
        (spr_)->x = (x_);                                                    \
        (spr_)->y = drawY;                                                   \
        if (signedValue < 0)                                                 \
        {                                                                    \
            value = -signedValue;                                            \
            negative = 1;                                                    \
            goto label_;                                                     \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            negative = 0;                                                    \
        }                                                                    \
label_:                                                                      \
        do                                                                   \
        {                                                                    \
            dividend = (s16)value;                                           \
            quotient = dividend / 10;                                        \
            remainder = dividend % 10;                                       \
            baseU = (spr_)->u;                                               \
            (spr_)->u = baseU + (s16)remainder * (spr_)->w;                  \
            GsSortSprite((spr_), OTablePt, 0);                                \
            (spr_)->x -= 12;                                                 \
            value = quotient;                                                \
            quotient <<= 16;                                                 \
            (spr_)->u = baseU;                                               \
        } while (quotient != 0);                                             \
        if (negative != 0)                                                   \
        {                                                                    \
            u32 signBaseU;                                                   \
                                                                             \
            signBaseU = (spr_)->u;                                           \
            (spr_)->u = signBaseU + (spr_)->w * ten;                         \
            GsSortSprite((spr_), OTablePt, 0);                                \
            (spr_)->u = signBaseU;                                           \
        }                                                                    \
    } while (0)

#define DRAW_SCORE_NUMBER_(value_, type_, jump_, label_, spr_, x_, y_,       \
                           seed_)                                            \
    do                                                                       \
    {                                                                        \
        s32 dividend;                                                        \
        s32 remainder;                                                       \
        s32 quotient;                                                        \
        u32 value;                                                           \
        s16 signedValue;                                                     \
        s32 drawY;                                                           \
                                                                             \
        DRAW_SCORE_VALUE_SEED_##seed_(value_, type_);                        \
        DRAW_SCORE_Y_SEED_PICK(jump_, drawY, y_);                            \
        (spr_)->x = (x_);                                                    \
        DRAW_SCORE_Y_STORE_PICK(jump_, spr_, drawY, y_);                     \
        if (jump_)                                                           \
        {                                                                    \
            if (signedValue < 0)                                             \
            {                                                                \
                value = -signedValue;                                        \
                negative = 1;                                                \
                goto label_;                                                 \
            }                                                                \
            else                                                             \
            {                                                                \
                negative = 0;                                                \
            }                                                                \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            negative = 0;                                                    \
            if (signedValue >= 0)                                            \
                goto label_;                                                 \
            value = -signedValue;                                            \
            negative = 1;                                                    \
        }                                                                    \
label_:                                                                      \
        do                                                                   \
        {                                                                    \
            dividend = (s16)value;                                           \
            quotient = dividend / 10;                                        \
            remainder = dividend % 10;                                       \
            baseU = (spr_)->u;                                               \
            (spr_)->u = baseU + (s16)remainder * (spr_)->w;                  \
            GsSortSprite((spr_), OTablePt, 0);                                \
            (spr_)->x -= 12;                                                 \
            value = quotient;                                                \
            quotient <<= 16;                                                 \
            (spr_)->u = baseU;                                               \
        } while (quotient != 0);                                             \
        if (negative != 0)                                                   \
        {                                                                    \
            u32 signBaseU;                                                   \
                                                                             \
            signBaseU = (spr_)->u;                                           \
            (spr_)->u = signBaseU + (spr_)->w * ten;                         \
            GsSortSprite((spr_), OTablePt, 0);                                \
            (spr_)->u = signBaseU;                                           \
        }                                                                    \
    } while (0)

#define DRAW_SCORE_COLON(sprite_, x_)                                        \
    do                                                                       \
    {                                                                        \
        u8 oldU;                                                             \
        s32 colonDigit;                                                      \
                                                                             \
        number.x = (x_);                                                     \
        oldU = (sprite_)->u;                                                 \
        colonDigit = 12;                                                     \
        (sprite_)->u = oldU + (sprite_)->w * colonDigit;                     \
        GsSortSprite((sprite_), OTablePt, 0);                                 \
        (sprite_)->u = oldU;                                                 \
    } while (0)

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/mission_score_screen", mission_score_screen);
#else
void mission_score_screen(void)
{
    GsSPRITE number;
    ScoreStats stats;
    ScoreResult result;
    GsSPRITE rankSprites[5];
    GsSPRITE characterSprites[2];
    GsIMAGE image;
    MissionScoreTail tail;
    register u_long *tim;
    register u_long *archive;
    register GsSPRITE *sprite;
    register GsSPRITE *initSprite;
    register GsSPRITE *numberSprite;
    register GsSPRITE *medal;
    register u8 *unlock;
    register PersistentState *statePtr;
    register s16 i;
    register s16 newPress;
    s32 brightness;
    register s32 stageItem;
    register s32 goNext;
    register s32 rankColour;
    s32 ten;
    u32 baseU;
    register s32 negative;
    s32 insertedRank;

    tail.oldPad = 0;
    init_score_stats(&stats);
    /* Preserve the retail scheduler boundary between the initial index seed
     * and the colour/score setup. */
    do {
        i = 0;
    } while (0);
    rankColour = 128;
    result = *calculate_score(&stats, CHOSEN_STAGE);

    tim = FileRead(NUMBER_TIM_PATH);
    {
        register GsSPRITE *initNumber = &number;

        InitScoreSprite(tim, &image, initNumber);
        initNumber->attribute |= 0x50000000;
        initNumber->x = -140;
        initNumber->y = -40;
        do {
        } while (0);
        brightness = rankColour;
        initNumber->r = brightness;
        initNumber->g = brightness;
        initNumber->b = brightness;
        initNumber->mx = initNumber->w >> 1;
        initNumber->my = initNumber->h >> 1;
    }
    /* NO do{}while(0) fence here: merging the pivot reset into the rgb block
     * lets the LoadTIMAndFree(tim) arg copy `move a0,s2` float toward the top
     * (target hoists it right after InitSprite).  Re-adding a fence pins the
     * arg copy back down and regresses (see STATUS). */
    number.mx = 0;
    number.my = 0;
    LoadTIMAndFree(tim);
    number.w = 12;

    {
        register u32 attributeMask;

        archive = FileRead(RANKS_ARCHIVE_PTRS[CHOSEN_LANGUAGE]);
        attributeMask = 0x50000000;
score_rank_sprite_init_loop:
        {
        u32 attribute;
        u32 width;

        tim = get_tim_from_archive(archive, i);
        initSprite = SCORE_SPRITE_AT(rankSprites, i);
        InitScoreSprite(tim, &image, initSprite);
        attribute = initSprite->attribute;
        initSprite->x = -160;
        initSprite->y = -120;
        width = initSprite->w;
        initSprite->r = rankColour;
        initSprite->g = rankColour;
        initSprite->b = rankColour;
        initSprite->mx = width >> 1;
        initSprite->attribute = attribute | attributeMask;
        initSprite->my = initSprite->h >> 1;
        SCORE_SPRITE_AT(rankSprites, i)->mx = 0;
        SCORE_SPRITE_AT(rankSprites, i)->my = 0;
        LoadTIM(tim);
        }
        i++;
        if (i < 5)
            goto score_rank_sprite_init_loop;
    }

    {
        register s32 characterColour;

        i = 0;
        characterColour = 128;
score_character_sprite_init_loop:
        {
            u32 attribute;
            u32 width;
            u32 height;

            tim = get_tim_from_archive(archive, i + 5);
            initSprite = &characterSprites[i];
            InitScoreSprite(tim, &image, initSprite);
            /* Retail keeps this dead attribute load (value overwritten
             * before any use) — a leftover of the rank-loop copy. */
            attribute = *(u32 volatile *)&initSprite->attribute;
            width = initSprite->w;
            height = initSprite->h;
            initSprite->x = -160;
            initSprite->y = -120;
            initSprite->g = initSprite->r = characterColour;
            initSprite->b = characterColour;
            initSprite->mx = width >> 1;
            initSprite->my = height >> 1;
            SCORE_SPRITE_AT(characterSprites, i)->mx = 0;
            SCORE_SPRITE_AT(characterSprites, i)->my = 0;
            LoadTIM(tim);
        }
        i++;
        if (i < 2)
            goto score_character_sprite_init_loop;
    }
    vfree(archive);

    tim = FileRead(TRN_SPRITE_PTRS[CHOSEN_LANGUAGE]);
    tail.background = FUN_8004f4f8(tim);
    vfree(tim);

    {
        register MissionScorePersistent *initState =
            (MissionScorePersistent *)0x80010000;

        for (i = 0; i < 5; i++)
        {
            if (initState->scores[i] == 0)
            {
                initState->scores[i] = 0x1A5C2;
                initState->characters[i] = 0;
                initState->grades[i] = 0;
            }
        }
    }

    insertedRank = -1;
    {
        register MissionScorePersistent *rankState =
            (MissionScorePersistent *)0x80010000;

        for (i = 0; i < 5; i++)
        {
            if (rankState->grades[i] < result.grade)
            {
                insertedRank = i;
                break;
            }
            if (rankState->grades[i] == result.grade &&
                stats.clock < rankState->scores[i])
            {
                insertedRank = i;
                break;
            }
        }
    }

    {
        register s32 found = insertedRank;

        if (found >= 0)
        {
            i = 4;
            if (found < 4)
            {
                register MissionScorePersistent *scoreState =
                    (MissionScorePersistent *)0x80010000;

                do
                {
                    scoreState->scores[i] = scoreState->scores[i - 1];
                    scoreState->characters[i] =
                        scoreState->characters[i - 1];
                    scoreState->grades[i] = scoreState->grades[i - 1];
                    i--;
                } while (insertedRank < (s16)i);
            }

            {
                register MissionScorePersistent *insertState =
                    (MissionScorePersistent *)0x80010000;
                register s32 insertedAt = insertedRank;

                insertState->scores[insertedAt] = stats.clock;
                insertState->characters[insertedAt] =
                    ((PersistentState *)insertState)->chr;
                insertState->grades[insertedAt] = result.grade;
            }
        }
    }

    _PlayMusic(12, 1);
    ten = 10;
    numberSprite = &number;
    for (;;)
    {
        u16 pad = GetRealPad(0);
        newPress = pad & (pad ^ tail.oldPad);
        tail.oldPad = pad;
        if (newPress & 0x20)
        {
            goNext = 0;
            break;
        }
        if (newPress & 0x800)
        {
            goNext = 1;
            break;
        }

        StartDrawing();
        DrawBG(tail.background);
        FUN_800515b0(&number, stats.clock, 0x46, -0x61, 1);

        DRAW_SCORE_NUMBER(stats.criticals, s32, 1, score_number_0,
                          &number, 0x16, -0x47);
        DRAW_SCORE_COLON(numberSprite, 0x1F);
        DRAW_SCORE_NUMBER_SV32(stats.stageEnemies - stats.stageBosses,
                               score_number_1, numberSprite, 0x2F, -0x47);
        DRAW_SCORE_NUMBER(result.field0, s16, 1, score_number_2,
                          &number, 0x66, -0x47);

        DRAW_SCORE_NUMBER(stats.murders, s32, 0, score_number_3,
                          &number, 0x16, -0x35);
        DRAW_SCORE_COLON(numberSprite, 0x1F);
        DRAW_SCORE_NUMBER(stats.stageEnemies, s32, 0, score_number_4,
                          numberSprite, 0x2F, -0x35);
        DRAW_SCORE_NUMBER(result.field2, s16, 0, score_number_5,
                          &number, 0x66, -0x35);

        DRAW_SCORE_NUMBER(stats.findEnemies, s32, 0, score_number_6,
                          &number, 0x23, -0x24);
        DRAW_SCORE_NUMBER(result.field6, s16, 0, score_number_7,
                          &number, 0x66, -0x24);
        DRAW_SCORE_NUMBER(result.score, s16, 0, score_number_8,
                          &number, 0x66, -0x12);

        if (result.grade == RANK_GRAND_MASTER)
        {
            medal = &ItemImage[D_8008ED50[CHOSEN_STAGE]]->sprite;
            medal->x = 0x8A;
            medal->y = -0xE;
            medal->scalex = 0x1000;
            medal->scaley = 0x1000;
            brightness = rcos((GameClock << 12) / 90) * 80;
            if (brightness < 0)
            {
                brightness += 0xFFF;
            }
            brightness = (brightness >> 12) + 0x7F;
            medal->r = medal->g = medal->b = brightness;
            GsSortSprite(medal, OTablePt, 1);
        }

        {
            GsSPRITE *rankSpriteBase;
            register GsSPRITE *rowSprite;

            i = 0;
            rowSprite = &number;
            rankSpriteBase = rankSprites;
score_row_loop:
            {
                register MissionScorePersistent *scoreState;
                register MissionScorePersistent *characterState;
                register MissionScorePersistent *rankState;

                DRAW_SCORE_NUMBER_ROW32(i + 1, score_row_number,
                                        rowSprite, -0x8F, i * 0x16 + 0x18);
                scoreState = (MissionScorePersistent *)0x80010000;
                FUN_800515b0(&number, scoreState->scores[i],
                             0x79, i * 0x16 + 0x18, 1);

                characterState = (MissionScorePersistent *)0x80010000;
                sprite = &characterSprites[characterState->characters[i]];
                sprite->x = -0x79;
                sprite->y = i * 0x16 + 0x16;
                if (i == insertedRank)
                {
                    brightness = rsin((GameClock << 12) / 90) * 60;
                    if (brightness < 0)
                    {
                        brightness += 0xFFF;
                    }
                    brightness = (brightness >> 12) + 100;
                }
                else
                {
                    brightness = 0x80;
                }
                sprite->r = sprite->g = sprite->b = brightness;
                do {
                } while (0);
                GsSortSprite(sprite, OTablePt, 1);

                rankState = (MissionScorePersistent *)0x80010000;
                {
                    register GsSPRITE *rankSprite =
                        &rankSpriteBase[rankState->grades[i]];
                    rankSprite->r = rankSprite->g = rankSprite->b = 0x7F;
                    rankSprite->scalex = rankSprite->scaley = 0xB33;
                    rankSprite->x = -0x2F;
                    rankSprite->y = i * 0x16 + 0x16;
                    GsSortSprite(rankSprite, OTablePt, 1);
                }
            }
            i++;
            if (i < 3)
            {
                goto score_row_loop;
            }
        }

        SkipFrame = 2;
        EndDrawing(0);
    }

    if (result.grade == RANK_GRAND_MASTER)
    {
        register PersistentState *state = (PersistentState *)0x80010000;

        stageItem = D_8008ED50[state->stage];
        if (state->stock[stageItem + (state->chr << 5)] == 0xFE)
        {
            state->stock[stageItem + (state->chr << 5)] += 3;
        }
        stageItem = D_8008ED50[state->stage];
        if (state->backup[stageItem] == 0xFE)
        {
            state->backup[stageItem] += 3;
        }
    }

    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    do {
        statePtr = (PersistentState *)0x80010000;
    } while (0);
    if (D_8001005C != 0)
    {
        LoadTIMAndFree(PathFileRead(D_80013AA8, D_80013AC0));
        FUN_800514d8();
    }
    DisposeBG(tail.background);

    if (goNext == 1)
    {
        FUN_8004f6c0(0x11);
    }
    else
    {
        statePtr->layout = 0xFF;
        FUN_8004f6c0(0x10);
    }
}
#endif
