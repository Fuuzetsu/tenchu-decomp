#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * AfsGetEntry (0x8005e950, 0x234 bytes) allocates and reads the AFS volume's
 * big-endian element table.  The demo calls getshort/getlong; retail contains
 * those helpers inline, including the address-taken stack short used to check
 * the per-record 0x4958 marker.
 *
 * Matching notes:
 *  - A hand-written back edge preserves the three explicit cursors.  A real
 *    do/while lets loop.c create parallel biased induction pointers for the
 *    name and packed-word accesses, making the function nine instructions
 *    too long.
 *  - The two error calls are written before the success body.  jump2 merges
 *    their common call/return suffix at the earlier target address while each
 *    branch still materializes its string directly in the a0 argument chain.
 *  - The nested zero-trip loops emit no code.  Their loop-depth weighting,
 *    plus one local weight on the element-base copy, reproduces the retail
 *    saved-register priorities.
 *  - The marker's high and low bytes intentionally use the raw and packed
 *    cursors respectively; the inline helper also preserves the target's
 *    address-taken stack-halfword store.
 */

extern void AdtMessageBox(char *fmt, ...);
extern void *valloc(u32 size);
extern void vfree(void *p);
extern int cd_read(FILE *f, void *buffer, int length);
extern char *strncpy(char *dst, const char *src, u32 n);
extern char D_800148D4[];
extern char D_800148F0[];
extern char D_80014910[];
extern char D_80014930[];

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

    if (handle->maxElements == 0) {
        AdtMessageBox(D_800148D4);
        return 0;
    }

    elements = valloc(handle->maxElements * sizeof(TAFSElement));
    if (elements == 0) {
        AdtMessageBox(D_800148F0);
        vfree(0);
        return 1;
    }

    buffer = valloc(handle->maxElements * sizeof(TAFSElement));
    if (buffer != 0) {
        goto entry_ready;
    }
    AdtMessageBox(D_80014910);
    return 1;

bad_index:
    AdtMessageBox(D_80014930);
    return 1;

entry_ready:
    cd_seek(handle->fpVol, handle->posElement, CDSEEK_SET);
    i = 0;
    cd_read(handle->fpVol, buffer,
            handle->maxElements * sizeof(TAFSElement));

    raw = buffer;
    do {
        do {
            if (handle->maxElements != 0) {
                do {
                    element = elements;
                } while (0);
                packed = raw + 1;
entry_loop:
                element->flag = ((u16)packed[1] << 8) | packed[2];
                element->pos = ((u32)packed[3] << 24) |
                               ((u32)packed[4] << 16) |
                               ((u32)packed[5] << 8) | packed[6];
                element->size = ((u32)packed[11] << 24) |
                                ((u32)packed[12] << 16) |
                                ((u32)packed[13] << 8) | packed[14];
                element->psize = ((u32)packed[7] << 24) |
                                 ((u32)packed[8] << 16) |
                                 ((u32)packed[9] << 8) | packed[10];
                strncpy((char *)element->name, (char *)buffer + 0x10, 0x13);
                element->name[0x13] = 0;
                AfsGetShort(&marker, buffer, packed);
                if (marker != 0x4958) {
                    goto bad_index;
                }
                packed += sizeof(TAFSElement);
                buffer += sizeof(TAFSElement);
                element->name[0x13] = 0;
                element++;
                if (handle->maxElements > ++i) {
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
