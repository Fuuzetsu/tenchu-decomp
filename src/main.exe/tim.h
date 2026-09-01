#ifndef TENCHU_TIM_H
#define TENCHU_TIM_H

#include "common.h"

typedef struct TIMBlockPosition TIMBlockPosition;
typedef struct TIMBlockSize TIMBlockSize;
typedef struct TIMDataBlock TIMDataBlock;
typedef struct TIMImageData TIMImageData;
typedef struct TIMFile TIMFile;

/* Native-endian wire layout of a PlayStation TIM image. */
struct TIMBlockPosition
{
    s16 x;
    s16 y;
};

struct TIMBlockSize
{
    u16 width;
    u16 height;
};

struct TIMDataBlock
{
    u32 byte_size; /* includes this header and the payload */
    TIMBlockPosition position;
    TIMBlockSize size;
    u_long data[1];
};

struct TIMImageData
{
    u32 mode;
    TIMDataBlock blocks[1]; /* optional CLUT block, then pixels */
};

struct TIMFile
{
    u32 id;
    TIMImageData image;
};

enum tim_pixel_mode
{
    TIM_PIXEL_MODE_4BPP = 0,
    TIM_PIXEL_MODE_8BPP = 1,
    TIM_PIXEL_MODE_16BPP = 2,
    TIM_PIXEL_MODE_24BPP = 3,
    TIM_PIXEL_MODE_MASK = 3
};

enum
{
    TIM_FILE_ID = 0x10,
    TIM_CLUT_FLAG_SHIFT = 3,
    TIM_BYTE_TO_WORD_SHIFT = 2
};

enum tim_mode_flag
{
    TIM_MODE_HAS_CLUT = 1 << TIM_CLUT_FLAG_SHIFT
};

#define TIM_PIXEL_MODE(mode) ((mode) & TIM_PIXEL_MODE_MASK)
#define TIM_HAS_CLUT(mode) (((mode) >> TIM_CLUT_FLAG_SHIFT) & 1)
#define TIM_FILE_IMAGE(file) \
    ((u_long *)&((TIMFile *)(file))->image.mode)

#endif
