#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "images.h"
#include "item.h"
#include "tim.h"
#include "vmemory.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libgpu.h>

/*
 * Retail IMAGES.C was extensively reordered after the demo and gained six
 * archive, background, volume, and process helpers. The manifest retains the
 * earlier source-line order independently.
 */

extern u8 CHOSEN_LANGUAGE;

/* Retail retains VoiceXaName and adds one filename pointer per localization. */
extern u8 *VoiceXaName;
extern u8 *VoiceXaNameF;
extern u8 *VoiceXaNameI;
extern u8 *VoiceXaNameJ;
extern TVoiceTable *EventVoiceTables[N_LANGUAGES];

/* The two non-localized banks and their literal INTRO.XA/TORA.XA paths. */
extern TVoiceTable IntroVoiceTable[];
extern TVoiceTable ToraVoiceTable[];
extern u8 *IntroVoiceXaName;
extern u8 *ToraVoiceXaName;

/* Fallback (language/range-independent) voice table. */
extern TVoiceTable CommonVoiceTable[];

/* PSX.SYM names IMAGES.C's original archive pointer ArcData. */
extern ArcFile *ArcData;
/* Qualified because INFOVIEW.C has its own static fInitialize. */
extern u8 Images_fInitialize;
extern GsIMAGE Images[N_IMAGES];

extern char msg_bad_music_no[];           /* "bad music no" */
extern char fmt_xa_path[];                /* "\\TENCHU\\XA\\%s;1" */
extern char fmt_playmusic_fail_chan_id[]; /* "playmusic fail %s  chan %d  id %d" */
extern char fmt_bad_voice_no[];           /* "bad voice no %d" */
extern char fmt_playvoice_fail_chan_id[]; /* "playvoice fail %s  chan %d  id %d" */
extern char fmt_bad_archive_index[];      /* "bad archive index %d" */
extern char path_image_images_arc[];      /* K:\\WORK\\CDIMAGE\\IMAGE\\images.arc */
extern char msg_bad_image_file[];         /* "bad image file" */
extern char msg_bad_image_index[];        /* "bad image index" */
extern char path_image_models_arc[];      /* K:\\WORK\\CDIMAGE\\IMAGE\\models.arc */
extern char path_tenchu_menu_exe_1[];     /* cdrom:\\TENCHU\\MENU.EXE;1 */
extern char path_tenchu_main_exe_1[];     /* cdrom:\\TENCHU\\MAIN.EXE;1 */
extern char path_tenchu_ending_exe_1[];   /* cdrom:\\TENCHU\\ENDING.EXE;1 */
extern char path_tenchu_trial_exe_1[];    /* cdrom:\\TENCHU\\TRIAL.EXE;1 */
extern char fmt_bad_process_id[];         /* "bad process id %x" */

extern void CdaStop(void);
extern int sprintf(char *buf, char *fmt, ...);
extern void SsSetMVol(int voll, int volr);
extern void set_cda_volume_(u8 voll, u8 volr);
extern int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode);
extern BackGround *SetupBG(GsIMAGE *image, s16 w, s16 h);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern void LoadExecEx(u8 *file, u32 stack, u32 size);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitSprite(struct GsIMAGE *image, struct GsSPRITE *sprite);
 *     IMAGES.C:79, 23 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct GsIMAGE * image
 *     param $s2       struct GsSPRITE * sprite
 * END PSX.SYM */

