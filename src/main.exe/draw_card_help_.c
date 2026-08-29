#include "common.h"
#include "main.exe.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern u8 *McardHelp;
extern Sprite3D *McardSprite;
extern s16 McardStateFlag;
extern s32 McardPageNow;
extern s32 McardAnswered;
extern u8 *McardPageText;
extern Sprite3D *McardButtons[];

extern void SetupTelop(u8 *telop, short line);
extern s32 telop_text_width_(u8 *str);
extern void draw_telop_line_(GsOT_TAG *org, s32 x, s32 y, u8 *str);

/* Draw one page of the memory-card help text and process its trailing prompt.
 * Pages and lines are both separated by double NULs.  A trailing '.' is an
 * acknowledge prompt; '?' selects between accept/cancel, with page 3 drawing
 * the two-choice selector.
 *
 * Keeping the
 * sparse pad values as a switch matters: cc1 emits the target's
 * PADLright-centered comparison tree and separately placed case tails.  Finally,
 * `n` intentionally serves as page offset, line number, and signed result so
 * all three non-overlapping lifetimes reuse s3.
 */
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
        SoundEx(0, 0x1f);
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

    if (text[-2] == '.')
    {
        goto period;
    }
    if (text[-2] == '?')
    {
        goto question;
    }
    goto done;

period:
GsSortSprite(&McardButtons[1]->sprite, OTablePt, 0);
if (pad != PADRright)
{
    goto done;
}
    goto accept;

question:
if (page == 3)
{
    if (McardStateFlag != 0)
    {
        McardButtons[2]->sprite.attribute &= ~SPR_TRANS;
        McardButtons[3]->sprite.attribute |= SPR_TRANS;
    }
    else
    {
        McardButtons[2]->sprite.attribute |= SPR_TRANS;
        McardButtons[3]->sprite.attribute &= ~SPR_TRANS;
    }
    GsSortSprite(&McardButtons[2]->sprite, OTablePt, 0);
    GsSortSprite(&McardButtons[3]->sprite, OTablePt, 0);
    GsSortSprite(&McardButtons[4]->sprite, OTablePt, 0);

    switch (pad)
    {
    case PADRright:
        if (McardStateFlag != 0)
        {
            goto accept;
        }
        goto cancel;

    case PADLleft:
        if (McardStateFlag != 1)
        {
            SoundEx(0, 0x30);
            McardStateFlag = 1;
        }
        break;

    case PADLright:
        if (McardStateFlag != 0)
        {
            SoundEx(0, 0x30);
            McardStateFlag = 0;
        }
        break;
    }
    goto done;
}
else
{
    GsSortSprite(&McardButtons[0]->sprite, OTablePt, 0);
    if (pad != PADRright)
    {
        goto check_cancel;
    }
}

accept:
    SoundEx(0, 0x30);
    n = 1;
    goto done;

check_cancel:
    if (pad != PADRdown)
    {
        goto done;
    }

cancel:
    SoundEx(0, 0x31);
    n = 2;

done:
    if (n != 0)
    {
        McardAnswered = 1;
    }
    /* (short) re-narrows n: byte-required (writer-width rule; measured). */
    return (short)n;
}
