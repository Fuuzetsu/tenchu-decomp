#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void AddEnemy(void);
 *     INFOVIEW.C:695, 58 src lines, frame 2064 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     stack sp+24     unsigned char [70][20] names
 *     stack sp+1424   struct TAdtSelect [70] ItemName
 *     reg   $s4       short i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s7       long type
 *     reg   $s5       long x
 *     reg   $s2       long y
 *     reg   $s0       long z
 *     reg   $s3       short r
 *     reg   $s2       short think
 *     stack sp+1984   struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern short *StageAppearance[10];
 *     extern struct WeaponModelType WeaponModel[41];
 *     extern int StageID;
 *     extern struct ThinkDBtype ThinkDB[20];
 *     extern struct TCameraStatus CamState;
 *     extern int CurrentEnemyID;
 * END PSX.SYM */

/*
 * The demo executable contains the 1148-byte earlier-build AddEnemy at
 * 0x80043390. Its control flow and PSX.SYM line records expose the ordinary
 * source behind retail's four extra bytes: sentinel scans for the stage and
 * weapon tables, direct indexed globals, and one compact set of locals reused
 * first as menu cursors and later as the spawned enemy's coordinates.
 *
 * Declaration order is code-generating here. Keeping the PSX.SYM order gives
 * retail's x/i/r/y allocation in s5/s4/s3/s2; reversing the whole list rotates
 * those four registers. The final lexical block likewise places named `pos`
 * at sp+0x7c0 and the zeroed temporary at sp+0x7d0. With direct global access,
 * gcc itself hoists StageAppearance/WeaponModel and emits their caller saves
 * around sprintf at sp+0x7e0/sp+0x7e4—no source-level spill model is needed.
 */

typedef struct ThinkDBtype
{
    u8 *name;
    s16 value;
} ThinkDBtype;

extern s16 *StageAppearance[];
extern ThinkDBtype ThinkDB[20];
extern s32 CurrentEnemyID;

extern char D_80013FA8[];
extern char D_80013FB4[];
extern char D_80097D48[];
extern char D_80097D50[];

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern int sprintf(char *buf, char *fmt, ...);
extern void *memset(void *s, int c, u32 n);
extern s32 leSetEnemy(s32 type, s16 think, s32 x, s32 y, s32 z, s32 r);

void AddEnemy(void)
{
    u8 names[70][20];
    TAdtSelect ItemName[70];
    s16 i;
    Humanoid *human;
    s32 type;
    s32 x;
    s32 y;
    s32 z;
    s16 r;
    s16 think;

    x = 0;
    i = 0;
    if (HumanData[0].type != -1)
    {
        while (HumanData[i].type != -1)
        {
            if (x >= 70)
                break;
            r = 0;
            while (StageAppearance[StageID + 1][r] != -1)
            {
                if (StageAppearance[StageID + 1][r] == HumanData[i].type)
                    break;
                r++;
            }
            if (StageAppearance[StageID + 1][r] != -1)
            {
                y = 0;
                while (WeaponModel[y].wid != -1)
                {
                    if (WeaponModel[y].wid == HumanData[i].wepid)
                        break;
                    y++;
                }
                sprintf((char *)names[x], D_80097D48,
                        HumanData[i].name, WeaponModel[y].name);
                ItemName[x].name = (char *)names[x];
                ItemName[x].value = HumanData[i].type;
                x++;
            }
            i++;
        }
    }

    ItemName[x].name = D_80097D50;
    ItemName[x++].value = -1;
    ItemName[x].name = 0;
    type = (s16)AdtSelect(D_80013FA8, ItemName, 0);
    if (type == -1)
        return;

    think = 0;
    r = 0;
    do
    {
        x = 0;
        i = 0;
        if (ThinkDB[0].name != 0)
        {
            while (ThinkDB[i].name != 0)
            {
                if (x >= 70)
                    break;
                if ((s16)r + 0x31 == ThinkDB[i].name[0])
                {
                    ItemName[x].name = (char *)ThinkDB[i].name;
                    ItemName[x].value = ThinkDB[i].value;
                    x++;
                }
                i++;
            }
        }
        ItemName[x].name = 0;
        think = think | AdtSelect(D_80013FB4, ItemName, 0);
    } while (think != 0x1111 && think != 0x2222 && ++r < 4);

    {
        VECTOR pos;
        VECTOR blood;

        x = CamState.Owner->model->locate.coord.t[0];
        y = CamState.Owner->model->locate.coord.t[1];
        z = CamState.Owner->model->locate.coord.t[2];
        r = CamState.Owner->model->rotate.vy;
        CurrentEnemyID = leSetEnemy(type, think, x, y, z, r);
        human = BreedLife(type, x, y, z, 0);
        human->model->rotate.vy = r;
        human->target = (ModelType *)CamState.Owner->model;

        memset(&blood, 0, sizeof(VECTOR));
        blood.vx = human->model->locate.coord.t[0];
        blood.vy = human->model->locate.coord.t[1] - 1200;
        blood.vz = human->model->locate.coord.t[2];
        pos = blood;
        SetBleeds(&pos, 400, 0, 50, 30, 0xffffff);
    }
}
