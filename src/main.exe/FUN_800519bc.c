#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 *     extern struct TCdaStatus CdaStatus;
 *     extern short SkipFrame;
 * END PSX.SYM */

/* STATUS: NON_MATCHING — clean pure-C reconstruction at the exact target
 * length (1448 bytes / 362 instructions), currently 11 differing linked bytes
 * across 9 instructions. There are no allocator-only no-op loop fences: the
 * normal fade update and guarded strip loop recover the target frame naturally.
 * Writing the first PathFileRead before all state initializers is the key natural
 * live-range split: cc1 hoists those independent writes around the call into the
 * target's exact prologue order. Only renderer register identities remain.
 *
 * HISTORICAL NOTES BELOW ARE SUPERSEDED and retained only as an experiment log.
 * KEY FINDING: the `do{sequence=0;}while(0)` fence was NOT load-bearing — it walled
 * off the basic block sched2 needs in order to interleave the callee-saved register
 * spills with the init writes; removing it (plain `sequence = 0;`) let the saves
 * interleave as retail does and closed 25 bytes on its own. Moving `fade_step = -8`
 * to just after the first PathFileRead lets it fill that call's branch delay slot.
 * Both position narrowings are now explicit in-place shifts (`position <<= 16;
 * position >>= 16;` and `position = counter << 16; position >>= 16;`) so the
 * sign-extend is `sll s0,s0,16; sra s0,s0,16` in place of a v0 scratch.
 * Retained: snapshotting old_pad before its write (delay slot), the distinct
 * full-word tpage capture (lw/nop/sra), capturing the call result/xbase before
 * their stores. LOAD-BEARING (confirmed by regression on removal): the outer strip
 * `do{}while(0)` (length mismatch without it) and the nested position-carrier
 * fences (regress to 100).
 * REMAINING (all greg register/schedule ties, no source lever found; rejected:
 * single-expr scroll=128, tpage/xbase var-merge, `3*x`):
 * (1) prologue — the prefix/s2 setup schedules after the sequence/fade spills
 * instead of first, and the two init constants land in v0 where retail reuses t0
 * (xbase materialized 0xff60/ori vs retail -160/addiu); (2) four caller-saved ties
 * where retail reuses t0 for load temps (tpage_word->a2, xbase_value->v1,
 * scroll-adjusted->a2) and v1 for the brightness `0xa0-position` intermediate (->v0)
 * — rtlguide hard-conflicts a2->v0 (p89/p96/p197), v0->v1 (p82); (3) position=counter's
 * in-place sign-extend is scheduled two insns before the GetTPage(0,0) arg moves.
 * Fuzzy: 94.48% (up from 90.88%).
 * ROUND 2 (fresh, skeptical re-verification of the above — all CONFIRMED, not
 * refuted): tools/autorules.py unguided (73 candidates) and --guided off a fresh
 * rtlguide run (160 candidates, budget-capped beam) both report no improving
 * edit; tools/permute.py (-j4, 16457 iterations, stop-on-zero) plateaus exactly
 * at 87 with zero candidates beating base.c. Four NEW hypotheses tested and
 * REJECTED (each rebuilt+remeasured, not just reasoned):
 * - `xbase` as `s16` (to get retail's `addiu -160` instead of `ori 0xff60` per
 *   the addiu/ori-names-the-type rule): regresses 87->1464. Root cause read off
 *   the objdump: cc1's CSE now recognises the signed -160 constant as the SAME
 *   value as the later `sprite.x = -0xa0;` (also genuinely -160 in target) and
 *   merges them into ONE persistent value, promoted to a callee-saved register
 *   (`li s3,-160` + an extra spill) to survive the 6 intervening calls — even
 *   though TARGET keeps these two materialisations separate (t0 early, v0 late,
 *   never merged) despite being the identical constant. Whatever makes retail
 *   avoid that merge is not reproduced by simply signing the constant.
 * - Same constant via a two-step signed local (`s16 xbase_init = -0xa0; stack.xbase
 *   = xbase_init;`, field left `u16`): identical regression to 1464, same root
 *   cause (the local still carries -160 as a signed SImode constant that CSE
 *   coalesces with sprite.x's).
 * - Replacing every `PSTATE->stage`/`PSTATE->language` in this function with the
 *   named externs `CHOSEN_STAGE`/`CHOSEN_LANGUAGE` (used elsewhere in this file
 *   for CHOSEN_CHARACTER/STAGE_LAYOUT_NUMBER, and pervasively in StageEndScreen/
 *   mission_score_screen): regresses 87->1488. cc1 cannot fold two INDEPENDENT
 *   extern symbol_refs' `%hi` together, so every use re-materialises its own
 *   lui+lbu instead of sharing one base register. CAUTION for future readers:
 *   the target .s file's own `%hi(CHOSEN_STAGE)` labels are NOT evidence of the
 *   source spelling — they are splat's disassembler matching the final computed
 *   address against the symbol table, and our baseline's `PSTATE->stage` access
 *   already produces BYTE-IDENTICAL bytes for that instruction (confirmed: it
 *   was never part of any diffed cluster). Do not re-attempt this lever without
 *   new evidence.
 * - Merging `adjusted = stack.scroll; adjusted += D_8008ECF8[lang][stage];` into
 *   one statement `adjusted = stack.scroll + D_8008ECF8[lang][stage];`: regresses
 *   87->97, same length, but DOES prove the two-statement split is a live,
 *   non-neutral lever for that hunk (load order changed) — just backwards from
 *   what's needed here. The reverse pairing (index loads merged some other way)
 *   was not tried; a future round could probe adjacent orderings of this hunk
 *   specifically (target: PSTATE->language/PSTATE->stage lbus BEFORE the
 *   `stack.scroll` reload, i.e. index computed before the reload it's added to).
 * rtlguide's "register goals" section (re-run fresh) shows the a2<->v0 and
 * v0<->v1 swaps are genuine HARD-CONFLICTs (cannot be reached by priority/weight
 * alone), but the four ->t0 goals (v0->t0 x4, a2->t0 x3, v1->t0 x2) carry NO
 * hard-conflict annotation — t0 is simply never tried by find_reg's numeric
 * search because a lower-numbered register is always free first. No source lever
 * was found this round to make t0 the winner; a future attempt should look for
 * what makes v0/v1/a2 UNAVAILABLE at those points in target (extend some other
 * value's live range to occupy them), not try to prefer t0 directly.
 * ROUND 3 (the 5 do{}while(0) fences audited one at a time — which are real,
 * which are ours): a macro search across every header AND every .c file's own
 * local #define in src/main.exe found NO function-like macro anywhere whose
 * body matches any of the 5 — the only do{}while(0)-shaped macros in this TU
 * family are DRAW_SCORE_NUMBER/DRAW_SCORE_COLON (mission_score_screen.c,
 * StageEndScreen.c), unrelated in shape and purpose. So none of the 5 are
 * macro hygiene by the letter of that test. Measured anyway, per-fence,
 * BOTH by hand (rebuilt+remeasured, one fence unwrapped at a time and in
 * combination) AND by `tools/autorules.py`'s independent `fence-unwrap` rule
 * (both agree exactly):
 *   - outer strip `do{ if(counter>=0){...} }while(0)` (L290): unwrapping it
 *     alone scores 87->1012 (autorules) — a de facto length mismatch, same
 *     conclusion as round 1's "length mismatch without it", now with an exact
 *     number. It wraps a REAL conditional block, not a bare statement — a
 *     plausible genuine "skip the rest of this case without goto" idiom. KEEP.
 *   - the three position-carrier wrappers around `position = signed_width -
 *     position;` (L307/L309/L311, nested 3 deep): unwrapping ANY ONE of them
 *     — even the innermost alone, even with the other two still wrapping it —
 *     scores 87->91, and unwrapping any subset (one, two, or all three) never
 *     scores worse OR better than 91: it is ONE dial, not three. Depth 3
 *     keeps tpage_value in s2 and signed_width in s1 (matching target);
 *     dropping below depth 3 flips the pair to s1/s2 swapped — a genuine
 *     local-alloc QTY_CMP_PRI tie, not a scheduling-order tie (the flat and
 *     fenced forms emit the IDENTICAL instruction sequence, only two register
 *     assignments swap).
 *   - `do{ sprite.tpage = tpage_result; }while(0)` (L318): unwrapped alone,
 *     87->93 — the single costliest fence, autorules-confirmed. Unwrapped
 *     TOGETHER with the other three (full flatten to 5 plain statements),
 *     the total is still only 91, i.e. its individual cost is absorbed once
 *     the other three are also gone (not additive — same 91 floor either way).
 *   CORRECTION to round 1: "the nested position-carrier fences (regress to
 *   100)" was measured against an earlier intermediate state of this file and
 *   is stale; the correct, freshly-measured number against the CURRENT source
 *   is 91 (any subset of the three) or 93 (tpage alone), never 100.
 *   A genuinely different, non-fence, non-flatten restructuring was also
 *   tried: folding the whole computation into one expression close to
 *   Ghidra's natural rendering (`xbase_value = stack.xbase; position =
 *   xbase_value - (signed_width - position) * 8; sprite.tpage =
 *   tpage_result;`) — this is WORSE (105), refuting the hypothesis that the
 *   5-statement decomposition through a reused `position` carrier is itself
 *   the byte-chased part; that decomposition is closer to target than the
 *   more "natural" single-expression alternative.
 *   PRECEDENT (same TU family): StageEndScreen.c has the identical pattern —
 *   `best_x = 0x80010000;` wrapped in FOUR nested do{}while(0) (its L498-510)
 *   with the comment "Preserve the target allocator weight for this reused
 *   identity", and its own STATUS header states verbatim: "The four nested
 *   do{}while(0) ... are LOAD-BEARING, measured: autorules' fence-unwrap
 *   scores each at 202->227. They are not scaffolding; do not remove them."
 *   That is this project's own established standard for exactly this
 *   situation: once a genuine macro search comes up empty, MEASURED
 *   NECESSITY — not textual macro provenance — decides whether a fence
 *   stays. This is NOT the SetWire / mission_score_screen situation (a worse
 *   byte count adopted because a genuinely BETTER, more complete alternative
 *   structure was found): here every alternative tried (flatten in every
 *   combination, single-expression fold) was strictly WORSE with no
 *   compensating gain in plausibility — there is no suppressed better
 *   structure to prefer, only an unresolved register-coloring tie the fences
 *   currently paper over. VERDICT: keep all 5 fences.
 *   Re-confirmed fresh: `tools/autorules.py` unguided (73 candidates,
 *   including all 5 fence-unwrap candidates individually) finds no improving
 *   edit; a bounded fresh permuter run (240s search + rescore, base score
 *   1095) never beats its own base score, consistent with round 2's
 *   16457-iteration run. No orphaned permuter state left behind.
 * ROUND 4 (re-verify with the -fno-builtin permute.py CC_FLAGS fix + hunt a
 * STRUCTURAL lever, per an owner directive that an 87B residual "hides a
 * missing/mis-shaped block"). RESULT: the structural premise is FALSIFIED and
 * 87 is re-confirmed as the hard floor.
 *   - CFG IS IDENTICAL: matchdiff/asmdiff show 362 vs 362 insns, no missing or
 *     extra branch, all 15 diff blocks are register renames + 2 local schedule
 *     swaps (prologue prefix-order, GetTPage(0,0) arg-move vs position sign-ext).
 *     There is no missing/mis-shaped block to find — the residual is genuinely
 *     sub-C register allocation (t0 numeric-order, the a2->v0/v0->v1 hard-conflicts).
 *   - FRESH permuter WITH the -fno-builtin fix (the reason this round exists):
 *     19444 iterations, -j4, --stop-on-zero. Authoritative full-link rescore:
 *     base.c 87 is best; every retained candidate rescores >=87 (output-860 has a
 *     BETTER proxy score 860 but rescores to 91). The fix changed nothing; the
 *     round-2/3 plateau claim stands.
 *   - autorules re-run (73 candidates) — no improving edit (unchanged).
 *   - NEW structural hypotheses tested & REJECTED (each rebuilt+remeasured):
 *     * index-first scroll (`adjusted = D_8008ECF8[..]; adjusted += stack.scroll;`
 *       — round 2's own "untried" suggestion): 87->102. The scroll-first split is
 *       optimal; the reverse pairing is worse, not better.
 *     * tpage_value as a direct `stack.tpage_base >> 16` OR a plain two-statement
 *       capture (dropping the identical-arm `if (strip_width!=0)`): LENGTH
 *       MISMATCH 1440 (-2 insns). That `if` is LOAD-BEARING: it is a control-flow
 *       merge that blocks cse's store->load forwarding of the prologue
 *       `stack.tpage_base = strip_px<<16;`, forcing the target's fresh lw/sra
 *       reload instead of reusing strip_px. Not scaffolding — it reconstructs a
 *       real target reload (cookbook "a redundant reload = a fresh dereference").
 *     * shared brightness intermediate (one `bright_base` var feeding both
 *       `(position+0xa0)*3` and `(0xa0-position)*3`): LENGTH MISMATCH 1436;
 *       cc1 merges the arms. Target keeps them separate (branch1 intermediate on
 *       v1, branch2 on v0 — the residual v0/v1 tie, a HARD-CONFLICT per rtlguide).
 *     * capturing the first PathFileRead 2nd arg into a local before the inits
 *       (to occupy v0 across the const materialisations, per round 2's hint):
 *       87->127; it hoists the whole index calc and reshuffles the prologue.
 *     * reorder `signed_width` birth before tpage + flatten the position-carriers
 *       (test whether birth-order gives target's s1/s2 without the fences):
 *       LENGTH MISMATCH 1452. Birth-order does NOT substitute for the loop_depth
 *       ref-weighting the nested fences provide.
 *   - FENCE VERDICT (owner asked to challenge them): the position-carrier nested
 *     do{}while(0) is the SAME matched technique as the matched sibling
 *     BriefingAndInventorySelectionScreen (lever #3: "NESTED do{}while(0) (two
 *     deep)... each level adds +1 loop_depth to flow.c's ref weighting, flipping
 *     the {v0,v1} pairing"; lever #5: do{}while(0) carrier fences). That sibling
 *     matched at 0 WITH these. So the fences ARE "the human structure the matched
 *     sibling shows", not invented scaffolding — KEPT. `siblingdiff --demo` found
 *     no PSX.EXE sibling, but ROUND 5 found the renderer in demo/GAME.EXE.
 * ROUND 5 (compiler output first, with the earlier cookbook conclusions treated
 * as hypotheses): demo/GAME.EXE has a copy of this strip renderer at
 * 0x800431e0..0x80043344. Its data flow is ordinary human C: `u = counter * 4`,
 * `GetTPage(0, 0, (s16)tpage + counter, 0x100)`, an x position derived from
 * `(s16)width - counter`, and edge brightness `(160 - x) * 3`. Literal ports of
 * those expressions do not reproduce this build's allocation: the direct call
 * expression scores 83 bytes and the single x expression changes the length.
 * The pinned compiler's sched2 dump explains the four-byte call-order residual:
 * the sign-extension and a0/a1 argument setup are equal-ready UID choices; the
 * spelling that fixes their order also shortens the counter lifetime and loses
 * the target's s0 position carrier after the call. A split right-edge brightness
 * expression produces retail's v1/v0 order exactly, but jump2 cross-jumps the
 * now-identical multiply tails (as `rtx_renumbered_equal_p` permits after reload)
 * and shortens the function to 1436 bytes. A whole padded working struct, moving
 * the first read, moving xbase across the call, narrow/volatile locals, and
 * direct human constant spellings were all measured neutral or worse. The one
 * coherent improvement retained is a named right-edge brightness intermediate;
 * it keeps the exact CFG and length and improves 77 -> 76 bytes.
 * ROUND 6: the target's duplicated left/right multiply tails survive jump2 only
 * when the source has distinct colour-store tails. Keeping one tail for the left
 * edge and one shared by the right edge/centre lets jump2 merge exactly the three
 * stores without cross-jumping the arithmetic. This is also ordinary per-arm C,
 * not a case-label fence: the resulting brightness region is byte-exact, global
 * allocation remains unchanged, and the whole-function residual improves 76 ->
 * 71 bytes at the exact target length. */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_800519bc", FUN_800519bc);
