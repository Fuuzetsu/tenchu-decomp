#include "common.h"
#include "main.exe.h"

extern void _new_card(void);
extern long _card_write(long chan, long block, unsigned char *buf);

long _card_clear(long chan)
{
    _new_card();
    return _card_write(chan, 0x3f, (unsigned char *)0);
}
