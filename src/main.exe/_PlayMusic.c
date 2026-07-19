#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void _PlayMusic(int MusicNo, int mode);
 *     IMAGES.C:438, 37 src lines, frame 264 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       int MusicNo
 *     param $s4       int mode
 *     stack sp+24     unsigned char [200] fname
 *     stack sp+224    struct CdlLOC start
 *     stack sp+232    struct CdlLOC end
 *     reg   $s2       struct TMusicTable * music
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, all 400 bytes / 100 instructions exact.
 *
 * _PlayMusic (0x8004ed54, 0x190 bytes) — dispatches a music-play request:
 * MusicNo in [0x3D, 0x63] (checked UNSIGNED, `MusicNo-0x3D < 0x27`) or
 * negative is an error (message box + stop CD audio); MusicNo < 0x13 plays
 * a CD-XA track from `MusicTable[MusicNo]`; otherwise it's a synthesized
 * "voice" cue played via `PlayVoice(MusicNo + 0x52)` (`+0x64` instead once
 * MusicNo > 99, so the id space doesn't collide past two digits).
 *
 * Splat/Ghidra split this one function into 3 pieces
 * (`_PlayMusic`/`play_stage_music__override__prt_8004ed94_...`/
 * `..._prt_8004edf4_...`) — they're contiguous addresses with plain
 * fallthrough between them (no jump table), just Ghidra mis-identifying
 * internal control-flow joins as separate functions; one real C function
 * reproduces all three.
 *
 * `MusicTable`'s real stride is 12 bytes, not PSX.SYM's stale 8-byte
 * `TMusicTable` (file@0, channel@4, min@5, sec@6) — the asm also reads
 * offsets 7/8 (the `end` CdlLOC's min/sec) and scales the index by 12
 * (`MusicNo*3<<2`), so the true struct has two more `u8` fields.
 *
 * The 3 embedded strings ("bad music no", the sprintf format, "playmusic
 * fail...") show as bare hex in the .s (no `%hi(SYMBOL)`) only because
 * splat never carved/named that unreferenced rodata — NOT because the
 * source used a literal pointer cast: writing them as literal casts
 * (`(char *)0x8001349C`) compiles the low half with `ori` (raw 32-bit
 * constant synthesis); the target's `addiu` (address-style combine)
 * needs a real named `extern char D_8001349C[];` (config/symbols.main.exe.txt
 * entries added), confirmed empirically. Contrast FUN_800568b8.c's
 * `(PersistentState *)0x80010000` cast, which really is a bare literal
 * (that lui has NO addiu at all, reused as a base for several field
 * offsets) — a different, narrower tell than "no %hi(SYMBOL) shown here".
 *
 * `MusicTable[MusicNo]`'s two `CdlLOC` locals (`start`/`end`) are declared
 * `CdlLOC[2]` (matching Ghidra's own `local_28[2]`/`local_20[2]` rendering
 * over PSX.SYM's singular `struct CdlLOC start`) and passed as the bare
 * array (decaying to `CdlLOC *`) to `CdPosToInt`/`CdIntToPos`/`CdaPlayXA` —
 * PSX.SYM's local TYPE (not count) is the "wrong in ~2 of 5" case here.
 *
 * `D_8001005A` is FUN_8004f68c.c's already-proven persisted volume byte
 * (passed to `FUN_8004fbf4` twice, identically, exactly as that file
 * does).
 *
 * The MusicNo<100-vs->=100 id offset (`+0x52`/`+0x64`) is a plain
 * default-then-override scalar (no address involved), so the literal
 * Ghidra shape (`id = MusicNo+0x52; if (MusicNo>99) id = MusicNo+0x64;`)
 * is safe here — the cookbook's default-then-override CAVEAT is
 * specifically about a shared ADDRESS pseudo stationary across a later
 * access to the same lvalue, not a plain scalar with no further reference.
 *
 * Two source identities close the former whole-function cascade. Expressing
 * the synthesized-voice arm before the XA arm reproduces retail's physical
 * body order. `InitMusicLocation` is an inlined source helper: its pointer
 * formal keeps each expanded stack-address use independent, so cc1
 * rematerializes sp+224/sp+232 instead of retaining two saved-register
 * aliases. That restores the original 264-byte frame and s0-s4 allocation.
 */
typedef struct
{
    u8 *file;    /* 0x0 */
    u8 channel;  /* 0x4 */
    u8 min;      /* 0x5 */
    u8 sec;      /* 0x6 */
    u8 endmin;   /* 0x7 */
    u8 endsec;   /* 0x8 */
    u8 pad[3];   /* 0x9 */
} TMusicTable; /* 0xC */

extern TMusicTable MusicTable[];
extern u8 D_8001005A;
extern char D_8001349C[];  /* "bad music no" */
extern char D_800134AC[];  /* "\TENCHU\XA\%s;1" */
extern char D_800134BC[];  /* "playmusic fail %s  chan %d  id %d" */

extern void AdtMessageBox(char *fmt, ...);
extern void CdaStop(void);
extern void PlayVoice(s32 id);
extern int sprintf(char *buf, char *fmt, ...);
extern void SsSetMVol(int voll, int volr);
extern void FUN_8004fbf4(u8 voll, u8 volr);
extern void *memset(void *s, int c, u32 n);
extern int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode);

static inline void InitMusicLocation(CdlLOC *location, u8 minute, u8 second)
{
    memset(location, 0, sizeof(CdlLOC));
    location->minute = minute;
    location->second = second;
    CdIntToPos(CdPosToInt(location) * 2 + 0x96, location);
}

void _PlayMusic(int MusicNo, int mode)
{
    u8 fname[200];
    CdlLOC start[2];
    CdlLOC end[2];
    TMusicTable *music;
    u8 min;
    u8 sec;
    int n;

    if (MusicNo < 0 || (u32)(MusicNo - 0x3D) < 0x27)
    {
        AdtMessageBox(D_8001349C, MusicNo);
        CdaStop();
    }
    else if (MusicNo >= 0x13)
    {
        n = MusicNo + 0x52;
        if (MusicNo > 99)
        {
            n = MusicNo + 0x64;
        }
        PlayVoice(n);
    }
    else
    {
        music = &MusicTable[MusicNo];
        sprintf((char *)fname, D_800134AC, music->file);
        SsSetMVol(0x7F, 0x7F);
        FUN_8004fbf4(D_8001005A, D_8001005A);

        min = music->min;
        sec = music->sec;
        InitMusicLocation(start, min, sec);

        min = music->endmin;
        sec = music->endsec;
        InitMusicLocation(end, min, sec);

        n = CdaPlayXA(fname, start, end, music->channel, (int)(short)mode);
        if (n == 0)
        {
            AdtMessageBox(D_800134BC, fname, music->channel, MusicNo);
        }
    }
}
