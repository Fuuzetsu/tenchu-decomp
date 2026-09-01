#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "score.h"
#include "misc.h"
#include "images.h"

#define SCORE_ROW_SPACING 0x16

/*
 * Post-mission score/high-score screen (0x80054B48, 0x121C bytes).
 *
 * STATUS: MATCHING — pure C, all 4636 bytes (1159 instructions) exact.
 *
 * The rank and character sprite initializers intentionally share the
 * function-scope `attribute` temporary.  The rank loop updates that value
 * before storing it, while the copied character initializer retains a
 * volatile attribute read whose value is overwritten.  GCC 2.8.1 consequently
 * allocates both loads to v1; making either initializer use a fresh temporary
 * changes the local-allocation quantities and the instruction schedule.
 *
 * The decimal rendering arithmetic is shared again without erasing its
 * allocation donors.  Eight ordinary sites use DRAW_SCORE_NUMBER; the
 * enemy-minus-bosses and row-ordinal sites retain their distinct setup and
 * call only DRAW_SCORE_DIGITS.  Their enclosing do/block shapes remain
 * load-bearing.  MissionScoreSpriteStorage likewise expresses the contiguous
 * result/rank/character stack-bank layout directly.
 * The dead rowScore declaration is also load-bearing: deleting its lexical
 * block lets GCC retain the 0x80010000 row base in s2 and makes the function
 * one instruction short, instead of rematerializing the base like retail.
 * The function-scope `work` short similarly spans the number-sprite shade and
 * the later stage-item index. Replacing its first role with a chained RGB
 * assignment makes the function four bytes short.
 */

typedef struct
{
    u16 oldPad;
    BackGround *background;
} MissionScoreTail;

typedef struct
{
    ScoreResult result;
    u32 rankReserved;
    GsSPRITE rankSprites[N_STAGE_RANKS];
    u32 characterReserved;
    GsSPRITE characterSprites[N_PLAYABLE_CHARACTERS];
} MissionScoreSpriteStorage;

extern u8 CHOSEN_CHARACTER;
extern compact_stage_id CHOSEN_STAGE;
extern u8 CHOSEN_LANGUAGE;
extern u8 STAGE_LAYOUT_NUMBER;

extern char NUMBER_TIM_PATH[];
extern char path_image_3[];  /* K:\\WORK\\CDIMAGE\\IMAGE\\ */
extern char path_font_tim[]; /* font.tim */

extern s16 StageItem[];

extern void vfree(void *ptr);
extern BackGround *load_background_(u_long *tim);
extern short DrawBG(BackGround *bg);
extern void DisposeBG(BackGround *background);
extern void draw_time_(GsSPRITE *number, s32 value, s32 x, s32 y,
                       s32 mode);
extern void FadeOutDirect(s16 time, s16 attribute, u8 r, u8 g, u8 b);
extern void clear_screen_(void);
extern void score_screen_input_(void);
extern void exec_process_(s32 screen);

