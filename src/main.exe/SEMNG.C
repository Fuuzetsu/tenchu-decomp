#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/*
 * The demo symbols place SetupSoundEffect before SoundEx. The retail and
 * trial executables place SoundEx first; the definitions below follow that
 * order, with both orders recorded in the translation-unit manifest.
 */

extern char *STAGE_SOUND_PREFICES[N_LANGUAGES];
extern char fmt_stage_vab[]; /* %sSTAGE%d%c.VAB */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SoundEx(struct VECTOR *locate, short seid);
 *     SEMNG.C:42, 15 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * locate
 *     param $a1       short seid
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct SoundEffect *StageSE;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

short SoundEx(VECTOR *locate, short seid)
{
    enum
    {
        SOUND_AUDIBLE_RADIUS = 18000,
        SOUND_AUDIBLE_HEIGHT = 10000,
        SOUND_FULL_VOLUME_RADIUS = 2000,
        SOUND_RUN_STEP_VOLUME = SOUND_VOLUME_MAX / 2
    };
    VECTOR *pp;
    s32 dist, dx, dz;
    s32 maxvol;
    s32 dy;
    s32 angle;
    s32 vol;
    s32 raw;

    pp = StagePlayer->locate;
    if (locate == 0 || locate == pp)
    {
        return PlaySE(
            StageSE, seid,
            SOUND_SPATIAL(0, (seid == SE_RUN_STEP) ? SOUND_RUN_STEP_VOLUME : SOUND_VOLUME_MAX));
    }

    dx = locate->vx - pp->vx;
    dz = locate->vz - pp->vz;
    dist = SquareRoot0(dx * dx + dz * dz);
    if (dist >= SOUND_AUDIBLE_RADIUS)
    {
        return -1;
    }
    raw = locate->vy - pp->vy;
    dy = (raw >= 0) ? raw : -raw;
    if (dy >= SOUND_AUDIBLE_HEIGHT)
    {
        return -1;
    }
    maxvol = SOUND_VOLUME_MAX;
    if (dist < SOUND_FULL_VOLUME_RADIUS && dy < SOUND_FULL_VOLUME_RADIUS)
    {
        angle = 0;
        dist = SOUND_VOLUME_MAX;
    }
    else
    {
        dist = maxvol - (dist << SOUND_LEVEL_SHIFT) / SOUND_AUDIBLE_RADIUS;
        dist = (dist * (SOUND_AUDIBLE_HEIGHT - dy)) / SOUND_AUDIBLE_HEIGHT;
        raw = ratan2(-dx, -dz);
        angle = raw - StagePlayer->rotate->vy;
        if (CamState.Mode == CMODE_DIRECTION)
        {
            angle -= CamState.DirectionRY;
        }
        if (angle > ANGLE_HALF)
        {
            angle = ANGLE_FULL - angle;
        }
        else if (angle <= -ANGLE_HALF)
        {
            angle += ANGLE_FULL;
        }
    }
    vol = SOUND_SPATIAL(angle, dist);
    return PlaySE(StageSE, seid, vol);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupSoundEffect(short mode, short stage);
 *     SEMNG.C:26, 12 src lines, frame 144 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short mode
 *     param $a1       short stage
 *     stack sp+24     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct SoundEffect *StageSE;
 * END PSX.SYM */

void SetupSoundEffect(character_kind character, short stage)
{
    u8 name[100];

    if (StageSE != 0)
    {
        DisposeSE(StageSE);
    }
    StageSE = 0;
    if (stage >= 0)
    {
        sprintf((char *)name, fmt_stage_vab,
                STAGE_SOUND_PREFICES[CHOSEN_LANGUAGE], stage,
                character == RIKIMARU_0 ? 'R' : 'A');
        StageSE = SetupSE((u8 *)FileRead(name));
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Sound(struct Humanoid *human, short seid);
 *     SEMNG.C:61, 8 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short seid
 *     reg   $a0       struct VECTOR * locate
 *     reg   $a1       short seid
 *     reg   $s0       long volume
 *     reg   $s1       long zz
 *     reg   $s2       long xx
 *     reg   $a1       struct VECTOR * player
 *     reg   $a0       struct VECTOR * locate
 *     reg   $a1       short seid
 *     reg   $s0       long volume
 *     reg   $s1       long zz
 *     reg   $s2       long xx
 *     reg   $a1       struct VECTOR * player
 *
 * Globals it touches, as the original declared them:
 *     extern short VoiceMode;
 * END PSX.SYM */

short Sound(Humanoid *human, short seid)
{
    if (SOUND_ID_HAS_PROGRAM(seid))
    {
        return SoundEx(human->locate, seid);
    }
    if (seid > CHAR_SE_SPECIAL)
    {
        if (VoiceMode != 0)
        {
            return -1;
        }
        if ((human->attribute & ATTR_SUSPEND) != 0)
        {
            return -1;
        }
    }
    return SoundEx(human->locate,
                   (short)SOUND_ID_WITH_PROGRAM(seid, human->sound));
}
