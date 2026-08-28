#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"

extern Humanoid *Me_MOTION_C;
extern s16 ARMOUR_EQUIPPED_;
extern Humanoid *DeadHumanoid;
/* MOTION.C's original direction-to-damage-animation table. */
extern s16 damagemotion[8];

extern int ReqLifeBar(Humanoid *h);
extern void reset_alert_duration(void);
extern void AttackCancelControl(s16 mode);
extern s16 PlayMotion(MotionManager *mmp, s16 mode);
extern void set_impact_ex_(VECTOR *pos, GsCOORDINATE2 *super,
                           short start_size, short end_size,
                           long start_color, long end_color,
                           s16 rotate, u16 rotate_speed, u16 time, u16 type);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DamageControl(void);
 *     MOTION.C:408, 236 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     stack sp+16     struct VECTOR p
 *     reg   $s1       struct VECTOR * pp
 *     reg   $s2       short i
 *     reg   $s0       short id
 *     reg   $s4       short deg
 *     reg   $s1       short dmg
 *     reg   $s5       short did
 *     reg   $s3       struct Humanoid * enemy
 *     reg   $v1       short i
 *     stack sp+32     struct SVECTOR pv
 *     reg   $v1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct VECTOR *dtL;
 *     extern short motMODE;
 *     extern short Criticals;
 *     extern short Murders;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short ActionHalt;
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR ConflictDistance;
 *     extern struct SVECTOR *dtR;
 *     extern short FriendHits;
 *     extern unsigned long *GlobalAreaMap;
 *     extern unsigned char gNannido;
 *     extern short EngageLevel;
 *     extern struct SVECTOR *dtV;
 * END PSX.SYM */

