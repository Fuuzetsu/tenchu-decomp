#include "common.h"
#include "ending.h"
#include "frontend.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/memcard.h"
#include "../main.exe/score.h"
#include "../main.exe/stage.h"
#include "../main.exe/sound.h"
#include "../main.exe/tuning.h"
#include "../main.exe/vmemory.h"

extern char path_ending_numbers[];
extern char *EndingResultBackgroundPaths[N_LANGUAGES];
extern char *EndingRankArchivePaths[N_LANGUAGES];

short RunEndingResults(void)
{
    GsSPRITE number;
    ScoreResult result;
    ScoreResult best;
    GsSPRITE rank;
    POLY_FT4 reward;
    ArcFile *archive;
    u_long *tim;
    BackGround *background;
    ScoreStats *stats;
    short phase = 1;
    short brightness = 128;
    short selection;

    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsInit3D();
    stats = &ENDING_STATE->score_stats;
    ClearEndingPages();
    DrawSync(0);
    PadShockAR(0, 0, 0, 0);
    SetupSoundEffect(0, 9);
    result = *calculate_score(&ENDING_STATE->score_stats, ENDING_STATE->StageNo);
    best = *calculate_score(
        &ENDING_STATE->stage_stats[ENDING_STATE->CharType]
                                 [ENDING_STATE->StageNo][ENDING_STATE->layout],
        ENDING_STATE->StageNo);
    if (result.grade == RANK_GRAND_MASTER)
        SetupImageToPolyFT4(GetImage(IMG_ICON_MANEBUE), &reward, -128, 49);

    tim = FileRead(path_ending_numbers);
    InitEndingScoreSprite(&number, tim, 20, 80, 128, GS_ATTR_SEMITRANS_ADD);
    LoadTIMAndFree(tim);
    number.w = 12;
    background = load_background_(
        FileRead(EndingResultBackgroundPaths[ENDING_STATE->language]));
    archive = (ArcFile *)FileRead(EndingRankArchivePaths[ENDING_STATE->language]);
    tim = get_tim_from_archive(archive, result.grade);
    InitEndingScoreSprite(&rank, tim, 0, 0, 128, GS_ATTR_SEMITRANS_ADD);
    LoadTIM(tim);
    DrawEndingBackground(128);
    _PlayMusic(12, 1);

    do
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            brightness -= 4;
            if (brightness <= 0)
            {
                phase++;
                brightness = 0;
            }
            DrawEndingBackground(brightness);
            break;
        case 2:
            EndingPreviousButtons = EndingButtons;
            EndingButtons = GetRealPad(0);
            EndingPreviousButtons =
                EndingButtons & (EndingButtons ^ EndingPreviousButtons);
            if (EndingPreviousButtons & PADRright)
            {
                selection = 0;
                phase++;
            }
            if (EndingPreviousButtons & PADstart)
            {
                selection = 1;
                phase++;
            }
            break;
        case 3:
            brightness += 4;
            if (brightness >= 128)
            {
                phase++;
                brightness = 128;
            }
            DrawEndingBackground(brightness);
            break;
        }
        DrawBG(background);
        DrawEndingTime(&number, stats->clock, 97, -93);
        DrawEndingNumber(&number, stats->criticals, 10, -53);
        DrawEndingNumber(&number, stats->stageEnemies - stats->stageBosses,
                         40, -53);
        DrawEndingNumber(&number, result.criticalScore, 82, -53);
        DrawEndingNumber(&number, best.criticalScore, 127, -53);
        DrawEndingNumber(&number, stats->murders, 10, -26);
        DrawEndingNumber(&number, stats->stageEnemies, 40, -26);
        DrawEndingNumber(&number, result.murderScore, 82, -26);
        DrawEndingNumber(&number, best.murderScore, 127, -26);
        DrawEndingNumber(&number, stats->findEnemies, 28, 1);
        DrawEndingNumber(&number, result.spottedScore, 82, 1);
        DrawEndingNumber(&number, best.spottedScore, 127, 1);
        DrawEndingNumber(&number, stats->friendHits, 28, 27);
        DrawEndingNumber(&number, result.friendPenalty, 82, 27);
        DrawEndingNumber(&number, best.friendPenalty, 127, 27);
        DrawEndingNumber(&number, result.score, 82, 57);
        DrawEndingNumber(&number, best.score, 127, 57);

        rank.x = -29;
        rank.y = 78;
        rank.r = rank.g = rank.b =
            (rsin((GameClock << 12) / 90) * 127) / 4096 + 127;
        GsSortSprite(&rank, OTablePt, ENDING_RESULTS_PRIORITY);
        if (result.grade == RANK_GRAND_MASTER)
        {
            reward.r0 = reward.g0 = reward.b0 =
                ((rcos((GameClock << 12) / 90) * 80) >> 12) + 127;
            GsSortPoly(&reward, OTablePt, ENDING_RESULTS_PRIORITY);
        }
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(0);
    } while (phase != 4);

    DisposeBG(background);
    vfree(archive);
    if (gfMemory)
        SaveEndingResults();
    return selection;
}

