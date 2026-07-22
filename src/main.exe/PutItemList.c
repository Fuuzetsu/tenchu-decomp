#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutItemList(void);
 *     INFOVIEW.C:366, 35 src lines, frame 56 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
extern GsSPRITE CursorImage;
extern GsOT *OTablePt;
extern Sprite3D *ItemImage[];
extern s16 ItemCursor;


static inline void PutItemCursorInline(short x, short y, short size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.y = y;
    CursorImage.scalex = size;
    CursorImage.scaley = size;
    CursorImage.rotate = CursorImage.rotate + rotdif;
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
    NumberImage.u = ou + (cols - q * 10) * 4;
    GsSortSprite(&NumberImage, OTablePt, 0);
    NumberImage.x = NumberImage.x - 6;
    cols = q;
    if (cols != 0)
        goto loop;
    NumberImage.u = ou;
}

void PutItemList(void)
{
    enum { ItemX = 140, ItemY = 100, ItemGap = 20 };
    s32 i;
    s32 x;

    SelectedItem = -1;
    x = ItemX;
    i = 0;
    while (1)
    {
        u32 s;

        if (!(i < 0x19))
            break;

        s = CamState.Owner->item[i];
        if (s != 0)
        {
            s32 n;

            n = s;
            if (s != 0xFF)
            {
                PutNumberInline(x + 0x16, ItemY, n, 0);
            }

            if (ItemCursor == i)
            {
                s32 ItemID;
                GsSPRITE *spr;

                PutItemCursorInline(x, ItemY - 8, 0x1000, -0x6000);

                ItemID = i * sizeof(ItemImage[0]);
                ItemID = *(s32 *)((u8 *)ItemImage + ItemID);
                SelectedItem = i;
                spr = &((Sprite3D *)ItemID)->sprite;
                spr->x = x;
                spr->y = ItemY - 8;
                spr->scalex = 0x1000;
                spr->scaley = 0x1000;
                GsSortSprite(spr, OTablePt, 0);
            }
            else
            {
                s32 ItemID;
                GsSPRITE *spr;

                ItemID = i * sizeof(ItemImage[0]);
                ItemID = *(s32 *)((u8 *)ItemImage + ItemID);
                spr = &((Sprite3D *)ItemID)->sprite;
                spr->x = x;
                spr->y = ItemY - 8;
                spr->scalex = 0xAAA;
                spr->scaley = 0xAAA;
                GsSortSprite(spr, OTablePt, 0);
            }
            x = x - ItemGap;
        }
        i = i + 1;
    }
}