/*
 * DamageControl (0x8001d6bc) — resolves item and humanoid collisions into
 * damage, knockback, animation, blood, score, and player-feedback effects.
 *
 * STATUS: MATCH — 5812 bytes / 1453 instructions, byte-exact including the
 * compiled switch's own .rodata jump table (was 1320 -> 112 across the first
 * Fable escalation, 112 -> 90 -> 6 -> 0 in the continuation; the first pass
 * re-derived the source decomposition from the target RTL shapes and landed
 * the s16 hp life-decrement tie; the continuation recovered the id/t/deg
 * identity and the assigned-abs knockback shape, see below).  Fence-free.
 *
 * What this session PROVED (the prior "below the C level / HARD-CONFLICT"
 * verdict was an artifact of the old decomposition, not a cc1 limit):
 *  - The dominant $a1->$a0 family was caused by the function-spanning cached
 *    pointer locals (pHVar14/pHVar17/pSVar1).  The original uses NO caches:
 *    each region loads Me_MOTION_C / StagePlayer / dtR / dtV fresh, and cc1's
 *    per-region CSE temps then land in $a0/$v1 exactly like retail.  Killing
 *    the caches dissolved every "hard conflict" rtlguide reported.
 *  - The do-while(0)+goto guard scaffolding around the 0x602 engage block is
 *    not original.  The real shape is a plain nested if with an else arm
 *    (`if (rand() % (EngageLevel+1) == 0) { type checks; if (rand()&1)
 *    motID=0x602; } else { motID=0x602; }`); cc1's cross-jump + eager delay
 *    fill reproduce the shared `sh motID` tail with per-predecessor `li 0x602`
 *    delay slots automatically.  Same for ITEM_NAPALM: a plain
 *    `if ((rand()&1)==0) motID=0x1003; else motID=0x1001;` (no next_mot temp,
 *    no fence) and ITEM_MAKIBISHI storing motID/motMODE directly; the shared
 *    store is cross-jumped, giving `j DC08 / li v0,0x100A` for that case.
 *  - `-(x/3) - 1` must be spelled `x / -3 - 1`: cc1 folds -x-1 into nor, but a
 *    NEGATIVE divisor makes expmed emit the reversed magic-division subtract
 *    (subu sign,hi) with a plain addiu -1 — the retail shape at both sites.
 *  - `dmg <<= 1` under enemy->itmctl==ITEM_GOSIN is
 *    `(u32)(dmg << 0x10) >> 0xf`
 *    (sll 16 / srl 15).  The old `(dmg<<16); dmg>>=0xf` truncated to zero via
 *    the short lvalue (real behavior bug, compiled to `move s1,zero`).
 *  - The armour block computes deg BEFORE the knockback: `deg=(short)dmg>>3;
 *    clamp; clamp; t=(short)dmg*5/2+0x50;` — no cached `(u16)dmg<<16`
 *    temp; every read re-extends dmg so the sll is shared/rematerialized.
 *  - Both ReqLifeBar sites are if/else (`who=enemy` in the taken arm, else
 *    `who=Me_MOTION_C`), which lets who coalesce with the Me load in $a0 and
 *    compile the else arm to zero code.
 *  - GetAbsolutePosition's third arg is (short)-converted at the call site
 *    (sll/sra interleaved into the pointer chain); set_impact_ex_'s rot arg is
 *    an s16 param (sll/sra, not andi — prototype changed in this TU), and its
 *    `rand() % 360` is precomputed into a temp so the 0xB60B60B7 magic pair
 *    forms before the 0xDCDCDC pair.
 *  - The exit block loads dtV INSIDE the mid==0x300/0x302 arm; the pad.time
 *    identical-arm fence was scaffolding and is gone.
 *  - PSX.SYM roles that survive: dmg=$s1, enemy=$s3, did=$s4, id=$s5.
 *
 * What the CONTINUATION proved (112 -> 90):
 *  - `id` is an INT loaded via `(u16)vector.pad` (lhu s5) with `(short)id`
 *    casts at every signed use (sll/sra 0x10).  The prior `s8 id` was
 *    BALLAST: its lbu/sll24 bytes were wrong, but the QI->HI conversion kept
 *    2 extra flow-time refs on t that held the deg/t allocation
 *    order.  With the correct width those refs belong to the switch-head
 *    extension TEMP (both sides read $s0=temp: `addu a1,s0` args), so the
 *    order had to come from somewhere real:
 *  - The passage halving loop is a REAL `while` loop, not the Ghidra
 *    if+goto.  cc1 duplicates the 3-way abs entry test at -O2 (identical
 *    bytes), and the NOTE_INSN_LOOP notes make flow2 count the body's
 *    `t <<= 1` refs at loop weight: p84 14->16 refs = priority
 *    2413->3678, restoring t > deg > enemy = s0/s2/s3 (dmg stays s1).
 *    19 single-register rows (incl. the %100 magic in s2 and the blood
 *    counter in s0) fell together.  regalloc.py's `--between 82 87 84`
 *    window plus the .flow dump's `Register N used M times` lines are the
 *    measurement loop for this class.
 *
 * What the 90 -> 0 step proved (the 0x8001e7d8-e858 knockback family):
 *  - The knockback abs is the ASSIGNED-abs statement
 *    `ad = __builtin_abs(did);` — cc1's mips abssi2 is ONE
 *    type-"multi" insn whose template emits `bgez %1,1f%# / subu %0,$0,%0 /
 *    1:` internally (same-register form; identical bytes to bgez/nop/negu).
 *    Because the branch lives INSIDE the template, reorg never sees an
 *    unfilled bgez: nothing can steal the `move s0,a1` t copy out of
 *    the lhu load-delay slot (the explicit `if (ad < 0) ad = -ad;` spelling
 *    exposes a real branch whose backward scan ALWAYS steals that copy —
 *    provably, from reorg.c's fill_simple_delay_slots), and the missing
 *    block boundary lets the whole surrounding schedule and allocation
 *    (dtR=$v1, vy=$v0, newvy=$a0 fresh) fall out with NO fence.  The
 *    unconditional dtR->vy store sits between the abs and the <0x400 test,
 *    where reorg lands it in the beqz delay.
 *  - The deg==3 arm is `MoveHumanoid(Me, (0x400 < __builtin_abs(
 *    (int)(short)did)) ? 0x46 : -0x46, 0)` — the abs INSIDE the call's
 *    ternary arg: a0=Me evaluates first (lw at the arm top, e830), the
 *    ±0x46 branches jump straight to the call point (no move_speed
 *    variable, no extra j/lw pair; the default-then-override spelling cost
 *    +4 length and 18 bytes).
 */

