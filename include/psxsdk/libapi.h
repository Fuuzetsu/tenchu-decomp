#ifndef PSXSDK_LIBAPI_H
#define PSXSDK_LIBAPI_H

#include <types.h>

typedef struct EXEC EXEC;
struct EXEC
{
    u_long pc0;
    u_long gp0;
    u_long t_addr;
    u_long t_size;
    u_long d_addr;
    u_long d_size;
    u_long b_addr;
    u_long b_size;
    u_long s_addr;
    u_long s_size;
    u_long sp;
    u_long fp;
    u_long gp;
    u_long ret;
    u_long base;
};

/* PSX.SYM retains this complete BIOS directory-entry layout (40 bytes). */
typedef struct DIRENTRY DIRENTRY;
struct DIRENTRY
{
    u_char name[20];
    long attr;
    long size;
    DIRENTRY *next;
    long head;
    u_char system[4];
};

long Exec(EXEC *exec, long argc, char **argv);
long InitPAD(char *buffer0, long length0, char *buffer1, long length1);
int EnterCriticalSection(void);
void ExitCriticalSection(void);
long Krom2RawAdd(u_long code);
long Krom2RawAdd2(u_short code);
void ChangeClearPAD(long mode);

void _new_card(void);
long _card_write(long channel, long block, unsigned char *buffer);
long _card_clear(long channel);

#endif
