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
enum
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
struct Humanoid;
extern struct EventSeqType *StageEvent;
extern struct EventSeqType *Event[2];
extern struct Humanoid *eTarget[2];

#endif
