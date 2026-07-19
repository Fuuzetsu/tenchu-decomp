#ifndef PSXSDK_LIBCD_H
#define PSXSDK_LIBCD_H

#include <types.h>

/* Minimal PsyQ 4.5 ABI declarations; see docs/psyq-headers.md. */

typedef void (*CdlCB)(u_char, u_char *);

typedef struct
{
    u_char minute;
    u_char second;
    u_char sector;
    u_char track;
} CdlLOC;

typedef struct
{
    u_char file;
    u_char chan;
    u_short pad;
} CdlFILTER;

typedef struct
{
    u_char val0;
    u_char val1;
    u_char val2;
    u_char val3;
} CdlATV;

typedef struct
{
    CdlLOC pos;
    u_long size;
    char name[16];
} CdlFILE;

int CdInit(void);
void CdFlush(void);
CdlFILE *CdSearchFile(CdlFILE *file, char *name);
CdlLOC *CdIntToPos(int sector, CdlLOC *position);
int CdControl(u_char command, u_char *param, u_char *result);
int CdControlB(u_char command, u_char *param, u_char *result);
int CdControlF(u_char command, u_char *param);
int CdGetSector(void *address, int size);
int CdPosToInt(CdlLOC *position);
int CdReady(int mode, u_char *result);
int CdSync(int mode, u_char *result);

#endif
