#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern int StageID;
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 *     extern struct Sprite3D *ItemImage[25];
 *     extern short SkipFrame;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — complete pure-C reconstruction with the exact target
 * length (6084 bytes / 1521 instructions), 0xf0-byte frame, 81 conditional
 * branches, 15 jumps, and 70 calls.  matchdiff reports 389 differing bytes
 * and fuzz-score reports 91.26%.  The decimal-rendering region now has the
 * target's evaluation and scheduling shape; most residuals are register
 * allocation in the stage scan and digit loops plus setup/post-game load
 * scheduling, not hidden asm.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/StageEndScreen", StageEndScreen);
#else

#include "item.h"

typedef struct
{
    u8 bosses;
    u8 enemies;
    u8 hidden_finds;
    u8 murders;
    u8 criticals;
    u8 friendly_hits;
    u8 pad[2];
    s32 clock;
} StageScoreStats;

typedef struct
{
    u16 value[6];
} StageScoreResult;

typedef struct
{
    u8 uid;
    u8 pad[0x1b];
} StageEndConfigEntry;

typedef struct
{
    u8 pad[0x10];
    Humanoid *Owner;
} StageEndCameraStatus;

typedef struct
{
    u8 pad[0x68];
    GsSPRITE sprite;
} StageRankIcon;

typedef struct BackGround BackGround;

typedef struct
{
    GsSPRITE digit;                 /* sp+0x18 */
    u8 pad_024[4];
    StageScoreStats stats;          /* sp+0x40 */
    u8 pad_04c[4];
    StageScoreResult current;       /* sp+0x50 */
    u8 pad_05c[4];
    StageScoreResult best;          /* sp+0x60 */
    u8 pad_06c[4];
    GsSPRITE rank;                  /* sp+0x70 */
    u8 pad_07c[4];
    GsIMAGE image;                  /* sp+0x98 */
    u8 pad_0b4[4];
    u16 old_pad;                    /* sp+0xb8 */
    u8 pad_0a2[2];
    BackGround *background;         /* sp+0xbc */
    u_long *rank_archive;           /* sp+0xc0 */
    u8 pad_0ac[4];
} StageEndScreenStack;

#define PSTATE ((PersistentState *)0x80010000)

extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_STAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 CHOSEN_LANGUAGE;
extern u8 D_80010048;
extern u8 D_8001005C;
extern s32 D_8001046C;
extern u8 SELECTED_ITEM_COUNTS[];
extern u8 ITEM_LOADOUT_BACKUP[];
extern u8 SHOP_STOCK_STATE_BY_CHAR[];
extern s16 D_8008EA78[];
extern s16 D_8008ED50[];
extern StageEndConfigEntry StageConfig[];
extern char NUMBER_TIM_PATH[];
extern char *RS_ARCHIVE_PTRS[];
extern char *RANK_ARCHIVE_PTRS[];
extern StageRankIcon *ItemImage[];
extern StageEndCameraStatus CamState;
extern s32 StageID;
extern s32 GameClock;
extern s16 SkipFrame;
extern GsOT *OTablePt;

extern void SetupAppearance(s32 character, s32 mode);
extern void PadShockAR(s32 port, s32 time, s32 left, s32 right);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, u8 b);
extern void FUN_80038ce0(void);
extern StageScoreStats *init_score_stats(StageScoreStats *stats);
extern StageScoreResult *calculate_score(StageScoreStats *stats, s16 stage);
extern void mission_score_screen(s32 stage);
extern u_long *FileRead(char *path);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void LoadTIM(u_long *tim);
extern BackGround *FUN_8004f4f8(u_long *tim);
extern void vfree(void *ptr);
extern u_long *get_tim_from_archive(u_long *archive, s16 index);
extern void _PlayMusic(s32 music, s32 mode);
extern u16 GetRealPad(s32 port);
extern void StartDrawing(void);
extern void DrawBG(BackGround *background);
extern void FUN_800515b0(GsSPRITE *sprite, s32 value, s32 x, s32 y, s32 mode);
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 priority);
extern s32 rsin(s32 angle);
extern s32 rcos(s32 angle);
extern void EndDrawing(s32 mode);
extern void DisposeBG(BackGround *background);
extern void FUN_80052ea8(PersistentState *state, StageScoreResult *result);
extern void FUN_800514d8(void);
extern void FUN_8004f6c0(s32 state);

static inline void StageEndInitSprite(u_long *tim, GsIMAGE *image,
    GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

#define DRAW_SCORE_NUMBER(value_, type_, jump_, label_,                    \
        x_, y_)                                                            \
    {                                                                      \
        s32 dividend;                                                      \
        s32 remainder;                                                     \
        s32 quotient;                                                      \
        u32 value;                                                         \
        s16 signed_value;                                                  \
        GsSPRITE *sprite;                                                  \
                                                                           \
        sprite = &stack.digit;                                             \
        value = (value_);                                                  \
        signed_value = (type_)value;                                       \
        sprite->x = (x_);                                                  \
        sprite->y = (y_);                                                  \
        if (jump_)                                                         \
        {                                                                  \
            if (signed_value < 0)                                          \
            {                                                              \
                value = -signed_value;                                     \
                negative = 1;                                              \
                goto label_;                                               \
            }                                                              \
            else                                                           \
            {                                                              \
                negative = 0;                                              \
            }                                                              \
        }                                                                  \
        else                                                               \
        {                                                                  \
            negative = 0;                                                  \
            if (signed_value >= 0)                                         \
                goto label_;                                               \
            value = -signed_value;                                         \
            negative = 1;                                                  \
        }                                                                  \
label_:                                                                    \
        do                                                                 \
        {                                                                  \
            dividend = (s16)value;                                         \
            quotient = dividend / 10;                                      \
            remainder = dividend % 10;                                     \
            base_u = sprite->u;                                            \
            sprite->u = base_u + (s16)remainder * sprite->w;               \
            GsSortSprite(sprite, OTablePt, 0);                              \
            sprite->x -= 12;                                               \
            value = quotient;                                              \
            quotient <<= 16;                                               \
            sprite->u = base_u;                                            \
        } while (quotient != 0);                                           \
        if (negative != 0)                                                 \
        {                                                                  \
            base_u = sprite->u;                                            \
            sprite->u = base_u + sprite->w * ten;                          \
            GsSortSprite(sprite, OTablePt, 0);                              \
            sprite->u = base_u;                                            \
        }                                                                  \
    }

#define DRAW_LAST_SCORE_NUMBER(value_, label_)                             \
    {                                                                      \
        s32 dividend;                                                      \
        s32 remainder;                                                     \
        s32 quotient;                                                      \
        u32 value;                                                         \
        s16 signed_value;                                                  \
        u8 base_u;                                                         \
        GsSPRITE *sprite;                                                  \
                                                                           \
        sprite = &stack.digit;                                             \
        value = (value_);                                                  \
        signed_value = (s16)value;                                         \
        sprite->x = best_x;                                                \
        sprite->y = 0x38;                                                  \
        negative = 0;                                                      \
        if (signed_value >= 0)                                             \
            goto label_;                                                   \
        value = -signed_value;                                             \
        negative = 1;                                                      \
label_:                                                                    \
        do                                                                 \
        {                                                                  \
            dividend = (s16)value;                                         \
            quotient = dividend / 10;                                      \
            remainder = dividend % 10;                                     \
            sprite->u = (s16)remainder * sprite->w +                       \
                (base_u = sprite->u);                                      \
            GsSortSprite(sprite, OTablePt, 0);                              \
            sprite->x -= 12;                                               \
            value = quotient;                                              \
            quotient <<= 16;                                               \
            sprite->u = base_u;                                            \
        } while (quotient != 0);                                           \
        if (negative != 0)                                                 \
        {                                                                  \
            sprite->u = base_u + sprite->w * ten;                          \
            GsSortSprite(sprite, OTablePt, 0);                              \
            sprite->u = base_u;                                            \
        }                                                                  \
    }

