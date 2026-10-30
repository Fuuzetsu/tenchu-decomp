#include "common.h"
#include "trial.exe.h"
#include "../main.exe/tim.h"
#include "../main.exe/adt.h"
#include "../main.exe/filesystem.h"
#include "../main.exe/images.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libgpu.h>
#include <psxsdk/libmcrd.h>
#include <psxsdk/libpad.h>
#include <psxsdk/libsnd.h>


/* TRIAL supports CD and development-PC reads. The loading splash and
 * path wrapper inline the same read operation used by FileRead. */

extern char fmt_load_cd[];       /* "%$LOAD(CD)\n%d[%s]" */
extern char msg_load_cd_error[]; /* "LOAD(CD) ERROR\n%d[%s]" */
extern char fmt_load_pc[];       /* "%$LOAD(PC)\n%d[%s]" */
extern char msg_load_pc_error[]; /* "LOAD(PC) ERROR\n%d[%s]" */
extern char path_tenchu_run_exe_1[]; /* \\TENCHU\\RUN.EXE;1 */
extern char fmt_concat_2[];          /* %s%s */
extern char path_tenchu_data[];      /* TENCHU\\DATA */
extern char path_demo_loading_tim[];  /* K:\WORK\CDIMAGE\DEMO\loading.tim */
extern char path_demo_load_ten_tim[]; /* K:\WORK\CDIMAGE\DEMO\load_ten.tim */


static void cbAccess(void);

void stop_access_meter_(void)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;

    VSyncCallback(0);
    if (AccessPower >= 0)
    {
        AccessImage.r0 = 0;
        AccessImage.g0 = 0;
        AccessImage.b0 = 0;
        AccessImage.r1 = 0;
        AccessImage.g1 = 0;
        AccessImage.b1 = 0;
        AccessImage.r2 = 0;
        AccessImage.g2 = 0;
        AccessImage.b2 = 0;
        AccessImage.r3 = 0;
        AccessImage.g3 = 0;
        AccessImage.b3 = 0;
        GetDrawEnv(&o_draw);
        if (AccessPower != 0)
            GetDispEnv(&o_disp);
        else
            GetDispEnv(&o_disp);
        n_draw = o_draw;
        n_draw.clip = o_disp.disp;
        n_draw.ofs[0] = o_disp.disp.x;
        n_draw.ofs[1] = o_disp.disp.y;
        PutDrawEnv(&n_draw);
        DrawPrim((u8 *)&AccessImage);
        PutDrawEnv(&o_draw);
    }
}

static __inline__ u_long *ReadFileData(u8 *filename)
{
    u_long *ret;

    CdaStop();
    if (AccessPower >= 0)
    {
        AccessPower = 0;
        VSyncCallback(cbAccess);
    }
    else
    {
        VSyncCallback(0);
    }
    if (ReadMode == READ_MODE_UNINITIALIZED)
    {
        TotalIO = 0;
        ReadMode = READ_SOURCE_DEVPC;
        PCinit();
    }
    switch (ReadMode & READ_SOURCE_MASK)
    {
    case READ_SOURCE_DEVPC:
        ret = LoadFromDEVPC(filename);
        break;
    case READ_SOURCE_CDROM:
        ret = LoadFromCDROM(filename);
        break;
    default:
        ret = 0;
        break;
    }
    stop_access_meter_();
    return ret;
}

void draw_loading_splash_(void)
{
    u_long *tim;
    GsIMAGE img;
    POLY_FT4 poly1;
    POLY_FT4 poly2;
    DISPENV disp;
    DRAWENV draw;
    DRAWENV draw2;

    tim = ReadFileData(path_demo_loading_tim);
    GetTIMInfo(tim, &img);
    LoadTIMAndFree(tim);
    SetupImageToPolyFT4(&img, &poly1, 0xD4, 0xDE);
    tim = ReadFileData(path_demo_load_ten_tim);
    GetTIMInfo(tim, &img);
    LoadTIMAndFree(tim);
    SetupImageToPolyFT4(&img, &poly2, 0xD4, 0xC0);
    GetDrawEnv(&draw);
    GetDispEnv(&disp);
    draw2 = draw;
    draw2.clip = disp.disp;
    draw2.ofs[0] = disp.disp.x;
    draw2.ofs[1] = disp.disp.y;
    PutDrawEnv(&draw2);
    DrawPrim((u8 *)&poly1);
    DrawPrim((u8 *)&poly2);
    DrawSync(0);
    PutDrawEnv(&draw);
}

