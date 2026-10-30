#include "common.h"
#include "trial.exe.h"
#include "mission.h"
#include "card.h"
#include "../main.exe/images.h"
#include "../main.exe/vmemory.h"

extern char CardPathFormat[];
extern char fmt_trial_save_title[];
extern char str_sjis_title[];
extern char path_trial_card_image_root[];
extern char path_trial_card_icon_01[];
extern char path_trial_card_icon_02[];
extern char path_trial_card_icon_03[];
extern char path_trial_card_icon_04[];
extern char path_trial_card_icon_05[];
extern char path_trial_card_icon_06[];
extern char path_trial_card_icon_07[];
extern char path_trial_card_icon_08[];
extern char path_trial_card_icon_09[];
extern char path_trial_card_icon_10[];
extern char path_trial_card_icon_11[];
extern char path_trial_card_icon_12[];
extern char path_trial_card_icon_default[];

/* Preset scores use icon.arc. User stages select one of the twelve
 * theme-specific three-frame icon archives. */
void PrepareTrialCardIcon(TCardHeader *header, s16 operation)
{
    char filename[64];
    char *archives[TRIAL_CARD_THEME_COUNT + 1] = {
        path_trial_card_icon_01,
        path_trial_card_icon_02,
        path_trial_card_icon_03,
        path_trial_card_icon_04,
        path_trial_card_icon_05,
        path_trial_card_icon_06,
        path_trial_card_icon_07,
        path_trial_card_icon_08,
        path_trial_card_icon_09,
        path_trial_card_icon_10,
        path_trial_card_icon_11,
        path_trial_card_icon_12,
        path_trial_card_icon_default,
    };
    u8 *icon;
    s32 i;

    switch (operation)
    {
    case TRIAL_CARD_ICON_LOAD:
        if (TrialUserMission != 0)
        {
            sprintf(filename, CardPathFormat, path_trial_card_image_root,
                    archives[TrialMission.header.theme]);
        }
        else
        {
            sprintf(filename, CardPathFormat, path_trial_card_image_root,
                    archives[TRIAL_CARD_THEME_COUNT]);
        }
        if (TrialCardIconArchive != NULL)
        {
            vfree(TrialCardIconArchive);
            TrialCardIconArchive = NULL;
        }
        TrialCardIconArchive = FileRead(filename);
        break;
    case TRIAL_CARD_ICON_COPY:
        for (i = 0; i < CARD_ICON_FRAME_COUNT; i++)
        {
            icon = (u8 *)get_tim_from_archive(TrialCardIconArchive, i);
            __builtin_memcpy(header->Icon[i], CARD_ICON_TIM_PIXELS(icon),
                             sizeof(header->Icon[i]));
        }
        __builtin_memcpy(header->Clut, CARD_ICON_TIM_CLUT(icon), sizeof(header->Clut));
        break;
    case TRIAL_CARD_ICON_RELEASE:
        if (TrialCardIconArchive != NULL)
        {
            vfree(TrialCardIconArchive);
            TrialCardIconArchive = NULL;
        }
        break;
    }
}

/* Unlike MAIN's flag, nonzero here means overwrite an existing file.
 * A new save must create the file successfully before writing its block. */
card_result SaveCard(s32 target, u8 *name, void *mem, s32 size, s16 overwrite)
{
    u8 filename[256];
    MemoryCardFileBlock block;
    s32 command;
    enum card_result result;
    enum memcard_channel channel;
    TCardHeader *header;
    void *data;

    result = CARD_RESULT_SUCCESS;
    header = &block.header;
    header->Magic[0] = 'S';
    header->Magic[1] = 'C';
    header->Type = SAVE_ICON_3_FRAMES;
    header->BlockEntry = CARD_FILE_BLOCKS;
    memset(header->Title, 0, sizeof(header->Title));
    channel = MEMCARD_CHANNEL_0;
    data = block.payload;
    if (TrialUserMission != 0)
    {
        sprintf(header->Title, fmt_trial_save_title, TrialMission.header.title);
    }
    else
    {
        sprintf(header->Title, str_sjis_title);
    }
    memset(header->reserve, 0, sizeof(header->reserve));
    PrepareTrialCardIcon(header, TRIAL_CARD_ICON_COPY);
    if (overwrite == 0)
    {
        sprintf(filename, CardPathFormat, TENCHU_ID, name);
        result = MemCardCreateFile(channel, (char *)filename, CARD_FILE_BLOCKS);
        if (result != CARD_RESULT_SUCCESS)
        {
            goto complete;
        }
    }
    {
        memcpy(data, mem, size);
        sprintf(filename, CardPathFormat, TENCHU_ID, name);
        result = MemCardWriteFile(channel, (char *)filename, &block, 0, sizeof(block));
        MemCardSync(MEMCARD_SYNC_BLOCKING, &command, &result);
    }
complete:
    return result;
}

