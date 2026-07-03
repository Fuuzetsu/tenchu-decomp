#include "common.h"
#include "main.exe.h"

/*
 * Split, not yet matched. A near-exact decomp (whole structure + FRAMES-absolute
 * + type-truncation solved; only 2 instruction-scheduling diffs remain) is saved
 * in the scratchpad as ProcItemManebue.nearmatch.c. Matching it required gotos +
 * a variable-compare to force a `mode` reload our cc1 (decompals GCC 2.8.1 rebuild)
 * doesn't emit naturally — likely a rebuild-vs-real-CC1PSX.EXE difference, pending
 * a decomp.me psyq-preset check. See the session notes / docs/toolchain.md.
 */
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemManebue", ProcItemManebue);
