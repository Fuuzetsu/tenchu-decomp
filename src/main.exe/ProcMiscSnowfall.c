#include "common.h"
#include "main.exe.h"
#include "misc.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscSnowfall(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:525, 53 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s2       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s4       struct TSnowfall * param
 *     reg   $s1       int i
 *     reg   $v0       int w
 *     reg   $v1       int h
 *     reg   $s0       struct SVECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/*
 * ProcMiscSnowfall (0x8004ced0, 0x26C bytes) — MISC_SNOWFALL's ProcMisc*
 * handler: MM_CREATE zeroes `mode` (sandwiched between a dead read of
 * TSnowfall.w and a self-store of TSnowfall.h — the retail body no longer
 * uses the grid dimensions the demo's PSX.SYM locals (w/h/i/pos) suggest a
 * fuller CREATE once set up; only the two reads/one self-store survive).
 * Every 4th tick (`GameClock & 3`) while armed (msg >= MM_DO), spawns
 * one snowflake: a small downward-biased jitter velocity and a position
 * randomized in a box around the camera, handed to SetSnow (the
 * EffectSlot particle pool).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Dispatch is the "short do-nothing case falls through, both real
 *    bodies are jump targets" 3-way: `if (msg==CREATE) goto do_create; if
 *    (MM_DO<=msg) goto do_tick; return;` — messages 1-3 (do-nothing) are the
 *    inline fallthrough, while CREATE and DO are both forward-jump targets,
 *    placed in that order (CREATE immediately after the dispatch, DO after
 *    CREATE) — matching neither a plain if/else-if (puts CREATE
 *    inline instead) nor if/else with the arms swapped (moves CREATE to
 *    the function's end) reproduces this; only the explicit 3-way goto
 *    ladder does.
 *  - `param = &m->param.snowfall;` is computed ONCE, unconditionally,
 *    at function entry (PSX.SYM's own register-resident `param` local) —
 *    used only by the CREATE branch's final `param->h = h;` store, but its
 *    address must be live from entry to reproduce the target's frame.
 *  - The CREATE branch's `w = ...->w;` (offset 0x18) is a genuinely DEAD
 *    read in the compiled binary — cc1 provably eliminates a plain unused
 *    local's load, so the field must be dereferenced through a
 *    `volatile`-qualified pointer to survive (both reads use `(volatile
 *    TSnowfall *)&m->param.snowfall`, matching the two plain `lw`s at
 *    0x18/0x1C).
 *  - The residual register tie (`param` landing in $v1 instead of the
 *    target's $a2) needed an extra, empirically-found lever: guarding the
 *    whole CREATE body in `if (h || GameClock) {body} else {body}` (two
 *    IDENTICAL copies — semantically a no-op since GameClock is genuinely
 *    unknowable at compile time, so cc1 can't fold the branch away and
 *    keeps both bodies' references alive going into register allocation,
 *    which is what shifts `param` onto $a2). `h` in the condition is read
 *    before its first real assignment; every well-defined substitute tried
 *    (a fresh local, `param->h`, `m->pause`, `GameClock` alone, `h = 0;`
 *    first) failed to reproduce the exact register, only reading the SAME
 *    later-assigned local worked — found by `tools/permute.py` and
 *    bisected down to this minimal shape; root mechanism (pseudo-numbering
 *    order feeding a register-allocator tie-break) not fully characterized
 *    beyond "an unresolvable branch on the reused pseudo works, nothing
 *    else tried does."
 *  - `ViewInfo.vrx - 3000 + rand() % 6000` (Ghidra's literal `A - C + B`)
 *    needed the fold-reassociation rewrite `ViewInfo.vrx + (rand() % 6000
 *    - 3000)` (cookbook Expressions) to get the target's schedule (the
 *    field load interleaved with the rand()%N divide's latency) instead of
 *    a same-length but differently-scheduled sequence.
 *  - `vel`/`pos` (the copies actually passed to SetSnow) must be
 *    declared BEFORE `jitter`/`posRaw` (the raw computed locals) for the
 *    target's stack slot assignment (call args at the lower addresses).
 */

extern void *memset(void *s, int c, u32 n);

void ProcMiscSnowfall(TMisc *m, TMiscMessage msg)
{
    TSnowfall *param = &m->param.snowfall;

    if (msg == MM_CREATE)
    {
        goto do_create;
    }
    if (MM_DO <= msg)
    {
        goto do_tick;
    }
    return;

do_create:
    {
        s32 w;
        s32 h;

        if (h || GameClock)
        {
            w = ((volatile TSnowfall *)&m->param.snowfall)->w;
            h = ((volatile TSnowfall *)&m->param.snowfall)->h;
            m->mode = 0;
            param->h = h;
        }
        else
        {
            w = ((volatile TSnowfall *)&m->param.snowfall)->w;
            h = ((volatile TSnowfall *)&m->param.snowfall)->h;
            m->mode = 0;
            param->h = h;
        }
    }
    return;

do_tick:
    if ((GameClock & 3) == 0)
    {
        SVECTOR vel;
        SVECTOR jitter;
        VECTOR pos;
        VECTOR posRaw;

        memset(&jitter, 0, sizeof(jitter));
        jitter.vx = rand() % 20 - 10;
        jitter.vy = rand() % 50 + 50;
        jitter.vz = rand() % 20 - 10;
        vel = jitter;

        memset(&posRaw, 0, sizeof(posRaw));
        posRaw.vx = ViewInfo.vrx + (rand() % 6000 - 3000);
        posRaw.vy = ViewInfo.vry + (rand() % 3000 - 6000);
        posRaw.vz = ViewInfo.vrz + (rand() % 6000 - 3000);
        pos = posRaw;
        SetSnow(&pos, &vel, 0x1000, 0);
    }
}
