#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * AfsOpen (0x8005ef34, 0x9c bytes) — finds `path` in the AFS volume's file
 * table (AfsFindFile) and, on a hit, looks for a free TAFSFileHandle
 * (`flagUse==0`), fills it in (info/pos/flagUse), and returns it. Reports via
 * AdtMessageBox and returns NULL when the file isn't found or the scan is
 * exhausted. Same proven TAFS/TAFSElement layout as AfsRead.c/AfsInit.c/
 * AfsClose.c/AfsFileSize.c. Real return type is `TAFSFileHandle *`
 * (established by LoadFromCDROM.c's callers), not Ghidra's `TAFSElement *`.
 *
 * Matching notes:
 *  - AfsInit allocates and clears 0x3C bytes, exactly five of PSX.SYM's
 *    0xC-byte TAFSFileHandles. Retail AfsOpen nevertheless advances by TWO
 *    handles while counting five iterations: it probes entries 0, 2, 4,
 *    then addresses beyond that allocation. `cur += 2` preserves this
 *    apparent retail skip/bounds bug without inventing a 0x18-byte struct.
 *  - The scan is a bottom-tested `do/while` with the counter incremented as
 *    the loop body's FIRST statement (matches Ghidra's literal order here;
 *    the asm's `addiu $v1,$v1,1` sits in the found-check branch's delay
 *    slot, i.e. unconditional every iteration, same shape either way it's
 *    read).
 *  - Flipping the guard to `if (entry == 0) {short} else {long}` (rather
 *    than the literal `if (entry != 0) {long}`) was needed to match the
 *    error-message address materialization order and the branch polarity —
 *    the long success arm belongs unnested/fallthrough here.
 *  - Each arm calls `AdtMessageBox` directly with its own literal format
 *    string (`AdtMessageBox(D_80014960, path);` / `AdtMessageBox(D_80014978,
 *    path);`), NOT funnelled through a shared `char *fmt;` set in each arm
 *    and called once after the merge. The funnelled form makes `fmt` a
 *    pseudo that crosses the if/else join, so its per-arm `%hi` half is a
 *    BLOCK-LOCAL temp that local-alloc resolves before global-alloc gives
 *    `fmt` its call-argument ($a0) preference — `combine_regs` then refuses
 *    to tie the two, and the `%hi` half defaults to $v0 instead of $a0
 *    (cookbook: "the %hi reload tie is combine_regs refusing to tie a
 *    block-crossing pseudo"). Two direct per-arm calls make each `%hi`/`%lo`
 *    pair block-local from the start, inheriting the call's $a0 preference
 *    immediately; cross-jump then re-merges the identical `jal
 *    AdtMessageBox`+`return 0` tail into the ONE shared copy the target has
 *    (byte-identical, no length change). This same fix also flips `handle`/
 *    `path`'s coloring to the target's $s1/$s0 (removing the `fmt` pseudo
 *    changes the two parameters' relative global-alloc priority ordering).
 *  - IMPORTANT: do NOT convert the `if (entry==0) {...} else {...}`
 *    wrapper into an early-return + fallthrough shape (`if (entry==0) {
 *    msg; return 0; } cur = ...; do {...} while(...); msg2; return 0;`).
 *    That reshapes the CFG so the loop's early-exit block (`cur->flagUse==
 *    0`) gets placed AFTER the loop-exhausted tail instead of before it,
 *    which puts the second message's whole address computation inside
 *    loop.c's scanned NOTE_INSN_LOOP_BEG..LOOP_END range (its
 *    `-da`/`.loop` dump shows `Insn N: regno R (life 1), savings 1 moved`)
 *    and hoists the `%hi` half into the preheader — one instruction
 *    SHORTER than the target, a length mismatch. Keep the if/else nesting;
 *    only duplicate the call inside each arm.
 *
 * STATUS: MATCH.
 */
extern TAFSElement *AfsFindFile(TAFS *handle, char *path, s32 mode);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80014960[]; /* "AfsOpen: %s not found\n" */
extern char D_80014978[]; /* "AfsOpen: no more handle\n[%s]" */

TAFSFileHandle *AfsOpen(TAFS *handle, char *path)
{
    TAFSElement *entry;
    TAFSFileHandle *cur;
    u32 count;

    entry = AfsFindFile(handle, path, AfsFlag_File);
    count = 0;
    if (entry == 0) {
        AdtMessageBox(D_80014960, path);
    } else {
        cur = handle->pHandle;
        do {
            count = count + 1;
            if (cur->flagUse == 0) {
                cur->info = entry;
                cur->pos = 0;
                cur->flagUse = 1;
                return cur;
            }
            cur += 2;
        } while (count < 5);
        AdtMessageBox(D_80014978, path);
    }
    return 0;
}