/* Identical score rules to MENU, including the training-stage penalty. */
extern ScoreResult STAGE_SCORE_COMPONENTS;

ScoreResult *calculate_score(ScoreStats *stats, packed_stage_id stage)
{
    ScoreResult *result;
    s32 score;
    s32 stealth_base;
    u8 spots;

    STAGE_SCORE_COMPONENTS.criticalScore = stats->criticals * SCORE_PER_CRITICAL;
    STAGE_SCORE_COMPONENTS.murderScore = stats->murders * SCORE_PER_MURDER;
    STAGE_SCORE_COMPONENTS.friendPenalty = stats->friendHits * SCORE_PER_FRIEND_HIT;

    spots = stats->findEnemies;
    stealth_base = STEALTH_BASE;

    if (spots != 0)
    {
        stealth_base = STEALTH_BASE_SEEN;
    }
    /* Training doubles the per-spot penalty.  `stage` is a StageConfig id,
     * not the campaign uid whose value 8 names Cure the Princess. */
    if (stage == STAGE_ID_TRAINING)
    {
        STAGE_SCORE_COMPONENTS.spottedScore =
            stealth_base - spots * (SPOT_PENALTY * 2);
    }
    else
    {
        STAGE_SCORE_COMPONENTS.spottedScore = stealth_base - spots * SPOT_PENALTY;
    }
    result = &STAGE_SCORE_COMPONENTS;
    if (result->spottedScore < 0)
    {
        result->spottedScore = 0;
    }

    result->score = STAGE_SCORE_COMPONENTS.criticalScore + result->murderScore +
                    result->friendPenalty + (u16)result->spottedScore;
    score = result->score;
    if (score < 0)
    {
        score = 0;
    }
    result->grade = score / SCORE_PER_GRADE;
    if (result->grade > RANK_GRAND_MASTER)
    {
        result->grade = RANK_GRAND_MASTER;
    }

    if (stats->stageBosses + stats->stageEnemies == 0)
    {
        memset(result, 0, sizeof(*result));
    }
    return result;
}
#define DRAW_DECIMAL_STRIP(sprite, value, quotient, base_u)             \
    do                                                               \
    {                                                                \
        (quotient) = (value) / 10;                                    \
        (base_u) = (sprite)->u;                                       \
        (sprite)->u = (base_u) + ((value) % 10) * (sprite)->w;          \
        GsSortSprite((sprite), OTablePt, ENDING_RESULTS_PRIORITY);     \
        (value) = (quotient);                                         \
        (sprite)->u = (base_u);                                       \
        (sprite)->x -= 12;                                            \
    } while ((quotient) != 0)