#else

typedef struct BackGround BackGround;

typedef struct
{
    u16 xbase;
    u8 pad_02[6];
    s32 scroll;
    u16 old_pad;
    u8 pad_0e[2];
    BackGround *background;
    s32 tpage_base;
} DemoScreenStack;

typedef struct
{
    char *background;
    char *foreground;
    u16 music;
    u16 pad;
} DemoScreenAssets;

typedef struct
{
    s32 StartPos;
    s32 CurPos;
    s32 EndPos;
    s16 mode;
    s16 CheckCount;
    u8 status;
    u8 voll;
    u8 volr;
    u8 flag;
    u8 command;
} TCdaStatus;

#define PSTATE ((PersistentState *)0x80010000)

extern u8 CHOSEN_CHARACTER;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 D_80010048;
extern char D_800137A0[];
extern DemoScreenAssets D_8008EA90[][11];
extern s16 D_8008ECA0[][11];
extern s16 D_8008ECF8[][11];
extern GsOT *OTablePt;
extern TCdaStatus CdaStatus;
extern s16 SkipFrame;

extern BackGround *FUN_8004f4f8(u_long *tim);
extern void vfree(void *ptr);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void FUN_80038ce0(void);
extern u16 GetRealPad(s32 port);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, u8 b);
extern void FUN_8004f6c0(s32 arg0);
extern void StartDrawing(void);
extern void _PlayMusic(s32 music, s32 mode);
extern s32 CdaGetCurrentLength(void);
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 depth);
extern void DrawBG(BackGround *bg);
extern void FUN_80038c0c(u8 *ot, s32 r, s32 g, s32 b);
extern void EndDrawing(s16 sync);
extern void DisposeBG(BackGround *bg);

