#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MakeCameraPosition(struct VECTOR *orgpos, struct SVECTOR *orgrot, struct SVECTOR *campos, struct SVECTOR *ref, struct GsRVIEW2 *vDif);
 *     CAMERA.C:533, 66 src lines, frame 104 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s4       struct VECTOR * orgpos
 *     param $s5       struct SVECTOR * orgrot
 *     param $s7       struct SVECTOR * campos
 *     param $fp       struct SVECTOR * ref
 *     param stack+16  struct GsRVIEW2 * vDif
 *     reg   $v1       int fwRot
 *     reg   $s2       struct SVECTOR * tmp
 *     reg   $s6       struct SVECTOR * prot
 *     reg   $s1       struct MATRIX * pmat
 *     reg   $s4       struct VECTOR * pos
 *     reg   $s5       struct SVECTOR * rot
 *     reg   $a0       int d
 *     reg   $v1       int ret
 *     reg   $s0       int dz
 *     reg   $s3       int dx
 *     reg   $v0       int y2
 *     reg   $s0       int y1
 *     stack sp+24     struct SVECTOR dvec
 *     stack sp+32     struct VECTOR ret
 *     stack sp+48     struct VECTOR start
 *     reg   $a2       int N
 *     reg   $s4       struct VECTOR * start
 *     reg   $a0       struct SVECTOR * v
 *     reg   $s1       int n
 *     reg   $s4       int pz
 *     reg   $s2       int py
 *     reg   $s3       int px
 *     reg   $s5       int vz
 *     reg   $s6       int vy
 *     reg   $s7       int vx
 *     reg   $s0       int i
 *     stack sp+120    struct GsRVIEW2 * vdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, all 660 bytes / 165 instructions exact.
 *
 * TransformCameraPoint's pointer formal keeps the two expanded output
 * addresses independent, so cc1 rematerializes &vc and &vd for the following
 * FUN_80039ddc call. The late tp alias supplies the target's s0=&target base
 * without extending its lifetime over the earlier calls. Spelling each
 * component delta as -va + vb preserves the target's independent-load order.
 * Retail removed the demo's fourth `SVECTOR *ref` input; its fourth argument
 * is the surviving fifth `GsRVIEW2 *vDif` output, as shown by all six stores.
 */

extern TMakeDifInfo ref;
extern TMakeDifInfo pnt;
extern SVECTOR scratch_rot_1f800040;
extern s32 scratch_trans_1f800094[2];

extern short FUN_8002fd9c(Humanoid *h);
extern void AntiWall(GsRVIEW2 *vinfo, GsRVIEW2 *target);
extern void MakeDifSub(VECTOR *src, VECTOR *target, VECTOR *dest, TMakeDifInfo *info);

static inline void TransformCameraPoint(SVECTOR *point, VECTOR *result,
                                        long *flag)
{
    RotTrans(point, result, flag);
}

