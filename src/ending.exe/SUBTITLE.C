#include "common.h"
#include "ending.h"
#include "../main.exe/font.h"
#include "../main.exe/vmemory.h"
#include "../main.exe/tuning.h"

/* The movie CAD interpreter uses twelve-byte animation records. Command 5
 * selects a voice, command 8 changes the caption, and command 1 waits for a
 * movie frame (zero ends the stream). */
enum
{
    MOVIE_CMD_WAIT = 1,
    MOVIE_CMD_VOICE = 5,
    MOVIE_CMD_SUBTITLE = 8,
    MOVIE_SUBTITLE_CLEAR = -1,
    MOVIE_SUBTITLE_Y = 212
};

typedef struct MovieCommand
{
    s16 command;
    s16 value;
    s16 sound;
    s16 reserved[3];
} MovieCommand;

extern u8 *EndingSubtitleData;
extern MovieCommand *EndingCommandCursor;
extern SoundEffect *EndingSubtitleVoices;
extern u8 EndingSubtitleText[];
extern char *EndingSubtitleDirectories[N_LANGUAGES];
extern char fmt_ending_subtitles[];

short ProcessEndingCommands(void)
{
    while (EndingCommandCursor->command != MOVIE_CMD_WAIT)
    {
        switch (EndingCommandCursor->command)
        {
        case CVA_CMD_SEQUENCE:
            break;
        case MOVIE_CMD_VOICE:
            PlaySEHigh(EndingSubtitleVoices, EndingCommandCursor->sound, 127);
            break;
        case MOVIE_CMD_SUBTITLE:
            if (EndingCommandCursor->value == MOVIE_SUBTITLE_CLEAR)
            {
                EndingSubtitleText[0] = 0;
            }
            else
            {
                SetupTelop((u8 *)strcpy((char *)EndingSubtitleText,
                                      (char *)EndingSubtitleData +
                                          EndingCommandCursor->value), 0);
            }
            break;
        }
        EndingCommandCursor++;
    }
    return EndingCommandCursor->value != 0;
}

void LoadEndingSubtitles(short stage)
{
    char filename[64];
    char character;
    short stage_number = stage;

    if (EndingSubtitleData)
        vfree(EndingSubtitleData);
    /* These are the only stages passed by the ending sequence. Credits
     * share Rikimaru's caption file, independently of the chosen player. */
    switch (stage)
    {
    case ENDING_CHARACTER_SUBTITLES:
        character = CHOSEN_CHARACTER == RIKIMARU_0 ? 'R' : 'A';
        break;
    case ENDING_CREDITS_SUBTITLES:
        character = 'R';
        break;
    }
    sprintf(filename, fmt_ending_subtitles,
            EndingSubtitleDirectories[CHOSEN_LANGUAGE], stage_number,
            character);
    EndingSubtitleData = FileRead(filename);
    EndingCommandCursor = (MovieCommand *)EndingSubtitleData;
    EndingSubtitleText[0] = 0;
}

short UpdateEndingSubtitles(int frame, SoundEffect *voices)
{
    int x;
    GsOT_TAG *ot;

    EndingSubtitleVoices = voices;
    if (!ProcessEndingCommands())
        return 0;
    if (EndingSubtitleText[0])
    {
        int width = telop_text_width_(EndingSubtitleText);

        x = -(width / 2);
        x += SCREEN_W / 2;
        ot = OTablePt->org;
        draw_telop_line_(ot, x, MOVIE_SUBTITLE_Y, EndingSubtitleText);
    }
    if (frame >= EndingCommandCursor->value)
        EndingCommandCursor++;
    return 1;
}

/* ENDING string storage, in retail object order. */
char ending_subtitle_japanese[] = "K:\\WORK\\CDIMAGE\\ANIM\\";
char ending_subtitle_italian[] = "K:\\WORK\\CDIMAGE\\ANIM\\ITALIAN\\";
char ending_subtitle_french[] = "K:\\WORK\\CDIMAGE\\ANIM\\FRENCH\\";
char ending_subtitle_english[] = "K:\\WORK\\CDIMAGE\\ANIM\\ENGLISH\\";
char fmt_ending_subtitles[] = "%sSTAGE%d%c.CAD";
