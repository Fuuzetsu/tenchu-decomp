#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * FileRead(unsigned char *filename);
 *     FILEIO.C:156, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $s0       unsigned char * filename
 *     reg   $s0       unsigned long * ret
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 *     extern int ReadMode;
 *     extern int TotalIO;
 * END PSX.SYM */

/*
 * FileRead (0x80019010, 0xE4 bytes) — the FILEIO.C dispatcher: re-arms the
 * memory-card access vsync callback (same PrepareAccess-style guard, but
 * inlined here after CdaStop() rather than a separate call), lazily
 * initializes the PC-link reader on first use (ReadMode == -1), then
 * dispatches on ReadMode & 3 to one of the three backends (0 = DEVPC,
 * 1 = MEMORY, 2 = CDROM; 3 has no backend and returns NULL) before running
 * the access-indicator draw (FUN_80018f00) and returning the loaded buffer.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - No named local exists in PSX.SYM for the dispatch value (only
 *    `filename`/`ret`) — write `switch (ReadMode & 3)` directly with no
 *    named temp; cc1's own dispatch pseudo never gets a source name. The
 *    asm's `slti $v0,$v1,2` (a SIGNED compare) over a single load of
 *    `ReadMode` confirms a real switch, not an if/else-if ladder (which
 *    would CSE the load and compare unsigned).
 *  - Case bodies sit in memory in ascending value order (0 DEVPC, 1 MEMORY,
 *    2 CDROM, then the shared default/join) — write the switch that way.
 *  - `filename` lives in $s0 across the CdaStop() call (the entry parameter
 *    survives to the dispatch calls); after its last use (the call whose
 *    case wins) $s0 is reused for `ret` — ordinary reg reuse, not a coding
 *    choice.
 *  - The `AccessPower`/`cbAccess` guard is the SAME idiom as PrepareAccess.c,
 *    including its opposite-polarity lever: the asm's `bltz` branches to the
 *    `f = 0` body with `AccessPower = 0; VSyncCallback(cbAccess);` as the
 *    FALLTHROUGH, so the source condition is `AccessPower >= 0`, the negation
 *    of Ghidra's `AccessPower < 0` rendering.
 *  - Unlike PrepareAccess.c (which never matched this guard — see its own
 *    header), this one DOES: `VSyncCallback` is called directly in each arm
 *    instead of through a shared `void (*f)(void)` local. With a shared `f`,
 *    `f` is a pseudo LIVE ACROSS the if/else join (block-crossing), so
 *    cc1's `local-alloc.c` `combine_regs` refuses to tie the `%hi(cbAccess)`
 *    temp to it (`reg_qty[sreg] == -1` for any pseudo not confined to one
 *    block — see cookbook's RTL-dump section) and the temp free-falls to the
 *    first free class register, $v0. Call `VSyncCallback` directly in each
 *    arm instead: its argument pseudo is now LOCAL to that one block, so
 *    `combine_regs` DOES tie the `%hi` temp to it (both die/are-reused at
 *    the same point), and that tied qty inherits the copy-suggestion of $a0
 *    from the `a0 = <arg>` call setup — landing the whole `lui`/`addiu` pair
 *    in $a0, matching the target exactly. `jump.c` cross-jump-merges the two
 *    calls' identical tail back into the single `jal VSyncCallback` the
 *    target has (same idiom as the cookbook's `Sound` two-literal-`return
 *    SoundEx(...)`-calls entry). MATCH (228/228 bytes).
 */

extern void CdaStop(void);
extern void cbAccess(void);
extern void VSyncCallback(void (*f)(void));
extern void PCinit(void);
extern u_long *LoadFromMEMORY(u8 *filename);
extern u_long *LoadFromDEVPC(u8 *filename);
extern u_long *LoadFromCDROM(u8 *filename);
extern void FUN_80018f00(void);

u_long *FileRead(u8 *filename)
{
    u_long *ret;

    CdaStop();
    if (AccessPower >= 0) {
        AccessPower = 0;
        VSyncCallback(cbAccess);
    } else {
        VSyncCallback(0);
    }
    if (ReadMode == -1) {
        TotalIO = 0;
        ReadMode = 0;
        PCinit();
    }
    switch (ReadMode & 3) {
    case 0:
        ret = LoadFromDEVPC(filename);
        break;
    case 1:
        ret = LoadFromMEMORY(filename);
        break;
    case 2:
        ret = LoadFromCDROM(filename);
        break;
    default:
        ret = 0;
        break;
    }
    FUN_80018f00();
    return ret;
}