s32 MakeCameraPosition(VECTOR *orgpos, SVECTOR *orgrot, SVECTOR *campos, GsRVIEW2 *vDif)
{
    GsRVIEW2 target;
    GsRVIEW2 *tp;
    VECTOR va, vb, vc, vd;
    long flag[2];
    TCameraStatus *cs;
    s32 fwRot;
    s32 d1, d2, d3;

    cs = &CamState;
    scratch_rot_1f800040.vx = orgrot->vx + FUN_8002fd9c(cs->Owner);
    scratch_rot_1f800040.vy = orgrot->vy;
    scratch_rot_1f800040.vz = orgrot->vz;
    RotMatrixYXZ((SVECTOR *)TENCHU_SCRATCHPAD(0x40),
                 (MATRIX *)TENCHU_SCRATCHPAD(0x80));
    scratch_trans_1f800094[0] = orgpos->vx;
    scratch_trans_1f800094[1] = orgpos->vy;
    scratch_trans_1f800094[2] = orgpos->vz;
    SetRotMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
    SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));

    RotTrans(campos, &va, flag);
    RotTrans(campos + 1, &vb, flag);
    TransformCameraPoint(campos + 2, &vc, flag);
    TransformCameraPoint(campos + 3, &vd, flag);

    fwRot = FUN_80039ddc(&vc, &vd, (VECTOR *)&target, 0);

    d1 = -va.vx;
    d1 += vb.vx;
    d1 *= fwRot;
    if (d1 < 0)
        d1 += 0xFFF;
    d2 = (-va.vy + vb.vy) * fwRot;
    target.vrx = (d1 >> 12) + va.vx;
    if (d2 < 0)
        d2 += 0xFFF;
    d3 = (-va.vz + vb.vz) * fwRot;
    target.vry = (d2 >> 12) + va.vy;
    if (d3 < 0)
        d3 += 0xFFF;
    target.vrz = (d3 >> 12) + va.vz;

    AntiWall(&ViewInfo, &target);
    tp = &target;

    if (cs->snap_pending == 1)
    {
        vDif->vpx = tp->vpx - ViewInfo.vpx;
        vDif->vpy = tp->vpy - ViewInfo.vpy;
        vDif->vpz = tp->vpz - ViewInfo.vpz;
        vDif->vrx = tp->vrx - ViewInfo.vrx;
        vDif->vry = tp->vry - ViewInfo.vry;
        vDif->vrz = tp->vrz - ViewInfo.vrz;
        cs->snap_pending = 0;
    }
    else
    {
        MakeDifSub((VECTOR *)&ViewInfo.vrx, (VECTOR *)&tp->vrx,
                   (VECTOR *)&vDif->vrx, &ref);
        MakeDifSub((VECTOR *)&ViewInfo, (VECTOR *)tp, (VECTOR *)vDif, &pnt);
    }

    return fwRot;
}

