#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * cd_open (0x8005f278, 0x108 bytes) — formats a CD-ROM path, claims the
 * first free entry in the ten-slot FileHandlePool, and retries CdSearchFile
 * up to ten times. A successful search marks the handle active and resets
 * its cursor; exhaustion reports the applicable error and returns NULL.
 *
 * Matching notes:
 *  - The override symbol at 0x8005f2b8 is only a sprintf call-site marker.
 *    The free-slot scan branches back into the first piece, so both pieces
 *    are one ordinary C function.
 *  - Two direct sprintf arms and two direct puts sites let cross-jump share
 *    each call tail while keeping each string's %hi/%lo pair in its target
 *    argument register. A funnelled format/message pointer uses $v0 instead.
 *  - The transient pool `candidate` is distinct from the final `file`.
 *    This delays the $s1 assignment until a free slot is found and preserves
 *    the target's caller-saved scan cursor.
 *  - Both narrow-counter increments follow their early-exit tests in source.
 *    Reorg moves each increment into the preceding branch delay slot because
 *    the counter is dead on the taken path. Besides matching that ordering,
 *    this colours the FileHandlePool base into the target's $a1; placing an
 *    increment before either test creates a different counter/base copy chain.
 *  - `path[80]` followed by s0/s1/ra gives the exact 0x70-byte frame.
 *
 * STATUS: MATCH (66/66 instructions).
 */

extern char fmt_cd_path[];             /* \\%s;1 */
extern char fmt_cd_file[];             /* %s;1 */
extern char str_open_out_of_handle[];  /* open:out of handle */
extern char msg_open_file_not_found[]; /* open:file not found */

extern int sprintf(char *buf, char *fmt, ...);
extern int puts(char *s);

FILE *cd_open(char *name)
{
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
    } while (index < 10);
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
        } while (retries < 10);
        puts(msg_open_file_not_found);
    }
    return NULL;
}
