#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * BreedLife(short type, long x, long y, long z, long r);
 *     APPEAR.C:202, 50 src lines, frame 160 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       short type
 *     param $s5       long x
 *     param $s7       long y
 *     param $s6       long z
 *     param stack+16  long r
 *     reg   $v0       long r
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       unsigned long * model
 *     reg   $s0       unsigned long idx
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

extern int sprintf(char *buf, char *fmt, ...);
extern int strcmp(char *a, char *b);

extern char msg_illigal_character_type[]; /* ILLIGAL CHARACTER TYPE */
extern char fmt_mad[];                    /* %s%s.MAD */
extern char path_human[];                 /* K:\\WORK\\CDIMAGE\\HUMAN\\ */

Humanoid *BreedLife(character_kind type, long x, long y, long z, long r)
{
    /* PSX.SYM and the retail multiply both show a full-width counter. */
    u32 idx;
    HumanDataType *row;
    HumanDataType *base;
    u_long *model;
    HumanDataType *pp;
    HumanDataType *tbl;
    HumanDataType *q;
    Humanoid *human;
    u8 name[100];

    idx = 0;
    if (HumanData[0].type == CHARACTER_KIND_END)
        goto illegal_type;
    while (HumanData[idx].type != CHARACTER_KIND_END)
    {
        base = HumanData;
        if (base[idx].type == type)
            break;
        idx++;
    }
    if (base[idx].type != CHARACTER_KIND_END)
        goto type_found;
illegal_type:
    SystemOut(msg_illigal_character_type);
type_found:
    tbl = HumanData;
    pp = &tbl[idx];
    model = pp->model;
    if (model == 0)
    {
        sprintf((char *)name, fmt_mad, path_human, pp->name);
        model = FileRead(name);
        pp->model = model;
        if (HumanData[0].type != CHARACTER_KIND_END)
        {
            q = pp;
            row = HumanData;
        scan_next:
            if (strcmp((char *)q->name, (char *)row->name) == 0)
            {
                row->model = model;
            }
            row++;
            if (row->type != CHARACTER_KIND_END)
                goto scan_next;
        }
    }

    human = CreateHumanoid(type, model);
    human->point[HUMANOID_HOME_X] = x;
    human->model->locate.coord.t[0] = x;
    human->model->locate.coord.t[1] = GetAreaMapLevel(
        GlobalAreaMap, x, y, z, AREA_LEVEL_STEP_DOWN);
    human->point[HUMANOID_HOME_Z] = z;
    human->model->locate.coord.t[2] = z;
    human->model->rotate.vy = r;
    UpdateCoordinate((ModelType *)human->model);

    if (type == NINJA_0)
    {
        human->item[ITEM_KUSURI] = 1;
    }
    if (type >= ANI)
    {
        if (type >= ARROW)
            return human;
        if (type < S1)
            goto done;
        goto high_type;
    }

    if (type < HANBE)
    {
        if (type >= RIKIMARU_1)
            return human;
        if (type < 0)
            return human;
    }
    human->attribute = human->attribute | PHASE_ALERT;
    EquipWeapon(human, WEAPON_DRAWN);
    SetNowMotion(human, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
    return human;
high_type:
    human->attribute = human->attribute | ATTR_FLOAT;
done:
    return human;
}
