#include "common.h"
#include "main.exe.h"

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
    while (event->mode != CVA_CMD_END)
    {
        if (event->mode == CVA_CMD_SEQUENCE)
        {
            sprintf((char *)buffer, fmt_num, event->payload.sequence.id);
            menu[count].name = buffer;
            menu[count].value = event->payload.sequence.id;
            count++;
            buffer += strlen((char *)buffer) + 1;
        }
        event++;
    }
    menu[count].name = str_cancel;
    menu[count].value = ADT_SELECT_CANCEL;
    count++;
    menu[count].name = NULL;

    selection = AdtSelect(str_event_test, menu, 0);
    if (selection != ADT_SELECT_CANCEL)
    {
        CVAsequence((s16)selection);
    }
}