void InitSprite(GsIMAGE *image, GsSPRITE *sprite)
{
    s32 texture_mode;
    s32 width_shift;

    *sprite = (GsSPRITE){0};
    sprite->b = 0x80;
    sprite->g = 0x80;
    sprite->r = 0x80;
    sprite->attribute = 0;
    sprite->scaley = FIXED_ONE;
    sprite->scalex = FIXED_ONE;
    if (image != 0)
    {
        texture_mode = TIM_PIXEL_MODE((u16)image->pmode);
        sprite->attribute =
            sprite->attribute | GS_ATTR_TEXTURE_MODE(texture_mode);
        width_shift = 2 - texture_mode;
        sprite->w = image->pw << width_shift;
        sprite->h = image->ph;
        sprite->tpage = GetTPage(texture_mode, 0, image->px, image->py);
        sprite->u = (u8)((image->px << width_shift) &
                          ((1 << (8 - texture_mode)) - 1));
        sprite->v = (u8)image->py;
        sprite->cx = image->cx;
        sprite->cy = image->cy;
        sprite->mx = sprite->w >> 1;
        sprite->my = sprite->h >> 1;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupImageToPolyFT4(struct GsIMAGE *image, struct POLY_FT4 *ply, short x, short y);
 *     IMAGES.C:106, 21 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct GsIMAGE * image
 *     param $s0       struct POLY_FT4 * ply
 *     param $a2       short x
 *     param $a3       short y
 *     reg   $a1       short tx
 *     reg   $a3       short ty
 *     reg   $t0       short th
 * END PSX.SYM */

void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply, short x, short y)
{
    s32 tp;
    s32 sh;
    s32 tw;
    u32 tx;
    u16 tx2;
    s32 px;
    u8 ty;
    u32 pw;
    u32 th;

    SetPolyFT4(ply);
    tp = TIM_PIXEL_MODE((u16)image->pmode);
    ply->tpage = GetTPage(tp, 1, image->px, image->py);
    ply->clut = GetClut(image->cx, image->cy);
    sh = 2 - tp;
    px = image->px;
    ty = (u8)image->py;
    pw = image->pw;
    th = image->ph;
    setRGB0(ply, 0x7F, 0x7F, 0x7F);
    ply->x0 = x;
    ply->y0 = y;
    ply->y1 = y;
    ply->x2 = x;
    tx = (px << sh) & ((1 << (8 - tp)) - 1);
    tw = pw << sh;
    x += tw;
    y += th;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
    tx2 = tx + tw;
    ply->x1 = x;
    ply->y2 = y;
    ply->x3 = x;
    ply->y3 = y;
    setUV4(ply, tx, ty, tx2, ty, tx, ty + th, tx2, ty + th);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupImageToPolyGT4(struct GsIMAGE *image, struct POLY_GT4 *ply, short x, short y);
 *     IMAGES.C:129, 25 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct GsIMAGE * image
 *     param $s0       struct POLY_GT4 * ply
 *     param $a2       short x
 *     param $a3       short y
 *     reg   $a1       short tx
 *     reg   $a3       short ty
 *     reg   $t0       short th
 * END PSX.SYM */

void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply, int x_arg, int y_arg)
{
    short x;
    short y;
    s32 tp;
    s32 sh;
    s32 tw;
    u32 tx;
    u16 tx2;
    s32 px;
    u8 ty;
    u32 pw;
    u32 th;

    x = x_arg;
    y = y_arg;

    SetPolyGT4(ply);
    tp = TIM_PIXEL_MODE((u16)image->pmode);
    ply->tpage = GetTPage(tp, 1, image->px, image->py);
    ply->clut = GetClut(image->cx, image->cy);
    sh = 2 - tp;
    px = image->px;
    ty = (u8)image->py;
    pw = image->pw;
    th = image->ph;
    setRGB0(ply, 0x7F, 0x7F, 0x7F);
    setRGB1(ply, 0x7F, 0x7F, 0x7F);
    setRGB2(ply, 0x7F, 0x7F, 0x7F);
    setRGB3(ply, 0x7F, 0x7F, 0x7F);
    ply->x0 = x;
    ply->y0 = y;
    ply->y1 = y;
    ply->x2 = x;
    tx = (px << sh) & ((1 << (8 - tp)) - 1);
    tw = pw << sh;
    x += tw;
    y += th;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
    tx2 = tx + tw;
    ply->x1 = x;
    ply->y2 = y;
    ply->x3 = x;
    ply->y3 = y;
    setUV4(ply, tx, ty, tx2, ty, tx, ty + th, tx2, ty + th);
}

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

static inline void InitMusicLocation(CdlLOC *location, u8 minute, u8 second)
{
    *location = (CdlLOC){0};
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
        (u32)(MusicNo - MUSIC_CUE_INVALID_FIRST) <
            MUSIC_CUE_INVALID_COUNT)
    {
        AdtMessageBox(msg_bad_music_no, MusicNo);
        CdaStop();
    }
    else if (MusicNo >= MUSIC_TRACK_COUNT)
    {
        PlayVoice(MusicNo +
                  (MusicNo >= MUSIC_CUE_TORA_FIRST
                       ? MUSIC_TORA_VOICE_ID_OFFSET
                       : MUSIC_NARRATION_VOICE_ID_OFFSET));
    }
    else
    {
        music = &MusicTable[MusicNo];
        sprintf((char *)fname, fmt_xa_path, music->file);
        SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
        set_cda_volume_(gSoundLevel, gSoundLevel);

        min = music->start_minute;
        sec = music->start_second;
        InitMusicLocation(&start, min, sec);

        min = music->end_minute;
        sec = music->end_second;
        InitMusicLocation(&end, min, sec);

        if (CdaPlayXA(fname, &start, &end, music->channel, (s16)mode) == 0)
        {
            AdtMessageBox(fmt_playmusic_fail_chan_id, fname, music->channel, MusicNo);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayVoice(int id);
 *     IMAGES.C:485, 35 src lines, frame 72 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int id
 *     reg   $s6       unsigned char * FileName
 *     stack sp+24     struct CdlLOC start
 *     stack sp+32     struct CdlLOC end
 *     reg   $s4       struct TVoiceTable * voice
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *     reg   $s2       struct CdlLOC * loc
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *VoiceXaName;
 *     extern unsigned char gSELevel;
 * END PSX.SYM */

static inline void BuildVoiceLocation(CdlLOC *loc, u8 min, u8 sec)
{
    s32 pos;

    *loc = (CdlLOC){0};
    loc->minute = min;
    loc->second = sec;
    pos = CdPosToInt(loc);
    CdIntToPos(pos * 2 + OFFSET, loc);
}

void PlayVoice(int id)
{
    u8 *FileName;
    TVoiceTable *voice;
    s32 volume;
    TVoiceTable *match;
    TVoiceTable *cursor;
    TVoiceTable *next;
    int end_marker;
    TVoiceTable *fallback;
    int fallback_end;
    u8 *filenames[N_LANGUAGES] = {
        VoiceXaName,
        VoiceXaNameF,
        VoiceXaNameI,
        VoiceXaNameJ,
    };
    TVoiceTable *tables[N_LANGUAGES];
    CdlLOC start;
    CdlLOC end;

    __builtin_memcpy(tables, EventVoiceTables, sizeof(tables));
    start = (CdlLOC){0};
    end = (CdlLOC){0};

    if (id >= VOICE_ID_INTRO_BASE)
    {
        if (id >= VOICE_ID_TORA_BASE)
        {
            voice = ToraVoiceTable;
            id -= VOICE_ID_TORA_BASE;
            FileName = ToraVoiceXaName;
        }
        else
        {
            voice = IntroVoiceTable;
            FileName = IntroVoiceXaName;
            id -= VOICE_ID_INTRO_BASE;
        }
        match = 0;
        if (voice->id != SOUND_TABLE_END)
        {
            do
            {
                next = cursor = voice;
                if (id != 0)
                {
                    voice = next;
                }
                else
                {
                    voice = next;
                }
                match = voice;
                if (id == cursor->id)
                    break;
                next = cursor + 1;
                voice = next;
            } while (next->id != SOUND_TABLE_END);
            if (id != cursor->id)
                match = 0;
        }
        goto found;
    }
    else
    {
        int language;
        u8 **filename_entry;
        TVoiceTable **voice_entry;

        filename_entry = filenames + CHOSEN_LANGUAGE;
        language = filename_entry - filenames;
        voice_entry = tables + language;
        /* Empty loop retained for code layout; its original source construct is unknown. */
        do
        {
        } while (0);
        voice = *voice_entry;
        FileName = *filename_entry;
        match = 0;
        if (voice->id != SOUND_TABLE_END)
        {
            end_marker = SOUND_TABLE_END;
            cursor = voice;
            do
            {
                next = match = cursor;
                if (id != 0)
                {
                    cursor = next;
                }
                else
                {
                    if (voice != 0)
                    {
                        cursor = next;
                    }
                    else
                    {
                        cursor = next;
                    }
                }
                if (id == cursor->id)
                    break;
                cursor++;
                match = 0;
            } while (cursor->id != end_marker);
        }
    }
found:
    if (match == 0)
    {
        fallback = CommonVoiceTable;
        if (fallback->id != SOUND_TABLE_END)
        {
            fallback_end = SOUND_TABLE_END;
            cursor = fallback;
            do
            {
                if (id == cursor->id)
                {
                    match = cursor;
                    goto found2;
                }
                cursor++;
            } while (cursor->id != fallback_end);
        }
        match = 0;
    found2:
        FileName = VoiceXaName;
        if (match == 0)
        {
            AdtMessageBox(fmt_bad_voice_no, id);
            CdaStop();
            return;
        }
    }

    volume = gSELevel;
    if (volume >= SOUND_VOLUME_MAX)
        volume = SOUND_VOLUME_MAX;
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    set_cda_volume_(volume, volume);

    {
        u8 min;
        u8 sec;

        min = match->start_minute;
        sec = match->start_second;
        BuildVoiceLocation(&start, min, sec);
    }

    {
        u8 min;
        u8 sec;

        min = match->end_minute;
        sec = match->end_second;
        BuildVoiceLocation(&end, min, sec);
    }

    if (CdaPlayXA(FileName, &start, &end, match->channel, CDA_ONCE) == 0)
    {
        AdtMessageBox(fmt_playvoice_fail_chan_id, FileName, match->channel, id);
    }
}

u_long *get_tim_from_archive(ArcFile *archive, int idx)
{
    s32 i;
    s32 entry_offset;

    if (archive->loaded == ARC_ENTRIES_RELATIVE)
    {
        i = 0;
        if (archive->count > 0)
        {
            do
            {
                entry_offset =
                    archive->entry[i].offset + ARC_ENTRY_TABLE_OFFSET;
                archive->entry[i].data =
                    (u_long *)((u8 *)archive + entry_offset);
                i++;
            } while (i < archive->count);
        }
        archive->loaded = ARC_ENTRIES_ABSOLUTE;
    }
    if (idx < 0 || archive->count <= idx)
    {
        AdtMessageBox(fmt_bad_archive_index, idx);
        return 0;
    }
    return archive->entry[idx].data;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct GsIMAGE * GetImage(int index);
 *     IMAGES.C:52, 16 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int index
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE Images[52];
 * END PSX.SYM */

GsIMAGE *GetImage(ImageArchiveId index)
{
    ArcFile *archive;
    u_long *adr;
    int i;

    if (Images_fInitialize == 0)
    {
        archive = (ArcFile *)FileRead(path_image_images_arc);
        if (archive->count < N_IMAGES)
        {
            AdtMessageBox(msg_bad_image_file);
        }
        for (i = 0; i < N_IMAGES; i++)
        {
            adr = get_tim_from_archive(archive, i);
            GetTIMInfo(adr, &Images[i]);
            LoadTIM(adr);
        }
        vfree(archive);
        Images_fInitialize = 1;
    }
    if ((unsigned)index >= N_IMAGES)
    {
        AdtMessageBox(msg_bad_image_index);
        return Images;
    }
    else
    {
        return Images + index;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * GetArcData(int index);
 *     IMAGES.C:158, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int index
 * END PSX.SYM */

u_long *GetArcData(int index)
{
    s32 i;
    ArcFile *arc;

    if (ArcData == 0)
    {
        ArcData = (ArcFile *)FileRead(path_image_models_arc);
    }
    arc = ArcData;
    if (arc->loaded == ARC_ENTRIES_RELATIVE)
    {
        for (i = 0; i < arc->count; i++)
        {
            arc->entry[i].data =
                (u_long *)((u8 *)arc +
                           (arc->entry[i].offset + ARC_ENTRY_TABLE_OFFSET));
        }
        arc->loaded = ARC_ENTRIES_ABSOLUTE;
    }
    if (index < 0 || arc->count <= index)
    {
        AdtMessageBox(fmt_bad_archive_index, index);
        return 0;
    }
    return arc->entry[index].data;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeImage(void);
 *     IMAGES.C:32, 16 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE Images[52];
 * END PSX.SYM */

void InitializeImage(void)
{
    ArcFile *archive;
    u_long *adr;
    int i;

    archive = (ArcFile *)FileRead(path_image_images_arc);
    if (archive->count < N_IMAGES)
    {
        AdtMessageBox(msg_bad_image_file);
    }
    for (i = 0; i < N_IMAGES; i++)
    {
        adr = get_tim_from_archive(archive, i);
        GetTIMInfo(adr, &Images[i]);
        LoadTIM(adr);
    }
    vfree(archive);
}

BackGround *load_background_(u_long *tim)
{
    BackGround *bg;
    s32 i;
    GsIMAGE im;

    GetTIMInfo(tim, &im);
    bg = SetupBG(&im, SCREEN_W, SCREEN_H);
    bg->sz = 100;
    LoadTIM(tim);
    i = 0;
    if (0 < bg->map.ncellw * bg->map.ncellh)
    {
        do
        {
            bg->index[i] = (u16)i;
            i++;
        } while (i < bg->map.ncellw * bg->map.ncellh);
    }
    return bg;
}

void load_archive_sprite_(ArcFile *archive, int idx)
{
    GsIMAGE img;
    u_long *adr;

    adr = get_tim_from_archive(archive, idx);
    GetTIMInfo(adr, &img);
    LoadTIM(adr);
    SetupSprite(0, &img);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayMusicFormID(int id);
 *     IMAGES.C:524, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int id
 * END PSX.SYM */

/* Explicit binding avoids an eight-byte offset error in the generated symbol. */
void PlayMusicFormID(s32 event_audio_id)
{
    s32 cue;
    s16 track;

    if (event_audio_id < MUSIC_EVENT_ID_BASE)
    {
        PlayVoice(event_audio_id);
        return;
    }
    cue = event_audio_id - MUSIC_EVENT_ID_BASE;
    track = 0;
    while (MusicIdByTrack[track] != SOUND_TABLE_END)
    {
        if (MusicIdByTrack[track] == cue)
        {
            break;
        }
        track++;
    }
    if (MusicIdByTrack[track] != SOUND_TABLE_END)
    {
        cue = track;
    }
    _PlayMusic(cue, CDA_REPEAT);
}

void apply_cd_volume_(void)
{
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    set_cda_volume_(gSoundLevel, gSoundLevel);
}

void exec_process_(int id)
{
    switch (id)
    {
    case PROCESS_MENU:
        LoadExecEx((u8 *)path_tenchu_menu_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case PROCESS_MAIN:
        LoadExecEx((u8 *)path_tenchu_main_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case PROCESS_ENDING:
        LoadExecEx((u8 *)path_tenchu_ending_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case PROCESS_TRIAL:
        LoadExecEx((u8 *)path_tenchu_trial_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    default:
        AdtMessageBox(fmt_bad_process_id, id);
        exec_process_(PROCESS_MENU);
        return;
    }
}

u8 *search_id_table_(u8 *table, s32 id)
{
    while (*table != SOUND_TABLE_END)
    {
        if (id == *table)
            return table;
        table += 6;
    }
    return 0;
}
