#include "common.h"
#include "tuning.h"
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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
    TThinkType value;
} ThinkDBtype;

extern ThinkDBtype ThinkDB[20];

extern char str_select_type[];          /* select type */
extern char str_custom_think_setting[]; /* custom think setting */
extern char fmt_pair[];                 /* %s %s */
extern u8 str_cancel_2[];               /* cancel */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern int sprintf(char *buf, char *fmt, ...);
extern void *memset(void *s, int c, u32 n);
extern enemy_layout_index leSetEnemy(s32 type, TThinkType think, s32 x,
                                     s32 y, s32 z, s16 r);

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
    TThinkType think;

    x = 0;
    i = 0;
    /* The entry guards before both scans are in the bytes (cc1 does not
     * fold them into the while's own top test; measured). */
    if (HumanData[0].type != CHARACTER_KIND_END)
    {
        while (HumanData[i].type != CHARACTER_KIND_END)
        {
            if (x >= 70)
                break;
            r = 0;
            while (StageAppearance[StageID + 1][r] != CHARACTER_KIND_END)
            {
                if (StageAppearance[StageID + 1][r] == HumanData[i].type)
                    break;
                r++;
            }
            if (StageAppearance[StageID + 1][r] != CHARACTER_KIND_END)
            {
                y = 0;
                while (WeaponModel[y].wid != WEAPON_KIND_END)
                {
                    if (WeaponModel[y].wid == HumanData[i].wepid)
                        break;
                    y++;
                }
                sprintf((char *)names[x], fmt_pair,
                        HumanData[i].name, WeaponModel[y].name);
                ItemName[x].name = names[x];
                ItemName[x].value = HumanData[i].type;
                x++;
            }
            i++;
        }
    }

    /* When the scan fills all 70 rows these two writes land at [70]/[71],
     * past the array: retail's own latent overflow. */
    ItemName[x].name = str_cancel_2;
    ItemName[x++].value = ADT_SELECT_CANCEL;
    ItemName[x].name = 0;
    /* (s16) re-narrows the s32 return: byte-required (writer-width rule;
     * measured). */
    type = (s16)AdtSelect(str_select_type, ItemName, 0);
    if (type == ADT_SELECT_CANCEL)
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
                if (r + '1' == ThinkDB[i].name[0])
                {
                    ItemName[x].name = ThinkDB[i].name;
                    ItemName[x].value = ThinkDB[i].value;
                    x++;
                }
                i++;
            }
        }
        ItemName[x].name = 0;
        think = think | AdtSelect(str_custom_think_setting, ItemName, 0);
    } while (think != THINK_MIX_PLAYER && think != THINK_MIX_PAD2 && ++r < 4);

    {
        VECTOR pos;
        VECTOR spot;

        x = CamState.Owner->model->locate.coord.t[0];
        y = CamState.Owner->model->locate.coord.t[1];
        z = CamState.Owner->model->locate.coord.t[2];
        r = CamState.Owner->model->rotate.vy;
        CurrentEnemyID = leSetEnemy(type, think, x, y, z, r);
        human = BreedLife(type, x, y, z, 0);
        human->model->rotate.vy = r;
        human->target = (ModelType *)CamState.Owner->model;

        memset(&spot, 0, sizeof(VECTOR));
        spot.vx = human->model->locate.coord.t[0];
        spot.vy = human->model->locate.coord.t[1] - 1200;
        spot.vz = human->model->locate.coord.t[2];
        pos = spot;
        SetBleeds(&pos, 400, 0, 50, 30, COLOR_WHITE);
    }
}
