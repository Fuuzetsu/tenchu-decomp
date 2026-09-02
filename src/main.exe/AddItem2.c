#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "infoview.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void AddItem2(void);
 *     INFOVIEW.C:958, 27 src lines, frame 240 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct PARAM_ITEM_STAY param
 *     reg   $s2       long x
 *     reg   $s0       long y
 *     reg   $s1       long z
 *     stack sp+24     struct TAdtSelect [25] ItemName
 *     stack sp+48     struct SVECTOR vec
 * END PSX.SYM */

extern char str_select_item[]; /* "select item" */
extern SVECTOR svec_y_n600[];                    /* smoke-puff velocity/offset const */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);

void AddItem2(void)
{
    s32 n;
    s32 sx, cx;
    s32 x, y, z;
    s32 h;
    ModelArchiveType *pm;

    {
        {
            TAdtSelect ItemName[ITEM_N];

            __builtin_memcpy(ItemName, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                             sizeof(ItemName));
            n = AdtSelect(str_select_item, ItemName, 0);
        }
    }

    {
        PARAM_ITEM_STAY param;
        SVECTOR vec;

        memset(&param, 0, sizeof(param));
        param.type = n;

        sx = rsin(CamState.Owner->model->rotate.vy) * 1000;
        pm = CamState.Owner->model;
        if (sx < 0)
            sx += FIXED_TRUNC_BIAS;
        h = pm->locate.coord.t[1];
        y = h;
        x = pm->locate.coord.t[0] - (sx >> FIXED_SHIFT);
        cx = rcos(pm->rotate.vy) * 1000;
        pm = CamState.Owner->model;
        z = pm->locate.coord.t[2] - (cx / FIXED_ONE);
        h = GetAreaMapLevel(GlobalAreaMap, x, y, z, AREA_LEVEL_STEP_DOWN);
        if (h != LEVEL_NONE)
        {
            param.locate.vx = x;
            param.locate.vy = h;
            param.locate.vz = z;
            ReqItemStay(&param);
            vec = svec_y_n600[0];
            SetSmoke(&param.locate, &vec, 3, 10);
        }
    }
}
