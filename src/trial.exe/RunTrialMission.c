#include "common.h"
#include "mission_editor.h"
#include "mission_hud.h"
#include "traps.h"
#include "../main.exe/action.h"
#include "../main.exe/effect.h"
#include "../main.exe/graphics.h"
#include "../main.exe/item.h"
#include "../main.exe/misc.h"
#include "../main.exe/sound.h"

static int TrialThemeMusic[TRIAL_THEME_COUNT] = {
    MUSIC_TRACK_STAGE9, MUSIC_TRACK_STAGE1, MUSIC_TRACK_STAGE2,
    MUSIC_TRACK_STAGE3, MUSIC_TRACK_STAGE10, MUSIC_TRACK_STAGE11,
    MUSIC_TRACK_STAGE4, MUSIC_TRACK_STAGE5, MUSIC_TRACK_STAGE6,
    MUSIC_TRACK_STAGE7, MUSIC_TRACK_STAGE8, MUSIC_TRACK_ONIKAGE
};

enum trial_mission_result
{
    TRIAL_TIME_EXPIRED = -4,
    TRIAL_OBJECTIVE_LOST = -3,
    TRIAL_STEALTH_FAILED = -2,
    TRIAL_PLAYER_DIED = -1,
    TRIAL_ABORTED = 0,
    TRIAL_COMPLETED = 1
};

enum
{
    TRIAL_RUNNING = 1,
    TRIAL_BEGIN_FAILURE = 2,
    TRIAL_BEGIN_SUCCESS = -1,
    TRIAL_SUCCESS_FINISH_NEXT = -32767,
    TRIAL_TIME_LIMIT_TICKS = (60 * 60 - 1) * 30,
    TRIAL_CUE_REACH_LORD_RIKIMARU = 101,
    TRIAL_CUE_REACH_LORD_AYAME = 102,
    TRIAL_CUE_ESCORT_RIKIMARU = 140,
    TRIAL_CUE_ESCORT_AYAME = 141,
    TRIAL_EVENT_VICTORY_A = 2,
    TRIAL_EVENT_VICTORY_B = 4,
    TRIAL_EVENT_TIME_EXPIRED = 8,
    TRIAL_EVENT_STEALTH_FAILED = 10,
    TRIAL_EVENT_HERB_RIKIMARU = 37,
    TRIAL_EVENT_HERB_AYAME = 38,
    TRIAL_SE_STEALTH_FAILURE = 0x20
};

/* Phase one runs the mission. Negative phases count down the success scene;
 * phases two and above count up the failure scene before resource cleanup. */
