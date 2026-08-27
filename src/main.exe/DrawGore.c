#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawGore(struct tag_EffectSlot *ef);
 *     EFFECT.C:1131, 28 src lines, frame 56 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 * materializes both bank bases independently. `node_y` and
 * `level` must remain separate around ComputeAreaLevel so the flat and sloped
 * paths cross-jump through the target multiply tail.  Likewise, the named
 * bleed_x/y/z values prevent reassociation of `(position - 60) + rand()%120`,
 * and the full-width `green` local preserves the target's li 0x7f10 before a
 * byte store.
 */
void DrawGore(TEffectSlot *ef)
{
    BloodType *param;
    GsSPRITE *spr;
    GsSPRITE *spr2;
    DrawGoreScratch scratch;
    u32 index;
    int state;

    param = &ef->param.blood;
    index = param->sprite;
    spr = &sprBlood[index];
    spr2 = &sprBloodStay[index];
    state = param->mode;

    switch (state)
    {
    case 3:
    {
        u16 fade;
        s32 fade_shift;
        s32 brightness;
        s32 half_brightness;
        s32 x;
        s32 y;
        s32 z;
        s32 size;
        s32 rotate;
        s32 otz;
        s16 scale;
        s16 screen_x;
        s16 screen_y;
        s32 value;
        s32 priority;

        fade = param->brightness - 5;
        param->brightness = fade;
        if ((s16)fade <= 0)
        {
            param->brightness = 0;
            ef->proc = 0;
        }

        spr->attribute = 0x50000000;
        x = param->px;
        y = param->py + param->vy;
        z = param->pz;
        size = param->scale;
        param->py = y;
        rotate = param->rotate;
        fade = param->brightness;
        fade_shift = (u32)fade << 16;
        brightness = (s16)fade;
        GetScreenPosition(x, y, z, &scratch.screen);
        otz = scratch.screen.vz;
        if (otz < 0x25)
        {
            return;
        }
        scale = (s16)((size * 300) / otz) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
        spr2->scaley = scale;
        spr2->scalex = scale;
        spr->rotate = rotate;
        spr2->rotate = rotate;
        screen_x = scratch.screen.vx;
        spr->x = screen_x;
        spr2->x = screen_x;
        screen_y = scratch.screen.vy;
        spr->y = screen_y;
        spr2->y = screen_y;
        half_brightness = brightness / 2;
        spr->r = (u8)brightness;
        spr->g = (u8)brightness;
        spr->b = (u8)brightness;
        spr2->r = (u8)half_brightness;
        spr2->g = (u8)half_brightness;
        spr2->b = (u8)half_brightness;

        value = (s32)((u16)scratch.screen.vz << 16) >> 0x12;
        if (value >= 0)
        {
            priority = 0x4e1;
            if (value < 0x4e2)
            {
                priority = value;
            }
        }
        else
        {
            priority = 0;
        }
        GsSortSprite(spr, OTablePt, (u16)priority);

        value = (s32)((u16)scratch.screen.vz << 16) >> 0x12;
        if (value >= 0)
        {
            priority = 0x4e1;
            if (value < 0x4e2)
            {
                priority = value;
            }
        }
        else
        {
            priority = 0;
        }
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

        param->scale += rand() % 0x1000;
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
        s32 node_y;
        AreaNodeType *node;
        int r;
        int random_x;
        int random_y;
        int random_z;
        s32 bleed_x;
        s32 bleed_y;
        s32 bleed_z;
        u16 count;
        SVECTOR *temporary;
        SVECTOR *velocity;
        long color;
        long green;
        int cursor;
        int searched;
        TEffectSlot *slot;
        TEffectSlot *base;
        TEffectSlot *found;
        BleedType *bleed;

        x = param->px;
        y = param->py;
        z = param->pz;
        x10 = x / 10;
        y10 = y / 10;
        z10 = z / 10;
        param->vy += 10;
        node = param->hint;
        if (node == 0 || y10 < (node_y = node->y) - 200 || node_y < y10 ||
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
            if (level != (s32)0x80000000)
            {
                level *= 10;
            }
        }
        else
        {
            level = node_y * 10;
        }
        if (param->py >= level)
        {
            param->vz = 0;
            param->vy = 0;
            param->vx = 0;
            if (level != (s32)0x80000000)
            {
                param->py = level;
            }
            else
            {
                param->vy = rand() % 8 + 8;
                param->rotate = 0;
                r = rand();
                param->sprite += 2;
                param->scale = r % 0x2ab + 0x555;
            }
            param->mode = 1;
            param->time = rand() % 10;
            SoundEx((VECTOR *)&param->px, 0x37);
        }
        else
        {
            count = param->time;
            param->time = count - 1;
            if ((s16)count <= 0)
            {
                ef->proc = 0;
            }
        }

        memset(&scratch.bleed.temporary.position, 0, sizeof(VECTOR));
        random_x = rand();
        bleed_x = param->px - 60;
        scratch.bleed.temporary.position.vx = bleed_x + random_x % 120;
        random_y = rand();
        bleed_y = param->py - 60;
        scratch.bleed.temporary.position.vy = bleed_y + random_y % 120;
        random_z = rand();
        bleed_z = param->pz - 60;
        scratch.bleed.temporary.position.vz = bleed_z + random_z % 120;
        scratch.bleed.position = scratch.bleed.temporary.position;
        temporary = &scratch.bleed.temporary.velocity;
        memset(&scratch.bleed.temporary.velocity, 0, sizeof(SVECTOR));
        velocity = &scratch.bleed.velocity;
        color = 0x7f1017;
        scratch.bleed.temporary.velocity.vx = param->vx / 2;
        scratch.bleed.temporary.velocity.vy = param->vy / 2;
        scratch.bleed.temporary.velocity.vz = param->vz / 2;
        *velocity = *temporary;

        base = EffectSlot;
        cursor = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        slot = base + cursor;
        searched = 0;
        do
        {
            cursor++;
            slot++;
            if (199 < cursor)
            {
                slot = base;
                cursor = 0;
            }
            searched++;
            if (slot->proc == 0)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = cursor + 1;
                bleed = &slot->param.bleed;
                if (199 < CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_)
                {
                    CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
                }
                found = slot;
                goto bleed_found;
            }
        } while (searched < 200);
        found = &dmy;
        bleed = &dmy.param.bleed;
    bleed_found:
        found->param.bleed.pos = scratch.bleed.position;
        found->param.bleed.vec = *velocity;
        bleed->time = 7;
        bleed->r = 0x7f;
        green = 0x7f10;
        bleed->g = green;
        bleed->b = color;
        bleed->mode = 0;
        found->proc = (void (*)())DrawBleed;
        break;
    }
    }

    {
        s32 size;
        s32 otz;
        s16 scale;
        s32 value;
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
        if (otz < 0x25)
        {
            return;
        }
        scale = (s16)((size * 300) / otz) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
        spr->x = scratch.screen.vx;
        spr->y = scratch.screen.vy;
        value = (s32)((u16)scratch.screen.vz << 16) >> 0x12;
        if (value >= 0)
        {
            priority = 0x4e1;
            if (value < 0x4e2)
            {
                priority = value;
            }
        }
        else
        {
            priority = 0;
        }
        GsSortSprite(spr, OTablePt, (u16)priority);
    }
}
