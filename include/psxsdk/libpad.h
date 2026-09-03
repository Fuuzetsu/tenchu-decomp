#ifndef PSXSDK_LIBPAD_H
#define PSXSDK_LIBPAD_H

void PadInitDirect(unsigned char *buffer0, unsigned char *buffer1);
void PadStartCom(void);
void PadStopCom(void);
int PadGetState(int port);
int PadInfoMode(int port, int mode, int index);
int PadSetAct(int port, unsigned char *actuators, int length);
int PadSetActAlign(int port, unsigned char *alignment);
int PadSetMainMode(int port, int mode, int lock);

#endif
