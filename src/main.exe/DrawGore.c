#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include "images.h"

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

/*
 * DrawGore (0x8003562c) — the per-frame effect callback SetGore
 * installs for a flung gore chunk; a four-phase state machine on
 * param->mode over the retail BloodType view. The default phase is the
 * flight: vy gains 10 per frame, and the floor height comes either
 * from the cached AreaNodeType hint (its y band and x1/z1/x2/z2 box
 * tested at 1/10 scale, ComputeAreaLevel when the node slopes) or,
 * when the hint no longer contains the chunk, from a fresh
 * GetAreaMapLevel probe at y - 300 that re-caches FieldArea. On
 * landing (py past the level) the velocity is zeroed and py pinned to
 * the floor; if the probe returned LEVEL_NONE instead it kicks back up
 * with vy = rand() % 8 + 8, resets rotate, advances `sprite` by 2 to
 * the splat pair and picks a scale of rand() % 683 + 1365 — a third to
 * a half of the 4.12 unit. Either way it enters mode 1 with
 * time = rand() % 10 and plays sound 0x37 at its own position. Not
 * landing just runs the countdown down and clears ef->proc at zero.
 * Every default-phase frame also drops one DrawBleed droplet: position
 * jittered by the R-unit per axis, velocity halved, colour 0x7f1017
 * and time 7, taken from the shared 200-slot EffectSlot pool by the
 * usual EFFECT_CURSOR_ round-robin scan, falling into the throwaway
 * `dmy` slot after 200 misses. Mode 1 swells the chunk by rand() %
 * 4096 each frame until its countdown expires, then hands to mode 2
 * with time = rand() % 90. Mode 2 only counts down, then sets
 * time = 0x80 and enters mode 3. Mode 3 is the settled pool:
 * brightness fades by 5 a frame and clears ef->proc at zero, and it
 * draws BOTH banks at one position — sprBlood[sprite] additively
 * (SPR_TRANS_ADD) at full brightness and sprBloodStay[sprite] at half
 * — sharing scale, rotation and screen position. Modes 0-2 fall
 * through to the shared tail, which integrates position by velocity
 * and sorts the single sprBlood sprite. Both draw paths project with
 * GetScreenPosition, return without drawing when the screen z is at or
 * under NEAR_DEPTH, scale by (size * 300) / z + 1, and sort into
 * OTablePt at z >> 2 clamped to DEPTH_LIMIT - 1 (negatives to 0).
 */

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
    enum
    {
        R = 60
    };
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

        spr->attribute = SPR_TRANS_ADD;
        x = param->px;
        y = param->py + param->vy;
        z = param->pz;
        size = param->scale;
        param->py = y;
        rotate = param->rotate;
        fade = param->brightness;
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

        value = (s16)(u16)scratch.screen.vz >> 2;
        if (value >= 0)
        {
            priority = DEPTH_LIMIT - 1;
            if (value < DEPTH_LIMIT)
            {
                priority = value;
            }
        }
        else
        {
            priority = 0;
        }
        GsSortSprite(spr, OTablePt, (u16)priority);

        value = (s16)(u16)scratch.screen.vz >> 2;
        if (value >= 0)
        {
            priority = DEPTH_LIMIT - 1;
            if (value < DEPTH_LIMIT)
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
            if (level != LEVEL_NONE)
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
            if (level != LEVEL_NONE)
            {
                param->py = level;
            }
            else
            {
                param->vy = rand() % 8 + 8;
                param->rotate = 0;
                r = rand();
                param->sprite += 2;
                /* random scale in [1/3, 1/2) of 4.12 one */
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
        bleed_x = param->px - R;
        scratch.bleed.temporary.position.vx = bleed_x + random_x % (R * 2);
        random_y = rand();
        bleed_y = param->py - R;
        scratch.bleed.temporary.position.vy = bleed_y + random_y % (R * 2);
        random_z = rand();
        bleed_z = param->pz - R;
        scratch.bleed.temporary.position.vz = bleed_z + random_z % (R * 2);
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
        cursor = EFFECT_CURSOR_;
        slot = base + cursor;
        searched = 0;
        do
        {
            cursor++;
            slot++;
            if (cursor > 199)
            {
                slot = base;
                cursor = 0;
            }
            searched++;
            if (slot->proc == 0)
            {
                EFFECT_CURSOR_ = cursor + 1;
                bleed = &slot->param.bleed;
                if (EFFECT_CURSOR_ > 199)
                {
                    EFFECT_CURSOR_ = 0;
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
        value = (s16)(u16)scratch.screen.vz >> 2;
        if (value >= 0)
        {
            priority = DEPTH_LIMIT - 1;
            if (value < DEPTH_LIMIT)
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
