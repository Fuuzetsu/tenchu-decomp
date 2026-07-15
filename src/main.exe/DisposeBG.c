#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeBG(struct BackGround *bg);
 *     3DCTRL.C:697, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct BackGround * bg
 * END PSX.SYM */

/*
 * DisposeBG (0x8001885c) — free a tile-BG object's three dynamic buffers
 * (`cell`@0x34, `work`@0x38, `index`@0x3C) then the BackGround itself. Same
 * null-check-then-free shape as DisposeAfterimage/DisposeMotionManager, one
 * more field. Ghidra renders the first free as `vfree(&bg->cell->u)` (a
 * union-address artifact of its GsBG/GsMAP struct nesting), but the raw .s
 * is a plain `lw $a0, 0x34($s0)` — a field LOAD, not an address computation
 * — so trust m2c's flat `vfree(arg0->unk34)` reading instead. Local
 * truncated view (pad to the first freed offset, `void *` fields) rather
 * than importing Ghidra's full nested GsBG/GsMAP, unused elsewhere in the
 * codebase (same convention as AfterimageType in ReqItemHappou/ReqItemLaunch).
 */
extern void vfree(void *p);

typedef struct
{
    u8 pad0[0x34];
    void *cell;
    void *work;
    void *index;
} BackGround;

void DisposeBG(BackGround *bg)
{
    if (bg != 0)
    {
        vfree(bg->cell);
        vfree(bg->work);
        vfree(bg->index);
        vfree(bg);
    }
}
