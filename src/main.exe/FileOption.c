#include "common.h"
#include "main.exe.h"

/*
 * FileOption (0x8005c5a8, 1108 bytes) — the debug menu's file/save submenu
 * (dispatch case 3 in DoInfoViewProc): save/load layouts to the memory card,
 * image re-init, SystemFlag toggle, music test by StageID, music-select menu,
 * engage-level presets, stock layout load.
 *
 * CURRENT(2): 275/277 instructions byte-identical; see the #if 0 draft below.
 * The ONLY residual is one instruction-ORDER swap in case 0xd:
 *     target: sb v1,6(v0); andi a0,v1,0xff; lw v0,gp(SystemFlag)
 *     ours:   andi a0,v1,0xff; sb v1,6(v0); lw v0,gp(SystemFlag)
 * (same insns, same registers, adjacent swap — 6 differing bytes whole-image).
 * Everything derived and verified: menu-table struct copies, (s16) dispatch
 * with case 1 laid out before case 0, u8 buf[7000] scratch sharing, split-
 * address (lui+lo_sum) symbol accesses [-msplit-addresses is ON in this cc1:
 * TARGET_DEFAULT includes MASK_SPLIT_ADDR — non-small extern symbols split,
 * small (≤ -G8) ones stay one-line macros], case-9's format pointer through a
 * `hi = (char *)0x80090000` variable (hi+0x7D70 per call), the case-9
 * terminator's base-first addu via a byte-cast shift index, the cross-jumped
 * leLayoutEnemy(0) tail, gp-relative SystemFlag (this TU defines it).
 *
 * Why the residual swap resists (sched1/sched2 dumps + gcc 2.8.1 sched.c
 * reading): the call-arg mask `k & 0xFF` is expanded (or combine-fused) at
 * the call, and the backward scheduler's equal-priority tiebreak
 * (schedule_select -> potential_hazard) always prefers the STORE (memory
 * unit, max_blockage>1) over the andi (alu, blockage 1 -> hazard 0), so the
 * sb is scheduled backward-first = placed forward-later: [andi][sb], every
 * time, in both sched passes. Every lever tried moves other bytes:
 *  - statement orders/temps for the mask and the SystemFlag RMW: no effect
 *    (T01-T08 all identical);
 *  - do{}while(0) fences (loop notes) around/between the statements DO pin
 *    the order ([sb] first) but then k dies at the andi and local/global
 *    alloc ties k into $a0 (in-place `andi a0,a0`), breaking move/bltz/sb
 *    registers and freeing the jal slot filler — net worse (5 lines);
 *  - multi-def mask hosts don't block the combine drag (links are
 *    per-reaching-def); cross-block defs change the branch-block bytes;
 *  - a `q = &SLN[0]`-style pointer or PSTATE cast either const-folds back
 *    (cse) or becomes the $at macro (small-symbol path).
 * decomp.me/psyq4.3 arbitration or a future idiom may crack it; the permuter
 * (450k iterations, 8 threads) found nothing below score 70.
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
extern void PlayMusicFromID(s32 id);
extern void load_layout(s32 no);
extern void leLayoutEnemy(s32 n);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", FileOption);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__switchD);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_1);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_0);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_2);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_3);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_4);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_5);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_6);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_7);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_8);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_9);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", debug_menu_file_option__override__prt_8005c898_8d2134c3);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_a);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_b);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_c);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_d);

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FileOption", switchD_8005c6cc__caseD_e);

/*
 * Stub-state jump table. The splat yaml routes vram 0x800147B8 through this
 * TU's .rodata ([0x3FB8, .rodata, FileOption]) so the ACTIVE function's
 * compiled switch table lands at the original address. In the stub state the
 * INCLUDE_ASM pieces emit no .rodata, so provide the original 14-entry table
 * verbatim (its absence shifts the whole image by 0x38 bytes).
 * DELETE this array when activating the #if 0 draft below.
 */
static const u32 switchD_8005c6cc_jtbl[14] = {
    0x8005C728, 0x8005C6D4, 0x8005C79C, 0x8005C7AC,
    0x8005C7C4, 0x8005C83C, 0x8005C84C, 0x8005C85C,
    0x8005C86C, 0x8005C87C, 0x8005C8EC, 0x8005C908,
    0x8005C928, 0x8005C948,
};

#if 0
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
        PlayMusicFromID(AdtSelect(D_8001423C, (debug_menu_choice *)buf, 0));
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
        load_layout(k & 0xFF);
        leLayoutEnemy(0);
        break;
    }
}
#endif
