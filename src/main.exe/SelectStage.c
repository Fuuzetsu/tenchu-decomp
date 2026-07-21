#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SelectStage(void);
 *     INFOVIEW.C:675, 15 src lines, frame 1016 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     stack sp+16     struct TAdtSelect [10] StageSelect
 *     stack sp+96     unsigned char [9][100] name
 *     reg   $s0       int i
 * END PSX.SYM */

/*
 * SelectStage (0x8005c404) — builds language, character, and stage debug
 * menus, then writes the selected values into the caller's persistent-state
 * record. A stage selection above 10 is the menu's retry entry, so all three
 * prompts repeat until a real stage is chosen.
 *
 * Matching notes:
 *  - Retail takes `TLinkInfo *ps`, despite the demo symbol's stale
 *    `void SelectStage(void)` prototype. Both retail callers pass the state
 *    pointer in a0, and this body stores its three results at +0x5e/+4/+5.
 *  - The local declarations reproduce the full 0x500-byte working window:
 *    language[5] at sp+0x10, player[3] at sp+0x38, stage[14] at sp+0x50,
 *    and name[11][100] at sp+0xc0. Their padding plus the saved-register
 *    area gives the target's 0x528 frame.
 *  - Capturing `StageConfig[i].uid` once keeps it live across `sprintf` in
 *    s0 and lets both stage-entry stores reuse one computed address. Reading
 *    the field separately at each use was three instructions too long.
 *  - The stage scan is deliberately a `while (1)` with an explicit top
 *    break. A natural `for` is folded into a bottom-tested loop and is two
 *    instructions short; retail keeps the top test and unconditional back
 *    jump, with `i++` in its delay slot.
 *  - `debug_menu_select_stage__override__prt_8005c4d8_8cf8befb` is an
 *    interior call-site prototype marker, not a second function. Its asm
 *    piece falls straight into `sprintf` and branches back into the first
 *    piece; this single C body matches the complete 420-byte carve.
 */

typedef struct
{
    TAdtSelect e[5];
} MenuLanguageTable;

typedef struct
{
    TAdtSelect e[3];
} MenuPlayerTable;

typedef struct
{
    u8 uid;
    u8 pad[3];
    char *name;
    char *path;
    s32 px;
    s32 py;
    s32 pz;
    s32 pr;
} StageConfigEntry;

extern MenuLanguageTable DEBUG_MENU_LANGUAGE_CHOICES;
extern MenuPlayerTable D_800141F4;
extern StageConfigEntry StageConfig[];
extern char D_80097DC0[];
extern char D_80097DC8[];
extern char D_8001420C[];
extern char D_8001421C[];
extern char D_8001422C[];

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern int sprintf(char *buf, char *fmt, ...);

void SelectStage(TLinkInfo *ps)
{
    MenuLanguageTable language;
    MenuPlayerTable player;
    TAdtSelect stage[14];
    char name[11][100];
    s32 i;
    s32 uid;

    language = DEBUG_MENU_LANGUAGE_CHOICES;
    player = D_800141F4;
    i = 0;
    while (1) {
        if (i >= 11) {
            break;
        }
        uid = StageConfig[i].uid;
        sprintf(name[i], D_80097DC0, uid, StageConfig[i].name);
        stage[uid].name = name[i];
        stage[uid].value = i;
        i++;
    }
    stage[i].name = D_80097DC8;
    stage[i].value = 11;
    stage[i + 1].name = NULL;

    do {
        ps->language = AdtSelect(D_8001420C, language.e, 0);
        ps->chr = AdtSelect(D_8001421C, player.e, 0);
        ps->stage = AdtSelect(D_8001422C, stage, 0);
    } while (ps->stage > 10);
}

// triage: MEDIUM — 105 insns, 2 loop, frame 0x528, 2 callees, ~0.12 to AddItem2
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Stack objects: 0x528 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// void FUN_8005c404(int param_1)
//
// {
//   uchar **ppuVar1;
//   undefined **ppuVar2;
//   TAdtSelect *pTVar3;
//   undefined **ppuVar4;
//   TAdtSelect *pTVar5;
//   undefined *puVar6;
//   char *pcVar7;
//   undefined *puVar8;
//   uint uVar9;
//   TStageConfig *pTVar10;
//   ulong uVar11;
//   TAdtSelect local_518;
//   char *local_510;
//   ulong local_50c;
//   char *local_508;
//   ulong local_504 [5];
//   TAdtSelect local_4f0;
//   undefined *local_4e8;
//   undefined4 local_4e4;
//   undefined4 local_4e0;
//   undefined4 local_4dc;
//   TAdtSelect local_4d8 [14];
//   char acStack_468 [1104];
//
//   ppuVar2 = &PTR_s_JAPANESE_800141c0;
//   pTVar3 = &local_518;
//   do {
//     pTVar5 = pTVar3;
//     ppuVar4 = ppuVar2;
//     puVar6 = ppuVar4[1];
//     pcVar7 = ppuVar4[2];
//     puVar8 = ppuVar4[3];
//     pTVar5->name = *ppuVar4;
//     pTVar5->value = (ulong)puVar6;
//     pTVar5[1].name = pcVar7;
//     pTVar5[1].value = (ulong)puVar8;
//     ppuVar2 = ppuVar4 + 4;
//     pTVar3 = pTVar5 + 2;
//   } while (ppuVar4 + 4 != (undefined **)&UNK_800141e0);
//   pcVar7 = acStack_468;
//   pTVar10 = StageConfig;
//   puVar6 = ppuVar4[5];
//   pTVar5[2].name = _UNK_800141e0;
//   pTVar5[2].value = (ulong)puVar6;
//   local_4f0.name = PTR_s_RIKIMARU_800141f4;
//   local_4f0.value = DAT_800141f8;
//   local_4e8 = PTR_s_AYAME_800141fc;
//   local_4e4 = DAT_80014200;
//   local_4e0 = DAT_80014204;
//   local_4dc = DAT_80014208;
//   for (uVar11 = 0; (int)uVar11 < 0xb; uVar11 = uVar11 + 1) {
//     uVar9 = (uint)pTVar10->uid;
//     ppuVar1 = &pTVar10->name;
//     pTVar10 = pTVar10 + 1;
//     sprintf(pcVar7,s__2d__s_80097dc0,uVar9,*ppuVar1);
//     local_4d8[uVar9].name = pcVar7;
//     pcVar7 = pcVar7 + 100;
//     local_4d8[uVar9].value = uVar11;
//   }
//   local_4d8[uVar11].name = &DAT_80097dc8;
//   local_4d8[uVar11].value = 0xb;
//   local_4d8[uVar11 + 1].name = (char *)0x0;
//   do {
//     uVar11 = AdtSelect("language select",&local_518,0);
//     *(char *)(param_1 + 0x5e) = (char)uVar11;
//     uVar11 = AdtSelect("player select",&local_4f0,0);
//     *(char *)(param_1 + 4) = (char)uVar11;
//     uVar11 = AdtSelect("stage select",local_4d8,0);
//     *(char *)(param_1 + 5) = (char)uVar11;
//   } while (10 < (uVar11 & 0xff));
//   return;
// }
