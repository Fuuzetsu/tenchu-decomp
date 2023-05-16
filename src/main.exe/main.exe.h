#include <psxsdk/libgs.h>

typedef u16 buttons_held;

extern void FUN_8001ada4(void);
extern buttons_held HELD_BUTTONS;

typedef struct
{
    char *choice_name;
    u32 choice_number;
} debug_menu_choice;

typedef struct some_tmd_map_link_struct some_tmd_map_link_struct;
struct some_tmd_map_link_struct
{
    GsCOORDINATE2 gscoord2;
    GsDOBJ2 gsdobj;
};

typedef enum weapon_kind weapon_kind;

enum weapon_kind
{
    NO_WEAPON = 0x00,
    KUMA_WEAPON = 0x01,
    NINJA_0_1_WEAPON = 0x02,
    RAT_CAT_DOG_WEAPON = 0x03,
    KODATI = 0x04,
    JYUTE = 0x05,
    JYUTEB = 0x06,
    EN = 0x07,
    ENB = 0x08,
    ANDON = 0x09,
    JYURUR = 0x0a,
    JYURUL = 0x0b,
    KOZUKA = 0x0c,
    IKARI = 0x10,
    BOU = 0x11,
    SABRE = 0x12,
    NINJA = 0x13,
    KEITOU = 0x14,
    KEITOUB = 0x15,
    KATANA_0 = 0x16,
    SAYA_0 = 0x17,
    TUKAH_0 = 0x18,
    HOUTOU = 0x19,
    SAYA_1 = 0x1a,
    TUKAH_1 = 0x1b,
    KATANA_1 = 0x1c,
    SAYAN = 0x1d,
    TUKAN = 0x1e,
    KATANA_2 = 0x1f,
    CROWR = 0x20,
    CROWL = 0x21,
    YARI = 0x22,
    KABUTUTI = 0x23,
    SASUMATA = 0x24,
    HALBERT = 0x25,
    KON = 0x26,
    NAGI = 0x27,
    ENGETU = 0x28,
    SEVEN = 0x29,
    KATANAL = 0x2a,
    SAYAL = 0x2b,
    TUKAL = 0x2c,
    TUKAANI = 0x2d,
    TEPPO = 0x30,
    GUN = 0x31,
    YUMI = 0x32,
    YAB_0 = 0x33,
    YAZUTU_0 = 0x34,
    KATAYUMI = 0x35,
    YAB_1 = 0x36,
    YAZUTU_1 = 0x37,
    END_OF_WEAPON_KIND_MARKER = 0xffff,
};

typedef enum character_kind character_kind;
enum character_kind
{
    RIKIMARU_0 = 0x00,
    AYAME_0 = 0x01,
    RIKIMARU_1 = 0x02,
    AYAME_1 = 0x03,
    ROUJYU = 0x04,
    HIME = 0x05,
    TONO = 0x06,
    KERAI_KATANA = 0x07,
    KERAI_YARI = 0x08,
    KERAI_YUMI = 0x09,
    ROUNIN_KATANA = 0x10,
    ROUNIN_1YARI = 0x11,
    ROUNIN_YUMI = 0x12,
    ROUBAN_KATANA = 0x13,
    ROUBAN_SASUMATA = 0x14,
    ROUBAN_YUMI = 0x15,
    ASIGARU_KATANA = 0x16,
    ASIGARU_YARI = 0x17,
    ASIGARU_YUMI = 0x18,
    SISI_KATANA = 0x19,
    SISI_YARI = 0x1a,
    SISI_YUMI = 0x1b,
    NINJAA = 0x20,
    NINJAB = 0x21,
    KUNOITI = 0x22,
    MANJI = 0x30,
    MANJI5_KEITOU = 0x31,
    MANJI5_ENGETU = 0x32,
    MANJI5_YUMI = 0x33,
    PIRATEA_HALBERT = 0x40,
    PIRATEA_TEPPO = 0x41,
    PIRATEB = 0x42,
    TENGU_JYUTE = 0x50,
    TENGU_KON = 0x51,
    TENGU_YUMI = 0x52,
    KIMEN13 = 0x60,
    ONIKERAI = 0x61,
    ONIKUNO_EN = 0x62,
    ONIKUNO_CROWR = 0x63,
    KABANE_HOUTOU = 0x70,
    KABANE_KABUTUTI = 0x71,
    KABANE_YUMI = 0x72,
    MOURYO_0 = 0x73,
    MOURYO_1 = 0x74,
    ECHIGOYA = 0x80,
    HANBE = 0x81,
    TUZI = 0x82,
    GOO = 0x83,
    ON = 0x84,
    BALMA = 0x85,
    KUMA_0 = 0x86,
    NINJA_0 = 0x87,
    MEIOU = 0x88,
    KUMA_1 = 0x89,
    NINJA_1 = 0x8a,
    ANI = 0x8b,
    TAZ = 0x8c,
    HIKONE = 0x8d,
    KATAOKA_KATAYUMI = 0x8e,
    KATAOKA_KOZUKA = 0x8f,
    CHONIN = 0x90,
    JOCHU = 0x91,
    MUSUME = 0x92,
    ZAININ = 0x93,
    BIZENYA = 0x94,
    NAKAI = 0x95,
    MEKAKE = 0x96,
    RAT = 0xa0,
    CAT = 0xa1,
    DOG_0 = 0xa2,
    DOG_1 = 0xa3,
    WOLF = 0xa4,
    FIREDOG = 0xa5,
    S1 = 0xa6,
    S2 = 0xa7,
    ARROW = 0xa8,
    NINKEN = 0xa9,
    END_OF_CHARACTER_KIND_MARKER = 0xffff,
};

