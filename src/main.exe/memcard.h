#ifndef MEMCARD_H
#define MEMCARD_H

#include "tim.h"

/* MEMCARD.C's one-card-block size, recovered from PSX.SYM. */
enum
{
    BLOCKSIZE = 8192,
    CARD_FILE_BLOCKS = 1
};

/* INFOVIEW's save/load menu contains the two choices "disk" (0) and
 * "card" (1). SaveSI/LoadSI dispatch on that value. */
enum save_storage
{
    SAVE_STORAGE_DISK = 0,
    SAVE_STORAGE_CARD = 1
};

/* The game consistently addresses the first card channel. Psy-Q encodes
 * other ports/slots in this integer, so keep it distinct from operation
 * modes and result codes at the API boundary. */
enum memcard_channel
{
    MEMCARD_CHANNEL_0 = 0
};

/* MemCardSync(0) waits for completion; a nonzero mode only polls the
 * library state. Every game-side operation uses the blocking form. */
enum memcard_sync_mode
{
    MEMCARD_SYNC_BLOCKING = 0,
    MEMCARD_SYNC_POLL = 1
};

enum memcard_open_mode
{
    MEMCARD_OPEN_READ_ONLY = 1
};

/* Fixed 16-colour, 16x16 TIM layout used by the three memory-card icon
 * frames. The concrete payload sizes make the two variable TIM blocks a
 * fully typed file. */
typedef struct TCardIconCLUTBlock TCardIconCLUTBlock;
struct TCardIconCLUTBlock
{
    u32 byte_size;
    TIMBlockPosition position;
    TIMBlockSize size;
    u16 colors[CARD_ICON_CLUT_COLORS];
};

typedef struct TCardIconPixelBlock TCardIconPixelBlock;
struct TCardIconPixelBlock
{
    u32 byte_size;
    TIMBlockPosition position;
    TIMBlockSize size;
    u8 pixels[CARD_ICON_BITMAP_SIZE];
};

typedef struct TCardIconTIM TCardIconTIM;
struct TCardIconTIM
{
    u32 id;
    u32 mode;
    TCardIconCLUTBlock clut;
    TCardIconPixelBlock image;
};

#define CARD_ICON_TIM_CLUT(icon) \
    ((u8 *)((TCardIconTIM *)(icon))->clut.colors)
#define CARD_ICON_TIM_PIXELS(icon) \
    ((u8 *)((TCardIconTIM *)(icon))->image.pixels)
#define CARD_ICON_TIM_END(icon) \
    ((u8 *)&((TCardIconTIM *)(icon))->image.pixels[CARD_ICON_BITMAP_SIZE])

/* Save UI states shared by update_card_message_ and update_card_screen_. */
typedef s16 card_state;
enum card_state
{
    CARD_STATE_EXIT = -1,
    CARD_STATE_SHOW_CHECKING = 0,
    CARD_STATE_CHECK_WAIT_1 = 1,
    CARD_STATE_CHECK_WAIT_2 = 2,
    CARD_STATE_PREPARE_CHECK = 3,
    CARD_STATE_CHECK = 4,
    CARD_STATE_NO_CARD = 10,
    CARD_STATE_EXIT_NO_CARD = 11,
    CARD_STATE_RESTART_NO_CARD = 12,
    CARD_STATE_DAMAGED = 20,
    CARD_STATE_EXIT_DAMAGED = 21,
    CARD_STATE_RESTART_DAMAGED = 22,
    CARD_STATE_FORMAT_PROMPT = 30,
    CARD_STATE_BEGIN_FORMAT = 31,
    CARD_STATE_CANCEL_FORMAT = 32,
    CARD_STATE_FORMAT_WAIT_1 = 33,
    CARD_STATE_FORMAT_WAIT_2 = 34,
    CARD_STATE_FORMAT = 35,
    CARD_STATE_FORMAT_FAILED = 36,
    CARD_STATE_RESTART_FORMAT_FAILED = 37,
    CARD_STATE_FORMAT_COMPLETE = 38,
    CARD_STATE_CARD_READY = 40,
    CARD_STATE_FILE_CHECK_WAIT_1 = 41,
    CARD_STATE_FILE_CHECK_WAIT_2 = 42,
    CARD_STATE_CHECK_SAVE_FILE = 43,
    CARD_STATE_BEGIN_NEW_SAVE = 50,
    CARD_STATE_NEW_SAVE_WAIT_1 = 51,
    CARD_STATE_NEW_SAVE_WAIT_2 = 52,
    CARD_STATE_WRITE_NEW_SAVE = 53,
    CARD_STATE_WRITE_COMPLETE = 54,
    CARD_STATE_FINISH_SAVE = 55,
    CARD_STATE_WRITE_FAILED = 56,
    CARD_STATE_WRITE_FAILURE_ACKNOWLEDGED = 57,
    CARD_STATE_OVERWRITE_GAME_DATA_PROMPT = 60,
    CARD_STATE_BEGIN_OVERWRITE = 61,
    CARD_STATE_CANCEL_OVERWRITE = 62,
    CARD_STATE_OVERWRITE_WAIT_1 = 63,
    CARD_STATE_OVERWRITE_WAIT_2 = 64,
    CARD_STATE_WRITE_OVERWRITE = 65,
    CARD_STATE_NOT_ENOUGH_SPACE_PROMPT = 70,
    CARD_STATE_EXIT_NOT_ENOUGH_SPACE = 71,
    CARD_STATE_RESTART_NOT_ENOUGH_SPACE = 72,
    CARD_STATE_CANNOT_SAVE_PROMPT = 90,
    CARD_STATE_EXIT_WITHOUT_SAVE = 91,
    CARD_STATE_RESTART_SAVE = 92,
    CARD_STATE_SAVE_COMPLETE_EXIT = 99
};

