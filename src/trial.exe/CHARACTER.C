#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/model.h"
#include "../main.exe/music.h"
#include "../main.exe/sound.h"
#include <psxsdk/libgpu.h>

static s16 CharacterSlideFrame = 0;
static s16 CharacterSliding = 0;

enum
{
    TRIAL_CHARACTER_MUSIC = 15,
    TRIAL_SOUND_ACCEPT = 4,
    TRIAL_SOUND_CANCEL = 5,
    TRIAL_SOUND_CHARACTER = 6
};

short ChooseTrialCharacter(void)
{
    short accepted = 1;
    short phase = 1;
    short brightness = 128;
    short animating = 0;
    u_long previous_buttons = 0;
    short character = RIKIMARU_0;
    u_long pressed;

    InitGraphicsSystem();
    ClearTrialDisplay(320, 480);
    ResetTrialCharacterGraphics();
    LoadTrialCharacterGraphics();
    _PlayMusic(TRIAL_CHARACTER_MUSIC, 1);
    ClearTrialPages();
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    do
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            brightness -= 12;
            if (brightness <= 0)
            {
                brightness = 0;
                phase++;
                previous_buttons = GetRealPad(0);
            }
            DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        case 2:
            pressed = ReadTrialMenuPad(0, &previous_buttons, animating);
            animating = UpdateTrialCharacterSelection(pressed, &character);
            if (pressed == PADRdown)
            {
                accepted = 0;
                phase++;
                SoundEx(NULL, TRIAL_SOUND_CANCEL);
            }
            else if (pressed == PADRright)
            {
                accepted = 1;
                phase++;
                SoundEx(NULL, TRIAL_SOUND_ACCEPT);
            }
            break;
        case 3:
            brightness += 12;
            if (brightness >= 128)
            {
                brightness = 128;
                phase++;
            }
            DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        default:
            phase = 0;
            break;
        }
        DrawBG(TrialCharacterBackground);
        DrawSprite(TrialCharacterPortraits[RIKIMARU_0]);
        DrawSprite(TrialCharacterPortraits[AYAME_0]);
        SkipFrame = 2;
        EndDrawing(-2);
    } while (phase != 0);
    ClearTrialPages();
    SetDispMask(0);
    DisposeTrialCharacterGraphics();
    TRIAL_STATE->CharType = character;
    return accepted;
}

/* The entering portrait brightens and moves forward over four updates;
 * the other portrait dims and moves to the corresponding side. */
int UpdateTrialCharacterSelection(u_long pressed, short *character)
{
    GsSPRITE *sprite;
    short step;

    if (CharacterSliding)
    {
        if (*character == RIKIMARU_0)
        {
            step = ++CharacterSlideFrame;
            MODEL_POSITION(TrialCharacterPortraits[RIKIMARU_0])->vx = step * 150 - 600;
            MODEL_POSITION(TrialCharacterPortraits[RIKIMARU_0])->vy = 20;
            MODEL_POSITION(TrialCharacterPortraits[RIKIMARU_0])->vz = 1100 - step * 200;
            sprite = &TrialCharacterPortraits[RIKIMARU_0]->sprite;
            sprite->r = sprite->g = sprite->b += 16;
            UpdateCoordinate((ModelType *)TrialCharacterPortraits[RIKIMARU_0]);
            step = CharacterSlideFrame;
            MODEL_POSITION(TrialCharacterPortraits[AYAME_0])->vx = step * 150;
            MODEL_POSITION(TrialCharacterPortraits[AYAME_0])->vy = 20;
            MODEL_POSITION(TrialCharacterPortraits[AYAME_0])->vz = step * 200 + 300;
            sprite = &TrialCharacterPortraits[AYAME_0]->sprite;
            sprite->r = sprite->g = sprite->b -= 16;
            UpdateCoordinate((ModelType *)TrialCharacterPortraits[AYAME_0]);
        }
        else
        {
            step = ++CharacterSlideFrame;
            MODEL_POSITION(TrialCharacterPortraits[AYAME_0])->vx = 600 - step * 150;
            MODEL_POSITION(TrialCharacterPortraits[AYAME_0])->vy = 20;
            MODEL_POSITION(TrialCharacterPortraits[AYAME_0])->vz = 1100 - step * 200;
            sprite = &TrialCharacterPortraits[AYAME_0]->sprite;
            sprite->r = sprite->g = sprite->b += 16;
            UpdateCoordinate((ModelType *)TrialCharacterPortraits[AYAME_0]);
            step = CharacterSlideFrame;
            MODEL_POSITION(TrialCharacterPortraits[RIKIMARU_0])->vx = step * -150;
            MODEL_POSITION(TrialCharacterPortraits[RIKIMARU_0])->vy = 20;
            MODEL_POSITION(TrialCharacterPortraits[RIKIMARU_0])->vz = step * 200 + 300;
            sprite = &TrialCharacterPortraits[RIKIMARU_0]->sprite;
            sprite->r = sprite->g = sprite->b -= 16;
            UpdateCoordinate((ModelType *)TrialCharacterPortraits[RIKIMARU_0]);
        }
        if (CharacterSlideFrame >= 4)
        {
            CharacterSlideFrame = 0;
            CharacterSliding = 0;
        }
    }
    else
    {
        if (pressed == PADLleft && *character != RIKIMARU_0)
        {
            *character = RIKIMARU_0;
            CharacterSliding = 1;
            SoundEx(NULL, TRIAL_SOUND_CHARACTER);
        }
        else if (pressed == PADLright && *character != AYAME_0)
        {
            *character = AYAME_0;
            CharacterSliding = 1;
            SoundEx(NULL, TRIAL_SOUND_CHARACTER);
        }
    }
    return CharacterSliding;
}