typedef enum character_status character_status;
enum character_status
{
    DEFAULT_STATE = 0x00,
    POSING_OR_PLAYING_SOME_CUTSCENE_ANIMATION_THING = 0x01,
    SLOW_WALKING = 0x02,
    SWIMMING = 0x03,
    AIMING_KAGINAWA = 0x04,
    IDLING = 0x05,
    MOVING = 0x06,
    ATTACKING = 0x07,
    FALLING_OR_PICKING_OR_DROPPING_ITEM = 0x08,
    JUMPING = 0x09,
    HANGING = 0x0a,
    CROUCHING = 0x0b,
    PRESSED_AGAINST_WALL = 0x0c,
    AIMING_WEAPON = 0x0e,
    USING_ITEM = 0x0f,
    RECOVERY_ANIMATION = 0x10,
    DEAD = 0x11,
};

typedef enum stage_rank stage_rank;
enum stage_rank
{
    RANK_THUG = 0x00,
    RANK_NOVICE = 0x01,
    RANK_NINJA = 0x02,
    RANK_MASTER_NINJA = 0x03,
    RANK_GRAND_MASTER = 0x04,
};

// typedef enum chosen_stage chosen_stage;
// enum chosen_stage
// {
//     PUNISH_THE_EVIL_MERCHANT = 0x01,
//     DELIVER_THE_SECRET_MESSAGE = 0x02,
//     RESCUE_THE_CAPTIVE_NINJA = 0x03,
//     INFILTRATE_THE_MANJI_CULT = 0x04,
//     DESTROY_THE_FOREIGN_PIRATES = 0x05,
//     CURE_THE_PRINCESS = 0x06,
//     RECLAIM_THE_CASTLE = 0x07,
//     FREE_THE_PRINCESS = 0x08,
//     TRAINING = 0x09,
//     CROSS_THE_CHECKPOINT = 0x0a,
//     EXECUTE_THE_CORRUPT_MINISTER = 0x0b,
//     NOT_CHOSEN = 0xffff,
// };

typedef enum chosen_stage_ix chosen_stage_ix;
enum chosen_stage_ix
{
    PUNISH_THE_EVIL_MERCHANT = 0x00,
    DELIVER_THE_SECRET_MESSAGE = 0x01,
    RESCUE_THE_CAPTIVE_NINJA = 0x02,
    INFILTRATE_THE_MANJI_CULT = 0x03,
    DESTROY_THE_FOREIGN_PIRATES = 0x04,
    CURE_THE_PRINCESS = 0x05,
    RECLAIM_THE_CASTLE = 0x06,
    FREE_THE_PRINCESS = 0x07,
    TRAINING = 0x08,
    CROSS_THE_CHECKPOINT = 0x09,
    EXECUTE_THE_CORRUPT_MINISTER = 0x0a,
};

// int debug_menu_choose(char *screen_header, debug_menu_choice *choices, char *param_3);