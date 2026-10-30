#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"

static const char ReachLordText[] = "\223a\202\314\202\340\202\306\202\326\214\374\202\251\202\246"; /* 殿のもとへ向かえ */
static const char AssassinateText[] = "\210\303\216E\202\271\202\346"; /* 暗殺せよ */
static const char EliminateEnemiesText[] = "\223G\202\360\212F\216E\202\265\202\311\202\271\202\346"; /* 敵を皆殺しにせよ */
static const char ProtectPrincessText[] = "\225P\202\360\216\347\202\352"; /* 姫を守れ */
static const char FindHerbText[] = "\213C\226\275\221\220\202\360\222T\202\271"; /* 気命草を探せ */

const TrialObjectiveLabels TrialObjectiveText = {{
    ReachLordText, AssassinateText, EliminateEnemiesText, ProtectPrincessText, FindHerbText
}};

void LoadTrialConfirmationGraphics(void)
{
    char filename[64];
    u_long *image;
    int i;
    Sprite3D *panel;
    GsSPRITE *sprite;

    sprintf(filename, "%sprv%02d.TIM", "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\",
            TrialMission.header.theme + 1);
    image = FileRead(filename);
    TrialConfirmationBackground = load_background_(image);
    vfree(image);
    for (i = 0; i < TRIAL_CONFIRM_SPRITES; i++)
    {
        panel = load_archive_sprite_(TrialImageArchive, i + 14);
        TrialConfirmationSprites[i] = panel;
        panel->sprite.my = 0;
        panel->sprite.mx = 0;
    }
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_HEADING]->sprite; /* heading */
    sprite->x = -33;
    sprite->y = -103;
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_FIELD_BARS]->sprite; /* field bars */
    sprite->x = -94;
    sprite->y = -74;
    sprite->attribute |= GS_ATTR_SEMITRANS_ADD;
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_HIGHLIGHT]->sprite; /* heading highlight */
    sprite->x = -52;
    sprite->y = -102;
    sprite->attribute |= GS_ATTR_SEMITRANS_ADD;
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_FIELD_LABELS]->sprite; /* field labels */
    sprite->x = -152;
    sprite->y = -74;
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_RECORD_HEADING]->sprite; /* best-record heading */
    sprite->x = 14;
    sprite->y = -2;
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_RANKS]->sprite; /* rank numbers */
    sprite->x = -90;
    sprite->y = 15;
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_CONTROLS]->sprite; /* start/back controls */
    sprite->y = 90;
    sprite->mx = sprite->w / 2;
    sprite->my = sprite->h / 2;
    sprite = &TrialConfirmationSprites[TRIAL_CONFIRM_STEALTH_LABEL]->sprite; /* stealth requirement */
    sprite->x = 80;
    sprite->y = -20;
    sprite->attribute |= GS_ATTR_SEMITRANS_ADD;
}
