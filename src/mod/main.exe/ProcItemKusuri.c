typedef void *pointer; /* Compatibility type for this non-matching mod hook. */
#include "../../../reference/ghidra_types.h"

#define CONCAT22(hi, lo) \
    (((unsigned int)(unsigned short)(hi) << 16) | (unsigned short)(lo))

/* Game functions + globals — resolved from main.exe.elf at link time by mkmod. */
extern void UpdateCoordinate(ModelType *m);
extern VECTOR *GetAbsolutePosition(ModelType *m, int x, int y, int z);
extern void DrawSprite(Sprite3D *s);
extern void ReqItemDrop(PARAM_ITEM_USE *p);
extern void UpdateMotion(MotionManager *m, int id);
extern void MoveHumanoid(Humanoid *h, unsigned short a, unsigned short b);
extern void SetBleed(VECTOR *pos, SVECTOR *vel, int a, int col);
extern void SoundEx(VECTOR *loc, int id);
extern void DeleteConflict(ModelType *m);
extern void dispose_weapon_data_of_char_(Humanoid *h, int a);
extern int ActionHalt;
extern int rand(void);
extern void *memset(void *s, int c, unsigned int n);

void ProcItemKusuri(tag_TItem *item)
{
    byte bVar1;
    short sVar2;
    MotionDataType *pMVar3;
    VECTOR *pVVar4;
    ModelType *pMVar5;
    ModelType *pMVar5b;
    ModelArchiveType *pMVar9;
    Sprite3D *pSVar10;
    MotionManager *pMVar11;
    SVECTOR *pSVar12;
    undefined4 uVar13, uVar14, uVar15;
    Sprite3D *sprt;
    Humanoid *pHVar16;
    TItemType TVar17;
    int iVar6, iVar7, iVar18;
    unsigned char buf[40];         /* the split-locals buffer (local_40..local_20) */
    pointer *ppuVar8;

    sprt = (Sprite3D *)item->model;
    if (item->mode == 0xff) {
        item->mode = '\0';
        return;
    }
    bVar1 = item->mode;
    if (bVar1 == 1) {
        pMVar11 = item->owner->motion;
        if (pMVar11->mid == 0xf01) {
            sVar2 = pMVar11->count;
            if (sVar2 == 0x37) {
                item->mode = '\x02';
                return;
            }
            if (sVar2 < 4) {
                return;
            }
            UpdateCoordinate(item->locate);
            pMVar5 = item->locate;
            pSVar12 = &pMVar5->rotate;
            pSVar10 = sprt;
            do {
                uVar14 = *(undefined4 *)(pMVar5->locate).coord.m[0];
                uVar15 = *(undefined4 *)((pMVar5->locate).coord.m[0] + 2);
                uVar13 = *(undefined4 *)((pMVar5->locate).coord.m[1] + 1);
                (pSVar10->locate).flg = (pMVar5->locate).flg;
                *(undefined4 *)(pSVar10->locate).coord.m[0] = uVar14;
                *(undefined4 *)((pSVar10->locate).coord.m[0] + 2) = uVar15;
                *(undefined4 *)((pSVar10->locate).coord.m[1] + 1) = uVar13;
                pMVar5 = (ModelType *)((pMVar5->locate).coord.m + 2);
                pSVar10 = (Sprite3D *)((pSVar10->locate).coord.m + 2);
            } while (pMVar5 != (ModelType *)pSVar12);
            sprt->scale = 0x2000;
            DrawSprite(sprt);
            return;
        }
        pVVar4 = GetAbsolutePosition(item->locate, 0, 0, 0);
        pHVar16 = item->owner;
        TVar17 = item->type;
        memset(buf, '\0', 0x28);
        *(int *)(buf + 8) = pVVar4->vx;
        *(int *)(buf + 12) = pVVar4->vy;
        *(int *)(buf + 16) = pVVar4->vz;
        *(int *)(buf + 0) = (int)TVar17;
        *(void **)(buf + 4) = pHVar16;
        *(int *)(buf + 24) = rand() % 200 + -100;
        *(int *)(buf + 28) = rand() % 100 + -200;
        *(int *)(buf + 32) = rand() % 200 + -100;
        ReqItemDrop((PARAM_ITEM_USE *)buf);
        ppuVar8 = item->proc;
        if (ppuVar8 == (pointer *)0x0) {
            return;
        }
        item->mode = 0xff;
    } else {
        if (bVar1 < 2) {
            if (bVar1 != 0) {
                return;
            }
            pHVar16 = item->owner;
            if ((ActionHalt == 0) && (0 < pHVar16->life)) {
                dispose_weapon_data_of_char_(pHVar16, 3);
                UpdateMotion(pHVar16->motion, 0xf01);
                pHVar16->status = 0xf;
                pMVar3 = pHVar16->motion->motion;
                MoveHumanoid(pHVar16, (unsigned short)pMVar3->orderspd,
                             (unsigned short)pMVar3->sidespd);
            }
            pMVar9 = item->owner->model;
            if (pMVar9->n < 0xf) {
                (item->locate->locate).super = &pMVar9->object[2]->locate;
            } else {
                (item->locate->locate).super = &pMVar9->object[0xe]->locate;
            }
            (item->locate->locate).coord.t[0] = 0;
            (item->locate->locate).coord.t[1] = 0x32;
            (item->locate->locate).coord.t[2] = 0;
            item->mode = item->mode + '\x01';
            return;
        }
        iVar18 = 0;
        if (bVar1 != 2) {
            return;
        }
        item->owner->life = 50;    /* MOD: heal to 50 instead of full (lifemax) */
        for (; iVar18 < 0x14; iVar18 = iVar18 + 1) {
            memset(buf + 0x10, '\0', 0x10);
            iVar6 = rand();
            *(int *)(buf + 16) = (item->owner->model->locate).coord.t[0] + -500 + iVar6 % 1000;
            iVar6 = rand();
            *(int *)(buf + 20) = (item->owner->model->locate).coord.t[1] + -0x4b0 + iVar6 % 1000;
            iVar6 = rand();
            *(int *)(buf + 8) = (item->owner->model->locate).coord.t[2] + -500 + iVar6 % 1000;
            *(int *)(buf + 0) = *(int *)(buf + 16);
            *(int *)(buf + 4) = *(int *)(buf + 20);
            *(int *)(buf + 12) = *(int *)(buf + 28);
            *(int *)(buf + 24) = *(int *)(buf + 8);
            memset(buf + 24, '\0', 8);
            iVar6 = rand();
            *(int *)(buf + 24) =
                CONCAT22((short)iVar6 + (short)(iVar6 / 10) * -10 + -0x1e,
                         (unsigned short)*(int *)(buf + 24));
            *(int *)(buf + 16) = *(int *)(buf + 24);
            *(int *)(buf + 20) = *(int *)(buf + 28);
            iVar7 = rand();
            iVar6 = iVar7;
            if (iVar7 < 0) {
                iVar6 = iVar7 + 0xf;
            }
            SetBleed((VECTOR *)buf, (SVECTOR *)(buf + 0x10),
                     iVar7 + (iVar6 >> 4) * -0x10 + 0xf, 0xffff7e);
        }
        SoundEx(item->owner->locate, 0x24);
        ppuVar8 = item->proc;
        if (ppuVar8 == (pointer *)0x0) {
            return;
        }
        item->mode = 0xff;
    }
    ((void (*)(tag_TItem *))ppuVar8)(item);
    DeleteConflict(item->locate);
    /* MOD: dropped the original's AdtMessageBox("item dispose fail ...") debug log
     * here — it only fires on a disposal failure (never in normal play), and cutting
     * it keeps this function within its original 1432-byte slot so it can be patched
     * IN PLACE (no trampoline / mod region), leaving the disc byte-faithful. */
    item->owner = (Humanoid *)0x0;
    item->proc = (pointer *)0x0;
    return;
}
