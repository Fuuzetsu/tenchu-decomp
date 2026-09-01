#ifndef TENCHU_TIMPACK_H
#define TENCHU_TIMPACK_H

#include "tim.h"

/* A TIM pack starts with its own ID, followed by this count/offset table. */
typedef struct
{
    u32 count;
    u32 offsets[1];
} TIMPackIndex;

#define TIM_PACK_IMAGE(offset_base, offset_entry)                  \
    TIM_FILE_IMAGE((int)(offset_base) + *(offset_entry))

#endif
