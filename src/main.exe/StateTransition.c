#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StateTransition(struct Humanoid *human);
 *     THINK.C:99, 109 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct Humanoid * human
 *     reg   $s2       short pad
 *     reg   $s1       short atr0
 *     reg   $s3       long ssr
 *     reg   $s0       short motid
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern long StrainRatio;
 *     extern struct PADtype *Pad;
 *     extern short Attrib;
 *     extern short ActionHalt;
 *     extern long Distance;
 *     extern short Degree;
 *     extern short SR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short FieldAttrib;
 *     extern unsigned char gNannido;
 *     extern short EngageLevel;
 *     extern short Findenemies;
 *     extern long GameClock;
 * END PSX.SYM */

/*
 * StateTransition (0x8002aad0) is the central humanoid AI-state dispatcher:
 * it updates perception, alert/engage state, movement hints, obstacle probes,
 * and the synthesized controller input for one frame.
 *
 * Matching notes (3,776 bytes / 942 compared instructions):
 *  - This is THINK.C code, so its TU-local small globals need the explicit
 *    StateTransition gp-extern list in Build.hs/permute.py.  The random engage
 *    test also needs maspsx's `--expand-div` compatibility sequence.
 *  - The three alert arms deliberately contain the same Findenemies tail.
 *    gcc's cross-jump pass merges those copies while carrying the already
 *    loaded Humanoid pointer into the join.  The case-0 boolean switch leaves
 *    a real label barrier so jump.c does not also merge its preceding type
 *    check with case 1.
 *  - The first FieldAttrib store is the comma side effect in the fifth
 *    GetAreaMapLevel argument.  This keeps the value live until all four
 *    register arguments have been loaded, matching the original store
 *    schedule and allocation without a fixed-register declaration.
 *  - The packed word at Humanoid+0xb0 is read with `word >> 16`, not a halfword
 *    pointer cast: the former gives the target `lh` plus delayed copy to pad.
 *  - The one-shot `do` around the case-0 chase resets emits no control-flow
 *    instructions.  Its loop-depth notes weight the two pointer uses enough
 *    for local-alloc to choose the target's $v0/$v1 order naturally.
 *  - Repeating `me->target` for the two coordinate reads makes cse preserve
 *    the loaded target pointer with the target's explicit copy.  The >=-form
 *    ternaries likewise expand the two absolute values directly as abssi2.
 *  - reset_alert_duration has an old-style declaration intentionally.  The
 *    case-0 call carries the already-loaded life value in $a0; the callee takes
 *    no arguments, but preserving that harmless call-site value keeps jump.c
 *    from cross-jumping the call itself with the other alert arms.
 */

extern Humanoid *StagePlayer;
extern Humanoid *Me_THINK_C;
extern PADtype *Pad;
extern s32 StrainRatio;
extern u16 Attrib;
extern s16 ActionHalt;
extern s32 Distance;
extern s16 Degree;
extern s16 SR;
extern u_long *GlobalAreaMap;
extern s16 FieldAttrib;
extern s16 EngageLevel;
extern s16 Findenemies;
extern s32 GameClock;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern s32 D_80097F10;
extern s32 D_80097F14;
extern u16 D_80097F18[2];
extern s32 D_80097F1C;
extern u8 gNannido;

extern s16 GetDirection(s32 dx, s32 dz, s16 roty);
extern s16 SearchTarget(Humanoid *human, s32 *distance, s16 *degree);
extern void GetMoveSpeed(SVECTOR *vect, s16 ry, s16 order, s16 side);
extern s32 GetAreaMapLevel(u32 *area, s32 x, s32 y, s32 z, u16 mode);
extern short SetNowMotion(Humanoid *human, short mid, short move);
extern short Sound(Humanoid *human, s32 seid);
extern void reset_alert_duration();
extern s16 Think2confirm(void);
extern s16 think_setting_small_rotation_small_steps_(void);
extern s16 Think3firstattack(void);
extern s16 turn_towards_player_(s32 x, s32 z);
extern s16 SetCommand(PADtype *pad, s16 command);
extern s16 update_pressed_buttons(PADtype *pad, s16 pressed);
extern s32 Think1ninja(void);
extern s32 rand(void);

