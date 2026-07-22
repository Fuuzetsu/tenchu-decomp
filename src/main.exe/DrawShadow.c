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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 *    VECTOR scale, MATRIX, SVECTOR screen result, and two long RotTransPers
 *    outputs.  No synthetic padding or scratch overlay is needed.
 *  - `height` comes from the archive's own `model->rotate.pad`, not from
 *    `object[0]`.  Keeping both archive reads on `human->model` lets CSE
 *    share that pointer and reproduces the target's argument-zero setup and
 *    object/rotation load order around GetAbsolutePosition.
 *  - The first status-3 random result is a block-local single-use temp so
 *    the rand call precedes the independent Y adjustment without retaining
 *    a copy.  The other random remainders stay inline; reusing one multi-def
 *    temp inserted four target-absent moves after the calls.
 *  - The EffectSlot scan is the established hand-written goto loop.  Its
 *    separate `effect` result is load-bearing even though it always equals
 *    `slot` on success: the transfer at the found edge produces the target's
 *    loop-exit move and restores the idx/count/slot register assignment.
 *  - D_80097F34 is viewed as ModelType: locate@0, rotate@0x50, and
 *    object@0x64 account for every use.  The chained scale assignment emits
 *    the target's reverse z/y/x stack-store order.
 */

extern short RefrectVector[16];
extern GsOT *OTablePt;
extern ModelType *D_80097F34;

extern VECTOR *GetAbsolutePosition(ModelType *model, s32 x, s32 y, s32 z);
extern void FUN_80037e0c(Humanoid *human, s32 mode);
extern void DrawSplash(TEffectSlot *ef);
void DrawShadow(Humanoid *human)
{
    VECTOR scale;
    MATRIX matrix;
    SVECTOR screen;
    s32 p;
    s32 flag;
    VECTOR *position;
    s32 height;
    u16 attribute;

    height = -human->model->rotate.pad;
    position = GetAbsolutePosition(human->model->object[0], 0, 0, 0);

    if (human->map.level < position->vy || human->map.level == (s32)0x80000000)
    {
        human->map.attrib |= 8;
    }
    position->vy = human->map.level;
    attribute = human->map.attrib;

    if (attribute & 8)
    {
        s32 vector_xy;

        vector_xy = *(s32 *)&human->vector;
        if ((vector_xy != 0 || human->motion->mid == 0x804 ||
             human->motion->mid == 0x710) &&
            human->map.height == 0 && (GameClock & 1) != 0)
        {
            s32 idx;
            s32 count;
            TEffectSlot *base;
            TEffectSlot *slot;
            TEffectSlot *effect;
            SplashType *param;
            s32 z;

            if (human->status == 3)
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

            idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
            count = 0;
            base = EffectSlot;
            slot = base + idx;
        loop:
            idx++;
            slot++;
            if (199 < idx)
            {
                slot = base;
                idx = 0;
            }
            if (slot->proc == 0)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
                if (199 < idx + 1)
                {
                    CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
                }
                effect = slot;
                goto found;
            }
            count++;
            if (199 < count)
            {
                effect = &dmy;
                goto found;
            }
            goto loop;
        found:
            param = &effect->param.splash;
            param->px = position->vx;
            param->py = position->vy;
            z = position->vz;
            param->sx = 0x2000;
            param->sy = 0x2000;
            param->speed = 4;
            param->mode = 0;
            param->pz = z;
            effect->proc = (void (*)())DrawSplash;
        }
    }
    else if (attribute & 0x100)
    {
        if (human->map.height == 0)
        {
            if ((GameClock & 0x3f) == 1)
            {
                FUN_80037e0c(human, 1);
            }
            else if ((GameClock & 0xf) == 0)
            {
                FUN_80037e0c(human, 0);
            }
        }
    }
    else
    {
        u8 angle;
        s32 depth;

        D_80097F34->locate.coord.t[0] = position->vx;
        D_80097F34->locate.coord.t[1] = position->vy;
        D_80097F34->locate.coord.t[2] = position->vz;

        scale.vx = scale.vy = scale.vz = height * 4 - (human->map.height >> 1);
        angle = *((u8 *)human + 0x2f);
        if (angle != 0)
        {
            D_80097F34->rotate.vx = 0x100;
            D_80097F34->rotate.vy = RefrectVector[*((u8 *)human + 0x2f)];
            D_80097F34->rotate.vz = 0;
        }
        else
        {
            D_80097F34->rotate.vx = 0;
            D_80097F34->rotate.vy = 0;
            D_80097F34->rotate.vz = 0;
        }

        RotMatrixYXZ(&D_80097F34->rotate, &D_80097F34->locate.coord);
        ScaleMatrix(&D_80097F34->locate.coord, &scale);
        D_80097F34->locate.flg = 0;
        GsGetLs(&D_80097F34->locate, &matrix);
        GsSetLsMatrix(&matrix);
        depth = RotTransPers(&UnitVector, (s32 *)&screen, &p, &flag);
        screen.vz = depth;
        if ((depth << 16) >> 18 < 0x4e2)
        {
            GsSortObject4(&D_80097F34->object, OTablePt, 2,
                          (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
        }
    }
}