void StageEndScreen(void)
{
    StageEndScreenStack stack;
    StageScoreResult *score;
    StageScoreStats *record;
    StageScoreStats *layout_record;
    GsSPRITE *icon;
    u_long *tim;
    u8 *best_stage;
    s16 selection;
    s32 pulse;
    s32 i;
    s32 layout_index;
    s32 next_stage_index;
    u32 layout_character_offset;
    u32 layout_stage_offset;
    s32 ten;
    s32 top_y;
    s32 current_x;
    s32 best_x;
    s16 stage_index;
    s16 item_index;
    u16 pad;
    u16 pressed;
    u32 base_u;
    s32 negative;

    selection = 0;
    stack.old_pad = 0;
    SetupAppearance(0, -1);
    PadShockAR(0, 0, 0, 0);
    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();

    stage_index = 0;
    if (D_8008EA78[0] != CHOSEN_STAGE)
    {
        do
        {
            stage_index++;
        } while (D_8008EA78[stage_index] != CHOSEN_STAGE);
    }
    {
        u8 *state;

        state = (u8 *)0x80010000;
        *(s32 *)(state + 0x46c) |= 1 << (stage_index - 1);
        if (state[5] == 7 && state[0x5e] == 0)
        {
            *(s32 *)(state + 0x46c) |= 0x400;
        }
    }

    init_score_stats(&stack.stats);
    score = calculate_score(&stack.stats, CHOSEN_STAGE);
    stack.current = *score;
    *(StageScoreStats *)0x8001004c = stack.stats;

    {
        StageScoreStats *base_record;
        u32 character_offset;
        u32 stage_offset;
        u8 *state;

        state = (u8 *)0x80010000;
        character_offset = (u32)state[4] * 0x1d4;
        stage_offset = (u32)state[5] * 0x24 + 0x80010064;
        base_record = (StageScoreStats *)(character_offset + stage_offset);
        record = (StageScoreStats *)((u8 *)base_record +
            (u32)state[6] * 0xc);
        score = calculate_score(record, state[5]);
    }
    stack.best = *score;
    if ((s16)stack.current.value[4] > (s16)stack.best.value[4] ||
        ((s16)stack.current.value[4] == (s16)stack.best.value[4] &&
        stack.stats.clock < record->clock))
    {
        *record = stack.stats;
        stack.best = stack.current;
    }

    {
        StageEndConfigEntry *config;

        config = &StageConfig[StageID];
        if (config->uid == 0)
        {
            mission_score_screen(StageID);
        }
    }

    {
        best_x = 0x80010000;
        if (((u8 *)best_x)[5] == 7)
        {
            if (((u8 *)best_x)[0x5e] == 0)
            {
                *(s32 *)((u8 *)best_x + 0x46c) |= 0x400;
            }
        }
        else
        {
        tim = FileRead(NUMBER_TIM_PATH);
        {
            GsSPRITE *sprite;

            sprite = &stack.digit;
            StageEndInitSprite(tim, &stack.image, sprite);
            sprite->attribute |= 0x50000000;
            top_y = -0x35;
            sprite->x = -0x8c;
            sprite->y = -0x28;
            sprite->r = 0x80;
            sprite->g = 0x80;
            sprite->b = 0x80;
            sprite->mx = sprite->w >> 1;
            sprite->my = sprite->h >> 1;
            sprite->mx = 0;
            sprite->my = 0;
            LoadTIMAndFree(tim);
            sprite->w = 12;
        }

        tim = FileRead(RS_ARCHIVE_PTRS[((u8 *)best_x)[0x5e]]);
        ten = 10;
        stack.background = FUN_8004f4f8(tim);
        vfree(tim);

        stack.rank_archive = FileRead(RANK_ARCHIVE_PTRS[((u8 *)best_x)[0x5e]]);
        current_x = 0x52;
        tim = get_tim_from_archive(stack.rank_archive,
            (s16)stack.current.value[5]);
        best_x = 0x7f;
        StageEndInitSprite(tim, &stack.image, &stack.rank);
        stack.rank.x = -0xa0;
        stack.rank.y = -0x78;
        stack.rank.r = 0x80;
        stack.rank.g = 0x80;
        stack.rank.b = 0x80;
        stack.rank.attribute |= 0x50000000;
        stack.rank.mx = stack.rank.w >> 1;
        stack.rank.my = stack.rank.h >> 1;
        stack.rank.mx = 0;
        stack.rank.my = 0;
        LoadTIM(tim);

        _PlayMusic(12, 1);
        while (1)
        {
            pad = GetRealPad(0);
            pressed = pad & (pad ^ stack.old_pad);
            stack.old_pad = pad;
            if ((pressed & 0x20) != 0)
            {
                selection = 0;
                break;
            }
            if ((pressed & 0x40) != 0)
            {
                switch (!!pad)
                {
                case 0:
                    selection = 2;
                    break;
                default:
                    selection = 2;
                    break;
                }
                break;
            }
            selection = 1;
            if ((pressed & 0x800) != 0)
            {
                break;
            }

            StartDrawing();
            DrawBG(stack.background);
            FUN_800515b0(&stack.digit, stack.stats.clock, 0x61, -0x5d, 0);
            DRAW_SCORE_NUMBER(stack.stats.criticals, s32, 1, number_0,
                10, top_y);
            DRAW_SCORE_NUMBER((u32)stack.stats.enemies -
                stack.stats.bosses, s32, 1, number_1, 0x28, top_y);
            DRAW_SCORE_NUMBER(stack.current.value[0], s16, 1, number_2,
                current_x, top_y);
            DRAW_SCORE_NUMBER(stack.best.value[0], s16, 1, number_3,
                best_x, top_y);

            DRAW_SCORE_NUMBER(stack.stats.murders, s32, 0, number_4,
                10, -0x1a);
            DRAW_SCORE_NUMBER(stack.stats.enemies, s32, 0, number_5,
                0x28, -0x1a);
            DRAW_SCORE_NUMBER(stack.current.value[1], s16, 0, number_6,
                current_x, -0x1a);
            DRAW_SCORE_NUMBER(stack.best.value[1], s16, 0, number_7,
                best_x, -0x1a);

            DRAW_SCORE_NUMBER(stack.stats.hidden_finds, s32, 0, number_8,
                0x1c, 1);
            DRAW_SCORE_NUMBER(stack.current.value[3], s16, 0, number_9,
                current_x, 1);
            DRAW_SCORE_NUMBER(stack.best.value[3], s16, 0, number_10,
                best_x, 1);

            DRAW_SCORE_NUMBER(stack.stats.friendly_hits, s32, 0, number_11,
                0x1c, 0x1a);
            DRAW_SCORE_NUMBER(stack.current.value[2], s16, 0, number_12,
                current_x, 0x1a);
            DRAW_SCORE_NUMBER(stack.best.value[2], s16, 0, number_13,
                best_x, 0x1a);

            DRAW_SCORE_NUMBER(stack.current.value[4], s16, 0, number_14,
                current_x, 0x38);
            DRAW_LAST_SCORE_NUMBER(stack.best.value[4], number_15);

            do
            {
                stack.rank.x = -0x19;
                stack.rank.y = 0x4e;
                pulse = rsin((GameClock << 12) / 90) * 0x7f;
            } while (0);
            if (pulse < 0)
            {
                pulse += 0xfff;
            }
            stack.rank.r = stack.rank.g = stack.rank.b =
                (pulse >> 12) + 0x7f;
            GsSortSprite(&stack.rank, OTablePt, 1);

            if ((s16)stack.current.value[5] == 4)
            {
                icon = &ItemImage[D_8008ED50[CHOSEN_STAGE]]->sprite;
                icon->x = -0x78;
                icon->y = 0x38;
                icon->scalex = 0x1000;
                icon->scaley = 0x1000;
                pulse = rcos((GameClock << 12) / 90) * 0x50;
                if (pulse < 0)
                {
                    pulse += 0xfff;
                }
                icon->r = icon->g = icon->b = (pulse >> 12) + 0x7f;
                GsSortSprite(icon, OTablePt, 1);
            }

            SkipFrame = 2;
            EndDrawing(0);
        }

            DisposeBG(stack.background);
            vfree(stack.rank_archive);
        }
    }

    FUN_80052ea8(PSTATE, &stack.current);
    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();

    best_stage = (u8 *)PSTATE + PSTATE->chr;
    if (best_stage[0x60] < StageConfig[PSTATE->stage].uid)
    {
        best_stage[0x60] = StageConfig[PSTATE->stage].uid;
    }

    item_index = 1;
    do
    {
        if (CamState.Owner->item[item_index] == 0xff)
        {
            PSTATE->stock[item_index + (PSTATE->chr << 5)] = 0xff;
        }
        else if (CamState.Owner->item[item_index] != 0)
        {
            if (PSTATE->stock[item_index + (PSTATE->chr << 5)] == 0xfe)
            {
                PSTATE->stock[item_index + (PSTATE->chr << 5)] += 2;
            }
            PSTATE->stock[item_index + (PSTATE->chr << 5)] +=
                CamState.Owner->item[item_index];
            if (PSTATE->stock[item_index + (PSTATE->chr << 5)] >= 100)
            {
                PSTATE->stock[item_index + (PSTATE->chr << 5)] = 99;
            }
        }
        PSTATE->counts[item_index] = 0;
        item_index++;
    } while (item_index < 0x14);

    i = 0;
    do
    {
        PSTATE->backup[i] = PSTATE->stock[i + (PSTATE->chr << 5)];
        i++;
    } while (i < 0x14);

    if (D_8001005C != 0 && CHOSEN_STAGE != 7)
    {
        FUN_800514d8();
    }

    switch (selection)
    {
    case 0:
        PSTATE->flags48 &= 0xfe;
        if (PSTATE->stage == 7)
        {
            FUN_8004f6c0(0x12);
        }
        else
        {
            next_stage_index = StageConfig[PSTATE->stage].uid;
            next_stage_index++;
            PSTATE->stage = *(u8 *)&D_8008EA78[next_stage_index];
            layout_character_offset = (u32)PSTATE->chr * 0x1d4;
            layout_stage_offset =
                (u32)PSTATE->stage * 0x24 + 0x80010064;
            layout_record = (StageScoreStats *)(layout_character_offset +
                layout_stage_offset);
            layout_index = 0;
layout_loop:
            if (layout_record->bosses + layout_record->enemies == 0)
                goto layout_done;
            layout_index++;
            if (layout_index < 3)
            {
                layout_record = (StageScoreStats *)((u8 *)layout_record + 0xc);
                goto layout_loop;
            }
layout_done:
            if (layout_index == 3)
            {
                STAGE_LAYOUT_NUMBER = 0xff;
            }
            else
            {
                STAGE_LAYOUT_NUMBER = layout_index;
            }
        }
        break;
    case 1:
        {
            u8 *state;

            state = (u8 *)0x80010000;
            state[0x48] |= 1;
        }
        break;
    case 2:
        STAGE_LAYOUT_NUMBER = 0xff;
        FUN_8004f6c0(0x10);
        break;
    }

    FUN_8004f6c0(0x11);
}

