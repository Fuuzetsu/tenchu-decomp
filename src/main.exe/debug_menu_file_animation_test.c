#include "common.h"
#include "main.exe.h"

/*
 * debug_menu_file_animation_test (0x800501b0) — scans CVAdata for event
 * records whose mode is zero, formats each event id into a menu label, and
 * runs the selected sequence. The final -1 menu value is the cancel entry.
 *
 * MATCHED (65/65 instructions, complete 260-byte carve). Matching notes:
 *  - `text[0x100]` at sp+0x10 followed by `menu[64]` at sp+0x110 fills the
 *    exact 0x300-byte working window and produces the target's 0x328 frame.
 *  - Walking a 12-byte CVAType pointer while using both mode and id lets
 *    loop.c retain parallel s2/s0 induction cursors. A natural `while`
 *    supplies the target's entry guard and conditional bottom back-edge.
 *  - CVAdata is gp-relative in this TU. The format string at 0x80097cd0
 *    needed an explicit linker binding because the interior override split
 *    left its address as raw lui/addiu immediates instead of a data symbol.
 *  - `debug_menu_file_animation_test__override__prt_800501f8_8d2134c3` is
 *    only a call-site prototype marker: piece 1 falls directly into sprintf,
 *    and the second piece branches back into the first. One C body spans it.
 */

extern char D_80097CD0[];
extern char D_80097CD4[];
extern char D_80013654[];

extern int sprintf(char *buf, char *fmt, ...);
extern int strlen(char *s);
extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern s16 CVAsequence(s16 sid);

void debug_menu_file_animation_test(void)
{
    char text[0x100];
    TAdtSelect menu[64];
    CVAType *event;
    char *buffer;
    s32 count;
    s32 selection;

    buffer = text;
    event = CVAdata;
    count = 0;
    while (event->mode != -1) {
        if (event->mode == 0) {
            sprintf(buffer, D_80097CD0, event->id);
            menu[count].name = buffer;
            menu[count].value = event->id;
            count++;
            buffer += strlen(buffer) + 1;
        }
        event++;
    }
    menu[count].name = D_80097CD4;
    menu[count].value = -1;
    count++;
    menu[count].name = NULL;

    selection = AdtSelect(D_80013654, menu, 0);
    if (selection != -1) {
        CVAsequence((s16)selection);
    }
}

// triage: MEDIUM — 65 insns, 1 loop, frame 0x328, 4 callees, ~0.06 to SetupAfterimage
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x328 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_800501b0(void)
//
// {
//   short sVar1;
//   int iVar2;
//   ulong uVar3;
//   short *psVar4;
//   char *buffer;
//   short *psVar5;
//   int iVar6;
//   char acStack_318 [256];
//   TAdtSelect local_218 [64];
//
//   buffer = acStack_318;
//   sVar1 = *DAT_80097cb8;
//   iVar6 = 0;
//   if (*DAT_80097cb8 != -1) {
//     psVar4 = DAT_80097cb8 + 1;
//     psVar5 = DAT_80097cb8;
//     iVar2 = iVar6;
//     do {
//       iVar6 = iVar2;
//       if (sVar1 == 0) {
//         sprintf(buffer,&DAT_80097cd0,(int)*psVar4);
//         iVar6 = iVar2 + 1;
//         local_218[iVar2].name = buffer;
//         local_218[iVar2].value = (int)*psVar4;
//         iVar2 = strlen(buffer);
//         buffer = buffer + iVar2 + 1;
//       }
//       psVar5 = psVar5 + 6;
//       sVar1 = *psVar5;
//       psVar4 = psVar4 + 6;
//       iVar2 = iVar6;
//     } while (*psVar5 != -1);
//   }
//   local_218[iVar6].name = s_cancel_80097cd4;
//   local_218[iVar6].value = 0xffffffff;
//   local_218[iVar6 + 1].name = (char *)0x0;
//   uVar3 = AdtSelect("event test",local_218,0);
//   if (uVar3 != 0xffffffff) {
//     CVAsequence((short)uVar3);
//   }
//   return;
// }
