#include <psxsdk/libgs.h>

typedef u16 buttons_held;

// One controller's input state; 14 bytes. GetRealPad indexes a
// [port][slot] table of these and reads the first (held) field.
typedef struct
{
    buttons_held held;
    u16 unk_2[6];
} controller_input;

extern void PadProc(void);
extern controller_input PadPort[][4];

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
//     CROSS_THE_CHECKPOs32 = 0x0a,
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
    CROSS_THE_CHECKPOs32 = 0x09,
    EXECUTE_THE_CORRUPT_MINISTER = 0x0a,
};

typedef enum button_mask button_mask;
enum button_mask
{
    NOTHING = 0x00,
    L2 = 0x01,
    R2 = 0x02,
    L1 = 0x04,
    R1 = 0x08,
    TRIANGLE = 0x10,
    CIRCLE = 0x20,
    CROSS = 0x40,
    SQUARE = 0x80,
    SELECT = 0x100,
    L3 = 0x200,
    R3 = 0x400,
    START = 0x800,
    UP = 0x1000,
    RIGHT = 0x2000,
    DOWN = 0x4000,
    LEFT = 0x8000,
    DUMMY_MASK = 0xffff,
};

typedef struct some_character_button_values some_character_button_values;
struct some_character_button_values
{
    u16 currently_pressed;   // enum button_mask, stored as 2 bytes
    u16 pressed_last_frame;
    u16 newly_pressed;
    s16 frames_since_new_input;
    u16 buttons_pressed_in_s16_succession[4];
};

typedef struct xyz xyz;
struct xyz
{
    s32 x;
    s32 y;
    s32 z;
};

typedef struct xyz_s xyz_s;
struct xyz_s
{
    s16 x;
    s16 y;
    s16 z;
};

typedef struct something_about_player_rotation_perhaps something_about_player_rotation_perhaps;
struct something_about_player_rotation_perhaps
{
    u8 field0_0x0;
    u8 field1_0x1;
    s16 character_rotation;
};

typedef struct some_animation_kind_ptr some_animation_kind_ptr;
struct some_animation_kind_ptr
{
    s16 animation_kind;
    s16 field1_0x2;
    void *data_ptr;
};

typedef struct char_state_related_camera_things char_state_related_camera_things;
struct char_state_related_camera_things
{
    u8 field0_0x0;
    u8 field1_0x1;
    u8 field2_0x2;
    u8 field3_0x3;
    u8 field4_0x4;
    u8 field5_0x5;
    u8 field6_0x6;
    u8 field7_0x7;
    u8 field8_0x8;
    u8 field9_0x9;
    u8 field10_0xa;
    u8 field11_0xb;
    u8 field12_0xc;
    u8 field13_0xd;
    u8 field14_0xe;
    u8 field15_0xf;
    u8 field16_0x10;
    u8 field17_0x11;
    u8 field18_0x12;
    u8 field19_0x13;
    u8 field20_0x14;
    u8 field21_0x15;
    u8 field22_0x16;
    u8 field23_0x17;
    xyz position;
    u8 field25_0x24;
    u8 field26_0x25;
    u8 field27_0x26;
    u8 field28_0x27;
    u8 field29_0x28;
    u8 field30_0x29;
    u8 field31_0x2a;
    u8 field32_0x2b;
    u8 field33_0x2c;
    u8 field34_0x2d;
    u8 field35_0x2e;
    u8 field36_0x2f;
    u8 field37_0x30;
    u8 field38_0x31;
    u8 field39_0x32;
    u8 field40_0x33;
    u8 field41_0x34;
    u8 field42_0x35;
    u8 field43_0x36;
    u8 field44_0x37;
    u8 field45_0x38;
    u8 field46_0x39;
    u8 field47_0x3a;
    u8 field48_0x3b;
    u8 field49_0x3c;
    u8 field50_0x3d;
    u8 field51_0x3e;
    u8 field52_0x3f;
    u8 field53_0x40;
    u8 field54_0x41;
    u8 field55_0x42;
    u8 field56_0x43;
    u8 field57_0x44;
    u8 field58_0x45;
    u8 field59_0x46;
    u8 field60_0x47;
    u8 field61_0x48;
    u8 field62_0x49;
    u8 field63_0x4a;
    u8 field64_0x4b;
    u8 field65_0x4c;
    u8 field66_0x4d;
    u8 field67_0x4e;
    u8 field68_0x4f;
    xyz_s fpv_camera_rotation;
    u8 field70_0x56;
    u8 field71_0x57;
    u8 field72_0x58;
    u8 field73_0x59;
    u8 field74_0x5a;
    u8 field75_0x5b;
    u8 field76_0x5c;
    u8 field77_0x5d;
    u8 field78_0x5e;
    u8 field79_0x5f;
    u8 field80_0x60;
    u8 field81_0x61;
    u8 field82_0x62;
    u8 field83_0x63;
    s16 some_num_of_things_in_something_about_current_animation;
    u8 field85_0x66;
    u8 field86_0x67;
    u8 field87_0x68;
    u8 field88_0x69;
    u8 field89_0x6a;
    u8 field90_0x6b;
    u8 field91_0x6c;
    u8 field92_0x6d;
    u8 field93_0x6e;
    u8 field94_0x6f;
    u8 field95_0x70;
    u8 field96_0x71;
    u8 field97_0x72;
    u8 field98_0x73;
    u8 field99_0x74;
    u8 field100_0x75;
    u8 field101_0x76;
    u8 field102_0x77;
};

