#include "common.h"
#include "menu.exe.h"
#include <psxsdk/kernel.h>
#include <psxsdk/libsn.h>

#define BOOT_EXEC_MAGIC 0xDEF0C0DEu

enum
{
    BOOT_EXEC_CDROM_PREFIX_SIZE = 6,
    BOOT_EXEC_NAME_CAPACITY = 0x50,
    EXEC_HEADER_BYTES = 2048
};

typedef struct
{
    u32 magic;
    char name[BOOT_EXEC_NAME_CAPACITY];
    u32 s_addr;
    u32 s_size;
} BootExecRecord;

extern EXEC MenuHostExec;
extern char fmt_menu_exec[];
extern char fmt_host_exec_header[];
extern char fmt_host_exec_text[];
extern char msg_host_exec_read_ok[];

void run_exec_file(u8 *name, u32 stack, u32 size)
{
    EXEC *exec;

    do
    {
        exec = CdReadExec(name);
    } while (CdReadSync(0, NULL) != 0);
    printf(fmt_menu_exec, exec);
    exec->s_addr = stack;
    exec->s_size = size;
    Exec(exec, 0, NULL);
}

void set_boot_exec_(u8 *file, u32 stack, u32 size)
{
    int i;
    u32 magic;
    BootExecRecord *rec;

    magic = BOOT_EXEC_MAGIC;
    rec = (BootExecRecord *)TENCHU_EXECUTABLE_HANDOFF_ADDRESS;
    rec->magic = magic;
    file += BOOT_EXEC_CDROM_PREFIX_SIZE;
    for (i = 0; file[i] != 0; i++)
        rec->name[i] = file[i];
    rec->name[i] = 0;
    rec->s_addr = stack;
    rec->s_size = size;
}

EXEC *LoadExecFromHost(char *name)
{
    u_long buffer[EXEC_HEADER_BYTES / sizeof(u_long)];
    struct XF_HDR *xf;
    int fd;

    fd = PCopen(name, 0, 0);
    if (fd == -1)
    {
        return NULL;
    }
    PCread(fd, buffer, EXEC_HEADER_BYTES);
    xf = (struct XF_HDR *)buffer;
    printf(fmt_host_exec_header, xf);
    printf(fmt_host_exec_text, xf->exec.t_addr, xf->exec.t_size);
    PCread(fd, (void *)xf->exec.t_addr, xf->exec.t_size);
    printf(msg_host_exec_read_ok);
    MenuHostExec = xf->exec;
    return &MenuHostExec;
}

/* MENU string storage. Keep the retail spelling and development paths. */
char fmt_host_exec_header[] = "xf = %x\n";
char fmt_host_exec_text[] = "t_addr = %x, t_size = %x\n";
char msg_host_exec_read_ok[] = "read ok\n";
char fmt_menu_exec[] = "exec = %x\n";
