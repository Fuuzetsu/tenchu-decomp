#include "common.h"
#include "main.exe.h"

/*
 * PutLifeBarS (0x8004ad54, 0x90 bytes) — draws up to 5 life bars from the
 * LifeBar[5] pool (stride 0x14), each entry counting down its own display
 * timer. Bottom-test-only do-while: i=0,i<5 provably true at entry folds
 * away the top test (cookbook Loops). Indexed as LifeBar[i].f rather than
 * a walking pointer — a p++ walk biases the base to whichever field is
 * touched LAST in the body (here the count field), giving negative
 * displacements for the others; array indexing keeps the natural
 * offsets 4/8/C/10 (cookbook Loops: "Index the table T[i].f ... when the
 * loop touches 2+ fields"). Twin: DrawEffect.c (0.16), same TU as
 * PutItemIcon.c/PutItemCursor.c.
 */
typedef struct
{
    u8 pad0[4];
    s32 life;
    s32 max;
    s32 count;
    s32 style;
} tag_LifeBarEntry; /* 0x14 */

extern tag_LifeBarEntry LifeBar[5];
extern void PutLifeBar(s32 x, s32 y, s32 life, s32 lifemax, s32 mode);

s32 PutLifeBarS(void)
{
    s32 i;
    s32 x;

    i = 0;
    x = -0x8C;
    do
    {
        if (LifeBar[i].count > 0)
        {
            PutLifeBar(x, -0x5A, LifeBar[i].life, LifeBar[i].max, LifeBar[i].style);
            LifeBar[i].count = LifeBar[i].count - 1;
        }
        x = x + 0x3C;
        i = i + 1;
    } while (i < 5);
    return 0;
}