/* Message pages selected by the save UI state machine. */
typedef u16 card_page;
enum card_page
{
    CARD_PAGE_NONE = 0,
    CARD_PAGE_DAMAGED = 2,
    CARD_PAGE_FORMAT_PROMPT = 3,
    CARD_PAGE_WRITING = 7,
    CARD_PAGE_WRITE_COMPLETE = 9,
    CARD_PAGE_FORMATTING = 10,
    CARD_PAGE_FORMAT_COMPLETE = 11,
    CARD_PAGE_FORMAT_FAILED = 12,
    CARD_PAGE_WRITE_FAILED = 14,
    CARD_PAGE_CHECKING = 16,
    CARD_PAGE_NO_CARD_SAVE_WARNING = 18,
    CARD_PAGE_CANNOT_SAVE_PROMPT = 20,
    CARD_PAGE_NO_CARD_CANNOT_SAVE_PROMPT = 24,
    CARD_PAGE_DAMAGED_CANNOT_SAVE_PROMPT = 25,
    CARD_PAGE_NOT_ENOUGH_SPACE_CANNOT_SAVE_PROMPT = 26,
    CARD_PAGE_OVERWRITE_GAME_DATA_PROMPT = 44
};

/* Psy-Q writes operation results as words. The game's MEMCARD.C wrappers
 * expose the same values through their original signed-short return ABI. */
typedef s16 card_result;
enum card_result
{
    CARD_RESULT_SUCCESS = 0,
    CARD_RESULT_NO_CARD = 1,
    CARD_RESULT_DAMAGED = 2,
    CARD_RESULT_NEW_CARD = 3,
    CARD_RESULT_UNFORMATTED = 4,
    CARD_RESULT_FILE_NOT_FOUND = 5,
    CARD_RESULT_FILE_EXISTS = 6,
    CARD_RESULT_FULL = 7
};

/* Psy-Q LIBMCRD interface used by both MEMCARD.C and INFOVIEW.C. */
extern enum card_result MemCardAccept(enum memcard_channel channel);
extern enum card_result MemCardOpen(enum memcard_channel channel, char *path,
                                    enum memcard_open_mode mode);
extern void MemCardClose(void);
extern enum card_result MemCardExist(enum memcard_channel channel);
extern enum card_result MemCardCreateFile(enum memcard_channel channel,
                                          char *name, s32 blocks);
extern enum card_result MemCardDeleteFile(enum memcard_channel channel,
                                          char *path);
extern enum card_result MemCardReadFile(enum memcard_channel channel,
                                        char *name, void *data, s32 offset,
                                        s32 size);
extern enum card_result MemCardWriteFile(enum memcard_channel channel,
                                         char *name, void *data, s32 offset,
                                         s32 size);
extern enum card_result MemCardFormat(enum memcard_channel channel);
extern s32 MemCardSync(enum memcard_sync_mode mode, s32 *command,
                       enum card_result *result);

extern void SaveSI(enum save_storage storage, u8 *name, void *data, s32 size);
extern void *LoadSI(enum save_storage storage, u8 *name);

/* MEMCARD.C-private originally; extern because that source is split here. */
extern unsigned char *TENCHU_ID;
/* INFOVIEW.C-private originally; extern because that source is split here. */
extern unsigned char *CID;

#endif
