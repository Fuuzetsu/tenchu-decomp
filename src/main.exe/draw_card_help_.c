#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "font.h"
#include "images.h"
#include "memcard.h"

s32 draw_card_help_(s32 page, s32 pad)
{
    s32 page_num;
    s32 n;
    u8 *scan;
    s32 y;
    u8 *text;
    u8 *end;
    s32 width;

    if (page == 0)
    {
        McardPageNow = 0;
        return 0;
    }

    if (McardPageNow != page)
    {
        page_num = 1;
        n = 0;
        if (page != 1)
        {
            scan = McardHelp;
            do
            {
                while (*scan != 0 || scan[1] != 0)
                {
                    scan++;
                    n++;
                }
                scan += 2;
                page_num++;
                n += 2;
            } while (page != page_num);
        }
        McardPageNow = page;
        McardAnswered = 0;
        McardPageText = McardHelp + n;
        SoundEx(0, SE_WEAPON_RECOVER);
    }

    GsSortSprite(&McardSprite->sprite, OTablePt, 0);

    y = 0;
    text = McardPageText;
    if (*text != 0)
    {
        scan = text;
        do
        {
            while (*scan++ != 0)
            {
            }
            y++;
        } while (*scan != 0);
    }
    y = -(y * 0x10) / 2;

    n = 0;
    while (*text != 0)
    {
        end = text;
        while (*end != 0)
        {
            end++;
        }
        SetupTelop(text, n++);
        width = telop_text_width_(text);
        draw_telop_line_(OTablePt->org, -(width / 2), y, text);
        text = end + 1;
        y += 0x10;
    }

    n = 0;
    if (McardAnswered != 0)
    {
        pad = 0;
    }

    switch (text[-2])
    {
    default:
        break;

    case '.':
        GsSortSprite(&McardButtons[MCARD_BUTTON_ACKNOWLEDGE]->sprite, OTablePt,
                     0);
        if (pad == PADRright)
        {
            SoundEx(0, SE_PROJECTILE_HIT);
            n = 1;
        }
        break;

    case '?':
        if (page == 3)
        {
            if (McardStateFlag != 0)
            {
                McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.attribute &=
                    ~GS_ATTR_SEMITRANS_ENABLE;
                McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.attribute |=
                    GS_ATTR_SEMITRANS_ENABLE;
            }
            else
            {
                McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.attribute |=
                    GS_ATTR_SEMITRANS_ENABLE;
                McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.attribute &=
                    ~GS_ATTR_SEMITRANS_ENABLE;
            }
            GsSortSprite(&McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite,
                         OTablePt, 0);
            GsSortSprite(&McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite,
                         OTablePt, 0);
            GsSortSprite(
                &McardButtons[MCARD_BUTTON_FORMAT_SELECTION_HINT]->sprite,
                OTablePt, 0);

            switch (pad)
            {
            case PADRright:
                if (McardStateFlag != 0)
                {
                    SoundEx(0, SE_PROJECTILE_HIT);
                    n = 1;
                }
                else
                {
                    SoundEx(0, SE_PROJECTILE_IMPACT);
                    n = 2;
                }
                break;

            case PADLleft:
                if (McardStateFlag != 1)
                {
                    SoundEx(0, SE_PROJECTILE_HIT);
                    McardStateFlag = 1;
                }
                break;

            case PADLright:
                if (McardStateFlag != 0)
                {
                    SoundEx(0, SE_PROJECTILE_HIT);
                    McardStateFlag = 0;
                }
                break;
            }
            break;
        }
        else
        {
            GsSortSprite(&McardButtons[MCARD_BUTTON_CONFIRM_CANCEL]->sprite,
                         OTablePt, 0);
            if (pad == PADRright)
            {
                SoundEx(0, SE_PROJECTILE_HIT);
                n = 1;
            }
            else if (pad == PADRdown)
            {
                SoundEx(0, SE_PROJECTILE_IMPACT);
                n = 2;
            }
        }
        break;
    }

    if (n != 0)
    {
        McardAnswered = 1;
    }
    /* Re-narrow after decrementing the wider return value. */
    return (short)n;
}
