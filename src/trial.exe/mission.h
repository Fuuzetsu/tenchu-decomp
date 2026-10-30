#ifndef TENCHU_TRIAL_MISSION_H
#define TENCHU_TRIAL_MISSION_H

/* Card I/O fixes these serialized boundaries. The map and placement records
 * still need their individual fields recovered from the editor's consumers. */
enum
{
    TRIAL_MISSION_METADATA_SIZE = 0x48,
    TRIAL_MISSION_HEADER_SIZE = 0xec,
    TRIAL_MISSION_MAP_SIZE = 0x800,
    TRIAL_MISSION_LAYOUT_SIZE = 0x1450,
    TRIAL_MISSION_SIZE = 0x1d3c,
    TRIAL_MISSION_RECORD_COUNT = 10,
    TRIAL_MISSION_CHECKSUM_MASK = 0x07ffffff,
    TRIAL_MISSION_PRESERVED_CHECKSUM_BIT = 0x08000000
};

/* The low three bits choose the objective; bit 3 forbids being spotted. */
enum
{
    TRIAL_REACH_LORD = 0,
    TRIAL_ASSASSINATE = 1,
    TRIAL_ELIMINATE_ENEMIES = 2,
    TRIAL_PROTECT_PRINCESS = 3,
    TRIAL_FIND_HERB = 4,
    TRIAL_OBJECTIVE_MASK = 7,
    TRIAL_REQUIRE_STEALTH = 8
};

/* Ten name bytes occupy a word-aligned field; the two following record
 * padding bytes are deliberately left untouched by leaderboard insertion. */
typedef struct TrialRecordName
{
    char text[10];
} TrialRecordName __attribute__((aligned(4)));

typedef struct TrialMissionRecord TrialMissionRecord;
struct TrialMissionRecord
{
    s32 time;              /* completion time in 30 Hz ticks; -1 is empty */
    TrialRecordName name;
    /* Two alignment bytes are not copied when records are shifted. */
}; /* 0x10; lower times rank first. */

typedef struct TrialMissionHeader TrialMissionHeader;
struct TrialMissionHeader
{
    u32 checksum;           /* 0x00 */
    u16 flags;              /* 0x04: objective in bits 0..2; stealth in bit 3 */
    s16 theme;              /* 0x06: shared stage assets and card icon */
    char title[32];         /* 0x08 */
    char author[16];        /* 0x28 */
    u16 unknown_setting;    /* 0x38: meaning still under review */
    char password[18];      /* 0x3a */
    TrialMissionRecord records[TRIAL_MISSION_RECORD_COUNT]; /* 0x4c */
}; /* 0xec */

typedef struct TrialMissionData TrialMissionData;
struct TrialMissionData
{
    TrialMissionHeader header;
    u8 map_data[TRIAL_MISSION_MAP_SIZE];       /* 0x00ec */
    u8 layout_data[TRIAL_MISSION_LAYOUT_SIZE]; /* 0x08ec */
}; /* 0x1d3c */

extern TrialMissionData TrialMission;

#endif
