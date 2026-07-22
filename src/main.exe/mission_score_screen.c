#include "common.h"
#include "main.exe.h"
#include "misc.h"

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
 * The decimal rendering sites remain expanded because their ordinary
 * per-site block structure and local divisor temporaries determine loop-depth
 * weighting and rematerialization.  MissionScoreSpriteStorage likewise
 * expresses the contiguous result/rank/character stack-bank layout directly.
 * The dead rowScore declaration is also load-bearing: deleting its lexical
 * block lets GCC retain the 0x80010000 row base in s2 and makes the function
 * one instruction short, instead of rematerializing the base like retail.
 */

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
    BackGround *background;
} MissionScoreTail;

typedef struct
{
    ScoreResult result;
    u32 rankReserved;
    GsSPRITE rankSprites[5];
    u32 characterReserved;
    GsSPRITE characterSprites[2];
} MissionScoreSpriteStorage;

extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_STAGE;
extern u8 CHOSEN_LANGUAGE;
extern u8 STAGE_LAYOUT_NUMBER;

extern char NUMBER_TIM_PATH[];
extern char *RANKS_ARCHIVE_PTRS[];
extern char *TRN_SPRITE_PTRS[];
extern char D_80013AA8[];
extern char D_80013AC0[];
extern u8 gfMemory;

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
extern void vfree(void *ptr);
extern u_long *get_tim_from_archive(u_long *archive, s32 index);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void LoadTIM(unsigned long *adr);
extern void _PlayMusic(s32 music, s32 mode);
extern s32 GetRealPad(s32 port);
extern void StartDrawing(void);
extern BackGround *FUN_8004f4f8(u_long *tim);
extern short DrawBG(BackGround *bg);
extern void EndDrawing(short sync);
extern void DisposeBG(BackGround *background);
extern void FUN_800515b0(GsSPRITE *number, s32 value, s16 x, s32 y,
                         s32 mode);
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

/* Round-10: every function-like draw macro (DRAW_SCORE_NUMBER family,
 * DRAW_SCORE_COLON, SCORE_SPRITE_AT) is EXPANDED to its literal per-site
 * text, verified byte-identical (same cpp token stream, same .o sha256).
 * cpp expands textually, so this is the same program cc1 always saw --
 * but each of the ~13 draw/colon/bank sites is now INDEPENDENTLY tunable,
 * which a shared macro spelling forbade.  */

/* The persistent high-score block, addressed by CONSTANT rather than through a
 * pointer local.  This is not cosmetic: with a pointer variable the address is
 * `(plus reg_state reg_index)` and expand_binop emits `addu t,state,index`
 * (base first); inlining the constant makes it `(plus reg_index CONST)`, and
 * expand's EXPAND_SUM/form_sum sorts the constant term LAST, emitting
 * `addu t,index,base` -- the target's operand order. */
