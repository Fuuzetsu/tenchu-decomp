#include "common.h"
#include "main.exe.h"

/*
 * HANDWRITTEN: PClseek (0x80060224, 36 bytes) is the PsyQ host-file lseek
 * stub; the guarded reference reconstruction below is byte-exact.
 * It shifts fd, offset, and mode from $a0-$a2 to the handler's private
 * $a1-$a3 convention, executes host-link trap 0x107, and returns the $v1
 * result or -1 when $v0 reports an error. The demo body is also identical.
 *
 * This is proven handwritten assembly, not a C allocator near miss:
 *  - trap 0x107 is unique in the image and is not one of cc1's break patterns;
 *  - shifting all arguments up one register has no C calling-convention cause;
 *  - the trap returns independent status and result values in $v0 and $v1;
 *  - the original-object label LSEEK_OBJ_1C survives in the symbol table.
 * There are no PSX.SYM C translation-unit or line records. Under
 * docs/gte-policy.md the canonical asm may be counted done once the owner adds
 * it to config/handwritten-asm.txt; this file does not make that policy change.
 * It is SDK code above the 0x80060000 game-code boundary in either case.
 *
 * Reconstruction constraints:
 *  - maspsx source must use the one-value spelling "break 0x107"; the
 *    disassembler's "break 0, 263" spelling is not accepted.
 *  - The trap, status branch, $v1-to-$v0 copy in the branch delay slot, and
 *    error -1 are one handwritten unit. cc1 supplies only the final return.
 *  - Pure C cannot express the trap ABI. Even with pinned-register inline asm,
 *    a shared result coalesces with $v1 and moves to $v0 only at the epilogue.
 *    Two direct returns instead produce independent materializations but add
 *    an unwanted jump (40 bytes). Neither spelling can put the success copy
 *    in the target branch's delay slot while retaining the 36-byte layout.
 *  - The default build therefore keeps the exact two-piece INCLUDE_ASM body.
 *    LSEEK_OBJ_1C is PClseek's epilogue; it was split only because the original
 *    object label survived.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PClseek", PClseek);
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PClseek", LSEEK_OBJ_1C);
#else
int PClseek(int fd, int offset, int mode)
{
    register int r_a3 __asm__("$7") = mode;
    register int r_a2 __asm__("$6") = offset;
    register int r_a1 __asm__("$5") = fd;
    register int r_v0 __asm__("$2");
    register int r_v1 __asm__("$3");

    __asm__ volatile("break 0x107\n\t"
                     "beqz %0, 0f\n\t"
                     "addu %0, %1, $zero\n\t"
                     "addiu %0, $zero, -1\n\t"
                     "0:"
                     : "=r"(r_v0), "=r"(r_v1)
                     : "r"(r_a1), "r"(r_a2), "r"(r_a3));

    return r_v0;
}
#endif