typedef struct player_item_counts_by_name player_item_counts_by_name;
struct player_item_counts_by_name
{
    u8 picbn_kaginawa;
    u8 picbn_shuriken;
    u8 picbn_makibisi;
    u8 picbn_kusuri;
    u8 picbn_fire;
    u8 picbn_smoke;
    u8 picbn_jirai;
    u8 picbn_dokudango;
    u8 picbn_gosikimai;
    u8 picbn_nemurigusuri;
    u8 picbn_kawarimi;
    u8 picbn_hensin;
    u8 picbn_goshinfuda;
    u8 picbn_shinsoku;
    u8 picbn_rikimarukochan;
    u8 picbn_happou;
    u8 picbn_ninken;
    u8 picbn_kaengeki;
    u8 picbn_manebue;
    u8 picbn_armour;
};

typedef struct special_item_counts_by_name special_item_counts_by_name;
struct special_item_counts_by_name
{
    u8 sicbn_gun;
    u8 sicbn_yumi;
    u8 sicbn_kaen;
    u8 sicbn_lightning_bolt;
    u8 sicbn_the_world;
};

typedef union player_item_counts player_item_counts;

union player_item_counts
{
    player_item_counts_by_name pic_by_name;
    u8 pic_by_index[20];
};

typedef union special_item_counts special_item_counts;
union special_item_counts
{
    special_item_counts_by_name sic_by_name;
    u8 sic_by_index[5];
};

typedef struct something_about_current_animation something_about_current_animation;
struct something_about_current_animation
{
    s16 animation_state_perhaps;
    s16 frames_since_animation_start;   // target loads with lh (signed)
    s16 seconds_elapsed;
    u16 num_of_some_things_in_offs_24;
    s16 field4_0x8;
    s16 field5_0xa;
    char_state_related_camera_things *char_state_related_camera_things;
    u8 *field7_0x10;
    some_animation_kind_ptr *animations;
    void *field9_0x18;
};

typedef struct inventory_item_counts inventory_item_counts;
struct inventory_item_counts
{
    player_item_counts player_item_counts;
    special_item_counts special_item_counts;
};

typedef s32 (*some_char_state_function)(void);

typedef enum item_kind2 item_kind2;
enum item_kind2
{
    ITEM_KIND_2_KAGINAWA = 0x00,
    ITEM_KIND_2_SHURIKEN = 0x01,
    ITEM_KIND_2_MAKIBISI = 0x02,
    ITEM_KIND_2_KUSURI = 0x03,
    ITEM_KIND_2_FIRE = 0x04,
    ITEM_KIND_2_SMOKE = 0x05,
    ITEM_KIND_2_JIRAI = 0x06,
    ITEM_KIND_2_DOKUDANGO = 0x07,
    ITEM_KIND_2_GOSIKIMAI = 0x08,
    ITEM_KIND_2_NEMURIGUSURI = 0x09,
    ITEM_KIND_2_KAWARIMI = 0x0a,
    ITEM_KIND_2_HENSIN = 0x0b,
    ITEM_KIND_2_GOSHINFUDA = 0x0c,
    ITEM_KIND_2_SHINSOKU = 0x0d,
    ITEM_KIND_2_RIKIMARUKOCHAN = 0x0e,
    ITEM_KIND_2_HAPPOU = 0x0f,
    ITEM_KIND_2_NINKEN = 0x10,
    ITEM_KIND_2_KAENGEKI = 0x11,
    ITEM_KIND_2_MANEBUE = 0x12,
    ITEM_KIND_2_ARMOUR = 0x13,
    ITEM_KIND_2_GUN = 0x14,
    ITEM_KIND_2_YUMI = 0x15,
    ITEM_KIND_2_KAEN = 0x16,
    ITEM_KIND_2_LIGHTNING_BOLT = 0x17,
    ITEM_KIND_2_THE_WORLD = 0x18,

