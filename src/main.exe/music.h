#ifndef TENCHU_MUSIC_H
#define TENCHU_MUSIC_H

/* Logical ids stored in event data and MusicIdByTrack. The names through
 * STAGE_OPEN8A are the original demo enum. Retail's stage-10/11 ESD roots
 * request 126/127 (MUSIC_EVENT_ID_BASE + 26/27), and MusicIdByTrack maps those
 * ids onto the two physical rows immediately after STAGE9. Retail id 30 is
 * Onikage's fight theme: STAGE4/7/8.ESD request event cue 130 at his three
 * encounters, which maps to MUSIC.XA channel 2 through the final table row. */
typedef enum MusicId MusicId;
enum MusicId
{
    MUSIC_ID_STAGE1 = 0,
    MUSIC_ID_STAGE2 = 1,
    MUSIC_ID_STAGE3 = 2,
    MUSIC_ID_STAGE4 = 3,
    MUSIC_ID_STAGE5 = 4,
    MUSIC_ID_STAGE6 = 5,
    MUSIC_ID_STAGE7 = 6,
    MUSIC_ID_STAGE8 = 7,
    MUSIC_ID_STAGE9 = 8,
    MUSIC_ID_GAMEOVER = 9,
    MUSIC_ID_COMPLETE = 10,
    MUSIC_ID_KIKI = 11,
    MUSIC_ID_TITLE = 12,
    MUSIC_ID_CHARA = 13,
    MUSIC_ID_BARMAR = 14,
    MUSIC_ID_MEIOU = 15,
    MUSIC_ID_STAGE_OPEN1 = 16,
    MUSIC_ID_STAGE_OPEN2 = 17,
    MUSIC_ID_STAGE_OPEN3 = 18,
    MUSIC_ID_STAGE_OPEN4 = 19,
    MUSIC_ID_STAGE_OPEN5 = 20,
    MUSIC_ID_STAGE_OPEN6 = 21,
    MUSIC_ID_STAGE_OPEN7 = 22,
    MUSIC_ID_STAGE_OPEN8 = 23,
    MUSIC_ID_STAGE_OPEN7A = 24,
    MUSIC_ID_STAGE_OPEN8A = 25,
    MUSIC_ID_STAGE10 = 26,
    MUSIC_ID_STAGE11 = 27,
    MUSIC_ID_ONIKAGE = 30
};

/* Physical rows in retail's XA MusicTable. MusicIdByTrack maps the logical ids
 * above onto these rows. */
typedef enum MusicTrack MusicTrack;
enum MusicTrack
{
    MUSIC_TRACK_STAGE1 = 0,
    MUSIC_TRACK_STAGE2 = 1,
    MUSIC_TRACK_STAGE3 = 2,
    MUSIC_TRACK_STAGE4 = 3,
    MUSIC_TRACK_STAGE5 = 4,
    MUSIC_TRACK_STAGE6 = 5,
    MUSIC_TRACK_STAGE7 = 6,
    MUSIC_TRACK_STAGE8 = 7,
    MUSIC_TRACK_STAGE9 = 8,
    MUSIC_TRACK_STAGE10 = 9,
    MUSIC_TRACK_STAGE11 = 10,
    MUSIC_TRACK_GAMEOVER = 11,
    MUSIC_TRACK_COMPLETE = 12,
    MUSIC_TRACK_KIKI = 13,
    MUSIC_TRACK_TITLE = 14,
    MUSIC_TRACK_CHARA = 15,
    MUSIC_TRACK_BARMAR = 16,
    MUSIC_TRACK_MEIOU = 17,
    MUSIC_TRACK_ONIKAGE = 18,
    MUSIC_TRACK_COUNT = 19
};

/* XA voice ids occupy three namespaces. Values below 100 select the current
 * language's EVENT_<language>.XA table; 100..199 select INTRO.XA, and values
 * from 200 select TORA.XA. Each table stores bank-local ids in its first byte,
 * so PlayVoice removes the corresponding base before searching it. */
enum voice_id_namespace
{
    VOICE_ID_INTRO_BASE = 100,
    VOICE_ID_TORA_BASE = 200
};

/* _PlayMusic's cue namespace starts with the physical MusicTable rows, then
 * uses 19..60 for mission narration in INTRO.XA. Values 61..99 are the hole
 * diagnosed as "bad music no"; values from 100 are TORA.XA cues. The first
 * narration cue maps to INTRO voice id 101 because that table begins at
 * bank-local id 1. */
enum music_cue_namespace
{
    MUSIC_CUE_NARRATION_FIRST = MUSIC_TRACK_COUNT,
    MUSIC_CUE_NARRATION_LAST = 60,
    MUSIC_CUE_INVALID_FIRST = MUSIC_CUE_NARRATION_LAST + 1,
    MUSIC_CUE_INVALID_LAST = 99,
    MUSIC_CUE_INVALID_COUNT =
        MUSIC_CUE_INVALID_LAST - MUSIC_CUE_INVALID_FIRST + 1,
    MUSIC_CUE_TORA_FIRST = 100,
    MUSIC_NARRATION_VOICE_ID_OFFSET =
        (VOICE_ID_INTRO_BASE + 1) - MUSIC_CUE_NARRATION_FIRST,
    MUSIC_TORA_VOICE_ID_OFFSET =
        VOICE_ID_TORA_BASE - MUSIC_CUE_TORA_FIRST,
    MUSIC_EVENT_ID_BASE = 100
};

/* Retail extends the demo's TMusicTable with an XA end time. */
typedef struct TMusicTable TMusicTable;
struct TMusicTable
{
    u8 *file;        /* 0x00 */
    u8 channel;      /* 0x04 */
    u8 start_minute; /* 0x05 */
    u8 start_second; /* 0x06 */
    u8 end_minute;   /* 0x07 */
    u8 end_second;   /* 0x08 */
}; /* 0x0C */

/* One sentinel-terminated cue row in EVENT_*.XA, INTRO.XA, or TORA.XA. */
typedef struct TVoiceTable TVoiceTable;
struct TVoiceTable
{
    u8 id;           /* 0x00: bank-local id */
    u8 channel;      /* 0x01 */
    u8 start_minute; /* 0x02 */
    u8 start_second; /* 0x03 */
    u8 end_minute;   /* 0x04 */
    u8 end_second;   /* 0x05 */
}; /* 0x06 */

extern TMusicTable MusicTable[MUSIC_TRACK_COUNT];
/* Physical track -> logical MusicId, followed by SOUND_TABLE_END. */
extern u8 MusicIdByTrack[MUSIC_TRACK_COUNT + 1];
extern void _PlayMusic(int cue, int mode);
extern void PlayVoice(int id);
extern void PlayMusicFormID(s32 event_audio_id);

#endif