short RunTrialMission(short character)
{
    RECT clear_area = {0, 0, 320, 480};
    short previous_events[4];
    VECTOR sound_position;
    int elapsed = 0;
    short result = TRIAL_ABORTED;
    short phase = TRIAL_RUNNING;
    short i;
    Humanoid *human;
    Humanoid *objective;

    ClearImage(&clear_area, 0, 0, 0);
    DrawSync(0);
    UpdateTrialTimeNotice(TRIAL_TIME_NOTICE_RESET);
    memset(previous_events, 0, sizeof(previous_events));
    ActivateTrialHumans(1);
    InfoViewPauseEnabled = 1;
    TrialRemainingEnemies = -1;
    clear_pad_send_();
    _PlayMusic(TrialThemeMusic[TrialMission.header.theme], 1);
    while (phase != 0)
    {
        PadProc();
        if (phase == TRIAL_RUNNING)
        {
            if (GetRealPad(0) == (PADselect | PADstart))
                break;
            if (GameClock % 30 == 0)
                ActivateTrialHumans(0);
        }
        ComputeAllConflict();
        StartDrawing();
        Camera();
        ControlAllHumanoid();
        DrawEffect();
        DoItemProc();
        if (phase == TRIAL_RUNNING)
        {
            DoInfoViewProc();
            DrawTrialEnemyCandles();
        }
        DoMiscProc();
        draw_visible_characters_();
        DrawTrialTerrainChunks(1);
        DrawTrialTerrainOrnaments();
        if (result != TRIAL_STEALTH_FAILED)
        {
            DrawTrialMissionTimer(elapsed);
            UpdateTrialTimeNotice(TRIAL_TIME_NOTICE_DRAW);
        }
        else
            DrawTrialFailureBanner();
        if (phase == TRIAL_RUNNING)
        {
            UpdateTrialArrowTraps();
            UpdateTrialCollapsingFloors();
        }
        EndDrawing(-2);

        if (phase == TRIAL_RUNNING)
        {
            if (previous_events[TRIAL_TIME_NOTICE_STEALTH_KILL] < Criticals)
            {
                GameClock -= 15 * 30;
                previous_events[(short)UpdateTrialTimeNotice(TRIAL_TIME_NOTICE_STEALTH_KILL)] = Criticals;
            }
            if (previous_events[TRIAL_TIME_NOTICE_KILL] < Murders)
            {
                GameClock -= 5 * 30;
                previous_events[(short)UpdateTrialTimeNotice(TRIAL_TIME_NOTICE_KILL)] = Murders;
            }
            if (previous_events[TRIAL_TIME_NOTICE_CIVILIAN] < FriendHits)
            {
                GameClock += 30 * 30;
                previous_events[(short)UpdateTrialTimeNotice(TRIAL_TIME_NOTICE_CIVILIAN)] = FriendHits;
            }
            if (previous_events[TRIAL_TIME_NOTICE_SPOTTED] < Findenemies)
            {
                if (TrialMission.header.flags & TRIAL_REQUIRE_STEALTH)
                {
                    for (i = 0; i < Humans; i++)
                    {
                        human = HumanGroup[i];
                        if (human->model->object[MODEL_PART_WAIST]->id != -1 &&
                            (human->attribute & ATTR_PHASE) == PHASE_ALERT)
                        {
                            StagePlayer->rotate->vy += GetDirection(
                                human->locate->vx - StagePlayer->locate->vx,
                                human->locate->vz - StagePlayer->locate->vz,
                                StagePlayer->rotate->vy);
                            break;
                        }
                    }
                    phase = TRIAL_BEGIN_FAILURE;
                    result = TRIAL_STEALTH_FAILED;
                }
                GameClock += 10 * 30;
                previous_events[(short)UpdateTrialTimeNotice(TRIAL_TIME_NOTICE_SPOTTED)] = Findenemies;
            }
            if (GameClock < 0)
            {
                GameClock = 0;
                elapsed = 0;
            }
            else
                elapsed = GameClock;
            if (elapsed >= TRIAL_TIME_LIMIT_TICKS)
            {
                phase = TRIAL_BEGIN_FAILURE;
                result = TRIAL_TIME_EXPIRED;
            }
            if (StagePlayer->status == STAT_DEAD || StagePlayer->life == 0)
            {
                phase = TRIAL_BEGIN_FAILURE;
                result = TRIAL_PLAYER_DIED;
            }
            if (phase == TRIAL_RUNNING && GameClock % 15 == 0)
            {
                switch (TrialMission.header.flags & TRIAL_OBJECTIVE_MASK)
                {
                case TRIAL_REACH_LORD:
                    objective = HumanGroup[1];
                    if (objective->status == STAT_DEAD)
                    {
                        phase = TRIAL_BEGIN_FAILURE;
                        result = TRIAL_OBJECTIVE_LOST;
                        CamState.Owner = objective;
                    }
                    if (phase == TRIAL_RUNNING && StagePlayer->map.height == 0 &&
                        StagePlayer->status != STAT_DAMAGE &&
                        __builtin_abs(objective->locate->vx - StagePlayer->locate->vx) < 2000 &&
                        __builtin_abs(objective->locate->vz - StagePlayer->locate->vz) < 2000 &&
                        objective->locate->vy == StagePlayer->locate->vy)
                    {
                        phase = TRIAL_BEGIN_SUCCESS;
                        CamState.Owner = objective;
                    }
                    break;
                case TRIAL_ASSASSINATE:
                    if (StagePlayer->map.height == 0 && HumanGroup[1]->status == STAT_DEAD)
                        phase = TRIAL_BEGIN_SUCCESS;
                    break;
                case TRIAL_ELIMINATE_ENEMIES:
                    if (StagePlayer->map.height == 0 && TrialRemainingEnemies == 0)
                        phase = TRIAL_BEGIN_SUCCESS;
                    break;
                case TRIAL_PROTECT_PRINCESS:
                    objective = HumanGroup[2];
                    if (HumanGroup[1]->status == STAT_DEAD)
                    {
                        phase = TRIAL_BEGIN_FAILURE;
                        result = TRIAL_OBJECTIVE_LOST;
                        CamState.Owner = HumanGroup[1];
                    }
                    if (objective->status == STAT_DEAD || objective->life == 0)
                    {
                        phase = TRIAL_BEGIN_FAILURE;
                        result = TRIAL_OBJECTIVE_LOST;
                        CamState.Owner = objective;
                    }
                    if (phase == TRIAL_RUNNING && StagePlayer->map.height == 0 &&
                        StagePlayer->status != STAT_DAMAGE &&
                        __builtin_abs(objective->locate->vx - HumanGroup[1]->locate->vx) < 2000 &&
                        __builtin_abs(objective->locate->vz - HumanGroup[1]->locate->vz) < 2000 &&
                        objective->locate->vy == HumanGroup[1]->locate->vy &&
                        objective->locate->vy <= 0)
                    {
                        phase = TRIAL_BEGIN_SUCCESS;
                        CamState.Owner = objective;
                    }
                    break;
                case TRIAL_FIND_HERB:
                    if (StagePlayer->map.height == 0 && StagePlayer->status != STAT_DAMAGE)
                    {
                        objective = HumanGroup[1];
                        if (__builtin_abs(objective->locate->vx - StagePlayer->locate->vx) < 2500 &&
                            __builtin_abs(objective->locate->vz - StagePlayer->locate->vz) < 2500 &&
                            objective->locate->vy == StagePlayer->locate->vy)
                            phase = TRIAL_BEGIN_SUCCESS;
                    }
                    break;
                }
                if (phase == TRIAL_BEGIN_SUCCESS)
                {
                    for (i = 0; i < Humans; i++)
                        if (HumanGroup[i]->life > 0)
                            HumanGroup[i]->life = HUMANOID_LIFE_INACTIVE;
                }
            }
        }
        else if (phase < 0)
        {
            if (phase == TRIAL_BEGIN_SUCCESS)
            {
                ActionHalt = ACTION_HALT_ACTIVE;
                InfoViewPauseEnabled = 0;
                SetupThinkFunction(StagePlayer, THINK_MIX_NONE);
                switch (TrialMission.header.flags & TRIAL_OBJECTIVE_MASK)
                {
                case TRIAL_REACH_LORD:
                    _PlayMusic(character == 0 ? TRIAL_CUE_REACH_LORD_RIKIMARU : TRIAL_CUE_REACH_LORD_AYAME, 0);
                    break;
                case TRIAL_ASSASSINATE:
                case TRIAL_ELIMINATE_ENEMIES:
                    PlayMusicFormID(rand() & 1 ? TRIAL_EVENT_VICTORY_A : TRIAL_EVENT_VICTORY_B);
                    break;
                case TRIAL_PROTECT_PRINCESS:
                    _PlayMusic(character == 0 ? TRIAL_CUE_ESCORT_RIKIMARU : TRIAL_CUE_ESCORT_AYAME, 0);
                    break;
                case TRIAL_FIND_HERB:
                    PlayMusicFormID(character == 0 ? TRIAL_EVENT_HERB_RIKIMARU : TRIAL_EVENT_HERB_AYAME);
                    break;
                }
                if (HumanGroup[1]->type == TONO)
                {
                    SetupThinkFunction(HumanGroup[1], THINK_MIX_NONE);
                    SetNowMotion(StagePlayer, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
                    PlayMotion(StagePlayer->motion, 1);
                    SetNowMotion(HumanGroup[1], MOT_ACTION_LOOP, MOTION_MOVE_APPLY);
                    PlayMotion(HumanGroup[1]->motion, 1);
                    StagePlayer->rotate->vy += GetDirection(
                        HumanGroup[1]->locate->vx - StagePlayer->locate->vx,
                        HumanGroup[1]->locate->vz - StagePlayer->locate->vz,
                        StagePlayer->rotate->vy);
                    HumanGroup[1]->rotate->vy += GetDirection(
                        StagePlayer->locate->vx - HumanGroup[1]->locate->vx,
                        StagePlayer->locate->vz - HumanGroup[1]->locate->vz,
                        HumanGroup[1]->rotate->vy);
                }
                if (HumanGroup[2]->type == HIME)
                {
                    SetupThinkFunction(HumanGroup[2], THINK_MIX_NONE);
                    SetNowMotion(HumanGroup[2], MOT_ACTION_VARIANT_8, MOTION_MOVE_APPLY);
                    PlayMotion(HumanGroup[2]->motion, 1);
                    HumanGroup[1]->rotate->vy += GetDirection(
                        HumanGroup[2]->locate->vx - HumanGroup[1]->locate->vx,
                        HumanGroup[2]->locate->vz - HumanGroup[1]->locate->vz,
                        HumanGroup[1]->rotate->vy);
                    HumanGroup[2]->rotate->vy += GetDirection(
                        HumanGroup[1]->locate->vx - HumanGroup[2]->locate->vx,
                        HumanGroup[1]->locate->vz - HumanGroup[2]->locate->vz,
                        HumanGroup[2]->rotate->vy);
                }
                if (HumanGroup[1]->type == TRIAL_HERB_OBJECT)
                {
                    SetNowMotion(StagePlayer, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
                    PlayMotion(StagePlayer->motion, 1);
                    StagePlayer->rotate->vy += GetDirection(
                        HumanGroup[1]->locate->vx - StagePlayer->locate->vx,
                        HumanGroup[1]->locate->vz - StagePlayer->locate->vz,
                        StagePlayer->rotate->vy);
                }
                if (CamState.Mode != CMODE_CRITICAL_HIT)
                    SetCameraMode(CMODE_CRITICAL_HIT);
            }
            phase--;
            if (phase < -400)
            {
                phase = 0;
                result = TRIAL_COMPLETED;
            }
            if ((TrialMission.header.flags & TRIAL_OBJECTIVE_MASK) != TRIAL_PROTECT_PRINCESS &&
                phase < -200)
                phase = TRIAL_SUCCESS_FINISH_NEXT;
            if (HumanGroup[1]->motion->mid == MOT_ACTION_LOOP && HumanGroup[1]->motion->loop)
                SetNowMotion(HumanGroup[1], MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        else
        {
            if (phase == TRIAL_BEGIN_FAILURE)
            {
                InfoViewPauseEnabled = 0;
                if (result == TRIAL_STEALTH_FAILED)
                {
                    Sound(StagePlayer, CHAR_VOICE_HURT);
                    PlayMusicFormID(TRIAL_EVENT_STEALTH_FAILED);
                }
                else if (result == TRIAL_TIME_EXPIRED)
                    PlayMusicFormID(TRIAL_EVENT_TIME_EXPIRED);
                else
                    ActionHalt = ACTION_HALT_STAGE_END;
                SetupThinkFunction(StagePlayer, THINK_MIX_NONE);
                if (CamState.Mode != CMODE_FALL)
                    SetCameraMode(CMODE_CRITICAL_HIT);
            }
            phase++;
            if (StagePlayer->status == STAT_DEAD)
            {
                if (StagePlayer->motion->count == 0 && StagePlayer->motion->loop)
                {
                    if (StagePlayer->map.attrib & MAP_DEATH)
                        phase = 0;
                    else if (phase > 200)
                        phase = 0;
                }
            }
            else if (phase > 200)
                phase = 0;
            if (result == TRIAL_STEALTH_FAILED && rand() % 30 == 0)
            {
                sound_position = *StagePlayer->locate;
                sound_position.vx += (rand() % 11 - 5) * 1000;
                sound_position.vz += (rand() % 11 - 5) * 1000;
                SoundEx(&sound_position, TRIAL_SE_STEALTH_FAILURE);
                if (StagePlayer->motion->mid == MOT_ENGAGE_STANCE)
                {
                    switch (rand() % 4)
                    {
                    case 0: i = MOT_CHASE_DASH_BACK; break;
                    case 1: i = MOT_CHASE_DASH_RIGHT; break;
                    case 2: i = MOT_CHASE_DASH_LEFT; break;
                    default: i = MOT_ENGAGE_STANCE; break;
                    }
                    SetNowMotion(StagePlayer, i, MOTION_MOVE_APPLY);
                }
            }
        }
    }
    FadeOutDirect(32, 2, 8, 8, 8);
    ClearImage(&clear_area, 0, 0, 0);
    DrawSync(0);
    GameClock = elapsed;
    CdaStop();
    UpdateTrialTerrainChunks(0);
    while (Humans)
        KillHumanoid(HumanGroup[0]);
    ClearItemLayout();
    reset_effects_();
    ResetInfoview(-1);
    return result;
}
