#include "common.h"
#include "frontend.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libpress.h>
#include <psxsdk/libsnd.h>

/* The demo's OPMOVIE.C symbols preserve DECENV and the helper names.
 * Retail adds the RGB24 argument and emits the callback before open_stream. */
typedef struct DECENV
{
    u_long *vlcbuf[2];
    int vlcid;
    u_short *imgbuf[2];
    int imgid;
    RECT rect;
    int isAbsoluteRect;
    RECT slice;
    int isdone;
    int is24bit;
} DECENV;

enum
{
    MOVIE_RING_SECTORS = 32,
    MOVIE_SECTOR_BYTES = 2048,
    MOVIE_VLC_BYTES = 0x28000,
    MOVIE_IMAGE_BYTES = 0x7800,
    MOVIE_DECODE_TIMEOUT = 0x800000,
    MOVIE_MACROBLOCK_SIZE = 16
};

extern DECENV MovieDec;
extern DECDCTTAB MovieVlcTable;
extern int MovieFrame;
extern int MovieReadMode;
extern int MovieFirstSlice;
extern int MovieWidth;
extern int MovieHeight;
extern u_long *MovieRing;
extern u_long *vlcbuf0;
extern u_long *vlcbuf1;
extern u_short *imgbuf0;
extern u_short *imgbuf1;
extern char str_movie_file_version[];
extern char msg_movie_decode_timeout[];

void strCallback(void);
u_long *strNext(DECENV *dec);
int strSetDefDecEnv(DECENV *dec, int x, int y, short rgb24);
int strInit(CdlLOC *loc, void (*callback)(void));
int strNextVlc(DECENV *dec);
void strSync(DECENV *dec, int mode);
void strKickCD(CdlLOC *loc);

int DecodeMovieFrame(void)
{
    DecDCTin(MovieDec.vlcbuf[MovieDec.vlcid], MovieDec.is24bit ? 3 : 2);
    DecDCTout((u_long *)MovieDec.imgbuf[MovieDec.imgid],
              MovieDec.slice.w *
              (((MovieDec.slice.h - 1) / MOVIE_MACROBLOCK_SIZE + 1) *
               MOVIE_MACROBLOCK_SIZE) / 2);
    strNextVlc(&MovieDec);
    strSync(&MovieDec, 0);
    return MovieFrame;
}

void strCallback(void)
{
    int id;
    RECT snap_rect;

    if (MovieDec.is24bit && StCdIntrFlag)
    {
        StCdInterrupt();
        StCdIntrFlag = 0;
    }
    id = MovieDec.imgid;
    snap_rect = MovieDec.slice;
    MovieDec.imgid = MovieDec.imgid ? 0 : 1;
    if (MovieFirstSlice && MovieDec.rect.w % MovieDec.slice.w)
    {
        MovieDec.slice.x += MovieDec.rect.w % MovieDec.slice.w;
        MovieFirstSlice = 0;
    }
    else
    {
        MovieDec.slice.x += MovieDec.slice.w;
    }
    if (MovieDec.slice.x < MovieDec.rect.x + MovieDec.rect.w)
    {
        DecDCTout((u_long *)MovieDec.imgbuf[MovieDec.imgid],
                  MovieDec.slice.w *
                  (((MovieDec.slice.h - 1) / MOVIE_MACROBLOCK_SIZE + 1) *
                   MOVIE_MACROBLOCK_SIZE) / 2);
    }
    else
    {
        MovieDec.isdone = 1;
        MovieFirstSlice = 1;
        MovieDec.slice.x = MovieDec.rect.x;
        MovieDec.slice.y = MovieDec.rect.y;
    }
    {
        DRAWENV draw;

        GetDrawEnv(&draw);
        snap_rect.x += draw.ofs[0];
        snap_rect.y += draw.ofs[1];
    }
    LoadImage(&snap_rect, (u_long *)MovieDec.imgbuf[id]);
}

u_long *strNext(DECENV *dec)
{
    u_long *addr;
    StHEADER *sector;
    int cnt;

    cnt = MOVIE_DECODE_TIMEOUT;
    while (StGetNext(&addr, (u_long **)&sector))
    {
        if (--cnt == 0)
        {
            return 0;
        }
    }
    MovieFrame = sector->frameCount;
    if (MovieWidth != sector->width || MovieHeight != sector->height)
    {
        RECT rect;

        setRECT(&rect, 0, 0,
                dec->is24bit ? MovieWidth * 3 / 2 : MovieWidth,
                MovieHeight * 2);
        if (dec->is24bit)
        {
            ClearImage(&rect, 0, 0, 0);
        }
        else
        {
            ClearImage(&rect, 64, 64, 64);
        }
        MovieWidth = sector->width;
        MovieHeight = sector->height;
    }
    dec->rect.w = dec->is24bit ? MovieWidth * 3 / 2 : MovieWidth;
    dec->rect.h = MovieHeight;
    dec->slice.h = MovieHeight;
    return addr;
}

