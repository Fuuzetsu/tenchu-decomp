#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3callaid(void);
 *     THINK_3.C:31, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       struct Humanoid * human
 *     reg   $v1       struct Humanoid * human
 *     reg   $a0       short type
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern int StageID;
 *     extern short Degree;
 *     extern short (*Think1Func[10])();
 *     extern short (*Think2Func[5])();
 *     extern short (*Think3Func[10])();
 *     extern struct PADtype *Pad;
 *     extern short (*Think4Func[6])();
 *     extern short Attrib;
 *     extern short StageEnemies;
 *     extern short StageCitizens;
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;
/* Per-stage reinforcement pair (StageID*2 + coin flip) — the stage's
 * own guard faction (retail data): rouban/rounin, ninja A+B, rouban,
 * Manji cultists, pirates, tengu, oni, kabane, kerai, asigaru, sisi. */
extern character_kind
    AIDHumanType[N_STAGE_CONFIGS][N_STAGE_REINFORCEMENT_CHOICES];
extern int rand(void);
extern s16 Think3escape(void);

short Think3callaid(void)
{
    Humanoid *human;
    Humanoid *newhuman;
    s32 r;

    if (Distance < 16500)
    {
        if (SR != SR_GONE)
        {
            SR = SR_NONE;
        }
        return Think3escape();
    }
    else
    {
        s16 ret;
        character_kind *aid = AIDHumanType[0];
        character_kind *type_ptr;
        character_kind type;
        ThinkFunc func;

        SR = SR_UNSEEN;
        r = rand();
        /* The flat byte-offset view retains the target's index-first address
         * expression; ordinary structured indexing recolors the table base. */
        type_ptr = (character_kind *)(
            (u8 *)aid +
            ((r % N_STAGE_REINFORCEMENT_CHOICES) * sizeof(character_kind) +
             StageID * N_STAGE_REINFORCEMENT_CHOICES *
                 sizeof(character_kind)));
        type = *type_ptr;
        newhuman = BreedLife(type,
                             Me_THINK_C->locate->vx,
                             Me_THINK_C->locate->vy,
                             Me_THINK_C->locate->vz,
                             (s32)Me_THINK_C->rotate->vy + (s32)Degree);
        human = Me_THINK_C;
        newhuman->target = human->target;
        KillHumanoid(human);
        newhuman->think[0] = Think1Func[THINK1_WATCH];
        newhuman->think[1] = Think2Func[THINK2_CONTACT];
        newhuman->think[2] = Think3Func[THINK3_ATK_CHASE];
        Pad = &newhuman->pad;
        func = Think4Func[THINK4_CONTACT];
        (Me_THINK_C = newhuman)->attribute |= ATTR_CUSTOMAI;
        newhuman->think[3] = func;
        EquipWeapon(newhuman, WEAPON_DRAWN);
        SetNowMotion(Me_THINK_C, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        Attrib = Me_THINK_C->attribute | PHASE_ALERT;
        ret = 0;
        if ((Me_THINK_C->type & PAGE_MASK) == PAGE_CIVILIAN)
        {
            StageEnemies++;
            StageCitizens--;
            ret = 0;
        }
        return ret;
    }
}
