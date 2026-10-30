#include "common.h"
#include "mission_editor.h"
#include "mission_hud.h"
#include "traps.h"
#include "../main.exe/appear.h"
#include "../main.exe/images.h"
#include "../main.exe/item.h"
#include "../main.exe/sound.h"
#include "../main.exe/timpack.h"

static const char CommonImages[] = "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\COMMON.TPD";
static const char TrialCommonImages[] = "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\TRIALCMN.TPD";
static const char ThemeImages[] = "%sIMAGE\\TRIAL%d.TPD";
static const char TrialDirectory[] = "K:\\WORK\\CDIMAGE\\TRIAL\\";
static const char TimerImage[] = "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\time.tim";
static const char NoticeImage[] = "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\bar.tim";
static const char FailureImage[] = "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\fail.tim";
static const char CandleImages[] = "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\rousoku.arc";

/* A positive mode loads the selected mission's theme and map. Mode zero
 * rebuilds the live editor map for a playtest; a negative mode unloads it. */
void SetupTrialMission(short character, short mode)
{
    GsIMAGE image;
    char filename[64];
    TrialPlacement *placements = TrialMission.placements;
    u_long *data;
    ArcFile *archive;
    Sprite3D *sprite;
    Humanoid *human;
    TrialPatrolPoint *patrol;
    TracePoint *trace;
    long x, y, z;
    short i, j;
    u16 policy;

    if (mode < 0)
    {
        SetupAppearance(0, -1);
        LoadTrialEditorTheme(-1);
    }
    else
    {
        if (mode)
        {
            SetupSoundEffect(character, TrialMission.header.theme);
            LoadTrialEditorTheme(TrialMission.header.theme);
            SetupAppearance(character, TrialMission.header.theme + 1);
            __builtin_memcpy(TrialEditorMap, TrialMission.map, sizeof(TrialEditorMap));
        }
        LoadTIMpackAndFree(FileRead(CommonImages));
        LoadTIMpackAndFree(FileRead(TrialCommonImages));
        sprintf(filename, ThemeImages, TrialDirectory, TrialMission.header.theme + 1);
        LoadTIMpackAndFree(FileRead(filename));
        InitializeImage();

        data = FileRead(TimerImage);
        GetTIMInfo(data, &image);
        LoadTIMAndFree(data);
        image.ph /= 11;
        sprite = SetupSprite(NULL, &image);
        TrialTimerDigits[0] = sprite->sprite;
        vfree(sprite);
        for (i = 0; i < 11; i++)
        {
            TrialTimerDigits[i] = TrialTimerDigits[0];
            TrialTimerDigits[i].v += i * image.ph;
        }

        data = FileRead(NoticeImage);
        GetTIMInfo(data, &image);
        LoadTIMAndFree(data);
        image.ph /= 4;
        sprite = SetupSprite(NULL, &image);
        TrialTimeNotices[0] = sprite->sprite;
        vfree(sprite);
        for (i = 0; i < 4; i++)
        {
            TrialTimeNotices[i] = TrialTimeNotices[0];
            TrialTimeNotices[i].v += i * image.ph;
            TrialTimeNotices[i].x = 0;
            TrialTimeNotices[i].y = -56;
            TrialTimeNotices[i].attribute |= GS_ATTR_SEMITRANS_ADD;
        }

        data = FileRead(FailureImage);
        GetTIMInfo(data, &image);
        LoadTIMAndFree(data);
        sprite = SetupSprite(NULL, &image);
        TrialFailureBanner = sprite->sprite;
        vfree(sprite);
        TrialFailureBanner.y = -76;
        TrialFailureBanner.r = TrialFailureBanner.g = TrialFailureBanner.b = 0;
        TrialFailureBanner.attribute |= GS_ATTR_SEMITRANS_ADD;

        archive = (ArcFile *)FileRead(CandleImages);
        for (i = 0; i < 3; i++)
        {
            data = get_tim_from_archive(archive, i);
            GetTIMInfo(data, &image);
            LoadTIM(data);
            sprite = SetupSprite(NULL, &image);
            TrialEnemyCandles[i] = sprite->sprite;
            vfree(sprite);
            TrialEnemyCandles[i].y = 90;
        }
        vfree(archive);
        ValidateTrialEditorMap();
        UpdateTrialTerrainChunks(2);

        for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
        {
            if (placements[i].kind != CHARACTER_KIND_END)
            {
                x = placements[i].x * 2000 + 1000;
                z = placements[i].z * -2000 - 1000;
                y = GetAreaMapLevel(GlobalAreaMap, x, -10000, z, 0);
                if (placements[i].kind == 0)
                {
                    human = BreedLife(character != 0, x, y, z, rand() & ANGLE_MASK);
                    ViewInfo.vpx = ViewInfo.vrx = human->locate->vx;
                    ViewInfo.vpy = ViewInfo.vry = human->locate->vy - 2000;
                    ViewInfo.vpz = ViewInfo.vrz = human->locate->vz;
                    SetupThinkFunction(human, THINK_MIX_PLAYER);
                }
                else
                {
                    human = BreedLife(placements[i].kind, x, y, z, rand() & ANGLE_MASK);
                    switch (placements[i].kind & PAGE_MASK)
                    {
                    case PAGE_BOSS:
                        /* Controller two can take over a boss at mission entry. */
                        if (GetPad(1) == PADstart)
                        {
                            policy = THINK_MIX_PAD2;
                            human->attribute |= PHASE_ALERT;
                        }
                        else
                        {
                            policy = placements[i].route_last ?
                                THINK_MIX(THINK1_TRACE, THINK2_CONFIRM, 0, 0) :
                                THINK_MIX(THINK1_WATCH, THINK2_CONFIRM, 0, 0);
                            if (rand() & 1)
                                policy |= THINK_MIX(0, 0, THINK3_ATK_CHASE, THINK4_CONTACT);
                            else
                                policy |= THINK_MIX(0, 0, THINK3_ATK_POINT, THINK4_CONTACT);
                        }
                        break;
                    case PAGE_CIVILIAN:
                        policy = (placements[i].route_last ? THINK1_TRACE : THINK1_WATCH) |
                            (i & 1 ? THINK_MIX(0, THINK2_CONFIRM, THINK3_ESCAPE, 0) :
                                     THINK_MIX(0, THINK2_CONTACT, THINK3_ESCAPE, 0)) |
                            (placements[i].kind == CHONIN ? THINK_MIX(0, 0, 0, THINK4_CONTACT) :
                                                           THINK_MIX(0, 0, 0, THINK4_ABANDON));
                        break;
                    default:
                        if (placements[i].kind == HIME)
                        {
                            if (GetPad(1) == PADselect)
                            {
                                policy = THINK_MIX_PAD2;
                                human->attribute |= PHASE_ALERT;
                            }
                            else
                                policy = placements[i].route_last ? THINK1_TRACE : THINK_BASIC_NONE;
                        }
                        else if (placements[i].kind == TONO)
                            policy = placements[i].route_last ? THINK1_TRACE : THINK_BASIC_NONE;
                        else if (placements[i].kind == TRIAL_HERB_OBJECT)
                        {
                            policy = THINK_MIX_NONE;
                            DeleteConflict(human->model->object[MODEL_PART_WAIST]);
                        }
                        else
                        {
                            policy = (placements[i].route_last ? THINK1_TRACE : THINK1_WATCH) |
                                (i & 1 ? THINK_MIX(0, THINK2_CONFIRM, 0, 0) :
                                         THINK_MIX(0, THINK2_CONTACT, 0, 0));
                            if (rand() & 1)
                                policy |= THINK_MIX(0, 0, THINK3_ATK_CHASE, 0);
                            else
                                policy |= THINK_MIX(0, 0, THINK3_ATK_POINT, 0);
                            if (rand() & 1)
                                policy |= THINK_MIX(0, 0, 0, THINK4_ABANDON);
                            else
                                policy |= THINK_MIX(0, 0, 0, THINK4_CONTACT);
                        }
                        break;
                    }
                    SetupThinkFunction(human, policy);

                    if (placements[i].route_last)
                    {
                        patrol = placements[i].route;
                        j = placements[i].route_last + 1;
                        if (patrol[0].x != patrol[placements[i].route_last].x ||
                            patrol[0].z != patrol[placements[i].route_last].z)
                            j += (short)(placements[i].route_last - 1);
                        trace = valloc((j + 1) * sizeof(*trace));
                        trace[j].pad = TRACE_POINT_END;
                        for (j = 0; j <= placements[i].route_last; j++)
                        {
                            trace[j].x = patrol[j].x * 2000 + 1000;
                            trace[j].z = patrol[j].z * -2000 - 1000;
                            trace[j].range = 500;
                            trace[j].pad = 0;
                            if (patrol[j].wait)
                                trace[j].pad = (patrol[j].facing + 1) |
                                               ((patrol[j].wait + 1) << 8);
                        }
                        /* Open paths retrace their interior points; closed paths
                         * already end at their first waypoint. */
                        if (patrol[0].x != patrol[placements[i].route_last].x ||
                            patrol[0].z != patrol[placements[i].route_last].z)
                        {
                            for (j = 1; j <= placements[i].route_last - 1; j++)
                                trace[placements[i].route_last + j] =
                                    trace[placements[i].route_last - j];
                        }
                        SetupTraceLine(human, trace);
                    }
                }
            }
        }

        /* Enemies target the princess in the escort objective. Story actors
         * and civilians keep the player's transform as their target. */
        human = HumanGroup[((TrialMission.header.flags & TRIAL_OBJECTIVE_MASK) ==
                            TRIAL_PROTECT_PRINCESS) * 2];
        for (i = 0; i < Humans; i++)
        {
            if ((HumanGroup[i]->type & PAGE_MASK) == PAGE_CIVILIAN ||
                HumanGroup[i]->type < 7)
                HumanGroup[i]->target = &HumanGroup[0]->model->locate;
            else
                HumanGroup[i]->target = &human->model->locate;
        }
        StagePlayer = HumanGroup[0];
        StageBosses = StageEnemies = StageCitizens = 0;
        for (i = 0; i < Humans; i++)
        {
            human = HumanGroup[i];
            if (human->lifemax >= 0 && human != HumanGroup[0] && human->type != NINKEN)
            {
                switch (human->type & PAGE_MASK)
                {
                case PAGE_CIVILIAN:
                    StageCitizens++;
                    break;
                case PAGE_BOSS:
                    StageBosses++;
                    /* fall through: bosses also count as enemies. */
                default:
                    StageEnemies++;
                    break;
                }
            }
        }
        if (HumanGroup[1]->type == TONO || (HumanGroup[1]->type & PAGE_MASK) == PAGE_BOSS)
        {
            StagePlayer->rotate->vy += GetDirection(
                HumanGroup[1]->locate->vx - StagePlayer->locate->vx,
                HumanGroup[1]->locate->vz - StagePlayer->locate->vz,
                StagePlayer->rotate->vy);
        }
        CamState.Owner = StagePlayer;
        SetCameraMode(CMODE_NORMAL);
        CamState.snap_pending = 1;
        GameClock = 0;
        ActionHalt = 0;
        AttackActionCount = 0;
        Criticals = Findenemies = Murders = FriendHits = 0;
        EmergencyNotice = 0;
        AttackActionCount = 0;
        StagePlayer->item[ITEM_KAGINAWA] = 255;
        StagePlayer->item[ITEM_SHURIKEN] = 5;
        StagePlayer->item[ITEM_MAKIBISHI] = 3;
        ItemCursor = 0;
        InitTrialTrapMap();
    }
}