// Ghidra decompilation (reference):
//
//
// void MakeCameraPosition(VECTOR *orgpos,SVECTOR *orgrot,SVECTOR *campos,SVECTOR *ref)
//
// {
//   short sVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   undefined1 local_88 [16];
//   int local_78;
//   int local_74;
//   VECTOR local_68;
//   VECTOR local_58;
//   VECTOR VStack_48;
//   VECTOR VStack_38;
//   long alStack_28 [2];
//
//   sVar1 = FUN_8002fd9c(CamState.Owner);
//   Scratchpad._64_2_ = orgrot->vx + sVar1;
//   Scratchpad._66_2_ = orgrot->vy;
//   Scratchpad._68_2_ = orgrot->vz;
//   RotMatrixYXZ((SVECTOR *)(Scratchpad + 0x40),(MATRIX *)(Scratchpad + 0x80));
//   Scratchpad._148_4_ = orgpos->vx;
//   Scratchpad._152_4_ = orgpos->vy;
//   Scratchpad._156_4_ = orgpos->vz;
//   SetRotMatrix((MATRIX *)(Scratchpad + 0x80));
//   SetTransMatrix((MATRIX *)(Scratchpad + 0x80));
//   RotTrans(campos,&local_68,alStack_28);
//   RotTrans(campos + 1,&local_58,alStack_28);
//   RotTrans(campos + 2,&VStack_48,alStack_28);
//   RotTrans(campos + 3,&VStack_38,alStack_28);
//   iVar2 = FUN_80039ddc(&VStack_48,&VStack_38,local_88,0);
//   iVar3 = (local_58.vx - local_68.vx) * iVar2;
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 0xfff;
//   }
//   iVar4 = (local_58.vy - local_68.vy) * iVar2;
//   local_88._12_4_ = (iVar3 >> 0xc) + local_68.vx;
//   if (iVar4 < 0) {
//     iVar4 = iVar4 + 0xfff;
//   }
//   iVar2 = (local_58.vz - local_68.vz) * iVar2;
//   local_78 = (iVar4 >> 0xc) + local_68.vy;
//   if (iVar2 < 0) {
//     iVar2 = iVar2 + 0xfff;
//   }
//   local_74 = (iVar2 >> 0xc) + local_68.vz;
//   AntiWall(&ViewInfo,(GsRVIEW2 *)local_88);
//   if (CamState.OldMode._1_1_ == 1) {
//     *(long *)ref = local_88._0_4_ - ViewInfo.vpx;
//     *(long *)&ref->vz = local_88._4_4_ - ViewInfo.vpy;
//     *(long *)(ref + 1) = local_88._8_4_ - ViewInfo.vpz;
//     *(long *)&ref[1].vz = local_88._12_4_ - ViewInfo.vrx;
//     *(int *)(ref + 2) = local_78 - ViewInfo.vry;
//     *(int *)&ref[2].vz = local_74 - ViewInfo.vrz;
//     CamState.OldMode._1_1_ = CMODE_NORMAL >> 8;
//   }
//   else {
//     MakeDifSub((VECTOR *)&ViewInfo.vrx,(VECTOR *)(local_88 + 0xc),(VECTOR *)&ref[1].vz,
//                (TMakeDifInfo *)&CamState.Valiation);
//     MakeDifSub((VECTOR *)&ViewInfo,(VECTOR *)local_88,(VECTOR *)ref,&pnt);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AntiWall(? *, s32 *, s32, s32);                   /* extern */
// s32 FUN_8002fd9c(s32);                              /* extern */
// s32 FUN_80039ddc(s32 *, s32 *, s32 *, ?);           /* extern */
// ? MakeDifSub(? *, s32 *, void *, ? *);              /* extern */
// ? RotMatrixYXZ(?, ?);                               /* extern */
// ? RotTrans(s32, s32 *, ? *);                        /* extern */
// ? SetRotMatrix(?);                                  /* extern */
// ? SetTransMatrix(?);                                /* extern */
// extern ? CamState;
// extern ? ref;
// extern ? ViewInfo;
// extern ? pnt;
//
// s32 MakeCameraPosition(void *arg0, void *arg1, s32 arg2, void *arg3) {
//     s32 sp10;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp24;
//     s32 sp30;
//     s32 sp40;
//     s32 sp50;
//     s32 sp60;
//     ? sp70;
//     s32 temp_v0;
//     s32 var_a0;
//     s32 var_v1;
//     s32 var_v1_2;
//
//     *(s16 *)0x1F800040 = arg1->unk0 + FUN_8002fd9c(CamState.unk10);
//     *(u16 *)0x1F800042 = arg1->unk2;
//     *(u16 *)0x1F800044 = arg1->unk4;
//     RotMatrixYXZ(0x1F800040, 0x1F800080);
//     *(s32 *)0x1F800094 = arg0->unk0;
//     *(s32 *)0x1F800098 = arg0->unk4;
//     *(s32 *)0x1F80009C = arg0->unk8;
//     SetRotMatrix(0x1F800080);
//     SetTransMatrix(0x1F800080);
//     RotTrans(arg2, &sp30, &sp70);
//     RotTrans(arg2 + 8, &sp40, &sp70);
//     RotTrans(arg2 + 0x10, &sp50, &sp70);
//     RotTrans(arg2 + 0x18, &sp60, &sp70);
//     temp_v0 = FUN_80039ddc(&sp50, &sp60, &sp10, 0);
//     var_v1 = (sp40 - sp30) * temp_v0;
//     if (var_v1 < 0) {
//         var_v1 += 0xFFF;
//     }
//     var_a0 = (sp44 - sp34) * temp_v0;
//     sp1C = (var_v1 >> 0xC) + sp30;
//     if (var_a0 < 0) {
//         var_a0 += 0xFFF;
//     }
//     var_v1_2 = (sp48 - sp38) * temp_v0;
//     sp20 = (var_a0 >> 0xC) + sp34;
//     if (var_v1_2 < 0) {
//         var_v1_2 += 0xFFF;
//     }
//     sp24 = (var_v1_2 >> 0xC) + sp38;
//     AntiWall(&ViewInfo, &sp10, sp34, sp38);
//     if (CamState.unk1D == 1) {
//         arg3->unk0 = (s32) (sp10 - ViewInfo.unk0);
//         arg3->unk4 = (s32) (sp10.unk4 - ViewInfo.unk4);
//         arg3->unk8 = (s32) (sp10.unk8 - ViewInfo.unk8);
//         arg3->unkC = (s32) (sp10.unkC - ViewInfo.unkC);
//         arg3->unk10 = (s32) (sp10.unk10 - ViewInfo.unk10);
//         arg3->unk14 = (s32) (sp10.unk14 - ViewInfo.unk14);
//         CamState.unk1D = 0U;
//     } else {
//         MakeDifSub(&ViewInfo + 0xC, &sp1C, arg3 + 0xC, &ref);
//         MakeDifSub(&ViewInfo, &sp10, arg3, &pnt);
//     }
//     return temp_v0;
// }
