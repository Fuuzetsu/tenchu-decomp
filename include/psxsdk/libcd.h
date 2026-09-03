#ifndef PSXSDK_LIBCD_H
#define PSXSDK_LIBCD_H

/* Official libcd.h primitive-command codes. */
#define CdlNop 0x01
#define CdlSetloc 0x02
#define CdlPlay 0x03
#define CdlReadN 0x06
#define CdlStandby 0x07
#define CdlStop 0x08
#define CdlPause 0x09
#define CdlMute 0x0B
#define CdlDemute 0x0C
#define CdlSetfilter 0x0D
#define CdlSetmode 0x0E
#define CdlGetlocL 0x10
#define CdlGetlocP 0x11
#define CdlSeekL 0x15
#define CdlSeekP 0x16
#define CdlReadS 0x1B

/* Official libcd.h mode bits (CdlSetmode's parameter). */
#define CdlModeDA 0x01
#define CdlModeAP 0x02
#define CdlModeRept 0x04
#define CdlModeSF 0x08
#define CdlModeSize0 0x10
#define CdlModeSize1 0x20
#define CdlModeRT 0x40
#define CdlModeSpeed 0x80

/* Official libcd.h drive-status bits (CdlSTAT). */
/* Interrupt results handed to a CdReadyCallback/CdSyncCallback. */
#define CdlNoIntr 0x00
#define CdlDataReady 0x01
#define CdlComplete 0x02
#define CdlAcknowledge 0x03
#define CdlDataEnd 0x04
#define CdlDiskError 0x05

#define CdlStatError 0x01
#define CdlStatStandby 0x02
#define CdlStatSeekError 0x04
#define CdlStatIdError 0x08
#define CdlStatShellOpen 0x10
#define CdlStatRead 0x20
#define CdlStatSeek 0x40
#define CdlStatPlay 0x80

#include <types.h>
#include <psxsdk/libapi.h>

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
    u_char name[16];
} CdlFILE;

int CdInit(void);
int CdLastCom(void);
void CdFlush(void);
CdlFILE *CdSearchFile(CdlFILE *file, char *name);
CdlLOC *CdIntToPos(int sector, CdlLOC *position);
int CdControl(u_char command, u_char *param, u_char *result);
int CdControlB(u_char command, u_char *param, u_char *result);
int CdControlF(u_char command, u_char *param);
int CdGetSector(void *address, int size);
int CdPosToInt(CdlLOC *position);
int CdReady(int mode, u_char *result);
EXEC *CdReadExec(u_char *name);
int CdReadSync(int mode, u_char *result);
int CdSync(int mode, u_char *result);

#endif
