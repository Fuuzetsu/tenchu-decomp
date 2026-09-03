#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern void AfsInit(TAFS *handle);
extern char *strcat(char *dst, const char *src);
/* Retail calls the one-argument FILE * definition through this stale
 * two-argument int declaration. */
extern int cd_open(char *name, int mode);
extern int AfsGetHeader(TAFS *handle);
extern int AfsGetEntry(TAFS *handle);
extern void AdtMessageBox(char *fmt, ...);
extern char str_ext_vol[];                    /* .VOL */
extern char msg_afsvolume_open_err[];         /* "AfsOpenVolume: %s open err\n" */
extern char msg_afsopenvolume_header_error[]; /* AfsOpenVolume: Header error */
extern char msg_afsopenvolume_entry_error[];  /* AfsOpenVolume: Entry error */

int AfsOpenVolume(TAFS *handle, char *path)
{
    char buf[80];

    AfsInit(handle);
    if (strlen(path) >= 0x4C)
    {
        return 1;
    }
    strcpy(buf, path);
    strcat(buf, str_ext_vol);
    handle->fpVol = (FILE *)cd_open(buf, 0);
    if (handle->fpVol == 0)
    {
        AdtMessageBox(msg_afsvolume_open_err, buf);
        return 1;
    }
    if (AfsGetHeader(handle) != 0)
    {
        AdtMessageBox(msg_afsopenvolume_header_error);
        return 2;
    }
    if (AfsGetEntry(handle) != 0)
    {
        AdtMessageBox(msg_afsopenvolume_entry_error);
        return 3;
    }
    return 0;
}
