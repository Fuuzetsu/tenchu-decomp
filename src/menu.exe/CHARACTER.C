#include "common.h"
#include "character.h"
#include "../main.exe/action.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/model.h"
#include "../main.exe/music.h"
#include "../main.exe/tim.h"
#include "../main.exe/tuning.h"
#include "../main.exe/vmemory.h"

extern char path_menu_rikimaru_texture[];
extern char path_menu_rikimaru_model[];
extern char path_menu_ayame_texture[];
extern char path_menu_ayame_model[];
extern char path_menu_ninja_sword[];
extern char path_menu_kodachi[];
extern char path_menu_character_motion[];

short SelectMenuCharacter(void)
{
    GsSPRITE *sprite;
    int phase = 1;
    int character;
    int fade = 128;
    int buttons = -1;
    int previous;
    int pulse = 0;

    CharacterImageArchive = FileRead(CharacterImagePaths[CHOSEN_LANGUAGE]);
    SetupCharacterModels(0);
    SetupCharacterImages(0);
    UpdateMotion(CharacterMotions[RIKIMARU_0], CHARACTER_MOTION_SELECTED);
    CharacterMotions[RIKIMARU_0]->count = 0;
    character = RIKIMARU_0;
    UpdateMotion(CharacterMotions[AYAME_0], CHARACTER_MOTION_WAITING);
    CharacterMotions[AYAME_0]->count = 0;
    _PlayMusic(15, 1);
    ClearMenuPages();
    while (CdaGetCurrentLength() <= 0)
        ;
    SetDispMask(1);
    GameClock = 0;
    while (phase != 0)
    {
        StartDrawing();
        PlayMotion(CharacterMotions[RIKIMARU_0], 1);
        if (CharacterMotions[RIKIMARU_0]->loop > 0)
            CharacterMotions[RIKIMARU_0]->loop = -1;
        PlayMotion(CharacterMotions[AYAME_0], 1);
        if (CharacterMotions[AYAME_0]->loop > 0)
            CharacterMotions[AYAME_0]->loop = -1;
        DrawModelArchive(CharacterModels[RIKIMARU_0], 0);
        DrawModelArchive(CharacterModels[AYAME_0], 0);
        if (phase == -2 &&
            (CharacterMotions[character]->count >= 24 ||
             CharacterMotions[character]->loop < 0))
        {
            if (CharacterMotions[character]->count == 24)
                Sound(0, 82);
            if (character == RIKIMARU_0)
                DrawOrnament(CharacterWeapons[0]);
            else
            {
                DrawOrnament(CharacterWeapons[1]);
                DrawOrnament(CharacterWeapons[2]);
            }
        }
        DrawBG(CharacterBackground);
        sprite = &CharacterSprites[character]->sprite;
        sprite->r = sprite->g = sprite->b =
            128 + (((rcos(pulse) + rsin(pulse * 3)) * 32) >> FIXED_SHIFT);
        pulse += 15;
        GsSortSprite(sprite, OTablePt, 3);
        GsSortSprite(&CharacterSprites[2]->sprite, OTablePt, 2);

        switch (phase)
        {
        case 1:
            fade -= 8;
            if (fade <= 0)
            {
                phase = 2;
                fade = 0;
            }
            DrawMenuBackground(fade);
            break;
        case -2:
            if (CharacterMotions[character]->loop != -1)
                break;
            /* Wait for the weapon-drawing animation, then fade out. */
        case -1:
            fade += 8;
            if (fade >= 128)
            {
                if (phase == -1)
                    character = -1;
                phase = 0;
            }
            DrawMenuBackground(fade);
            break;
        default:
            if (GameClock >= 5000)
                phase = -1;
            break;
        }
        SkipFrame = 2;
        EndDrawing(0);
        previous = buttons;
        buttons = GetRealPad(0);
        if (previous == 0 && buttons != 0)
        {
            if (phase < 0)
                continue;
            GameClock = 0;
            if (buttons == PADRdown)
                phase = -1;
            else if (buttons == PADRright)
            {
                Sound(0, character != RIKIMARU_0 ? 8 : 7);
                UpdateMotion(CharacterMotions[character], CHARACTER_MOTION_CONFIRM);
                fade = 0;
                phase = -2;
                UpdateMotion(CharacterMotions[1 - character], CHARACTER_MOTION_SELECTED);
            }
            else if (buttons == PADLleft && character != RIKIMARU_0)
            {
                Sound(0, 6);
                UpdateMotion(CharacterMotions[character], CHARACTER_MOTION_WAITING);
                character = RIKIMARU_0;
                UpdateMotion(CharacterMotions[character], CHARACTER_MOTION_SELECTED);
            }
            else if (buttons == PADLright && character == RIKIMARU_0)
            {
                Sound(0, 6);
                UpdateMotion(CharacterMotions[character], CHARACTER_MOTION_WAITING);
                character = AYAME_0;
                UpdateMotion(CharacterMotions[character], CHARACTER_MOTION_SELECTED);
            }
        }
    }
    CdaStop();
    ClearMenuPages();
    SetDispMask(0);
    SetupCharacterModels(1);
    SetupCharacterImages(1);
    vfree(CharacterImageArchive);
    return character;
}

