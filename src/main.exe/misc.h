#ifndef MISC_H
#define MISC_H

/*
 * Shared types of the original MISC.C translation unit (the misc-object
 * spawner AddMisc.c and its dispatch table ProcMisc*, plus InitMisc/
 * DoMiscProc). Layouts follow the demo's PSX.SYM (reference/psxsym-types.h,
 * union MISC__181fake) and are proven by the matched AddMisc.c and
 * ResetAllMisc.c implementations as well as the processors that share them.
 */

/* Misc-object kinds named by the demo's PSX.SYM. Retail's AddMisc switch
 * also has three unnamed extension cases, 5-7. */
typedef enum MiscType MiscType;
enum MiscType
{
    MISC_FIRE = 0,
    MISC_DOOR = 1,
    MISC_PITFALL = 2,
    MISC_SNOWFALL = 3,
    MISC_SPRITE = 4,
    /* The last three are invented names (not in any psxsym source): 5
     * installs a scrolling texture, 6 is the stage bonfire (flickering
     * sprFrame sprite, rising embers, crackle loop), 7 a looping
     * positional sound. */
    MISC_TEXSCROLL = 5,
    MISC_BONFIRE = 6,
    MISC_SOUND = 7
};

/* Misc-object pool bound from MISC.C's anonymous enum. */
enum
{
    MaxMisc = 200
};

/* Retail table extents shared by InitMisc and the individual processors. */
enum
{
    N_DOOR_TYPES = 11,
    N_PITFALL_TYPES = 3,
    N_MISC_SPRITE_TYPES = 2
};

typedef struct tag_TMisc TMisc;

/* The three authored words carried by a construction-file effect record and
 * copied into a new misc slot before its kind-specific CREATE handler runs. */
typedef struct MiscSpawnParameters MiscSpawnParameters;
struct MiscSpawnParameters
{
    s32 a;
    s32 b;
    s32 c;
}; /* 0xC */

typedef u8 misc_pause_state;
enum misc_pause_state
{
    MISC_ACTIVE = 0,
    MISC_PAUSED = 1
};

typedef u8 door_mode;
enum door_mode
{
    DOOR_MODE_IDLE = 0,
    DOOR_MODE_OPENING = 1
};

typedef u8 pitfall_mode;
enum pitfall_mode
{
    PITFALL_MODE_CLOSED = 0,
    PITFALL_MODE_OPENING = 1,
    PITFALL_MODE_OPEN = 2
};

/* The MISC_SPRITE variant of the param union (MISC__181fake's `sprite`
 * member, union MISC__181fake in reference/psxsym-types.h) — a single byte
 * at the union's base offset, reused after CREATE clamps/narrows the raw
 * `init.a` read down to a valid sprite-table index. */
typedef struct TSprite
{
    u8 type; /* 0x0 */
} TSprite;

/* The MISC_DOOR variant of the parameter union (PSX.SYM's TDoor). */
typedef struct TDoor
{
    ModelType *locate; /* 0x0 */
    s16 r;             /* 0x4 */
    s16 dr;            /* 0x6 */
    u8 type;           /* 0x8 */
} TDoor;               /* 0xC */

/* The MISC_SNOWFALL variant of the param union (MISC__181fake's `snowfall`
 * member, reference/psxsym-types.h). */
typedef struct TSnowfall
{
    s32 w;          /* 0x0 */
    s32 h;          /* 0x4 */
    SVECTOR *snows; /* 0x8 */
} TSnowfall;

/* Door and pitfall rows share their authored orientation/type pair before
 * MM_CREATE replaces the payload with the live TDoor/TPitfall view. */
typedef struct MiscHingedModelInitParameters MiscHingedModelInitParameters;
struct MiscHingedModelInitParameters
{
    s32 rotation; /* 0x0 */
    s32 type;     /* 0x4 */
    s32 reserved; /* 0x8 */
}; /* 0xC */

/* MISC_FIRE's nonzero `a` selects the puff processor.  Once selected, its
 * other two authored parameters are the horizontal jitter radii. */
typedef struct MiscPuffParameters MiscPuffParameters;
struct MiscPuffParameters
{
    s32 selector; /* 0x0 */
    s32 x_radius; /* 0x4 */
    s32 z_radius; /* 0x8 */
}; /* 0xC */

/* The bonfire row uses only the first authored parameter, as its sprite
 * scale.  The tail remains part of the shared three-word payload. */
typedef struct MiscBonfireParameters MiscBonfireParameters;
struct MiscBonfireParameters
{
    s32 scale;       /* 0x0 */
    s32 reserved[2]; /* 0x4 */
}; /* 0xC */

