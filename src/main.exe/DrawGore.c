#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"
#include "images.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawGore(struct tag_EffectSlot *ef);
 *     EFFECT.C:1131, 28 src lines, frame 56 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_EffectSlot * ef
 *     reg   $s1       struct GoreType * param
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $t1       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $a3       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 * END PSX.SYM */

typedef union DrawGoreScratch
{
    SVECTOR screen;
    struct
    {
        SVECTOR velocity;
        VECTOR position;
        union
        {
            VECTOR position;
            SVECTOR velocity;
        } temporary;
    } bleed;
} DrawGoreScratch;

extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);
extern void DrawBleed(TEffectSlot *ef);

/*
 * MATCH. This is the retail form of EFFECT.C's DrawGore, installed by
 * SetGore. It is closely related to DrawBlood, but always emits a small
 * DrawBleed particle and uses a 60-unit position jitter. Retail radically
 * redesigns the demo's GoreType state into the BloodType view used here.
 *
 * The explicit scratch union is the original sp+0x18..sp+0x3f workspace:
 * the projection SVECTOR, bleed VECTOR, and temporary VECTOR/SVECTOR all
 * overlap, keeping the target's 0x60-byte frame. sprBloodStay is the original
 * name of the second blood-sprite bank; retail expands both demo singletons
 * to four sprites. Naming it separately is load-bearing because the target
 * materializes both bank bases independently. The named base_x/y/z values
 * prevent reassociation of `(position - 60) + rand()%120`,
 * and the full-width `green` local preserves the target's li 0x7f10 before a
 * byte store. The pool's iteration count is PSX.SYM's `i`; the scan's
 * existing `slot` also carries the found/fallback result, so a second
 * `found` alias is unnecessary. The retained `velocity` pointer is also a
 * measured aggregate-copy boundary: spelling its destination directly costs
 * 15 diff lines.
 */