void SetupCharacterImages(short dispose)
{
    GsIMAGE image;
    int i;
    u_long *data;

    if (dispose)
    {
        DisposeBG(CharacterBackground);
        i = 0;
        while (i < CHARACTER_SELECTION_SPRITES)
            vfree(CharacterSprites[i++]);
    }
    else
    {
        data = get_tim_from_archive(CharacterImageArchive, 0);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        CharacterBackground = SetupBG(&image, SCREEN_W, SCREEN_H);
        for (i = CharacterBackground->map.ncellw * CharacterBackground->map.ncellh - 1;
             i >= 0; i--)
        {
            CharacterBackground->index[i] = i;
        }
        CharacterBackground->sz = 1249;
        data = get_tim_from_archive(CharacterImageArchive, 2);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        CharacterSprites[0] = SetupSprite(0, &image);
        CharacterSprites[1] = SetupSprite(0, &image);
        CharacterSprites[0]->sprite.x = CharacterSprites[0]->sprite.w / 2 - SCREEN_W / 2;
        CharacterSprites[1]->sprite.x = SCREEN_W / 2 - CharacterSprites[1]->sprite.w / 2;
        CharacterSprites[0]->sprite.y = CharacterSprites[1]->sprite.y =
            -SCREEN_H / 2 + CharacterSprites[0]->sprite.h / 2;
        CharacterSprites[0]->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
        CharacterSprites[1]->sprite.attribute |=
            GS_ATTR_SEMITRANS_ADD | CHARACTER_SELECTION_MIRROR_X;
        data = get_tim_from_archive(CharacterImageArchive, 1);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        CharacterSprites[2] = SetupSprite(0, &image);
        CharacterSprites[2]->sprite.y = -96;
        CharacterSprites[2]->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
    }
}

