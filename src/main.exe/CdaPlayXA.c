#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int CdaPlayXA(unsigned char *fname, struct CdlLOC *start, struct CdlLOC *end, unsigned char channel, int mode);
 *     OPAUDIO.C:133, 39 src lines, frame 96 bytes, saved-reg mask 0x807f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s3       unsigned char * fname
 *     param $s5       struct CdlLOC * start
 *     param $s4       struct CdlLOC * end
 *     param $s6       unsigned char channel
 *     param stack+16  int mode
 *     stack sp+16     struct CdlFILE cf
 *     stack sp+40     struct CdlFILTER filter
 *     stack sp+48     unsigned char [4] param
 *     stack sp+56     struct CdlLOC loc
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

/*
 * CdaPlayXA (0x8004f99c, 0x164 bytes) — starts CD-XA audio playback of
 * `fname`, if CD-audio playback isn't already active: stops it (CdaStop,
 * idempotent), searches for the file (CdSearchFile), and on a hit computes
 * StartPos/EndPos in CD sectors (StartPos = file's own position + 0x96
 * sectors of read-ahead; EndPos defaults to StartPos + the file's own
 * length in sectors (size>>11) unless an explicit `end` CdlLOC is given;
 * `start` further offsets StartPos when given), arms the drive (cd_control
 * mode-set 0xe, VSync(3) to let it settle), resets the status/check-count
 * bookkeeping, sets the play filter (file=1, chan=channel) via cd_control
 * 0xd, and installs the vsync-driven pump callback (cbCheckCD). Returns 1
 * on success, 0 if already playing or the file wasn't found. Same proven
 * TCdaStatus struct as CdaStop.c/CdaReady.c/CdaGetCurrentLength.c.
 *
 * Matching notes:
 *  - The two nested `if`s Ghidra shows (`if (flag&1) { CdaStop(); if
 *    (CdSearchFile(...)) {...long body...} }`, falling off the end to
 *    `return 0;`) put "return 1" textually FIRST (nested) and the shared
 *    "return 0" LAST — the opposite of what the asm wants. The asm has
 *    "return 1" fall straight into the epilogue (no jump) and BOTH failure
 *    points jump forward to a single shared "return 0" block. Respelling as
 *    two early-exit guard clauses (`if (flag&1)==0) return 0;` /
 *    `if (CdSearchFile(...)==0) return 0;`) with the long success body
 *    UNNESTED and LAST fixes this (cookbook's guard-clause-with-two-returns
 *    family, but inverted from the usual "short error nested" shape since
 *    here the LONG body is what needs to sit last/unnested).
 *  - `if (end != 0) {CdPosToInt block} else {size>>0xb block}` — Ghidra's
 *    literal polarity (`if (end==0) {size path} else {CdPosToInt path}`)
 *    lays the SHORT arm out first; the target puts the LONGER (CdPosToInt)
 *    arm first/fallthrough instead (cookbook's "flip back when the success
 *    arm is the longer one", generalized to "whichever arm is longer sits
 *    fallthrough-first" even outside a success/error framing).
 *  - `param`/`filter` are two independent small stack buffers passed to the
 *    two cd_control calls (0xe then 0xd) — not one shared buffer.
 *
 * STATUS: NON_MATCHING — length ours 88 insns vs target 89 (one missing
 * instruction), 39 differing lines, but EVERY instruction from the first
 * CdPosToInt call onward (0x8004fa18-0x8004fad8) sits at the SAME ADDRESS
 * as the target with only registers renamed — the control flow above is
 * fully correct. The one missing instruction is the `mode` parameter's own
 * load: target loads it VERY early (`lw $s0,0x68($sp)`, scheduled right
 * after — and filling the load-delay slot of — the `CdaStatus.flag` byte
 * load, BEFORE the `flag&1` guard even branches), keeping it alive in its
 * own callee-saved register ($s0) across every intervening call. Our
 * compile instead sinks the load to its single real use
 * (`CdaStatus.mode = mode;`, deep inside), needing one fewer callee-saved
 * register and shifting every later register down by one — 39 of the 39
 * differing lines are exactly this renumbering (s1..s6 in target vs s0..s5
 * here) plus the corresponding shifted addresses.
 *
 * Ruled out: the cookbook's own fix for this exact class ("A stack
 * parameter used exactly once is load-sunk to its use... A local copy `s32
 * va = ry;` defeats it", AddMisc.c) does NOT work here — verified via a
 * direct `cc1 -dg` RTL dump. AddMisc's va/vb/vc each have TWO further
 * downstream uses; `mode` has exactly one (`CdaStatus.mode = mode;`). With
 * only one downstream use, an earlier copy-propagation/combine pass folds
 * `m = mode; ...; CdaStatus.mode = m;` back into a single reference at the
 * ORIGINAL use site before update_equiv_regs ever sees the copy (confirmed
 * in the `-dg` dump: the load lands at the later instruction regardless of
 * where `m = mode;` is placed — function entry, right after the first
 * guard, anywhere). This is a new, narrower member of the "stack parameter
 * sink" family: the defeat-the-sink trick needs >=2 real downstream uses to
 * survive combine; a genuinely single-use stack parameter has no available
 * source lever. Not permuter-eligible per the cookbook's iteration protocol
 * (permuter only cracks SAME-LENGTH residuals; this is one instruction
 * short).
 */
