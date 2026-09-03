#include "common.h"
#include "main.exe.h"
#include "adt.h"

extern AdtFntState AdtFnt;
extern s32 AdtMessageBoxCount;                                         /* AdtMessageBox call counter */
extern char msg_adtinit_not_called[]; /* "*** AdtInit not called ***" */
extern char fmt_messagebox_count[];                                    /* "AdtMessageBox #%d\n\n" */
extern char msg_press_start[];                                         /* "\n\nPress start to continue..." */

extern s32 AdtVsprintf(s32 *args, char *dst, u32 n, char *fmt);
extern s32 VSync(s32 mode);

enum adt_message_mode
{
    ADT_MESSAGE_MODAL,
    ADT_MESSAGE_NO_WAIT,
    ADT_MESSAGE_TEXT_ONLY
};

void AdtMessageBox(char *fmt, ...)
{
    s32 mode;
    s32 count;
    char buf[504]; /* print limit is 500; 504 is the frame-layout size */
    TAdtDisp ad;

    mode = ADT_MESSAGE_MODAL;
    if (AdtPadRead == AdtDmyPadRead)
        fmt = msg_adtinit_not_called;
    if (AdtFnt.quiet == ADT_QUIET)
        return;
    if (*fmt == '%')
    {
        if (fmt[1] != '#')
        {
            if (fmt[1] == '$')
            {
                mode = ADT_MESSAGE_NO_WAIT;
                fmt += 2;
            }
        }
        else
        {
            mode = ADT_MESSAGE_TEXT_ONLY;
            fmt += 2;
        }
    }
    /* Holding Select+Start skips ADT message boxes. */
    if ((AdtPadRead(0) & PADselect) && (AdtPadRead(0) & PADstart))
        return;

    AdtVsprintf((s32 *)((char *)&fmt + sizeof(fmt)), buf, 0x1F4, fmt);
    AdtGetDisp(&ad);
    if (mode < ADT_MESSAGE_TEXT_ONLY)
    {
        DrawPrim(&ad.bg);
        count = AdtMessageBoxCount + 1;
        AdtMessageBoxCount = count;
        FntPrint(fmt_messagebox_count, count);
    }
    FntPrint(buf);
    if (mode == ADT_MESSAGE_MODAL)
        FntPrint(msg_press_start);
    FntFlush(-1);
    DrawSync(0);
    VSync(2);
    if (mode == ADT_MESSAGE_MODAL)
    {
        while (AdtPadRead(0) & PADstart)
            VSync(0);
        while (!(AdtPadRead(0) & PADstart))
            VSync(0);
        DrawPrim(&ad.bg);
        DrawSync(0);
    }
    FntLoad(AdtFnt.tx, AdtFnt.ty);
    FntOpen(AdtFnt.x, AdtFnt.y, AdtFnt.w, AdtFnt.h,
            AdtFnt.isbg, AdtFnt.n);
    LoadImage(&ad.rect, ad.backup);
    DrawSync(0);
    PutDrawEnv(&ad.draw);
    PutDispEnv(&ad.disp);
}
