#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawBlood(struct tag_EffectSlot *ef);
 *     EFFECT.C:657, 85 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s0       struct tag_EffectSlot * ef
 *     reg   $s2       struct BloodType * blood
 *     reg   $s3       struct GsSPRITE * spr
 *     reg   $s2       struct AreaNodeType ** hint
 *     reg   $t3       long x
 *     reg   $t2       long y
 *     reg   $a3       long z
 *     reg   $a2       int sz
 *     reg   $t1       int sy
 *     reg   $a1       int sx
 *     reg   $v1       long rety
 *     reg   $a0       struct AreaNodeType * area
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vec
 *     reg   $s3       struct GsSPRITE * sprt
 *     reg   $s0       long scale
 *     stack sp+24     struct SVECTOR scr
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsSPRITE sprBloodStay;
 *     extern struct GsOT *OTablePt;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern long GameClock;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawBlood (0x80032950, EFFECT.C:657) updates the four-phase blood-sprite
 * state machine, performs its inline area-height/collision query, emits a
 * small bleed particle on alternating frames, and projects/sorts either one
 * or two blood sprites.
 *
 * Matching notes:
 *  - The state dispatch is a switch whose physical source body order is
 *    3, 2, 1, default.  cc1's balanced comparison tree still tests state 2
 *    first, while retaining that body layout.
 *  - `DrawBloodScratch` spells the exact stack overlay mechanically reported
 *    by stackplan: scr@sp+0x18, pos@sp+0x20, and a reusable aggregate scratch
 *    at sp+0x30.  Building a VECTOR/SVECTOR in `temp` and assigning it out
 *    reproduces the target's word and unaligned aggregate copies.
 *  - The default terrain path is CGetLevel's matched guard shape inlined.
 *    Computing `sy` before loading/computing `z` is load-bearing: it gives the
 *    target x/y/z divide schedule and t2/s0/a3 register assignment.
 *  - In state 3, assigning `color_signed` before GetScreenPosition makes the
 *    value live across the call in pre-schedule RTL.  The scheduler then moves
 *    the actual sign-extension into the following guard's delay slot.  This
 *    creates the target's sixth saved register and exact s0-s5 allocation
 *    without any register-asm steering.
 *  - The three bleed jitter axes need distinct rand and base locals.  Each
 *    `base = blood->p? - R` sits after rand in the C but schedules between
 *    the multiply and its magic-divide tail, matching the target.  Reusing one
 *    rand/base local instead adds moves or delays the position load.
 *  - Write the final x integration before y even though the scheduled target
 *    stores y first; this is the same source-order/scheduler distinction seen
 *    in the matched effect donors.
 *  - The two scale divisions have runtime divisors, so this function needs
 *    maspsx `--expand-div` in both Build.hs and permute.py.
 */

typedef struct DrawBloodScratch
{
    SVECTOR scr;
    VECTOR pos;
    VECTOR temp;
} DrawBloodScratch;

extern long ComputeAreaLevel(AreaNodeType *area, long x, long z);
extern void *memset(void *dst, int value, u32 size);

