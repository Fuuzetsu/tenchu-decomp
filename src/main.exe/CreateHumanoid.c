#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * CreateHumanoid(short type, unsigned long *mad);
 *     HUMAN.C:36, 31 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       short type
 *     param $s0       unsigned long * mad
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct ModelType World;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

extern void *vcalloc(u32 size, u8 c);
extern ModelArchiveType *LoadModelArchive(u_long *adr, ModelType *prnt);
extern void SetupThinkFunction(Humanoid *human, TThinkType type);

extern char msg_human_overflow[]; /* HUMAN OVERFLOW */

Humanoid *CreateHumanoid(character_kind type, unsigned long *mad)
{
    Humanoid *human;
    s16 conflict_id;
    u16 hh;
    u16 hh2;
    u16 ww;
    s32 half;
    s32 nhalf;
    s16 oldHumans;

    if (mad == 0 || Humans >= MAX_HUMANS)
    {
        SystemOut(msg_human_overflow);
    }
    human = (Humanoid *)vcalloc(sizeof(Humanoid), 0);
    human->type = type;
    human->status = STAT_NORMAL;
    human->attribute = 0;
    human->model = LoadModelArchive(mad, &World);
    human->locate = MODEL_POSITION(human->model);
    human->rotate = &human->model->rotate;
    human->model->attribute = MODEL_ATTR_CULL_BEHIND | MODEL_ATTR_CULL_SCREEN |
                              MODEL_ATTR_CULL_FAR;
    SetupThinkFunction(human, THINK_MIX_NONE);
    SetupCharacterParameter(type, human);
    hh = human->height;
    human->model->clip.vy = -((s16)hh / 2);
    UpdateMotion(human->motion, 0);
    GetAreaMapVector(GlobalAreaMap, &human->map, human->locate, human->width,
                     AREA_LEVEL_STEP_DOWN);
    SetupWeapon(human);
    conflict_id = InsertConflict(human->model->object[MODEL_PART_WAIST]);
    hh2 = human->height;
    ConflictObject[conflict_id].size.vy = half = (s16)hh2 / 2;
    nhalf = -half;
    ConflictObject[conflict_id].offset.vy =
        nhalf - human->model->rotate.pad;
    ww = human->width;
    ConflictObject[conflict_id].common = human;
    ConflictObject[conflict_id].size.vx =
        ConflictObject[conflict_id].size.vz =
        (s16)ww / 2;
    if (type == KUMA_0 || type == KUMA_1)
    {
        ConflictObject[conflict_id].offset.vy = -0x1C5;
        ConflictObject[conflict_id].offset.vz = 0xC0;
    }
    oldHumans = Humans;
    Humans++;
    HumanGroup[oldHumans] = human;
    return human;
}