int open_stream(char *path, int x, int y, int read_mode, short rgb24)
{
    char str[128];
    CdlFILE file;

    MovieFirstSlice = 1;
    MovieRing = valloc(MOVIE_RING_SECTORS * MOVIE_SECTOR_BYTES);
    vlcbuf0 = valloc(MOVIE_VLC_BYTES);
    vlcbuf1 = valloc(MOVIE_VLC_BYTES);
    imgbuf0 = valloc(MOVIE_IMAGE_BYTES);
    imgbuf1 = valloc(MOVIE_IMAGE_BYTES);
    strcpy(str, path);
    strcat(str, str_movie_file_version);
    while (CdSearchFile(&file, str) == 0)
    {
    }
    MovieReadMode = read_mode;
    strSetDefDecEnv(&MovieDec, x, y, rgb24);
    strInit(&file.pos, strCallback);
    DecDCTvlcBuild(MovieVlcTable);
    SsSetSerialVol(SS_SERIAL_A, gSoundLevel, gSoundLevel);
    strNextVlc(&MovieDec);
    return 0;
}

void SyncMovieFrame(void)
{
    strSync(&MovieDec, 0);
}

void close_stream(void)
{
    u_char param[8];

    DecDCToutCallback(0);
    StUnSetRing();
    CdControlB(CdlPause, 0, 0);
    ChangeClearPAD(0);
    vfree(MovieRing);
    vfree(vlcbuf0);
    vfree(vlcbuf1);
    vfree(imgbuf0);
    vfree(imgbuf1);
}

int strSetDefDecEnv(DECENV *dec, int x, int y, short rgb24)
{
    short slice_width;

    dec->is24bit = rgb24;
    dec->vlcid = 0;
    dec->imgid = 0;
    dec->isdone = 0;
    dec->rect.x = x;
    dec->rect.y = y;
    dec->vlcbuf[0] = vlcbuf0;
    dec->vlcbuf[1] = vlcbuf1;
    dec->imgbuf[0] = imgbuf0;
    dec->imgbuf[1] = imgbuf1;
    slice_width = 16;
    if (dec->is24bit)
    {
        slice_width = 24;
    }
    dec->slice.w = slice_width;
    dec->slice.x = x;
    dec->slice.y = y;
    return 0;
}

int strInit(CdlLOC *loc, void (*callback)(void))
{
    DecDCTReset(0);
    DecDCToutCallback(callback);
    StSetRing(MovieRing, MOVIE_RING_SECTORS);
    StSetStream(MovieDec.is24bit, 0, -1, 0, 0);
    strKickCD(loc);
    return 0;
}

int strNextVlc(DECENV *dec)
{
    int cnt = MOVIE_DECODE_TIMEOUT;
    u_long *addr;

    while ((addr = strNext(dec)) == 0)
    {
        if (--cnt == 0)
            return -1;
    }
    dec->vlcid = !dec->vlcid;
    DecDCTvlc2(addr, dec->vlcbuf[dec->vlcid], MovieVlcTable);
    StFreeRing(addr);
    return 0;
}

void strSync(DECENV *dec, int mode)
{
    volatile u_long cnt;

    cnt = MOVIE_DECODE_TIMEOUT;
    while (dec->isdone == 0)
    {
        if (--cnt == 0)
        {
            printf(msg_movie_decode_timeout);
            dec->isdone = 1;
            dec->slice.x = dec->rect.x;
            dec->slice.y = dec->rect.y;
        }
    }
    dec->isdone = 0;
}

void strKickCD(CdlLOC *loc)
{
    do
    {
        while (CdControl(CdlSetloc, (u_char *)loc, 0) == 0)
        {
        }
        if (MovieReadMode & CdlModeSpeed)
        {
            u_char param;

            param = CdlModeSpeed;
            while (CdControl(CdlSetmode, &param, 0) == 0)
            {
            }
            VSync(3);
        }
    } while (CdRead2(MovieReadMode) == 0);
}

/* ENDING string storage. Keep the retail spelling and development paths. */
char msg_movie_decode_timeout[] = "time out in decoding !\n";
