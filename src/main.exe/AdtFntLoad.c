#include "common.h"
#include "main.exe.h"

/*
 * AdtFntLoad (0x8005fc64) — font-adapter load: loads the font pattern into
 * VRAM via the PSYQ libgpu FntLoad(tx,ty), then stashes the (tx,ty) it was
 * loaded at into the font-adapter state D_8008F1B8 (same global as
 * AdtQuiet.c's `quiet` field at offset 0x20 — this TU only reaches offsets
 * 0x18/0x1C, so it redeclares its own narrower view per the project's
 * per-TU struct convention; see AdtQuiet.c / handle_char_state_attacking_-
 * SEVEN_.c's dtM_type for the same "declare only the fields you touch" idiom).
 * Same cached-parameter shape as AddXG4/AddXF4 just above it in address
 * space: both params are read twice (once as the call args, once as the
 * struct stores) so no separate temps are needed.
 */
typedef struct
{
    u8 pad[0x18];
    s32 tx;
    s32 ty;
} AdtFntState;

extern AdtFntState D_8008F1B8;
extern void FntLoad(int tx, int ty);

void AdtFntLoad(int tx, int ty)
{
    FntLoad(tx, ty);
    D_8008F1B8.tx = tx;
    D_8008F1B8.ty = ty;
}
