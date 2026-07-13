#include "common.h"
#include "main.exe.h"

/*
 * AfsFindFile (0x8005eb84, 0x230 bytes) normalizes an AFS path, resolves each
 * directory component to its numeric entry prefix, then returns the matching
 * file-table element.  The two 200-byte arrays account for the target's exact
 * 0x194-byte working stack window.
 *
 * The demo body calls AfsFilenameFix and subAfsFindFile, while retail contains
 * their instruction bodies twice in-line.  Keeping local inline definitions
 * recovers those return islands and the byte-offset/index pair used by each
 * table scan.  The outer scan must remain a real while loop whose first body
 * statement derives cursorPath in two steps.  cc1 then duplicates the loop
 * test at entry and independently forms buffer[cursor] for the bottom test and
 * cursorPath for the next iteration; an if-wrapped do loop CSEs those addresses
 * and is one instruction short.  The mask parameter is full-width like the
 * matched subAfsFindFile helper: narrowing it inserts an andi/sign extension.
 */
typedef struct FILE FILE;
typedef struct TAFS TAFS;
typedef struct TAFSElement TAFSElement;
typedef struct TAFSFileHandle TAFSFileHandle;

struct TAFSElement
{
    u16 flag;
    u32 pos;
    u32 size;
    u32 psize;
    u8 name[20];
};

struct TAFS
{
    FILE *fpVol;
    s32 fModified;
    u32 posElement;
    TAFSElement *pElement;
    u32 maxElements;
    s32 maxElementArea;
    TAFSFileHandle *pHandle;
};

extern char *strncpy(char *dst, const char *src, u32 n);
extern char *strcpy(char *dst, const char *src);
extern int strncmp(const char *a, const char *b, u32 n);
extern int sprintf(char *dst, const char *fmt, ...);
extern int toupper(int c);
extern char AfsPathFormat[];

static __inline__ void AfsFilenameFixInline(char *path)
{
    char *p;

    if (*path != 0) {
        p = path;
        do {
            *p = toupper(*p);
            p++;
        } while (*p != 0);
    }
}

static __inline__ u32 subAfsFindFileInline(TAFS *handle, char *name, u32 mask)
{
    u32 i;

    i = 0;
    if (handle->maxElements != 0) {
        do {
            if (strncmp(name, (char *)handle->pElement[i].name, 20) == 0 &&
                (mask & handle->pElement[i].flag) != 0) {
                return i;
            }
            i++;
        } while (i < handle->maxElements);
    }
    return 0xffffffff;
}

TAFSElement *AfsFindFile(TAFS *handle, char *path, u32 flags)
{
    char buffer[200];
    char component[200];
    char *cursorPath;
    s32 cursor;
    s32 entryIndex;

    strncpy(buffer, path, 199);
    buffer[199] = 0;

    AfsFilenameFixInline(buffer);

    cursor = 0;
    while (buffer[cursor] != 0) {
        cursorPath = buffer;
        cursorPath += cursor;
        if (*cursorPath == '\\') {
            *cursorPath = 0;
            entryIndex = subAfsFindFileInline(handle, buffer, 2);
            if (entryIndex < 0) {
                goto not_found;
            }
            strcpy(component, buffer + cursor + 1);
            sprintf(buffer, AfsPathFormat, entryIndex, component);
            cursor = 0;
        } else {
            cursor++;
        }
    }

    entryIndex = subAfsFindFileInline(handle, buffer, flags);
    if (entryIndex < 0) {
not_found:
        return 0;
    }
    return &handle->pElement[entryIndex];
}
