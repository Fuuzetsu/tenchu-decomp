#ifndef _LIBETC_H_
#define _LIBETC_H_

/* PsyQ libetc.h controller-button masks (the PadRead()-format word).
 *
 * Tenchu's own pad layer stores this exact layout: ComPad.c composes
 * PadPort.button as ~(rxbuf[3] | (rxbuf[2] << 8)), which puts the
 * select/start/D-pad report byte in the high half and the shoulder/shape
 * byte in the low half — the same bit assignment PadRead() returns, so
 * the SDK's own names apply to every pad word in the game (PadPort.button,
 * PADtype press/trig, dtPAD, and the AI's virtual pad commands).
 *
 * Verified against retail behaviour: PADLup drives forward movement,
 * PADLleft/PADLright turn, PADRleft (Square) attacks, PADRdown (Cross)
 * jumps, PADRup (Triangle) uses the item, PADL1 centers the camera,
 * PADstart pauses/advances menus. */
#define PADLup (1 << 12)
#define PADLdown (1 << 14)
#define PADLleft (1 << 15)
#define PADLright (1 << 13)
#define PADRup (1 << 4)    /* Triangle */
#define PADRdown (1 << 6)  /* Cross */
#define PADRleft (1 << 7)  /* Square */
#define PADRright (1 << 5) /* Circle */
#define PADi (1 << 9)
#define PADj (1 << 10)
#define PADk (1 << 8)
#define PADl (1 << 3)
#define PADm (1 << 1)
#define PADn (1 << 2)
#define PADo (1 << 0)
#define PADh (1 << 11)
#define PADL1 PADn
#define PADL2 PADo
#define PADR1 PADl
#define PADR2 PADm
#define PADstart PADh
#define PADselect PADk

int CheckCallback(void);
void PadInit(int mode);
u_long PadRead(int id);
void PadStop(void);
int ResetCallback(void);
int RestartCallback(void);
int StopCallback(void);
int VSync(int mode);
int VSyncCallback(void (*callback)(void));
int VSyncCallbacks(int channel, void (*callback)(void));
long GetVideoMode(void);
long SetVideoMode(long mode);

#endif