static inline void TimToDemoSprite(u_long *file, GsIMAGE *image,
                                   GsSPRITE *sprite)
{
    GetTIMInfo(file, image);
    InitSprite(image, sprite);
}

void FUN_800519bc(void)
{
    GsIMAGE strip_image;
    GsSPRITE sprite;
    GsIMAGE image;
    u16 xbase;
    s32 scroll;
    u16 old_pad;
    BackGround *background;
    s32 tpage_base;
    u_long *file;
    u16 pad;
    u16 previous_pad;
    s16 fade;
    s16 counter;
    s32 page_x;
    s16 sequence;
    s32 fade_step;
    s32 adjusted;
    s32 scroll_value;
    s16 strip_width;
    u16 strip_px;
    u16 tpage_result;
    s32 intensity;
    u8 brightness;
    s32 edge_brightness;
    s32 scaled_brightness;
    s32 tpage_word;
    s32 tpage_value;
    s16 signed_width;
    s16 i;

    file = PathFileRead((u8 *)D_800137A0,
                        D_8008EA90[PSTATE->language][PSTATE->stage].background);
    sequence = 0;
    fade = 0xfe;
    scroll = -0xa000;
    old_pad = 0;
    xbase = (u16)(scroll >> 8);
    fade_step = -8;
    background = FUN_8004f4f8(file);
    vfree(file);

    file = PathFileRead((u8 *)D_800137A0,
                        D_8008EA90[PSTATE->language][PSTATE->stage].foreground);
    TimToDemoSprite(file, &image, &sprite);
    sprite.x = -0xa0;
    sprite.y = -0x78;
    sprite.r = 0x80;
    sprite.g = 0x80;
    sprite.b = 0x80;
    sprite.attribute |= 0x50000000;
    sprite.mx = sprite.w >> 1;
    sprite.my = sprite.h >> 1;
    sprite.mx = 0;
    sprite.my = 0;
    GetTIMInfo(file, &strip_image);
    LoadTIMAndFree(file);
    strip_width = strip_image.pw;
    strip_px = (u16)strip_image.px;
    sprite.w = 0x10;
    sprite.y = -0x68;
    FUN_80038ce0();
    tpage_base = (s32)strip_px << 16;

    while (1)
    {
        previous_pad = old_pad;
        pad = GetRealPad(0);
        old_pad = pad;
        if ((pad & (pad ^ previous_pad) & 0x820) != 0)
        {
            fade_step = 8;
        }

        if ((pad & 0x900) == 0x900)
        {
            for (i = 0; i < 0x14; i++)
            {
                PSTATE->stock[i + CHOSEN_CHARACTER * 0x20] =
                    PSTATE->backup[i];
            }
            FadeOutDirect(0x20, 2, 8, 8, 8);
            FUN_80038ce0();
            STAGE_LAYOUT_NUMBER = 0xff;
            D_80010048 &= 0xfe;
            FUN_8004f6c0(0x10);
        }

        page_x = fade;
        if (page_x >= 0xff)
        {
            break;
        }

        StartDrawing();
        switch (sequence)
        {
        case 0:
            if (page_x == 0)
            {
                s16 music;

                music = D_8008EA90[PSTATE->language][PSTATE->stage].music;
                if (PSTATE->chr == 1 && PSTATE->language == 3 &&
                    (u32)(PSTATE->stage - 6) < 2)
                {
                    music++;
                }
                _PlayMusic(music, 0);
                sequence = 1;
            }
            break;

        case 1:
            if (CdaGetCurrentLength() > 0)
            {
                sequence = 2;
                counter = 0;
            }
            break;

        case 2:
            if (D_8008ECA0[PSTATE->language][PSTATE->stage] < counter++)
            {
                sequence = 3;
            }
            break;

        case 3:
            counter = strip_width - 4;
            if (counter >= 0)
            {
                tpage_word = tpage_base;
                if (strip_width == 0)
                    tpage_value = tpage_word >> 16;
                else
                    tpage_value = tpage_word >> 16;
                signed_width = strip_width;
                do
                {
                        sprite.u = counter << 2;
                        tpage_result = GetTPage(0, 0,
                            tpage_value + (s16)counter, 0x100);
                        page_x = signed_width - (s16)counter;
                        page_x <<= 3;
                        sprite.tpage = tpage_result;
                        page_x = xbase - page_x;
                        sprite.x = page_x;
                        page_x <<= 16;
                        page_x >>= 16;
                        if (page_x < -0xa0)
                        {
                            goto brightness_zero;
                        }
                        if (page_x >= -0x78)
                        {
                            goto brightness_normal;
                        }
                        brightness = (page_x + 0xa0) * 3;
                        goto brightness_left_store;

brightness_normal:
                        if (page_x <= 0xa0)
                        {
                            goto brightness_within;
                        }

brightness_zero:
                        sprite.r = 0;
                        sprite.g = 0;
                        sprite.b = 0;
                        goto brightness_done;

brightness_within:
                        if (page_x < 0x79)
                        {
                            goto brightness_center;
                        }
                        scaled_brightness = edge_brightness = 0xa0 - page_x;
                        scaled_brightness <<= 1;
                        scaled_brightness += edge_brightness;
                        brightness = scaled_brightness;
                        goto brightness_right_store;

brightness_center:
                        brightness = 0x80;
                        goto brightness_right_store;

brightness_left_store:
                        sprite.r = brightness;
                        sprite.g = brightness;
                        sprite.b = brightness;
                        goto brightness_done;

brightness_right_store:
                        sprite.r = brightness;
                        sprite.g = brightness;
                        sprite.b = brightness;
                        goto brightness_done;
brightness_done:
                        sprite.x -= 8;
                        GsSortSprite(&sprite, OTablePt, 1);
                        counter -= 4;
                        sprite.x += 8;
                } while (counter >= 0);
            }

            if ((CdaStatus.status & 0x60) == 0)
            {
                fade_step = 8;
            }
            scroll_value = scroll;
            adjusted = scroll_value +
                D_8008ECF8[PSTATE->language][PSTATE->stage];
            scroll = adjusted;
            if (adjusted < 0)
            {
                adjusted += 0xff;
            }
            xbase = (u32)adjusted >> 8;
            break;
        }

        DrawBG(background);
        fade += fade_step;
        if (fade < 0)
            fade = 0;
        else if (fade > 0xff)
            fade = 0xff;
        intensity = fade & 0xff;
        if (fade != 0)
        {
            FUN_80038c0c(*(u8 **)((u8 *)OTablePt + 4),
                          intensity, intensity, intensity);
        }
        SkipFrame = 2;
        EndDrawing(0);
    }

    DisposeBG(background);
}

