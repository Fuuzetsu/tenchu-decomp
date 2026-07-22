#ifndef TENCHU_PADCMD_H
#define TENCHU_PADCMD_H

struct PADtype;

extern void GetPadXY(short no, short *x, short *y);
extern short GetPad(short no);
extern short GetCommand(struct PADtype *pad);
extern short SetCommand(struct PADtype *pad, short cmd);

#endif
