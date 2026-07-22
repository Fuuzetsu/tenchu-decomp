#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SaveCard(int target, unsigned char *name, void *mem, long size);
 *     MEMCARD.C:157, 38 src lines, frame 8464 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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
 *     param $s5       unsigned char * name
 *     param $s6       void * mem
 *     param $s7       long size
 *     stack sp+24     unsigned char [200] fn
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     reg   $s4       long chan
 *     stack sp+224    unsigned char [8192] block
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $s3       void * data
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $t1       unsigned char * icon3
 *     reg   $s1       unsigned char * icon2
 *     reg   $s0       unsigned char * icon1
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

typedef struct
{
    u8 bytes[0x20];
} SaveCardPalette;

typedef struct
{
    u8 bytes[0x80];
} SaveCardIcon;

extern unsigned char *TENCHU_ID;
extern char CardPathFormat[];
extern char D_80013BE4[];

extern void *memset(void *dst, s32 value, u32 size);
extern void *memcpy(void *dst, const void *src, u32 size);
extern int sprintf(char *buf, char *fmt, ...);
extern u8 *GetArcData(s32 id);
extern s32 MemCardCreateFile(s32 chan, char *name, s32 blocks);
extern s32 MemCardWriteFile(s32 chan, char *name, void *data, s32 offset,
                            s32 size);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

/*
 * The 0x200-byte card header and payload are two views into a single 8 KiB
 * block.  Struct assignment deliberately preserves the original compiler's
 * aligned/unaligned icon-copy loop pairs.
 */
