#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern char fmt_cd_path[];             /* \\%s;1 */
extern char fmt_cd_file[];             /* %s;1 */
extern char str_open_out_of_handle[];  /* open:out of handle */
extern char msg_open_file_not_found[]; /* open:file not found */

extern int sprintf(char *buf, char *fmt, ...);
extern int puts(char *s);

FILE *cd_open(char *name)
{
    enum
    {
        CD_SEARCH_RETRY_LIMIT = 10
    };
    char path[80];
    FILE *candidate;
    FILE *file;
    CdlFILE *found;
    s16 index;
    s16 retries;

    if (*name != '\\')
    {
        sprintf(path, fmt_cd_path, name);
    }
    else
    {
        sprintf(path, fmt_cd_file, name);
    }

    index = 0;
    do
    {
        candidate = &FileHandlePool[index];
        if (candidate->flagUse == 0)
        {
            file = candidate;
            goto have_handle;
        }
        index++;
    } while (index < N_CD_FILE_HANDLES);
    file = NULL;

have_handle:
    retries = 0;
    if (file == NULL)
    {
        puts(str_open_out_of_handle);
    }
    else
    {
        do
        {
            found = CdSearchFile(&file->finfo, path);
            if (found != NULL)
            {
                file->flagUse = 1;
                file->pos = 0;
                return file;
            }
            retries++;
        } while (retries < CD_SEARCH_RETRY_LIMIT);
        puts(msg_open_file_not_found);
    }
    return NULL;
}
