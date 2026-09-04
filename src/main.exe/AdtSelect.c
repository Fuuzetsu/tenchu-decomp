#include "common.h"
#include "main.exe.h"
#include "adt.h"

extern char str_select_item_2[]; /* select item */
extern char fmt_count_pair[];    /*  (%d/%d) */
extern char fmt_str[];           /* %s */
extern char str_blank_line[];    /* "\n\n" */
extern char str_arrow[];         /* -> */
extern char str_spaces[];        /*    */
extern char str_newline_4[];     /* "\n" */

s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode)
{
    TAdtDisp ad;
    s32 last;
    u32 trg;
    s32 count;
    s32 pages;
    s32 page;
    s32 first;
    u32 pad;
    short i;
    char *fmt;

    /* pad is read uninitialized on the first pass below (trg = pad before
     * the first AdtPadRead): retail's own. */
    do
    {
    } while (AdtPadRead(0) != 0);

    if (title == 0)
        title = str_select_item_2;

    for (count = 0; menu[count].name != 0; count++)
    {
    }

    pages = count / 18 + 1;
    AdtGetDisp(&ad);

    while (1)
    {
        DrawPrim(&ad.bg);
        trg = pad;
        pad = AdtPadRead(0);
        trg = ~trg & pad;
        page = mode / 18;
        first = page * 18;
        last = first + 18;
        if (count < last)
            last = count;
        FntPrint(fmt_str, title);
        if (pages > 1)
            FntPrint(fmt_count_pair, page + 1, pages);
        FntPrint(str_blank_line);
        for (i = first; i < last; i++)
        {
            if (mode == i)
                fmt = str_arrow;
            else
                fmt = str_spaces;
            FntPrint(fmt);
            FntPrint(menu[i].name);
            FntPrint(str_newline_4);
        }
        FntFlush(-1);
        VSync(3);
        if (pad & (PADstart | PADRright))
            break;
        if (pad & PADRdown)
        {
            mode = count - 1;
            break;
        }
        if (trg & PADLup)
            i = -1;
        else if (trg & PADLdown)
            i = 1;
        else if (trg & PADLleft)
            i = -18;
        else if (trg & PADLright)
            i = 18;
        else
            i = 0;
        mode += i;
        if (mode < 0)
            mode = 0;
        else if (count <= mode)
            mode = count - 1;
    }
    AdtReleaseDisp(&ad);
    return menu[mode].value;
}
