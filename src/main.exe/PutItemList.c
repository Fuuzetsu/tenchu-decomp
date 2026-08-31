#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutItemList(void);
 *     INFOVIEW.C:366, 35 src lines, frame 56 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s3       int i
 *     reg   $s4       int x
 *     reg   $s0       unsigned int s
 *     reg   $v1       int n
 *     reg   $s2       int ou
 *     reg   $s3       int ItemID
 *     reg   $a0       struct GsSPRITE * spr
 *     reg   $s3       int ItemID
 *     reg   $a0       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern short SelectedItem;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsSPRITE CursorImage;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsOT *OTablePt;
 *     extern short ItemCursor;
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * STATUS: MATCHING — all 504 bytes / 126 instructions exact.
 *
 * Draws each carried item count (except the 0xFF unlimited sentinel), the
 * rotating cursor for the selected kind, and the corresponding bright or dim
 * item icon.  The two small inline routines preserve the natural same-TU
 * boundaries of INFOVIEW.C's immediately neighbouring PutNumber and
 * PutItemCursor operations; this is what gives loop.c the target's NumberImage
 * and CursorImage preheader hoists without the old constant locals and
 * identical-arm fence.
 *
 * The decisive recovery was the meaning and lifetime of the demo's locals.
 * `s` is the carried count loaded into $s0, while each branch-local `ItemID`
 * first holds `i * sizeof(ItemImage[0])` and is then reused for the loaded
 * item pointer.  Fresh loop RTL shows GCC combining those two branch-local
 * arithmetic givs into one reduced offset: its init is emitted after the
 * hoists (`move s5,s3`) and its backedge update is `addiu s5,s5,4`.
 * Hand-writing that machine offset as a function-wide counter created the old
 * 27-byte sched2 local minimum and contradicted this compiler-generated shape.
 *
 * Keeping each arm's own ItemID/spr scope and GsSortSprite call is also
 * intentional: jump2 merges the final calls while retaining the target's
 * branch-local ItemImage address producers; shared `x = x - ItemGap` fills
 * the merged call's delay slot.
 */

static inline void PutItemCursorInline(short x, short y, short size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.y = y;
    CursorImage.scalex = size;
    CursorImage.scaley = size;
    CursorImage.rotate += rotdif;
    GsSortSprite(&CursorImage, OTablePt, 1);
}

static inline void PutNumberInline(int x, int y, int cols, int n)
{
    int ou;
    int q;

    ou = NumberImage.u;
    NumberImage.w = 4;
    NumberImage.x = (s16)x;
    NumberImage.y = (s16)y;
loop:
    q = cols / 10;
    NumberImage.u = ou + (cols % 10) * 4;
    GsSortSprite(&NumberImage, OTablePt, 0);
    NumberImage.x -= 6;
    cols = q;
    if (cols != 0)
        goto loop;
    NumberImage.u = ou;
}

void PutItemList(void)
{
    enum
    {
        ItemX = 140,
        ItemY = 100,
        ItemGap = 20
    };
    s32 i;
    s32 x;

    SelectedItem = -1;
    x = ItemX;
    i = 0;
    while (1)
    {
        u32 s;

        if (i >= 25)
            break;

        s = CamState.Owner->item[i];
        if (s != 0)
        {
            s32 n;

            n = s;
            if (s != ITEM_INFINITE)
            {
                PutNumberInline(x + 22, ItemY, n, 0);
            }

            if (ItemCursor == i)
            {
                GsSPRITE *spr;

                PutItemCursorInline(x, ItemY - 8, FIXED_ONE, SPRITE_ROTATION(-6)); /* spin 6 deg/frame (GsSPRITE.rotate is degrees<<12) */

                SelectedItem = i;
                spr = &ItemImage[i]->sprite;
                spr->x = x;
                spr->y = ItemY - 8;
                spr->scalex = FIXED_ONE;
                spr->scaley = FIXED_ONE;
                GsSortSprite(spr, OTablePt, 0);
            }
            else
            {
                GsSPRITE *spr;

                spr = &ItemImage[i]->sprite;
                spr->x = x;
                spr->y = ItemY - 8;
                spr->scalex = 0xAAA;
                spr->scaley = 0xAAA;
                GsSortSprite(spr, OTablePt, 0);
            }
            x -= ItemGap;
        }
        i++;
    }
}