    ITEM_KIND_2_EXTEND = 0xffff,
};

typedef struct character_state character_state;
struct character_state
{
    u16 character_kind;   // enum character_kind, stored as 2 bytes
    u16 character_status; // enum character_status, stored as 2 bytes
    u16 some_character_marker_thing;
    s16 character_rotation_speed;
    u16 current_health;
    u16 max_health;
    s16 field6_0xc;
    s16 field7_0xe;
    some_character_button_values buttons;
    u8 field9_0x20;
    u8 field10_0x21;
    u8 field11_0x22;
    u8 field12_0x23;
    u8 field13_0x24;
    u8 field14_0x25;
    u8 field15_0x26;
    u8 field16_0x27;
    u8 field17_0x28;
    u8 field18_0x29;
    u8 field19_0x2a;
    u8 field20_0x2b;
    u8 field21_0x2c;
    u8 field22_0x2d;
    u8 field23_0x2e;
    u8 field24_0x2f;
    void *field25_0x30;
    void *field26_0x34;
    VECTOR *some_kind_of_current_position;
    something_about_player_rotation_perhaps *something_about_player_rotation_perhaps;
    xyz_s some_rotation_vector_for_tmd;
    u8 field30_0x46;
    u8 field31_0x47;
    xyz current_position;
    u8 field33_0x54;
    u8 field34_0x55;
    u8 field35_0x56;
    u8 field36_0x57;
    char_state_related_camera_things *camera_related;
    something_about_current_animation *something_about_current_animation;
    some_char_state_function *think_setting0;
    some_char_state_function *think_setting1;
    some_char_state_function *think_setting2;
    some_char_state_function *think_setting3;
    u8 field43_0x70;
    u8 field44_0x71;
    u8 field45_0x72;
    u8 field46_0x73;
    char_state_related_camera_things *another_camera_related_perhaps;
    s32 some_x_position;
    s32 some_z_position;
    s32 some_other_x_position;
    s32 some_other_z_position;
    u8 field52_0x88;
    u8 field53_0x89;
    byte field54_0x8a;
    byte field55_0x8b;
    s16 index_s32o_animation_collection;
    weapon_kind weapon_kind;
    u8 field58_0x90;
    u8 field59_0x91;
    u8 field60_0x92;
    u8 field61_0x93;
    some_tmd_map_link_struct *right_hand_active_weapon_tmd;
    some_tmd_map_link_struct *left_hand_active_weapon_tmd;
    some_tmd_map_link_struct *right_hand_inactive_weapon_tmd;
    some_tmd_map_link_struct *left_hand_inactive_weapon_tmd;
    u8 field66_0xa4;
    u8 field67_0xa5;
    u8 field68_0xa6;
    u8 field69_0xa7;
    u8 field70_0xa8;
    u8 field71_0xa9;
    u8 field72_0xaa;
    u8 field73_0xab;
    u16 character_kind_sound_mask;
    item_kind2 active_item;
    u8 field76_0xb0;
    u8 field77_0xb1;
    u8 field78_0xb2;
    u8 field79_0xb3;
    inventory_item_counts inventory;
    u8 field81_0xcd;
    u8 field82_0xce;
    u8 field83_0xcf;
};
// s32 AdtSelect(char *screen_header, debug_menu_choice *choices, char *param_3);

extern int turn_towards_player_(int x_diff, int z_diff);
extern character_state *Me_THINK_C;
extern s16 SR;
extern s16 Attrib;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern char FONT_FILE_NAME;
extern char IMAGES_PREFIX_STR;

extern void *PathFileRead(char *resource_prefix, char *resource_name);
extern void *GetTIMInfo(u_long *tim, GsIMAGE *im);
extern void LoadTIMAndFree(u_long *tim);
extern void load_font_image_into_global(GsIMAGE *image);
