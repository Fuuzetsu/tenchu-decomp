#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "model.h"
#include "tim.h"
#include "graphics.h"
#include "effect.h"
#include "score.h"
#include "appear.h"
#include "item.h"
#include "images.h"
#include "vmemory.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern u8 STAGE_LAYOUT_NUMBER;
extern char NUMBER_TIM_PATH[];

static inline void StageEndInitSprite(u_long *tim, GsIMAGE *image,
                                      GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

#define DRAW_SCORE_NUMBER(value_, type_, clear_first_,     \
                          x_, y_)                            \
    {                                                        \
        s32 dividend;                                        \
        s32 remainder;                                       \
        s32 quotient;                                        \
        u32 value;                                           \
        s16 signed_value;                                    \
        GsSPRITE *sprite;                                    \
                                                             \
        sprite = &digit;                                     \
        value = (value_);                                    \
        signed_value = (type_)value;                         \
        sprite->x = (x_);                                    \
        sprite->y = (y_);                                    \
        if (clear_first_)                                    \
        {                                                    \
            negative = 0;                                    \
            if (signed_value < 0)                            \
            {                                                \
                value = -signed_value;                       \
                negative = 1;                                \
            }                                                \
        }                                                    \
        else if (signed_value < 0)                           \
        {                                                    \
            value = -signed_value;                           \
            negative = 1;                                    \
        }                                                    \
        else                                                 \
        {                                                    \
            negative = 0;                                    \
        }                                                    \
        do                                                   \
        {                                                    \
            dividend = (s16)value;                           \
            quotient = dividend / 10;                        \
            remainder = dividend % 10;                       \
            base_u = sprite->u;                              \
            sprite->u = base_u + (s16)remainder * sprite->w; \
            GsSortSprite(sprite, OTablePt, 0);               \
            sprite->x -= 12;                                 \
            value = quotient;                                \
            quotient <<= 16;                                 \
            sprite->u = base_u;                              \
        } while (quotient != 0);                             \
        if (negative != 0)                                   \
        {                                                    \
            u32 sign_base_u;                                 \
            s32 divisor;                                         \
                                                             \
            divisor = 10;                                        \
            sign_base_u = sprite->u;                         \
            sprite->u = sign_base_u + sprite->w * divisor;       \
            GsSortSprite(sprite, OTablePt, 0);               \
            sprite->u = sign_base_u;                         \
        }                                                    \
    }

#define DRAW_LAST_SCORE_NUMBER(value_)                 \
    {                                                  \
        s32 dividend;                                  \
        s32 remainder;                                 \
        s32 quotient;                                  \
        u32 value;                                     \
        s16 signed_value;                              \
        u8 base_u;                                     \
        GsSPRITE *sprite;                              \
                                                       \
        sprite = &digit;                               \
        value = (value_);                              \
        signed_value = (s16)value;                     \
        sprite->x = best_x;                            \
        sprite->y = 0x38;                              \
        negative = 0;                                  \
        if (signed_value < 0)                          \
        {                                              \
            value = -signed_value;                     \
            negative = 1;                              \
        }                                              \
        do                                             \
        {                                              \
            dividend = (s16)value;                     \
            quotient = dividend / 10;                  \
            remainder = dividend % 10;                 \
            sprite->u = (s16)remainder * sprite->w +   \
                        (base_u = sprite->u);          \
            GsSortSprite(sprite, OTablePt, 0);         \
            sprite->x -= 12;                           \
            value = quotient;                          \
            quotient <<= 16;                           \
            sprite->u = base_u;                        \
        } while (quotient != 0);                       \
        if (negative != 0)                             \
        {                                              \
            u32 sign_base_u;                           \
            s32 divisor;                                   \
                                                       \
            divisor = 10;                                  \
            sign_base_u = sprite->u;                   \
            sprite->u = sign_base_u + sprite->w * divisor; \
            GsSortSprite(sprite, OTablePt, 0);         \
            sprite->u = sign_base_u;                   \
        }                                              \
    }

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

