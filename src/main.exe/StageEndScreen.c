#include "common.h"
#include "main.exe.h"
#include "score.h"
#include "appear.h"
#include "item.h"

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
 *     extern unsigned char gfMemory;
 * END PSX.SYM */

/*
 * Byte-identical C reconstruction (6084 bytes).
 *
 * The five current-score fields deliberately capture x = 0x52 in separate
 * block locals.  GCC 2.8.1 combines those real loop movables into the target's
 * single saved-register constant, after the decimal /10 magic and per-sign
 * ten constants.  A function-wide coordinate initializes too early and
 * produces a different schedule.
 *
 * The score-table character and stage strides remain explicit because a
 * direct three-dimensional subscript changes retail's register allocation.
 */

typedef struct
{
    GsSPRITE digit;                 /* sp+0x18 */
    u8 pad_024[4];
    ScoreStats stats;               /* sp+0x40 */
    u8 pad_04c[4];
    ScoreResult current;            /* sp+0x50 */
    u8 pad_05c[4];
    ScoreResult best;               /* sp+0x60 */
    u8 pad_06c[4];
    GsSPRITE rank;                  /* sp+0x70 */
    u8 pad_07c[4];
    GsIMAGE image;                  /* sp+0x98 */
    u8 pad_0b4[4];
    u16 old_pad;                    /* sp+0xb8 */
    u8 pad_0a2[2];
    BackGround *background;         /* sp+0xbc */
} StageEndScreenStack;

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_STAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern s16 D_8008EA78[];
extern s16 D_8008ED50[];
extern char NUMBER_TIM_PATH[];
extern char *RS_ARCHIVE_PTRS[];
extern char *RANK_ARCHIVE_PTRS[];
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, u8 b);
extern void FUN_80038ce0(void);
extern void mission_score_screen(s32 stage);
extern BackGround *FUN_8004f4f8(u_long *tim);
extern void vfree(void *ptr);
extern void _PlayMusic(s32 music, s32 mode);
extern short DrawBG(BackGround *bg);
extern void FUN_800515b0(GsSPRITE *sprite, s32 value, s32 x, s32 y, s32 mode);
extern void DisposeBG(BackGround *background);
extern void FUN_80052ea8(TLinkInfo *state, ScoreResult *result);
extern void FUN_800514d8(void);
extern void FUN_8004f6c0(s32 state);

static inline void StageEndInitSprite(u_long *tim, GsIMAGE *image,
    GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

static inline s32 StageEndNextStageOffset(s32 index)
{
    index++;
    return index * 2;
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
            u32 sign_base_u;                                               \
            s32 ten;                                                       \
                                                                           \
            ten = 10;                                                      \
            sign_base_u = sprite->u;                                       \
            sprite->u = sign_base_u + sprite->w * ten;                     \
            GsSortSprite(sprite, OTablePt, 0);                              \
            sprite->u = sign_base_u;                                       \
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
            u32 sign_base_u;                                               \
            s32 ten;                                                       \
                                                                           \
            ten = 10;                                                      \
            sign_base_u = sprite->u;                                       \
            sprite->u = sign_base_u + sprite->w * ten;                     \
            GsSortSprite(sprite, OTablePt, 0);                              \
            sprite->u = sign_base_u;                                       \
        }                                                                  \
    }

