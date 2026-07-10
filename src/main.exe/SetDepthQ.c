#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetDepthQ", SetDepthQ);

// triage: HARD — 4 insns, 2 COP2 moves — no C form (inline-asm policy), 0 callees
// likely-relevant cookbook sections:
//   - Toolchain gotchas: COP2 data moves — assemble, but have no C spelling; blocked on the same inline-asm policy as GetPad/PClseek

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetDepthQ(undefined4 param_1,undefined4 param_2)
//
// {
//   gte_ldDQA(param_1);
//   gte_ldDQB(param_2);
//   return;
// }