void StateTransition(Humanoid *human)
{
    s16 pad;
    s16 atr0;
    s32 ssr;
    s16 motid;
    s32 distance;
    SVECTOR vect;

    ssr = StrainRatio;
    Me_THINK_C = human;
    Pad = &human->pad;
    Attrib = human->attribute;
    atr0 = Attrib & 0xffec;

    if (human == StagePlayer)
    {
        D_80097F1C = ssr;
        StrainRatio = 0x7fffffff;
    }

    if ((u16)(human->status - 0x10) < 2)
    {
        if (human != StagePlayer && human->life > 0 && StrainRatio > 0)
        {
            StrainRatio = -0x8000;
        }
        update_pressed_buttons(Pad, 0);
        return;
    }

    if (ActionHalt != 0)
    {
        s32 dx;
        s32 dz;
        s16 direction;

        pad = 0;
        if ((u16)(human->type - 0x10) < 0x70)
        {
            dx = human->target->locate.coord.t[0] - human->locate->vx;
            dz = human->target->locate.coord.t[2] - human->locate->vz;
            direction = GetDirection(dx, dz, human->rotate->vy);
            if (direction > Me_THINK_C->turn)
            {
                pad = 0x2000;
            }
            else
            {
                if (-Me_THINK_C->turn > direction)
                {
                    pad = -0x8000;
                }
            }
            if (SquareRoot0(dx * dx + dz * dz) < 2000)
            {
                pad |= 0x4000;
            }
        }
        update_pressed_buttons(Pad, pad);
        return;
    }

    if ((Attrib & 4) == 0)
    {
        if (human == StagePlayer && FRAMES_UNTIL_END_OF_ALERT != 0)
        {
            FRAMES_UNTIL_END_OF_ALERT--;
            if (FRAMES_UNTIL_END_OF_ALERT < 0)
            {
                FRAMES_UNTIL_END_OF_ALERT = 0;
            }
        }
        pad = ((think_func_)Me_THINK_C->think[0])();
        update_pressed_buttons(Pad, pad);
        return;
    }

    SR = SearchTarget(human, &Distance, &Degree);
    if (Me_THINK_C->target == (ModelType *)StagePlayer->model)
    {
        distance = Distance;
    }
    else
    {
        s32 dx;
        s32 dy;
        s32 dz;

        dx = StagePlayer->locate->vx - Me_THINK_C->locate->vx;
        dy = StagePlayer->locate->vy - Me_THINK_C->locate->vy;
        dz = StagePlayer->locate->vz - Me_THINK_C->locate->vz;
        distance = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }

    {
        s16 active_item;

        active_item = StagePlayer->itmctl;
        if (active_item == 0xb || active_item == 0x12)
        {
            u16 kind;

            kind = Me_THINK_C->type & 0xf0;
            if (kind != 0x80 && kind != 0xa0 &&
                (active_item != 0x12 || (Attrib & 3) != 2))
            {
                if (FRAMES_UNTIL_END_OF_ALERT != 0)
                {
                    FRAMES_UNTIL_END_OF_ALERT = 0;
                }
                SR = -2;
            }
        }
        else if (FRAMES_UNTIL_END_OF_ALERT != 0)
        {
            if (FRAMES_UNTIL_END_OF_ALERT == 1)
            {
                SR = -1;
            }
            else if ((Attrib & 3) == 0)
            {
                SR = 2;
            }
            else if (SR == 2)
            {
                SR = 1;
            }
        }
    }

    GetMoveSpeed(&vect, Me_THINK_C->rotate->vy,
                 (s16)(Me_THINK_C->width * 2), 0);
    D_80097F10 = GetAreaMapLevel(GlobalAreaMap,
                                 Me_THINK_C->locate->vx + vect.vx,
                                 Me_THINK_C->locate->vy - 0xbea,
                                 Me_THINK_C->locate->vz + vect.vz, 0x1a);
    {
        u16 field_attrib;

        field_attrib = FieldAttrib;
        D_80097F14 = GetAreaMapLevel(GlobalAreaMap,
                                     Me_THINK_C->locate->vx - vect.vx,
                                     Me_THINK_C->locate->vy - 0xbea,
                                     Me_THINK_C->locate->vz - vect.vz,
                                     (D_80097F18[0] = field_attrib, 0x1a));
    }
    D_80097F18[1] = FieldAttrib;

    switch (Attrib & 3)
    {
    case 0:
        if (distance < StrainRatio)
        {
            StrainRatio = distance;
        }
        pad = ((think_func_)Me_THINK_C->think[0])();
        if (Me_THINK_C->type >= 7)
        {
            if (SR == 1)
            {
                Humanoid *me;
                s32 life;

                SetNowMotion(Me_THINK_C, 0x80e, 1);
                if (SetNowMotion(Me_THINK_C, 0x106, 1) == 0)
                {
                    Sound(Me_THINK_C, 0xd);
                }
                me = Me_THINK_C;
                life = me->life;
                Attrib = atr0 | 2;
                do
                {
                    me->chase[1] = 0;
                    me->chase[0] = 0;
                } while (0);
                if (life > 0)
                {
                    Humanoid *alert_me;

                    reset_alert_duration(life);
                    alert_me = Me_THINK_C;
                    switch (alert_me->type < 0x80)
                    {
                    case 0:
                        goto after_state;
                    default:
                        if (alert_me->target == (ModelType *)StagePlayer->model)
                        {
                            Findenemies++;
                        }
                        goto after_state;
                    }
                }
            }
            else if (SR == 2)
            {
                if (FRAMES_UNTIL_END_OF_ALERT != 0)
                {
                    SetNowMotion(Me_THINK_C, 0x80e, 1);
                    Me_THINK_C->chase[1] = 0;
                    Me_THINK_C->chase[0] = 0;
                }
                Attrib = atr0 | 1;
                Sound(Me_THINK_C, 0xc);
            }
        }
        goto after_state;

    case 1:
        if (FRAMES_UNTIL_END_OF_ALERT != 0 || (Attrib & 0x10) != 0)
        {
            if (StrainRatio > 0)
            {
                StrainRatio = -0x8000;
            }
            if (Attrib & 0x10)
            {
                pad = think_setting_small_rotation_small_steps_();
            }
            else
            {
                pad = Think2confirm();
            }
        }
        else
        {
            if (StrainRatio > 0 || StrainRatio < -distance)
            {
                StrainRatio = -distance;
            }
            pad = ((think_func_)Me_THINK_C->think[1])();
        }

        if (SR == 1 || ((Attrib & 0x4000) != 0 && SR > 0))
        {
            Attrib = atr0 | 2;
            if ((Attrib & 0x40) == 0)
            {
                SetNowMotion(Me_THINK_C, 0x80e, 1);
            }
            Me_THINK_C->chase[1] = 0;
            Me_THINK_C->chase[0] = 0;
            Sound(Me_THINK_C, 0xd);
            if (Me_THINK_C->life > 0)
            {
                Humanoid *me;

                reset_alert_duration();
                me = Me_THINK_C;
                if (me->type < 0x80)
                {
                    if (me->target == (ModelType *)StagePlayer->model)
                    {
                        Findenemies++;
                    }
                }
            }
        }
        else if (FRAMES_UNTIL_END_OF_ALERT < 2 &&
                 (u16)(SR + 2) < 2)
        {
            if ((Me_THINK_C->type & 0xf0) != 0x80)
            {
                SetNowMotion(Me_THINK_C, 0x80f, 1);
            }
            Attrib = atr0;
        }
        goto after_state;

    case 2:
    {
        if (Attrib & 0x40)
        {
            if (Me_THINK_C->target == (ModelType *)StagePlayer->model)
            {
                StrainRatio = 0;
            }
            else
            {
                if (StrainRatio > 0)
                {
                    StrainRatio = -0x8000;
                }
            }
        }
        else
        {
            StrainRatio = -1;
        }

        if (Attrib & 0x10)
        {
            pad = ((think_func_)Me_THINK_C->think[2])();
        }
        else
        {
            if ((Attrib & 0x40) == 0)
            {
                SetNowMotion(Me_THINK_C, 0x80e, 1);
            }
            pad = Think3firstattack();
        }

        if (pad & 0x80)
        {
            Humanoid *me;
            s32 dy;
            s32 random;
            s32 me_y;
            s32 target_y;

            if (StagePlayer->motion->mid == 0x1009)
            {
                goto mask_attack;
            }

            me = Me_THINK_C;
            target_y = me->target->locate.coord.t[1];
            me_y = me->locate->vy;
            dy = target_y - me_y;
            dy = dy >= 0 ? dy : -dy;
            if (dy < 2000)
            {
                goto random_attack;
            }
            if ((me->wpatk >> 4) == 3)
            {
                goto random_attack;
            }

        mask_attack:
            pad &= 0xf000;
            goto attack_checked;

        random_attack:
            random = rand();
            if (random % 4 - 2 >= (s32)gNannido)
            {
                pad = 0;
            }
        }

    attack_checked:
        if (SR == -2)
        {
            Humanoid *me;
            s32 target_x;
            s32 target_z;

            me = Me_THINK_C;
            target_x = me->target->locate.coord.t[0];
            Attrib = atr0 | 0x13;
            me->chase[0] = target_x;
            target_z = me->target->locate.coord.t[2];
            me->actscnt = 1;
            me->chase[1] = target_z;
        }

        if (Me_THINK_C->field76_0xb0 == 0)
        {
            if ((pad & 0x4000) &&
                ((D_80097F18[1] & 0x204) || D_80097F14 > 5000))
            {
                Me_THINK_C->field76_0xb0 = 0x1000001e;
            }
            if ((pad & 0x1000) &&
                ((D_80097F18[0] & 0x204) || D_80097F10 > 5000))
            {
                pad = turn_towards_player_(0, 0) & 0xa000;
            }
            if (StagePlayer->motion->mid == 0xe01 &&
                (rand() % (EngageLevel + 1) == 0 ||
                 (Me_THINK_C->type & 0xf0) == 0x80))
            {
                motid = (rand() & 1) ? 3 : 4;
                pad = SetCommand(&Me_THINK_C->pad, motid);
            }
            goto after_state;
        }
        goto update_hint;
    }

    case 3:
        if (StrainRatio > 0)
        {
            StrainRatio = -0x8000;
        }
        pad = ((think_func_)Me_THINK_C->think[3])();
        if ((Attrib & 0x400) && Me_THINK_C->field76_0xb0 == 0)
        {
            Me_THINK_C->field76_0xb0 =
                Degree > 0 ? 0x20000008 : 0x80000008;
        }
        if ((Attrib & 3) == 2)
        {
            Humanoid *me;

            Sound(Me_THINK_C, 0xd);
            reset_alert_duration();
            me = Me_THINK_C;
            if (me->type < 0x80 && (Attrib & 0x10) == 0)
            {
                if (me->target == (ModelType *)StagePlayer->model)
                {
                    Findenemies++;
                }
            }
        }
        goto after_state;
    }

after_state:
    if (Me_THINK_C->field76_0xb0 != 0)
    {
update_hint:
        pad = Me_THINK_C->field76_0xb0 >> 16;
        {
            s32 count;

            count = *(u8 *)&Me_THINK_C->field76_0xb0 - 1;
            if (count != 0)
            {
                Me_THINK_C->field76_0xb0 = (pad << 16) | count;
            }
            else if (pad & 0xa000)
            {
                Me_THINK_C->field76_0xb0 =
                    ((rand() % 3 + 1) * 0x1e) | 0x10000000;
            }
            else
            {
                Me_THINK_C->field76_0xb0 = 0;
            }
        }
    }

    {
        Humanoid *me;

        me = Me_THINK_C;
        if (me->status == 10)
        {
            s32 degree;
            s32 abs_degree;

            degree = Degree;
            abs_degree = degree;
            abs_degree = abs_degree >= 0 ? abs_degree : -abs_degree;
            pad = 0x1000;
            if (abs_degree >= 500)
            {
                pad = 0x4000;
                if ((Attrib & 3) == 0)
                {
                    pad = 0x1000;
                }
                else
                {
                    s32 hint;

                    hint = 0x8000000f;
                    if (degree > 0)
                    {
                        hint = 0x2000000f;
                    }
                    me->field76_0xb0 = hint;
                }
            }
        }
        else if ((D_80097F18[0] & 0x204) && (pad & 0x1000))
        {
            pad &= 0xefff;
        }
        else if ((D_80097F18[1] & 0x204) && (pad & 0x4000))
        {
            pad &= 0xbfff;
        }
        else if (Me_THINK_C->motion->count == 0)
        {
            s32 abs_degree;

            abs_degree = Degree;
            if (abs_degree < 0)
            {
                abs_degree = -abs_degree;
            }
            if (abs_degree < 500 &&
                ((think_func_)Me_THINK_C->think[0] == (think_func_)Think1ninja ||
                 ((Me_THINK_C->type & 0xf0) == 0x20 && gNannido != DIFFICULTY_EASY)))
            {
                s32 level;
                s32 next_level;
                s32 abs_next;

                GetMoveSpeed(&vect, Me_THINK_C->rotate->vy,
                             (s16)(Me_THINK_C->width * 5), 0);
                level = GetAreaMapLevel(GlobalAreaMap,
                                        Me_THINK_C->locate->vx,
                                        Me_THINK_C->locate->vy - 0xbea,
                                        Me_THINK_C->locate->vz, 0x19);
                next_level = GetAreaMapLevel(GlobalAreaMap,
                                             Me_THINK_C->locate->vx + vect.vx,
                                             Me_THINK_C->locate->vy - 0xbea,
                                             Me_THINK_C->locate->vz + vect.vz,
                                             0x1a);
                if (level == Me_THINK_C->map.level)
                {
                    abs_next = next_level >= 0 ? next_level : -next_level;
                    if (abs_next < 500)
                    {
                        goto set_obstacle_pad;
                    }
                }
                if (next_level >= 0x17d5)
                {
    set_obstacle_pad:
                    pad = 0x1040;
                }
                goto tail;
            }
            goto periodic_check;
        }
        else
        {
    periodic_check:
            if (GameClock == (GameClock / 90) * 90 &&
                (((u16)Me_THINK_C->map.attrib & 0x100) ||
                 ((pad & 0x1000) && D_80097F10 < 0x899 &&
                  D_80097F10 != (s32)0x80000000) ||
                 ((pad & 0x4000) && D_80097F14 < 0x899 &&
                  D_80097F14 != (s32)0x80000000)))
            {
                pad |= 0x40;
            }
        }
    }

tail:
    if (Me_THINK_C->life < 0)
    {
        StrainRatio = ssr;
    }
    Me_THINK_C->attribute = Attrib;

    update_pressed_buttons(Pad, pad);
}