void StageEndScreen(void)
{
    StageEndScreenStack stack;
    ScoreResult *score;
    ScoreStats *record;
    ScoreStats *layout_record;
    GsSPRITE *icon;
    u_long *tim;
    u_long *rank_archive;
    s16 selection;
    s32 dispatch;
    s32 pulse;
    s32 i;
    s32 layout_index;
    u32 layout_character_offset;
    u32 layout_stage_offset;
    s32 top_y;
    s16 second_x;
    s32 best_x;
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

    item_index = 0;
    while (D_8008EA78[item_index] != CHOSEN_STAGE)
    {
        item_index++;
    }
    {
        TLinkInfo *state;

        state = PSTATE;
        state->mission_flags |= 1 << (item_index - 1);
        if (state->StageNo == 7 && state->language == 0)
        {
            state->mission_flags |= 0x400;
        }
    }

    init_score_stats(&stack.stats);
    score = calculate_score(&stack.stats, CHOSEN_STAGE);
    stack.current = *score;
    PSTATE->score_stats = stack.stats;

    {
        ScoreStats *base_record;
        u32 character_offset;
        u32 stage_offset;
        TLinkInfo *state;

        state = PSTATE;
        character_offset = (u32)state->CharType *
            sizeof(state->stage_stats[0]);
        stage_offset = (u32)state->StageNo *
            sizeof(state->stage_stats[0][0]) +
            (u32)&PSTATE->stage_stats;
        base_record = (ScoreStats *)(character_offset + stage_offset);
        record = base_record + state->layout;
        score = calculate_score(record, state->StageNo);
    }
    stack.best = *score;
    if (stack.current.score > stack.best.score ||
        (stack.current.score == stack.best.score &&
        stack.stats.clock < record->clock))
    {
        *record = stack.stats;
        stack.best = stack.current;
    }

    {
        TStageConfig *config;

        config = &StageConfig[StageID];
        if (config->uid == 0)
        {
            mission_score_screen(StageID);
        }
    }

    {
        /* Preserve the target allocator weight for this reused identity. */
        do
        {
            do
            {
                do
                {
                    do
                    {
                        best_x = TENCHU_PERSISTENT_STATE_ADDRESS;
                    } while (0);
                } while (0);
            } while (0);
        } while (0);
        if (((TLinkInfo *)best_x)->StageNo == 7)
        {
            if (((TLinkInfo *)best_x)->language == 0)
            {
                ((TLinkInfo *)best_x)->mission_flags |= 0x400;
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
                sprite->x = -0x8c;
                sprite->y = -0x28;
                sprite->r = 0x80;
                sprite->g = 0x80;
                sprite->b = 0x80;
                sprite->mx = sprite->w >> 1;
                sprite->my = sprite->h >> 1;
                stack.digit.mx = 0;
                stack.digit.my = 0;
                LoadTIMAndFree(tim);
                top_y = -0x35;
                stack.digit.w = 12;
            }

            tim = FileRead(RS_ARCHIVE_PTRS[((u8 *)best_x)[0x5e]]);
            stack.background = FUN_8004f4f8(tim);
            vfree(tim);
            rank_archive =
                FileRead(RANK_ARCHIVE_PTRS[((u8 *)best_x)[0x5e]]);
            tim = get_tim_from_archive(rank_archive,
                stack.current.grade);
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
            second_x = 0x28;
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
                {
                    s32 dividend;
                    s32 remainder;
                    s32 quotient;
                    u32 value;
                    GsSPRITE *sprite;
                    s32 enemy_count;

                    do
                    {
                        sprite = &stack.digit;
                    } while (0);
                    enemy_count = stack.stats.stageEnemies;
                    i = stack.stats.stageBosses;
                    dispatch = second_x;
                    sprite->x = dispatch;
                    sprite->y = top_y;
                    pulse = enemy_count - i;
                    value = pulse;
                    if (pulse < 0)
                    {
                        value = -pulse;
                        negative = 1;
                        goto number_1;
                    }
                    else
                    {
                        negative = 0;
                    }
number_1:
                    do
                    {
                        dividend = (s16)value;
                        quotient = dividend / 10;
                        remainder = dividend % 10;
                        base_u = sprite->u;
                        sprite->u = base_u + (s16)remainder * sprite->w;
                        GsSortSprite(sprite, OTablePt, 0);
                        sprite->x -= 12;
                        value = quotient;
                        quotient <<= 16;
                        sprite->u = base_u;
                    } while (quotient != 0);
                    if (negative != 0)
                    {
                        u32 sign_base_u;
                        s32 ten;

                        ten = 10;
                        sign_base_u = sprite->u;
                        sprite->u = sign_base_u + sprite->w * ten;
                        GsSortSprite(sprite, OTablePt, 0);
                        sprite->u = sign_base_u;
                    }
                }
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER(stack.current.criticalScore, s16, 1, number_2,
                        x, top_y);
                }
                DRAW_SCORE_NUMBER(stack.best.criticalScore, s16, 1, number_3,
                    best_x, top_y);

                DRAW_SCORE_NUMBER(stack.stats.murders, s32, 0, number_4,
                    10, -0x1a);
                DRAW_SCORE_NUMBER(stack.stats.stageEnemies, s32, 0, number_5,
                    0x28, -0x1a);
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER(stack.current.murderScore, s16, 0, number_6,
                        x, -0x1a);
                }
                DRAW_SCORE_NUMBER(stack.best.murderScore, s16, 0, number_7,
                    best_x, -0x1a);

                DRAW_SCORE_NUMBER(stack.stats.findEnemies, s32, 0, number_8,
                    0x1c, 1);
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER((u16)stack.current.spottedScore,
                        s16, 0, number_9, x, 1);
                }
                DRAW_SCORE_NUMBER((u16)stack.best.spottedScore,
                    s16, 0, number_10, best_x, 1);

                DRAW_SCORE_NUMBER(stack.stats.friendHits, s32, 0, number_11,
                    0x1c, 0x1a);
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER((u16)stack.current.friendPenalty,
                        s16, 0, number_12, x, 0x1a);
                }
                DRAW_SCORE_NUMBER((u16)stack.best.friendPenalty,
                    s16, 0, number_13, best_x, 0x1a);

                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER((u16)stack.current.score,
                        s16, 0, number_14, x, 0x38);
                }
                DRAW_LAST_SCORE_NUMBER((u16)stack.best.score, number_15);

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

                if (stack.current.grade == 4)
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
            vfree(rank_archive);
        }
    }

    FUN_80052ea8(PSTATE, &stack.current);
    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();

    if (PSTATE->StageNoMAX[PSTATE->CharType] <
        StageConfig[PSTATE->StageNo].uid)
    {
        PSTATE->StageNoMAX[PSTATE->CharType] =
            StageConfig[PSTATE->StageNo].uid;
    }

    item_index = 1;
    do
    {
        if (CamState.Owner->item[item_index] == 0xff)
        {
            PSTATE->stock[item_index + (PSTATE->CharType << 5)] = 0xff;
        }
        else if (CamState.Owner->item[item_index] != 0)
        {
            if (PSTATE->stock[item_index + (PSTATE->CharType << 5)] == 0xfe)
            {
                PSTATE->stock[item_index + (PSTATE->CharType << 5)] += 2;
            }
            PSTATE->stock[item_index + (PSTATE->CharType << 5)] +=
                CamState.Owner->item[item_index];
            if (PSTATE->stock[item_index + (PSTATE->CharType << 5)] >= 100)
            {
                PSTATE->stock[item_index + (PSTATE->CharType << 5)] = 99;
            }
        }
        PSTATE->counts[item_index] = 0;
        item_index++;
    } while (item_index < 0x14);

    i = 0;
    do
    {
        PSTATE->backup[i] = PSTATE->stock[i + (CHOSEN_CHARACTER << 5)];
        i++;
    } while (i < 0x14);

    if (gfMemory != 0)
    {
        dispatch = TENCHU_PERSISTENT_STATE_ADDRESS;
        dispatch = *(volatile u8 *)(dispatch + 5);
        if (dispatch != 7)
        {
            FUN_800514d8();
        }
    }

    dispatch = selection;
    switch (dispatch)
    {
    case 0:
        PSTATE->flags48 &= 0xfe;
        if (PSTATE->StageNo == 7)
        {
            FUN_8004f6c0(0x12);
        }
        else
        {
            u8 *next_stage;
            u32 layout_base;

            layout_base = (u32)&PSTATE->stage_stats;
            next_stage = (u8 *)D_8008EA78;
            PSTATE->StageNo = next_stage[StageEndNextStageOffset(
                StageConfig[PSTATE->StageNo].uid)];
            layout_character_offset = (u32)PSTATE->CharType *
                sizeof(PSTATE->stage_stats[0]);
            layout_stage_offset =
                (u32)PSTATE->StageNo *
                sizeof(PSTATE->stage_stats[0][0]) + layout_base;
            layout_record = (ScoreStats *)(layout_character_offset +
                layout_stage_offset);
            layout_index = 0;
layout_loop:
            if (layout_record->stageBosses + layout_record->stageEnemies == 0)
                goto layout_done;
            layout_index++;
            if (layout_index < 3)
            {
                layout_record++;
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
        PSTATE->flags48 |= 1;
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