static inline void InitScoreSprite(u_long *tim, GsIMAGE *image,
                                   GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

#define DRAW_SCORE_DIGITS(sprite_, value_, negative_)                  \
    {                                                                 \
        do                                                            \
        {                                                             \
            dividend = (s16)value_;                                   \
            quotient = dividend / 10;                                 \
            remainder = dividend % 10;                                \
            baseU = sprite_->u;                                        \
            sprite_->u = baseU + (s16)remainder * sprite_->w;          \
            GsSortSprite(sprite_, OTablePt, 0);                        \
            sprite_->x -= 12;                                          \
            value_ = quotient;                                         \
            quotient <<= 16;                                           \
            sprite_->u = baseU;                                        \
        } while (quotient != 0);                                      \
        if (negative_ != 0)                                           \
        {                                                             \
            u32 signBaseU;                                             \
            s32 minusGlyph;                                            \
                                                                      \
            minusGlyph = 10;                                           \
            signBaseU = sprite_->u;                                    \
            sprite_->u = signBaseU + sprite_->w * minusGlyph;          \
            GsSortSprite(sprite_, OTablePt, 0);                        \
            sprite_->u = signBaseU;                                    \
        }                                                             \
    }

#define DRAW_SCORE_COLON(sprite_)                         \
    {                                                     \
        u8 oldU;                                          \
        s32 colonDigit;                                   \
                                                          \
        number.x = 0x1F;                                  \
        oldU = sprite_->u;                                 \
        colonDigit = 12;                                  \
        sprite_->u = oldU + sprite_->w * colonDigit;       \
        GsSortSprite(sprite_, OTablePt, 0);                \
        sprite_->u = oldU;                                 \
    }

#define DRAW_SCORE_NUMBER(sprite_, value_, type_, clear_first_, x_, y_) \
    {                                                               \
        s32 dividend;                                               \
        s32 remainder;                                              \
        s32 quotient;                                               \
        u32 value;                                                  \
        s16 signedValue;                                            \
        s32 drawY;                                                  \
                                                                    \
        value = value_;                                             \
        signedValue = type_ value;                                  \
        drawY = y_;                                                 \
        sprite_->x = x_;                                            \
        sprite_->y = drawY;                                         \
        if (clear_first_)                                           \
        {                                                           \
            negative = 0;                                           \
            if (signedValue < 0)                                    \
            {                                                       \
                value = -signedValue;                               \
                negative = 1;                                       \
            }                                                       \
        }                                                           \
        else if (signedValue < 0)                                   \
        {                                                           \
            value = -signedValue;                                   \
            negative = 1;                                           \
        }                                                           \
        else                                                        \
        {                                                           \
            negative = 0;                                           \
        }                                                           \
        DRAW_SCORE_DIGITS(sprite_, value, negative);                 \
    }

#define SCORE_SPRITE_AT(bank_, index_)                               \
    ((GsSPRITE *)((u8 *)(bank_) + (index_) * sizeof(GsSPRITE)))

/* Round-18 re-collapse: all ten signed-digit tails share DRAW_SCORE_DIGITS,
 * and the eight ordinary sites also share DRAW_SCORE_NUMBER.  The derived
 * enemy count and row ordinal keep their distinct setup; passing the enemy
 * subtraction directly moved 233 assembly lines.  The enclosing do carriers
 * are also deliberate: removing one moves 54-63 lines.  Colon rendering and
 * the four sprite-bank address expressions are shared separately so their
 * differing post-colon assignments and surrounding allocation stay visible. */

/* The persistent high-score block, addressed by CONSTANT rather than through a
 * pointer local.  This is not cosmetic: with a pointer variable the address is
 * `(plus reg_state reg_index)` and expand_binop emits `addu t,state,index`
 * (base first); inlining the constant makes it `(plus reg_index CONST)`, and
 * expand's EXPAND_SUM/form_sum sorts the constant term LAST, emitting
 * `addu t,index,base` -- the target's operand order. */
#define SCORE_STATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)
#define result storage.result
#define rankSprites storage.rankSprites
#define characterSprites storage.characterSprites

void mission_score_screen(void)
{
    GsSPRITE number;
    ScoreStats stats;
    MissionScoreSpriteStorage storage;
    GsIMAGE image;
    MissionScoreTail tail;
    register u_long *tim;
    register u_long *archive;
    register GsSPRITE *initSprite;
    register GsSPRITE *sprite;
    register GsSPRITE *medal;
    register GsSPRITE *medalDraw;
    register TLinkInfo *statePtr;
    register s16 i;
    register s16 newPress;
    s16 work;
    s32 medalBrightness;
    s32 rowBrightness;
    register s32 stageItem;
    register s32 goNext;
    register s32 rankColour;
    u32 baseU;
    register s32 negative;
    s32 insertedRank;
    register s32 resultX;
    u32 attribute;

    tail.oldPad = 0;
    init_score_stats(&stats);
    result = *calculate_score(&stats, CHOSEN_STAGE);
    i = 0;
    rankColour = 128;

    tim = FileRead(NUMBER_TIM_PATH);
    {
        register GsSPRITE *initNumber = &number;

        InitScoreSprite(tim, &image, initNumber);
        initNumber->attribute |= GS_ATTR_SEMITRANS_ADD;
        initNumber->x = -140;
        initNumber->y = -40;
        work = 128;
        initNumber->r = work;
        initNumber->g = work;
        initNumber->b = work;
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

        archive = FileRead(
            MISSION_SCORE_RANK_ARCHIVE_PATHS[CHOSEN_LANGUAGE]);
        attributeMask = GS_ATTR_SEMITRANS_ADD;
    score_rank_sprite_init_loop:
    {
        u32 width;

        tim = get_tim_from_archive(archive, i);
        /* The two-step base+offset walk is byte-required (plain
         * &rankSprites[i] indexing mismatches badly; measured). */
        initSprite = (GsSPRITE *)((u8 *)&storage +
                                  i * sizeof(GsSPRITE));
        initSprite = (GsSPRITE *)((u8 *)initSprite +
                                  sizeof(ScoreResult) + sizeof(u32));
        InitScoreSprite(tim, &image, initSprite);
        attribute = initSprite->attribute;
        initSprite->x = -160;
        initSprite->y = -120;
        width = initSprite->w;
        initSprite->r = rankColour;
        initSprite->g = rankColour;
        initSprite->b = rankColour;
        attribute |= attributeMask;
        initSprite->attribute = attribute;
        initSprite->mx = width >> 1;
        initSprite->my = initSprite->h >> 1;
        SCORE_SPRITE_AT(rankSprites, i)->mx = 0;
        SCORE_SPRITE_AT(rankSprites, i)->my = 0;
        LoadTIM(tim);
    }
        i++;
        if (i < N_STAGE_RANKS)
            goto score_rank_sprite_init_loop;
    }

    {
        register s32 characterColour;

        i = 0;
        characterColour = 128;
    score_character_sprite_init_loop:
    {
        u32 width;
        u32 height;

        tim = get_tim_from_archive(archive, i + RANK_ARCHIVE_RIKIMARU);
        initSprite = (GsSPRITE *)((u8 *)&storage +
                                  i * sizeof(GsSPRITE));
        initSprite = (GsSPRITE *)((u8 *)initSprite +
                                  sizeof(ScoreResult) +
                                  2 * sizeof(u32) +
                                  N_STAGE_RANKS * sizeof(GsSPRITE));
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
        if (i < N_PLAYABLE_CHARACTERS)
            goto score_character_sprite_init_loop;
    }
    vfree(archive);

    tim = FileRead(MISSION_SCORE_BACKGROUND_PATHS[CHOSEN_LANGUAGE]);
    tail.background = load_background_(tim);
    vfree(tim);

    {
        for (i = 0; i < N_HIGH_SCORES; i++)
        {
            if (SCORE_STATE->t_time[i] == 0)
            {
                SCORE_STATE->t_time[i] = SCORE_CLOCK_MAX;
                SCORE_STATE->t_char[i] = RIKIMARU_0;
                SCORE_STATE->t_dani[i] = RANK_THUG;
            }
        }
    }

    insertedRank = -1;
    {
        for (i = 0; i < N_HIGH_SCORES; i++)
        {
            if (SCORE_STATE->t_dani[i] < result.grade)
            {
                insertedRank = i;
                break;
            }
            if (result.grade == SCORE_STATE->t_dani[i] &&
                stats.clock < SCORE_STATE->t_time[i])
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
            i = N_HIGH_SCORES - 1;
            if (found < N_HIGH_SCORES - 1)
            {
                do
                {
                    SCORE_STATE->t_time[i] = SCORE_STATE->t_time[i - 1];
                    SCORE_STATE->t_char[i] = SCORE_STATE->t_char[i - 1];
                    SCORE_STATE->t_dani[i] = SCORE_STATE->t_dani[i - 1];
                    i--;
                } while (insertedRank < i);
            }

            {
                register s32 insertedAt = insertedRank;

                SCORE_STATE->t_time[insertedAt] = stats.clock;
                SCORE_STATE->t_char[insertedAt] = SCORE_STATE->CharType;
                SCORE_STATE->t_dani[insertedAt] = result.grade;
            }
        }
    }

    _PlayMusic(MUSIC_TRACK_COMPLETE, CDA_REPEAT);
    resultX = 0x66;
    for (;;)
    {
        u16 pad;

        pad = GetRealPad(PAD_PORT_1);
        newPress = pad & (pad ^ tail.oldPad);
        tail.oldPad = pad;
        if (newPress & PADRright)
        {
            goNext = 0;
            break;
        }
        if (newPress & PADstart)
        {
            goNext = 1;
            break;
        }

        StartDrawing();
        DrawBG(tail.background);
        draw_time_(&number, stats.clock, 70, -97, 1);

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = &number;
            DRAW_SCORE_NUMBER(drawnSprite, stats.criticals, (s32), 0, 0x16,
                              -0x47);
        } while (0);
        {
            s32 drawX;
            GsSPRITE *numberSprite;

            numberSprite = &number;

            do
            {
                DRAW_SCORE_COLON(numberSprite);
                drawX = 0x2F;
            } while (0);
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s32 signedValue;
                s32 drawY;
                GsSPRITE *drawnSprite;

                drawY = -0x47;
                signedValue = stats.stageEnemies;
                value = stats.stageBosses;
                signedValue -= value;
                numberSprite->x = drawX;
                numberSprite->y = drawY;
                drawnSprite = numberSprite;
                value = (s16)signedValue;
                if (signedValue < 0)
                {
                    value = -signedValue;
                    negative = 1;
                }
                else
                {
                    negative = 0;
                }
                DRAW_SCORE_DIGITS(drawnSprite, value, negative);
            } while (0);
        }
        sprite = &number;
        DRAW_SCORE_NUMBER(sprite, result.criticalScore, (s16), 0, resultX,
                          -0x47);

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = &number;
            DRAW_SCORE_NUMBER(drawnSprite, stats.murders, (s32), 1, 0x16,
                              -0x35);
        } while (0);
        {
            GsSPRITE *numberSprite;
            GsSPRITE *drawnSprite;

            numberSprite = &number;
            DRAW_SCORE_COLON(numberSprite);
            drawnSprite = numberSprite;
            do
                DRAW_SCORE_NUMBER(drawnSprite, stats.stageEnemies, (s32), 1,
                                  0x2F, -0x35)
            while (0);
        }
        {
            GsSPRITE *drawnSprite;

            drawnSprite = &number;
            DRAW_SCORE_NUMBER(drawnSprite, result.murderScore, (s16), 1, resultX,
                              -0x35);
        }

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = &number;
            DRAW_SCORE_NUMBER(drawnSprite, stats.findEnemies, (s32), 1, 0x23,
                              -0x24);
        } while (0);
        {
            GsSPRITE *drawnSprite;

            drawnSprite = &number;
            DRAW_SCORE_NUMBER(drawnSprite, (u16)result.spottedScore, (s16), 1,
                              resultX, -0x24);
        }

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = &number;
            DRAW_SCORE_NUMBER(drawnSprite, (u16)result.score, (s16), 1, resultX,
                              -0x12);
        } while (0);

        if (result.grade == RANK_GRAND_MASTER)
        {
            medal = &ItemImage[StageItem[CHOSEN_STAGE]]->sprite;
            medal->x = 0x8A;
            medal->y = -0xE;
            medal->scalex = FIXED_ONE;
            medal->scaley = FIXED_ONE;
            medalBrightness =
                rcos((GameClock << FIXED_SHIFT) / MEDAL_PULSE_PERIOD) *
                MEDAL_PULSE_AMPLITUDE;
            if (medalBrightness < 0)
            {
                medalDraw = medal;
                medalBrightness += FIXED_TRUNC_BIAS;
            }
            else
            {
                medalDraw = medal;
            }
            medalBrightness =
                (medalBrightness >> FIXED_SHIFT) + 0x7F;
            medalDraw->r = medalDraw->g = medalDraw->b = medalBrightness;
            GsSortSprite(medalDraw, OTablePt, 1);
        }

        {
            GsSPRITE *rankSpriteBase;
            register GsSPRITE *rowSprite;

            i = 0;
            rowSprite = &number;
            /* Allocation carrier: keep rankSpriteBase above the shared divisor. */
            rankSpriteBase = rankSprites;
            /* allocation staging: folded after flow -- not recovered arithmetic */
            rankSpriteBase = (GsSPRITE *)(((u32)rankSpriteBase + (u32)rankSpriteBase) - (u32)rankSpriteBase);
        do
        {
            s32 dividend;
            s32 remainder;
            s32 quotient;
            u16 value;
            s16 signedValue;
            s32 widenedValue;
            s32 drawY;
            register s32 rowNegative;

            signedValue = i + 1;
            value = signedValue;
            drawY = (i * SCORE_ROW_SPACING + 0x18);
            rowSprite->x = -0x8F;
            rowSprite->y = drawY;
            widenedValue = signedValue;
            if (widenedValue < 0)
            {
                value = -widenedValue;
                rowNegative = 1;
            }
            else
            {
                rowNegative = 0;
            }
            DRAW_SCORE_DIGITS(rowSprite, value, rowNegative);
            draw_time_(&number, SCORE_STATE->t_time[i],
                       0x79, i * SCORE_ROW_SPACING + 0x18, 1);
            {
                /* Dead local retained by the row-rendering template. */
                s32 rowScore;
            }

            {
                GsSPRITE *characterSpriteBase = characterSprites;
                TLinkInfo *rowState =
                    (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;

                sprite = &characterSpriteBase[rowState->t_char[i]];
            }
            sprite->x = -0x79;
            sprite->y = i * SCORE_ROW_SPACING + SCORE_ROW_SPACING;
            if (i == insertedRank)
            {
                rowBrightness =
                    rsin((GameClock << FIXED_SHIFT) / MEDAL_PULSE_PERIOD) *
                    ROW_PULSE_AMPLITUDE;
                if (rowBrightness < 0)
                {
                    rowBrightness += FIXED_TRUNC_BIAS;
                }
                rowBrightness = (rowBrightness >> FIXED_SHIFT) + 100;
            }
            else
            {
                rowBrightness = 0x80;
            }
            sprite->r = sprite->g = sprite->b = rowBrightness;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            GsSortSprite(sprite, OTablePt, 1);

            {
                register GsSPRITE *rankSprite =
                    &rankSpriteBase[SCORE_STATE->t_dani[i]];
                rankSprite->r = rankSprite->g = rankSprite->b = 0x7F;
                rankSprite->scalex = rankSprite->scaley = 0xB33;
                rankSprite->x = -0x2F;
                rankSprite->y = i * SCORE_ROW_SPACING + SCORE_ROW_SPACING;
                GsSortSprite(rankSprite, OTablePt, 1);
            }
            i++;
        } while (i < 3);
        }

        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(0);
    }

    if (result.grade == RANK_GRAND_MASTER)
    {
        register TLinkInfo *state =
            (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;

        stageItem = StageItem[state->StageNo];
        work = stageItem;
        if (state->gItem[state->CharType][work] == ITEM_LOCKED)
        {
            /* += 3 on a 0xFE ITEM_LOCKED slot wraps the u8 to 1: the
             * unlock hands the player a single item. */
            state->gItem[state->CharType][work] += 3;
        }
        stageItem = StageItem[state->StageNo];
        if (state->saveItem[stageItem] == ITEM_LOCKED)
        {
            state->saveItem[stageItem] += 3;
        }
    }

    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();
    /* Allocation carrier: keep the persistent-state role ahead of goNext. */
    /* allocation staging: folded after flow -- not recovered arithmetic */
    statePtr = (TLinkInfo *)(TENCHU_PERSISTENT_STATE_ADDRESS + (u32)insertedRank - (u32)insertedRank);
    if (gfMemory != 0)
    {
        LoadTIMAndFree(PathFileRead(path_image_3, path_font_tim));
        score_screen_input_();
    }
    DisposeBG(tail.background);

    if (goNext == 1)
    {
        exec_process_(PROCESS_MAIN);
    }
    else
    {
        statePtr->layout = STAGE_LAYOUT_RANDOM;
        exec_process_(PROCESS_MENU);
    }
}
#undef SCORE_SPRITE_AT
#undef DRAW_SCORE_NUMBER
#undef DRAW_SCORE_COLON
#undef DRAW_SCORE_DIGITS
#undef result
#undef rankSprites
#undef characterSprites