#define SCORE_STATE ((MissionScorePersistent *)TENCHU_PERSISTENT_STATE_ADDRESS)
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
    register u8 *unlock;
    register TLinkInfo *statePtr;
    register s16 i;
    register s16 newPress;
    s16 brightness;
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
        initNumber->attribute |= 0x50000000;
        initNumber->x = -140;
        initNumber->y = -40;
        brightness = 128;
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
        u32 width;

        tim = get_tim_from_archive(archive, i);
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
        ((GsSPRITE *)((u8 *)(rankSprites) +
                      (i) * sizeof(GsSPRITE)))->mx = 0;
        ((GsSPRITE *)((u8 *)(rankSprites) +
                      (i) * sizeof(GsSPRITE)))->my = 0;
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
            u32 width;
            u32 height;

            tim = get_tim_from_archive(archive, i + 5);
            initSprite = (GsSPRITE *)((u8 *)&storage +
                                      i * sizeof(GsSPRITE));
            initSprite = (GsSPRITE *)((u8 *)initSprite +
                                      sizeof(ScoreResult) +
                                      2 * sizeof(u32) +
                                      5 * sizeof(GsSPRITE));
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
            ((GsSPRITE *)((u8 *)(characterSprites) +
                          (i) * sizeof(GsSPRITE)))->mx = 0;
            ((GsSPRITE *)((u8 *)(characterSprites) +
                          (i) * sizeof(GsSPRITE)))->my = 0;
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
        for (i = 0; i < 5; i++)
        {
            if (SCORE_STATE->scores[i] == 0)
            {
                SCORE_STATE->scores[i] = 0x1A5C2;
                SCORE_STATE->characters[i] = 0;
                SCORE_STATE->grades[i] = 0;
            }
        }
    }

    insertedRank = -1;
    {
        for (i = 0; i < 5; i++)
        {
            if (SCORE_STATE->grades[i] < result.grade)
            {
                insertedRank = i;
                break;
            }
            if (result.grade == SCORE_STATE->grades[i] &&
                stats.clock < SCORE_STATE->scores[i])
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
                do
                {
                    SCORE_STATE->scores[i] = SCORE_STATE->scores[i - 1];
                    SCORE_STATE->characters[i] =
                        SCORE_STATE->characters[i - 1];
                    SCORE_STATE->grades[i] = SCORE_STATE->grades[i - 1];
                    i--;
                } while (insertedRank < (s16)i);
            }

            {
                register s32 insertedAt = insertedRank;

                SCORE_STATE->scores[insertedAt] = stats.clock;
                SCORE_STATE->characters[insertedAt] =
                    ((TLinkInfo *)SCORE_STATE)->CharType;
                SCORE_STATE->grades[insertedAt] = result.grade;
            }
        }
    }

    _PlayMusic(12, 1);
    resultX = 0x66;
    for (;;)
    {
        u16 pad;

        pad = GetRealPad(0);
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

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = &number;
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                ((drawY) = (-0x47));
                drawnSprite->x = 0x16;
                (value = (stats.criticals), signedValue = (s32)value);
                drawnSprite->y = drawY;
                if (1)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_0;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_0;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_0:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = drawnSprite->u;
                    drawnSprite->u =
                        baseU + (s16)remainder * drawnSprite->w;
                    GsSortSprite(drawnSprite, OTablePt, 0);
                    drawnSprite->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    drawnSprite->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = drawnSprite->u;
                    drawnSprite->u = signBaseU + drawnSprite->w * ten;
                    GsSortSprite(drawnSprite, OTablePt, 0);
                    drawnSprite->u = signBaseU;
                }
            }
        } while (0);
        {
        s32 drawX;
        GsSPRITE *numberSprite;

        numberSprite = &number;

        do
        {
            u8 oldU;
            s32 colonDigit;

            number.x = (0x1F);
            oldU = (numberSprite)->u;
            colonDigit = 12;
            (numberSprite)->u = oldU + (numberSprite)->w * colonDigit;
            GsSortSprite((numberSprite), OTablePt, 0);
            (numberSprite)->u = oldU;
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
                goto score_number_1;
            }
            else
            {
                negative = 0;
            }
            score_number_1:
            do
            {
                dividend = (s16)value;
                quotient = dividend / 10;
                remainder = dividend % 10;
                baseU = drawnSprite->u;
                drawnSprite->u =
                    baseU + (s16)remainder * drawnSprite->w;
                GsSortSprite(drawnSprite, OTablePt, 0);
                drawnSprite->x -= 12;
                value = quotient;
                quotient <<= 16;
                drawnSprite->u = baseU;
            } while (quotient != 0);
            if (negative != 0)
            {
                u32 signBaseU;
                s32 ten;

                ten = 10;
                signBaseU = drawnSprite->u;
                drawnSprite->u = signBaseU + drawnSprite->w * ten;
                GsSortSprite(drawnSprite, OTablePt, 0);
                drawnSprite->u = signBaseU;
            }
        } while (0);
        }
        do
        {
            sprite = &number;
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s32 drawY;

                value = result.criticalScore;
                drawY = -0x47;
                sprite->x = resultX;
                sprite->y = drawY;
                if (1)
                {
                    if ((s16)value < 0)
                    {
                        value = -(s16)value;
                        negative = 1;
                        goto score_number_2;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if ((s16)value >= 0)
                        goto score_number_2;
                    value = -(s16)value;
                    negative = 1;
                }
        score_number_2:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = sprite->u;
                    sprite->u = baseU + (s16)remainder * sprite->w;
                    GsSortSprite(sprite, OTablePt, 0);
                    sprite->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    sprite->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = sprite->u;
                    sprite->u = signBaseU + sprite->w * ten;
                    GsSortSprite(sprite, OTablePt, 0);
                    sprite->u = signBaseU;
                }
            }
        } while (0);

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (stats.murders), signedValue = (s32)value);
                drawY = -0x35;
                (drawnSprite)->x = (0x16);
                drawnSprite->y = -0x35;
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_3;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_3;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_3:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            }
        } while (0);
        {
            GsSPRITE *numberSprite;
            GsSPRITE *drawnSprite;

            numberSprite = &number;
            {
                u8 oldU;
                s32 colonDigit;

                number.x = (0x1F);
                oldU = (numberSprite)->u;
                colonDigit = 12;
                (numberSprite)->u = oldU + (numberSprite)->w * colonDigit;
                GsSortSprite((numberSprite), OTablePt, 0);
                (numberSprite)->u = oldU;
                drawnSprite = numberSprite;
            }
            do
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (stats.stageEnemies), signedValue = (s32)value);
                drawY = -0x35;
                (drawnSprite)->x = (0x2F);
                drawnSprite->y = -0x35;
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_4;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_4;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_4:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            } while (0);
        }
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (result.murderScore), signedValue = (s16)value);
                drawY = -0x35;
                drawnSprite->x = resultX;
                drawnSprite->y = drawY;
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_5;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_5;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_5:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            }
        }

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = (stats.findEnemies), signedValue = (s32)value);
                ((drawY) = (-0x24));
                (drawnSprite)->x = (0x23);
                ((drawnSprite)->y = (-0x24));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_6;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_6;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_6:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            }
        } while (0);
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = ((u16)result.spottedScore), signedValue = (s16)value);
                ((drawY) = (-0x24));
                drawnSprite->x = resultX;
                ((drawnSprite)->y = (-0x24));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_7;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_7;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_7:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            }
        }

        do
        {
            GsSPRITE *drawnSprite;

            drawnSprite = (&number);
            {
                s32 dividend;
                s32 remainder;
                s32 quotient;
                u32 value;
                s16 signedValue;
                s32 drawY;

                (value = ((u16)result.score), signedValue = (s16)value);
                ((drawY) = (-0x12));
                drawnSprite->x = resultX;
                ((drawnSprite)->y = (-0x12));
                if (0)
                {
                    if (signedValue < 0)
                    {
                        value = -signedValue;
                        negative = 1;
                        goto score_number_8;
                    }
                    else
                    {
                        negative = 0;
                    }
                }
                else
                {
                    negative = 0;
                    if (signedValue >= 0)
                        goto score_number_8;
                    value = -signedValue;
                    negative = 1;
                }
        score_number_8:
                do
                {
                    dividend = (s16)value;
                    quotient = dividend / 10;
                    remainder = dividend % 10;
                    baseU = (drawnSprite)->u;
                    (drawnSprite)->u = baseU + (s16)remainder * (drawnSprite)->w;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->x -= 12;
                    value = quotient;
                    quotient <<= 16;
                    (drawnSprite)->u = baseU;
                } while (quotient != 0);
                if (negative != 0)
                {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (drawnSprite)->u;
                    (drawnSprite)->u = signBaseU + (drawnSprite)->w * ten;
                    GsSortSprite((drawnSprite), OTablePt, 0);
                    (drawnSprite)->u = signBaseU;
                }
            }
        } while (0);

        if (result.grade == RANK_GRAND_MASTER)
        {
            medal = &ItemImage[D_8008ED50[CHOSEN_STAGE]]->sprite;
            medal->x = 0x8A;
            medal->y = -0xE;
            medal->scalex = 0x1000;
            medal->scaley = 0x1000;
            medalBrightness = rcos((GameClock << 12) / 90) * 80;
            if (medalBrightness < 0)
            {
                medalDraw = medal;
                medalBrightness += 0xFFF;
            }
            else
            {
                medalDraw = medal;
            }
            medalBrightness = (medalBrightness >> 12) + 0x7F;
            medalDraw->r = medalDraw->g = medalDraw->b = medalBrightness;
            GsSortSprite(medalDraw, OTablePt, 1);
        }

        {
            GsSPRITE *rankSpriteBase;
            register GsSPRITE *rowSprite;

            i = 0;
            rowSprite = &number;
            do { do { do { do
            {
                rankSpriteBase = rankSprites;
            } while (0); } while (0); } while (0); } while (0);
score_row_loop:
            {
                do
                {
                    s32 dividend;
                    s32 remainder;
                    s32 quotient;
                    u16 value;
                    s16 signedValue;
                    s32 widenedValue;
                    s32 drawY;
                    register s32 negative;

                    signedValue = (i + 1);
                    value = signedValue;
                    drawY = (i * 0x16 + 0x18);
                    (rowSprite)->x = (-0x8F);
                    (rowSprite)->y = drawY;
                    widenedValue = signedValue;
                    if (widenedValue < 0)
                    {
                        value = -widenedValue;
                        negative = 1;
                        goto score_row_number;
                    }
                    else
                    {
                        negative = 0;
                    }
                score_row_number:
                    do
                    {
                        dividend = (s16)value;
                        quotient = dividend / 10;
                        remainder = dividend % 10;
                        baseU = (rowSprite)->u;
                        (rowSprite)->u = baseU + (s16)remainder * (rowSprite)->w;
                        GsSortSprite((rowSprite), OTablePt, 0);
                        (rowSprite)->x -= 12;
                        value = quotient;
                        quotient <<= 16;
                        (rowSprite)->u = baseU;
                    } while (quotient != 0);
                    if (negative != 0)
                    {
                    u32 signBaseU;
                    s32 ten;

                    ten = 10;
                    signBaseU = (rowSprite)->u;
                        (rowSprite)->u = signBaseU + (rowSprite)->w * ten;
                        GsSortSprite((rowSprite), OTablePt, 0);
                        (rowSprite)->u = signBaseU;
                    }
                } while (0);
                FUN_800515b0(&number, SCORE_STATE->scores[i],
                             0x79, i * 0x16 + 0x18, 1);
                {
                    /* Dead local retained by the row-rendering template. */
                    s32 rowScore;
                }

                {
                    GsSPRITE *characterSpriteBase = characterSprites;
                    MissionScorePersistent *rowState =
                        (MissionScorePersistent *)TENCHU_PERSISTENT_STATE_ADDRESS;

                    sprite = &characterSpriteBase[
                        rowState->characters[i]];
                }
                sprite->x = -0x79;
                sprite->y = i * 0x16 + 0x16;
                if (i == insertedRank)
                {
                    rowBrightness = rsin((GameClock << 12) / 90) * 60;
                    if (rowBrightness < 0)
                    {
                        rowBrightness += 0xFFF;
                    }
                    rowBrightness = (rowBrightness >> 12) + 100;
                }
                else
                {
                    rowBrightness = 0x80;
                }
                sprite->r = sprite->g = sprite->b = rowBrightness;
                do {
                } while (0);
                GsSortSprite(sprite, OTablePt, 1);

                {
                    register GsSPRITE *rankSprite =
                        &rankSpriteBase[SCORE_STATE->grades[i]];
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
        register TLinkInfo *state =
            (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;

        stageItem = D_8008ED50[state->StageNo];
        brightness = stageItem;
        if (state->stock[brightness + (state->CharType << 5)] == 0xFE)
        {
            state->stock[brightness + (state->CharType << 5)] += 3;
        }
        stageItem = D_8008ED50[state->StageNo];
        if (state->backup[stageItem] == 0xFE)
        {
            state->backup[stageItem] += 3;
        }
    }

    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    do {
        statePtr = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    } while (0);
    if (gfMemory != 0)
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
#undef result
#undef rankSprites
#undef characterSprites
