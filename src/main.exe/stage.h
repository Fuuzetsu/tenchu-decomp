#ifndef STAGE_H
#define STAGE_H

/* Originally private to STAGE.C; shared here because that source file is
 * reconstructed as several translation units. Event and eTarget are the
 * exact names/types recovered from PSX.SYM. */
struct EventSeqType;
struct Humanoid;
extern struct EventSeqType *StageEvent;
extern struct EventSeqType *Event[2];
extern struct Humanoid *eTarget[2];

#endif
