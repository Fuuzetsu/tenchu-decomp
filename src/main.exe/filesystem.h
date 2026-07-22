#ifndef TENCHU_FILESYSTEM_H
#define TENCHU_FILESYSTEM_H

#include <psxsdk/libcd.h>

/* FILEIO.C/AFS types recorded in the demo's PSX.SYM. */
typedef struct TFileHandle TFileHandle;
typedef TFileHandle FILE;
typedef struct TAFSElement TAFSElement;
typedef struct TAFSFileHandle TAFSFileHandle;
typedef struct TAFS TAFS;

/* FILEIO's original seek-origin type from PSX.SYM. */
typedef enum TSeekMode TSeekMode;
enum TSeekMode
{
    CDSEEK_SET = 0,
    CDSEEK_CUR = 1,
    CDSEEK_END = 2
};

enum
{
    AfsFlag_File = 1,
    AfsFlag_Folder = 2
};

struct TFileHandle
{
    CdlFILE finfo;
    int flagUse;
    long pos;
};

struct TAFSElement
{
    unsigned short flag;
    unsigned long pos;
    unsigned long size;
    unsigned long psize;
    unsigned char name[20];
};

struct TAFSFileHandle
{
    int flagUse;
    unsigned long pos;
    TAFSElement *info;
};

struct TAFS
{
    TFileHandle *fpVol;
    int fModified;
    unsigned long posElement;
    TAFSElement *pElement;
    unsigned long maxElements;
    int maxElementArea;
    TAFSFileHandle *pHandle;
};

extern TAFS systemAFS;
/* FILEIO.C's ten-slot CD handle pool, named by the demo symbol data. */
extern FILE FileHandlePool[10];

int cd_seek(FILE *f, int offset, TSeekMode whence);

#endif