/* A sound-emitter row arrives as three words.  MM_CREATE repacks those same
 * twelve bytes in place into the runtime deadline/range view below. */
typedef union MiscSoundIndexWord MiscSoundIndexWord;
union MiscSoundIndexWord
{
    s32 word;
    u8 index;
}; /* 0x4 */

typedef struct MiscSoundInitParameters MiscSoundInitParameters;
struct MiscSoundInitParameters
{
    MiscSoundIndexWord sound; /* 0x0 */
    s32 min_delay;            /* 0x4 */
    s32 max_delay;            /* 0x8 */
}; /* 0xC */

typedef struct MiscSoundSchedule MiscSoundSchedule;
struct MiscSoundSchedule
{
    s32 next;        /* 0x0 */
    s16 min_delay;   /* 0x4 */
    s16 max_delay;   /* 0x6 */
    u8 sound_index;  /* 0x8 */
}; /* 0xC */

/* Construction rows store an offset into the contiguous ambient-sound range
 * beginning at direct stage sound 0x44. */
enum
{
    MISC_SOUND_ID_BASE = 0x44
};

/* The MISC_PITFALL variant of the parameter union. */
typedef struct TPitfall
{
    ModelType *locate; /* 0x0 */
    s16 r;             /* 0x4 */
    u8 type;           /* 0x6 */
} TPitfall;

/* Misc-object lifecycle messages named by the demo's PSX.SYM. */
typedef enum TMiscMessage TMiscMessage;
enum TMiscMessage
{
    MM_CREATE = 0,
    MM_DESTROY = 1,
    MM_PAUSE = 2,
    MM_RESUME = 3,
    MM_DO = 4
};

/* The construction file supplies `init`; MM_CREATE then selects or builds the
 * view owned by the installed processor. */
typedef union MiscParameters MiscParameters;
union MiscParameters
{
    MiscSpawnParameters init;
    MiscHingedModelInitParameters hinged_init;
    TDoor door;
    TPitfall pitfall;
    TSnowfall snowfall;
    TSprite sprite;
    MiscPuffParameters puff;
    MiscBonfireParameters bonfire;
    MiscSoundInitParameters sound_init;
    MiscSoundSchedule sound;
}; /* 0xC */

struct tag_TMisc
{
    void (*proc)(TMisc *, TMiscMessage); /* 0x00 */
    s32 x;                               /* 0x04 */
    s32 y;                               /* 0x08 */
    s32 z;                               /* 0x0C */
    s32 count;                           /* 0x10 */
    misc_pause_state pause;              /* 0x14 */
    u8 mode;                             /* 0x15 (PSX.SYM's original type) */
    MiscParameters param;                /* 0x18 */
}; /* 0x24 */

/* InitMisc replaces each table's archive identity in place with the loaded
 * model pointer. MODEL_ARCHIVE_NONE leaves that half of the door absent. */
typedef union MiscModelReference MiscModelReference;
union MiscModelReference
{
    ModelArchiveId archive_id;
    ModelType *model;
}; /* 0x4 */

typedef union MiscSpriteReference MiscSpriteReference;
union MiscSpriteReference
{
    ImageArchiveId image_id;
    Sprite3D *sprite;
}; /* 0x4 */

typedef struct
{
    MiscModelReference Model[2]; /* 0x0 */
    s16 HitSize;                 /* 0x8 */
} DoorDataType;                  /* 0xC, MISC__183fake */

typedef struct
{
    MiscModelReference Model[2]; /* 0x0 */
    s16 HitSize;                 /* 0x8 */
} PitfallDataType;               /* 0xC, MISC__184fake */

typedef struct
{
    MiscSpriteReference spr; /* 0x0 */
    s32 scale;               /* 0x4 */
} SpriteDataType;            /* 0x8, MISC__185fake */

extern TMisc misc[MaxMisc];
extern DoorDataType DoorData[N_DOOR_TYPES];
/* Retail adds a third pitfall variant after the demo's recovered [2]. */
extern PitfallDataType PitfallData[N_PITFALL_TYPES];
extern SpriteDataType SpriteData[N_MISC_SPRITE_TYPES];

/* MISC.C's original file-static fInitial, qualified because ITEM.C has a
 * distinct same-named static. Set by InitMisc and checked by DoMiscProc. */
extern u8 Misc_fInitial;

void AddMisc(MiscType type, s32 x, s32 y, s32 z, s32 a, s32 b, s32 c);
extern void AdtMessageBox(char *fmt, ...);
extern int rand(void);

#endif
