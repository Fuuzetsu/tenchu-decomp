#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short FallCheck(void);
 *     MOTION.C:256, 31 src lines, frame 40 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+24     struct SVECTOR vect
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern short RefrectMove[16][2];
 *     extern struct VECTOR *dtL;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

/*
 * FallCheck (0x8001cc90, 0x220 bytes) — transition a sufficiently high
 * falling humanoid into motion 0x803, apply the facing-dependent landing
 * displacement, and cancel the current attack.
 *
 * Matching notes:
 *  - The zero-attribute path is the positive wrapper around the height and
 *    status switch. This keeps the attribute guard as a direct branch to the
 *    epilogue and leaves the jump table's shared return-zero island between
 *    case 7 and the default body.
 *  - The default and looping case 7 jump to `fall`, after the common
 *    return-zero statement. That source layout emits the target's physical
 *    case order: case 7, return-zero, default.
 *  - Comparing CVAhuman[] against the global Me_MOTION_C (rather than the
 *    cached `human`) preserves the target's CSE copy in the scan prologue.
 */

extern Humanoid *Me_MOTION_C;

extern void AttackCancelControl(short mode);

short FallCheck(void)
{
    Humanoid *human;
    VECTOR *locate;
    short i;

    if (motID == 0x803)
    {
        return 1;
    }
    if (motID != 0x70f)
    {
        if (Me_MOTION_C->status == 9 && Me_MOTION_C->map.height > 0)
        {
            return 1;
        }
        if (((u16)Me_MOTION_C->attribute & 0x20) == 0)
        {
            if (Me_MOTION_C->map.height < 0x3e9)
            {
                return 0;
            }
            switch ((short)((u16)Me_MOTION_C->status - 4))
            {
            case 7:
                if (dtM->loop != 0)
                {
                    goto fall;
                }
            case 0:
            case 6:
            case 9:
            case 12:
            case 13:
                break;
            default:
                goto fall;
            }
        }
    }
    return 0;

fall:
    dtM->mask = 0x7fff;
    human = Me_MOTION_C;
    locate = dtL;
    locate->vx += (human->width * RefrectMove[human->map.angleH][0]) >> 2;
    locate->vz += (human->width * RefrectMove[human->map.angleH][1]) >> 2;
    motMODE = 0;
    motID = 0x803;
    if (MotionUpdateMode != 0)
    {
        i = 0;
        do
        {
            if (CVAhuman[i].human == Me_MOTION_C)
            {
                goto found;
            }
            i = i + 1;
        } while (i < 5);
    }
    SetNowMotion(Me_MOTION_C, motID, motMODE);
    motMODE = -1;
found:
    if (Me_MOTION_C->status == 0xb)
    {
        dtM->count >>= 2;
    }
    AttackCancelControl(3);
    return -1;
}