#undef DRAW_SCORE_NUMBER
#undef DRAW_LAST_SCORE_NUMBER
#undef PSTATE

#endif

// triage: VERY-HARD — 1521 insns, mul/div, 22 loop, 28 callees, ~0.12 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 22 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80054230) */
// /* WARNING: Removing unreachable block (ram,0x80053c10) */
// /* WARNING: Removing unreachable block (ram,0x80053cf0) */
// /* WARNING: Removing unreachable block (ram,0x800542bc) */
// /* WARNING: Removing unreachable block (ram,0x80053c9c) */
// /* WARNING: Removing unreachable block (ram,0x80053d7c) */
// /* WARNING: Removing unreachable block (ram,0x80053f90) */
// /* WARNING: Removing unreachable block (ram,0x8005401c) */
//
// void StageEndScreen(void)
//
// {
//   bool bVar1;
//   uchar uVar2;
//   char cVar3;
//   ushort uVar4;
//   short sVar5;
//   uint *puVar6;
//   ulong *puVar7;
//   int iVar8;
//   int iVar9;
//   ushort uVar10;
//   undefined4 uVar11;
//   byte *pbVar12;
//   int iVar13;
//   uint *puVar14;
//   uint uVar15;
//   GsSPRITE local_d8;
//   uint local_b0;
//   uint local_ac;
//   uint local_a8;
//   undefined4 local_a0;
//   undefined4 local_9c;
//   undefined4 local_98;
//   undefined4 local_90;
//   undefined4 local_8c;
//   uint local_88;
//   GsSPRITE local_80;
//   GsIMAGE GStack_58;
//   ushort local_38;
//   BackGround *local_34;
//   ulong *local_30;
//
//   uVar15 = 0;
//   local_38 = 0;
//   SetupAppearance(0,-1);
//   PadShockAR(0,0,0,0);
//   FadeOutDirect(0x20,2,'\b','\b');
//   FUN_80038ce0();
//   iVar13 = 0;
//   sVar5 = 0;
//   uVar4 = DAT_8008ea78;
//   while (uVar4 != (byte)PersistentState._5_1_) {
//     iVar13 = iVar13 + 1;
//     sVar5 = (short)iVar13;
//     uVar4 = *(ushort *)((int)&DAT_8008ea78 + (iVar13 * 0x10000 >> 0xf));
//   }
//   PersistentState._1132_4_ = PersistentState._1132_4_ | 1 << ((int)sVar5 - 1U & 0x1f);
//   if ((PersistentState._5_1_ == '\a') && (PersistentState._94_1_ == '\0')) {
//     PersistentState._1132_4_ = PersistentState._1132_4_ | 0x400;
//   }
//   FUN_8004e964(&local_b0);
//   puVar6 = (uint *)FUN_8004e794(&local_b0,PersistentState._5_1_);
//   local_a0 = *puVar6;
//   local_9c = puVar6[1];
//   local_98 = puVar6[2];
//   PersistentState._76_4_ = local_b0;
//   PersistentState._80_4_ = local_ac;
//   PersistentState._84_4_ = local_a8;
//   puVar14 = (uint *)((uint)(byte)PersistentState._4_1_ * 0x1d4 +
//                      (uint)(byte)PersistentState._5_1_ * 0x24 + -0x7ffeff9c +
//                     (uint)(byte)PersistentState._6_1_ * 0xc);
//   puVar6 = (uint *)FUN_8004e794(puVar14,PersistentState._5_1_);
//   local_90 = *puVar6;
//   local_8c = puVar6[1];
//   local_88 = puVar6[2];
//   if (((short)local_88 < (short)local_98) ||
//      (((short)local_98 == (short)local_88 && ((int)local_a8 < (int)puVar14[2])))) {
//     *puVar14 = local_b0;
//     puVar14[1] = local_ac;
//     puVar14[2] = local_a8;
//     local_90 = local_a0;
//     local_8c = local_9c;
//     local_88 = local_98;
//   }
//   if (StageConfig[StageID].uid == '\0') {
//     FUN_80054b48();
//   }
//   if (PersistentState._5_1_ == '\a') {
//     if (PersistentState._94_1_ == '\0') {
//       PersistentState._1132_4_ = PersistentState._1132_4_ | 0x400;
//     }
//   }
//   else {
//     puVar7 = FileRead("K:\\WORK\\CDIMAGE\\DEMO\\number.tim");
//     GetTIMInfo(puVar7,&GStack_58);
//     InitSprite(&GStack_58,&local_d8);
//     local_d8.attribute = local_d8.attribute | 0x50000000;
//     local_d8.x = -0x8c;
//     local_d8.y = -0x28;
//     local_d8.r = 0x80;
//     local_d8.g = 0x80;
//     local_d8.b = 0x80;
//     local_d8.mx = 0;
//     local_d8.my = 0;
//     LoadTIMAndFree(puVar7);
//     local_d8.w = 0xc;
//     puVar7 = FileRead((&PTR_s_K__WORK_CDIMAGE_DEMO_rs_eng_tim_8008ef38)
//                       [(byte)PersistentState._94_1_]);
//     local_34 = (BackGround *)FUN_8004f4f8(puVar7);
//     vfree((undefined *)puVar7);
//     local_30 = FileRead((&PTR_s_K__WORK_CDIMAGE_DEMO_Ranks_e_Arc_8008ef48)
//                         [(byte)PersistentState._94_1_]);
//     puVar7 = (ulong *)FUN_8004f1d8(local_30,(int)local_98._2_2_);
//     GetTIMInfo(puVar7,&GStack_58);
//     InitSprite(&GStack_58,&local_80);
//     local_80.x = -0xa0;
//     local_80.y = -0x78;
//     local_80.r = 0x80;
//     local_80.g = 0x80;
//     local_80.b = 0x80;
//     local_80.attribute = local_80.attribute | 0x50000000;
//     local_80.mx = 0;
//     local_80.my = 0;
//     LoadTIM(puVar7);
//     _PlayMusic(0xc,1);
//     while( true ) {
//       uVar4 = GetRealPad(0);
//       uVar10 = uVar4 & (uVar4 ^ local_38);
//       local_38 = uVar4;
//       if ((uVar10 & 0x20) != 0) break;
//       if ((uVar10 & 0x40) != 0) {
//         uVar15 = 2;
//         goto LAB_800547c8;
//       }
//       uVar15 = 1;
//       if ((uVar10 & 0x800) != 0) goto LAB_800547c8;
//       StartDrawing();
//       DrawBG(local_34);
//       FUN_800515b0(&local_d8,local_a8,0x61,0xffffffa3,0);
//       uVar2 = local_d8.u;
//       uVar15 = local_ac & 0xff;
//       local_d8.x = 10;
//       local_d8.y = -0x35;
//       if ((int)uVar15 < 0) {
//         uVar15 = -uVar15;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       local_d8.x = 0x28;
//       local_d8.y = -0x35;
//       uVar15 = (local_b0 >> 8 & 0xff) - (local_b0 & 0xff);
//       if ((int)uVar15 < 0) {
//         uVar15 = -uVar15;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_a0 & 0xffff;
//       local_d8.x = 0x52;
//       local_d8.y = -0x35;
//       if ((short)local_a0 < 0) {
//         uVar15 = -(int)(short)local_a0;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_90 & 0xffff;
//       local_d8.x = 0x7f;
//       local_d8.y = -0x35;
//       if ((short)local_90 < 0) {
//         uVar15 = -(int)(short)local_90;
//         bVar1 = true;
//       }
//       else {
//         bVar1 = false;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_b0 >> 0x18;
//       local_d8.x = 10;
//       local_d8.y = -0x1a;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = local_b0 >> 8 & 0xff;
//       local_d8.x = 0x28;
//       local_d8.y = -0x1a;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = (uint)local_a0._2_2_;
//       local_d8.x = 0x52;
//       local_d8.y = -0x1a;
//       bVar1 = false;
//       if ((short)local_a0._2_2_ < 0) {
//         uVar15 = -(int)(short)local_a0._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = (uint)local_90._2_2_;
//       local_d8.x = 0x7f;
//       local_d8.y = -0x1a;
//       bVar1 = false;
//       if ((short)local_90._2_2_ < 0) {
//         uVar15 = -(int)(short)local_90._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_b0 >> 0x10 & 0xff;
//       local_d8.x = 0x1c;
//       local_d8.y = 1;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = (uint)local_9c._2_2_;
//       local_d8.x = 0x52;
//       local_d8.y = 1;
//       bVar1 = false;
//       if ((short)local_9c._2_2_ < 0) {
//         uVar15 = -(int)(short)local_9c._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = (uint)local_8c._2_2_;
//       local_d8.x = 0x7f;
//       local_d8.y = 1;
//       bVar1 = false;
//       if ((short)local_8c._2_2_ < 0) {
//         uVar15 = -(int)(short)local_8c._2_2_;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_ac >> 8 & 0xff;
//       local_d8.x = 0x1c;
//       local_d8.y = 0x1a;
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       uVar15 = local_9c & 0xffff;
//       local_d8.x = 0x52;
//       local_d8.y = 0x1a;
//       bVar1 = false;
//       if ((short)local_9c < 0) {
//         uVar15 = -(int)(short)local_9c;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_8c & 0xffff;
//       local_d8.x = 0x7f;
//       local_d8.y = 0x1a;
//       bVar1 = false;
//       if ((short)local_8c < 0) {
//         uVar15 = -(int)(short)local_8c;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_98 & 0xffff;
//       local_d8.x = 0x52;
//       local_d8.y = 0x38;
//       bVar1 = false;
//       if ((short)local_98 < 0) {
//         uVar15 = -(int)(short)local_98;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       uVar15 = local_88 & 0xffff;
//       local_d8.x = 0x7f;
//       local_d8.y = 0x38;
//       bVar1 = false;
//       if ((short)local_88 < 0) {
//         uVar15 = -(int)(short)local_88;
//         bVar1 = true;
//       }
//       do {
//         sVar5 = (short)uVar15;
//         uVar15 = (int)sVar5 / 10;
//         local_d8.u = uVar2 + (char)((uint)(((int)sVar5 % 10) * 0x10000) >> 0x10) * (char)local_d8.w;
//         GsSortSprite(&local_d8,OTablePt,0);
//         local_d8.x = local_d8.x + -0xc;
//       } while ((uVar15 & 0xffff) != 0);
//       if (bVar1) {
//         local_d8.u = uVar2 + (char)local_d8.w * '\n';
//         GsSortSprite(&local_d8,OTablePt,0);
//       }
//       local_80.x = -0x19;
//       local_80.y = 0x4e;
//       local_d8.u = uVar2;
//       iVar13 = rsin((GameClock * 0x1000) / 0x5a);
//       iVar13 = iVar13 * 0x7f;
//       if (iVar13 < 0) {
//         iVar13 = iVar13 + 0xfff;
//       }
//       local_80.r = (char)(iVar13 >> 0xc) + '\x7f';
//       local_80.g = local_80.r;
//       local_80.b = local_80.r;
//       GsSortSprite(&local_80,OTablePt,1);
//       if (local_98._2_2_ == 4) {
//         iVar8 = GameClock * 0x1000;
//         iVar13 = (&DAT_8008e5bc)[*(short *)(&DAT_8008ed50 + (uint)(byte)PersistentState._5_1_ * 2)];
//         *(undefined2 *)(iVar13 + 0x6c) = 0xff88;
//         *(undefined2 *)(iVar13 + 0x6e) = 0x38;
//         *(undefined2 *)(iVar13 + 0x84) = 0x1000;
//         *(undefined2 *)(iVar13 + 0x86) = 0x1000;
//         iVar8 = rcos(iVar8 / 0x5a);
//         iVar8 = iVar8 * 0x50;
//         if (iVar8 < 0) {
//           iVar8 = iVar8 + 0xfff;
//         }
//         cVar3 = (char)(iVar8 >> 0xc) + '\x7f';
//         *(char *)(iVar13 + 0x7e) = cVar3;
//         *(char *)(iVar13 + 0x7d) = cVar3;
//         *(char *)(iVar13 + 0x7c) = cVar3;
//         GsSortSprite((GsSPRITE *)(iVar13 + 0x68),OTablePt,1);
//       }
//       SkipFrame = 2;
//       EndDrawing(0);
//     }
//     uVar15 = 0;
// LAB_800547c8:
//     DisposeBG(local_34);
//     vfree((undefined *)local_30);
//   }
//   FUN_80052ea8(&PersistentState,&local_a0);
//   FadeOutDirect(0x20,2,'\b','\b');
//   FUN_80038ce0();
//   iVar13 = 1;
//   if ((byte)(&PersistentState.field_0x60)[(byte)PersistentState._4_1_] <
//       StageConfig[(byte)PersistentState._5_1_].uid) {
//     (&PersistentState.field_0x60)[(byte)PersistentState._4_1_] =
//          StageConfig[(byte)PersistentState._5_1_].uid;
//   }
//   iVar8 = 0x10000;
//   do {
//     iVar8 = iVar8 >> 0x10;
//     if ((CamState.Owner)->item[iVar8] == 0xff) {
//       (&PersistentState.field_0x40c)[iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20] = 0xff;
//     }
//     else if ((CamState.Owner)->item[iVar8] != '\0') {
//       iVar9 = iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20;
//       if ((&PersistentState.field_0x40c)[iVar9] == -2) {
//         (&PersistentState.field_0x40c)[iVar9] = 0;
//       }
//       iVar9 = iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20;
//       (&PersistentState.field_0x40c)[iVar9] =
//            (&PersistentState.field_0x40c)[iVar9] + (CamState.Owner)->item[iVar8];
//       iVar8 = iVar8 + (uint)(byte)PersistentState._4_1_ * 0x20;
//       if (99 < (byte)(&PersistentState.field_0x40c)[iVar8]) {
//         (&PersistentState.field_0x40c)[iVar8] = 99;
//       }
//     }
//     (&PersistentState.field_0x7)[(short)iVar13] = 0;
//     iVar13 = iVar13 + 1;
//     iVar8 = iVar13 * 0x10000;
//   } while (iVar13 * 0x10000 >> 0x10 < 0x14);
//   iVar13 = 0;
//   do {
//     iVar8 = iVar13 + 1;
//     (&PersistentState.field_0x27)[iVar13] =
//          (&PersistentState.field_0x40c)[iVar13 + (uint)(byte)PersistentState._4_1_ * 0x20];
//     iVar13 = iVar8;
//   } while (iVar8 < 0x14);
//   if ((PersistentState._92_1_ != '\0') && (PersistentState._5_1_ != '\a')) {
//     FUN_800514d8();
//   }
//   if (uVar15 == 1) {
//     PersistentState._72_1_ = PersistentState._72_1_ | 1;
//     goto LAB_80054b10;
//   }
//   if (uVar15 < 2) {
//     if (uVar15 != 0) goto LAB_80054b10;
//     PersistentState._72_1_ = PersistentState._72_1_ & 0xfe;
//     if (PersistentState._5_1_ != '\a') {
//       PersistentState._5_1_ =
//            *(undefined1 *)(&DAT_8008ea78 + StageConfig[(byte)PersistentState._5_1_].uid + 1);
//       iVar13 = 0;
//       pbVar12 = (byte *)((uint)(byte)PersistentState._4_1_ * 0x1d4 +
//                         (uint)(byte)PersistentState._5_1_ * 0x24 + -0x7ffeff9c);
//       do {
//         if ((uint)*pbVar12 + (uint)pbVar12[1] == 0) break;
//         iVar13 = iVar13 + 1;
//         pbVar12 = pbVar12 + 0xc;
//       } while (iVar13 < 3);
//       if (iVar13 == 3) {
//         PersistentState._6_1_ = 0xff;
//       }
//       else {
//         PersistentState._6_1_ = SUB41(iVar13,0);
//       }
//       goto LAB_80054b10;
//     }
//     uVar11 = 0x12;
//   }
//   else {
//     if (uVar15 != 2) goto LAB_80054b10;
//     PersistentState._6_1_ = 0xff;
//     uVar11 = 0x10;
//   }
//   FUN_8004f6c0(uVar11);
// LAB_80054b10:
//   FUN_8004f6c0(0x11);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? DisposeBG(s32);                                   /* extern */
// ? DrawBG(s32);                                      /* extern */
// ? EndDrawing(?);                                    /* extern */
// ? FUN_80038ce0();                                   /* extern */
// s32 FUN_8004f4f8(s32);                              /* extern */
// ? FUN_8004f6c0(?);                                  /* extern */
// ? FUN_800514d8(s32, ? *, u8 *, s16);                /* extern */
// ? FUN_800515b0(s32 *, s32, ?, ?, s32);              /* extern */
// ? FUN_80052ea8(?, u16 *);                           /* extern */
// ? FadeOutDirect(?, ?, ?, ?, s32);                   /* extern */
// s32 FileRead(? *);                                  /* extern */
// u16 GetRealPad(?);                                  /* extern */
// ? GetTIMInfo(s32, ? *);                             /* extern */
// ? GsSortSprite(s32 *, s32, ?);                      /* extern */
// ? InitSprite(? *, s32 *);                           /* extern */
// ? LoadTIM(s32);                                     /* extern */
// ? LoadTIMAndFree(s32);                              /* extern */
// ? PadShockAR(?, ?, ?, ?);                           /* extern */
// ? SetupAppearance(?, ?);                            /* extern */
// ? StartDrawing();                                   /* extern */
// ? _PlayMusic(?, ?);                                 /* extern */
// void *calculate_score(s32 *, u8, ?);                /* extern */
// s32 get_tim_from_archive(s32, s16);                 /* extern */
// ? init_score_stats(s32 *, s32 *);                   /* extern */
// ? mission_score_screen(s32);                        /* extern */
// s32 rcos(s32);                                      /* extern */
// s32 rsin(s32);                                      /* extern */
// ? vfree(s32);                                       /* extern */
// extern u8 CHOSEN_CHARACTER;
// extern u8 CHOSEN_LANGUAGE;
// extern u8 CHOSEN_STAGE;
// extern ? CamState;
// extern u8 D_80010048;
// extern u8 D_8001005C;
// extern s32 D_8001046C;
// extern ? ItemImage;
// extern s16 D_8008EA78;
// extern ? D_8008ED50;
// extern s32 GameClock;
// extern ? NUMBER_TIM_PATH;
// extern s32 OTablePt;
// extern ? RANK_ARCHIVE_PTRS;
// extern ? RS_ARCHIVE_PTRS;
// extern u8 SELECTED_ITEM_COUNTS;
// extern ? SHOP_STOCK_STATE_BY_CHAR;
// extern s8 STAGE_LAYOUT_NUMBER;
// extern s16 SkipFrame;
// extern ? StageConfig;
// extern s32 StageID;
//
// void StageEndScreen(void) {
//     s32 sp18;
//     s16 sp20;
//     s16 sp30;
//     s16 sp32;
//     s32 sp40;
//     u16 sp50;
//     u16 sp54;
//     s16 sp58;
//     u16 sp60;
//     u16 sp64;
//     s16 sp68;
//     s32 sp70;
//     s16 sp74;
//     s16 sp76;
//     s8 sp84;
//     s8 sp85;
//     s8 sp86;
//     s16 sp88;
//     s16 sp8A;
//     ? sp98;
//     u16 spB8;
//     s32 spBC;
//     s32 spC0;
//     ? var_a0_2;
//     s16 temp_s0_4;
//     s16 temp_v0;
//     s16 temp_v0_12;
//     s16 temp_v0_9;
//     s16 var_a3;
//     s16 var_a3_2;
//     s16 var_v1_2;
//     s32 *temp_s0;
//     s32 *temp_s0_34;
//     s32 temp_a0;
//     s32 temp_a1_2;
//     s32 temp_v0_4;
//     s32 temp_v0_5;
//     s32 temp_v0_6;
//     s32 temp_v0_7;
//     s32 temp_v1_2;
//     s32 var_a0;
//     s32 var_s1;
//     s32 var_s3;
//     s32 var_s3_10;
//     s32 var_s3_11;
//     s32 var_s3_12;
//     s32 var_s3_13;
//     s32 var_s3_14;
//     s32 var_s3_15;
//     s32 var_s3_16;
//     s32 var_s3_2;
//     s32 var_s3_3;
//     s32 var_s3_4;
//     s32 var_s3_5;
//     s32 var_s3_6;
//     s32 var_s3_7;
//     s32 var_s3_8;
//     s32 var_s3_9;
//     s32 var_v0;
//     s32 var_v0_2;
//     s32 var_v0_3;
//     s32 var_v0_4;
//     s8 temp_v0_10;
//     s8 temp_v0_11;
//     s8 var_a1;
//     u16 temp_s0_14;
//     u16 temp_s0_16;
//     u16 temp_s0_20;
//     u16 temp_s0_22;
//     u16 temp_s0_26;
//     u16 temp_s0_28;
//     u16 temp_s0_30;
//     u16 temp_s0_32;
//     u16 temp_s0_6;
//     u16 temp_s0_8;
//     u16 temp_v0_8;
//     u16 temp_v1;
//     u16 var_v1_10;
//     u16 var_v1_11;
//     u16 var_v1_13;
//     u16 var_v1_14;
//     u16 var_v1_15;
//     u16 var_v1_16;
//     u16 var_v1_3;
//     u16 var_v1_4;
//     u16 var_v1_7;
//     u16 var_v1_8;
//     u8 *temp_a0_2;
//     u8 *temp_a1;
//     u8 *temp_v0_13;
//     u8 *temp_v1_6;
//     u8 *temp_v1_7;
//     u8 temp_s0_10;
//     u8 temp_s0_11;
//     u8 temp_s0_12;
//     u8 temp_s0_13;
//     u8 temp_s0_15;
//     u8 temp_s0_17;
//     u8 temp_s0_18;
//     u8 temp_s0_19;
//     u8 temp_s0_21;
//     u8 temp_s0_23;
//     u8 temp_s0_24;
//     u8 temp_s0_25;
//     u8 temp_s0_27;
//     u8 temp_s0_29;
//     u8 temp_s0_2;
//     u8 temp_s0_31;
//     u8 temp_s0_33;
//     u8 temp_s0_3;
//     u8 temp_s0_5;
//     u8 temp_s0_7;
//     u8 temp_s0_9;
//     u8 temp_s1;
//     u8 temp_s1_10;
//     u8 temp_s1_11;
//     u8 temp_s1_12;
//     u8 temp_s1_13;
//     u8 temp_s1_14;
//     u8 temp_s1_15;
//     u8 temp_s1_16;
//     u8 temp_s1_2;
//     u8 temp_s1_3;
//     u8 temp_s1_4;
//     u8 temp_s1_5;
//     u8 temp_s1_6;
//     u8 temp_s1_7;
//     u8 temp_s1_8;
//     u8 temp_s1_9;
//     u8 temp_v0_14;
//     u8 temp_v0_15;
//     u8 temp_v1_3;
//     u8 temp_v1_4;
//     u8 temp_v1_5;
//     u8 temp_v1_8;
//     u8 var_v1;
//     u8 var_v1_12;
//     u8 var_v1_5;
//     u8 var_v1_6;
//     u8 var_v1_9;
//     void *temp_v0_2;
//     void *temp_v0_3;
//     void *var_a0_3;
//
//     var_s1 = 0;
//     spB8 = 0;
//     SetupAppearance(0, -1);
//     PadShockAR(0, 0, 0, 0);
//     FadeOutDirect(0x20, 2, 8, 8, 8);
//     FUN_80038ce0();
//     var_a3 = 0;
//     if (D_8008EA78 != CHOSEN_STAGE) {
//         do {
//             temp_v0 = var_a3 + 1;
//             var_a3 = temp_v0;
//         } while (*(((s32) (temp_v0 << 0x10) >> 0xF) + &D_8008EA78) != CHOSEN_STAGE);
//     }
//     temp_a0 = D_8001046C | (1 << (var_a3 - 1));
//     D_8001046C = temp_a0;
//     if ((u8) D_8001046C == 7) {
//         if ((u8) D_8001046C == 0) {
//             D_8001046C = temp_a0 | 0x400;
//         }
//     }
//     init_score_stats(&sp40, &D_8001046C);
//     temp_v0_2 = calculate_score(&sp40, CHOSEN_STAGE);
//     sp50 = (unaligned s32) temp_v0_2->unk0;
//     sp54 = (unaligned s32) temp_v0_2->unk4;
//     sp58 = (unaligned s32) temp_v0_2->unk8;
//     (void *)0x8001004C->unk0 = sp40;
//     (void *)0x8001004C->unk4 = sp44;
//     (void *)0x8001004C->unk8 = sp48;
//     temp_s0 = (*(u8 *)0x80010000 * 0x1D4) + ((*(u8 *)0x80010000 * 0x24) + 0x80010064) + (*(u8 *)0x80010000 * 0xC);
//     temp_v0_3 = calculate_score(temp_s0, *(u8 *)0x80010000, 0x80010064);
//     sp60 = (unaligned s32) temp_v0_3->unk0;
//     sp64 = (unaligned s32) temp_v0_3->unk4;
//     sp68 = (unaligned s32) temp_v0_3->unk8;
//     if ((sp68 < sp58) || ((sp58 == sp68) && (sp48 < temp_s0->unk8))) {
//         temp_s0->unk0 = sp40;
//         temp_s0->unk4 = sp44;
//         temp_s0->unk8 = sp48;
//         sp60 = (unaligned s32) sp50;
//         sp64 = (unaligned s32) sp54;
//         sp68 = (unaligned s32) sp58;
//     }
//     if (*((StageID * 0x1C) + &StageConfig) == 0) {
//         mission_score_screen(StageID);
//     }
//     if (CHOSEN_STAGE == 7) {
//         if (CHOSEN_LANGUAGE == 0) {
//             CHOSEN_LANGUAGE = (s32) ((s32) CHOSEN_LANGUAGE | 0x400);
//         }
//     } else {
//         temp_v0_4 = FileRead(&NUMBER_TIM_PATH);
//         GetTIMInfo(temp_v0_4, &sp98);
//         InitSprite(&sp98, &sp18);
//         sp18 |= 0x50000000;
//         sp18.unk4 = -0x8CU;
//         sp18.unk6 = -0x28;
//         sp18.unk14 = 0x80;
//         sp18.unk15 = 0x80;
//         sp18.unk16 = 0x80;
//         sp18.unk18 = (s16) ((u16) sp18.unk8 >> 1);
//         sp18.unk1A = (s16) ((u16) sp18.unkA >> 1);
//         sp30 = 0;
//         sp32 = 0;
//         LoadTIMAndFree(temp_v0_4);
//         sp20 = 0xC;
//         temp_v0_5 = FileRead(*((CHOSEN_LANGUAGE * 4) + &RS_ARCHIVE_PTRS));
//         spBC = FUN_8004f4f8(temp_v0_5);
//         vfree(temp_v0_5);
//         temp_v0_6 = FileRead(*((CHOSEN_LANGUAGE * 4) + &RANK_ARCHIVE_PTRS));
//         spC0 = temp_v0_6;
//         temp_v0_7 = get_tim_from_archive(temp_v0_6, (s16) sp5A);
//         GetTIMInfo(temp_v0_7, &sp98);
//         InitSprite(&sp98, &sp70);
//         sp74 = -0xA0;
//         sp76 = -0x78;
//         sp84 = 0x80;
//         sp85 = 0x80;
//         sp86 = 0x80;
//         sp70 |= 0x50000000;
//         sp88 = (s16) (sp78 >> 1);
//         sp88 = 0;
//         sp8A = (s16) (sp7A >> 1);
//         sp8A = 0;
//         LoadTIM(temp_v0_7);
//         _PlayMusic(0xC, 1);
// loop_19:
//         temp_v0_8 = GetRealPad(0);
//         temp_v1 = spB8;
//         spB8 = temp_v0_8;
//         temp_v1_2 = temp_v0_8 & (temp_v0_8 ^ temp_v1);
//         if (!(temp_v1_2 & 0x20)) {
//             if (!(temp_v1_2 & 0x40)) {
//                 var_s1 = 1;
//                 if (!(temp_v1_2 & 0x800)) {
//                     StartDrawing();
//                     DrawBG(spBC);
//                     FUN_800515b0(&sp18, sp48, 0x61, -0x5D, 0);
//                     var_v1 = (u8) sp44;
//                     sp18.unk4 = 0xAU;
//                     sp18.unk6 = -0x35;
//                     if ((s32) var_v1 < 0) {
//                         var_v1 = (u8) -(s32) var_v1;
//                         var_s3 = 1;
//                     } else {
//                         var_s3 = 0;
//                     }
//                     do {
//                         temp_s0_2 = var_v1 / 10;
//                         temp_s1 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1 + ((s16) (var_v1 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1 = temp_s0_2;
//                         sp18.unkE = temp_s1;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_2 << 0x10) != 0);
//                     if (var_s3 != 0) {
//                         temp_s0_3 = temp_s1 & 0xFF;
//                         sp18.unkE = (u8) (temp_s0_3 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_3;
//                     }
//                     sp18.unk4 = 0x28U;
//                     sp18.unk6 = -0x35;
//                     temp_v0_9 = unksp41 - (u8) sp40;
//                     var_v1_2 = temp_v0_9;
//                     if (temp_v0_9 < 0) {
//                         var_v1_2 = -temp_v0_9;
//                         var_s3_2 = 1;
//                     } else {
//                         var_s3_2 = 0;
//                     }
//                     do {
//                         temp_s0_4 = var_v1_2 / 10;
//                         temp_s1_2 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_2 + ((s16) (var_v1_2 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_2 = temp_s0_4;
//                         sp18.unkE = temp_s1_2;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_4 << 0x10) != 0);
//                     temp_s0_5 = temp_s1_2 & 0xFF;
//                     if (var_s3_2 != 0) {
//                         sp18.unkE = (u8) (temp_s0_5 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_5;
//                     }
//                     var_v1_3 = sp50;
//                     sp18.unk4 = 0x52U;
//                     sp18.unk6 = -0x35;
//                     if ((s16) var_v1_3 < 0) {
//                         var_v1_3 = (u16) -(s16) var_v1_3;
//                         var_s3_3 = 1;
//                     } else {
//                         var_s3_3 = 0;
//                     }
//                     do {
//                         temp_s0_6 = var_v1_3 / 10;
//                         temp_s1_3 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_3 + ((s16) (var_v1_3 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_3 = temp_s0_6;
//                         sp18.unkE = temp_s1_3;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_6 << 0x10) != 0);
//                     temp_s0_7 = temp_s1_3 & 0xFF;
//                     if (var_s3_3 != 0) {
//                         sp18.unkE = (u8) (temp_s0_7 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_7;
//                     }
//                     var_v1_4 = sp60;
//                     sp18.unk4 = 0x7FU;
//                     sp18.unk6 = -0x35;
//                     if ((s16) var_v1_4 < 0) {
//                         var_v1_4 = (u16) -(s16) var_v1_4;
//                         var_s3_4 = 1;
//                     } else {
//                         var_s3_4 = 0;
//                     }
//                     do {
//                         temp_s0_8 = var_v1_4 / 10;
//                         temp_s1_4 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_4 + ((s16) (var_v1_4 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_4 = temp_s0_8;
//                         sp18.unkE = temp_s1_4;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_8 << 0x10) != 0);
//                     temp_s0_9 = temp_s1_4 & 0xFF;
//                     if (var_s3_4 != 0) {
//                         sp18.unkE = (u8) (temp_s0_9 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_9;
//                     }
//                     var_v1_5 = unksp43;
//                     sp18.unk4 = 0xAU;
//                     sp18.unk6 = -0x1A;
//                     var_s3_5 = 0;
//                     if ((s32) var_v1_5 < 0) {
//                         var_v1_5 = (u8) -(s32) var_v1_5;
//                         var_s3_5 = 1;
//                     }
//                     do {
//                         temp_s0_10 = var_v1_5 / 10;
//                         temp_s1_5 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_5 + ((s16) (var_v1_5 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_5 = temp_s0_10;
//                         sp18.unkE = temp_s1_5;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_10 << 0x10) != 0);
//                     temp_s0_11 = temp_s1_5 & 0xFF;
//                     if (var_s3_5 != 0) {
//                         sp18.unkE = (u8) (temp_s0_11 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_11;
//                     }
//                     var_v1_6 = unksp41;
//                     sp18.unk4 = 0x28U;
//                     sp18.unk6 = -0x1A;
//                     var_s3_6 = 0;
//                     if ((s32) var_v1_6 < 0) {
//                         var_v1_6 = (u8) -(s32) var_v1_6;
//                         var_s3_6 = 1;
//                     }
//                     do {
//                         temp_s0_12 = var_v1_6 / 10;
//                         temp_s1_6 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_6 + ((s16) (var_v1_6 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_6 = temp_s0_12;
//                         sp18.unkE = temp_s1_6;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_12 << 0x10) != 0);
//                     temp_s0_13 = temp_s1_6 & 0xFF;
//                     if (var_s3_6 != 0) {
//                         sp18.unkE = (u8) (temp_s0_13 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_13;
//                     }
//                     var_v1_7 = (u16) sp52;
//                     sp18.unk4 = 0x52U;
//                     sp18.unk6 = -0x1A;
//                     var_s3_7 = 0;
//                     if ((s16) var_v1_7 < 0) {
//                         var_v1_7 = (u16) -(s16) var_v1_7;
//                         var_s3_7 = 1;
//                     }
//                     do {
//                         temp_s0_14 = var_v1_7 / 10;
//                         temp_s1_7 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_7 + ((s16) (var_v1_7 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_7 = temp_s0_14;
//                         sp18.unkE = temp_s1_7;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_14 << 0x10) != 0);
//                     temp_s0_15 = temp_s1_7 & 0xFF;
//                     if (var_s3_7 != 0) {
//                         sp18.unkE = (u8) (temp_s0_15 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_15;
//                     }
//                     var_v1_8 = sp62;
//                     sp18.unk4 = 0x7FU;
//                     sp18.unk6 = -0x1A;
//                     var_s3_8 = 0;
//                     if ((s16) var_v1_8 < 0) {
//                         var_v1_8 = (u16) -(s16) var_v1_8;
//                         var_s3_8 = 1;
//                     }
//                     do {
//                         temp_s0_16 = var_v1_8 / 10;
//                         temp_s1_8 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_8 + ((s16) (var_v1_8 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_8 = temp_s0_16;
//                         sp18.unkE = temp_s1_8;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_16 << 0x10) != 0);
//                     temp_s0_17 = temp_s1_8 & 0xFF;
//                     if (var_s3_8 != 0) {
//                         sp18.unkE = (u8) (temp_s0_17 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_17;
//                     }
//                     var_v1_9 = unksp42;
//                     sp18.unk4 = 0x1CU;
//                     sp18.unk6 = 1;
//                     var_s3_9 = 0;
//                     if ((s32) var_v1_9 < 0) {
//                         var_v1_9 = (u8) -(s32) var_v1_9;
//                         var_s3_9 = 1;
//                     }
//                     do {
//                         temp_s0_18 = var_v1_9 / 10;
//                         temp_s1_9 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_9 + ((s16) (var_v1_9 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_9 = temp_s0_18;
//                         sp18.unkE = temp_s1_9;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_18 << 0x10) != 0);
//                     temp_s0_19 = temp_s1_9 & 0xFF;
//                     if (var_s3_9 != 0) {
//                         sp18.unkE = (u8) (temp_s0_19 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_19;
//                     }
//                     var_v1_10 = (u16) sp56;
//                     sp18.unk4 = 0x52U;
//                     sp18.unk6 = 1;
//                     var_s3_10 = 0;
//                     if ((s16) var_v1_10 < 0) {
//                         var_v1_10 = (u16) -(s16) var_v1_10;
//                         var_s3_10 = 1;
//                     }
//                     do {
//                         temp_s0_20 = var_v1_10 / 10;
//                         temp_s1_10 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_10 + ((s16) (var_v1_10 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_10 = temp_s0_20;
//                         sp18.unkE = temp_s1_10;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_20 << 0x10) != 0);
//                     temp_s0_21 = temp_s1_10 & 0xFF;
//                     if (var_s3_10 != 0) {
//                         sp18.unkE = (u8) (temp_s0_21 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_21;
//                     }
//                     var_v1_11 = sp66;
//                     sp18.unk4 = 0x7FU;
//                     sp18.unk6 = 1;
//                     var_s3_11 = 0;
//                     if ((s16) var_v1_11 < 0) {
//                         var_v1_11 = (u16) -(s16) var_v1_11;
//                         var_s3_11 = 1;
//                     }
//                     do {
//                         temp_s0_22 = var_v1_11 / 10;
//                         temp_s1_11 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_11 + ((s16) (var_v1_11 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_11 = temp_s0_22;
//                         sp18.unkE = temp_s1_11;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_22 << 0x10) != 0);
//                     temp_s0_23 = temp_s1_11 & 0xFF;
//                     if (var_s3_11 != 0) {
//                         sp18.unkE = (u8) (temp_s0_23 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_23;
//                     }
//                     var_v1_12 = unksp45;
//                     sp18.unk4 = 0x1CU;
//                     sp18.unk6 = 0x1A;
//                     var_s3_12 = 0;
//                     if ((s32) var_v1_12 < 0) {
//                         var_v1_12 = (u8) -(s32) var_v1_12;
//                         var_s3_12 = 1;
//                     }
//                     do {
//                         temp_s0_24 = var_v1_12 / 10;
//                         temp_s1_12 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_12 + ((s16) (var_v1_12 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_12 = temp_s0_24;
//                         sp18.unkE = temp_s1_12;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_24 << 0x10) != 0);
//                     temp_s0_25 = temp_s1_12 & 0xFF;
//                     if (var_s3_12 != 0) {
//                         sp18.unkE = (u8) (temp_s0_25 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_25;
//                     }
//                     var_v1_13 = sp54;
//                     sp18.unk4 = 0x52U;
//                     sp18.unk6 = 0x1A;
//                     var_s3_13 = 0;
//                     if ((s16) var_v1_13 < 0) {
//                         var_v1_13 = (u16) -(s16) var_v1_13;
//                         var_s3_13 = 1;
//                     }
//                     do {
//                         temp_s0_26 = var_v1_13 / 10;
//                         temp_s1_13 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_13 + ((s16) (var_v1_13 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_13 = temp_s0_26;
//                         sp18.unkE = temp_s1_13;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_26 << 0x10) != 0);
//                     temp_s0_27 = temp_s1_13 & 0xFF;
//                     if (var_s3_13 != 0) {
//                         sp18.unkE = (u8) (temp_s0_27 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_27;
//                     }
//                     var_v1_14 = sp64;
//                     sp18.unk4 = 0x7FU;
//                     sp18.unk6 = 0x1A;
//                     var_s3_14 = 0;
//                     if ((s16) var_v1_14 < 0) {
//                         var_v1_14 = (u16) -(s16) var_v1_14;
//                         var_s3_14 = 1;
//                     }
//                     do {
//                         temp_s0_28 = var_v1_14 / 10;
//                         temp_s1_14 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_14 + ((s16) (var_v1_14 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_14 = temp_s0_28;
//                         sp18.unkE = temp_s1_14;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_28 << 0x10) != 0);
//                     temp_s0_29 = temp_s1_14 & 0xFF;
//                     if (var_s3_14 != 0) {
//                         sp18.unkE = (u8) (temp_s0_29 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_29;
//                     }
//                     var_v1_15 = (u16) sp58;
//                     sp18.unk4 = 0x52U;
//                     sp18.unk6 = 0x38;
//                     var_s3_15 = 0;
//                     if ((s16) var_v1_15 < 0) {
//                         var_v1_15 = (u16) -(s16) var_v1_15;
//                         var_s3_15 = 1;
//                     }
//                     do {
//                         temp_s0_30 = var_v1_15 / 10;
//                         temp_s1_15 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_15 + ((s16) (var_v1_15 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_15 = temp_s0_30;
//                         sp18.unkE = temp_s1_15;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_30 << 0x10) != 0);
//                     temp_s0_31 = temp_s1_15 & 0xFF;
//                     if (var_s3_15 != 0) {
//                         sp18.unkE = (u8) (temp_s0_31 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_31;
//                     }
//                     var_v1_16 = (u16) sp68;
//                     sp18.unk4 = 0x7FU;
//                     sp18.unk6 = 0x38;
//                     var_s3_16 = 0;
//                     if ((s16) var_v1_16 < 0) {
//                         var_v1_16 = (u16) -(s16) var_v1_16;
//                         var_s3_16 = 1;
//                     }
//                     do {
//                         temp_s0_32 = var_v1_16 / 10;
//                         temp_s1_16 = sp18.unkE;
//                         sp18.unkE = (u8) (temp_s1_16 + ((s16) (var_v1_16 % 10) * sp18.unk8));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         var_v1_16 = temp_s0_32;
//                         sp18.unkE = temp_s1_16;
//                         sp18.unk4 = (u16) (sp18.unk4 - 0xC);
//                     } while ((temp_s0_32 << 0x10) != 0);
//                     temp_s0_33 = temp_s1_16 & 0xFF;
//                     if (var_s3_16 != 0) {
//                         sp18.unkE = (u8) (temp_s0_33 + (sp18.unk8 * 0xA));
//                         GsSortSprite(&sp18, OTablePt, 0);
//                         sp18.unkE = temp_s0_33;
//                     }
//                     sp74 = -0x19;
//                     sp76 = 0x4E;
//                     var_v0 = rsin((GameClock << 0xC) / 90) * 0x7F;
//                     if (var_v0 < 0) {
//                         var_v0 += 0xFFF;
//                     }
//                     temp_v0_10 = (var_v0 >> 0xC) + 0x7F;
//                     sp86 = temp_v0_10;
//                     sp85 = temp_v0_10;
//                     sp84 = temp_v0_10;
//                     GsSortSprite(&sp70, OTablePt, 1);
//                     if ((s16) sp5A == 4) {
//                         temp_s0_34 = *((*((CHOSEN_STAGE * 2) + &D_8008ED50) * 4) + &ItemImage) + 0x68;
//                         temp_s0_34->unk4 = -0x78;
//                         temp_s0_34->unk6 = 0x38;
//                         temp_s0_34->unk1C = 0x1000;
//                         temp_s0_34->unk1E = 0x1000;
//                         var_v0_2 = rcos((GameClock << 0xC) / 90) * 0x50;
//                         if (var_v0_2 < 0) {
//                             var_v0_2 += 0xFFF;
//                         }
//                         temp_v0_11 = (var_v0_2 >> 0xC) + 0x7F;
//                         temp_s0_34->unk16 = temp_v0_11;
//                         temp_s0_34->unk15 = temp_v0_11;
//                         temp_s0_34->unk14 = temp_v0_11;
//                         GsSortSprite(temp_s0_34, OTablePt, 1);
//                     }
//                     SkipFrame = 2;
//                     EndDrawing(0);
//                     goto loop_19;
//                 }
//             } else {
//                 var_s1 = 2;
//             }
//         } else {
//             var_s1 = 0;
//         }
//         DisposeBG(spBC);
//         vfree(spC0);
//     }
//     FUN_80052ea8(0x80010000, &sp50);
//     FadeOutDirect(0x20, 2, 8, 8, 8);
//     FUN_80038ce0();
//     temp_a1 = &(&CHOSEN_CHARACTER)[CHOSEN_CHARACTER];
//     temp_v1_3 = *((CHOSEN_CHARACTER * 0x1C) + &StageConfig);
//     var_a3_2 = 1;
//     if ((u8) temp_a1->unk60 < temp_v1_3) {
//         temp_a1->unk60 = temp_v1_3;
//     }
//     var_v0_3 = 1 << 0x10;
//     do {
//         temp_a1_2 = var_v0_3 >> 0x10;
//         temp_v1_4 = (CamState.unk10 + temp_a1_2)->unkB4;
//         if (temp_v1_4 == 0xFF) {
//             (&SELECTED_ITEM_COUNTS)[temp_a1_2 + (SELECTED_ITEM_COUNTS << 5)] = temp_v1_4;
//             goto block_124;
//         }
//         var_v0_4 = var_a3_2 << 0x10;
//         if (temp_v1_4 != 0) {
//             temp_a0_2 = &(&SELECTED_ITEM_COUNTS)[temp_a1_2 + (SELECTED_ITEM_COUNTS << 5)];
//             temp_v1_5 = *temp_a0_2;
//             if (temp_v1_5 == 0xFE) {
//                 *temp_a0_2 = temp_v1_5 + 2;
//             }
//             temp_v0_13 = &(&SELECTED_ITEM_COUNTS)[temp_a1_2 + (SELECTED_ITEM_COUNTS << 5)];
//             *temp_v0_13 += (CamState.unk10 + temp_a1_2)->unkB4;
//             temp_v1_6 = &(&SELECTED_ITEM_COUNTS)[temp_a1_2 + (SELECTED_ITEM_COUNTS << 5)];
//             var_v0_4 = var_a3_2 << 0x10;
//             if ((u8) *temp_v1_6 >= 0x64U) {
//                 *temp_v1_6 = 0x63;
// block_124:
//                 var_v0_4 = var_a3_2 << 0x10;
//             }
//         }
//         (&SELECTED_ITEM_COUNTS)[var_v0_4 >> 0x10] = 0;
//         temp_v0_12 = var_a3_2 + 1;
//         var_a3_2 = temp_v0_12;
//         var_v0_3 = var_a3_2 << 0x10;
//     } while (temp_v0_12 < 0x14);
//     var_a0 = 0;
//     do {
//         temp_v1_7 = var_a0 + &SHOP_STOCK_STATE_BY_CHAR;
//         temp_v0_14 = *(var_a0 + (CHOSEN_CHARACTER << 5) + &SHOP_STOCK_STATE_BY_CHAR);
//         var_a0 += 1;
//         *temp_v1_7 = temp_v0_14;
//     } while (var_a0 < 0x14);
//     if (D_8001005C != 0) {
//         if (CHOSEN_STAGE != 7) {
//             FUN_800514d8(var_a0, &SHOP_STOCK_STATE_BY_CHAR, &SELECTED_ITEM_COUNTS, var_a3_2);
//         }
//     }
//     switch (var_s1) {                               /* irregular */
//     case 0:
//         temp_v1_8 = D_80010048;
//         D_80010048 &= 0xFE;
//         if (temp_v1_8 == 7) {
//             var_a0_2 = 0x12;
// block_149:
//             FUN_8004f6c0(var_a0_2);
//         } else {
//             temp_v0_15 = D_80010048;
//             var_a1 = 0;
//             D_80010048 = (&D_8008EA78)[*((D_80010048 * 0x1C) + &StageConfig) + 1];
//             var_a0_3 = (temp_v0_15 * 0x1D4) + ((D_80010048 * 0x24) + 0x80010064);
// loop_141:
//             if ((var_a0_3->unk0 + var_a0_3->unk1) != 0) {
//                 var_a1 += 1;
//                 var_a0_3 += 0xC;
//                 if (var_a1 >= 3) {
//
//                 } else {
//                     goto loop_141;
//                 }
//             }
//             if (var_a1 == 3) {
//                 STAGE_LAYOUT_NUMBER = -1;
//             } else {
//                 STAGE_LAYOUT_NUMBER = var_a1;
//             }
//         }
//         break;
//     case 1:
//         D_80010048 |= 1;
//         break;
//     case 2:
//         STAGE_LAYOUT_NUMBER = -1;
//         var_a0_2 = 0x10;
//         goto block_149;
//     }
//     FUN_8004f6c0(0x11);
// }
