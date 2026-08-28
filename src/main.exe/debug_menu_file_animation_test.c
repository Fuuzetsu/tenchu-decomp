#include "common.h"
#include "main.exe.h"

/*
 * debug_menu_file_animation_test (0x800501b0) — scans CVAdata for event
 * records whose mode is zero, formats each event id into a menu label, and
 * runs the selected sequence. The final -1 menu value is the cancel entry.
 *
 * MATCHED (65/65 instructions, complete 260-byte carve). Matching notes:
 *  - `text[0x100]` at sp+0x10 followed by `menu[64]` at sp+0x110 fills the
 *    exact 0x300-byte working window and produces the target's 0x328 frame.
 *  - Walking a 12-byte CVAType pointer while using both mode and id lets
 *    loop.c retain parallel s2/s0 induction cursors. A natural `while`
 *    supplies the target's entry guard and conditional bottom back-edge.
 *  - CVAdata is gp-relative in this TU. The format string at 0x80097cd0
 *    needed an explicit linker binding because the interior override split
 *    left its address as raw lui/addiu immediates instead of a data symbol.
 *  - `debug_menu_file_animation_test__override__prt_800501f8_8d2134c3` is
 *    only a call-site prototype marker: piece 1 falls directly into sprintf,
 *    and the second piece branches back into the first. One C body spans it.
 */

extern char fmt_num[];        /* %d */
extern u8 str_cancel[];       /* cancel */
extern char str_event_test[]; /* event test */

extern int sprintf(char *buf, char *fmt, ...);
extern int strlen(char *s);
extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern s16 CVAsequence(s16 sid);

void debug_menu_file_animation_test(void)
{
    u8 text[0x100];
    TAdtSelect menu[64];
    CVAType *event;
    u8 *buffer;
    s32 count;
    s32 selection;

    buffer = text;
    event = CVAdata;
    count = 0;
    while (event->mode != -1)
    {
        if (event->mode == CVA_CMD_SEQUENCE)
        {
            sprintf((char *)buffer, fmt_num, event->id);
            menu[count].name = buffer;
            menu[count].value = event->id;
            count++;
            buffer += strlen((char *)buffer) + 1;
        }
        event++;
    }
    menu[count].name = str_cancel;
    menu[count].value = -1;
    count++;
    menu[count].name = NULL;

    selection = AdtSelect(str_event_test, menu, 0);
    if (selection != -1)
    {
        CVAsequence((s16)selection);
    }
}
