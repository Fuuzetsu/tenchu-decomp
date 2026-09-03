#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "infoview.h"
#include "layout_save.h"
#include "memcard.h"

#define N_MUSIC_IDS 161
#define FILE_SLOT_INITIAL_SELECTION 16

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void FileOption(void);
 *     INFOVIEW.C:1021, 119 src lines, frame 7888 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct TAdtSelect [17] ItemName
 *     stack sp+152    struct TAdtSelect [5] SelectIO
 *     stack sp+192    struct TAdtSelect [18] SelectSlot
 *     reg   $s2       int TargetIO
 *     reg   $s1       unsigned char * fname
 *     reg   $s0       void * pBuf
 *     stack sp+336    unsigned char [7000] Buf
 *     reg   $s1       int i
 *     stack sp+7552   unsigned char [26][12] msg
 *     stack sp+7336   struct TAdtSelect [27] targets
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern int StageID;
 *     extern short EngageLevel;
 *     extern unsigned char gNannido;
 * END PSX.SYM */

extern MusicTrack MusicByStage[N_STAGE_CONFIGS];
/* Unknown bound keeps this symbol out of small-data addressing. */
extern u8 STAGE_LAYOUT_NUMBER[];

extern char str_file_option[]; /* "file option" */
extern char msg_load_ok[]; /* "load ok?" */
extern char msg_load_no[]; /* "load no?" */
extern char msg_save_ok[]; /* "save ok?" */
extern char msg_save_no[]; /* "save no?" */
extern char str_select_music[]; /* "select music" */
extern char str_layout_no[]; /* "layout no" */
extern char fmt_num_2[]; /* "%d" */

extern void lePackEnemyLayout(void *buf, long size);
extern void load_save_slot_(enum save_storage storage, u8 *name);
extern void InitializeImage(void);
extern void SetupStageSequence(void);
extern void CVAsetup(void);
extern void debug_menu_file_animation_test(void);
extern void load_layout(s32 no);

void FileOption(void)
{
    enum FileOptionChoice
    {
        SAVE = 0,
        LOAD = 1,
        STOCK_IMAGES = 2,
        DEBUG_PRINT = 3,
        PLAY_MUSIC = 4,
        STOP_MUSIC = 5,
        EVENT_UPDATE = 6,
        ANIM_UPDATE = 7,
        /* Retail inserts animation test, shifting the remaining demo cases. */
        TEST_ANIMATION = 8,
        TEST_MUSIC = 9,
        EASY_GAME = 0xA,
        NORMAL_GAME = 0xB,
        HARD_GAME = 0xC,
        STOCK_LAYOUT = 0xD
    };
    s16 n;
    enum save_storage storage;
    u8 *fname;
    LayoutSaveData *pBuf;
    s32 k;
    s32 i;
    TAdtSelect *targets;
    u8(*messages)[5];
    TAdtSelect ItemName[20];
    TAdtSelect SelectIO[5];
    TAdtSelect SelectSlot[18];
    LayoutSaveData Buf;

    __builtin_memcpy(ItemName, DEBUG_MENU_FILE_CHOICES, sizeof(ItemName));
    __builtin_memcpy(SelectIO, DEBUG_MENU_SAVE_LOAD_CHOICES, sizeof(SelectIO));
    __builtin_memcpy(SelectSlot, DEBUG_MENU_FILE_LAYOUT_CHOICES, sizeof(SelectSlot));
    n = AdtSelect(str_file_option, ItemName, 0);
    if (n == ADT_SELECT_CANCEL)
        return;
    switch (n)
    {
    case LOAD:
        storage = AdtSelect(msg_load_ok, SelectIO, 3);
        if (storage == ADT_SELECT_CANCEL)
            return;
        fname = (u8 *)AdtSelect(msg_load_no, SelectSlot,
                                FILE_SLOT_INITIAL_SELECTION);
        if (fname == (u8 *)ADT_SELECT_CANCEL)
            return;
        /* The caller-side mask is in the bytes (the callee masks again;
         * the SAVE twin passes storage unmasked): retail's own. */
        load_save_slot_(storage & 0xFF, fname);
        leLayoutEnemy(ENEMY_LAYOUT_EDIT);
        break;
    case SAVE:
        storage = AdtSelect(msg_save_ok, SelectIO, 3);
        if (storage != ADT_SELECT_CANCEL)
        {
            fname = (u8 *)AdtSelect(msg_save_no, SelectSlot,
                                    FILE_SLOT_INITIAL_SELECTION);
            if (fname != (u8 *)ADT_SELECT_CANCEL)
            {
                pBuf = &Buf;
                lePackEnemyLayout(pBuf->enemies, ENESIZE);
                PackItemLayout(pBuf->items, ITEMSIZE);
                SaveSI(storage, fname, pBuf, sizeof(Buf));
            }
        }
        break;
    case STOCK_IMAGES:
        InitializeImage();
        break;
    case DEBUG_PRINT:
        SystemFlag ^= SYSFLAG_DEBUGPRINT;
        break;
    case PLAY_MUSIC:
        __builtin_memcpy((MusicTrack *)&Buf, MusicByStage,
                         sizeof(MusicTrack) * N_STAGE_CONFIGS);
        _PlayMusic(((MusicTrack *)&Buf)[StageID], CDA_REPEAT);
        break;
    case STOP_MUSIC:
        CdaStop();
        break;
    case EVENT_UPDATE:
        SetupStageSequence();
        break;
    case ANIM_UPDATE:
        CVAsetup();
        break;
    case TEST_ANIMATION:
        debug_menu_file_animation_test();
        break;
    case TEST_MUSIC:
        targets = (TAdtSelect *)&Buf;
        messages = (u8(*)[5])((u8 *)&Buf + sizeof(TAdtSelect) *
                              (N_MUSIC_IDS + 1));
        for (i = 0; i < N_MUSIC_IDS; i++)
        {
            sprintf((char *)messages[i], fmt_num_2, i);
            targets[i].name = messages[i];
            targets[i].value = i;
        }
        {
            TAdtSelect *terminator = targets + i;

            terminator->name = 0;
        }
        PlayMusicFormID(AdtSelect(
            str_select_music, (TAdtSelect *)&Buf, 0));
        break;
    case EASY_GAME:
        EngageLevel = 3;
        gNannido = DIFFICULTY_EASY;
        break;
    case NORMAL_GAME:
        EngageLevel = 2;
        gNannido = DIFFICULTY_NORMAL;
        break;
    case HARD_GAME:
        EngageLevel = 1;
        gNannido = DIFFICULTY_HARD;
        break;
    case STOCK_LAYOUT:
        __builtin_memcpy(&Buf, DEBUG_MENU_FILE_LOAD_STOCK_LAYOUT_CHOICES,
                         sizeof(DEBUG_MENU_FILE_LOAD_STOCK_LAYOUT_CHOICES));
        k = AdtSelect(str_layout_no, (TAdtSelect *)&Buf, 0);
        if (k < 0)
            break;
        STAGE_LAYOUT_NUMBER[0] = k;
        SystemFlag &= ~SYSFLAG_RANDOM_LAYOUT;
        load_layout(STAGE_LAYOUT_NUMBER[0]);
        leLayoutEnemy(ENEMY_LAYOUT_EDIT);
        break;
    }
}
