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
 * handler: MM_CREATE zeroes `mode` (sandwiched between a retained read of
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
 *  - `param = &m->param.snowfall;` is computed once, unconditionally, at
 *    function entry (PSX.SYM's own register-resident `param` local). The
 *    volatile view retains retail's otherwise dead `w` read while keeping
 *    the source well-defined; `param` itself remains typed normally for the
 *    final self-store.
 *  - Binding `h` to retail's $v1 resolves the old compiler's tie between the
 *    dead volatile-read result, `h`, and `param`. This replaces the former
 *    undefined read plus duplicated branches with the CREATE operation the
 *    binary actually performs.
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
    register s32 h asm("$3");
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

        w = ((volatile TSnowfall *)&m->param.snowfall)->w;
        h = ((volatile TSnowfall *)&m->param.snowfall)->h;
        m->mode = 0;
        param->h = h;
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
