#ifndef TENCHU_TMDFILE_H
#define TENCHU_TMDFILE_H

#include "common.h"
#include <psxsdk/libgs.h>

/* Mapped linked-TMD header and the ID-prefixed file wrapper used by archives. */
typedef struct
{
    u_long flags;
    u_long object_count;
    struct TMD_STRUCT objects[1];
} TMDData;

typedef struct
{
    u_long id;
    TMDData data;
} TMDFile;

enum
{
    TMD_FLAG_MAPPED = 1,
    TMD_OBJECT_WORDS = sizeof(struct TMD_STRUCT) / sizeof(u_long)
};

#define TMD_FILE_BYTE_OFFSET(member) ((u_long)&((TMDFile *)0)->member)
#define TMD_DATA_BYTE_OFFSET(member) ((u_long)&((TMDData *)0)->member)
#define TMD_FILE_DATA(file)                                        \
    ((u_long *)((int)(file) + TMD_FILE_BYTE_OFFSET(data)))
#define TMD_DATA_OBJECTS(data)                                     \
    ((u_long *)((int)(data) + TMD_DATA_BYTE_OFFSET(objects)))
#define TMD_FILE_OBJECTS(file) TMD_DATA_OBJECTS(TMD_FILE_DATA(file))

#endif
