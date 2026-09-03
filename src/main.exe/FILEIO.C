#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "filesystem.h"
#include "images.h"
#include "vmemory.h"
#include <psxsdk/libcd.h>
#include <psxsdk/libgpu.h>

/*
 * Retail FILEIO.C adds three helpers and rearranges the earlier demo
 * definitions. The manifest preserves both orders independently.
 */

extern char fmt_load_cd[];       /* "%$LOAD(CD)\n%d[%s]" */
extern char msg_load_cd_error[]; /* "LOAD(CD) ERROR\n%d[%s]" */
extern char msg_memory_load_is_disabled[]; /* *** memory load is disabled now *** */
extern char fmt_load_pc[];       /* "%$LOAD(PC)\n%d[%s]" */
extern char msg_load_pc_error[]; /* "LOAD(PC) ERROR\n%d[%s]" */
extern char path_tenchu_run_exe_1[]; /* \\TENCHU\\RUN.EXE;1 */
extern char fmt_concat_2[];          /* %s%s */
extern u8 str_acqurememorydisk[16];  /* "ACQUREMEMORYDISK" */
extern char path_tenchu_data[];      /* TENCHU\\DATA */
extern char path_demo_loading_tim[];  /* K:\WORK\CDIMAGE\DEMO\loading.tim */
extern char path_demo_load_ten_tim[]; /* K:\WORK\CDIMAGE\DEMO\load_ten.tim */

extern void VSyncCallback(void (*f)(void));
extern TAFSFileHandle *AfsOpen(TAFS *handle, char *path);
extern int AfsFileSize(TAFS *handle, TAFSFileHandle *fh);
extern u32 AfsRead(TAFS *volume, TAFSFileHandle *fd, void *buffer, u32 length);
extern int AfsClose(TAFSFileHandle *fd);
extern int AfsOpenVolume(TAFS *handle, char *path);
extern void AdtMessageBox(char *fmt, ...);
extern void CdaStop(void);
extern int PCopen(char *name, int mode, int share);
extern int PClseek(int fd, int offset, TSeekMode whence);
extern int PCread(int fd, void *buf, int size);
extern int PCclose(int fd);
extern int PCcreat(char *name, int mode);
extern int PCwrite(int fd, void *buf, int size);
extern void PCinit(void);
extern void cd_init(void);
extern void save_pad_analog_(void);
extern void SsEnd(void);
extern void SsQuit(void);
extern void PadStopCom(void);
extern void MemCardStop(void);
extern void MemCardEnd(void);
extern void StopCallback(void);
extern void set_boot_exec_(u8 *file, u32 stack, u32 size);
extern void run_exec_file(u8 *name, u32 stack, u32 size);

extern u_long *LoadFromMEMORY(u8 *filename);
extern u_long *LoadFromDEVPC(u8 *filename);
extern u_long *LoadFromCDROM(u8 *filename);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void cbAccess(void);
 *     FILEIO.C:115, 27 src lines, frame 240 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_GT4 AccessImage;
 *     extern int AccessPower;
 * END PSX.SYM */

static void cbAccess(void)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    u32 intensity;

    intensity = (AccessPower + 8) & 0xff;
    AccessPower = intensity;
    AccessImage.r0 = intensity;
    AccessImage.g0 = AccessImage.r0;
    AccessImage.b0 = 0xff - AccessImage.r0;
    AccessImage.r1 = AccessImage.r0;
    AccessImage.g1 = AccessImage.b0;
    AccessImage.b1 = AccessImage.r0;
    AccessImage.r2 = AccessImage.b0;
    AccessImage.g2 = AccessImage.r0;
    AccessImage.b2 = AccessImage.r0;
    AccessImage.r3 = AccessImage.r0;
    AccessImage.g3 = AccessImage.r0;
    AccessImage.b3 = AccessImage.r0;
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * FileRead(unsigned char *filename);
 *     FILEIO.C:156, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       unsigned char * filename
 *     reg   $s0       unsigned long * ret
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 *     extern int ReadMode;
 *     extern int TotalIO;
 * END PSX.SYM */

