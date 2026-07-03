#include "common.h"
#include "main.exe.h"

/*
 * ProcItemManebue (0x8004a1d8) — the manebue (lure whistle) item processor.
 * mode 0: silence the alert, set the owner's whistling state, play the sound,
 * arm a 30-frame timer; mode 1: count the timer down, then dispose of the item
 * (call its proc, drop the conflict, clear owner/proc).
 *
 * Matching notes (all verified against the original bytes):
 *  - `p = &item->param` mirrors the original's cached pointer (it lives in $s1
 *    across the calls); indexing off `item` directly doesn't allocate $s1.
 *  - The `zero` variable and the goto ladder reproduce the original dispatch:
 *    cc1 CSEs a plain `mode == 0` chain into one load, but the original reloads
 *    `mode` after the 0xff test and keeps the case bodies out of line.
 *  - FRAMES_UNTIL_END_OF_ALERT is a plain small extern here, and this file is
 *    deliberately NOT in Build.hs's maspsxGpExterns list: the original item TU
 *    did not define it (think's TU does), so ASPSX addressed it absolutely
 *    (lui $at) — unlike Think1sleep, where the same symbol is gp-relative.
 */
typedef struct tag_TItem tag_TItem;

/* Partial owner (Ghidra: Humanoid) — only the 2-byte field the whistle sets. */
typedef struct { u8 pad[0xae]; s16 active_item; } manebue_owner;

/* param union at 0x20; the whistle only touches a byte timer at +0xC. */
typedef struct { u8 pad0[0xc]; u8 timer; u8 pad1[0x27]; } item_param;

struct tag_TItem
{
    manebue_owner *owner;        /* 0x00 (Humanoid *) */
    void *model;                 /* 0x04 */
    item_kind2 type;             /* 0x08 */
    void (*proc)(tag_TItem *);   /* 0x0c */
    void *locate;                /* 0x10 */
    int coll_mode;               /* 0x14 */
    int coll_pause;              /* 0x18 */
    s16 coll_size;               /* 0x1c */
    s16 coll_ofsY;               /* 0x1e */
    item_param param;            /* 0x20 */
    u8 mode;                     /* 0x54 */
};

extern void SoundEx(VECTOR *loc, int id);
extern void DeleteConflict(void *model);
extern void AdtMessageBox(char *fmt, ...);
extern char D_800121CC[];

void ProcItemManebue(tag_TItem *item)
{
    item_param *p;
    u8 cVar1;
    s32 zero;

    p = &item->param;
    zero = 0;
    if (item->mode == 0xff)
    {
        item->mode = 0;
        return;
    }
    if (item->mode == zero)
        goto mode0;
    if (item->mode == 1)
        goto mode1;
    return;
mode0:
    FRAMES_UNTIL_END_OF_ALERT = 0;
    item->owner->active_item = item->type;
    SoundEx((VECTOR *)0x0, 0x43);
    p->timer = 0x1e;
    item->mode = item->mode + 1;
    return;
mode1:
    cVar1 = p->timer - 1;
    p->timer = cVar1;
    if (cVar1 == 0)
    {
        item->owner->active_item = 0;
        if (item->proc != 0)
        {
            item->mode = 0xff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
    }
}
