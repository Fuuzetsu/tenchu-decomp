#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "filesystem.h"
#include "vmemory.h"

extern char msg_afsgetentry_empty_index[]; /* AfsGetEntry: empty index */
extern char msg_afsgetenty_no_memory[];    /* AfsGetEnty: memory not enough! */
extern char msg_afsgetentry_no_memory[];   /* AfsGetEntry: memory not enough! */
extern char msg_illigal_index[];

static __inline__ void AfsGetShort(u16 *dst, u8 *src, u8 *next)
{
    *dst = ((u16)src[0] << 8) | next[0];
}

int AfsGetEntry(TAFS *handle)
{
    TAFSElement *elements;
    TAFSElement *element;
    u8 *buffer;
    u8 *raw;
    u8 *packed;
    u32 i;
    u16 marker;

    if (handle->maxElements == 0)
    {
        AdtMessageBox(msg_afsgetentry_empty_index);
        return 0;
    }

    elements = valloc(handle->maxElements * sizeof(TAFSElement));
    do
    {
        if (elements == 0)
        {
            AdtMessageBox(msg_afsgetenty_no_memory);
            vfree(elements);
            return 1;
        }
    } while (0);

    buffer = valloc(handle->maxElements * sizeof(AFSIndexEntry));
    if (buffer == 0)
    {
        AdtMessageBox(msg_afsgetentry_no_memory);
        return 1;
    }

    cd_seek(handle->fpVol, handle->posElement, CDSEEK_SET);
    i = 0;
    cd_read(handle->fpVol, buffer,
            handle->maxElements * sizeof(AFSIndexEntry));

    raw = buffer;
    /* Wrapper loops retain the original control-flow boundaries; their source form is unknown. */
    do
    {
        do
        {
            if (handle->maxElements != 0)
            {
                element = elements;
                packed = raw + 1;
            entry_loop:
                /* Retail carries this cursor one byte into the record;
                 * step back only for the typed wire-format view. */
                element->flag = AFS_READ_BE16(
                    ((AFSIndexEntry *)(packed - 1))->flag);
                element->pos = AFS_READ_BE32(
                    ((AFSIndexEntry *)(packed - 1))->position);
                element->size = AFS_READ_BE32(
                    ((AFSIndexEntry *)(packed - 1))->size);
                element->psize = AFS_READ_BE32(
                    ((AFSIndexEntry *)(packed - 1))->packed_size);
                strncpy((char *)element->name,
                        (char *)((AFSIndexEntry *)buffer)->name,
                        sizeof(element->name) - 1);
                element->name[sizeof(element->name) - 1] = 0;
                AfsGetShort(&marker, buffer, packed);
                if (marker != AFS_ELEMENT_MARK)
                {
                    AdtMessageBox(msg_illigal_index);
                    return 1;
                }
                packed += sizeof(AFSIndexEntry);
                buffer += sizeof(AFSIndexEntry);
                element->name[sizeof(element->name) - 1] = 0;
                element++;
                if (handle->maxElements > ++i)
                {
                    goto entry_loop;
                }
            }
        } while (0);
    } while (0);

    handle->pElement = elements;
    handle->maxElementArea = handle->maxElements;
    vfree(raw);
    return 0;
}
