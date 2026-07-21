#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * AdtFntLoad (0x8005fc64) — font-adapter load: loads the font pattern into
 * VRAM via the PSYQ libgpu FntLoad(tx,ty), then stashes the (tx,ty) it was
 * loaded at into the font-adapter state D_8008F1B8 (same global as
 * AdtQuiet.c's `quiet` field at offset 0x20).
 * Same cached-parameter shape as AddXG4/AddXF4 just above it in address
 * space: both params are read twice (once as the call args, once as the
 * struct stores) so no separate temps are needed.
 */
extern AdtFntState D_8008F1B8;

void AdtFntLoad(int tx, int ty)
{
    FntLoad(tx, ty);
    D_8008F1B8.tx = tx;
    D_8008F1B8.ty = ty;
}
