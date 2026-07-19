#ifndef LIBGS_H
#define LIBGS_H

#include <psxsdk/libgte.h>
#include <types.h>

/* Minimal ABI declarations; provenance and validation: docs/psyq-headers.md. */
typedef unsigned char PACKET;

typedef struct
{
    VECTOR scale;
    SVECTOR rotate;
    VECTOR trans;
} GsCOORD2PARAM;

typedef struct _GsCOORDINATE2 GsCOORDINATE2;
struct _GsCOORDINATE2
{
    u_long flg;
    MATRIX coord;
    MATRIX workm;

    GsCOORD2PARAM *param;
    GsCOORDINATE2 *super;
    GsCOORDINATE2 *sub;
};

typedef struct
{
    MATRIX view;
    GsCOORDINATE2 *super;
} GsVIEW2;

typedef struct
{
    long vpx, vpy, vpz;
    long vrx, vry, vrz;
    long rz;
    GsCOORDINATE2 *super;
} GsRVIEW2;

typedef struct
{
    int vx, vy, vz;
    unsigned char r, g, b;
} GsF_LIGHT;

typedef struct
{
    unsigned p : 24;
    unsigned char num : 8;
} GsOT_TAG;

typedef struct
{
    unsigned long length;
    GsOT_TAG *org;
    unsigned long offset;
    unsigned long point;
    GsOT_TAG *tag;
} GsOT;

typedef struct
{
    u_long attribute;
    GsCOORDINATE2 *coord2;
    u_long *tmd;
    u_long id;
} GsDOBJ2;

typedef struct
{
    u_long attribute;
    GsCOORDINATE2 *coord2;
    u_long *pmd;
    u_long *base;
    u_long *sv;
    u_long id;
} GsDOBJ3;

typedef struct
{
    u_long attribute;
    GsCOORDINATE2 *coord2;
    u_long *tmd;
    u_long id;
} GsDOBJ4;

typedef struct
{
    u_long attribute;
    GsCOORDINATE2 *coord2;
    u_long *tmd;
    u_long *packet;
    u_long id;
} GsDOBJ5;

/* 0x24 bytes — layout proven by BriefingAndInventorySelectionScreen's stack
 * (two sprites at sp+0x18/0x40, GsIMAGE at 0x68, next local at 0x88). */
typedef struct
{
    u_long attribute;     /* 0x00 */
    short x, y;           /* 0x04 */
    u_short w, h;         /* 0x08 */
    u_short tpage;        /* 0x0C */
    u_char u, v;          /* 0x0E */
    short cx, cy;         /* 0x10 */
    u_char r, g, b;       /* 0x14 */
    short mx, my;         /* 0x18 */
    short scalex, scaley; /* 0x1C */
    long rotate;          /* 0x20 */
} GsSPRITE;

typedef struct
{
    u_char u, v;
    u_short cba;
    u_short flag;
    u_short tpage;
} GsCELL;

typedef struct
{
    u_char cellw, cellh;
    u_short ncellw, ncellh;
    GsCELL *base;
    u_short *index;
} GsMAP;

typedef struct
{
    u_long attribute;
    short x, y;
    short w, h;
    short scrollx, scrolly;
    u_char r, g, b;
    GsMAP *map;
    short mx, my;
    short scalex, scaley;
    long rotate;
} GsBG;

typedef struct
{
    u_long attribute;
    short x0, y0;
    short x1, y1;
    u_char r, g, b;
} GsLINE;

typedef struct
{
    u_long attribute;
    short x0, y0;
    short x1, y1;
    u_char r0, g0, b0;
    u_char r1, g1, b1;
} GsGLINE;

typedef struct
{
    u_long attribute;
    short x, y;
    u_short w, h;
    u_char r, g, b;
} GsBOXF;

typedef struct
{
    short dqa;
    long dqb;
    u_char rfc, gfc, bfc;
} GsFOGPARAM;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short v0, v1;
    u_short v2, p;
} TMD_P_NF3;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_char r1, g1, b1, p1;
    u_char r2, g2, b2, p2;
    u_short v0, v1;
    u_short v2, p;
} TMD_P_NG3;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_NF4;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char r0, g0, b0, code;
    u_char r1, g1, b1, p1;
    u_char r2, g2, b2, p2;
    u_char r3, g3, b3, p3;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_NG4;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char r0, g0, b0, p1;
    u_short v0, v1;
    u_short v2, p2;
} TMD_P_TNF3;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char r0, g0, b0, p1;
    u_char r1, g1, b1, p2;
    u_char r2, g2, b2, p3;
    u_short v0, v1;
    u_short v2, p4;
} TMD_P_TNG3;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char tu3, tv3;
    u_short p1;
    u_char r0, g0, b0, p2;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_TNF4;

