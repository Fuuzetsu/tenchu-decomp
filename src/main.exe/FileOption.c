#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void FileOption(void);
 *     INFOVIEW.C:1021, 119 src lines, frame 7888 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     stack sp+16     struct TAdtSelect [17] ItemName
 *     stack sp+152    struct TAdtSelect [5] SelectIO
 *     stack sp+192    struct TAdtSelect [18] SelectSlot
 *     reg   $s2       int TargetIO
 *     reg   $s1       unsigned char * fname
 *     reg   $s0       void * pBuf
 *     stack sp+336    unsigned char [7000] Buf
 *     reg   $s1       int i
 *     stack sp+7552   unsigned char [26][12] msg
 *     stack sp+7336   struct TAdtSelect [27] targets
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern int StageID;
 *     extern short EngageLevel;
 * END PSX.SYM */

/*
 * FileOption (0x8005c5a8, 1108 bytes) — the debug menu's file/save submenu
 * (dispatch case 3 in DoInfoViewProc): save/load layouts to the memory card,
 * image re-init, SystemFlag toggle, music test by StageID, music-select menu,
 * engage-level presets, stock layout load.
 *
 * STATUS: MATCHING — pure C, all 1108 bytes / 277 instructions exact, with
 * the target's 13 conditional branches, 13 jumps, 21 calls, and 2 returns.
 * Everything derived and verified: menu-table struct copies, (s16) dispatch
 * with case 1 laid out before case 0, u8 buf[7000] scratch sharing, split-
 * address (lui+lo_sum) symbol accesses [-msplit-addresses is ON in this cc1:
 * TARGET_DEFAULT includes MASK_SPLIT_ADDR — non-small extern symbols split,
 * small (≤ -G8) ones stay one-line macros], case-9's format pointer through a
 * `hi = (char *)0x80090000` variable (hi+0x7D70 per call), the case-9
 * terminator's base-first addu via a byte-cast shift index, the cross-jumped
 * leLayoutEnemy(0) tail, gp-relative SystemFlag (this TU defines it).
 *
 * The final scheduler tie closes by passing the byte that was just stored:
 * `load_layout(STAGE_LAYOUT_NUMBER[0])`.  cc1 store-forwards that read to the
 * same `andi a0,v1,0xff` as the old `k & 0xff` spelling, while the memory
 * dependency keeps `sb v1,6(v0)` before the mask.  No load survives.  This is
 * the narrow source-level lever that the earlier statement/fence/permuter
 * searches missed: express a same-width store-to-load dependency and let CSE
 * erase the reload, rather than pinning the schedule with loop notes.
 *
 * gp smalls of this TU: SystemFlag (Build.hs maspsxGpExterns + permute.py).
 * EngageLevel/StageID/D_80010058 are other TUs' smalls -> absolute macros.
 */

typedef struct { debug_menu_choice e[20]; } MENU_FILE_TBL;      /* 0xA0 */
typedef struct { debug_menu_choice e[5];  } MENU_SAVELOAD_TBL;  /* 0x28 */
typedef struct { debug_menu_choice e[18]; } MENU_FLAYOUT_TBL;   /* 0x90 */
typedef struct { s32 e[11]; } MUSIC_TBL;                        /* 0x2C */
typedef struct { debug_menu_choice e[5];  } MENU_STOCK_TBL;     /* 0x28 */

/* gp-relative — defined by this (file-option) TU; Build.hs maspsxGpExterns */
extern u32 SystemFlag;
/* other TUs' smalls — plain absolute externs */
extern s16 EngageLevel;
extern u8 D_80010058;
extern s32 StageID;

extern MENU_FILE_TBL DEBUG_MENU_FILE_CHOICES;
extern MENU_SAVELOAD_TBL DEBUG_MENU_SAVE_LOAD_CHOICES;
extern MENU_FLAYOUT_TBL DEBUG_MENU_FILE_LAYOUT_CHOCIES;
extern MENU_STOCK_TBL DEBUG_MENU_FILE_LOAD_STOCK_LAYOUT_CHOICES;
extern MUSIC_TBL D_80014554;            /* music id by stage */
/* declared as an unknown-size array ON PURPOSE: not-small -> split-address
 * (lui+lo_sum through an allocated reg), where BIS's scalar `extern u8`
 * spelling would be sdata-flagged and become a $at macro store */
extern u8 STAGE_LAYOUT_NUMBER[];

