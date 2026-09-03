#ifndef PSXSDK_LIBSND_H
#define PSXSDK_LIBSND_H

/* Official libsnd.h attribute constants (the subset the game uses). */
#define SS_SOFF 0
#define SS_SON 1
#define SS_MIX 0
#define SS_REV 1
#define SS_SERIAL_A 0
#define SS_SERIAL_B 1

void SsVabClose(short vab_id);
short SsVabTransCompleted(short mode);
short SsVabOpenHead(unsigned char *header, short vab_id);
short SsVabTransBody(unsigned char *body, short vab_id);

void SsInit(void);
void SsQuit(void);
void SsSetTickMode(long mode);
void SsStart(void);
void SsEnd(void);

void SsSetMVol(short volume_left, short volume_right);
void SsSetSerialAttr(char channel, char attribute, char mode);
void SsSetSerialVol(char channel, short volume_left, short volume_right);
void SsSetMono(void);
void SsSetStereo(void);

short SsUtKeyOnV(short voice, short vab_id, short program, short tone,
                  short note, short fine, short volume_left,
                  short volume_right);
short SsUtAutoPan(short voice, short start, short end, short duration);
void SsUtAllKeyOff(short mode);

#endif