u_long *FileRead(u8 *filename)
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
    case READ_SOURCE_MEMORY:
        ret = LoadFromMEMORY(filename);
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

    tim = FileRead(path_demo_loading_tim);
    GetTIMInfo(tim, &img);
    LoadTIMAndFree(tim);
    SetupImageToPolyFT4(&img, &poly1, 0xD4, 0xDE);
    tim = FileRead(path_demo_load_ten_tim);
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitFileSystem(int mode);
 *     FILEIO.C:62, 40 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int mode
 *
 * Globals it touches, as the original declared them:
 *     extern int ReadMode;
 *     extern int TotalIO;
 *     extern unsigned long *virtual_memory_pool;
 *     extern struct MemoryDiskType *MDfat;
 *     extern struct TAFS systemAFS;
 * END PSX.SYM */

void InitFileSystem(file_read_mode mode)
{
    u_long *saved_pool;

    ReadMode = mode;
    mode = mode & READ_SOURCE_MASK;
    TotalIO = 0;
    switch (mode)
    {
    case READ_SOURCE_DEVPC:
        PCinit();
        break;
    case READ_SOURCE_MEMORY:
        PCinit();
        if (strncmp((char *)TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS,
                    (char *)str_acqurememorydisk,
                    TENCHU_PC_MEMORY_HANDSHAKE_SIZE) != 0)
        {
            vinit(0, 0);
            __builtin_memcpy((void *)TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS,
                             str_acqurememorydisk, sizeof(str_acqurememorydisk));
            ReadMode |= READ_MODE_MEMORY_DISK_MASK;
        }
        if (ReadMode & READ_MODE_MEMORY_DISK_MASK)
        {
            saved_pool = virtual_memory_pool;
            vinit((void *)TENCHU_PC_MEMORY_POOL_ADDRESS,
                  TENCHU_PC_MEMORY_POOL_SIZE);
            vcalloc(MEMORY_DISK_SCRATCH_SIZE, 0);
            virtual_memory_pool = saved_pool;
        }
        MDfat = (MemoryDiskType *)TENCHU_PC_MEMORY_PAYLOAD_ADDRESS;
        break;
    case READ_SOURCE_CDROM:
        CdInit();
        cd_init();
        AfsOpenVolume(&systemAFS, path_tenchu_data);
        break;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * PathFileRead(unsigned char *path, unsigned char *name);
 *     FILEIO.C:185, 8 src lines, frame 280 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * path
 *     param $a1       unsigned char * name
 *     stack sp+16     unsigned char [256] filename
 * END PSX.SYM */

u_long *PathFileRead(u8 *path, u8 *name)
{
    u8 filename[256];

    sprintf((char *)filename, fmt_concat_2, path, name);
    return FileRead(filename);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int FileWrite(unsigned char *filename, void *data, long size);
 *     FILEIO.C:197, 13 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * filename
 *     param $a1       void * data
 *     param $a2       long size
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void LoadExecEx(unsigned char *file, unsigned long stack, unsigned long size);
 *     INFOVIEW.C:1173, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * file
 *     param $a1       unsigned long stack
 *     param $a2       unsigned long size
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitAccessInfo(void);
 *     FILEIO.C:106, 4 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_GT4 AccessImage;
 *     extern int AccessPower;
 * END PSX.SYM */

void InitAccessInfo(void)
{
    SetupImageToPolyGT4(GetImage(IMG_LOADING), &AccessImage, 0xd6, 0xd9);
    AccessPower = 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromDEVPC(unsigned char *filename);
 *     FILEIO.C:214, 29 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * filename
 *
 * Globals it touches, as the original declared them:
 *     extern int TotalIO;
 *     extern int ReadMode;
 *     extern unsigned long *MemoryLoadAddress;
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromMEMORY(unsigned char *filename);
 *     FILEIO.C:247, 45 src lines, frame 64 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       unsigned char * filename
 *     reg   $s1       unsigned long * vmp
 *     reg   $s2       unsigned long * data
 *     stack sp+16     unsigned char [24] name
 *     reg   $s0       short i
 *     reg   $a2       short j
 *     reg   $s4       unsigned char * filename
 *     reg   $s2       int fd
 *     reg   $s3       int size
 *     reg   $s1       unsigned long * data
 * END PSX.SYM */

u_long *LoadFromMEMORY(u8 *filename)
{
    AdtMessageBox(msg_memory_load_is_disabled);
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromCDROM(unsigned char *filename);
 *     FILEIO.C:296, 31 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * filename
 *
 * Globals it touches, as the original declared them:
 *     extern int TotalIO;
 *     extern struct TAFS systemAFS;
 *     extern int ReadMode;
 *     extern unsigned long *MemoryLoadAddress;
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PrepareAccess(void);
 *     FILEIO.C:144, 8 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 * END PSX.SYM */

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
