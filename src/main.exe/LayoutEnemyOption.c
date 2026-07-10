#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void LayoutEnemyOption(void);
 *     INFOVIEW.C:849, 55 src lines, frame 192 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct TAdtSelect [11] ItemName
 *     stack sp+104    struct TAdtSelect [3] OkCancel
 *     stack sp+128    struct TAdtSelect [7] ItemName
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 * END PSX.SYM */

/*
 * LayoutEnemyOption (0x8005bb18, 644 bytes) — the debug menu's "enemy layout
 * option" submenu (DoInfoViewProc dispatch case 0): add/remove/layout enemies,
 * clear layout (with ok/cancel confirm), path layout sub-submenu
 * (find/add/reset path on the latched enemy), enemy count message box,
 * camera-owner select.
 *
 * Matching notes (byte-verified; see docs/matching-cookbook.md):
 *  - Table-switch TU (second after BriefingAndInventorySelectionScreen): the
 *    8-entry jump table routes through this object's .rodata, carved in the
 *    yaml at file 0x3F98 (vram 0x80014798). The INCLUDE_ASM stub state needs
 *    ALL split pieces (copy the list from .shake/gen/main.exe/src/<Name>.c —
 *    reverse.py seeds only the first) plus a static const u32 jtbl[8] with
 *    the original words.
 *  - All three menu buffers are PLAIN LOCALS of this function, unlike
 *    DoInfoViewProc's inline helpers: the frame (0x10 args + 0x58 + 0x18 +
 *    0x38 + ra/pad = 0xC0) has no temp-slot overlap, and each buffer address
 *    rematerializes per use because the block-move loop labels break the cse
 *    windows. The ok/cancel copy (m2) really happens at ENTRY, before the
 *    first AdtSelect, though it's only read in case 4.
 *  - Wrapper-struct assignments give the copy shapes for free: 0x58 and 0x38
 *    are the 16-bytes-per-iteration loop + 8-byte tail, 0x18 is the unrolled
 *    3+3 lw/sw batch.
 *  - Outer dispatch: separate `(n & 0xffff) != 0xffff` guard (andi + ori
 *    0xFFFF + beq) then `switch ((s16)n)` — 8 contiguous cases = casesi
 *    tablejump (sltiu 8 bounds + lw/jr); bodies laid out in source order
 *    0..7; case 7 falls into the shared exit.
 *  - Inner path submenu: `k = (s16)AdtSelect(...)` with int k extends ONCE
 *    at the assignment; `if (k != -1)` then a 3-case switch whose SOURCE
 *    order is case 1, case 2, case 0 — recovered from the target's body
 *    layout (bodies are in source order; the balanced compare tree sorts by
 *    value regardless).
 *  - D_80097D44 is this TU's gp small (Build.hs maspsxGpExterns +
 *    permute.py GP_EXTERNS); everything else is absolute externs.
 */

typedef struct { debug_menu_choice e[11]; } MENU_LAYOUT_TBL;   /* 0x58 */
typedef struct { debug_menu_choice e[3];  } MENU_CONFIRM_TBL;  /* 0x18 */
typedef struct { debug_menu_choice e[7];  } MENU_PATH_TBL;     /* 0x38 */

typedef struct
{
    VECTOR TargetVector;         /* 0x00 */
    Humanoid *Owner;             /* 0x10 */
    s32 Mode;                    /* 0x14 */
    s16 DirectionRX;             /* 0x18 */
    s16 DirectionRY;             /* 0x1A */
    s32 OldMode;                 /* 0x1C */
    u8 Valiation;                /* 0x20 */
} TCameraStatus;

extern TCameraStatus CamState;
extern s16 Humans;
/* gp-relative — defined by this (debug-menu) TU; Build.hs maspsxGpExterns */
extern s32 D_80097D44;                      /* latched enemy (leFindEnemy) */

extern MENU_LAYOUT_TBL DEBUG_MENU_ENEMY_LAYOUT_OPTIONS;
extern MENU_PATH_TBL DEBUG_MENU_ENEMY_PATH_SETTING_OPTIONS;
extern MENU_CONFIRM_TBL D_800140A8;         /* ok / cancel */
extern char D_800140C0[];                   /* "enemy layout option" */
extern char D_800140D4[];                   /* "clear ok?" */
extern char D_80014004[];                   /* "path layout option" */
extern char D_800140E0[];                   /* "layout %d enemies" */

extern s32 AdtSelect(char *title, debug_menu_choice *menu, s32 mode);
extern void AddEnemy(void);
extern void leRemoveEnemy(void);
extern void leLayoutEnemy(s32 mode);
extern void leClearLayout(void);
extern s32 leFindEnemy(void);
extern void leAddPath(s32 n, s32 x, s32 y, s32 z);
extern void leResetPath(s32 n);
extern void SelectCameraOwnerOption(void);   /* AdtMessageBox comes from item.h */

void LayoutEnemyOption(void)
{
    s32 n;
    s32 k;
    MENU_LAYOUT_TBL m1;
    MENU_CONFIRM_TBL m2;
    MENU_PATH_TBL m3;

    m1 = DEBUG_MENU_ENEMY_LAYOUT_OPTIONS;
    m2 = D_800140A8;
    n = AdtSelect(D_800140C0, m1.e, 0);
    if ((n & 0xFFFF) != 0xFFFF)
    {
        switch ((s16)n)
        {
        case 0:
            AddEnemy();
            break;
        case 1:
            leRemoveEnemy();
            break;
        case 2:
            leLayoutEnemy(0);
            break;
        case 3:
            leLayoutEnemy(1);
            break;
        case 4:
            if (AdtSelect(D_800140D4, m2.e, 1) == 1)
            {
                leClearLayout();
            }
            break;
        case 5:
            m3 = DEBUG_MENU_ENEMY_PATH_SETTING_OPTIONS;
            k = (s16)AdtSelect(D_80014004, m3.e, 0);
            if (k != -1)
            {
                switch (k)
                {
                case 1:
                    leAddPath(D_80097D44,
                              CamState.Owner->model->locate.coord.t[0],
                              CamState.Owner->model->locate.coord.t[1],
                              CamState.Owner->model->locate.coord.t[2]);
                    break;
                case 2:
                    leResetPath(D_80097D44);
                    break;
                case 0:
                    D_80097D44 = leFindEnemy();
                    break;
                }
            }
            break;
        case 6:
            AdtMessageBox(D_800140E0, Humans);
            break;
        case 7:
            SelectCameraOwnerOption();
            break;
        }
    }
}
