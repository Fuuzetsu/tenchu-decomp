#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"
#include "misc.h"
#include "images.h"
#include "item.h"
#include "tmdfast.h"

extern char fmt_unknown_door_type[];    /* unknown door type %d */
extern char fmt_unknown_pitfall_type[]; /* unknown pitfall type %d */
extern char fmt_undefined_effect[];     /* undefined effect %d */
extern char msg_misc_not_initialized[]; /* misc not initialized */
extern char msg_unknown_sprite_type[];  /* unknown sprite type */
extern u8 *MiscTimNames[7];
extern u8 path_image_2[]; /* K:\\WORK\\CDIMAGE\\IMAGE\\ */

extern ModelType *LoadModel(u_long *adr);
extern void DisposeModel(ModelType *model);
extern short DrawModel(ModelType *objp);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern void DrawSpriteXYZ(GsSPRITE *sprt, s32 x, s32 y, s32 z, s32 scale);
extern void SetupTexScroll(GsIMAGE *im, short vx, short vy);

/*
 * The demo line records order the shared routines as ResetAllMisc, InitMisc,
 * ProcMiscFire, ProcMiscDoor, ProcMiscPitfall, ProcMiscSnowfall,
 * ProcMiscSprite, AddMisc, and DoMiscProc. Retail adds three handlers and
 * moves ResetAllMisc behind DoMiscProc. ProcMiscFire and ProcMiscSprite remain
 * in their debug-proven source positions below; their static inline copies are
 * emitted at the end of the object by GCC 2.8.
 */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitMisc(void);
 *     MISC.C:162, 83 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $a0       int iDoor1
 *     reg   $s1       int iDoor2
 *     reg   $v0       struct ModelType * data
 *     reg   $a0       int id1
 *     reg   $s1       int id2
 *     reg   $v0       struct ModelType * data
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 *     extern struct MISC__183fake DoorData[11];
 *     extern struct MISC__185fake SpriteData[2];
 *     extern struct MISC__184fake PitfallData[2];
 * END PSX.SYM */