void SetupCharacterModels(short dispose)
{
    int i;

    if (dispose)
    {
        for (i = 0; i < N_PLAYABLE_CHARACTERS; i++)
        {
            vfree(CharacterModelData[i]);
            DisposeModelArchive(CharacterModels[i]);
            vfree(CharacterWeaponData[i]);
            DisposeMotionManager(CharacterMotions[i]);
            DisposeOrnament(CharacterWeapons[i]);
        }
        DisposeOrnament(CharacterWeapons[2]);
        vfree(CharacterMotionData);
        return;
    }
    LoadTIMpackAndFree(FileRead(path_menu_rikimaru_texture));
    CharacterModelData[RIKIMARU_0] = FileRead(path_menu_rikimaru_model);
    CharacterModels[RIKIMARU_0] =
        LoadModelArchive(CharacterModelData[RIKIMARU_0], &World);
    CharacterModels[RIKIMARU_0]->locate.coord.t[0] = -1100;
    CharacterModels[RIKIMARU_0]->locate.coord.t[1] = 800;
    LoadTIMpackAndFree(FileRead(path_menu_ayame_texture));
    CharacterModelData[AYAME_0] = FileRead(path_menu_ayame_model);
    CharacterModels[AYAME_0] =
        LoadModelArchive(CharacterModelData[AYAME_0], &World);
    CharacterModels[AYAME_0]->locate.coord.t[0] = 1100;
    CharacterModels[AYAME_0]->locate.coord.t[1] = 800;
    CharacterWeaponData[RIKIMARU_0] = FileRead(path_menu_ninja_sword);
    CharacterWeapons[0] = LoadOrnament(CharacterWeaponData[RIKIMARU_0]);
    CharacterWeaponData[AYAME_0] = FileRead(path_menu_kodachi);
    CharacterWeapons[1] = LoadOrnament(CharacterWeaponData[AYAME_0]);
    CharacterWeapons[2] = LoadOrnament(CharacterWeaponData[AYAME_0]);
    GsInitCoordinate2(&CharacterModels[RIKIMARU_0]->object[MODEL_PART_WEAPON_HAND_0]->locate,
                      &CharacterWeapons[0]->locate);
    GsInitCoordinate2(&CharacterModels[AYAME_0]->object[MODEL_PART_WEAPON_HAND_0]->locate,
                      &CharacterWeapons[1]->locate);
    GsInitCoordinate2(&CharacterModels[AYAME_0]->object[MODEL_PART_WEAPON_HAND_1]->locate,
                      &CharacterWeapons[2]->locate);
    ViewInfo.vpx = ViewInfo.vpy = ViewInfo.vrx = ViewInfo.vry = ViewInfo.vrz = 0;
    ViewInfo.vpz = -4000;
    GsSetRefView2(&ViewInfo);
    CharacterMotionData = FileRead(path_menu_character_motion);
    CommonMotion = LoadMotion(CharacterMotionData);
    SetupMotionRegist(RikimaruSelectionMotions);
    SetupMotionRegist(AyameSelectionMotions);
    CharacterMotions[RIKIMARU_0] =
        SetupMotionManager(CharacterModels[RIKIMARU_0], RikimaruSelectionMotions);
    CharacterMotions[AYAME_0] =
        SetupMotionManager(CharacterModels[AYAME_0], AyameSelectionMotions);
}

/* MENU string storage. Keep the retail spelling and development paths. */
char CharacterJapaneseArchive[] = "K:\\WORK\\CDIMAGE\\DEMO\\player\\play_j.arc";
char CharacterItalianArchive[] = "K:\\WORK\\CDIMAGE\\DEMO\\player\\play_i.arc";
char CharacterFrenchArchive[] = "K:\\WORK\\CDIMAGE\\DEMO\\player\\play_f.arc";
char CharacterEnglishArchive[] = "K:\\WORK\\CDIMAGE\\DEMO\\player\\play_e.arc";
char path_menu_rikimaru_texture[] = "K:\\WORK\\CDIMAGE\\HUMAN\\rikimaru.tpd";
char path_menu_rikimaru_model[] = "K:\\WORK\\CDIMAGE\\HUMAN\\rikimaru.mad";
char path_menu_ayame_texture[] = "K:\\WORK\\CDIMAGE\\HUMAN\\ayame.tpd";
char path_menu_ayame_model[] = "K:\\WORK\\CDIMAGE\\HUMAN\\ayame.mad";
char path_menu_ninja_sword[] = "K:\\WORK\\CDIMAGE\\HUMAN\\WEAPON\\ninja.tmd";
char path_menu_kodachi[] = "K:\\WORK\\CDIMAGE\\HUMAN\\WEAPON\\kodati.tmd";
char path_menu_character_motion[] = "K:\\WORK\\CDIMAGE\\HUMAN\\motion\\stage9.amd";