/* The record table is deliberately excluded. Changing the high scores does
 * not change the mission checksum. Bit 27 is preserved separately. */
u32 CalculateTrialMissionChecksum(void)
{
    s32 layout_sum;
    s32 map_sum;
    s32 metadata_sum;
    s32 size;
    s32 i;
    u8 *cursor;
    u32 result;

    layout_sum = 0;
    map_sum = 0;
    metadata_sum = 0;
    size = TRIAL_MISSION_METADATA_SIZE;
    cursor = (u8 *)&TrialMission.header.flags;
    for (i = 0; i < size; i++)
    {
        metadata_sum += *cursor++;
    }
    size = TRIAL_MISSION_MAP_SIZE;
    cursor = (u8 *)TrialMission.map;
    for (i = 0; i < size; i++)
    {
        map_sum += *cursor++;
    }
    size = TRIAL_MISSION_LAYOUT_SIZE;
    for (i = 0; i < size; i++)
    {
        layout_sum += *cursor++;
    }
    result = (metadata_sum * TRIAL_MISSION_MAP_SIZE + layout_sum * 64 + map_sum) &
             TRIAL_MISSION_CHECKSUM_MASK;
    if (result == 0)
    {
        result = 1;
    }
    if (TrialMission.header.checksum & TRIAL_MISSION_PRESERVED_CHECKSUM_BIT)
    {
        result |= TRIAL_MISSION_PRESERVED_CHECKSUM_BIT;
    }
    return result;
}

card_result CheckTrialCard(void)
{
    s32 command;
    enum card_result result;

    result = MemCardExist(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &command, &result);
    return result;
}

card_result ChkCard(void)
{
    s32 cmd;
    enum card_result result;

    result = MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}

card_result check_card_file_(char *name)
{
    char path[200];
    s32 cmd;
    enum card_result result;
    s32 acceptCmd;
    enum card_result acceptResult;

    result = MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    sprintf(path, CardPathFormat, TENCHU_ID, name);
    acceptResult = MemCardOpen(MEMCARD_CHANNEL_0, path,
                               MEMCARD_OPEN_READ_ONLY);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &acceptCmd, &acceptResult);
    if (acceptResult == CARD_RESULT_SUCCESS)
    {
        MemCardClose();
    }
    return acceptResult;
}


s16 GetTrialCardDirectory(char *pattern)
{
    s32 count;
    s32 result;
    s32 returned_count;

    result = MemCardGetDirentry(MEMCARD_CHANNEL_0, pattern, TrialCardDirectory,
                               &count, 0, TRIAL_CARD_DIRECTORY_CAPACITY);
    if (result != CARD_RESULT_SUCCESS)
    {
        returned_count = (s16)-result;
    }
    else
    {
        returned_count = (s16)count;
    }
    return returned_count;
}

card_result DeleteCard(u8 *name)
{
    u8 fn[128];
    s32 cmd;
    enum card_result result;

    sprintf((char *)fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardDeleteFile(MEMCARD_CHANNEL_0, (char *)fn);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}


card_result FormatCard(void)
{
    s32 cmd;
    enum card_result result;

    result = MemCardFormat(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}



/* A null destination loads the complete mission; otherwise only its header
 * is copied out for the card browser. The caller supplies the read extent. */
card_result LoadTrialCard(s32 target, u8 *path, void *header, s32 size)
{
    u8 *allocation;
    s32 command;
    enum card_result result;

    allocation = valloc(size);
    if (header != NULL)
    {
        result = MemCardReadFile(MEMCARD_CHANNEL_0, (char *)path, allocation,
                                sizeof(TCardHeader), size);
    }
    else
    {
        result = MemCardReadFile(MEMCARD_CHANNEL_0, (char *)path, allocation, 0, size);
    }
    MemCardSync(MEMCARD_SYNC_BLOCKING, &command, &result);
    if (result == CARD_RESULT_SUCCESS)
    {
        if (header != NULL)
        {
            __builtin_memcpy(header, allocation, sizeof(TrialMissionHeader));
        }
        else
        {
            __builtin_memcpy(&TrialMission, allocation + sizeof(TCardHeader),
                             sizeof(TrialMission));
        }
    }
    vfree(allocation);
    return result;
}
