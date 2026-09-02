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
    if (HumanData[0].type != CHARACTER_KIND_END)
    {
        while (HumanData[i].type != CHARACTER_KIND_END)
        {
            if (x >= 70)
                break;
            r = 0;
            while (StageAppearance[STAGE_NUMBER(StageID)][r] != CHARACTER_KIND_END)
            {
                if (StageAppearance[STAGE_NUMBER(StageID)][r] == HumanData[i].type)
                    break;
                r++;
            }
            if (StageAppearance[STAGE_NUMBER(StageID)][r] != CHARACTER_KIND_END)
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
    /* Re-narrow the wider return value before storing it. */
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
        human->target = &CamState.Owner->model->locate;

        memset(&spot, 0, sizeof(VECTOR));
        spot.vx = human->model->locate.coord.t[0];
        spot.vy = human->model->locate.coord.t[1] - 1200;
        spot.vz = human->model->locate.coord.t[2];
        pos = spot;
        SetBleeds(&pos, 400, 0, 50, 30, COLOR_WHITE);
    }
}
