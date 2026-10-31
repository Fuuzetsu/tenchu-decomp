#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "filesystem.h"
#include "vmemory.h"

extern char msg_afsgetentry_empty_index[]; /* AfsGetEntry: empty index */
extern char msg_afsgetenty_no_memory[];    /* AfsGetEnty: memory not enough! */
extern char msg_afsgetentry_no_memory[];   /* AfsGetEntry: memory not enough! */
extern char msg_illigal_index[];

static __inline__ void AfsGetShort(u16 *dst, const u8 *src, const u8 *next)
{
    *dst = ((u16)src[0] << 8) | next[0];
}

int AfsGetEntry(TAFS *handle)
{
    TAFSElement *elements;
    TAFSElement *element;
    AFSIndexEntry *record;
    AFSIndexEntry *records;
    u8 *packed;
    u32 i;
    u32 entry_count;
    int more_entries;
    u16 marker;

    if (handle->maxElements == 0)
    {
        AdtMessageBox(msg_afsgetentry_empty_index);
        return 0;
    }

    elements = valloc(handle->maxElements * sizeof(TAFSElement));
    if (elements == 0)
    {
        AdtMessageBox(msg_afsgetenty_no_memory);
        vfree(elements);
        return 1;
    }

    record = valloc(handle->maxElements * sizeof(AFSIndexEntry));
    if (record != 0)
        goto read_index;
    AdtMessageBox(msg_afsgetentry_no_memory);
    return 1;

invalid_index:
    AdtMessageBox(msg_illigal_index);
    return 1;

read_index:
    cd_seek(handle->fpVol, handle->posElement, CDSEEK_SET);
    i = 0;
    cd_read(handle->fpVol, record,
            handle->maxElements * sizeof(AFSIndexEntry));

    records = record;
    if (handle->maxElements != 0)
    {
        element = elements;
        packed = records->marker + 1;
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
                (char *)record->name,
                sizeof(element->name) - 1);
        element->name[sizeof(element->name) - 1] = 0;
        AfsGetShort(&marker, record->marker, packed);
        if (marker != AFS_ELEMENT_MARK)
        {
            goto invalid_index;
        }
        packed += sizeof(AFSIndexEntry);
        record++;
        element->name[sizeof(element->name) - 1] = 0;
        element++;
        entry_count = handle->maxElements;
        AFS_ADVANCE_INDEX(i, entry_count, more_entries);
        if (more_entries)
        {
            goto entry_loop;
        }
    }

    handle->pElement = elements;
    handle->maxElementArea = handle->maxElements;
    vfree(records);
    return 0;
}