s16 SaveCard(s32 target, u8 *name, void *mem, s32 size, s16 write_data)
{
    u8 fn[200];
    u8 block[BLOCKSIZE];
    s32 cmd;
    s32 result;
    s32 chan;
    TCardHeader *header;
    void *data;
    u8 *icon1;
    u8 *icon2;
    u8 *icon3;

    header = (TCardHeader *)block;

    header->Magic[0] = 0x53;
    header->Magic[1] = 0x43;
    header->Type = 0x13;
    header->BlockEntry = 1;
    memset(header->Title, 0, sizeof(header->Title));
    sprintf(header->Title, D_80013BE4);
    memset(header->reserve, 0, sizeof(header->reserve));

    icon1 = GetArcData(0x16);
    icon2 = GetArcData(0x17);
    chan = 0;
    icon3 = GetArcData(0x18);
    data = block + sizeof(TCardHeader);
    *(SaveCardPalette *)header->Clut = *(SaveCardPalette *)(icon1 + 0x14);
    *(SaveCardIcon *)header->Icon[0] = *(SaveCardIcon *)(icon1 + 0x40);
    *(SaveCardIcon *)header->Icon[1] = *(SaveCardIcon *)(icon2 + 0x40);
    *(SaveCardIcon *)header->Icon[2] = *(SaveCardIcon *)(icon3 + 0x40);

    sprintf(fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardCreateFile(chan, fn, 1);
    if ((result == 0 || result == 6) && write_data != 0)
    {
        memcpy(data, mem, size);
        result = MemCardWriteFile(chan, fn, block, 0, BLOCKSIZE);
        MemCardSync(0, &cmd, &result);
    }
    return result;
}

// triage: HARD — 239 insns, 6 loop, frame 0x2110, 7 callees, ~0.09 to AddItem2
// likely-relevant cookbook sections:
//   - Loops: 6 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x2110 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short SaveCard(int target,uchar *name,undefined *mem,long size)
//
// {
//   ulong *puVar1;
//   ulong *puVar2;
//   ulong *puVar3;
//   ulong *puVar4;
//   ulong *puVar5;
//   ulong uVar6;
//   ulong uVar7;
//   ulong uVar8;
//   short in_stack_00000010;
//   char acStack_20f8 [200];
//   undefined1 local_2030;
//   undefined1 local_202f;
//   undefined1 local_202e;
//   undefined1 local_202d;
//   uchar auStack_202c [64];
//   uchar auStack_1fec [28];
//   ulong local_1fd0;
//   ulong local_1fcc;
//   ulong local_1fc8;
//   ulong local_1fc4;
//   ulong local_1fc0;
//   ulong local_1fbc;
//   ulong local_1fb8;
//   ulong local_1fb4;
//   ulong local_1fb0 [32];
//   ulong local_1f30 [32];
//   ulong local_1eb0 [32];
//   uchar auStack_1e30 [7680];
//   long lStack_30;
//   long local_2c;
//
//   local_2030 = 0x53;
//   local_202f = 0x43;
//   local_202e = 0x13;
//   local_202d = 1;
//   memset(auStack_202c,'\0',0x40);
//   sprintf((char *)auStack_202c,&DAT_80013be4);
//   memset(auStack_1fec,'\0',0x1c);
//   puVar1 = GetArcData(0x16);
//   puVar2 = GetArcData(0x17);
//   puVar3 = GetArcData(0x18);
//   puVar5 = local_1fb0;
//   puVar4 = puVar1 + 0x10;
//   local_1fd0 = puVar1[5];
//   local_1fcc = puVar1[6];
//   local_1fc8 = puVar1[7];
//   local_1fc4 = puVar1[8];
//   local_1fc0 = puVar1[9];
//   local_1fbc = puVar1[10];
//   local_1fb8 = puVar1[0xb];
//   local_1fb4 = puVar1[0xc];
//   if (((uint)puVar4 & 3) == 0) {
//     do {
//       uVar6 = puVar4[1];
//       uVar7 = puVar4[2];
//       uVar8 = puVar4[3];
//       *puVar5 = *puVar4;
//       puVar5[1] = uVar6;
//       puVar5[2] = uVar7;
//       puVar5[3] = uVar8;
//       puVar4 = puVar4 + 4;
//       puVar5 = puVar5 + 4;
//     } while (puVar4 != puVar1 + 0x30);
//   }
//   else {
//     do {
//       uVar6 = puVar4[1];
//       uVar7 = puVar4[2];
//       uVar8 = puVar4[3];
//       *puVar5 = *puVar4;
//       puVar5[1] = uVar6;
//       puVar5[2] = uVar7;
//       puVar5[3] = uVar8;
//       puVar4 = puVar4 + 4;
//       puVar5 = puVar5 + 4;
//     } while (puVar4 != puVar1 + 0x30);
//   }
//   puVar4 = local_1f30;
//   puVar1 = puVar2 + 0x10;
//   if (((uint)puVar1 & 3) == 0) {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar4 = *puVar1;
//       puVar4[1] = uVar6;
//       puVar4[2] = uVar7;
//       puVar4[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar4 = puVar4 + 4;
//     } while (puVar1 != puVar2 + 0x30);
//   }
//   else {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar4 = *puVar1;
//       puVar4[1] = uVar6;
//       puVar4[2] = uVar7;
//       puVar4[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar4 = puVar4 + 4;
//     } while (puVar1 != puVar2 + 0x30);
//   }
//   puVar2 = local_1eb0;
//   puVar1 = puVar3 + 0x10;
//   if (((uint)puVar1 & 3) == 0) {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar2 = *puVar1;
//       puVar2[1] = uVar6;
//       puVar2[2] = uVar7;
//       puVar2[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar2 = puVar2 + 4;
//     } while (puVar1 != puVar3 + 0x30);
//   }
//   else {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar2 = *puVar1;
//       puVar2[1] = uVar6;
//       puVar2[2] = uVar7;
//       puVar2[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar2 = puVar2 + 4;
//     } while (puVar1 != puVar3 + 0x30);
//   }
//   sprintf(acStack_20f8,&DAT_80097d08,PTR_s_BISLPS_01901_80097d04,name);
//   local_2c = MemCardCreateFile(0,acStack_20f8,1);
//   if (((local_2c == 0) || (local_2c == 6)) && (in_stack_00000010 != 0)) {
//     memcpy(auStack_1e30,mem,size);
//     local_2c = MemCardWriteFile(0,acStack_20f8,(ulong *)&local_2030,0,0x2000);
//     MemCardSync(0,&lStack_30,&local_2c);
//   }
//   return (short)local_2c;
// }
