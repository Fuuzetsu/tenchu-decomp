#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "images.h"
#include "padcmd.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 *     extern short SkipFrame;
 * END PSX.SYM */

/*
 * Pre-mission briefing / item selection screen (0x80052084, 0xE24 bytes).
 *
 * STATUS: MATCHING — all 905 instructions are byte-identical.
 *
 * Matching constraints:
 *  - Keep GetRealPad's recovered full-word `long` return type. A u16 return
 *    moves the cheat-command sign extension ahead of the new-press chain.
 *  - The entry clamp must re-read `mx < cq->gItem[n]`; the similar case-1
 *    clamp must retain `mx < c`. CSE makes the former byte-neutral while its
 *    preference set fixes the store-address register.
 *  - In case 0x1f, keep both eligibility tests as ordinary short-circuit
 *    guards. Changing either one alone creates a paired register conflict.
 *  - Preserve the hand-split `hx`/`hy` cursor shift pairs and intervening
 *    `k = cursor` copy. Their overlapping lifetimes produce the interleaved
 *    extensions and branch-delay-slot fill.
 *  - Retain the do/while(0) boundary around each bounce arm's temporary
 *    update, and the separate boundary around the cursor-move block.
 *  - Digit entry needs an int `t1 = cap` temporary but an inline
 *    `av = t1 - taken`; naming both operands changes local allocation.
 *  - `newpress` is the edge-triggered mask. `selected_kinds` and `taken`
 *    are distinct counts, and the right/down handlers have no shared guard.
 *  - Keep the grid's multi-definition `int c = (u8)var`, the shown loop's
 *    `(s16)j` path through grid y, and the digit loop's int `d`/`quo` with
 *    its loop-carried copy at the bottom.
 *  - Spell all seven item indices as `[idx + (ps->CharType << 5)]` and keep
 *    the grid traversal as a real for loop; both shapes affect expansion and
 *    delay-slot duplication.
 *  - Preserve the two `dsp->u` memory rereads. They seed the required s1/s2
 *    register assignment; caching either value changes the allocation.
 */

/* The persistent state is accessed three ways in the original, on purpose:
 *  - through short-lived pointer locals (q/ps/r below) -> reg+disp addressing;
 *  - through PSTATE casts in the two entry loops -> one hoisted 0x80010000;
 *  - through plain extern globals -> assembler one-line macro (lui+op pairs).
 * Array-indexing spelling picks the addu operand order: `p->arr[i]` puts the
 * base first, `(&p->arr[0])[i]` puts the index first, the extern-symbol form
 * puts the (hoisted) %hi base first.
 */
#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern u8 CHOSEN_CHARACTER;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 ARMOUR_USED; /* persistent blob 0x1a: blocks re-buying ITEM_ARMOUR */
extern ShopItemDefault SHOP_ITEM_DEFAULTS[];
extern char *ITEM_SEL_SPRITE_PTRS[];
extern char NUMBER_TIM_PATH[];
extern u8 *ITEM_HELP_TIM_PATHS[4];
extern s16 CARRY_30_ITEMS_CHEAT_APPLIED; /* gp-relative (TU-local .sdata) */

extern int rand(void);
extern void vfree(void *p);
extern BackGround *load_background_(u_long *tim);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, u8 b);
extern void clear_screen_(void);
extern void exec_process_(int arg);
extern short DrawBG(BackGround *bg);
/* Retail's only caller omits PutNumber's dead fourth parameter. */
extern void PutNumber();
extern void DisposeBG(BackGround *bg);
extern int check_cheat_command_(s16 pad, s16 newpress);
extern void briefing_screen_(void);

/*
 * The two TIM-sprite setup blocks are inlined static helpers (same mechanism
 * as DoInfoViewProc's menus): the GsIMAGE scratch is the helper's own local,
 * so its address expands from the inlined frame base (bare register -- every
 * call gets a direct addiu into the arg register instead of a CSE'd
 * callee-saved pseudo), and the two inline expansions reuse one freed temp
 * slot after the caller's locals (spr @ +0, hspr @ +0x28, tim @ +0x50).
 * NOTE: keep the helpers inside the guard -- in the stub state cc1 emits
 * unreferenced static inlines as standalone code (+32 insns).
 */