#endif

// triage: HARD — 362 insns, 2 loop, 19 callees, ~0.11 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_800519bc(void)
//
// {
//   char cVar1;
//   ushort uVar2;
//   ulong *puVar3;
//   short sVar4;
//   int iVar5;
//   uint uVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int unaff_s4;
//   uint uVar11;
//   uint uVar12;
//   int iVar13;
//   GsIMAGE GStack_a8;
//   GsSPRITE local_88;
//   GsIMAGE GStack_60;
//   ushort local_40;
//   int local_38;
//   ushort local_34;
//   BackGround *local_30;
//   int local_2c;
//
//   uVar12 = 0;
//   uVar11 = 0xfe;
//   local_38 = -0xa000;
//   local_34 = 0;
//   local_40 = 0xff60;
//   iVar13 = -8;
//   puVar3 = PathFileRead((uchar *)"K:\\WORK\\CDIMAGE\\DEMO\\",
//                         (&PTR_s_st001_tim_8008ea90)
//                         [(uint)(byte)PersistentState._5_1_ * 3 +
//                          (uint)(byte)PersistentState._94_1_ * 0x21]);
//   local_30 = (BackGround *)FUN_8004f4f8(puVar3);
//   vfree((undefined *)puVar3);
//   puVar3 = PathFileRead((uchar *)"K:\\WORK\\CDIMAGE\\DEMO\\",
//                         (&PTR_s_mojo1_tim_8008ea94)
//                         [(uint)(byte)PersistentState._5_1_ * 3 +
//                          (uint)(byte)PersistentState._94_1_ * 0x21]);
//   GetTIMInfo(puVar3,&GStack_60);
//   InitSprite(&GStack_60,&local_88);
//   local_88.x = -0xa0;
//   local_88.y = -0x78;
//   local_88.r = 0x80;
//   local_88.g = 0x80;
//   local_88.b = 0x80;
//   local_88.attribute = local_88.attribute | 0x50000000;
//   local_88.mx = 0;
//   local_88.my = 0;
//   GetTIMInfo(puVar3,&GStack_a8);
//   LoadTIMAndFree(puVar3);
//   local_88.w = 0x10;
//   local_88.y = -0x68;
//   FUN_80038ce0();
//   local_2c = (uint)(ushort)GStack_a8.px << 0x10;
//   do {
//     uVar2 = GetRealPad(0);
//     if ((uVar2 & (uVar2 ^ local_34) & 0x820) != 0) {
//       iVar13 = 8;
//     }
//     local_34 = uVar2;
//     if ((uVar2 & 0x900) == 0x900) {
//       iVar5 = 0;
//       do {
//         sVar4 = (short)iVar5;
//         iVar5 = iVar5 + 1;
//         (&PersistentState.field_0x40c)[(int)sVar4 + (uint)(byte)PersistentState._4_1_ * 0x20] =
//              (&PersistentState.field_0x27)[sVar4];
//       } while (iVar5 * 0x10000 >> 0x10 < 0x14);
//       FadeOutDirect(0x20,2,'\b','\b');
//       FUN_80038ce0();
//       PersistentState._6_1_ = 0xff;
//       PersistentState._72_1_ = PersistentState._72_1_ & 0xfe;
//       FUN_8004f6c0(0x10);
//     }
//     if (0xfe < (short)uVar11) {
//       DisposeBG(local_30);
//       return;
//     }
//     StartDrawing();
//     if (uVar12 == 1) {
//       iVar5 = CdaGetCurrentLength();
//       if (0 < iVar5) {
//         uVar12 = 2;
//         unaff_s4 = 0;
//       }
//     }
//     else if (uVar12 < 2) {
//       if ((uVar12 == 0) && ((short)uVar11 == 0)) {
//         sVar4 = *(short *)(&DAT_8008ea98 +
//                           (uint)(byte)PersistentState._94_1_ * 0x84 +
//                           (uint)(byte)PersistentState._5_1_ * 0xc);
//         if ((PersistentState._4_1_ == '\x01') &&
//            (((byte)PersistentState._94_1_ == 3 && ((byte)PersistentState._5_1_ - 6 < 2)))) {
//           sVar4 = sVar4 + 1;
//         }
//         _PlayMusic((int)sVar4,0);
//         uVar12 = 1;
//       }
//     }
//     else if (uVar12 == 2) {
//       sVar4 = (short)unaff_s4;
//       unaff_s4 = unaff_s4 + 1;
//       if (*(short *)(&DAT_8008eca0 +
//                     ((uint)(byte)PersistentState._94_1_ * 0xb + (uint)(byte)PersistentState._5_1_) *
//                     2) < sVar4) {
//         uVar12 = 3;
//       }
//     }
//     else {
//       iVar5 = GStack_a8.pw - 4;
//       if (uVar12 == 3) {
//         if (-1 < iVar5 * 0x10000) {
//           iVar10 = local_2c >> 0x10;
//           do {
//             local_88.u = (uchar)(iVar5 << 2);
//             local_88.tpage = GetTPage(0,0,iVar10 + (short)iVar5,0x100);
//             iVar7 = (uint)local_40 + ((int)(short)GStack_a8.pw - (int)(short)iVar5) * -8;
//             iVar8 = iVar7 * 0x10000;
//             iVar9 = iVar8 >> 0x10;
//             if (iVar9 < -0xa0) {
// LAB_80051dd4:
//               local_88.b = '\0';
//             }
//             else {
//               cVar1 = (char)((uint)iVar8 >> 0x10);
//               if (iVar9 < -0x78) {
//                 local_88.b = (cVar1 + -0x60) * '\x03';
//               }
//               else {
//                 if (0xa0 < iVar9) goto LAB_80051dd4;
//                 if (iVar9 < 0x79) {
//                   local_88.b = 0x80;
//                 }
//                 else {
//                   local_88.b = (-0x60 - cVar1) * '\x03';
//                 }
//               }
//             }
//             local_88.x = (short)iVar7 + -8;
//             local_88.r = local_88.b;
//             local_88.g = local_88.b;
//             GsSortSprite(&local_88,OTablePt,1);
//             iVar5 = iVar5 + -4;
//             local_88.x = local_88.x + 8;
//           } while (-1 < iVar5 * 0x10000);
//         }
//         if ((CdaStatus.status & 0x60) == 0) {
//           iVar13 = 8;
//         }
//         local_38 = local_38 +
//                    u_TXjhSXE__8008ecf8
//                    [(uint)(byte)PersistentState._94_1_ * 0xb + (uint)(byte)PersistentState._5_1_];
//         iVar10 = local_38;
//         if (local_38 < 0) {
//           iVar10 = local_38 + 0xff;
//         }
//         local_40 = (ushort)((uint)iVar10 >> 8);
//         unaff_s4 = iVar5;
//       }
//     }
//     DrawBG(local_30);
//     uVar11 = uVar11 + iVar13;
//     iVar5 = (int)(uVar11 * 0x10000) >> 0x10;
//     if (iVar5 < 0) {
//       uVar11 = 0;
//     }
//     else if (0xff < iVar5) {
//       uVar11 = 0xff;
//     }
//     uVar6 = uVar11 & 0xff;
//     if ((uVar11 & 0xffff) != 0) {
//       FUN_80038c0c(OTablePt->org,uVar6,uVar6,uVar6);
//     }
//     SkipFrame = 2;
//     EndDrawing(0);
//   } while( true );
// }
