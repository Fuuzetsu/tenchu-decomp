#ifndef STAGE_H
#define STAGE_H

/* Originally private to STAGE.C; shared here because that source file is
 * reconstructed as several translation units. Event and eTarget are the
 * exact names/types recovered from PSX.SYM. */
struct EventSeqType;

/* EventSeqType.mode — the stage-event trigger kinds StageSequence
 * dispatches on (invented names, read off each case): always, target
 * inside a km-grid zone, attribute mask set, status equals, motion
 * equals, life at or below, player within 2000 of the target, stage
 * time reached, and play-music-now. */
enum event_trigger_kind
{
    EVTRIG_ALWAYS = 0,
    EVTRIG_ZONE = 1,
    EVTRIG_ATTRIBUTE = 2,
    EVTRIG_STATUS = 3,
    EVTRIG_MOTION = 4,
    EVTRIG_LIFE = 5,
    EVTRIG_NEAR = 6,
    EVTRIG_TIME = 7,
    EVTRIG_MUSIC = 8
};

/* Two event sequences run in parallel. Event IDs 2 and 3 are their root
 * scripts and continue even while the player is dead. */
enum stage_event_slot
{
    STAGE_EVENT_PRIMARY = 0,
    STAGE_EVENT_SECONDARY = 1,
    N_STAGE_EVENT_SLOTS = 2
};

enum
{
    EVENT_ID_STAGE_START = 0,
    EVENT_ROOT_FIRST = 2,
    EVENT_ROOT_LAST = EVENT_ROOT_FIRST + N_STAGE_EVENT_SLOTS - 1,
    EVENT_RIKIMARU_FINALE = 100
};

struct Humanoid;
extern struct EventSeqType *StageEvent;
extern struct EventSeqType *Event[N_STAGE_EVENT_SLOTS];
extern struct Humanoid *eTarget[N_STAGE_EVENT_SLOTS];

extern void StartStageSequence(void);
extern s32 StageSequence(void);
extern void UpdateEvent(short slot, short event_id);
extern void SetupStageSequence(void);

#endif
