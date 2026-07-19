#include "common.h"
#include "main.exe.h"
#include "gte.h"

/*
 * FUN_80059008 (0x80059008, 0x398 bytes) — a DrawTMD-family fast primitive
 * renderer (TMD primitive code 0x2d), the sibling of the switch arms in
 * FUN_80058a54: it transforms each quad's four vertices through the GTE
 * (RTPT + a backface NCLIP + a per-quad RTPS) and assembles a POLY_GT4
 * (code 0x3c, len 0xc) packet, then hands the working state to FUN_80057b80.
 * Compiled-style GTE function under the restricted gte.h policy
 * (docs/gte-policy.md).
 *
 * This is the flat-colour TMD_P_TNF4 sibling of FUN_80058c70.  Keeping the
 * primitive's original record type is essential: under the common -O2 flags,
 * loop.c derives the target's one record cursor from the named fields.  The
 * old u_short-offset draft created two competing induction pointers and only
 * matched when strength reduction was disabled for this artificial file.
 *
 * The final prologue match comes from two ordinary reused locals.  The loop
 * counts down a body-local copy of param_4, and one scalar initializes both
 * packet words (4 and 0x96).  The latter makes the two constant definitions
 * one multi-set quantity.  cc1 therefore does not give the first definition a
 * birthing priority bump, leaving a sched1 slot for the count copy; sched2 then
 * emits the s4 save/copy after the packet pointer, HWD0, and length leaders.
 * This replaces the old claimed 26-byte scheduling floor with coherent source.
 */

extern void FUN_80057b80(u_long *outv, u_long *packet, int mode);

u_long *FUN_80059008(u_short *param_1, u_long param_2, u_long *param_3, int param_4,
                     volatile u_long param_5, volatile u_long param_6, u_long *param_7)
{
    int iVar1;
    int iVar2;
    u_long uVar3;
    int iVar3;
    int iVar4;
    int cnt;
    int init;
    u_long uVar5;
    u_long *pkt;
    u_long uVar7;
    u_long uVar8;
    SVECTOR *r0;
    TMD_P_TNF4 *primitive;
    u_long *puVar5;
    u_char uVar6;
    SVECTOR *r0_00;
    SVECTOR *r2;
    SVECTOR *r1;
    u_long *local_38;

    pkt = param_7;
    iVar1 = HWD0;
    init = 4;
    *pkt = init;
    puVar5 = pkt + 0x38;
    local_38 = puVar5;
    iVar2 = VWD0;
    *(short *)(pkt + 0xd) = (short)(iVar1 / 2);
    *(short *)((int)pkt + 0x36) = (short)(iVar2 / 2);
    uVar5 = param_6;
    uVar7 = param_5;
    uVar3 = *(u_long *)(uVar5 + 4);
    init = 0x96;
    pkt[8] = init;
    *(u_char *)((int)pkt + 0x4f) = 0xc;
    iVar4 = 0x3c;
    pkt[3] = uVar7;
    pkt[5] = (u_long)param_3;
    *(u_char *)((int)pkt + 0x53) = iVar4;
    pkt[4] = uVar3;
    cnt = param_4;
    if (cnt != 0) {
        r0 = (SVECTOR *)(pkt + 0x20);
        r1 = (SVECTOR *)(pkt + 0x26);
        r2 = (SVECTOR *)(pkt + 0x2c);
        r0_00 = (SVECTOR *)(pkt + 0x32);
        iVar3 = iVar4;
        primitive = (TMD_P_TNF4 *)param_1;
        do {
            *(short *)(pkt + 0x20) = *(u_short *)(primitive->v0 * 8 + param_2);
            *(short *)((int)pkt + 0x82) = *(u_short *)(primitive->v0 * 8 + param_2 + 2);
            *(short *)(pkt + 0x21) = *(u_short *)(primitive->v0 * 8 + param_2 + 4);
            *(short *)(pkt + 0x26) = *(u_short *)(primitive->v1 * 8 + param_2);
            *(short *)((int)pkt + 0x9a) = *(u_short *)(primitive->v1 * 8 + param_2 + 2);
            *(short *)(pkt + 0x27) = *(u_short *)(primitive->v1 * 8 + param_2 + 4);
            *(short *)(pkt + 0x2c) = *(u_short *)(primitive->v2 * 8 + param_2);
            *(short *)((int)pkt + 0xb2) = *(u_short *)(primitive->v2 * 8 + param_2 + 2);
            *(short *)(pkt + 0x2d) = *(u_short *)(primitive->v2 * 8 + param_2 + 4);
            *(short *)(pkt + 0x32) = *(u_short *)(primitive->v3 * 8 + param_2);
            *(short *)((int)pkt + 0xca) = *(u_short *)(primitive->v3 * 8 + param_2 + 2);
            *(short *)(pkt + 0x33) = *(u_short *)(primitive->v3 * 8 + param_2 + 4);
            *local_38 = (u_long)r0;
            local_38[1] = (u_long)r1;
            local_38[2] = (u_long)r2;
            local_38[3] = (u_long)r0_00;
            gte_ldv3(r0, r1, r2);
            gte_rtpt();
            *(short *)(pkt + 0x25) = *(u16 *)&primitive->tu0;
            *(short *)(pkt + 0x2b) = *(u16 *)&primitive->tu1;
            uVar8 = (u_long)(pkt + 0x23);
            gte_stsxy3((u_long *)uVar8, pkt + 0x29, pkt + 0x2f);
            gte_nclip();
            *(short *)(pkt + 0x31) = *(u16 *)&primitive->tu2;
            *(short *)(pkt + 0x37) = *(u16 *)&primitive->tu3;
            gte_stopz(pkt + 6);
            if (0 < (int)pkt[6]) {
                gte_ldv0(r0_00);
                gte_rtps();
                pkt[0x22] = *(u_long *)&primitive->r0;
                uVar6 = (u_char)iVar3;
                *(u_char *)((int)pkt + 0x8b) = uVar6;
                pkt[0x28] = *(u_long *)&primitive->r0;
                *(u_char *)((int)pkt + 0xa3) = uVar6;
                pkt[0x2e] = *(u_long *)&primitive->r0;
                *(u_char *)((int)pkt + 0xbb) = uVar6;
                pkt[0x34] = *(u_long *)&primitive->r0;
                *(u_char *)((int)pkt + 0xd3) = uVar6;
                gte_stsxy(pkt + 0x35);
                uVar3 = (u_long)(pkt + 0x24);
                uVar8 = (u_long)(pkt + 0x2a);
                uVar7 = (u_long)(pkt + 0x30);
                gte_stsz4((u_long *)uVar3, (u_long *)uVar8, (u_long *)uVar7, pkt + 0x36);
                *(short *)((int)pkt + 0x5a) = primitive->clut;
                *(short *)((int)pkt + 0x66) = primitive->tpage;
                FUN_80057b80(puVar5, pkt, 0);
            }
            cnt = cnt + -1;
            primitive++;
        } while (cnt != 0);
    }
    return (u_long *)pkt[5];
}
