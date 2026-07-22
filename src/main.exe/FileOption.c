#include "common.h"
#include "main.exe.h"
#include "infoview.h"

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
 *     extern unsigned char gNannido;
 * END PSX.SYM */

/*
 * FileOption (0x8005c5a8, 1108 bytes) — the debug menu's file/save submenu
 * (dispatch case 3 in DoInfoViewProc): save/load layouts to the memory card,
 * image re-init, SystemFlag toggle, music test by StageID, music-select menu,
 * engage-level presets, stock layout load.
 *
 * STATUS: MATCHED — pure C, all 1108 bytes / 277 instructions exact, with
 * the target's 13 conditional branches, 13 jumps, 21 calls, and 2 returns.
 * Everything derived and verified: local menu-template copies, (s16) dispatch
 * with case 1 laid out before case 0, the shared 7000-byte work area, split-
 * address (lui+lo_sum) symbol accesses [-msplit-addresses is ON in this cc1:
 * TARGET_DEFAULT includes MASK_SPLIT_ADDR — non-small extern symbols split,
 * small (≤ -G8) ones stay one-line macros], the case-9 terminator's base-first
 * addu via a byte-cast shift index, the cross-jumped leLayoutEnemy(0) tail,
 * and this TU's gp-relative SystemFlag accesses.
 *
 * The final scheduler tie closes by passing the byte that was just stored:
 * `load_layout(STAGE_LAYOUT_NUMBER[0])`.  cc1 store-forwards that read to the
 * same `andi a0,v1,0xff` as the old `k & 0xff` spelling, while the memory
 * dependency keeps `sb v1,6(v0)` before the mask.  No load survives.  This is
 * the narrow source-level lever that the earlier statement/fence/permuter
 * searches missed: express a same-width store-to-load dependency and let CSE
 * erase the reload, rather than pinning the schedule with loop notes.
 *
 * SystemFlag is gp-relative in this TU (Build.hs maspsxGpExterns + permute.py).
 * EngageLevel/StageID/gNannido are other TUs' smalls -> absolute macros.
 * The FILE_WORK union expresses the retail stack reuse directly.  Its indexed
 * targets/messages view is the human shape recorded in PSX.SYM (with larger
 * retail bounds); loop.c derives the target's three walking pointers from it.
 * That source shape also lets case 9 name D_80097D70 directly: cc1 keeps its
 * `%hi` half loop-invariant while forming `%lo` at each call, producing both
 * the retail instruction schedule and ordinary relocations.
 */

typedef struct { s32 e[11]; } MUSIC_TBL;                        /* 0x2C */
typedef union {
    u8 bytes[7000];
    struct {
        TAdtSelect targets[162];
        u8 msg[161][5];
    } music;
} FILE_WORK;

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
extern char D_80097D70[];   /* "%d" */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern void lePackEnemyLayout(void *buf, long size);
extern void PackItemLayout(void *buf, long size);
extern void SaveSI(int target, u8 *name, void *mem, long size);
extern void FUN_8003cd04(int target, u8 *name);
extern void InitializeImage(void);
extern void _PlayMusic(s32 id, s32 mode);
extern void CdaStop(void);
extern void SetupStageSequence(void);
extern void CVAsetup(void);
extern void debug_menu_file_animation_test(void);
extern void sprintf(char *s, char *fmt, ...);
extern void PlayMusicFormID(s32 id);
extern void load_layout(s32 no);

void FileOption(void)
{
    s16 n;
    s32 TargetIO;
    u8 *fname;
    s32 k;
    s32 i;
    TAdtSelect *targets;
    u8 (*messages)[5];
    TAdtSelect ItemName[20];
    TAdtSelect SelectIO[5];
    TAdtSelect SelectSlot[18];
    FILE_WORK work;

    __builtin_memcpy(ItemName, DEBUG_MENU_FILE_CHOICES, sizeof(ItemName));
    __builtin_memcpy(SelectIO, DEBUG_MENU_SAVE_LOAD_CHOICES, sizeof(SelectIO));
    __builtin_memcpy(SelectSlot, DEBUG_MENU_FILE_LAYOUT_CHOCIES, sizeof(SelectSlot));
    n = AdtSelect(D_80014518, ItemName, 0);
    if (n == -1)
        return;
    switch (n)
    {
    case 1:
        TargetIO = AdtSelect(D_80014524, SelectIO, 3);
        if (TargetIO == -1)
            return;
        fname = (u8 *)AdtSelect(D_80014530, SelectSlot, 0x10);
        if (fname == (u8 *)-1)
            return;
        FUN_8003cd04(TargetIO & 0xFF, fname);
        leLayoutEnemy(0);
        break;
    case 0:
        TargetIO = AdtSelect(D_8001453C, SelectIO, 3);
        if (TargetIO != -1)
        {
            fname = (u8 *)AdtSelect(D_80014548, SelectSlot, 0x10);
            if (fname != (u8 *)-1)
            {
                lePackEnemyLayout(work.bytes, 5000);
                PackItemLayout(work.bytes + 5000, 2000);
                SaveSI(TargetIO, fname, work.bytes, 7000);
            }
        }
        break;
    case 2:
        InitializeImage();
        break;
    case 3:
        SystemFlag ^= SYSFLAG_DEBUGPRINT;
        break;
    case 4:
        *(MUSIC_TBL *)work.bytes = D_80014554;
        _PlayMusic(((s32 *)work.bytes)[StageID], 1);
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
        targets = work.music.targets;
        messages = work.music.msg;
        for (; i < 0xA1; i++)
        {
            sprintf((char *)messages[i], D_80097D70, i);
            targets[i].name = messages[i];
            targets[i].value = i;
        }
        ((TAdtSelect *)((u8 *)targets + (i << 3)))->name = 0;
        PlayMusicFormID(AdtSelect(
            D_8001423C, (TAdtSelect *)work.bytes, 0));
        break;
    case 0xA:
        EngageLevel = 3;
        gNannido = DIFFICULTY_EASY;
        break;
    case 0xB:
        EngageLevel = 2;
        gNannido = DIFFICULTY_NORMAL;
        break;
    case 0xC:
        EngageLevel = 1;
        gNannido = DIFFICULTY_HARD;
        break;
    case 0xD:
        __builtin_memcpy(work.bytes, DEBUG_MENU_FILE_LOAD_STOCK_LAYOUT_CHOICES,
                         sizeof(DEBUG_MENU_FILE_LOAD_STOCK_LAYOUT_CHOICES));
        k = AdtSelect(D_800145A8, (TAdtSelect *)work.bytes, 0);
        if (k < 0)
            break;
        STAGE_LAYOUT_NUMBER[0] = k;
        SystemFlag &= ~SYSFLAG_RANDOM_LAYOUT;
        load_layout(STAGE_LAYOUT_NUMBER[0]);
        leLayoutEnemy(0);
        break;
    }
}
