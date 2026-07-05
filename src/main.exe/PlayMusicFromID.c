#include "common.h"
#include "main.exe.h"

/*
 * STATUS: NON_MATCHING — our draft is 180 bytes (45 insns) vs target's 176
 * (44 insns): one genuine extra instruction plus the register/operand-order
 * diffs it drags along (asmdiff.py: 15 differing lines in 7 blocks — use
 * asmdiff.py here, not matchdiff.py, which overshoots its window into the
 * next not-yet-split function and reports a misleadingly huge byte count).
 *
 * PlayMusicFromID (0x8004f5dc, 0xb0 bytes) — dispatches a combined
 * voice/music id: ids below 100 are voice clips (PlayVoice(id) directly);
 * ids >= 100 are music, remapped by `id - 100` through a sentinel-terminated
 * (0xFF) remap table `MusicIDTable[]` — same linear-search-with-sentinel shape
 * as GetMotionID/GetAttackDBID, but over a u8 table instead of a struct
 * array, and folding the "keep searching" step and the sentinel pre-check
 * into two breaks per iteration instead of one loop condition.
 *
 * `p` and `flag` are read UNINITIALIZED on the "table's very first entry is
 * already the sentinel" path (m2c's arg3 is this exact tell — a register
 * read by a call with no reaching def on that path): both are only assigned
 * inside `if (MusicIDTable[0] != 0xFF)`, yet both feed the shared
 * `_PlayMusic(...)` call reached by EITHER path. This isn't a decomp
 * artifact — the target genuinely passes whatever garbage is in $a2/$a3 on
 * that path (MusicIDTable[0]==0xFF never happens in the shipped table, so
 * it's dead in practice). Do not "fix" it by hoisting the assignments out of
 * the if — confirmed necessary (removing it produces a fresh, correct
 * address, which does not match).
 *
 * Residual (root-caused, matches the cookbook's NAMED "la/address-
 * materialization reload tie" early-stop, so NOT permuted): target computes
 * %hi(MusicIDTable) directly into $a2 (`lui a2,%hi`), reuses $a2 as the
 * check's own base AND (after the branch) folds the low half into the SAME
 * register (`addiu a2,a2,%lo`) — one register for the whole address, no
 * separate temp. Our build computes %hi into $v0/a scratch, materializes the
 * full address into a FRESH register ($t0), and only THEN copies it into
 * $a2 for later use (`move a2,t0`) — an extra instruction plus a knock-on
 * addu-operand-order flip (`addu v0,a2,v0` vs target's `addu v0,v0,a2`)
 * through the rest of the loop. Tried and ruled out: declaration/statement
 * order of `p`/`flag` (no effect); goto-loop vs `for(;;)` (needed — the
 * `for(;;)` form let cc1 constant-propagate i==0 into the FIRST loop
 * iteration and peel it, an even worse divergence; the goto form matches
 * the target's un-peeled shape); reusing `flag` instead of a fresh literal
 * `0xFF` in the loop's sentinel-recheck (needed — fixed a separate stray
 * `li 0xff`); indexing the loop via the bare `MusicIDTable[i]` instead of
 * `p[i]` (worse — duplicates the address computation instead of sharing
 * it); `i[p]` vs `p[i]` spelling (no effect, unlike the struct-array
 * addu-order lever — a plain pointer variable doesn't carry the same
 * spelling sensitivity). This is a register/reload allocation choice with
 * no remaining source lever, not a structural error.
 */

/* splat's auto D_8008EA2C had drifted to 0x8008ea34 (+8 bytes, pre-existing
   accumulation drift in a still-raw data blob) — bound fresh at the correct
   address in config/symbols.main.exe.txt (see the cookbook's drifted-D_
   note). */
extern u8 MusicIDTable[];
extern void PlayVoice(s32 id);
extern void _PlayMusic(s32 id, s32 one, u8 *table, u8 flag);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PlayMusicFromID", PlayMusicFromID);
#else
void PlayMusicFromID(s32 param_1)
{
    s32 MusicNo;
    u8 *p;
    u8 flag;
    s16 i;
    s16 j;

    if (param_1 < 100)
    {
        PlayVoice(param_1);
        return;
    }
    MusicNo = param_1 - 100;
    i = 0;
    if (MusicIDTable[0] != 0xFF)
    {
        flag = 0xFF;
        p = MusicIDTable;
    search:
        j = i + 1;
        if (p[i] == MusicNo)
        {
            goto found;
        }
        i = j;
        if (p[j] != flag)
        {
            goto search;
        }
    found:
        if (p[i] != 0xFF)
        {
            MusicNo = i;
        }
    }
    _PlayMusic(MusicNo, 1, p, flag);
}
#endif
