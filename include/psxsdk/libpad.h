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

void PAD_init(char *buffer0, long length0, char *buffer1, long length1);
void InitPAD2(char *buffer0, long length0, char *buffer1, long length1);
void PAD_init2(char *buffer0, long length0, char *buffer1, long length1);
void kernel_start_pad_(void);
void _patch_pad(void);
void _remove_ChgclrPAD(void);
extern int PadInitFlag;

#endif
