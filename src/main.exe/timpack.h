#ifndef TENCHU_TIMPACK_H
#define TENCHU_TIMPACK_H

#include "common.h"

/* A TIM pack starts with its own ID, followed by this count/offset table. */
typedef struct
{
    u32 count;
    u32 offsets[1];
} TIMPackIndex;

/* Each table offset lands on an entry ID immediately before the TIM data. */
typedef struct
{
    u32 id;
    u32 tim[1];
} TIMPackEntry;

#define TIM_PACK_ENTRY_DATA_OFFSET \
    ((u_long)&((TIMPackEntry *)0)->tim)
#define TIM_PACK_IMAGE(offset_base, offset_entry)                  \
    ((u_long *)((int)(offset_base) + *(offset_entry) +             \
                TIM_PACK_ENTRY_DATA_OFFSET))

#endif
