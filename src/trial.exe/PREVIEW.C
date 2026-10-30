#include "common.h"
#include "mission_menu.h"
#include <psxsdk/libgpu.h>

const char TrialEmptyRecordName[] = "\201|\201|\201|\201|";
const char TrialEmptyRecordTime[] = "\201|\201|\201F\201|\201|";
const char TrialRecordTimeFormat[] = "%c%c%c%c\201F%c%c%c%c";
const char TrialMaximumRecordTime[] = "\202T\202X\201F\202T\202X";

/* Display the three fastest records with full-width Shift-JIS digits. */
void FormatTrialMissionPreview(TrialMissionHeader *header, TrialMissionPreview *preview)
{
    int i;
    int seconds;

    preview->available = 1;
    preview->author = header->author;
    preview->title = header->title;
    preview->theme = header->theme;
    for (i = 0; i < TRIAL_PREVIEW_RECORDS; i++)
    {
        if (header->records[i].time < 0)
        {
            sprintf(preview->names[i], TrialEmptyRecordName);
            sprintf(preview->times[i], TrialEmptyRecordTime);
        }
        else
        {
            __builtin_memcpy(preview->names[i], header->records[i].name.text, 10);
            seconds = header->records[i].time / 30;
            if (seconds / 60 < 60)
                sprintf(preview->times[i], TrialRecordTimeFormat,
                        0x82, seconds / 600 + 0x4f,
                        0x82, seconds / 60 % 10 + 0x4f,
                        0x82, seconds % 60 / 10 + 0x4f,
                        0x82, seconds % 60 % 10 + 0x4f);
            else
                sprintf(preview->times[i], TrialMaximumRecordTime);
        }
    }
}

/* The five introductory missions are always present. The following eleven
 * presets follow the persistent progress bits, including the special final
 * stage flag. Card-slot selection is unused for this CD list. */
short LoadTrialPresetPreviews(short card_slot, TrialMissionHeader *headers,
                              TrialMissionPreview *previews)
{
    u32 bit = 1;
    char *titles[TRIAL_PRESET_COUNT * 2] = {
        "\230V\222\206\212\326\222J\202\251\202\347\202\314\216\216\227\373\201@\202\273\202\314\210\352", "\212\326\222J\201@\222\274\222\211", /* 老中関谷からの試練　その一 */
        "\230V\222\206\212\326\222J\202\251\202\347\202\314\216\216\227\373\201@\202\273\202\314\223\361", "\212\326\222J\201@\222\274\222\211", /* 老中関谷からの試練　その二 */
        "\230V\222\206\212\326\222J\202\251\202\347\202\314\216\216\227\373\201@\202\273\202\314\216O", "\212\326\222J\201@\222\274\222\211", /* 老中関谷からの試練　その三 */
        "\230V\222\206\212\326\222J\202\251\202\347\202\314\216\216\227\373\201@\202\273\202\314\216l", "\212\326\222J\201@\222\274\222\211", /* 老中関谷からの試練　その四 */
        "\230V\222\206\212\326\222J\202\251\202\347\202\314\216\216\227\373\201@\202\273\202\314\214\334", "\212\326\222J\201@\222\274\222\211", /* 老中関谷からの試練　その五 */
        "\211z\214\343\211\256\202\323\202\275\202\275\202\321", "\223\214\201@\216\207\211_\215\326", /* 越後屋ふたたび */
        "\227c\202\253\215\260\202\314\214\304\202\321\220\272", "\223\214\201@\216\207\211_\215\326", /* 幼き魂の呼び声 */
        "\210\305\202\311\211\350\220\201\202\255\214\365", "\223\214\201@\216\207\211_\215\326", /* 闇に芽吹く光 */
        "\226\276\220_\221o\211\345\227\254\201@\221\316\201@\223\214\224E\227\254", "\223\214\201@\216\207\211_\215\326", /* 明神双牙流　対　東忍流 */
        "\224\222\202\253\220\341\202\311\214\214\202\314\223k\211\324\202\360", "\223\214\201@\216\207\211_\215\326", /* 白き雪に血の徒花を */
        "\215\304\227\325\202\271\202\265\216\327\220_", "\223\214\201@\216\207\211_\215\326", /* 再臨せし邪神 */
        "\215\320\211\320\202\314\224g\216~\217\352", "\223\214\201@\216\207\211_\215\326", /* 災禍の波止場 */
        "\223V\213\347\202\314\220\261\202\335\202\265\227\354\216R", "\223\214\201@\216\207\211_\215\326", /* 天狗の棲みし霊山 */
        "\224E\223\271\202\306\202\315\214\354\202\351\216\226\202\306\214\251\202\302\202\257\202\275\202\350", "\223\214\201@\216\207\211_\215\326", /* 忍道とは護る事と見つけたり */
        "\211e\202\306\211A", "\223\214\201@\216\207\211_\215\326", /* 影と陰 */
        "\214\273\203Y\203\213\216\226\226\263\203L\221\343\225\250", "\203}\203b\203h\201E\203N\203\215\201[\203h", /* 現ズル事無キ代物 */
    };
    short count = 5;
    int i;
    int j;

    for (i = 0; i < 5; i++)
    {
        previews[i].id = i;
        headers[i].theme = 0;
    }
    for (i = 5; i < TRIAL_PRESET_COUNT; i++, bit <<= 1)
    {
        if (TRIAL_STATE->mission_flags & bit)
        {
            ++count;
            previews[count - 1].id = i;
            headers[count - 1].theme = i - 4;
        }
    }
    for (i = 0; i < count; i++)
    {
        sprintf(headers[i].title, "%s", titles[previews[i].id * 2]);
        sprintf(headers[i].author, "%s", titles[previews[i].id * 2 + 1]);
        for (j = 0; j < TRIAL_PREVIEW_RECORDS; j++)
        {
            headers[i].records[j].name =
                TRIAL_PRESET_RECORD_NAME(previews[i].id, j);
            headers[i].records[j].time =
                TRIAL_PERSISTENT_STATE->preset_records[previews[i].id][j].time;
        }
        FormatTrialMissionPreview(&headers[i], &previews[i]);
    }
    return count;
}

/* Emit an unscaled GPU sprite, then its texture-page command. The ordering
 * table prepends packets, so the page command executes before the sprite. */
void SortTrialSpritePrimitive(GsSPRITE *sprite, short transparent, GsOT *ot, int priority)
{
    SPRT *work = (SPRT *)TENCHU_SCRATCHPAD(0);
    PACKET *packet;

    packet = GsGetWorkBase();
    setSprt(work);
    work->r0 = sprite->r;
    work->g0 = sprite->g;
    work->b0 = sprite->b;
    work->x0 = sprite->x;
    work->y0 = sprite->y;
    work->u0 = sprite->u;
    work->v0 = sprite->v;
    setClut(work, sprite->cx, sprite->cy);
    work->w = sprite->w;
    work->h = sprite->h;
    setSemiTrans(work, transparent);
    *(SPRT *)packet = *work;
    addPrim(ot->org + priority, packet);
    packet += sizeof(SPRT);
    setDrawTPage((DR_TPAGE *)packet, 0, 1, sprite->tpage);
    addPrim(ot->org + priority, packet);
    GsSetWorkBase(packet + sizeof(DR_TPAGE));
}
