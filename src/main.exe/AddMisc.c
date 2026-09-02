#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddMisc(enum MiscType type, int x, int y, int z, int a, int b, int c);
 *     MISC.C:636, 47 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       enum MiscType type
 *     param $a1       int x
 *     param $a2       int y
 *     param $a3       int z
 *     param stack+16  int a
 *     param stack+20  int b
 *     param stack+24  int c
 *     reg   $a0       int a
 *     reg   $t1       int b
 *     reg   $t2       int c
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

extern u8 *MiscTimNames[7];                                /* the seven water/warp TIM names */
extern u8 path_image_2[]; /* "K:\\WORK\\CDIMAGE\\IMAGE\\" */
extern char fmt_undefined_effect[];                        /* "undefined effect %d" */

extern void ProcMiscFire(TMisc *m, TMiscMessage msg);
extern void proc_misc_puff_(TMisc *m, TMiscMessage msg);
extern void ProcMiscDoor(TMisc *m, TMiscMessage msg);
extern void ProcMiscPitfall(TMisc *m, TMiscMessage msg);
extern void ProcMiscSnowfall(TMisc *m, TMiscMessage msg);
extern void ProcMiscSprite(TMisc *m, TMiscMessage msg);
extern void proc_misc_bonfire_(TMisc *m, TMiscMessage msg);
extern void proc_misc_sound_(TMisc *m, TMiscMessage msg);
extern void SetupTexScroll(GsIMAGE *im, short vx, short vy);
extern void AdtMessageBox(char *fmt, ...);

void AddMisc(MiscType type, s32 x, s32 y, s32 z, s32 a, s32 b, s32 c)
{
    TMisc *base = misc;
    TMisc *p;
    u8 *tim_names[7];
    GsIMAGE tm;
    u8 **name_table = tim_names;
    GsIMAGE *ptm = &tm;
    s32 va = a;
    s32 vb = b;
    s32 vc = c;
    u8 **selected_name;
    u_long *adr;

    p = base;
loop:
    if (p->proc == 0)
    {
        do
        {
            p->mode = 0;
            p->x = x;
            p->y = y;
            p->z = z;
            p->param.init.a = va;
            p->param.init.b = vb;
            p->param.init.c = vc;
            switch (type)
            {
            case MISC_FIRE:
                if (va == 0)
                    p->proc = ProcMiscFire;
                else
                    p->proc = proc_misc_puff_;
                break;
            case MISC_DOOR:
                p->proc = ProcMiscDoor;
                break;
            case MISC_PITFALL:
                p->proc = ProcMiscPitfall;
                break;
            case MISC_SNOWFALL:
                p->proc = ProcMiscSnowfall;
                break;
            case MISC_SPRITE:
                p->proc = ProcMiscSprite;
                break;
            case MISC_TEXSCROLL:
                __builtin_memcpy(tim_names, MiscTimNames, sizeof(tim_names));
                selected_name = name_table + x;
                adr = PathFileRead(path_image_2, *selected_name);
                GetTIMInfo(adr, ptm);
                LoadTIMAndFree(adr);
                SetupTexScroll(ptm, y, z);
                return;
            case MISC_BONFIRE:
                p->proc = proc_misc_bonfire_;
                break;
            case MISC_SOUND:
                p->proc = proc_misc_sound_;
                break;
            default:
                AdtMessageBox(fmt_undefined_effect, type);
                return;
            }
            p->proc(p, MM_CREATE);
            p->pause = MISC_PAUSED;
        } while (0);
        return;
    }
    do
    {
        do
        {
            p++;
            /* The original ownership test compares the addresses as signed values. */
            if ((s32)p < (s32)(base + MaxMisc))
                goto loop;
        } while (0);
    } while (0);
}