typedef struct CdlLOC CdlLOC;
typedef struct CdlFILE CdlFILE;

struct CdlLOC
{
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
};

struct CdlFILE
{
    CdlLOC pos;
    u32 size;
    s8 name[16];
};

struct CdlFILTER
{
    u8 file;
    u8 chan;
};

typedef struct
{
    s32 StartPos;   /* 0x00 */
    s32 CurPos;     /* 0x04 */
    s32 EndPos;     /* 0x08 */
    s16 mode;       /* 0x0C */
    s16 CheckCount; /* 0x0E */
    u8 status;      /* 0x10 */
    u8 voll;        /* 0x11 */
    u8 volr;        /* 0x12 */
    u8 flag;        /* 0x13 */
    u8 field9_0x14; /* 0x14 */
} TCdaStatus;

extern TCdaStatus CdaStatus;
extern void CdaStop(void);
extern CdlFILE *CdSearchFile(CdlFILE *loc, char *name);
extern s32 CdPosToInt(CdlLOC *pos);
extern void cd_control(u8 cmd, u8 *param, u8 *result);
extern void VSync(s32 mode);
extern void VSyncCallback(void (*func)(void));
extern void cbCheckCD(void);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CdaPlayXA", CdaPlayXA);
#else
int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode)
{
    CdlFILE cf;
    struct CdlFILTER filter;
    u8 param[4];
    s32 iVar2;

    if ((CdaStatus.flag & 1) == 0) {
        return 0;
    }
    CdaStop();
    if (CdSearchFile(&cf, (char *)fname) == 0) {
        return 0;
    }
    CdaStatus.mode = mode;
    iVar2 = CdPosToInt(&cf.pos);
    CdaStatus.StartPos = iVar2 + 0x96;
    if (end != 0) {
        iVar2 = CdPosToInt(end);
        CdaStatus.EndPos = CdaStatus.StartPos + iVar2;
    } else {
        CdaStatus.EndPos = CdaStatus.StartPos + (cf.size >> 0xb);
    }
    if (start != 0) {
        iVar2 = CdPosToInt(start);
        CdaStatus.StartPos = CdaStatus.StartPos + iVar2;
    }
    param[0] = 0xc9;
    cd_control(0xe, param, 0);
    VSync(3);
    CdaStatus.field9_0x14 = 0x1b;
    CdaStatus.CheckCount = 0;
    CdaStatus.status = 0;
    filter.file = 1;
    filter.chan = channel;
    cd_control(0xd, (u8 *)&filter, 0);
    VSyncCallback(cbCheckCD);
    return 1;
}
#endif
