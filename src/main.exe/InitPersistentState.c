#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gNannido;
 *     extern unsigned char gSound;
 *     extern unsigned char gSoundLevel;
 *     extern unsigned char gSELevel;
 *     extern unsigned char gfMemory;
 * END PSX.SYM */

/*
 * InitPersistentState (0x80016134) — if the persistent-state blob at
 * 0x80010000 is uninitialised or corrupt (CHOSEN_CHARACTER has any bit but
 * bit0 set, or CHOSEN_STAGE is out of range >10), wipe it (memset 0xE70) and
 * seed it with defaults: magic 0x19981110 at offset 0, audio/config bytes,
 * StageNoMAX[2], the per-character shop-stock row 0 (0xFE-filled then
 * patched), a copy of that row into char 1's slot, the default item counts,
 * then pick mono/stereo and re-enter the stage-select menu. Returns 0 when it
 * (re)initialised, 1 when the existing state was already valid.
 *
 * StageNoMAX is the original demo TLinkInfo member name: both builds keep one
 * highest-stage byte per character, though retail moves the pair from +3 to
 * +0x60. control_scheme at +0x5F is retail-only and selects one of the four
 * pad-remapping table rows. The mono/stereo dispatch takes InitSoundEffect's
 * inverted `!= 0 ? Stereo : Mono` polarity (beqz into the physically-later
 * Mono block).
 *
 * STATUS: MATCHING — pure C, all 368 bytes / 92 instructions exact.
 *
 * The stock fill spells the target's two induction values directly: `i`
 * counts down while `stockp` walks down, and the fixed 0x40c displacement is
 * retained on the store. Building `stockp` as `0x80010000 | i` yields the
 * target's `lui/or` producer without letting CSE merge it with `ps`.
 *
 * `magic` and `fill` are named producer values. The empty one-shot boundary
 * after `magic` partitions setup scheduling without changing allocation
 * weight, producing `lui/ori magic; li fill; li i; lui/or stockp; lui ps`.
 * Direct `return 0` / `return 1` paths keep the result in $v0 rather than
 * introducing a function-wide carrier and an epilogue copy.
 */

extern void SsSetMono(void);
extern void SsSetStereo(void);
extern void SelectStage(TLinkInfo *ps);

s32 InitPersistentState(void)
{
    TLinkInfo *pg =
        (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    TLinkInfo *ps;
    s32 i;
    u32 magic;
    u8 fill;
    u8 *stockp;

    if ((pg->CharType & 0xfe) != 0 || 10 < pg->StageNo)
    {
        memset((void *)TENCHU_PERSISTENT_STATE_ADDRESS, 0,
               TENCHU_PERSISTENT_STATE_SIZE);
        magic = 0x19981110;

        fill = 0xfe;
        i = 0x1f;
        stockp = (u8 *)(TENCHU_PERSISTENT_STATE_ADDRESS | i);
        ps = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
        ps->magic = magic;
        ps->Nannido = 0;
        ps->Stereo = 1;
        ps->SoundLevel = 0x7f;
        ps->SELevel = 0x7f;
        ps->fMemory = 0;
        ps->Anakon = 1;
        ps->StageNoMAX[1] = 1;
        ps->StageNoMAX[0] = 1;
        do
        {
            stockp[0x40c] = fill;
            i--;
            stockp--;
        } while (i >= 0);
        ps->gItem[0] = 0xff;
        ps->gItem[1] = 6;
        ps->gItem[2] = 6;
        ps->gItem[3] = 2;
        ps->gItem[4] = 1;
        ps->gItem[5] = 1;
        ps->gItem[7] = 3;
        ps->gItem[8] = 5;
        __builtin_memcpy(&ps->gItem[0x20], &ps->gItem[0], 0x20);
        ps->selItem[1] = 10;
        ps->selItem[0] = 0xff;
        ps->selItem[2] = 5;
        ps->selItem[3] = 2;
        ps->layout = 0xff;
        if (ps->Stereo != 0)
        {
            SsSetStereo();
        }
        else
        {
            SsSetMono();
        }
        SelectStage(ps);
        ps->control_scheme = 0;
        return 0;
    }
    return 1;
}
