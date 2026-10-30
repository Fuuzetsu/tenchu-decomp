#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/font.h"

u16 TrialDefaultTiles[TRIAL_THEME_COUNT] = {
    0, 0, 0, 0, 11, 0, 0, 1, 0, 0, 0, 0
};

/* Character IDs are shared with the engine; labels are the editor's own
 * Japanese names, including the herb objective and final fallback. */
typedef struct TrialCharacterLabel
{
    s16 kind;
    char *text;
} TrialCharacterLabel;

static TrialCharacterLabel CharacterLabels[] = {
    { RIKIMARU_0, "\203\166\203\214\203\103\203\204\201\133" }, /* プレイヤー */
    { TONO, "\223\141\201\100" }, /* 殿　 */
    { HIME, "\213\145\225\120" }, /* 菊姫 */
    { TAZ, "\226\276\222\161\223\143\222\337" }, /* 明智田鶴 */
    { ECHIGOYA, "\211\172\214\343\211\256\223\277\225\272\211\161" }, /* 越後屋徳兵衛 */
    { HANBE, "\215\262\201\130\226\330\224\274\225\272\211\161" }, /* 佐々木半兵衛 */
    { GOO, "\213\364\201\100" }, /* 偶　 */
    { ANI, "\226\276\222\161\220\353\217\134\230\131" }, /* 明智尖十郎 */
    { KATAOKA_KATAYUMI, "\225\320\211\252\220\114\211\161\226\345" }, /* 片岡伸衛門 */
    { ON, "\211\205\201\100" }, /* 怨　 */
    { BALMA, "\203\157\203\213\203\175\201\133" }, /* バルマー */
    { KUMA_0, "\214\106\201\100" }, /* 熊　 */
    { HIKONE, "\225\106\215\252\226\355\213\343\230\131" }, /* 彦根弥九郎 */
    { NINJA_0, "\213\123\211\101" }, /* 鬼陰 */
    { KERAI_KATANA, "\211\306\227\210\201\136\223\201" }, /* 家来／刀 */
    { KERAI_YARI, "\211\306\227\210\201\136\221\204" }, /* 家来／槍 */
    { KERAI_YUMI, "\211\306\227\210\201\136\213\174" }, /* 家来／弓 */
    { ROUNIN_KATANA, "\230\121\220\154\201\136\223\201" }, /* 浪人／刀 */
    { ROUNIN_1YARI, "\230\121\220\154\201\136\221\204" }, /* 浪人／槍 */
    { ROUNIN_YUMI, "\230\121\220\154\201\136\213\174" }, /* 浪人／弓 */
    { NINJAB, "\224\105\216\322\201\136\223\201" }, /* 忍者／刀 */
    { NINJAA, "\224\105\216\322\201\136\222\334" }, /* 忍者／爪 */
    { KUNOITI, "\213\343\203\155\210\352" }, /* 九ノ一 */
    { ROUBAN_KATANA, "\230\123\224\324\201\136\223\201" }, /* 牢番／刀 */
    { ROUBAN_SASUMATA, "\230\123\224\324\201\136\221\204" }, /* 牢番／槍 */
    { ROUBAN_YUMI, "\230\123\224\324\201\136\213\174" }, /* 牢番／弓 */
    { ASIGARU_KATANA, "\221\253\214\171\201\136\223\201" }, /* 足軽／刀 */
    { ASIGARU_YARI, "\221\253\214\171\201\136\221\204" }, /* 足軽／槍 */
    { ASIGARU_YUMI, "\221\253\214\171\201\136\213\174" }, /* 足軽／弓 */
    { SISI_KATANA, "\216\165\216\155\201\136\223\201" }, /* 志士／刀 */
    { SISI_YARI, "\216\165\216\155\201\136\221\204" }, /* 志士／槍 */
    { SISI_YUMI, "\216\165\216\155\201\136\213\174" }, /* 志士／弓 */
    { MANJI, "\203\175\203\223\203\127\217\103\215\163\221\155" }, /* マンジ修行僧 */
    { MANJI5_KEITOU, "\203\175\203\223\203\127\221\155\201\136\223\201" }, /* マンジ僧／刀 */
    { MANJI5_ENGETU, "\203\175\203\223\203\127\221\155\201\136\221\204" }, /* マンジ僧／槍 */
    { MANJI5_YUMI, "\203\175\203\223\203\127\221\155\201\136\213\174" }, /* マンジ僧／弓 */
    { PIRATEA_HALBERT, "\212\103\221\257\201\136\221\204" }, /* 海賊／槍 */
    { PIRATEA_TEPPO, "\212\103\221\257\201\136\217\145" }, /* 海賊／銃 */
    { PIRATEB, "\212\103\221\257\201\136\236\236\226\137" }, /* 海賊／棍棒 */
    { TENGU_JYUTE, "\223\126\213\347\201\136\217\254\223\201" }, /* 天狗／小刀 */
    { TENGU_KON, "\223\126\213\347\201\136\226\137" }, /* 天狗／棒 */
    { TENGU_YUMI, "\223\126\213\347\201\136\213\174" }, /* 天狗／弓 */
    { WOLF, "\230\124\201\100" }, /* 狼　 */
    { KIMEN13, "\213\123\201\136\223\201" }, /* 鬼／刀 */
    { ONIKERAI, "\213\123\201\136\221\204" }, /* 鬼／槍 */
    { ONIKUNO_EN, "\224\312\216\341\201\136\217\254\223\201" }, /* 般若／小刀 */
    { ONIKUNO_CROWR, "\224\312\216\341\201\136\222\334" }, /* 般若／爪 */
    { KABANE_HOUTOU, "\216\162\225\272\201\136\223\201" }, /* 屍兵／刀 */
    { KABANE_KABUTUTI, "\216\162\225\272\201\136\221\204" }, /* 屍兵／槍 */
    { KABANE_YUMI, "\216\162\225\272\201\136\213\174" }, /* 屍兵／弓 */
    { MOURYO_0, "\202\340\202\244\202\350\202\345\202\244\225\272" }, /* もうりょう兵 */
    { FIREDOG, "\216\147\202\242\226\202" }, /* 使い魔 */
    { NAKAI, "\222\254\226\272" }, /* 町娘 */
    { JOCHU, "\222\254\226\272" }, /* 町娘 */
    { CHONIN, "\222\254\220\154" }, /* 町人 */
    { TRIAL_HERB_OBJECT, "\213\103\226\275\221\220" }, /* 気命草 */
    { CHARACTER_KIND_END, "\202\273\202\314\221\274" }, /* その他 */
};