extern char D_80014518[];   /* "file option" */
extern char D_80014524[];   /* "load ok?" */
extern char D_80014530[];   /* "load no?" */
extern char D_8001453C[];   /* "save ok?" */
extern char D_80014548[];   /* "save no?" */
extern char D_8001423C[];   /* "select music" */
extern char D_800145A8[];   /* "layout no" */

extern s32 AdtSelect(char *title, debug_menu_choice *menu, s32 mode);
extern void lePackEnemyLayout(u8 *buf, s32 size);
extern void PackItemLayout(u8 *buf, s32 size);
extern void SaveSI(s32 sel, s32 no, u8 *buf, s32 size);
extern void FUN_8003cd04(s32 sel, s32 no);
extern void InitializeImage(void);
extern void _PlayMusic(s32 id, s32 mode);
extern void CdaStop(void);
extern void SetupStageSequence(void);
extern void CVAsetup(void);
extern void debug_menu_file_animation_test(void);
extern void sprintf(char *s, char *fmt, ...);
extern void PlayMusicFormID(s32 id);
extern void load_layout(s32 no);
extern void leLayoutEnemy(s32 n);

void FileOption(void)
{
    s16 n;
    s32 sel;
    s32 no;
    s32 k;
    s32 i;
    debug_menu_choice *p;
    debug_menu_choice *q;
    char *s;
    char *hi;
    MENU_FILE_TBL m1;
    MENU_SAVELOAD_TBL m2;
    MENU_FLAYOUT_TBL m3;
    u8 buf[7000];

    m1 = DEBUG_MENU_FILE_CHOICES;
    m2 = DEBUG_MENU_SAVE_LOAD_CHOICES;
    m3 = DEBUG_MENU_FILE_LAYOUT_CHOCIES;
    n = AdtSelect(D_80014518, (debug_menu_choice *)&m1, 0);
    if (n == -1)
        return;
    switch (n)
    {
    case 1:
        sel = AdtSelect(D_80014524, (debug_menu_choice *)&m2, 3);
        if (sel == -1)
            return;
        no = AdtSelect(D_80014530, (debug_menu_choice *)&m3, 0x10);
        if (no == -1)
            return;
        FUN_8003cd04(sel & 0xFF, no);
        leLayoutEnemy(0);
        break;
    case 0:
        sel = AdtSelect(D_8001453C, (debug_menu_choice *)&m2, 3);
        if (sel != -1)
        {
            no = AdtSelect(D_80014548, (debug_menu_choice *)&m3, 0x10);
            if (no != -1)
            {
                lePackEnemyLayout(buf, 5000);
                PackItemLayout(buf + 5000, 2000);
                SaveSI(sel, no, buf, 7000);
            }
        }
        break;
    case 2:
        InitializeImage();
        break;
    case 3:
        SystemFlag ^= 1;
        break;
    case 4:
        *(MUSIC_TBL *)buf = D_80014554;
        _PlayMusic(((s32 *)buf)[StageID], 1);
        break;
    case 5:
        CdaStop();
        break;
    case 6:
        SetupStageSequence();
        break;
    case 7:
        CVAsetup();
        break;
    case 8:
        debug_menu_file_animation_test();
        break;
    case 9:
        i = 0;
        hi = (char *)0x80090000;
        q = (debug_menu_choice *)buf;
        p = q;
        s = (char *)(buf + 0x510);
        for (; i < 0xA1; i++)
        {
            sprintf(s, hi + 0x7D70, i);
            p->choice_name = s;
            p->choice_number = i;
            p++;
            s += 5;
        }
        ((debug_menu_choice *)((u8 *)q + (i << 3)))->choice_name = 0;
        PlayMusicFormID(AdtSelect(D_8001423C, (debug_menu_choice *)buf, 0));
        break;
    case 0xA:
        EngageLevel = 3;
        D_80010058 = 0;
        break;
    case 0xB:
        EngageLevel = 2;
        D_80010058 = 1;
        break;
    case 0xC:
        EngageLevel = 1;
        D_80010058 = 2;
        break;
    case 0xD:
        *(MENU_STOCK_TBL *)buf = DEBUG_MENU_FILE_LOAD_STOCK_LAYOUT_CHOICES;
        k = AdtSelect(D_800145A8, (debug_menu_choice *)buf, 0);
        if (k < 0)
            break;
        STAGE_LAYOUT_NUMBER[0] = k;
        SystemFlag &= ~8;
        load_layout(STAGE_LAYOUT_NUMBER[0]);
        leLayoutEnemy(0);
        break;
    }
}
