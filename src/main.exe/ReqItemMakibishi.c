#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemMakibishi(struct PARAM_ITEM_DROP *p);
 *     ITEM.C:1235, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_DROP * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_drop * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v0       int x
 *     reg   $v1       int y
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

extern void ProcItemMakibishi(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemMakibishi(PARAM_ITEM_DROP *p)
{
    TItem *item;
    TItem *ret;
    param_drop *param;
    VECTOR *pos;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT_VIA_CURSOR(found);
found:
    param = &item->param.drop;
    if (item == 0)
        return 0;
    {
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemMakibishi);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    x = p->vec.vx;
    y = p->vec.vy;
    z = p->vec.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    item->param.drop.koro.hint = 0;
    param->koro.status = KORO_NORMAL;
    SoundEx(pos, SE_CALTROP_THROW);
    return 1;
}
