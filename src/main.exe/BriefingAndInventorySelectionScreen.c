#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "infoview.h"
#include "images.h"
#include "padcmd.h"
#include "sound.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern u8 CHOSEN_CHARACTER;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 ARMOUR_USED; /* persistent blob 0x1a: blocks re-buying ITEM_ARMOUR */
extern ShopItemDefault SHOP_ITEM_DEFAULTS[];
extern char NUMBER_TIM_PATH[];
extern s16 CARRY_30_ITEMS_CHEAT_APPLIED; /* gp-relative (TU-local .sdata) */

extern int rand(void);
extern void vfree(void *p);
extern BackGround *load_background_(u_long *tim);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, u8 b);
extern void clear_screen_(void);
extern void exec_process_(int arg);
extern short DrawBG(BackGround *bg);
extern void DisposeBG(BackGround *bg);
extern int check_cheat_command_(s16 pad, s16 newpress);
extern void briefing_screen_(void);

static inline ArcFile *LoadHelpArchive(TLinkInfo *q)
{
    u8 *paths[N_LANGUAGES];

    __builtin_memcpy(paths, ITEM_HELP_ARCHIVE_PATHS, sizeof(paths));
    return (ArcFile *)FileRead(paths[q->language]);
}

static inline void TimToSprite(u_long *buf, GsSPRITE *sp)
{
    GsIMAGE tim;

    GetTIMInfo(buf, &tim);
    InitSprite(&tim, sp);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 *     extern short SkipFrame;
 * END PSX.SYM */

void BriefingAndInventorySelectionScreen(void)
{
    GsSPRITE spr;
    GsSPRITE hspr;
    s16 bounce;
    s16 pad;
    u16 cap;
    u16 taken;
    BackGround *bg;
    ArcFile *harc;
    GsSPRITE *p;
    ItemHelpImageId help_image;
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

    pad = -1;
    cap = NORMAL_ITEM_CARRY_LIMIT;
    cursor = 0;
    taken = 0;
    help_image = ITEM_HELP_NONE;

    for (i = 0; i < N_LOADOUT_ITEMS; i++)
    {
        PSTATE->saveItem[i] = PSTATE->gItem[CHOSEN_CHARACTER][i];
    }
    for (j = 0; j < N_LOADOUT_ITEMS; j++)
    {
        PSTATE->selItem[j] = 0;
    }
    q = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    uid = StageConfig[q->StageNo].uid;
    q->selItem[ITEM_KAGINAWA] = ITEM_INFINITE;
    if (uid == STAGE_UID_TRAINING)
    {
        q->selItem[ITEM_SHURIKEN] = 5;
        return;
    }
    if ((q->GameRetry & GAME_RETRY_REPLAY) == 0)
    {
        briefing_screen_();
    }
    bounce = 0;
    scale = FIXED_ONE;
    buf = FileRead(ITEM_SELECTION_SCREEN_PATHS[q->language]);
    bg = load_background_(buf);
    vfree(buf);
    buf = FileRead(NUMBER_TIM_PATH);
    p = &spr;
    TimToSprite(buf, p);
    spr.attribute |= GS_ATTR_SEMITRANS_ADD;
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
        for (ci = 0; ci < N_SHOP_ITEMS; ci++)
        {
            u8 c = cq->gItem[CHOSEN_CHARACTER][SHOP_ITEM_DEFAULTS[ci].itemIndex];
            s32 mx = SHOP_ITEM_DEFAULTS[ci].maxStock;
            if (c != ITEM_LOCKED && mx < cq->gItem[CHOSEN_CHARACTER][SHOP_ITEM_DEFAULTS[ci].itemIndex])
            {
                cq->gItem[CHOSEN_CHARACTER][SHOP_ITEM_DEFAULTS[ci].itemIndex] = mx;
            }
        }
    }

    ps = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    do
    {
        rand();
        newpress = (u16)pad;
        pad = GetRealPad(PAD_PORT_1);
        newpress = (u16)pad & ((u16)pad ^ newpress);
        id = check_cheat_command_(pad, newpress);
        /* Cheat ids are one-based; retail narrows after subtracting. */
        cheat = id - 1;
        switch (cheat)
        {
        case CHEAT_ITEM_CAP - 1:
            if (CARRY_30_ITEMS_CHEAT_APPLIED == 0)
            {
                CARRY_30_ITEMS_CHEAT_APPLIED = 1;
                cap = CHEAT_ITEM_CARRY_LIMIT;
            }
            break;
        case CHEAT_ITEM_REFILL - 1:
            for (j = ITEM_SHURIKEN; j < ITEM_NEMURI; j++)
            {
                int n = SAVE_ITEM_INDEX(ps->CharType, j);
                if (TLINKINFO_FLAT_STOCK(ps, n) == ITEM_LOCKED)
                {
                    TLINKINFO_FLAT_STOCK(ps, n) = 1;
                }
                else
                {
                    TLINKINFO_FLAT_STOCK(ps, n) =
                        TLINKINFO_FLAT_STOCK(ps, n) + 1;
                }
            }
            for (j = ITEM_NEMURI; j < N_LOADOUT_ITEMS; j++)
            {
                int n = SAVE_ITEM_INDEX(ps->CharType, j);
                if (TLINKINFO_FLAT_STOCK(ps, n) != ITEM_LOCKED)
                {
                    TLINKINFO_FLAT_STOCK(ps, n) =
                        TLINKINFO_FLAT_STOCK(ps, n) + 1;
                }
            }
            {
                TLinkInfo *cq =
                    (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
                for (ci = 0; ci < N_SHOP_ITEMS; ci++)
                {
                    u8 c = cq->gItem[CHOSEN_CHARACTER][SHOP_ITEM_DEFAULTS[ci].itemIndex];
                    s32 mx = SHOP_ITEM_DEFAULTS[ci].maxStock;
                    if (c != ITEM_LOCKED && mx < c)
                    {
                        cq->gItem[CHOSEN_CHARACTER][SHOP_ITEM_DEFAULTS[ci].itemIndex] = mx;
                    }
                }
            }
            break;
        case CHEAT_ITEM_UNLOCK - 1:
            for (j = ITEM_NEMURI; j < N_LOADOUT_ITEMS; j++)
            {
                int n = SAVE_ITEM_INDEX(ps->CharType, j);
                if (TLINKINFO_FLAT_STOCK(ps, n) == ITEM_LOCKED)
                {
                    TLINKINFO_FLAT_STOCK(ps, n) = 1;
                }
            }
            break;
        case CHEAT_ARMOUR - 1:
            if (ps->CharType != RIKIMARU_0)
            {
                u8 already = ps->selItem[ITEM_ARMOUR];
                if ((already != 0 ||
                     TLINKINFO_STOCK(ps, ps->CharType, ITEM_ARMOUR) == 1) &&
                    (s16)selected_kinds < MAX_SELECTED_ITEMS)
                {
                    if (already == 0)
                    {
                        selected_kinds++;
                        taken++;
                    }
                    ps->selItem[ITEM_ARMOUR] = ITEM_INFINITE;
                    TLINKINFO_STOCK(ps, ps->CharType, ITEM_ARMOUR) = 0;
                    SoundEx(0, SE_MENU_APPLY);
                }
            }
            break;
        case CHEAT_QUIT - 1:
            for (j7 = 0; j7 < N_LOADOUT_ITEMS; j7++)
            {
                TLINKINFO_STOCK(ps, CHOSEN_CHARACTER, j7) =
                    (&ps->saveItem[0])[j7];
            }
            FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
            clear_screen_();
            STAGE_LAYOUT_NUMBER = STAGE_LAYOUT_RANDOM;
            GameRetry = GameRetry & (u8)~GAME_RETRY_REPLAY;
            exec_process_(PROCESS_MENU);
            break;
        }
        if (newpress == PADstart)
        {
            goto quit;
        }
        StartDrawing();
        DrawBG(bg);
        for (j = 0; j < N_SHOP_ITEMS; j++)
        {
            int n = SHOP_ITEM_DEFAULTS[j].itemIndex;
            u8 c = TLINKINFO_STOCK(ps, ps->CharType, n);
            if (c != ITEM_LOCKED)
            {
                x = SHOP_ITEM_DEFAULTS[j].x;
                y = SHOP_ITEM_DEFAULTS[j].y;
                if (c != ITEM_INFINITE)
                {
                    PutNumber(x + 0x1A, y + 8, c);
                }
                PutItemIcon(n, x, y, FIXED_ONE);
            }
        }

        PutItemCursor(SHOP_ITEM_DEFAULTS[cursor].x, SHOP_ITEM_DEFAULTS[cursor].y, FIXED_ONE, -3 * FIXED_ONE);
        {
            int ddx, ddy, hx, hy;
            int k;

            /* Empty loop retained for code layout; its original source construct is unknown. */
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
                for (si = 0; si < N_SHOP_ITEMS; si++)
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
            help_image = ITEM_HELP_NONE;
        }
        if (newpress != 0 && pad == PADRright)
        {
            newpress = 0;
            bounce = 1;
            {
                s16 idx = SHOP_ITEM_DEFAULTS[cursor].itemIndex;
                scale = 0x200;
                if (TLINKINFO_STOCK(ps, ps->CharType, idx) != 0 &&
                    TLINKINFO_STOCK(ps, ps->CharType, idx) != ITEM_LOCKED)
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
                                TLINKINFO_STOCK(ps, ps->CharType, idx)--;
                            }
                            SoundEx(0, SE_ITEM_TRANSFER);
                        }
                        else
                        {
                            SoundEx(0, SE_ITEM_UNAVAILABLE);
                            help_image = ITEM_HELP_KIND_LIMIT_REACHED;
                            selected_kinds--;
                        }
                    }
                    else
                    {
                        SoundEx(0, SE_ITEM_UNAVAILABLE);
                        help_image = ITEM_HELP_ITEM_LIMIT_REACHED;
                    }
                }
            }
        }
        if (newpress != 0 && pad == PADRdown)
        {
            s16 idx = SHOP_ITEM_DEFAULTS[cursor].itemIndex;
            bounce = 2;
            {
                u8 c = (&ps->selItem[0])[idx];
                scale = 0x1400;
                if (c != 0)
                {
                    if (c == ITEM_INFINITE)
                    {
                        (&ps->selItem[0])[idx] = 0;
                        TLINKINFO_STOCK(ps, ps->CharType, idx) = 1;
                        selected_kinds--;
                    }
                    else
                    {
                        (&ps->selItem[0])[idx] = c - 1;
                        TLINKINFO_STOCK(ps, ps->CharType, idx)++;
                        if ((&ps->selItem[0])[idx] == 0)
                        {
                            selected_kinds--;
                        }
                    }
                    taken--;
                    SoundEx(0, SE_WEAPON_RECOVER);
                }
            }
            help_image = ITEM_HELP_NONE;
        }
        if ((s16)scale < FIXED_ONE)
        {
            scale += 0xC0;
        }
        if (help_image == ITEM_HELP_NONE &&
            TLINKINFO_STOCK(ps, ps->CharType,
                            SHOP_ITEM_DEFAULTS[cursor].itemIndex) != ITEM_LOCKED)
        {
            help_image =
                ITEM_HELP_FOR_ITEM(SHOP_ITEM_DEFAULTS[cursor].itemIndex);
        }
        if (help_image != ITEM_HELP_NONE)
        {
            buf = get_tim_from_archive(harc, help_image);
            TimToSprite(buf, &hspr);
            hspr.x = -160;
            hspr.y = -120;
            hspr.r = 0x80;
            hspr.g = 0x80;
            hspr.b = 0x80;
            hspr.attribute |= GS_ATTR_SEMITRANS_ADD;
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
            /* Empty loop retained for code layout; its original source construct is unknown. */
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
            /* Empty loop retained for code layout; its original source construct is unknown. */
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
            /* Empty loop retained for code layout; its original source construct is unknown. */
            do
            {
            } while (0);
            if ((s16)t < FIXED_ONE)
            {
                bounce = 0;
            }
        }
        shown = 0;
        for (j = 0; j < N_LOADOUT_ITEMS; j++)
        {
            u8 c;
            y = (s16)j;
            c = (&ps->selItem[0])[y];
            if (c != 0)
            {
                if (c != ITEM_INFINITE)
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
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(0);
    } while (1);

quit:
    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();
    if (PSTATE->selItem[ITEM_MANEBUE] != 0)
    {
        PSTATE->selItem[ITEM_MANEBUE] = ITEM_INFINITE;
    }
    for (j = ITEM_KAGINAWA; j < ITEM_NEMURI; j++)
    {
        if (PSTATE->gItem[PSTATE->CharType][j] == 0)
        {
            PSTATE->gItem[PSTATE->CharType][j] = ITEM_LOCKED;
        }
    }
    vfree(harc);
    DisposeBG(bg);
}
