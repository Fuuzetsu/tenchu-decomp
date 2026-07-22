#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern u8 *D_80097D24;
extern Sprite3D *D_80097D28;
extern s16 CardStateFlag;
extern s32 D_80097D38;
extern s32 D_80097D3C;
extern u8 *D_80097D40;
extern Sprite3D *D_800C2D58[];
extern GsOT *OTablePt;

extern short SoundEx(VECTOR *locate, short seid);
extern void SetupTelop(u8 *telop, short line);
extern s32 FUN_800576e8(u8 *str);
extern void FUN_800570b8(GsOT_TAG *org, s32 x, s32 y, u8 *str);

/* Draw one page of the memory-card help text and process its trailing prompt.
 * Pages and lines are both separated by double NULs.  A trailing '.' is an
 * acknowledge prompt; '?' selects between accept/cancel, with page 3 drawing
 * the two-choice selector.
 *
 * The one-shot loops are zero-code cc1 allocation fences.  Their loop-depth
 * weights put end/text/y/n in the target's s0/s1/s2/s3 order; rtlguide and
 * regalloc.py identify this as allocation, not control-flow.  Keeping the
 * sparse pad values as a switch also matters: cc1 emits the target's
 * 0x2000-centered comparison tree and separately placed case tails.  Finally,
 * `n` intentionally serves as page offset, line number, and signed result so
 * all three non-overlapping lifetimes reuse s3.
 */
s32 FUN_8005b17c(s32 page, s32 pad)
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
        D_80097D38 = 0;
        return 0;
    }

    if (D_80097D38 != page)
    {
        page_num = 1;
        n = 0;
        if (page != 1)
        {
            scan = D_80097D24;
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
        D_80097D38 = page;
        D_80097D3C = 0;
        D_80097D40 = D_80097D24 + n;
        SoundEx(0, 0x1f);
    }

    GsSortSprite(&D_80097D28->sprite, OTablePt, 0);

    y = 0;
    text = D_80097D40;
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
    y = (y * -0x10) / 2;

    n = 0;
    while (*text != 0)
    {
        end = text;
        while (*end != 0)
        {
            end++;
        }
        SetupTelop(text, n++);
        width = FUN_800576e8(text);
        FUN_800570b8(OTablePt->org, -(width / 2), y, text);
        text = end + 1;
        y += 0x10;
    }

    n = 0;
    if (D_80097D3C != 0)
    {
        pad = n;
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
    {
        GsSortSprite(&D_800C2D58[1]->sprite, OTablePt, 0);
        if (pad != 0x20)
        {
            goto done;
        }
    }
    goto accept;

question:
    {
        if (page == 3)
        {
            if (CardStateFlag != 0)
            {
                D_800C2D58[2]->sprite.attribute &= 0xbfffffff;
                D_800C2D58[3]->sprite.attribute |= 0x40000000;
            }
            else
            {
                D_800C2D58[2]->sprite.attribute |= 0x40000000;
                D_800C2D58[3]->sprite.attribute &= 0xbfffffff;
            }
            GsSortSprite(&D_800C2D58[2]->sprite, OTablePt, 0);
            GsSortSprite(&D_800C2D58[3]->sprite, OTablePt, 0);
            GsSortSprite(&D_800C2D58[4]->sprite, OTablePt, 0);

            switch (pad)
            {
            case 0x20:
                if (CardStateFlag != 0)
                {
                    goto accept;
                }
                goto cancel;

            case 0x8000:
                if (CardStateFlag != 1)
                {
                    SoundEx(0, 0x30);
                    CardStateFlag = 1;
                }
                break;

            case 0x2000:
                if (CardStateFlag != 0)
                {
                    SoundEx(0, 0x30);
                    CardStateFlag = 0;
                }
                break;
            }
            goto done;
        }
        else
        {
            GsSortSprite(&D_800C2D58[0]->sprite, OTablePt, 0);
            if (pad != 0x20)
            {
                goto check_cancel;
            }
        }
    }

accept:
    SoundEx(0, 0x30);
    n = 1;
    goto done;

check_cancel:
    if (pad != 0x40)
    {
        goto done;
    }

cancel:
    SoundEx(0, 0x31);
    n = 2;

done:
    if (n != 0)
    {
        D_80097D3C = 1;
    }
    return (short)n;
}
