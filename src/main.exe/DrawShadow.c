#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawShadow(struct Humanoid *human);
 *     EFFECT.C:1572, 89 src lines, frame 168 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct Humanoid * human
 *     stack sp+24     struct VECTOR scl
 *     reg   $s1       struct VECTOR * loc
 *     stack sp+40     struct MATRIX mat
 *     reg   $s0       int height
 *     reg   $s1       struct VECTOR * pos
 *     reg   $v1       struct SplashType * param
 *     reg   $a1       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct ModelType * model
 *     stack sp+112    struct VECTOR pos
 *     stack sp+128    struct SVECTOR sv
 *     reg   $t2       short time
 *     reg   $s1       struct _GsCOORDINATE2 * super
 *     reg   $v1       struct FrameType * param
 *     reg   $t1       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 *     stack sp+72     struct SVECTOR scr
 *     stack sp+148    long flag
 *     stack sp+144    long p
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern struct ModelType *ShadowMdl;
 *     extern short RefrectVector[16];
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawShadow (0x80038394, EFFECT.C:1572) updates the humanoid's ground
 * position/attributes, emits a splash while the character is moving on an
 * eligible frame, or builds and draws the flattened ground-shadow model.
 *
 * Matching notes:
 *  - The retail 0x60-byte frame is the natural packing of the original
 *    VECTOR scl, MATRIX mat, SVECTOR scr result, and two long RotTransPers
 *    outputs.  No synthetic padding or scratch overlay is needed.
 *  - `height` comes from the archive's own `model->rotate.pad`, not from
 *    `object[0]`.  Keeping both archive reads on `human->model` lets CSE
 *    share that pointer and reproduces the target's argument-zero setup and
 *    object/rotation load order around GetAbsolutePosition.
 *  - The first status-3 random result is a block-local single-use temp so
 *    the rand call precedes the independent Y adjustment without retaining
 *    a copy.  The other random remainders stay inline; reusing one multi-def
 *    temp inserted four target-absent moves after the calls.
 *  - The EffectSlot scan directly indexes `EffectSlot[idx]` in a bottom-tested
 *    loop. Strength reduction generates the target's pointer walk; `slot`
 *    carries only the found/fallback result recorded by PSX.SYM.
 *  - ShadowMdl is viewed as ModelType: locate@0, rotate@0x50, and
 *    object@0x64 account for every use.  The chained scl assignment emits
 *    the target's reverse z/y/x stack-store order.
 */

extern void DrawSplash(TEffectSlot *ef);
void DrawShadow(Humanoid *human)
{
    VECTOR scl;
    MATRIX mat;
    SVECTOR scr;
    s32 p;
    s32 flag;
    VECTOR *position;
    s32 height;

    height = -human->model->rotate.pad;
    position = GetAbsolutePosition(human->model->object[MODEL_PART_WAIST], 0, 0, 0);

    if (human->map.level < position->vy || human->map.level == LEVEL_NONE)
    {
        human->map.attrib |= MAP_WOOD; /* airborne overload */
    }
    position->vy = human->map.level;
    if (human->map.attrib & MAP_WOOD)
    {
        if ((human->vector.vx != 0 || human->vector.vy != 0 ||
             human->motion->mid == MOT_STATE_LAND ||
             human->motion->mid == MOT_ATTACK_DIVE_LAND) &&
            human->map.height == 0 && (GameClock & 1) != 0)
        {
            s32 idx;
            s32 count;
            TEffectSlot *slot;
            SplashType *param;
            s32 z;

            if (human->status == STAT_SWIM)
            {
                s32 r = rand();

                position->vy += 100;
                position->vx += r % 600 - 300;
                position->vz += rand() % 600 - 300;
            }
            else
            {
                position->vx += rand() % 200 - 100;
                position->vz += rand() % 200 - 100;
            }

            FIND_EFFECT_SLOT(idx, count, slot, found);
        found:
            param = &slot->param.splash;
            param->px = position->vx;
            param->py = position->vy;
            z = position->vz;
            param->sx = 0x2000;
            param->sy = 0x2000;
            param->speed = 4;
            param->mode = SPLASH_MODE_SPAWN;
            param->pz = z;
            slot->proc = DrawSplash;
        }
    }
    else if (human->map.attrib & MAP_DAMAGE)
    {
        if (human->map.height == 0)
        {
            if ((GameClock & 0x3f) == 1)
            {
                spawn_damage_effect_(human, DAMAGE_EFFECT_NAPALM);
            }
            else if ((GameClock & 0xf) == 0)
            {
                spawn_damage_effect_(human, DAMAGE_EFFECT_ATTACHED_FLASH);
            }
        }
    }
    else
    {
        ShadowMdl->locate.coord.t[0] = position->vx;
        ShadowMdl->locate.coord.t[1] = position->vy;
        ShadowMdl->locate.coord.t[2] = position->vz;

        scl.vx = scl.vy = scl.vz = height * 4 - (human->map.height >> 1);
        if (human->map.angleH != 0)
        {
            ShadowMdl->rotate.vx = 0x100;
            ShadowMdl->rotate.vy = RefrectVector[human->map.angleH];
            ShadowMdl->rotate.vz = 0;
        }
        else
        {
            ShadowMdl->rotate.vx = 0;
            ShadowMdl->rotate.vy = 0;
            ShadowMdl->rotate.vz = 0;
        }

        RotMatrixYXZ(&ShadowMdl->rotate, &ShadowMdl->locate.coord);
        ScaleMatrix(&ShadowMdl->locate.coord, &scl);
        ShadowMdl->locate.flg = 0;
        GsGetLs(&ShadowMdl->locate, &mat);
        GsSetLsMatrix(&mat);
        scr.vz = RotTransPers(&UnitVector, (s32 *)&scr, &p, &flag);
        if (scr.vz >> 2 < DEPTH_LIMIT)
        {
            GsSortObject4(&ShadowMdl->object, OTablePt, 2,
                          (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
        }
    }
}