void DamageControl(void)
{
    MotionManager *mmp;
    short did;
    short deg;
    short ret;
    short t;
    short newvy;
    int ad;
    Humanoid *enemy;
    int id;
    short dmg;
    SVECTOR dir;
    VECTOR p;
    SVECTOR pv;

    id = (u16)(Me_MOTION_C->vector).pad;
    dmg = 0;
    if (Me_MOTION_C->life < 1)
    {
        return;
    }
    if (MotionUpdateMode != 0)
    {
        return;
    }
    if (motID == 0x301)
    {
        return;
    }
    if (Me_MOTION_C == StagePlayer)
    {
        SetCameraMode(CMODE_NORMAL);
    }
    if ((Me_MOTION_C->type & 0xf0U) == PAGE_BEAST)
    {
        enemy = (Humanoid *)ConflictObject[(short)id].common;
        if (enemy != (Humanoid *)CONFLICT_OWNER_ITEM)
        {
            Sound(enemy, 4);
            DeleteConflict(ConflictObject[(short)id].model);
            deg = GetAttackDBID(enemy, enemy->motion->mid);
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - (u16)BattleDB[deg].power;
                Me_MOTION_C->life = hp;
                if (hp < 0 || (Me_MOTION_C->attribute & ATTR_ALERT) == 0)
                {
                    Me_MOTION_C->life = 0;
                }
            }
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetImpact(&p, 0x6000, 2);
            if (StagePlayer == enemy)
            {
                PadShockAR(0, 0xff, 10, 10);
            }
        }
        else
        {
            Me_MOTION_C->life = 0;
        }
        ReqLifeBar(Me_MOTION_C);
        if (Me_MOTION_C->life != 0)
        {
            motID = MOT_DAMAGE;
            motMODE = 1;
            Sound(Me_MOTION_C, 6);
            reset_alert_duration();
        }
        else
        {
            motID = MOT_DEAD;
            motMODE = 1;
            if ((Me_MOTION_C->type != NINKEN) &&
                ((StagePlayer == enemy || (enemy == (Humanoid *)CONFLICT_OWNER_ITEM))))
            {
                if ((Me_MOTION_C->attribute & (ATTR_ALERT | 0x2)) == 0)
                {
                    Criticals++;
                }
                else
                {
                    Murders++;
                }
            }
            if ((Me_MOTION_C->attribute & (ATTR_ALERT | 0x2)) != 0)
            {
                Sound(Me_MOTION_C, 8);
                reset_alert_duration();
            }
        }
        {
            short i;

            if (MotionUpdateMode != 0)
            {
                for (i = 0; i < 5; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto attack_cancel;
                    }
                }
            }
            SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = -1;
        }
    attack_cancel:
        AttackCancelControl(3);
        return;
    }
    if (motID < 0x714)
    {
        goto resolve_hit;
    }
    if (motID < 0x71a)
    {
        goto attack_break;
    }
    if (motID == 0x1009)
    {
        return;
    }
    goto resolve_hit;
/* motID 0x714..0x719 (ATTACK-family moves, per the Act table): the hit
 * cancels the move — reset ActionHalt, pick recover/idle, nudge down */
attack_break:
    ActionHalt = 0;
    if (Me_MOTION_C == StagePlayer)
    {
        SetCameraMode(CMODE_NORMAL);
    }
    if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
    {
        motID = 0x501;
        motMODE = 1;
    }
    else
    {
        motID = 0;
        motMODE = 1;
    }
    dtL->vy--;
    return;
