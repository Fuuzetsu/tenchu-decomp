#include "common.h"
#include "text_entry.h"

void DisposeTrialTextEntry(void)
{
    int i;

    if (TrialTextBackground)
        DisposeBG(TrialTextBackground);
    if (TrialTextPanels)
        vfree(TrialTextPanels);
    for (i = 0; i < 5; i++)
    {
        if (TrialTextCategoryButtons[i])
            vfree(TrialTextCategoryButtons[i]);
        if (TrialTextCategorySprites[i])
            vfree(TrialTextCategorySprites[i]);
    }
    for (i = 0; i < 4; i++)
        if (TrialTextShoulderButtons[i])
            vfree(TrialTextShoulderButtons[i]);
    if (TrialTextHeading)
        vfree(TrialTextHeading);
    for (i = 0; i < 2; i++)
        if (TrialTextScrollArrows[i])
            vfree(TrialTextScrollArrows[i]);
    if (TrialKeyboardQuad)
        vfree(TrialKeyboardQuad);
    if (TrialEditedTextQuad)
        vfree(TrialEditedTextQuad);
    if (TrialKeyboardCursor)
    {
        if (TrialKeyboardCursor->sprite)
            vfree(TrialKeyboardCursor->sprite);
        if (TrialKeyboardCursor->characters)
            vfree(TrialKeyboardCursor->characters);
        vfree(TrialKeyboardCursor);
    }
    if (TrialEditedTextCursor)
    {
        if (TrialEditedTextCursor->sprite)
            vfree(TrialEditedTextCursor->sprite);
        if (TrialEditedTextCursor->characters)
            vfree(TrialEditedTextCursor->characters);
        vfree(TrialEditedTextCursor);
    }
    for (i = 0; i < 45; i++)
        if (TrialKanjiRanges[i].column_starts)
            vfree(TrialKanjiRanges[i].column_starts);
    vfree(TrialTextArchive);
}
