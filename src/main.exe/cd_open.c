#include "common.h"
#include "main.exe.h"

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

typedef struct CdlLOC CdlLOC;
typedef struct CdlFILE CdlFILE;
typedef struct FILE FILE;

struct CdlLOC
{
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
};

struct CdlFILE
{
    CdlLOC pos;
    u32 size;
    s8 name[16];
};

struct FILE
{
    CdlFILE finfo;
    s32 flagUse;
    s32 pos;
};

extern FILE FileHandlePool[10];
extern char D_80097E80[];
extern char D_80097E88[];
extern char D_80014A08[];
extern char D_80014A1C[];

extern int sprintf(char *buf, char *fmt, ...);
extern CdlFILE *CdSearchFile(CdlFILE *file, char *name);
extern int puts(char *s);

FILE *cd_open(char *name)
{
    char path[80];
    FILE *candidate;
    FILE *file;
    CdlFILE *found;
    s16 index;
    s16 retries;

    if (*name != '\\') {
        sprintf(path, D_80097E80, name);
    } else {
        sprintf(path, D_80097E88, name);
    }

    index = 0;
    do {
        candidate = &FileHandlePool[index];
        if (candidate->flagUse == 0) {
            file = candidate;
            goto have_handle;
        }
        index++;
    } while (index < 10);
    file = NULL;

have_handle:
    retries = 0;
    if (file == NULL) {
        puts(D_80014A08);
    } else {
        do {
            found = CdSearchFile(&file->finfo, path);
            if (found != NULL) {
                file->flagUse = 1;
                file->pos = 0;
                return file;
            }
            retries++;
        } while (retries < 10);
        puts(D_80014A1C);
    }
    return NULL;
}

// triage: MEDIUM — 66 insns, 4 loop, 3 callees, ~0.08 to DisposeWeapon
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// disc_file_descriptor_t * cd_open(char *param_1)
//
// {
//   int iVar1;
//   CdlFILE *pCVar2;
//   int iVar3;
//   disc_file_descriptor_t *_30;
//   char *pcVar4;
//   char acStack_60 [80];
//
//   if (*param_1 == '\\') {
//     pcVar4 = s__s_1_80097e88;
//   }
//   else {
//     pcVar4 = s___s_1_80097e80;
//   }
//   sprintf(acStack_60,pcVar4);
//   iVar3 = 0;
//   iVar1 = 0;
//   do {
//     _30 = (disc_file_descriptor_t *)(&FileHandlePool[0].location.minute + (iVar1 >> 0xb));
//     iVar3 = iVar3 + 1;
//     if (*(int *)(&FileHandlePool[0].used + (iVar1 >> 0xb)) == 0) goto LAB_8005f304;
//     iVar1 = iVar3 * 0x10000;
//   } while (iVar3 * 0x10000 >> 0x10 < 10);
//   _30 = (disc_file_descriptor_t *)0x0;
// LAB_8005f304:
//   iVar1 = 0;
//   if (_30 == (disc_file_descriptor_t *)0x0) {
//     pcVar4 = "open:out of handle";
//   }
//   else {
//     do {
//       pCVar2 = CdSearchFile((CdlFILE *)_30,acStack_60);
//       iVar1 = iVar1 + 1;
//       if (pCVar2 != (CdlFILE *)0x0) {
//         _30->used = '\x01';
//         _30->field19_0x19 = '\0';
//         _30->field20_0x1a = '\0';
//         _30->field21_0x1b = '\0';
//         _30->cursor = 0;
//         return _30;
//       }
//     } while (iVar1 * 0x10000 >> 0x10 < 10);
//     pcVar4 = "open:file not found";
//   }
//   puts(pcVar4);
//   return (disc_file_descriptor_t *)0x0;
// }