resolve_hit:
    dtM->mask = 0x7fff;
    AttackCancelControl(3);
    {
        Humanoid *conflict;

        t = id;
        conflict = (Humanoid *)ConflictObject[t].common;
        if (conflict == (Humanoid *)CONFLICT_OWNER_ITEM)
        {
            if (Me_MOTION_C->status == STAT_DAMAGE)
            {
                return;
            }
            GetConflictResult((ModelType *)Me_MOTION_C->model, t);
            t = GetItemType((int)t);
            switch (t)
            {
            case ITEM_MAKIBISHI:
                dmg = 3;
                motID = 0x100a;
                motMODE = 1;
                break;
            case ITEM_SHURIKEN:
                if ((short)dmg == 0)
                {
                    dmg = 0x14;
                }
                if ((Me_MOTION_C->type == NINJA_0) || (Me_MOTION_C->type == NINJA_1))
                {
                    Me_MOTION_C->item[ITEM_SHURIKEN]++;
                }
                /* fall through: the shared zero-damage test preserves an existing 20 */
            case ITEM_HAPPOU:
                if ((short)dmg == 0)
                {
                    dmg = 0x1e;
                }
                /* fall through */
            case ITEM_GUN:
                if ((short)dmg == 0)
                {
                    dmg = 0x14;
                }
                /* fall through */
            case ITEM_ARROW:
                if ((short)dmg == 0)
                {
                    dmg = 10;
                }
                {
                    p.vx = dtL->vx;
                    motID = MOT_DAMAGE;
                    p.vy = dtL->vy - Me_MOTION_C->height / 2;
                    p.vz = dtL->vz;
                }
                motMODE = 1;
                SetBlood(&p, 5, 0x5a);
                break;
            case ITEM_NAPALM:
                dmg = 0x19;
                if ((rand() & 1) == 0)
                {
                    motID = 0x1003;
                }
                else
                {
                    motID = 0x1001;
                }
                motMODE = 1;
                break;
            case ITEM_FIRE:
            case ITEM_JIRAI:
            case ITEM_LIGHTNINGBOLT:
                dmg = 0x1e;
                if (t == ITEM_JIRAI)
                {
                    dmg = 0x2d;
                }
                if ((Me_MOTION_C->map.attrib & MAP_WATER) == 0)
                {
                    (Me_MOTION_C->map).height = 1;
                }
                break;
            default:
                dmg = 0;
                break;
            }
            if (0 < (Me_MOTION_C->map).height)
            {
                int ad;

                did = GetDirection((int)ConflictDistance.vx, (int)ConflictDistance.vz, dtR->vy);
                ad = (int)did;
                if (ad < 0)
                {
                    ad = -ad;
                }
                if (ad < 0x400)
                {
                    motID = 0x1005;
                    motMODE = 0;
                    dtR->vy = dtR->vy + did;
                    MoveHumanoid(Me_MOTION_C, -0x46, 0);
                }
                else
                {
                    motID = 0x1006;
                    motMODE = 0;
                    dtR->vy = (0x800 + did) + dtR->vy;
                    MoveHumanoid(Me_MOTION_C, 0x46, 0);
                }
            }
            if ((Me_MOTION_C == StagePlayer) && (ARMOUR_EQUIPPED_ != 0))
            {
                dmg = ((short)dmg * 7) / 10;
            }
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - dmg;
                Me_MOTION_C->life = (short)hp;
                if (hp <= 0)
                {
                    Me_MOTION_C->life = 0;
                    if ((u32)(u16)motID - 0x1005 > 1)
                    {
                        motID = MOT_DEAD;
                        motMODE = 1;
                    }
                    Sound(Me_MOTION_C, 8);
                    {
                        TItemType item_type;

                        item_type = GetItemType((s16)id);
                        if ((item_type < ITEM_GUN) ||
                            ((ITEM_ARROW < item_type && (item_type != ITEM_LIGHTNINGBOLT))))
                        {
                            if ((Me_MOTION_C->type & 0xf0U) == PAGE_CIVILIAN)
                            {
                                FriendHits++;
                            }
                            else if ((Me_MOTION_C->attribute & (ATTR_ALERT | 0x2)) == 0)
                            {
                                Criticals++;
                            }
                            else
                            {
                                Murders++;
                            }
                        }
                    }
                }
                else
                {
                    int r;
                    short sound_id;

                    r = rand();
                    sound_id = 7;
                    if ((r & 1) != 0)
                    {
                        sound_id = 6;
                    }
                    Sound(Me_MOTION_C, sound_id);
                }
            }
            if (StagePlayer == Me_MOTION_C)
            {
                PadShockAR(0, 0xff, 10, 0x14);
            }
            else
            {
                ReqLifeBar(Me_MOTION_C);
            }
            reset_alert_duration();
        }
        else
        {
            enemy = conflict;
            if (((Me_MOTION_C->type & 0xf0U) == PAGE_BOSS) && (enemy != StagePlayer))
            {
                return;
            }
            {
                t = 1;
                dir.vx = enemy->locate->vx - dtL->vx;
                dir.vy = enemy->locate->vy - dtL->vy;
                dir.vz = enemy->locate->vz - dtL->vz;
                while ((100 < __builtin_abs((int)dir.vx)) ||
                       (100 < __builtin_abs((int)dir.vy)) ||
                       (100 < __builtin_abs((int)dir.vz)))
                {
                    t = t << 1;
                    /* Retail uses direct arithmetic halves for all three components. */
                    dir.vx >>= 1;
                    dir.vy >>= 1;
                    dir.vz >>= 1;
                }
                p = *dtL;
                p.vy = p.vy - 1000;
                if (GetAreaMapPassage(GlobalAreaMap, &p, &dir, t) != 0)
                {
                    return;
                }
            }
            {
                int ad;

                did = GetDirection(enemy->locate->vx - dtL->vx,
                                   enemy->locate->vz - dtL->vz, dtR->vy);
                deg = GetAttackDBID(enemy, enemy->motion->mid);
                if (Me_MOTION_C != StagePlayer)
                {
                    if ((((Me_MOTION_C->status != STAT_ATTACK) &&
                          ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)) &&
                         ((Me_MOTION_C->map).height == 0)) &&
                        (gNannido != DIFFICULTY_EASY))
                    {
                        if (rand() % (EngageLevel + 1) == 0)
                        {
                            if ((Me_MOTION_C->type == NINJA_0) || (Me_MOTION_C->type == NINJA_1))
                            {
                                if ((rand() & 1) != 0)
                                {
                                    motID = 0x602;
                                }
                            }
                        }
                        else
                        {
                            motID = 0x602;
                        }
                    }
                }
                ad = did;
                if (ad < 0)
                {
                    ad = -ad;
                }
                if (ad < 700)
                {
                    if (motID == 0x602)
                    {
                        goto counter_attack;
                    }
                    if (motID == MOT_ENGAGE)
                    {
                        return;
                    }
                }
                if (motID != 0x100c)
                {
                    goto take_damage;
                }
            counter_attack:
                /* Retail's own redundancy: unreachable here with
                 * MOT_ENGAGE (both entries guard on 0x602/0x100c), yet
                 * the binary carries the duplicate test — reproduced
                 * faithfully. */
                if (motID == MOT_ENGAGE)
                {
                    return;
                }
                ret = UpdateMotion(dtM, MOT_ENGAGE);
                if (ret != 0)
                {
                    int conflict_id;
                    VECTOR *blood_pos;

                    conflict_id = (int)(*Me_MOTION_C->model->object)->id;
                    if (conflict_id >= 0)
                    {
                        dtL->vx = ConflictObject[conflict_id].position.vx;
                        dtL->vz = ConflictObject[conflict_id].position.vz;
                    }
                    mmp = dtM;
                    dtR->vy = dtR->vy + did;
                    Me_MOTION_C->status = STAT_ENGAGE;
                    mmp->count = 0;
                    PlayMotion(mmp, 1);
                    dmg = (u16)BattleDB[deg].power;
                    dtM->loop = -dmg - 8;
                    MoveHumanoid(Me_MOTION_C, -((short)((dmg * 5) / 2) + 0x50), 0);
                    if (enemy->status == STAT_ATTACK)
                    {
                        enemy->motion->loop = dmg / -3 - 1;
                        (enemy->vector).vz = 0;
                        (enemy->vector).vx = 0;
                        if (StagePlayer == enemy)
                        {
                            PadShockAR(0, 0x7f, 10, 0);
                        }
                    }
                    DeleteConflict(ConflictObject[(short)id].model);
                    blood_pos = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, (short)(dmg * 10 + 100), 0);
                    {
                        t = 0;
                        do
                        {
                            pv.vx = rand() % 100 - 0x32;
                            pv.vy = rand() % 100 - 0x32;
                            pv.vz = rand() % 100 - 0x32;
                            SetBleed(blood_pos, &pv, rand() % 0x14 + 0x14, 0xffff00);
                            t++;
                        } while (t < 10);
                    }
                    {
                        Humanoid *who;

                        if (StagePlayer == Me_MOTION_C)
                        {
                            PadShockAR(0, 0x7f, 10, 0);
                            who = enemy;
                        }
                        else
                        {
                            who = Me_MOTION_C;
                        }
                        ReqLifeBar(who);
                    }
                    {
                        s16 r;

                        r = rand() % 0x168;
                        set_impact_ex_(blood_pos, 0, 0x2000, 0x6000, 0xdcdcdc, 0, r, 6, 9, 1);
                    }
                    if ((rand() & 1) != 0)
                    {
                        Sound(Me_MOTION_C, 10);
                    }
                    if ((enemy->type & 0xf0U) != PAGE_BEAST)
                    {
                        Sound(Me_MOTION_C, 3);
                    }
                    return;
                }
            }
        take_damage:
        {
            int conflict_id;

            conflict_id = (int)(*Me_MOTION_C->model->object)->id;
            if (conflict_id >= 0)
            {
                dtL->vx = ConflictObject[conflict_id].position.vx;
                dtL->vz = ConflictObject[conflict_id].position.vz;
            }
        }
            dmg = (u16)BattleDB[deg].power;
            if (enemy != StagePlayer)
            {
                goto npc_attack;
            }
            if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
            {
                goto difficulty_bonus;
            }
            Me_MOTION_C->life = 0;
            goto recheck_attacker;
        npc_attack:
            if (Me_MOTION_C != StagePlayer)
            {
                dmg = dmg / 3;
            }
        recheck_attacker:
            if (enemy != StagePlayer)
            {
                goto apply_multipliers;
            }
        difficulty_bonus:
            dmg = dmg - ((u8)gNannido - 2);
        apply_multipliers:
            if (enemy->type == NINKEN)
            {
                dmg = (short)dmg * 6;
            }
            if (Me_MOTION_C->itmctl == ITEM_GOSIN)
            {
                dmg = dmg / 3;
            }
            if (enemy->itmctl == ITEM_GOSIN)
            {
                dmg = (u32)(dmg << 0x10) >> 0xf;
            }
            {
                if ((Me_MOTION_C == StagePlayer) && (ARMOUR_EQUIPPED_ != 0))
                {
                    dmg = ((short)dmg * 7) / 10;
                }
                deg = (short)dmg >> 3;
                if (deg > 3)
                {
                    deg = 3;
                }
                if (0 < (Me_MOTION_C->map).height)
                {
                    deg = 3;
                }
                t = (short)dmg * 5 / 2 + 0x50;
                {
                    newvy = dtR->vy + did;
                    ad = __builtin_abs(did);
                    dtR->vy = newvy;
                    if (ad < 0x400)
                    {
                        t = -t;
                    }
                    else
                    {
                        dtR->vy = newvy - 0x800;
                    }
                }
                if (deg == 3)
                {
                    MoveHumanoid(Me_MOTION_C,
                                 (0x400 < __builtin_abs(did)) ? 0x46 : -0x46, 0);
                }
                else
                {
                    MoveHumanoid(Me_MOTION_C, t, 0);
                }
            }
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - dmg;
                Me_MOTION_C->life = (short)hp;
                if (hp <= 0)
                {
                    Me_MOTION_C->life = 0;
                    DeadHumanoid = Me_MOTION_C;
                    if (deg == 3)
                    {
                        goto directional_death;
                    }
                    {
                        short i;

                        motID = MOT_DEAD;
                        motMODE = 1;
                        if (MotionUpdateMode != 0)
                        {
                            for (i = 0; i < 5; i++)
                            {
                                if (CVAhuman[i].human == Me_MOTION_C)
                                {
                                    goto death_motion_set;
                                }
                            }
                        }
                        SetNowMotion(Me_MOTION_C, motID, motMODE);
                        motMODE = -1;
                    death_motion_set:
                        if ((rand() & 1) != 0)
                        {
                            motID = 0x1101;
                            motMODE = 1;
                        }
                        goto score_kill;
                    }
                directional_death:
                {
                    int ad;

                    ad = did;
                    if (ad < 0)
                    {
                        ad = -ad;
                    }
                    if (ad > 0x400)
                    {
                        deg = deg + 4;
                    }
                    dtM->mid = -1;
                    motID = damagemotion[deg];
                }
                score_kill:
                    if (enemy == StagePlayer)
                    {
                        if ((Me_MOTION_C->type & 0xf0U) == PAGE_CIVILIAN)
                        {
                            FriendHits++;
                        }
                        else if ((Me_MOTION_C->attribute & (ATTR_ALERT | 0x2)) == 0)
                        {
                            Criticals++;
                        }
                        else
                        {
                            Murders++;
                        }
                    }
                    if ((Me_MOTION_C->attribute & (ATTR_ALERT | 0x2)) != 0)
                    {
                        Sound(Me_MOTION_C, 8);
                        goto alerted;
                    }
                }
                else
                {
                    int ad;

                    ad = did;
                    if (ad < 0)
                    {
                        ad = -ad;
                    }
                    if (ad > 0x400)
                    {
                        deg = deg + 4;
                    }
                    dtM->mid = -1;
                    motID = damagemotion[deg];
                    motMODE = 0;
                alerted:
                    reset_alert_duration();
                }
            }
            if (enemy->status == STAT_ATTACK)
            {
                enemy->motion->loop = (short)dmg / -3 - 1;
                (enemy->vector).vz = 0;
                (enemy->vector).vx = 0;
                if (StagePlayer == enemy)
                {
                    PadShockAR(0, 0xff, 10, 10);
                }
            }
            DeleteConflict(ConflictObject[(short)id].model);
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetBlood(&p, 5, 0x78);
            SetImpact(&p, 0x6000, 2);
            {
                Humanoid *who;

                if (StagePlayer == Me_MOTION_C)
                {
                    PadShockAR(0, 0x7f, 10, 0x1e);
                    who = enemy;
                }
                else
                {
                    who = Me_MOTION_C;
                }
                ReqLifeBar(who);
            }
            {
                int r;
                short sound_id;

                r = rand();
                sound_id = 7;
                if ((r & 1) != 0)
                {
                    sound_id = 6;
                }
                Sound(Me_MOTION_C, sound_id);
                r = rand();
                sound_id = 4;
                if ((r & 1) == 0)
                {
                    if (Me_MOTION_C->life == 0)
                    {
                        sound_id = 5;
                    }
                }
                Sound(enemy, sound_id);
            }
        }
    }
    if ((Me_MOTION_C->life == 0) && (Me_MOTION_C->item[ITEM_KAWARIMI] != 0))
    {
        ReqItemDefault(Me_MOTION_C, ITEM_KAWARIMI);
        Me_MOTION_C->life = Me_MOTION_C->lifemax;
        if ((u32)(u16)motID - 0x1005 > 1)
        {
            motID = 0x1002;
            motMODE = 1;
        }
    }
    (Me_MOTION_C->pad).time = 0;
    if ((dtM->mid == MOT_SWIM) || (dtM->mid == 0x302))
    {
        SVECTOR *v;

        v = dtV;
        v->vz = 0;
        v->vx = 0;
        if (Me_MOTION_C->life != 0)
        {
            motMODE = 0xffff;
            return;
        }
        motID = 0x1108;
        motMODE = 1;
    }
    {
        short i;

        if (MotionUpdateMode != 0)
        {
            for (i = 0; i < 5; i++)
            {
                if (CVAhuman[i].human == Me_MOTION_C)
                {
                    return;
                }
            }
        }
        SetNowMotion(Me_MOTION_C, motID, motMODE);
        motMODE = -1;
    }
    return;
}
