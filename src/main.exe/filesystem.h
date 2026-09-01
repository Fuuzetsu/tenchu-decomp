#ifndef TENCHU_FILESYSTEM_H
#define TENCHU_FILESYSTEM_H

#include <psxsdk/libcd.h>

/* FILEIO.C/AFS types recorded in the demo's PSX.SYM. */
typedef struct TFileHandle TFileHandle;
typedef TFileHandle FILE;
typedef struct TAFSElement TAFSElement;
typedef struct TAFSFileHandle TAFSFileHandle;
typedef struct TAFS TAFS;
typedef struct MemoryDiskType MemoryDiskType;

/* ReadMode combines the active backend in its low two bits with loader
 * policy flags. */
typedef s32 file_read_mode;
enum file_read_mode
{
    READ_MODE_UNINITIALIZED = -1,
    READ_SOURCE_DEVPC = 0,
    READ_SOURCE_MEMORY = 1,
    READ_SOURCE_CDROM = 2,
    READ_SOURCE_MASK = 3,
    READ_MODE_TRACE = 4,
    READ_MODE_ACQUIRE_MEMORY_DISK = 8,
    READ_MODE_MEMORY_DISK_MASK =
        READ_SOURCE_MEMORY | READ_MODE_ACQUIRE_MEMORY_DISK
};

enum
{
    MEMORY_DISK_SCRATCH_SIZE = 0x8000
};

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

/* Per-record marker in the volume's IX table: big-endian "IX". */
#define AFS_ELEMENT_MARK 0x4958

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

struct MemoryDiskType
{
    unsigned char name[24];
    unsigned long size;
    unsigned long *data;
};

enum
{
    N_CD_FILE_HANDLES = 10,
    N_AFS_FILE_HANDLES = 5
};

/* FILEIO.C-private originally; extern because that source is split here. */
extern TAFS systemAFS;
extern MemoryDiskType *MDfat;
extern file_read_mode ReadMode;
/* FILEIO.C's ten-slot CD handle pool, named by the demo symbol data. */
extern FILE FileHandlePool[N_CD_FILE_HANDLES];

int cd_seek(FILE *f, int offset, TSeekMode whence);

#endif
