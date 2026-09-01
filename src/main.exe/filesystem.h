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
typedef struct AFSVolumeHeader AFSVolumeHeader;
typedef struct AFSIndexEntry AFSIndexEntry;

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

enum
{
    CD_DATA_SECTOR_PAYLOAD_SIZE = 2048,
    CD_DATA_SECTOR_HEADER_TAIL_SIZE = 8
};

/* Prefix requested from a CdlModeSize1 sector by cd_read_sectors_. The raw
 * sector begins with the three BCD location bytes consumed by CdPosToInt;
 * the fourth CdlLOC byte and the remaining header bytes are not interpreted
 * by the file reader, which copies the following 2048-byte payload. */
typedef struct CdDataSector CdDataSector;
struct CdDataSector
{
    CdlLOC location;                                      /* 0x000 */
    u8 header_tail[CD_DATA_SECTOR_HEADER_TAIL_SIZE];      /* 0x004 */
    u8 payload[CD_DATA_SECTOR_PAYLOAD_SIZE];              /* 0x00C */
}; /* 0x80C */

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

/* Big-endian on-disk AFS_VOL_200 structures. The runtime TAFSElement below
 * is the native-endian form produced by AfsGetEntry. */
enum
{
    AFS_VOLUME_SIGNATURE_SIZE = 12,
    AFS_VOLUME_HEADER_SIZE = 0x28,
    AFS_ELEMENT_NAME_SIZE = 20
};

struct AFSVolumeHeader
{
    u8 signature[AFS_VOLUME_SIGNATURE_SIZE]; /* 0x00: "AFS_VOL_200" */
    u8 element_count[4];                     /* 0x0C: big-endian */
    u8 index_position[4];                    /* 0x10: big-endian */
    u8 reserved[AFS_VOLUME_HEADER_SIZE - AFS_VOLUME_SIGNATURE_SIZE - 8];
}; /* 0x28 */

struct AFSIndexEntry
{
    u8 marker[2];                  /* 0x00: "IX" */
    u8 flag[2];                    /* 0x02: big-endian AfsFlag_* */
    u8 position[4];                /* 0x04: big-endian */
    u8 packed_size[4];             /* 0x08: big-endian */
    u8 size[4];                    /* 0x0C: big-endian */
    u8 name[AFS_ELEMENT_NAME_SIZE]; /* 0x10 */
}; /* 0x24 */

#define AFS_READ_BE32(bytes)                                                \
    (((u32)(bytes)[0] << 24) | ((u32)(bytes)[1] << 16) |                   \
     ((u32)(bytes)[2] << 8) | (u32)(bytes)[3])
#define AFS_INDEX_BYTE_OFFSET(member) \
    ((u32)&((AFSIndexEntry *)0)->member)

struct TFileHandle
{
    CdlFILE finfo;
    int flagUse;
    long pos;
};

/* Per-record marker in the volume's IX table. */
#define AFS_ELEMENT_MARK (('I' << 8) | 'X')

struct TAFSElement
{
    unsigned short flag;
    unsigned long pos;
    unsigned long size;
    unsigned long psize;
    unsigned char name[AFS_ELEMENT_NAME_SIZE];
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
