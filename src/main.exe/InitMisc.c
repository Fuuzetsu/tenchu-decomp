#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitMisc(void);
 *     MISC.C:162, 83 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s1       int i
 *     reg   $a0       int iDoor1
 *     reg   $s1       int iDoor2
 *     reg   $v0       struct ModelType * data
 *     reg   $a0       int id1
 *     reg   $s1       int id2
 *     reg   $v0       struct ModelType * data
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 *     extern struct tag_TItem items[30];
 *     extern struct MISC__183fake DoorData[11];
 *     extern struct MISC__185fake SpriteData[2];
 *     extern struct MISC__184fake PitfallData[2];
 * END PSX.SYM */

/*
 * InitMisc (0x8004c1e8, 0x168 bytes) — one-time setup of the misc-object
 * pool: clears all 200 misc[] slots (bottom-test walking-pointer loop,
 * `misc + 199` down to `misc`), then loads the door/pitfall pair-of-models
 * tables and the two ambient sprites (rain/snow "steam"), and finally sets
 * the init latch DoMiscProc waits on.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `misc + 199` (a nonzero-offset pointer into a big absolute extern
 *    array) forces materialization of misc's OWN base address (lui+addiu)
 *    plus a THIRD addiu for the +199*sizeof(tag_TMisc) offset — the
 *    "offset-0 folds, a nonzero offset materializes" rule; ordinary pointer
 *    arithmetic reproduces it with no special spelling.
 *  - DoorData/PitfallData's `Model[2]` fields double as int archive-index
 *    slots before this function ever runs (the original static initializer
 *    packs a GetArcData index — or -1 for "none" — into the same word later
 *    overwritten with the loaded ModelType pointer); PSX.SYM's own locals
 *    (`int iDoor1`/`iDoor2`) confirm the field is READ as a plain int here,
 *    cast off the ModelType* field (`(s32)door->Model[0]`), while other
 *    already-matched files access the same field as a ModelType* — no
 *    conflict, this is a different TU's own read of the field's raw bits.
 *  - Both `Model[0]`/`Model[1]` are read UNCONDITIONALLY before either `if`
 *    (`iDoor2` cached because the first `if`'s GetArcData/LoadModel calls
 *    would clobber a caller-saved copy of it; `iDoor1` is consumed
 *    immediately by its own call and needs no separate temp beyond the
 *    parameter register) — same "pointer/value cached only when it must
 *    survive a call" shape as ProcMiscDoor's twins.
 *  - SpriteData's own `.spr` field is likewise read (cast to int) as the
 *    GetImage index BEFORE being overwritten with the real Sprite3D*.
 *  - The final `EFFECT_SPAWNERS_INITIALISED = 1;` is this TU's own
 *    gp-relative small (Build.hs maspsxGpExterns); DoMiscProc reads it.
 */

extern u_long *GetArcData(s32 index);
extern ModelType *LoadModel(u_long *adr);
extern GsIMAGE *GetImage(s32 index);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

void InitMisc(void)
{
    tag_TMisc *tm;
    s32 i;

    i = 199;
    tm = misc;
    tm = tm + 199;
    do
    {
        tm->proc = 0;
        i--;
        tm--;
    } while (-1 < i);

    {
        s32 iDoor1;
        s32 iDoor2;
        ModelType *data;

        for (i = 0; i < 0xB; i++)
        {
            iDoor1 = (s32)DoorData[i].Model[0];
            iDoor2 = (s32)DoorData[i].Model[1];
            if (iDoor1 != -1)
            {
                data = LoadModel(GetArcData(iDoor1));
                DoorData[i].Model[0] = data;
            }
            if (iDoor2 != -1)
            {
                data = LoadModel(GetArcData(iDoor2));
                DoorData[i].Model[1] = data;
            }
        }
    }

    {
        SpriteDataType *spr;
        GsIMAGE *image;
        Sprite3D *sprite;
        u32 attr;

        i = 0;
        attr = 0x50000000;
        spr = SpriteData;
        do
        {
            i++;
            image = GetImage((s32)spr->spr);
            sprite = SetupSprite((Sprite3D *)0, image);
            spr->spr = sprite;
            sprite->sprite.attribute = attr;
            spr->spr->scale = spr->scale;
            spr++;
        } while (i < 2);
    }

    {
        s32 id1;
        s32 id2;
        ModelType *data;

        for (i = 0; i < 3; i++)
        {
            id1 = (s32)PitfallData[i].Model[0];
            id2 = (s32)PitfallData[i].Model[1];
            if (id1 != -1)
            {
                data = LoadModel(GetArcData(id1));
                PitfallData[i].Model[0] = data;
            }
            if (id2 != -1)
            {
                data = LoadModel(GetArcData(id2));
                PitfallData[i].Model[1] = data;
            }
        }
    }

    EFFECT_SPAWNERS_INITIALISED = 1;
}

// triage: MEDIUM — 90 insns, 4 loop, 4 callees, ~0.07 to ResetAllMisc
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void InitMisc(void)
//
// {
//   tag_TMisc *ptVar1;
//   ulong *puVar2;
//   ModelType *pMVar3;
//   GsIMAGE *image;
//   Sprite3D *pSVar4;
//   _183fake_25 *p_Var5;
//   _185fake_27 *p_Var6;
//   _184fake_26 *p_Var7;
//   int iVar8;
//   ModelType *pMVar9;
//
//   iVar8 = 199;
//   ptVar1 = misc + 199;
//   do {
//     ptVar1->proc = (undefined **)0x0;
//     iVar8 = iVar8 + -1;
//     ptVar1 = ptVar1 + -1;
//   } while (-1 < iVar8);
//   iVar8 = 0;
//   p_Var5 = DoorData;
//   do {
//     pMVar9 = p_Var5->Model[1];
//     if (p_Var5->Model[0] != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)p_Var5->Model[0]);
//       pMVar3 = LoadModel(puVar2);
//       p_Var5->Model[0] = pMVar3;
//     }
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)pMVar9);
//       pMVar9 = LoadModel(puVar2);
//       p_Var5->Model[1] = pMVar9;
//     }
//     iVar8 = iVar8 + 1;
//     p_Var5 = p_Var5 + 1;
//   } while (iVar8 < 0xb);
//   iVar8 = 0;
//   p_Var6 = SpriteData;
//   do {
//     iVar8 = iVar8 + 1;
//     image = GetImage((int)p_Var6->spr);
//     pSVar4 = SetupSprite((Sprite3D *)0x0,image);
//     p_Var6->spr = pSVar4;
//     (pSVar4->sprite).attribute = 0x50000000;
//     p_Var6->spr->scale = p_Var6->scale;
//     p_Var6 = p_Var6 + 1;
//   } while (iVar8 < 2);
//   iVar8 = 0;
//   p_Var7 = PitfallData;
//   do {
//     pMVar9 = p_Var7->Model[1];
//     if (p_Var7->Model[0] != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)p_Var7->Model[0]);
//       pMVar3 = LoadModel(puVar2);
//       p_Var7->Model[0] = pMVar3;
//     }
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)pMVar9);
//       pMVar9 = LoadModel(puVar2);
//       p_Var7->Model[1] = pMVar9;
//     }
//     iVar8 = iVar8 + 1;
//     p_Var7 = p_Var7 + 1;
//   } while (iVar8 < 3);
//   DAT_80097c44 = 1;
//   return;
// }
