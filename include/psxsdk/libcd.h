#ifndef PSXSDK_LIBCD_H
#define PSXSDK_LIBCD_H

#include <types.h>

/* PsyQ 4.5/4.6 LIBCD.H ABI declarations used by Tenchu. */

typedef void (*CdlCB)(u_char, u_char *);

typedef struct CdlLOC CdlLOC;
struct CdlLOC
{
    u_char minute;
    u_char second;
    u_char sector;
    u_char track;
};

typedef struct CdlFILTER CdlFILTER;
struct CdlFILTER
{
    u_char file;
    u_char chan;
    u_short pad;
};

typedef struct CdlATV CdlATV;
struct CdlATV
{
    u_char val0;
    u_char val1;
    u_char val2;
    u_char val3;
};

typedef struct CdlFILE CdlFILE;
struct CdlFILE
{
    CdlLOC pos;
    u_long size;
    char name[16];
};

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
