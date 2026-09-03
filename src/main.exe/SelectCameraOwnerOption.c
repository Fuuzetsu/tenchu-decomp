#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SelectCameraOwnerOption(void);
 *     INFOVIEW.C:796, 27 src lines, frame 592 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct TAdtSelect [31] targets
 *     stack sp+264    unsigned char [30][10] msg
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

extern char fmt_num_2[]; /* %d */                                /* "%d" */
extern char str_select_camera_owner[]; /* select camera owner */ /* "select camera owner" */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);

void SelectCameraOwnerOption(void)
{
    enum
    {
        MAX_CAMERA_OWNER_CHOICES = 35,
        N_CAMERA_OWNER_MENU_ENTRIES = MAX_CAMERA_OWNER_CHOICES + 1,
        CAMERA_OWNER_LABEL_SIZE = 10
    };
    int i;
    TAdtSelect targets[N_CAMERA_OWNER_MENU_ENTRIES];
    u8 msg[MAX_CAMERA_OWNER_CHOICES][CAMERA_OWNER_LABEL_SIZE];

    if (Humans < MAX_CAMERA_OWNER_CHOICES)
    {
        for (i = 0; i < Humans; i++)
        {
            sprintf((char *)msg[i], fmt_num_2, i);
            targets[i].name = msg[i];
            targets[i].value = (u_long)HumanGroup[i];
        }
        targets[i].name = NULL;
        CamState.Owner = (Humanoid *)AdtSelect(str_select_camera_owner, targets, 0);
        ViewInfo.vrx = CamState.Owner->model->locate.coord.t[0];
        ViewInfo.vry = CamState.Owner->model->locate.coord.t[1];
        ViewInfo.vrz = CamState.Owner->model->locate.coord.t[2];
        ViewInfo.vpx = ViewInfo.vrx;
        ViewInfo.vpy = ViewInfo.vry - 5000;
        ViewInfo.vpz = ViewInfo.vrz;
    }
}
