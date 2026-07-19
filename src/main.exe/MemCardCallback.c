#include "common.h"
#include "main.exe.h"

/*
 * MemCardCallback (0x80081fec) — install a new memory-card driver callback and
 * return the previous one (the standard PSY-Q `old = f(new); ...; f(old);`
 * save/restore pattern used around every card operation: MemCardOpen,
 * MemCardGetDirentry, MemCardCreateFile, MemCardDeleteFile all call it in
 * pairs). `D_800CD7B8` is a ~0x20-byte path buffer (used by MemCardOpen via
 * strcat/open) that sits immediately before the callback slot, hence the pad.
 *
 * STATUS: MATCHING — 20 bytes. This is Sony's stock LIBMCRD implementation.
 * Its original object places the private callback scalar at BSS offset 0x74
 * and addresses it through one unsplit `la`. Keeping the slot as an explicit
 * pointer under LIBMCRD.OBJ's `-mno-split-addresses` compiler profile reproduces
 * that `lui v1` / `addiu v1,v1` pair; the normal game-TU split-address mode
 * folds the low relocation into the load/store instead and is one instruction
 * short. The profile is inherited by the complete original-object member list
 * in Build.hs, not selected for this function.
 */

typedef void (*MemCardCallbackFn)(u_long, u_long);

extern MemCardCallbackFn D_800CD7D8[];

MemCardCallbackFn MemCardCallback(MemCardCallbackFn func)
{
    MemCardCallbackFn *slot;
    MemCardCallbackFn old;

    slot = D_800CD7D8;
    old = *slot;
    *slot = func;
    return old;
}
