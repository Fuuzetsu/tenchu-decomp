#include "common.h"
#include "main.exe.h"

/*
 * _card_clear (0x80082ab4) — stock PsyQ libcard (Ghidra places it in CARD.OBJ):
 * reset the card state, then wipe the directory by writing block 0x3f from a
 * NULL buffer, returning _card_write's result. `chan` is cached in a
 * callee-saved register across _new_card so the second call can pass it.
 */
extern void _new_card(void);
extern long _card_write(long chan, long block, unsigned char *buf);

long _card_clear(long chan)
{
    _new_card();
    return _card_write(chan, 0x3f, (unsigned char *)0);
}