typedef struct
{
    u_char out, in, dummy, cd;
    u_char tu0, tv0;
    u_short clut;
    u_char tu1, tv1;
    u_short tpage;
    u_char tu2, tv2;
    u_short p0;
    u_char tu3, tv3;
    u_short p1;
    u_char r0, g0, b0, p2;
    u_char r1, g1, b1, p3;
    u_char r2, g2, b2, p4;
    u_char r3, g3, b3, p5;
    u_short v0, v1;
    u_short v2, v3;
} TMD_P_TNG4;

struct TMD_STRUCT
{
    u_long *vertop;
    u_long vern;
    u_long *nortop;
    u_long norn;
    u_long *primtop;
    u_long primn;
    u_long scale;
};

typedef struct
{
    short vx, vy, vz;
    u_char tu, tv;
} VERT;

typedef struct
{
    u_long pmode;
    short px;
    short py;
    u_short pw;
    u_short ph;
    u_long *pixel;
    short cx;
    short cy;
    u_short cw;
    u_short ch;
    u_long *clut;
} GsIMAGE;

void GsGetTimInfo(unsigned long *image, GsIMAGE *tim);
void GsInitGraph(unsigned short x, unsigned short y, unsigned short intmode,
                 unsigned short dith, unsigned short vrammode);
void GsInit3D(void);
void GsMapModelingData(unsigned long *model);
void GsSetProjection(long distance);
void GsSetLightMode(int mode);
void GsSetFogParam(GsFOGPARAM *fog);
void GsSetAmbient(long r, long g, long b);
void GsDrawOt(GsOT *ot);
void GsSetWorkBase(PACKET *work);
void GsSortObject4(GsDOBJ2 *object, GsOT *ot, int shift,
                   unsigned long *scratch);
void GsSortSprite(GsSPRITE *sprite, GsOT *ot, unsigned short priority);
/* DrawTargetS still needs its target-proven full-width declaration; centralize
 * GsSortLine once its canonical unsigned-short call shape is recovered. */
void GsSortPoly(void *primitive, GsOT *ot, unsigned short priority);
void GsClearOt(unsigned short offset, unsigned short point, GsOT *ot);
void GsDefDispBuff(unsigned short x0, unsigned short y0,
                   unsigned short x1, unsigned short y1);
void GsSortClear(unsigned char r, unsigned char g, unsigned char b, GsOT *ot);
void GsSwapDispBuff(void);
void GsInitCoordinate2(GsCOORDINATE2 *super, GsCOORDINATE2 *base);
void GsMulCoord0(MATRIX *m1, MATRIX *m2, MATRIX *m3);
void GsMulCoord2(MATRIX *m1, MATRIX *m2);
void GsMulCoord3(MATRIX *m1, MATRIX *m2);
void GsGetLw(GsCOORDINATE2 *coord, MATRIX *matrix);
void GsGetLs(GsCOORDINATE2 *coord, MATRIX *matrix);
void GsLinkObject4(unsigned long model, GsDOBJ2 *object, int index);
int GsSetRefView2(GsRVIEW2 *view);
void GsSetLsMatrix(MATRIX *matrix);
void GsInitFixBg16(GsBG *background, unsigned long *work);
PACKET *GsGetWorkBase(void);

PACKET *GsA4divNF3(TMD_P_NF3 *primitive, VERT *vertices, PACKET *packet,
                   int count, int shift, GsOT *ot, u_long *scratch);
PACKET *GsA4divNG3(TMD_P_NG3 *primitive, VERT *vertices, PACKET *packet,
                   int count, int shift, GsOT *ot, u_long *scratch);
PACKET *GsA4divTNF3(TMD_P_TNF3 *primitive, VERT *vertices, PACKET *packet,
                    int count, int shift, GsOT *ot, u_long *scratch);
PACKET *GsA4divTNG3(TMD_P_TNG3 *primitive, VERT *vertices, PACKET *packet,
                    int count, int shift, GsOT *ot, u_long *scratch);
PACKET *GsA4divNF4(TMD_P_NF4 *primitive, VERT *vertices, PACKET *packet,
                   int count, int shift, GsOT *ot, u_long *scratch);
PACKET *GsA4divNG4(TMD_P_NG4 *primitive, VERT *vertices, PACKET *packet,
                   int count, int shift, GsOT *ot, u_long *scratch);
PACKET *GsA4divTNF4(TMD_P_TNF4 *primitive, VERT *vertices, PACKET *packet,
                    int count, int shift, GsOT *ot, u_long *scratch);
PACKET *GsA4divTNG4(TMD_P_TNG4 *primitive, VERT *vertices, PACKET *packet,
                    int count, int shift, GsOT *ot, u_long *scratch);

extern PACKET *GsOUT_PACKET_P;
extern MATRIX GsWSMATRIX;
extern MATRIX GsWSMATRIX_ORG;
extern MATRIX GsIDMATRIX;
extern MATRIX GsIDMATRIX2;
extern long HWD0, VWD0;
extern u_long GsLMODE, GsLIGNR, GsLIOFF, GsTON;

#endif
