#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadCard(int target, unsigned char *name);
 *     MEMCARD.C:119, 35 src lines, frame 8448 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       int target
 *     param $a1       unsigned char * name
 *     stack sp+24     unsigned char [200] fn
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     stack sp+224    unsigned char [8192] block
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

typedef struct
{
    u8 bytes[TENCHU_PERSISTENT_STATE_SIZE];
} LoadCardPersistentBlob;

extern char CardPathFormat[];

extern void *valloc(u32 size);
extern void vfree(void *ptr);
extern s32 MemCardAccept(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);
extern s32 MemCardReadFile(s32 chan, char *name, void *data, s32 offset,
                           s32 size);
extern int sprintf(char *buf, char *fmt, ...);

/*
 * The two apparent Ghidra buffers are one 8 KiB card block: the card header
 * occupies its first 0x200 bytes and the persistent payload begins at
 * `block + sizeof(TCardHeader)`. Assigning that payload as one 0xe70-byte
 * struct reproduces
 * the compiler's aligned/unaligned copy-loop pair.
 */
s16 LoadCard(s32 target, u8 *name)
{
    void *temp;
    u8 fn[200];
    u8 block[BLOCKSIZE];
    s32 cmd;
    s32 result;

    temp = valloc(BLOCKSIZE);
    result = MemCardAccept(0);
    MemCardSync(0, &cmd, &result);
    sprintf(fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardReadFile(0, fn, block, 0, BLOCKSIZE);
    MemCardSync(0, &cmd, &result);
    if (result != 0)
    {
        vfree(temp);
        temp = 0;
    }
    else
    {
        *(LoadCardPersistentBlob *)TENCHU_PERSISTENT_STATE_ADDRESS =
            *(LoadCardPersistentBlob *)(block + sizeof(TCardHeader));
    }
    vfree(temp);
    return result;
}

// triage: MEDIUM — 69 insns, 1 loop, frame 0x2100, 6 callees, ~0.08 to UpdateOrnament
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x2100 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short LoadCard(int target,uchar *name)
//
// {
//   undefined *pt;
//   undefined4 *puVar1;
//   TLinkInfo *pPVar2;
//   undefined4 uVar3;
//   undefined4 uVar4;
//   undefined4 uVar5;
//   char acStack_20e8 [200];
//   ulong auStack_2020 [128];
//   undefined4 local_1e20 [1920];
//   long lStack_20;
//   long local_1c;
//
//   pt = valloc(0x2000);
//   local_1c = MemCardAccept(0);
//   MemCardSync(0,&lStack_20,&local_1c);
//   sprintf(acStack_20e8,&DAT_80097d08,PTR_s_BISLPS_01901_80097d04,name);
//   local_1c = MemCardReadFile(0,acStack_20e8,auStack_2020,0,0x2000);
//   MemCardSync(0,&lStack_20,&local_1c);
//   pPVar2 = &PersistentState;
//   if (local_1c == 0) {
//     puVar1 = local_1e20;
//     do {
//       uVar3 = puVar1[1];
//       uVar4 = puVar1[2];
//       uVar5 = puVar1[3];
//       *(undefined4 *)pPVar2 = *puVar1;
//       *(undefined4 *)&pPVar2->field_0x4 = uVar3;
//       *(undefined4 *)&pPVar2->field_0x8 = uVar4;
//       *(undefined4 *)&pPVar2->field_0xc = uVar5;
//       puVar1 = puVar1 + 4;
//       pPVar2 = (TLinkInfo *)&pPVar2->field_0x10;
//     } while (puVar1 != local_1e20 + 0x39c);
//   }
//   else {
//     vfree(pt);
//     pt = (undefined *)0x0;
//   }
//   vfree(pt);
//   return (short)local_1c;
// }
