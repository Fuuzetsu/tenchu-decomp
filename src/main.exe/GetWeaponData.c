#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetWeaponData(struct Humanoid *human, short body, short wid, short wpid, int wep);
 *     APPEAR.C:270, 25 src lines, frame 144 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct Humanoid * human
 *     param $s3       short body
 *     param $t1       short wid
 *     param $a3       short wpid
 *     param stack+16  int wep
 *     reg   $s2       short wep
 *     reg   $a0       short i
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct WeaponType WeaponDB[28];
 *     extern struct WeaponModelType WeaponModel[41];
 * END PSX.SYM */

extern char fmt_tmd[];           /* %s%s.TMD */
extern char path_human_weapon[]; /* K:\\WORK\\CDIMAGE\\HUMAN\\WEAPON\\ */

extern int sprintf(char *buf, char *fmt, ...);
extern OrnamentType *LoadOrnament(u_long *adr);

static inline void FindWeaponId(Humanoid *human, weapon_kind wid, s16 wpid)
{
    s16 i;

    i = 0;
    while (WeaponDB[i].ilup1.pad != WEAPON_KIND_END)
    {
        if (WeaponDB[i].ilup1.pad == wid)
        {
            human->wepid[wpid] = i;
            break;
        }
        i++;
    }
}

void GetWeaponData(Humanoid *human, model_part_index body, weapon_kind wid,
                   s16 wpid,
                   int wep)
{
    s16 w;
    s16 i;
    u8 name[100];
    OrnamentType *base;

    w = (s16)wep;
    if (wpid >= 0)
    {
        FindWeaponId(human, wid, wpid);
    }

    if (w >= 0)
    {
        i = 0;
        while (WeaponModel[i].wid != WEAPON_KIND_END)
        {
            if (WeaponModel[i].wid == wid)
            {
                break;
            }
            i++;
        }
        if (WeaponModel[i].wid != WEAPON_KIND_END)
        {
            if (WeaponModel[i].model == 0)
            {
                sprintf(name, fmt_tmd, path_human_weapon, WeaponModel[i].name);
                WeaponModel[i].model = FileRead(name);
            }
            base = LoadOrnament(WeaponModel[i].model);
            human->weapon[w] = base;
            GsInitCoordinate2(&human->model->object[body]->locate,
                              &base->locate);
        }
    }
}