void DrawGore(TEffectSlot *ef)
{
    enum
    {
        JITTER_RADIUS = 60,
        GORE_COLOR = RGB24(127, 16, 23)
    };
    BloodType *param;
    GsSPRITE *spr;
    GsSPRITE *spr2;
    DrawGoreScratch scratch;

    param = &ef->param.blood;
    spr = &sprBlood[param->sprite];
    spr2 = &sprBloodStay[param->sprite];
    switch (param->mode)
    {
    case 3:
    {
        s32 brightness;
        s32 size;
        s32 rotate;
        s16 scale;
        s32 sort_depth;
        s32 priority;

        param->brightness -= 5;
        if ((s16)param->brightness <= 0)
        {
            param->brightness = 0;
            ef->proc = 0;
        }

        spr->attribute = SPR_TRANS_ADD;
        param->py += param->vy;
        size = param->scale;
        rotate = param->rotate;
        brightness = (s16)param->brightness;
        GetScreenPosition(param->px, param->py, param->pz, &scratch.screen);
        if (scratch.screen.vz <= NEAR_DEPTH)
        {
            return;
        }
        scale = (s16)((size * PROJECTION_DISTANCE) / scratch.screen.vz) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
        spr2->scaley = scale;
        spr2->scalex = scale;
        spr->rotate = rotate;
        spr2->rotate = rotate;
        spr2->x = spr->x = scratch.screen.vx;
        spr2->y = spr->y = scratch.screen.vy;
        spr->r = (u8)brightness;
        spr->g = (u8)brightness;
        spr->b = (u8)brightness;
        spr2->r = (u8)(brightness / 2);
        spr2->g = (u8)(brightness / 2);
        spr2->b = (u8)(brightness / 2);

        sort_depth = (s16)(u16)scratch.screen.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(spr, OTablePt, (u16)priority);

        sort_depth = (s16)(u16)scratch.screen.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(spr2, OTablePt, (u16)priority);
        return;
    }

    case 2:
    {
        u16 count;

        count = param->time;
        param->time = count - 1;
        if ((s16)count <= 0)
        {
            param->time = 0x80;
            param->mode++;
        }
        break;
    }

    case 1:
    {
        u16 count;

        param->scale += rand() % FIXED_ONE;
        count = param->time;
        param->time = count - 1;
        if ((s16)count <= 0)
        {
            param->mode++;
            param->time = rand() % 90;
        }
        break;
    }

    default:
    {
        s32 x;
        s32 y;
        s32 z;
        s32 x10;
        s32 y10;
        s32 z10;
        s32 level;
        AreaNodeType *node;
        int scale_random;
        int random_x;
        int random_y;
        int random_z;
        s32 base_x;
        s32 base_y;
        s32 base_z;
        SVECTOR *velocity;
        long color;
        long green;
        int cursor;
        int i;
        TEffectSlot *slot;
        TEffectSlot *base;
        BleedType *bleed;

        x = param->px;
        y = param->py;
        z = param->pz;
        x10 = x / 10;
        y10 = y / 10;
        z10 = z / 10;
        param->vy += 10;
        node = param->hint;
        if (node == 0 || y10 < node->y - 200 || node->y < y10 ||
            x10 < node->x1 || z10 < node->z1 || node->x2 < x10 ||
            node->z2 < z10)
        {
            level = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z, 0);
            if (y <= level && FieldArea->division == -1)
            {
                param->hint = FieldArea;
            }
        }
        else if (node->dy != 0)
        {
            level = ComputeAreaLevel(node, x10, z10);
            if (level != LEVEL_NONE)
            {
                level *= 10;
            }
        }
        else
        {
            level = node->y * 10;
        }
        if (param->py >= level)
        {
            param->vz = 0;
            param->vy = 0;
            param->vx = 0;
            if (level != LEVEL_NONE)
            {
                param->py = level;
            }
            else
            {
                param->vy = rand() % 8 + 8;
                param->rotate = 0;
                scale_random = rand();
                param->sprite += 2;
                /* random scale in [1/3, 1/2) of 4.12 one */
                param->scale = scale_random % 0x2ab + 0x555;
            }
            param->mode = 1;
            param->time = rand() % 10;
            SoundEx((VECTOR *)&param->px, SE_BLOOD_SPLATTER);
        }
        else
        {
            u16 count;

            count = param->time;
            param->time = count - 1;
            if ((s16)count <= 0)
            {
                ef->proc = 0;
            }
        }

        memset(&scratch.bleed.temporary.position, 0, sizeof(VECTOR));
        random_x = rand();
        base_x = param->px - JITTER_RADIUS;
        scratch.bleed.temporary.position.vx =
            base_x + random_x % (JITTER_RADIUS * 2);
        random_y = rand();
        base_y = param->py - JITTER_RADIUS;
        scratch.bleed.temporary.position.vy =
            base_y + random_y % (JITTER_RADIUS * 2);
        random_z = rand();
        base_z = param->pz - JITTER_RADIUS;
        scratch.bleed.temporary.position.vz =
            base_z + random_z % (JITTER_RADIUS * 2);
        scratch.bleed.position = scratch.bleed.temporary.position;
        memset(&scratch.bleed.temporary.velocity, 0, sizeof(SVECTOR));
        velocity = &scratch.bleed.velocity;
        color = GORE_COLOR;
        scratch.bleed.temporary.velocity.vx = param->vx / 2;
        scratch.bleed.temporary.velocity.vy = param->vy / 2;
        scratch.bleed.temporary.velocity.vz = param->vz / 2;
        *velocity = scratch.bleed.temporary.velocity;

        base = EffectSlot;
        cursor = EFFECT_CURSOR_;
        slot = base + cursor;
        i = 0;
        do
        {
            cursor++;
            slot++;
            if (cursor > N_EFFECT_SLOTS - 1)
            {
                slot = base;
                cursor = 0;
            }
            i++;
            if (slot->proc == 0)
            {
                EFFECT_CURSOR_ = cursor + 1;
                bleed = &slot->param.bleed;
                if (EFFECT_CURSOR_ > N_EFFECT_SLOTS - 1)
                {
                    EFFECT_CURSOR_ = 0;
                }
                goto bleed_found;
            }
        } while (i < N_EFFECT_SLOTS);
        slot = &dmy;
        bleed = &dmy.param.bleed;
    bleed_found:
        slot->param.bleed.pos = scratch.bleed.position;
        slot->param.bleed.vec = *velocity;
        bleed->time = 7;
        bleed->r = GORE_COLOR >> 16;
        green = GORE_COLOR >> 8;
        bleed->g = green;
        bleed->b = color;
        bleed->mode = 0;
        slot->proc = (void (*)())DrawBleed;
        break;
    }
    }

    {
        s32 size;
        s32 otz;
        s16 scale;
        s32 sort_depth;
        s32 priority;

        param->px += param->vx;
        param->py += param->vy;
        param->pz += param->vz;
        spr->rotate = param->rotate;
        spr->attribute = 0;
        spr->r = param->brightness;
        spr->g = param->brightness;
        spr->b = param->brightness;
        size = param->scale;
        GetScreenPosition(param->px, param->py, param->pz, &scratch.screen);
        otz = scratch.screen.vz;
        if (otz <= NEAR_DEPTH)
        {
            return;
        }
        scale = (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
        spr->x = scratch.screen.vx;
        spr->y = scratch.screen.vy;
        sort_depth = (s16)(u16)scratch.screen.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(spr, OTablePt, (u16)priority);
    }
}
