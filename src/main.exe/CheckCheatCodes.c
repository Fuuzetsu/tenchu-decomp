#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

/*
 * CheckCheatCodes (0x8004b354) — matches the just-entered button record
 * `rec` (n halfwords) against two hidden cheat sequences. If it equals the
 * first (D_8008E4F0), it plays a chime and opens the SAME debug item-cheat
 * menus DoInfoViewProc's ItemAddMenu uses (pick an item, then a count) and
 * adds the chosen count to the current character's carried-item stock. If it
 * equals the second (D_8008E4C4), it sets SystemFlag bit 1. Either match
 * plays SoundEx(0, seid) at the end (seid = 0x4c for the item cheat, 10 for
 * the flag cheat); the item cheat ALSO plays SoundEx(0,10) up front.
 *
 * The two menu tables + the `CamState.Owner->item[]` `+=` idiom are verbatim
 * from DoInfoViewProc.c's ItemAddMenu (same TU family):
 * `*(MENU_ITEM_TBL *)mi = DEBUG_MENU_ITEM_CHOICE_OPTIONS;` copies the 0xC8
 * table as one struct assignment (emit_block_move 16-byte loop + 8-byte
 * tail), and the second AdtSelect's result is added to item[sel] where sel is
 * the first AdtSelect's result (captured into the callee-saved reg in the
 * second call's delay slot).
 *
 * Matching notes:
 *  - Both cheat paths end in SoundEx(0, seid) but written as TWO explicit
 *    per-branch calls (`SoundEx(0, 0x4c)` / `SoundEx(0, 10)`), NOT a shared
 *    `SoundEx(0, seid)` after the if/else. cross-jump merges only the common
 *    `jal SoundEx` tail (leaving the delay slot a nop), so each branch
 *    materialises its own `a0 = 0`; the shared-call form instead hoists a
 *    single `a0 = 0` into the merged jal's delay slot (17-byte diff).
 *  - **The item pointer MUST be reached as `CamState.Owner` (a nonzero +0x10
 *    struct-member load), NOT the offset-0 alias symbol
 *    CURRENTLY_SELECTED_CHARACTER_STATE_PTR (@0x80089f00 == &CamState.Owner).**
 *    They resolve to the same address, but the alias compiles the pointer
 *    load offset-0 (`lui a2,%hi; lw a2,%lo(a2)` — hi folded into the dest
 *    register), while the +0x10 member load splits the base into a separate
 *    register (`lui v1,%hi(CamState); lw a2,0x10+%lo(v1)`), which is the
 *    target's exact 2-byte-different reload choice. NEW RULE candidate:
 *    an offset-0 alias vs the enclosing struct's nonzero-offset member is a
 *    real source lever over the -msplit-addresses hi-register choice.
 */
typedef struct { TAdtSelect e[25]; } MENU_ITEM_TBL;  /* 0xC8 */
typedef struct { TAdtSelect e[4]; } MENU_COUNT_TBL;  /* 0x20 */

extern MENU_ITEM_TBL DEBUG_MENU_ITEM_CHOICE_OPTIONS;
extern MENU_COUNT_TBL D_800124CC;
extern char D_800124C0[]; /* "select item" */
extern char D_800124EC[]; /* "number of" */
extern u16 D_8008E4F0[];  /* cheat sequence 1 */
extern u16 D_8008E4C4[];  /* cheat sequence 2 */
extern TCameraStatus CamState;
extern u32 SystemFlag;

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern s32 memcmp(void *a, void *b, s32 n);

void CheckCheatCodes(s16 *rec, int n)
{
    s32 sel;
    u8 mi[0xC8];

    if (memcmp(rec, D_8008E4F0, n << 1) == 0) {
        SoundEx(0, 10);
        *(MENU_ITEM_TBL *)mi = DEBUG_MENU_ITEM_CHOICE_OPTIONS;
        sel = AdtSelect(D_800124C0, (TAdtSelect *)mi, 0);
        *(MENU_COUNT_TBL *)mi = D_800124CC;
        CamState.Owner->item[sel] +=
            AdtSelect(D_800124EC, (TAdtSelect *)mi, 0);
        SoundEx(0, 0x4c);
    } else {
        if (memcmp(rec, D_8008E4C4, n << 1) != 0) {
            return;
        }
        SystemFlag |= 2;
        SoundEx(0, 10);
    }
}

// triage: EASY — 91 insns, 1 loop, 3 callees, ~0.16 to AddItem2
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void CheckCheatCodes(undefined4 param_1,int param_2)
//
// {
//   adt_menu_entry_t *paVar1;
//   TAdtSelect *pTVar2;
//   int iVar3;
//   adt_menu_entry_t *paVar4;
//   TAdtSelect *pTVar5;
//   short seid;
//   ulong uVar6;
//   char *pcVar7;
//   ulong uVar8;
//   TAdtSelect local_d8;
//   char *local_d0;
//   ulong local_cc;
//   char *local_c8;
//   ulong local_c4 [45];
//
//   iVar3 = memcmp(param_1,&DAT_8008e4f0,param_2 << 1);
//   if (iVar3 == 0) {
//     SoundEx((VECTOR *)0x0,10);
//     paVar1 = adt_menu_entry_t_ARRAY_800123f8;
//     pTVar2 = &local_d8;
//     do {
//       pTVar5 = pTVar2;
//       paVar4 = paVar1;
//       uVar6 = paVar4->index;
//       pcVar7 = paVar4[1].label;
//       uVar8 = paVar4[1].index;
//       pTVar5->name = paVar4->label;
//       pTVar5->value = uVar6;
//       pTVar5[1].name = pcVar7;
//       pTVar5[1].value = uVar8;
//       paVar1 = paVar4 + 2;
//       pTVar2 = pTVar5 + 2;
//     } while (paVar4 + 2 != adt_menu_entry_t_ARRAY_800123f8 + 0x18);
//     uVar6 = paVar4[2].index;
//     pTVar5[2].name = adt_menu_entry_t_ARRAY_800123f8[0x18].label;
//     pTVar5[2].value = uVar6;
//     uVar6 = AdtSelect("select item",&local_d8,0);
//     local_d8.name = PTR_s_10_800124cc;
//     local_d8.value = DAT_800124d0;
//     local_d0 = PTR_s_100_800124d4;
//     local_cc = DAT_800124d8;
//     local_c8 = PTR_s_FULL_800124dc;
//     local_c4[0] = DAT_800124e0;
//     local_c4[1] = DAT_800124e4;
//     local_c4[2] = DAT_800124e8;
//     uVar8 = AdtSelect("number of",&local_d8,0);
//     seid = 0x4c;
//     (CamState.Owner)->item[uVar6] = (CamState.Owner)->item[uVar6] + (char)uVar8;
//   }
//   else {
//     iVar3 = memcmp(param_1,&DAT_8008e4c4,param_2 << 1);
//     if (iVar3 != 0) {
//       return;
//     }
//     seid = 10;
//     SystemFlag = SystemFlag | 2;
//   }
//   SoundEx((VECTOR *)0x0,seid);
//   return;
// }
