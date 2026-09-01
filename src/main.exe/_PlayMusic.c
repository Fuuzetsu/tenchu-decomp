#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void _PlayMusic(int MusicNo, int mode);
 *     IMAGES.C:438, 37 src lines, frame 264 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int MusicNo
 *     param $s4       int mode
 *     stack sp+24     unsigned char [200] fname
 *     stack sp+224    struct CdlLOC start
 *     stack sp+232    struct CdlLOC end
 *     reg   $s2       struct TMusicTable * music
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gSoundLevel;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, all 400 bytes / 100 instructions exact.
 *
 * _PlayMusic (0x8004ed54, 0x190 bytes) — dispatches a music-play request:
 * MusicNo in the reserved cue range (checked UNSIGNED) or negative is an
 * error (message box + stop CD audio); MusicNo below MUSIC_TRACK_COUNT plays
 * a CD-XA track from `MusicTable[MusicNo]`; otherwise it's a synthesized
 * "voice" cue translated into one of PlayVoice's numbered banks.
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
 * needs a real named `extern char msg_bad_music_no[];` (config/symbols.main.exe.txt
 * entries added), confirmed empirically. Contrast clamp_shop_stock_.c's
 * `(TLinkInfo *)0x80010000` cast, which really is a bare literal
 * (that lui has NO addiu at all, reused as a base for several field
 * offsets) — a different, narrower tell than "no %hi(SYMBOL) shown here".
 *
 * `MusicTable[MusicNo]`'s two `CdlLOC` locals use PSX.SYM's original
 * singular `start`/`end` declarations. Ghidra rendered each as a two-element
 * array only because the following stack object begins one `CdlLOC` later.
 *
 * `gSoundLevel` is apply_cd_volume_.c's already-proven persisted volume byte
 * (passed to `set_cda_volume_` twice, identically, exactly as that file
 * does).
 *
 * The two voice-bank offsets can be passed as a conditional expression
 * directly to PlayVoice. Likewise, testing CdaPlayXA's return value directly
 * preserves retail's branch. Neither result needs the generic `n` carrier
 * from the first reconstruction, matching PSX.SYM's local inventory as well
 * as the retail instructions.
 *
 * Two source identities close the former whole-function cascade. Expressing
 * the synthesized-voice arm before the XA arm reproduces retail's physical
 * body order. `InitMusicLocation` is an inlined source helper: its pointer
 * formal keeps each expanded stack-address use independent, so cc1
 * rematerializes sp+224/sp+232 instead of retaining two saved-register
 * aliases. That restores the original 264-byte frame and s0-s4 allocation.
 */
/* Retail extends PSX.SYM's TMusicTable with an XA end time. */
typedef struct TMusicTable
{
    u8 *file;   /* 0x0 */
    u8 channel; /* 0x4 */
    u8 min;     /* 0x5 */
    u8 sec;     /* 0x6 */
    u8 endmin;  /* 0x7 */
    u8 endsec;  /* 0x8 */
} TMusicTable;  /* 0xC */

extern TMusicTable MusicTable[MUSIC_TRACK_COUNT];
extern char msg_bad_music_no[];           /* "bad music no" */
extern char fmt_xa_path[];                /* "\TENCHU\XA\%s;1" */
extern char fmt_playmusic_fail_chan_id[]; /* "playmusic fail %s  chan %d  id %d" */

extern void AdtMessageBox(char *fmt, ...);
extern void CdaStop(void);
extern void PlayVoice(s32 id);
extern int sprintf(char *buf, char *fmt, ...);
extern void SsSetMVol(int voll, int volr);
extern void set_cda_volume_(u8 voll, u8 volr);
extern void *memset(void *s, int c, u32 n);
extern int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode);

static inline void InitMusicLocation(CdlLOC *location, u8 minute, u8 second)
{
    memset(location, 0, sizeof(CdlLOC));
    location->minute = minute;
    location->second = second;
    CdIntToPos(CdPosToInt(location) * 2 + OFFSET, location);
}

void _PlayMusic(int MusicNo, int mode)
{
    u8 fname[200];
    CdlLOC start;
    CdlLOC end;
    TMusicTable *music;
    u8 min;
    u8 sec;

    if (MusicNo < 0 ||
        (u32)(MusicNo - MUSIC_CUE_RESERVED_FIRST) <
            MUSIC_CUE_RESERVED_COUNT)
    {
        AdtMessageBox(msg_bad_music_no, MusicNo);
        CdaStop();
    }
    else if (MusicNo >= MUSIC_TRACK_COUNT)
    {
        PlayVoice(MusicNo +
                  (MusicNo >= MUSIC_CUE_EXTENDED_FIRST
                       ? MUSIC_EXTENDED_VOICE_ID_OFFSET
                       : MUSIC_VOICE_ID_OFFSET));
    }
    else
    {
        music = &MusicTable[MusicNo];
        sprintf((char *)fname, fmt_xa_path, music->file);
        SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
        set_cda_volume_(gSoundLevel, gSoundLevel);

        min = music->min;
        sec = music->sec;
        InitMusicLocation(&start, min, sec);

        min = music->endmin;
        sec = music->endsec;
        InitMusicLocation(&end, min, sec);

        if (CdaPlayXA(fname, &start, &end, music->channel, (s16)mode) == 0)
        {
            AdtMessageBox(fmt_playmusic_fail_chan_id, fname, music->channel, MusicNo);
        }
    }
}