void InitMisc(void)
{
    TMisc *tm;
    s32 i;

    i = MaxMisc - 1;
    tm = misc;
    tm += (MaxMisc - 1);
    do
    {
        tm->proc = 0;
        i--;
        tm--;
    } while (i >= 0);

    {
        ModelArchiveId iDoor1;
        ModelArchiveId iDoor2;
        ModelType *data;

        for (i = 0; i < N_DOOR_TYPES; i++)
        {
            iDoor1 = (ModelArchiveId)DoorData[i].Model[0];
            iDoor2 = (ModelArchiveId)DoorData[i].Model[1];
            if (iDoor1 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(iDoor1));
                DoorData[i].Model[0] = data;
            }
            if (iDoor2 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(iDoor2));
                DoorData[i].Model[1] = data;
            }
        }
    }

    {
        SpriteDataType *spr;
        u32 attr;

        i = 0;
        attr = GS_ATTR_SEMITRANS_ADD;
        spr = SpriteData;
        do
        {
            i++;
            spr->spr = SetupSprite((Sprite3D *)0,
                                   GetImage((ImageArchiveId)spr->spr));
            spr->spr->sprite.attribute = attr;
            spr->spr->scale = spr->scale;
            spr++;
        } while (i < N_MISC_SPRITE_TYPES);
    }

    {
        ModelArchiveId id1;
        ModelArchiveId id2;
        ModelType *data;

        for (i = 0; i < N_PITFALL_TYPES; i++)
        {
            id1 = (ModelArchiveId)PitfallData[i].Model[0];
            id2 = (ModelArchiveId)PitfallData[i].Model[1];
            if (id1 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(id1));
                PitfallData[i].Model[0] = data;
            }
            if (id2 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(id2));
                PitfallData[i].Model[1] = data;
            }
        }
    }

    Misc_fInitial = 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscFire(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:249, 41 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     stack sp+16     struct SVECTOR vec
 *     stack sp+24     struct VECTOR pos
 * END PSX.SYM */

static inline void ProcMiscFire(TMisc *m, TMiscMessage msg)
{
    switch (msg)
    {
    case MM_CREATE:
        m->mode = 0;
        m->count = 10;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if (m->mode != 0)
            break;
        m->count--;
        if (m->count < 1)
        {
            SVECTOR vec = {
                .vx = 0,
                .vy = -35,
                .vz = 0
            };
            VECTOR pos;

            pos.vx = m->x;
            pos.vy = m->y;
            pos.vz = m->z;
            SetExplosion(&pos, &vec);
            vec.vx = 75;
            vec.vy = 180;
            vec.vz = 75;
            SetHinoko(&pos, &vec, 10);
            vec.vx = 0;
            vec.vy = -200;
            vec.vz = 0;
            SetSmoke(&pos, &vec, 20, 6);
            m->count = rand() % 150;
            SoundEx(&pos, SE_FIRE);
        }
        break;
    }
}

static void proc_misc_bonfire_(TMisc *m, TMiscMessage msg)
{
    SVECTOR direction[2];
    GsSPRITE *frame;

    direction[0] = (SVECTOR){
        .vx = 0,
        .vy = -60,
        .vz = 0
    };
    frame = &sprFrame[GameClock % MaxFrames];

    switch (msg)
    {
    case MM_CREATE:
        m->mode = 0;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if (m->mode != 0)
            break;

        frame->r = frame->g = frame->b = (u8)(rand() % 100 + 100);
        DrawSpriteXYZ(frame, m->x, m->y, m->z,
                      m->param.bonfire.scale);

        if ((GameClock & 0xF) == 0)
        {
            VECTOR bleed_pos = {
                .vx = m->x,
                .vy = m->y,
                .vz = m->z
            };

            SetBleedsDir(&bleed_pos, direction, 100, 10, 30,
                         RGB24(100, 100, 60));
        }

        if (GameClock % 79 == 0)
        {
            VECTOR pos = {
                .vx = m->x,
                .vy = m->y,
                .vz = m->z
            };

            SoundEx(&pos, SE_BONFIRE);
        }
        break;
    }
}

static void proc_misc_sound_(TMisc *m, TMiscMessage msg)
{
    MiscSoundSchedule *sched;
    MiscSoundSchedule tmp;
    s32 lo;

    sched = &m->param.sound;

    switch (msg)
    {
    case MM_CREATE:
        tmp.min_delay = m->param.sound_init.min_delay;
        tmp.max_delay = m->param.sound_init.max_delay;
        tmp.sound_index = (u8)m->param.sound_init.sound;
        tmp.next = GameClock;
        *sched = tmp;
        m->mode = 0;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if (m->mode != 0)
            break;
        if (sched->next > GameClock)
            break;

        {
            VECTOR pos = {
                .vx = m->x,
                .vy = m->y,
                .vz = m->z
            };

            SoundEx(&pos, sched->sound_index + MISC_SOUND_ID_BASE);
        }

        if (sched->max_delay - sched->min_delay > 0)
        {
            lo = GameClock +
                 (rand() % (sched->max_delay - sched->min_delay) +
                  sched->min_delay);
        }
        else
        {
            lo = GameClock + sched->min_delay;
        }
        sched->next = lo;
        break;
    }
}

static inline void proc_misc_puff_(TMisc *m, TMiscMessage msg)
{
    VECTOR pos;
    SVECTOR dir;
    s32 x, z;

    switch (msg)
    {
    case MM_CREATE:
        m->mode = 0;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if (m->mode != 0)
            break;

        x = m->x;
        pos.vx = (1 <= (m->param.puff.x_radius << 1))
                     ? x + (rand() % (m->param.puff.x_radius << 1) -
                            m->param.puff.x_radius)
                     : x - m->param.puff.x_radius;

        pos.vy = m->y;
        z = m->z;
        pos.vz = (1 <= (m->param.puff.z_radius << 1))
                     ? z + (rand() % (m->param.puff.z_radius << 1) -
                            m->param.puff.z_radius)
                     : z - m->param.puff.z_radius;

        if ((GameClock & 1) == 0)
        {
            dir.vx = 0;
            dir.vy = -400;
            dir.vz = 0;
            pos.vy += 2000;
            SetSmoke(&pos, &dir, 1, 1);
            pos.vy -= 2000;
            SetSplash(&pos, 4 * FIXED_ONE, 2 * FIXED_ONE, 10);
        }
        break;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscDoor(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:294, 116 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s1       struct TDoor * param
 *     reg   $s2       int r
 *     reg   $a1       int type
 *     reg   $v0       int t
 *     reg   $v0       int cid
 *     reg   $v0       int dir
 *     reg   $s2       int w
 *     reg   $s0       struct ModelType * model
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct MISC__183fake DoorData[11];
 * END PSX.SYM */

enum
{
    DOOR_ANGLE_STEP = ANGLE_FULL / 64,
    DOOR_OPEN_ANGLE = ANGLE_QUADRANT - DOOR_ANGLE_STEP,
    DOOR_OPENING_OFFSET_DIVISOR = 5 * ANGLE_HALF
};

static void ProcMiscDoor(TMisc *m, TMiscMessage msg)
{
    TDoor *param;

    param = &m->param.door;
    switch (msg)
    {
    case MM_CREATE:
    {
        s32 type;
        s32 rotation;

        type = m->param.hinged_init.type;
        rotation = m->param.hinged_init.rotation;
        if (type >= N_DOOR_TYPES)
        {
            AdtMessageBox(fmt_unknown_door_type, type);
            type = DOOR_KIND_MON6;
        }
        m->mode = DOOR_MODE_IDLE;
        param->r = 0;
        param->type = type;
        param->locate = LoadModel(0);
        param->locate->locate.coord.t[0] = m->x;
        param->locate->locate.coord.t[1] = m->y;
        param->locate->locate.coord.t[2] = m->z;
        param->locate->rotate.vx = 0;
        param->locate->rotate.vy = rotation;
        param->locate->rotate.vz = 0;
        UpdateCoordinate(param->locate);
        return;
    }

    case MM_DESTROY:
        DeleteConflict(param->locate);
        DisposeModel(param->locate);
        return;

    case MM_PAUSE:
        DeleteConflict(param->locate);
        return;

    case MM_RESUME:
    {
        s32 conflict_id;
        s32 height;
        s16 width;

        conflict_id = InsertConflict(param->locate);
        ConflictObject[conflict_id].offset.vx = 0;
        height = DoorData[param->type].HitSize;
        ConflictObject[conflict_id].offset.vz = 0;
        ConflictObject[conflict_id].offset.vy = -height / 2;
        width = DoorData[param->type].HitSize;
        ConflictObject[conflict_id].common = (void *)CONFLICT_OWNER_DOOR;
        ConflictObject[conflict_id].size.pad = CONFLICT_SOFT;
        ConflictObject[conflict_id].size.vy = width;
        width = (width / 3) * 2;
        ConflictObject[conflict_id].size.vz =
            ConflictObject[conflict_id].size.vx = width;
        param->r = 0;
        return;
    }

    default:
    {
        s32 width;
        ModelType *model;

        switch (m->mode)
        {
        case DOOR_MODE_IDLE:
            if ((param->locate->attribute & MODEL_ATTR_CONFLICT) != 0)
            {
                s32 conflict_id;

                conflict_id = GetConflictResult(param->locate, CONFLICT_NONE);
                if (ConflictObject[conflict_id].common !=
                    (void *)CONFLICT_OWNER_DOOR)
                {
                    s32 relative_angle;

                    relative_angle = ratan2(
                        ConflictObject[conflict_id].position.vz -
                            param->locate->locate.coord.t[2],
                        ConflictObject[conflict_id].position.vx -
                            param->locate->locate.coord.t[0]);
                    relative_angle += param->locate->rotate.vy;
                    if (((relative_angle + 2 * ANGLE_FULL) % ANGLE_FULL) <=
                        ANGLE_HALF)
                        param->dr = DOOR_ANGLE_STEP;
                    else
                        param->dr = -DOOR_ANGLE_STEP;
                    m->mode++;
                    if (param->r == 0)
                        SoundEx(MODEL_POSITION(param->locate), SE_MECHANISM);
                }
            }
            break;

        case DOOR_MODE_OPENING:
        {
            s32 angle;

            angle = __builtin_abs(param->r);
            if (angle < DOOR_OPEN_ANGLE)
                param->r += param->dr;
            else
                m->mode = DOOR_MODE_IDLE;
        }
        break;
        }
        {
            s32 angle;

            width = DoorData[param->type].HitSize;
            angle = __builtin_abs(param->r);
            width -= (width * angle) / DOOR_OPENING_OFFSET_DIVISOR;
            model = DoorData[param->type].Model[0];
        }
        if (model != (ModelType *)MODEL_ARCHIVE_NONE)
        {
            GsCOORDINATE2 *parent;

            parent = &param->locate->locate;
            model->locate.coord.t[0] = -width;
            model->locate.coord.t[1] = 0;
            model->locate.coord.t[2] = 0;
            model->locate.super = parent;
            model->rotate.vy = param->r;
            UpdateCoordinate(model);
            DrawModel(model);
        }

        model = DoorData[param->type].Model[1];
        if (model != (ModelType *)MODEL_ARCHIVE_NONE)
        {
            GsCOORDINATE2 *parent;

            parent = &param->locate->locate;
            model->locate.coord.t[0] = width;
            model->locate.coord.t[1] = 0;
            model->locate.coord.t[2] = 0;
            model->locate.super = parent;
            model->rotate.vy = -param->r;
            UpdateCoordinate(model);
            DrawModel(model);
        }
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscPitfall(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:414, 106 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s2       struct TPitfall * param
 *     reg   $s0       struct ModelType * model
 *     reg   $s1       short w
 *     reg   $s0       int r
 *     reg   $a1       int type
 *     reg   $v0       int t
 *
 * Globals it touches, as the original declared them:
 *     extern struct MISC__184fake PitfallData[2];
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

static void ProcMiscPitfall(TMisc *m, TMiscMessage msg)
{
    TPitfall *param;
    short w;

    param = &m->param.pitfall;
    switch (msg)
    {
    case MM_CREATE:
    {
        int type;
        int t;

        type = m->param.hinged_init.type;
        t = m->param.hinged_init.rotation;
        if (type >= N_PITFALL_TYPES)
        {
            AdtMessageBox(fmt_unknown_pitfall_type, type);
            type = PITFALL_KIND_OTO_LEFT;
        }
        m->mode = PITFALL_MODE_CLOSED;
        param->r = 0;
        param->type = type;
        param->locate = LoadModel(0);
        param->locate->locate.coord.t[0] = m->x;
        param->locate->locate.coord.t[1] = m->y;
        param->locate->locate.coord.t[2] = m->z;
        param->locate->rotate.vx = 0;
        param->locate->rotate.vy = t;
        param->locate->rotate.vz = 0;
        UpdateCoordinate(param->locate);
    }
        return;

    case MM_DESTROY:
        DeleteConflict(param->locate);
        DisposeModel(param->locate);
        return;

    case MM_PAUSE:
        DeleteConflict(param->locate);
        return;

    case MM_RESUME:
    {
        int conflict_id;

        w = PitfallData[param->type].HitSize;
        conflict_id = InsertConflict(param->locate);
        ConflictObject[conflict_id].offset.vx = 0;
        ConflictObject[conflict_id].offset.vy = 0;
        ConflictObject[conflict_id].offset.vz = 0;
        ConflictObject[conflict_id].common = (void *)CONFLICT_OWNER_DOOR;
        ConflictObject[conflict_id].size.pad =
            CONFLICT_SOFT;
        ConflictObject[conflict_id].size.vx = w;
        ConflictObject[conflict_id].size.vy =
            ConflictObject[conflict_id].size.vz = (w / 3) * 2;
    }
        return;

    default:
    {
        ModelType *model;
        ConflictObjectType *conflict;
        int conflict_id;
        int mode;

        mode = m->mode;
        switch (mode)
        {
        case PITFALL_MODE_CLOSED:
            if ((param->locate->attribute & MODEL_ATTR_CONFLICT) != 0)
            {
                conflict = ConflictObject;
                conflict_id =
                    GetConflictResult(param->locate, CONFLICT_NONE);
                if (conflict[conflict_id].common !=
                    (void *)CONFLICT_OWNER_DOOR)
                {
                    m->mode++;
                    SoundEx(MODEL_POSITION(param->locate), SE_MECHANISM);
                }
            }
            break;

        case PITFALL_MODE_OPENING:
            param->r += ANGLE_QUADRANT / 6;
            if (param->r >= ANGLE_QUADRANT)
            {
                param->r = ANGLE_QUADRANT;
                m->mode++;
            }
            break;

        case PITFALL_MODE_OPEN:
            break;
        }

        model = PitfallData[param->type].Model[0];
        w = PitfallData[param->type].HitSize;
        if (model != (ModelType *)MODEL_ARCHIVE_NONE)
        {
            model->locate.super = &param->locate->locate;
            model->locate.coord.t[0] = -w;
            model->locate.coord.t[1] = 0;
            model->locate.coord.t[2] = 0;
            model->rotate.vz = param->r;
            UpdateCoordinate(model);
            DrawModel(model);
        }
        model = PitfallData[param->type].Model[1];
        if (model != (ModelType *)MODEL_ARCHIVE_NONE)
        {
            model->locate.super = &param->locate->locate;
            model->locate.coord.t[0] = w;
            model->locate.coord.t[1] = 0;
            model->locate.coord.t[2] = 0;
            model->rotate.vz = -param->r;
            UpdateCoordinate(model);
            DrawModel(model);
        }
    }
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscSnowfall(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:525, 53 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s4       struct TSnowfall * param
 *     reg   $s1       int i
 *     reg   $v0       int w
 *     reg   $v1       int h
 *     reg   $s0       struct SVECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

static void ProcMiscSnowfall(TMisc *m, TMiscMessage msg)
{
    TSnowfall *param = &m->param.snowfall;

    switch (msg)
    {
    case MM_CREATE:
    {
        s32 w = m->param.snowfall.w;
        s32 h = m->param.snowfall.h;

        m->mode = 0;
        param->w = w;
        param->h = h;
        break;
    }

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if ((GameClock & 3) == 0)
        {
            SVECTOR velocity = {
                .vx = rand() % 20 - 10,
                .vy = rand() % 50 + 50,
                .vz = rand() % 20 - 10
            };
            VECTOR position = {
                .vx = ViewInfo.vrx + (rand() % SNOW_SPAN - SNOW_RANGE),
                .vy = ViewInfo.vry + (rand() % SNOW_RANGE - SNOW_SPAN),
                .vz = ViewInfo.vrz + (rand() % SNOW_SPAN - SNOW_RANGE)
            };

            SetSnow(&position, &velocity, FIXED_ONE, SNOW_SPRITE_DEFAULT);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscSprite(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:582, 50 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $v1       int type
 *     reg   $s0       struct Sprite3D * s
 * END PSX.SYM */

static inline void ProcMiscSprite(TMisc *m, TMiscMessage msg)
{
    s32 type;
    Sprite3D *s;

    switch (msg)
    {
    case MM_CREATE:
        type = m->param.init.a;
        if (type >= N_MISC_SPRITE_TYPES)
        {
            AdtMessageBox(msg_unknown_sprite_type);
            type = MISC_SPRITE_FIRE1;
        }
        m->mode = 0;
        m->param.sprite.type = (misc_sprite_kind)type;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        s = SpriteData[m->param.sprite.type].spr;
        s->sprite.b = s->sprite.g = s->sprite.r =
            (u8)(rand() % 60 + 0x62);
        s->locate.coord.t[0] = m->x;
        s->locate.coord.t[1] = m->y;
        s->locate.coord.t[2] = m->z;
        UpdateCoordinate((ModelType *)s);
        DrawSprite(s);
        break;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddMisc(enum MiscType type, int x, int y, int z, int a, int b, int c);
 *     MISC.C:636, 47 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       enum MiscType type
 *     param $a1       int x
 *     param $a2       int y
 *     param $a3       int z
 *     param stack+16  int a
 *     param stack+20  int b
 *     param stack+24  int c
 *     reg   $a0       int a
 *     reg   $t1       int b
 *     reg   $t2       int c
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

void AddMisc(MiscType type, s32 x, s32 y, s32 z, s32 a, s32 b, s32 c)
{
    TMisc *base = misc;
    TMisc *p;
    u8 *tim_names[7];
    GsIMAGE tm;
    u8 **name_table = tim_names;
    GsIMAGE *ptm = &tm;
    s32 va = a;
    s32 vb = b;
    s32 vc = c;
    u8 **selected_name;
    u_long *adr;

    p = base;
loop:
    if (p->proc == 0)
    {
        do
        {
            p->mode = 0;
            p->x = x;
            p->y = y;
            p->z = z;
            p->param.init.a = va;
            p->param.init.b = vb;
            p->param.init.c = vc;
            switch (type)
            {
            case MISC_FIRE:
                if (va == 0)
                    p->proc = ProcMiscFire;
                else
                    p->proc = proc_misc_puff_;
                break;
            case MISC_DOOR:
                p->proc = ProcMiscDoor;
                break;
            case MISC_PITFALL:
                p->proc = ProcMiscPitfall;
                break;
            case MISC_SNOWFALL:
                p->proc = ProcMiscSnowfall;
                break;
            case MISC_SPRITE:
                p->proc = ProcMiscSprite;
                break;
            case MISC_TEXSCROLL:
                __builtin_memcpy(tim_names, MiscTimNames, sizeof(tim_names));
                selected_name = name_table + x;
                adr = PathFileRead(path_image_2, *selected_name);
                GetTIMInfo(adr, ptm);
                LoadTIMAndFree(adr);
                SetupTexScroll(ptm, y, z);
                return;
            case MISC_BONFIRE:
                p->proc = proc_misc_bonfire_;
                break;
            case MISC_SOUND:
                p->proc = proc_misc_sound_;
                break;
            default:
                AdtMessageBox(fmt_undefined_effect, type);
                return;
            }
            p->proc(p, MM_CREATE);
            p->pause = MISC_PAUSED;
        } while (0);
        return;
    }
    do
    {
        do
        {
            p++;
            /* The original ownership test compares the addresses as signed values. */
            if ((s32)p < (s32)(base + MaxMisc))
                goto loop;
        } while (0);
    } while (0);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoMiscProc(void);
 *     MISC.C:713, 19 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

/* Misc visibility distance from MISC.C's anonymous enum. */
enum
{
    LEN = 15000
};

void DoMiscProc(void)
{
    TMisc *p;
    GsRVIEW2 *view;
    void (*proc)(TMisc *, TMiscMessage);

    if (Misc_fInitial == 0)
    {
        AdtMessageBox(msg_misc_not_initialized);
    }
    else
    {
        if (GameClock % 10 == 0)
        {
            s32 i;

            i = 0;
            view = &ViewInfo;
            p = misc;
        cull_loop:
            proc = p->proc;
            if (proc != 0)
            {
                if (__builtin_abs(view->vrx - p->x) < LEN &&
                    __builtin_abs(view->vry - p->y) < LEN &&
                    __builtin_abs(view->vrz - p->z) < LEN)
                {
                    if (p->pause != MISC_ACTIVE)
                    {
                        proc(p, MM_RESUME);
                        p->pause = MISC_ACTIVE;
                    }
                }
                else if (p->pause == MISC_ACTIVE)
                {
                    p->proc(p, MM_PAUSE);
                    p->pause = MISC_PAUSED;
                }
            }
            i++;
            p++;
            if (i < MaxMisc)
                goto cull_loop;
        }
        {
            s32 i;

            DrawTMDmode = TMD_BANK_FOG;
            for (i = 0; i < MaxMisc; i++)
            {
                if (misc[i].proc != 0 && misc[i].pause == MISC_ACTIVE)
                {
                    misc[i].proc(&misc[i], MM_DO);
                }
            }
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ResetAllMisc(void);
 *     MISC.C:147, 11 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

void ResetAllMisc(void)
{
    TMisc *p;
    s32 i;

    for (i = 0; i < MaxMisc; i++)
    {
        p = &misc[i];
        if (p->proc != 0)
        {
            p->proc(p, MM_DESTROY);
            p->proc = 0;
        }
    }
}