/* Draw a minutes/seconds time value from a digit sprite. */
void DrawEndingTime(GsSPRITE *sprite, s32 time, s32 x, s32 y)
{
    s16 value;
    s32 signedValue;
    s16 quotient;
    s32 negative;
    s32 lastNegative;
    s16 multiplier;
    s32 signBase;
    u8 baseU;

    time /= 30;
    value = time / 60;
    sprite->y = y;
    sprite->x = x - 32;
    signedValue = value;
    if (signedValue < 0)
    {
        value = -signedValue;
        negative = 1;
    }
    else
    {
        negative = 0;
    }

    DRAW_DECIMAL_STRIP(sprite, value, quotient, baseU);

    if (negative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, ENDING_RESULTS_PRIORITY);
        sprite->u = signBase;
    }

    time %= 60;
    sprite->y = y;
    sprite->x = x - 12;
    value = time / 10;
    signedValue = value;
    negative = 0;
    if (signedValue < 0)
    {
        value = -signedValue;
        negative = 1;
    }

    DRAW_DECIMAL_STRIP(sprite, value, quotient, baseU);

    if (negative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, ENDING_RESULTS_PRIORITY);
        sprite->u = signBase;
    }

    sprite->x = x;
    value = time % 10;
    signedValue = value;
    sprite->y = y;
    if (signedValue < 0)
    {
        value = -signedValue;
        lastNegative = 1;
    }
    else
    {
        lastNegative = 0;
    }

    DRAW_DECIMAL_STRIP(sprite, value, quotient, baseU);

    if (lastNegative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, ENDING_RESULTS_PRIORITY);
        sprite->u = signBase;
    }

}

#undef DRAW_DECIMAL_STRIP

void DrawEndingNumber(GsSPRITE *sp, s16 number, s16 x, s16 y)
{
    u8 u;
    bool neg;
    s16 value;
    s32 signed_value;
    s16 quotient;
    s16 multiplier;
    s32 width;
    s32 sign_base;

    value = number;
    signed_value = number;
    sp->x = x;
    sp->y = y;
    if (signed_value < 0)
    {
        value = -signed_value;
        neg = true;
    }
    else
    {
        neg = false;
    }
    do
    {
        quotient = value / 10;
        u = sp->u;
        sp->u += (value % 10) * sp->w;
        GsSortSprite(sp, OTablePt, ENDING_RESULTS_PRIORITY);
        value = quotient;
        sp->u = u;
        sp->x -= 12;
    } while (quotient != 0);
    sign_base = u & 0xff;
    if (neg)
    {
        width = sp->w;
        multiplier = 10;
        sp->u = sign_base + multiplier * width;
        GsSortSprite(sp, OTablePt, ENDING_RESULTS_PRIORITY);
        sp->u = sign_base;
    }
}

void InitEndingScoreSprite(GsSPRITE *sprite, u_long *tim, int x, int y,
                           int brightness, u_long attributes)
{
    GsIMAGE image;

    GetTIMInfo(tim, &image);
    InitSprite(&image, sprite);
    sprite->x = x - SCREEN_W / 2;
    sprite->y = y - SCREEN_H / 2;
    sprite->mx = 0;
    sprite->my = 0;
    sprite->attribute |= attributes;
    sprite->r = brightness;
    sprite->g = brightness;
    sprite->b = brightness;
}

void SaveEndingResults(void)
{
    /* Retail leaves the initial held-button value uninitialized. After the
     * first poll, the save UI receives a press only after complete release. */
    int buttons;
    int previous;
    int pressed;

    ENDING_STATE->StageNoMAX[ENDING_STATE->CharType] = STAGE_UID_FREE_PRINCESS;
    do
    {
        StartDrawing();
        previous = buttons;
        buttons = GetRealPad(0);
        pressed = 0;
        if (previous == 0 && buttons != 0)
            pressed = buttons;
        pressed = update_card_screen_(pressed);
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(2);
    } while (pressed == 0);
}

/* ENDING string storage, in retail object order. */
char path_ending_numbers[] = "K:\\WORK\\CDIMAGE\\DEMO\\number.tim";
char ending_result_japanese[] = "K:\\WORK\\CDIMAGE\\DEMO\\rs_e_jap.tim";
char ending_result_italian[] = "K:\\WORK\\CDIMAGE\\DEMO\\rs_e_ita.tim";
char ending_result_french[] = "K:\\WORK\\CDIMAGE\\DEMO\\rs_e_fre.tim";
char ending_result_english[] = "K:\\WORK\\CDIMAGE\\DEMO\\rs_e_eng.tim";
char ending_rank_japanese[] = "K:\\WORK\\CDIMAGE\\DEMO\\Ranks_j.Arc";
char ending_rank_italian[] = "K:\\WORK\\CDIMAGE\\DEMO\\Ranks_i.Arc";
char ending_rank_french[] = "K:\\WORK\\CDIMAGE\\DEMO\\Ranks_f.Arc";
char ending_rank_english[] = "K:\\WORK\\CDIMAGE\\DEMO\\Ranks_e.Arc";