void DrawBlood(TEffectSlot *ef)
{
    enum
    {
        R = 80
    };
    BloodType *blood;
    GsSPRITE *spr;
    GsSPRITE *sprt;
    DrawBloodScratch scratch;
    u8 index;
    u8 state;
    s32 color_signed;

    blood = &ef->param.blood;
    index = blood->sprite;
    spr = &sprBlood[index];
    sprt = &sprBloodStay[index];

    state = blood->mode;
    switch (state)
    {
    case 3:
    {
        s16 fade;
        s32 color_shifted;
        s16 sc;
        s16 screen_x;
        s16 screen_y;
        s32 scale;
        long rotate;
        long y;
        s32 otz;
        s32 t;
        s32 pri;
        s32 half;

        fade = blood->brightness;
        fade = fade - 5;
        blood->brightness = fade;
        if (fade <= 0)
        {
            blood->brightness = 0;
            ef->proc = 0;
        }
        spr->attribute = SPR_TRANS_ADD;
        scale = blood->scale;
        y = blood->py + blood->vy;
        blood->py = y;
        rotate = blood->rotate;
        color_shifted = (u32)blood->brightness << 16;
        color_signed = color_shifted >> 16;
        GetScreenPosition(blood->px, y, blood->pz, &scratch.scr);
        otz = scratch.scr.vz;
        if (otz < 0x25)
        {
            return;
        }
        sc = (s16)((scale * 300) / otz) + 1;
        spr->scaley = sc;
        spr->scalex = sc;
        sprt->scaley = sc;
        sprt->scalex = sc;
        spr->rotate = rotate;
        sprt->rotate = rotate;
        screen_x = scratch.scr.vx;
        spr->x = screen_x;
        sprt->x = screen_x;
        screen_y = scratch.scr.vy;
        spr->y = screen_y;
        sprt->y = screen_y;
        spr->r = (u8)color_signed;
        spr->g = (u8)color_signed;
        spr->b = (u8)color_signed;
        half = color_signed / 2;
        sprt->r = (u8)half;
        sprt->g = (u8)half;
        sprt->b = (u8)half;

        t = (s16)(u16)scratch.scr.vz >> 2;
        if (t >= 0)
        {
            pri = 0x4e1;
            if (t < 0x4e2)
            {
                pri = t;
            }
        }
        else
        {
            pri = 0;
        }
        GsSortSprite(spr, OTablePt, (u16)pri);

        t = (s16)(u16)scratch.scr.vz >> 2;
        if (t >= 0)
        {
            pri = 0x4e1;
            if (t < 0x4e2)
            {
                pri = t;
            }
        }
        else
        {
            pri = 0;
        }
        GsSortSprite(sprt, OTablePt, (u16)pri);
        return;
    }

    case 2:
    {
        u16 oldtime;

        oldtime = blood->time;
        blood->time = oldtime - 1;
        if ((s16)oldtime <= 0)
        {
            blood->time = 0x80;
            blood->mode++;
        }
        break;
    }

    case 1:
    {
        s32 scale_rnd;
        s32 time_rnd;
        u16 oldtime;

        scale_rnd = rand();
        blood->scale = blood->scale + scale_rnd % 0x1000;
        oldtime = blood->time;
        blood->time = oldtime - 1;
        if ((s16)oldtime <= 0)
        {
            blood->mode++;
            time_rnd = rand();
            blood->time = time_rnd % 90;
        }
        break;
    }

    default:
    {
        long x;
        long y;
        long z;
        int sx;
        int sy;
        int sz;
        long rety;
        AreaNodeType *area;
        u16 oldtime;
        s32 vy_rnd;
        s32 scale_rnd;
        s32 time_rnd;
        s32 bleed_x;
        s32 bleed_y;
        s32 bleed_z;
        s32 bleed_time;
        long base_x;
        long base_y;
        long base_z;

        x = blood->px;
        y = blood->py;
        sx = x / 10;
        blood->vy = blood->vy + 10;
        area = (AreaNodeType *)blood->hint;
        sy = y / 10;
        z = blood->pz;
        sz = z / 10;
        if (area == 0 || area->y - 200 > sy || sy > area->y ||
            sx < area->x1 || sz < area->z1 || area->x2 < sx || area->z2 < sz)
        {
            rety = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z, 0);
            if (y <= rety && FieldArea->division == -1)
            {
                blood->hint = FieldArea;
            }
        }
        else if (area->dy != 0)
        {
            rety = ComputeAreaLevel(area, sx, sz);
            if (rety != (long)0x80000000)
            {
                rety = rety * 10;
            }
        }
        else
        {
            rety = area->y * 10;
        }

        if (blood->py >= rety)
        {
            blood->vz = 0;
            blood->vy = 0;
            blood->vx = 0;
            if (rety != (long)0x80000000)
            {
                blood->py = rety;
            }
            else
            {
                vy_rnd = rand();
                blood->vy = vy_rnd % 8 + 8;
                blood->rotate = 0;
                scale_rnd = rand();
                blood->sprite = blood->sprite + 2;
                blood->scale = scale_rnd % 0x2ab + 0x555;
            }
            blood->mode = 1;
            time_rnd = rand();
            blood->time = time_rnd % 10;
            SoundEx((VECTOR *)&blood->px, 0x37);
        }
        else
        {
            oldtime = blood->time;
            blood->time = oldtime - 1;
            if ((s16)oldtime <= 0)
            {
                ef->proc = 0;
            }
        }

        if (GameClock & 1)
        {
            memset(&scratch.temp, 0, sizeof(VECTOR));
            bleed_x = rand();
            base_x = blood->px - R;
            scratch.temp.vx = base_x + bleed_x % (R * 2);
            bleed_y = rand();
            base_y = blood->py - R;
            scratch.temp.vy = base_y + bleed_y % (R * 2);
            bleed_z = rand();
            base_z = blood->pz - R;
            scratch.temp.vz = base_z + bleed_z % (R * 2);
            scratch.pos = scratch.temp;
            memset(&scratch.temp, 0, sizeof(SVECTOR));
            ((SVECTOR *)&scratch.temp)->vx = blood->vx / 2;
            ((SVECTOR *)&scratch.temp)->vy = blood->vy / 2;
            ((SVECTOR *)&scratch.temp)->vz = blood->vz / 2;
            scratch.scr = *(SVECTOR *)&scratch.temp;
            bleed_time = rand();
            SetBleed(&scratch.pos, &scratch.scr, bleed_time % 10 + 10, 0x7f1017);
        }
        break;
    }
    }
{
    s16 sc;
    s32 scale;
    s32 otz;
    s32 t;
    s32 pri;

    blood->px = blood->px + blood->vx;
    blood->py = blood->py + blood->vy;
    blood->pz = blood->pz + blood->vz;
    spr->rotate = blood->rotate;
    spr->attribute = 0;
    spr->r = blood->brightness;
    spr->g = blood->brightness;
    spr->b = blood->brightness;
    scale = blood->scale;
    GetScreenPosition(blood->px, blood->py, blood->pz, &scratch.scr);
    otz = scratch.scr.vz;
    if (otz < 0x25)
    {
        return;
    }
    sc = (s16)((scale * 300) / otz) + 1;
    spr->scaley = sc;
    spr->scalex = sc;
    spr->x = scratch.scr.vx;
    spr->y = scratch.scr.vy;
    t = (s16)(u16)scratch.scr.vz >> 2;
    if (t >= 0)
    {
        pri = 0x4e1;
        if (t < 0x4e2)
        {
            pri = t;
        }
    }
    else
    {
        pri = 0;
    }
    GsSortSprite(spr, OTablePt, (u16)pri);
}
}