void StageEndScreen(void)
{
    GsSPRITE digit;
    ScoreStats stats;
    ScoreResult current, best;
    GsSPRITE rank;
    GsIMAGE image;
    struct
    {
        u16 old_pad;
        BackGround *background;
    } ui;
    ScoreResult *score;
    ScoreStats *record;
    ScoreStats *layout_record;
    GsSPRITE *icon;
    u_long *tim;
    ArcFile *rank_archive;
    enum
    {
        /* What the player picked on the stage-end screen. */
        STAGE_END_ADVANCE = 0, /* next stage, or the ending after the last */
        STAGE_END_REPLAY = 1,  /* raise GAME_RETRY_REPLAY and run it again */
        STAGE_END_QUIT = 2     /* back to the main menu */
    };
    s16 selection;
    s32 work;
    s32 pulse;
    s32 i;
    s32 layout_index;
    s32 top_y;
    s16 second_x;
    s32 best_x;
    s16 item_index;
    u16 pad;
    u16 pressed;
    u32 base_u;
    s32 negative;

    selection = STAGE_END_ADVANCE;
    ui.old_pad = 0;
    SetupAppearance(RIKIMARU_0, APPEARANCE_STAGE_NONE);
    PadShockAR(PAD_PORT_1, RUMBLE_POWER_OFF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_NONE);
    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();

    item_index = 0;
    while (StageOrder[item_index] != CHOSEN_STAGE)
    {
        item_index++;
    }
    {
        TLinkInfo *state;

        state = PSTATE;
        state->mission_flags |= MISSION_COMPLETION_FLAG(item_index);
        /* StageConfig id 7 is campaign uid 10, the final mission.  Nothing
         * in main.exe reads the language-specific flag back, so its eventual
         * effect remains unknown. */
        if (state->StageNo == STAGE_ID_FREE_PRINCESS &&
            state->language == LANG_ENGLISH)
        {
            state->mission_flags |= MISSION_FLAG_ENGLISH_FINAL_STAGE;
        }
    }

    init_score_stats(&stats);
    score = calculate_score(&stats, CHOSEN_STAGE);
    current = *score;
    PSTATE->score_stats = stats;

    {
        ScoreStats *base_record;
        u32 character_byte_offset;
        ScoreStats *stage_record;
        ScoreStats (*records)[N_STAGE_SCORE_SLOTS][N_STAGE_LAYOUTS];
        TLinkInfo *state;

        state = PSTATE;
        records = state->stage_stats;
        character_byte_offset = state->CharType * sizeof(*records);
        stage_record = records[0][state->StageNo];
        base_record = (ScoreStats *)(character_byte_offset +
                                     (u32)stage_record);
        record = base_record + state->layout;
        score = calculate_score(record, state->StageNo);
    }
    best = *score;
    if (current.score > best.score ||
        (current.score == best.score &&
         stats.clock < record->clock))
    {
        *record = stats;
        best = current;
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
        do
        {
            best_x = TENCHU_PERSISTENT_STATE_ADDRESS;
        } while (0);
        if (((TLinkInfo *)best_x)->StageNo == STAGE_ID_FREE_PRINCESS)
        {
            do
            {
                if (((TLinkInfo *)best_x)->language == LANG_ENGLISH)
                {
                    ((TLinkInfo *)best_x)->mission_flags |=
                        MISSION_FLAG_ENGLISH_FINAL_STAGE;
                }
            } while (0);
        }
        else
        {
            tim = FileRead(NUMBER_TIM_PATH);
            {
                GsSPRITE *sprite;

                sprite = &digit;
                StageEndInitSprite(tim, &image, sprite);
                sprite->attribute |= GS_ATTR_SEMITRANS_ADD;
                sprite->x = -0x8c;
                sprite->y = -0x28;
                sprite->r = 0x80;
                sprite->g = 0x80;
                sprite->b = 0x80;
                sprite->mx = sprite->w >> 1;
                sprite->my = sprite->h >> 1;
                digit.mx = 0;
                digit.my = 0;
                LoadTIMAndFree(tim);
                top_y = -0x35;
                digit.w = 12;
            }

            tim = FileRead(STAGE_RESULT_BACKGROUND_PATHS[
                ((TLinkInfo *)best_x)->language]);
            ui.background = load_background_(tim);
            vfree(tim);
            rank_archive =
                (ArcFile *)FileRead(STAGE_RESULT_RANK_ARCHIVE_PATHS[
                    ((TLinkInfo *)best_x)->language]);
            tim = get_tim_from_archive(rank_archive,
                                       current.grade);
            best_x = 0x7f;
            StageEndInitSprite(tim, &image, &rank);
            rank.x = -160;
            rank.y = -120;
            rank.r = 0x80;
            rank.g = 0x80;
            rank.b = 0x80;
            rank.attribute |= GS_ATTR_SEMITRANS_ADD;
            rank.mx = rank.w >> 1;
            rank.my = rank.h >> 1;
            rank.mx = 0;
            rank.my = 0;
            LoadTIM(tim);

            _PlayMusic(MUSIC_TRACK_COMPLETE, CDA_REPEAT);
            second_x = 0x28;
            while (1)
            {
                pad = GetRealPad(PAD_PORT_1);
                pressed = pad & (pad ^ ui.old_pad);
                ui.old_pad = pad;
                if ((pressed & PADRright) != 0)
                {
                    selection = STAGE_END_ADVANCE;
                    break;
                }
                if ((pressed & PADRdown) != 0)
                {
                    /* Retail keeps identical switch arms here; their original distinction is unknown. */
                    switch (!!pad)
                    {
                    case STAGE_END_ADVANCE:
                        selection = STAGE_END_QUIT;
                        break;
                    default:
                        selection = STAGE_END_QUIT;
                        break;
                    }
                    break;
                }
                selection = STAGE_END_REPLAY;
                if ((pressed & PADstart) != 0)
                {
                    break;
                }

                StartDrawing();
                DrawBG(ui.background);
                draw_time_(&digit, stats.clock, 97, -93, 0);
                DRAW_SCORE_NUMBER(stats.criticals, s32, 0, 10, top_y);
                {
                    s32 dividend;
                    s32 remainder;
                    s32 quotient;
                    u32 value;
                    GsSPRITE *sprite;
                    s32 enemy_count;

                    sprite = &digit;
                    /* Empty loop retained for code layout; its original source construct is unknown. */
                    do
                    {
                    } while (0);
                    enemy_count = stats.stageEnemies;
                    i = stats.stageBosses;
                    work = second_x;
                    sprite->x = work;
                    sprite->y = top_y;
                    pulse = enemy_count - i;
                    value = pulse;
                    if (pulse < 0)
                    {
                        value = -pulse;
                        negative = 1;
                    }
                    else
                    {
                        negative = 0;
                    }
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
                        s32 divisor;

                        divisor = 10;
                        sign_base_u = sprite->u;
                        sprite->u = sign_base_u + sprite->w * divisor;
                        GsSortSprite(sprite, OTablePt, 0);
                        sprite->u = sign_base_u;
                    }
                }
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER(current.criticalScore, s16, 0, x, top_y);
                }
                DRAW_SCORE_NUMBER(best.criticalScore, s16, 0, best_x,
                                  top_y);

                DRAW_SCORE_NUMBER(stats.murders, s32, 1, 10, -0x1a);
                DRAW_SCORE_NUMBER(stats.stageEnemies, s32, 1, 0x28, -0x1a);
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER(current.murderScore, s16, 1, x, -0x1a);
                }
                DRAW_SCORE_NUMBER(best.murderScore, s16, 1, best_x,
                                  -0x1a);

                DRAW_SCORE_NUMBER(stats.findEnemies, s32, 1, 0x1c, 1);
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER((u16)current.spottedScore, s16, 1, x, 1);
                }
                DRAW_SCORE_NUMBER((u16)best.spottedScore, s16, 1,
                                  best_x, 1);

                DRAW_SCORE_NUMBER(stats.friendHits, s32, 1, 0x1c, 0x1a);
                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER((u16)current.friendPenalty, s16, 1, x, 0x1a);
                }
                DRAW_SCORE_NUMBER((u16)best.friendPenalty, s16, 1,
                                  best_x, 0x1a);

                {
                    s32 x;

                    x = 0x52;
                    DRAW_SCORE_NUMBER((u16)current.score, s16, 1, x, 0x38);
                }
                DRAW_LAST_SCORE_NUMBER((u16)best.score);

                rank.x = -25;
                rank.y = 78;
                pulse = rsin((GameClock << FIXED_SHIFT) / 90) * 0x7f;
                /* Empty loop retained for code layout; its original source construct is unknown. */
                do
                {
                } while (0);
                if (pulse < 0)
                {
                    pulse += FIXED_TRUNC_BIAS;
                }
                rank.r = rank.g = rank.b =
                    (pulse >> FIXED_SHIFT) + 0x7f;
                GsSortSprite(&rank, OTablePt, 1);

                if (current.grade == RANK_GRAND_MASTER)
                {
                    icon = &ItemImage[StageItem[CHOSEN_STAGE]]->sprite;
                    icon->x = -120;
                    icon->y = 0x38;
                    icon->scalex = FIXED_ONE;
                    icon->scaley = FIXED_ONE;
                    pulse =
                        rcos((GameClock << FIXED_SHIFT) / 90) * 0x50;
                    icon->r = icon->g = icon->b = (pulse / FIXED_ONE) + 0x7f;
                    GsSortSprite(icon, OTablePt, 1);
                }

                SkipFrame = SKIPFRAME_AFTER_LOAD;
                EndDrawing(0);
            }

            DisposeBG(ui.background);
            vfree(rank_archive);
        }
    }

    award_stage_items_(PSTATE, &current);
    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();

    if (PSTATE->StageNoMAX[PSTATE->CharType] <
        StageConfig[PSTATE->StageNo].uid)
    {
        PSTATE->StageNoMAX[PSTATE->CharType] =
            StageConfig[PSTATE->StageNo].uid;
    }

    item_index = ITEM_SHURIKEN;
    do
    {
        if (CamState.Owner->item[item_index] == ITEM_INFINITE)
        {
            PSTATE->gItem[PSTATE->CharType][item_index] = ITEM_INFINITE;
        }
        else if (CamState.Owner->item[item_index] != 0)
        {
            if (PSTATE->gItem[PSTATE->CharType][item_index] == ITEM_LOCKED)
            {
                PSTATE->gItem[PSTATE->CharType][item_index] += 2;
            }
            PSTATE->gItem[PSTATE->CharType][item_index] +=
                CamState.Owner->item[item_index];
            if (PSTATE->gItem[PSTATE->CharType][item_index] >= 100)
            {
                PSTATE->gItem[PSTATE->CharType][item_index] = 99;
            }
        }
        PSTATE->selItem[item_index] = 0;
        item_index++;
    } while (item_index < N_LOADOUT_ITEMS);

    i = 0;
    do
    {
        PSTATE->saveItem[i] = PSTATE->gItem[CHOSEN_CHARACTER][i];
        i++;
    } while (i < N_LOADOUT_ITEMS);

    work = TENCHU_PERSISTENT_STATE_ADDRESS;
    if (gfMemory != 0 &&
        ((TLinkInfo *)work)->StageNo != STAGE_ID_FREE_PRINCESS)
    {
        score_screen_input_();
    }

    switch (selection)
    {
    case STAGE_END_ADVANCE:
        PSTATE->GameRetry &= (u8)~GAME_RETRY_REPLAY;
        if (PSTATE->StageNo == STAGE_ID_FREE_PRINCESS)
        {
            exec_process_(PROCESS_ENDING);
        }
        else
        {
            PSTATE->StageNo =
                StageOrder[NEXT_STAGE_UID(StageConfig[PSTATE->StageNo].uid)];
            layout_record = &PSTATE->stage_stats[PSTATE->CharType]
                                                [PSTATE->StageNo][0];
            layout_index = 0;
            while (1)
            {
                if (layout_record->stageBosses + layout_record->stageEnemies != 0)
                {
                    layout_index++;
                    if (layout_index < N_STAGE_LAYOUTS)
                    {
                        layout_record++;
                        continue;
                    }
                }
                break;
            }
            if (layout_index == N_STAGE_LAYOUTS)
            {
                STAGE_LAYOUT_NUMBER = STAGE_LAYOUT_RANDOM;
            }
            else
            {
                STAGE_LAYOUT_NUMBER = layout_index;
            }
        }
        break;
    case STAGE_END_REPLAY:
        PSTATE->GameRetry |= GAME_RETRY_REPLAY;
        break;
    case STAGE_END_QUIT:
        STAGE_LAYOUT_NUMBER = STAGE_LAYOUT_RANDOM;
        exec_process_(PROCESS_MENU);
        break;
    }

    exec_process_(PROCESS_MAIN);
}

#undef DRAW_SCORE_NUMBER
#undef DRAW_LAST_SCORE_NUMBER
#undef PSTATE
