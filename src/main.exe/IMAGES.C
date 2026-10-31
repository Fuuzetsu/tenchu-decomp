#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "model.h"
#include "adt.h"
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

/* Convert the VRAM word coordinate to a texel offset within its page. */
#define GET_IMAGE_TEXTURE_U(texture_u, vram_x, shift, mask)                   \
    do {                                                                     \
        (texture_u) = (vram_x) << (shift);                                    \
        (texture_u) &= (mask);                                                \
    } while (0)

/* Advance both screen coordinates to the opposite rectangle corner. */
#define OFFSET_IMAGE_CORNER(x, y, width, height)                             \
    do {                                                                     \
        (x) += (width);                                                      \
        (y) += (height);                                                     \
    } while (0)

#define SHADE_IMAGE_FT4(p) (setRGB0(p, 0x7F, 0x7F, 0x7F))
#define SHADE_IMAGE_GT4(p)                                                    \
    (setRGB0(p, 0x7F, 0x7F, 0x7F), setRGB1(p, 0x7F, 0x7F, 0x7F),           \
     setRGB2(p, 0x7F, 0x7F, 0x7F), setRGB3(p, 0x7F, 0x7F, 0x7F))

/*
 * Shared TIM rectangle mapping for the flat and Gouraud packet layouts.
 * The caller has set the texture page from the captured TIM flags. Normalize
 * that working mode for pixel geometry, and advance the working x/y values
 * after storing the near corner. Each packet layout supplies its colour setter.
 */
#define MAP_IMAGE_QUAD(image, polygon, x, y, mode, shade)                     \
    do {                                                                     \
        s32 width_shift;                                                     \
        s32 pixel_width;                                                     \
        u32 left_u;                                                          \
        u16 right_u;                                                         \
        s32 vram_x;                                                          \
        u8 top_v;                                                            \
        u32 word_width;                                                      \
        u32 height;                                                          \
        u32 u_mask;                                                          \
                                                                             \
        (polygon)->clut = GetClut((image)->cx, (image)->cy);                   \
        (mode) = TIM_PIXEL_MODE(mode);                                       \
        width_shift = 2 - (mode);                                            \
        u_mask = (1 << (8 - (mode))) - 1;                                     \
        vram_x = (image)->px;                                                \
        top_v = (u8)(image)->py;                                             \
        word_width = (image)->pw;                                            \
        height = (image)->ph;                                                \
        shade(polygon);                                                      \
        (polygon)->x0 = (x);                                                 \
        (polygon)->y0 = (y);                                                 \
        (polygon)->y1 = (y);                                                 \
        (polygon)->x2 = (x);                                                 \
        GET_IMAGE_TEXTURE_U(left_u, vram_x, width_shift, u_mask);             \
        pixel_width = word_width << width_shift;                             \
        OFFSET_IMAGE_CORNER(x, y, pixel_width, height);                       \
        right_u = left_u + pixel_width;                                      \
        (polygon)->x1 = (x);                                                 \
        (polygon)->y2 = (y);                                                 \
        (polygon)->x3 = (x);                                                 \
        (polygon)->y3 = (y);                                                 \
        setUV4(polygon, left_u, top_v, right_u, top_v,                        \
               left_u, top_v + height, right_u, top_v + height);             \
    } while (0)

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
    u16 tim_mode;

    SetPolyFT4(ply);
    tim_mode = (u16)image->pmode;
    ply->tpage = GetTPage(TIM_PIXEL_MODE(tim_mode), 1, image->px, image->py);
    MAP_IMAGE_QUAD(image, ply, x, y, tim_mode, SHADE_IMAGE_FT4);
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
    short x = x_arg;
    short y = y_arg;
    u16 tim_mode;

    SetPolyGT4(ply);
    tim_mode = (u16)image->pmode;
    ply->tpage = GetTPage(TIM_PIXEL_MODE(tim_mode), 1, image->px, image->py);
    MAP_IMAGE_QUAD(image, ply, x, y, tim_mode, SHADE_IMAGE_GT4);
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

/* Search a sentinel-terminated voice bank without changing its records. */
static inline TVoiceTable *FindVoiceRecord(TVoiceTable *table, int id)
{
    int i;

    for (i = 0; table[i].id != SOUND_TABLE_END; i++)
    {
        if (id == table[i].id)
            return &table[i];
    }
    return NULL;
}

void PlayVoice(int id)
{
    u8 *FileName;
    s32 volume;
    TVoiceTable *match;
    TVoiceTable *cursor;
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
        TVoiceTable *voice;

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
        match = FindVoiceRecord(voice, id);
    }
    else
    {
        TVoiceTable *voice;
        int language;

        language = CHOSEN_LANGUAGE;
        FileName = filenames[language];
        voice = tables[language];
        match = FindVoiceRecord(voice, id);
    }
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

Sprite3D *load_archive_sprite_(ArcFile *archive, int idx)
{
    GsIMAGE img;
    u_long *adr;

    adr = get_tim_from_archive(archive, idx);
    GetTIMInfo(adr, &img);
    LoadTIM(adr);
    return SetupSprite(0, &img);
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