static inline u_long *LoadHelpArchive(TLinkInfo *q)
{
    u8 *paths[4];

    __builtin_memcpy(paths, ITEM_HELP_TIM_PATHS, sizeof(paths));
    return FileRead(paths[q->language]);
}

static inline void TimToSprite(u_long *buf, GsSPRITE *sp)
{
    GsIMAGE tim;

    GetTIMInfo(buf, &tim);
    InitSprite(&tim, sp);
}

void BriefingAndInventorySelectionScreen(void)
{
    GsSPRITE spr;
    GsSPRITE hspr;
    s16 bounce;
    union
    {
        u16 u;
        s16 s;
    } pad;
    u16 cap;
    u16 taken;
    BackGround *bg;
    u_long *harc;
    GsSPRITE *p;
    int help;
    TLinkInfo *q;
    TLinkInfo *ps;
    GsSPRITE *dsp;
    u_long *buf;
    int cursor;
    int scale;
    int selected_kinds;
    s16 newpress;
    int i;  /* entry backup loop */
    s16 j;  /* selection/count loop, case1/3 loops, grid, shown, cursor dx, epilogue */
    s32 x;  /* grid x, cursor dy */
    s32 y;  /* grid y */
    s16 j7; /* case 7 loop */
    int ci; /* clamp loops */
    int si; /* cursor search */
    s16 shown;
    s16 av;
    int t;
    int uid;
    int id;
    s16 cheat;

    pad.s = -1;
    cap = 15;
    cursor = 0;
    taken = 0;
    help = -1;

    for (i = 0; i < 0x14; i++)
    {
        PSTATE->saveItem[i] = PSTATE->gItem[i + CHOSEN_CHARACTER * 0x20];
    }
    for (j = 0; j < 0x14; j++)
    {
        PSTATE->selItem[j] = 0;
    }
    q = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    uid = StageConfig[q->StageNo].uid;
    q->selItem[0] = 0xFF;
    if (uid == 0)
    {
        q->selItem[1] = 5;
        return;
    }
    if ((q->GameRetry & 1) == 0)
    {
        briefing_screen_();
    }
    bounce = 0;
    scale = FIXED_ONE;
    buf = FileRead(ITEM_SEL_SPRITE_PTRS[q->language]);
    bg = load_background_(buf);
    vfree(buf);
    buf = FileRead(NUMBER_TIM_PATH);
    /* The p alias over spr is byte-required (direct spr. member writes
     * recolor the address register; measured — the help block below gets
     * away without one). */
    p = &spr;
    TimToSprite(buf, p);
    spr.attribute |= SPR_TRANS_ADD;
    p->x = -160;
    p->y = -120;
    p->r = 0x80;
    p->g = 0x80;
    p->b = 0x80;
    p->mx = p->w >> 1;
    p->my = p->h >> 1;
    spr.mx = 0;
    spr.my = 0;
    LoadTIMAndFree(buf);
    spr.w = 0xC;

    selected_kinds = 1;
    harc = LoadHelpArchive(q);

    {
        TLinkInfo *cq =
            (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
        for (ci = 0; ci < 0x13; ci++)
        {
            int n = SHOP_ITEM_DEFAULTS[ci].itemIndex + CHOSEN_CHARACTER * 0x20;
            u8 c = cq->gItem[n];
            s32 mx = SHOP_ITEM_DEFAULTS[ci].maxStock;
            if (c != ITEM_LOCKED && mx < cq->gItem[n])
            {
                cq->gItem[n] = mx;
            }
        }
    }

    ps = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    do
    {
        rand();
        newpress = pad.u;
        pad.u = GetRealPad(0);
        newpress = pad.u & (pad.u ^ newpress);
        id = check_cheat_command_(pad.s, newpress);
        /* The subtract-then-narrow is retail's own: addiu -1 then an
         * sll/sra s16 truncation before the bound check. The s16 `cheat`
         * temp is byte-required HERE because `id` is an int (the direct
         * switch(id) with unbiased cases was measured off); contrast
         * EquipWeapon, whose source field is already short and whose
         * biased local proved to be an artifact. Cases are combo ids
         * minus one. */
        cheat = id - 1;
        switch (cheat)
        {
        case CHEAT_ITEM_CAP - 1:
            if (CARRY_30_ITEMS_CHEAT_APPLIED == 0)
            {
                CARRY_30_ITEMS_CHEAT_APPLIED = 1;
                cap = 30;
            }
            break;
        case CHEAT_ITEM_REFILL - 1:
            for (j = 1; j < 9; j++)
            {
                int n = j + ps->CharType * 0x20;
                if ((&ps->gItem[0])[n] == ITEM_LOCKED)
                {
                    (&ps->gItem[0])[n] = 1;
                }
                else
                {
                    /* The (&arr[0])[i] decay spelling here and below is the
                     * measured addu operand-order lever (plain arr[i]
                     * flips it; same class as PlayMusicFormID). */
                    (&ps->gItem[0])[n] = (&ps->gItem[0])[n] + 1;
                }
            }
            for (j = 9; j < 0x14; j++)
            {
                int n = j + ps->CharType * 0x20;
                if ((&ps->gItem[0])[n] != ITEM_LOCKED)
                {
                    (&ps->gItem[0])[n] = (&ps->gItem[0])[n] + 1;
                }
            }
            {
                TLinkInfo *cq =
                    (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
                for (ci = 0; ci < 0x13; ci++)
                {
                    int n = SHOP_ITEM_DEFAULTS[ci].itemIndex + CHOSEN_CHARACTER * 0x20;
                    u8 c = cq->gItem[n];
                    s32 mx = SHOP_ITEM_DEFAULTS[ci].maxStock;
                    if (c != ITEM_LOCKED && mx < c)
                    {
                        cq->gItem[n] = mx;
                    }
                }
            }
            break;
        case CHEAT_ITEM_UNLOCK - 1:
            for (j = 9; j < 0x14; j++)
            {
                int n = j + ps->CharType * 0x20;
                if ((&ps->gItem[0])[n] == ITEM_LOCKED)
                {
                    (&ps->gItem[0])[n] = 1;
                }
            }
            break;
        case CHEAT_ARMOUR - 1:
            if (ps->CharType != RIKIMARU_0)
            {
                u8 already = ps->selItem[ITEM_ARMOUR];
                if ((already != 0 ||
                     (&ps->gItem[ITEM_ARMOUR])[ps->CharType * 0x20] == 1) &&
                    (s16)selected_kinds < MAX_SELECTED_ITEMS)
                {
                    if (already == 0)
                    {
                        selected_kinds++;
                        taken++;
                    }
                    ps->selItem[ITEM_ARMOUR] = 0xFF;
                    (&ps->gItem[ITEM_ARMOUR])[ps->CharType * 0x20] = 0;
                    SoundEx(0, SE_MENU_APPLY);
                }
            }
            break;
        case CHEAT_QUIT - 1:
            for (j7 = 0; j7 < 0x14; j7++)
            {
                (&ps->gItem[0])[j7 + (CHOSEN_CHARACTER << 5)] =
                    (&ps->saveItem[0])[j7];
            }
            FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_MODE, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
            clear_screen_();
            STAGE_LAYOUT_NUMBER = 0xFF;
            GameRetry = GameRetry & 0xFE;
            exec_process_(PROCESS_MENU);
            break;
        }
        if (newpress == PADstart)
        {
            goto quit;
        }
        StartDrawing();
        DrawBG(bg);
        for (j = 0; j < 0x13; j++)
        {
            int n = SHOP_ITEM_DEFAULTS[j].itemIndex;
            u8 c = (&ps->gItem[0])[n + (ps->CharType << 5)];
            if (c != ITEM_LOCKED)
            {
                x = SHOP_ITEM_DEFAULTS[j].x;
                y = SHOP_ITEM_DEFAULTS[j].y;
                if (c != 0xFF)
                {
                    PutNumber(x + 0x1A, y + 8, c);
                }
                PutItemIcon(n, x, y, FIXED_ONE);
            }
        }

        PutItemCursor(SHOP_ITEM_DEFAULTS[cursor].x, SHOP_ITEM_DEFAULTS[cursor].y, FIXED_ONE, SPRITE_ROTATION(-3));
        {
            int ddx, ddy, hx, hy;
            int k;

            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            shown = 0x10;
            if ((newpress & PADLdown) == 0)
            {
                shown = 0;
                if ((newpress & PADLup) != 0)
                {
                    shown = -0x10;
                }
            }
            j = 0x10;
            if ((newpress & PADLright) == 0)
            {
                j = 0;
                if ((newpress & PADLleft) != 0)
                {
                    j = -0x10;
                }
            }
            hx = j << 0x10;
            hy = shown << 0x10;
            k = cursor;
            ddx = hx >> 0x10;
            ddy = hy >> 0x10;
            if (ddx != 0 || ddy != 0)
            {
                int best = 0x7FFFFFFF;
                int bi = cursor;
                int tx = SHOP_ITEM_DEFAULTS[bi].x + ddx;
                int ty = SHOP_ITEM_DEFAULTS[bi].y + ddy;
                for (si = 0; si < 0x13; si++)
                {
                    int ex = SHOP_ITEM_DEFAULTS[si].x - tx;
                    int ey = SHOP_ITEM_DEFAULTS[si].y - ty;
                    int d = ex * ex + ey * ey;
                    if (d < best && 0 <= ex * ddx && 0 <= ey * ddy && si != k)
                    {
                        best = d;
                        bi = si;
                    }
                }
                cursor = bi;
            }
        }
        if ((newpress & (PADLup | PADLright | PADLdown | PADLleft)) != 0)
        {
            SoundEx(0, SE_UI_CURSOR);
            help = -1;
        }
        if (newpress != 0 && pad.s == PADRright)
        {
            newpress = 0;
            bounce = 1;
            {
                s16 idx = SHOP_ITEM_DEFAULTS[cursor].itemIndex;
                scale = 0x200;
                if ((&ps->gItem[0])[idx + (ps->CharType << 5)] != 0 &&
                    (&ps->gItem[0])[idx + (ps->CharType << 5)] != ITEM_LOCKED)
                {
                    if ((s16)taken < cap)
                    {
                        u8 cnt = (&ps->selItem[0])[idx];
                        if (cnt == 0)
                        {
                            selected_kinds++;
                        }
                        if ((s16)selected_kinds < MAX_SELECTED_ITEMS)
                        {
                            if (idx != ITEM_ARMOUR || ARMOUR_USED == 0)
                            {
                                (&ps->selItem[0])[idx] = cnt + 1;
                                taken++;
                                (&ps->gItem[0])[idx + (ps->CharType << 5)]--;
                            }
                            SoundEx(0, SE_ITEM_TRANSFER);
                        }
                        else
                        {
                            SoundEx(0, SE_ITEM_UNAVAILABLE);
                            help = 0x14;
                            selected_kinds--;
                        }
                    }
                    else
                    {
                        SoundEx(0, SE_ITEM_UNAVAILABLE);
                        help = 0x13;
                    }
                }
            }
        }
        if (newpress != 0 && pad.s == PADRdown)
        {
            s16 idx = SHOP_ITEM_DEFAULTS[cursor].itemIndex;
            bounce = 2;
            {
                u8 c = (&ps->selItem[0])[idx];
                scale = 0x1400;
                if (c != 0)
                {
                    if (c == 0xFF)
                    {
                        (&ps->selItem[0])[idx] = 0;
                        (&ps->gItem[0])[idx + (ps->CharType << 5)] = 1;
                        selected_kinds--;
                    }
                    else
                    {
                        (&ps->selItem[0])[idx] = c - 1;
                        (&ps->gItem[0])[idx + (ps->CharType << 5)]++;
                        if ((&ps->selItem[0])[idx] == 0)
                        {
                            selected_kinds--;
                        }
                    }
                    taken--;
                    SoundEx(0, SE_WEAPON_RECOVER);
                }
            }
            help = -1;
        }
        if ((s16)scale < FIXED_ONE)
        {
            scale += 0xC0;
        }
        if (help == -1 &&
            (&ps->gItem[0])[SHOP_ITEM_DEFAULTS[cursor].itemIndex +
                            (ps->CharType << 5)] != ITEM_LOCKED)
        {
            help = SHOP_ITEM_DEFAULTS[cursor].itemIndex - 1;
        }
        if (help != -1)
        {
            buf = get_tim_from_archive(harc, help);
            TimToSprite(buf, &hspr);
            hspr.x = -160;
            hspr.y = -120;
            hspr.r = 0x80;
            hspr.g = 0x80;
            hspr.b = 0x80;
            hspr.attribute |= SPR_TRANS_ADD;
            hspr.mx = hspr.w >> 1;
            hspr.my = hspr.h >> 1;
            hspr.mx = 0;
            hspr.my = 0;
            LoadTIM(buf);
            hspr.x = -146;
            hspr.y = 35;
            GsSortSprite(&hspr, OTablePt, 1);
        }
        if (bounce == 1)
        {
            t = scale + 0x10;
            scale = t;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            if ((s16)t > 0x1400)
            {
                bounce ^= 1;
            }
        }
        else if (bounce == 0)
        {
            t = scale - 0x10;
            scale = t;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            if ((s16)t < FIXED_ONE)
            {
                bounce ^= 1;
            }
        }
        else if (bounce == 2)
        {
            t = scale - 0x10;
            scale = t;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            if ((s16)t < FIXED_ONE)
            {
                bounce = 0;
            }
        }
        shown = 0;
        for (j = 0; j < 0x14; j++)
        {
            u8 c;
            y = (s16)j;
            c = (&ps->selItem[0])[y];
            if (c != 0)
            {
                if (c != 0xFF)
                {
                    PutNumber(0xA6 - shown * 0x19, 0x62, c);
                }
                PutItemIcon(y, (s16)(0x8C - shown * 0x19), 0x5A, FIXED_ONE);
                shown++;
            }
        }
        {
            int neg;
            int m;
            int tv;
            int rem;
            int quo;
            int d;
            int t1;

            t1 = cap;
            dsp = &spr;
            dsp->x = 0x22;
            dsp->y = -50;
            av = t1 - taken;
            tv = (s16)av;
            if (tv < 0)
            {
                av = -tv;
                neg = 1;
            }
            else
            {
                neg = 0;
            }
            do
            {
                d = av;
                quo = d / 10;
                x = dsp->u;
                rem = d - quo * 10;
                dsp->u += (s16)rem * dsp->w;
                GsSortSprite(dsp, OTablePt, 0);
                dsp->u = x;
                dsp->x -= 0xC;
                av = quo;
            } while ((s16)av != 0);
            if (neg)
            {
                int c = (u8)dsp->u;
                m = 10;
                dsp->u = c + dsp->w * m;
                GsSortSprite(dsp, OTablePt, 0);
                dsp->u = c;
            }
        }
        SkipFrame = 2;
        EndDrawing(0);
    } while (1);

quit:
    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_MODE, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();
    if (PSTATE->selItem[ITEM_MANEBUE] != 0)
    {
        PSTATE->selItem[ITEM_MANEBUE] = 0xFF;
    }
    for (j = 0; j < 9; j++)
    {
        int n = j + PSTATE->CharType * 0x20;
        if (PSTATE->gItem[n] == 0)
        {
            PSTATE->gItem[n] = ITEM_LOCKED;
        }
    }
    vfree(harc);
    DisposeBG(bg);
}
