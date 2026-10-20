#ifndef PSXSDK_LIBPRESS_H
#define PSXSDK_LIBPRESS_H

#include <types.h>

typedef unsigned short DECDCTTAB[34816];
typedef void (*DecDCTCallback)(void);

void DecDCTReset(int mode);
void DecDCTin(u_long *buffer, int mode);
void DecDCTout(u_long *buffer, int words);
int DecDCToutCallback(DecDCTCallback callback);
int DecDCTvlc2(u_long *input, u_long *output, DECDCTTAB table);
void DecDCTvlcBuild(u_short *table);

#endif