void InitNewTrialMission(void)
{
    memset(&TrialMission, 0, sizeof(TrialMission));
    TrialMission.header.flags = 0;
    TrialMission.header.theme = 0;
    FillTrialMissionMap();
    memset(TrialMission.header.records, 0xff, sizeof(TrialMission.header.records));
    SetupTrialObjectivePlacements(1);
}

/* The readable draft is four bytes short: row-index normalization differs
 * from the original. Keep the original function until this compiler pattern
 * is recovered; it is deliberately excluded from the matching-C count. */
#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/trial.exe/asm/nonmatchings/EDITORUTIL", FillTrialMissionMap);
#else
void FillTrialMissionMap(void)
{
    short x = 0;
    short z = 0;
    TrialMissionData *mission = &TrialMission;
    u16 tile = TrialDefaultTiles[mission->header.theme];

    do
    {
        mission->map[z][x][0] = tile;
        mission->map[z][x][1] = TRIAL_EMPTY_TILE;
        if (++x >= TRIAL_MAP_WIDTH)
        {
            x = 0;
            z++;
        }
    } while (z < TRIAL_MAP_HEIGHT);
}

#endif

/* The placement validator uses this terrain-kind classification: the five
 * regular support classes return 1, class 0xa0 returns 0, and others -1.
 * The same switch is inlined in the live placement-validation path. */
short ClassifyTrialTerrainSupport(short kind)
{
    switch (kind)
    {
    case 0x10:
    case 0x20:
    case 0x30:
    case 0x40:
    case 0x50:
        return 1;
    case 0xa0:
        return 0;
    default:
        return -1;
    }
}

/* Upload one TIM and expose its vertical frames as ordinary LIBGS sprites.
 * With zero frames, only the full-image sprite at index zero is initialized. */
void LoadTrialSpriteStrip(ArcFile *archive, GsSPRITE *sprites,
                          short image_index, short frame_count)
{
    GsIMAGE image;
    u_long *tim;
    Sprite3D *sprite;
    short i;

    tim = get_tim_from_archive(archive, image_index);
    GetTIMInfo(tim, &image);
    LoadTIM(tim);
    if (frame_count)
        image.ph /= frame_count;
    sprite = SetupSprite(NULL, &image);
    sprites[0] = sprite->sprite;
    vfree(sprite);
    for (i = 0; i < frame_count; i++)
    {
        sprites[i] = sprites[0];
        sprites[i].v += i * image.ph;
    }
}

/* The editor always playtests as Rikimaru, using its unsaved mission data. */
void PlaytestTrialMission(void)
{
    SetupTrialMission(RIKIMARU_0, 0);
    RunTrialMission(RIKIMARU_0);
}

char *PrepareTrialCharacterLabel(short kind)
{
    short i = 0;

    while (CharacterLabels[i].kind != CHARACTER_KIND_END)
    {
        if (CharacterLabels[i].kind == kind)
            break;
        i++;
    }
    SetupTelop(CharacterLabels[i].text, 0);
    return CharacterLabels[i].text;
}
