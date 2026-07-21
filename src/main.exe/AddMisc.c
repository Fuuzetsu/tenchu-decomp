#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddMisc(enum MiscType type, int x, int y, int z, int a, int b, int c);
 *     MISC.C:636, 47 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $t0       enum MiscType type
 *     param $a1       int x
 *     param $a2       int y
 *     param $a3       int z
 *     param stack+16  int a
 *     param stack+20  int b
 *     param stack+24  int c
 *     reg   $a0       int a
 *     reg   $t1       int b
 *     reg   $t2       int c
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

/*
 * AddMisc (0x8004d13c) — the misc-object/effect spawner (fire, doors,
 * pitfalls, snowfall, sprites, water TIM swaps...). Scans the misc[] pool for
 * a free slot (proc == 0), fills in position + the three init params, then
 * dispatches on `type` to install the processor and kick it once with
 * MM_CREATE (0). type 5 doesn't allocate anything: it loads one of seven
 * water/warp TIMs (name picked by x) and uploads it via FUN_80032720.
 *
 * First jump-table switch in a scan loop; the table links from this TU's
 * .rodata at the original 0x800127E8 (splat carve [0x1FE8, .rodata, AddMisc],
 * same mechanism as BriefingAndInventorySelectionScreen).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The pool scan is a hand-rolled GOTO loop, not do/while: with loop notes,
 *    loop.c hoists the jump-table la, type<<2 and &tbl[x] to the preheader and
 *    builds a p+0x14 induction variable (none of which the target has). The
 *    goto loop keeps them all in place; the bound is re-anchored each
 *    iteration by cse as one addiu off the register that la'd misc.
 *  - `base` holds that la: declared FIRST with its initializer, and
 *    `p = base;` written as a STATEMENT so the tp/ptm/va/vb/vc initializers
 *    sit between the la and the copy — cse2 has a (set REG0 REG1) special
 *    case that would otherwise rewrite the ADJACENT pair to move the la into
 *    p and flip the copy direction.
 *  - tp/ptm are pointer variables initialized at declaration: their addius
 *    are the two prologue frame addresses (t1/s1); the calls then pass plain
 *    registers (no per-call addiu rematerialization, unlike &tm spelled at
 *    each use).
 *  - va/vb/vc are copies of the three STACK parameters. A stack parm used
 *    once (REG_N_REFS == 2) has its entry load sunk to the use by
 *    local-alloc's update_equiv_regs (REG_EQUIV machinery) when no loop notes
 *    cover the use; copying to a local makes update_equiv_regs substitute the
 *    parm INTO the copy insn, so the lw itself lands at the copy's position
 *    (entry, decl order = the original's load order) and the copy's pseudo —
 *    having no REG_EQUIV note — must get a hard register. Raw parms also
 *    lose priority races (REG_LIVE_LENGTH is doubled for equiv-noted regs).
 *  - The do{}while(0) around stores+switch+call+pause is the regalloc lever
 *    (depth-2 refs) that produces the target's caller-saved assignment
 *    [base a0, va a2, vb a3, vc t0, tp t1]; the switch's sltiu bound check
 *    floats to the top of the block only because it is INSIDE the note range
 *    (loop notes are scheduler barriers), and the wrapper must extend through
 *    p->pause = 1 or jump.c moves case 0's then-arm (a jump out of the note
 *    range) to the end of the function. The nested double wrapper at the
 *    loop bottom weights base's bound ref so base outranks va for a0.
 *  - Case 5's name table is copied from an anonymous-initializer-style data
 *    blob kept as an extern (TimNameBlock cast, like BIS's HelpPathBlock):
 *    the original's u8*[7]={"Water1.tim",...} would emit the strings + table
 *    into THIS object and can't reproduce the interleaved original .data.
 *    tp->n[x] (struct-member spelling) gives the base+index addu; plain
 *    tp[x] on a char** emits index+base.
 *  - The loop bound compares SIGNED ((s32) casts): the target uses slt, and
 *    Ghidra renders the condition as (int)ptVar1 < -0x7ff3da88.
 */

typedef struct { char *n[7]; } TimNameBlock; /* 0x1C */
extern char *D_80012788[]; /* the seven water/warp TIM names */
extern char D_800127A4[];  /* "K:\\WORK\\CDIMAGE\\IMAGE\\" */
extern char D_800127BC[];  /* "undefined effect %d" */

extern void ProcMiscFire(tag_TMisc *m, s32 msg);
extern void FUN_8004d6d4(tag_TMisc *m, s32 msg);
extern void ProcMiscDoor(tag_TMisc *m, s32 msg);
extern void ProcMiscPitfall(tag_TMisc *m, s32 msg);
extern void ProcMiscSnowfall(tag_TMisc *m, s32 msg);
extern void ProcMiscSprite(tag_TMisc *m, s32 msg);
extern void FUN_8004c350(tag_TMisc *m, s32 msg);
extern void FUN_8004c59c(tag_TMisc *m, s32 msg);
extern void FUN_80032720(GsIMAGE *im, short y, short z);
extern void AdtMessageBox(char *fmt, ...);

void AddMisc(s32 type, s32 x, s32 y, s32 z, s32 a, s32 b, s32 c)
{
    tag_TMisc *base = misc;
    tag_TMisc *p;
    char *tbl[7];
    GsIMAGE tm;
    TimNameBlock *tp = (TimNameBlock *)tbl;
    GsIMAGE *ptm = &tm;
    s32 va = a;
    s32 vb = b;
    s32 vc = c;
    u_long *adr;

    p = base;
loop:
    if (p->proc == 0)
    {
        do
        {
            p->mode = 0;
            p->x = x;
            p->y = y;
            p->z = z;
            p->param.init.a = va;
            p->param.init.b = vb;
            p->param.init.c = vc;
            switch (type)
            {
            case 0:
                if (va == 0)
                    p->proc = ProcMiscFire;
                else
                    p->proc = FUN_8004d6d4;
                break;
            case 1:
                p->proc = ProcMiscDoor;
                break;
            case 2:
                p->proc = ProcMiscPitfall;
                break;
            case 3:
                p->proc = ProcMiscSnowfall;
                break;
            case 4:
                p->proc = ProcMiscSprite;
                break;
            case 5:
                *(TimNameBlock *)tbl = *(TimNameBlock *)D_80012788;
                adr = PathFileRead(D_800127A4, tp->n[x]);
                GetTIMInfo(adr, ptm);
                LoadTIMAndFree(adr);
                FUN_80032720(ptm, y, z);
                return;
            case 6:
                p->proc = FUN_8004c350;
                break;
            case 7:
                p->proc = FUN_8004c59c;
                break;
            default:
                AdtMessageBox(D_800127BC, type);
                return;
            }
            p->proc(p, 0);
            p->pause = 1;
        } while (0);
        return;
    }
    do
    {
        do
        {
            p++;
            if ((s32)p < (s32)(base + 200))
                goto loop;
        } while (0);
    } while (0);
}
