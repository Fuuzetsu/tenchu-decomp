#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * AfsGetEntry (0x8005e950, 0x234 bytes) allocates and reads the AFS volume's
 * big-endian element table.  The demo calls getshort/getlong; retail contains
 * those helpers inline, including the address-taken stack short used to check
 * each record's AFS_ELEMENT_MARK.
 *
 * Matching notes:
 *  - A hand-written back edge preserves the three explicit cursors.  A real
 *    do/while lets loop.c create parallel biased induction pointers for the
 *    name and packed-word accesses, making the function nine instructions
 *    too long.
 *  - The two error calls are written before the success body.  jump2 merges
 *    their common call/return suffix at the earlier target address while each
 *    branch still materializes its string directly in the a0 argument chain.
 *  - The nested zero-trip loops emit no code.  Their loop-depth weighting
 *    reproduces the retail saved-register priorities; the depth-2 pair is
 *    irreducible (i's only refs outside it are fence-toxic: i = 0 blocks a
 *    code motion, the back edge costs a branch, and enclosing the label
 *    revives the induction-pointer explosion).  The old third level on the
 *    element-base copy is split per the DefaultActionHumanoid method onto
 *    the elements error arm below (elements must stay above raw).
 *  - The marker's high and low bytes intentionally use the raw and packed
 *    cursors respectively; the inline helper also preserves the target's
 *    address-taken stack-halfword store.
 */

extern void AdtMessageBox(char *fmt, ...);
extern void *valloc(u32 size);
extern void vfree(void *p);
extern int cd_read(FILE *f, void *buffer, int length);
extern char *strncpy(char *dst, const char *src, u32 n);
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
    elements = (TAFSElement *)(((u32)elements + (u32)elements) -
                               (u32)elements);
    /* Folded after flow: retains the allocation weight of the former
     * allocation-failure wrapper while keeping the null test ordinary. */
    if (elements == 0)
    {
        AdtMessageBox(msg_afsgetenty_no_memory);
        vfree(elements);
        return 1;
    }

    buffer = valloc(handle->maxElements * sizeof(AFSIndexEntry));
    if (buffer != 0)
    {
        goto entry_ready;
    }
    AdtMessageBox(msg_afsgetentry_no_memory);
    return 1;

bad_index:
    AdtMessageBox(msg_illigal_index);
    return 1;

entry_ready:
    cd_seek(handle->fpVol, handle->posElement, CDSEEK_SET);
    i = 0;
    cd_read(handle->fpVol, buffer,
            handle->maxElements * sizeof(AFSIndexEntry));

    raw = buffer;
    /* One-shot fences: the depth-2 pair is byte-required and irreducible
     * (collapse measured; cookbook, and the header note). */
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
                    goto bad_index;
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