void InitFileSystem(file_read_mode mode)
{
    ReadMode = mode;
    mode = mode & READ_SOURCE_MASK;
    TotalIO = 0;
    switch (mode)
    {
    case READ_SOURCE_DEVPC:
        PCinit();
        break;
    case READ_SOURCE_CDROM:
        CdInit();
        cd_init();
        AfsOpenVolume(&systemAFS, path_tenchu_data);
        break;
    }
}

u_long *FileRead(u8 *filename)
{
    return ReadFileData(filename);
}

u_long *PathFileRead(u8 *path, u8 *name)
{
    u8 filename[256];

    sprintf((char *)filename, fmt_concat_2, path, name);
    return ReadFileData(filename);
}

int FileWrite(unsigned char *filename, void *data, long size)
{
    long fd;

    if ((data == 0) | (size < 1))
        return 0;
    fd = PCcreat((char *)filename, 0);
    if (fd == -1)
        return 0;
    PCwrite(fd, data, size);
    PCclose(fd);
    return 1;
}

void LoadExecEx(u8 *file, u32 stack, u32 size)
{
    save_pad_analog_();
    CdaStop();
    SsEnd();
    SsQuit();
    ResetGraph(3);
    PadStopCom();
    MemCardStop();
    MemCardEnd();
    StopCallback();
    set_boot_exec_(file, stack, size);
    CdInit();
    run_exec_file(path_tenchu_run_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
}

void InitAccessInfo(void)
{
    AccessPower = 0;
}

u_long *LoadFromDEVPC(u8 *filename)
{
    s32 fd;
    s32 size;
    u_long *buff;

    TotalIO++;
    fd = PCopen((char *)filename, 0, 0);
    if (fd != -1)
    {
        size = PClseek(fd, 0, CDSEEK_END);
        if (size > 0)
        {
            if (ReadMode & READ_MODE_TRACE)
            {
                AdtMessageBox(fmt_load_pc, TotalIO, filename);
            }
            PClseek(fd, 0, CDSEEK_SET);
            if (MemoryLoadAddress == 0)
            {
                buff = (u_long *)valloc(size);
            }
            else
            {
                buff = MemoryLoadAddress;
                MemoryLoadAddress = 0;
            }
            PCread(fd, buff, size);
            PCclose(fd);
            return buff;
        }
    }
    AdtMessageBox(msg_load_pc_error, TotalIO, filename);
    return 0;
}

u_long *LoadFromCDROM(u8 *filename)
{
    AdtQuietMode quiet;
    TAFSFileHandle *fd;
    s32 size;
    u_long *buff;

    TotalIO++;
    quiet = AdtQuiet(ADT_NORMAL);
    fd = AfsOpen(&systemAFS, (char *)filename);
    if (fd != 0)
    {
        if (ReadMode & READ_MODE_TRACE)
        {
            AdtMessageBox(fmt_load_cd, TotalIO, filename);
        }
        size = AfsFileSize(&systemAFS, fd);
        if (MemoryLoadAddress == 0)
        {
            buff = (u_long *)valloc(size);
        }
        else
        {
            buff = MemoryLoadAddress;
            MemoryLoadAddress = 0;
        }
        AfsRead(&systemAFS, fd, buff, size);
        AfsClose(fd);
        AdtQuiet(quiet);
        return buff;
    }
    AdtQuiet(quiet);
    AdtMessageBox(msg_load_cd_error, TotalIO, filename);
    return 0;
}

/* TRIAL retains callback registration, but does not animate the meter. */
static void cbAccess(void)
{
}

void PrepareAccess(void)
{
    if (AccessPower >= 0)
    {
        AccessPower = 0;
        VSyncCallback(cbAccess);
    }
    else
    {
        VSyncCallback(0);
    }
}
