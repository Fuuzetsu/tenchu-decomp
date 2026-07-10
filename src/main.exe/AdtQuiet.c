#include "common.h"
#include "main.exe.h"

typedef struct
{
    u8 pad[0x20];
    s32 quiet;
} AdtFnt;

extern AdtFnt D_8008F1B8;

s32 AdtQuiet(s32 quiet)
{
    s32 old = D_8008F1B8.quiet;
    D_8008F1B8.quiet = quiet;
    return old;
}
