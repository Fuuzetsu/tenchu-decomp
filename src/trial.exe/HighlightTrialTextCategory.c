#include "common.h"
#include "text_entry.h"

void HighlightTrialTextCategory(short old_index, short new_index)
{
    GsSPRITE *sprite;

    sprite = &TrialTextCategorySprites[old_index]->sprite;
    sprite->r = sprite->g = sprite->b = 128;
    sprite = &TrialTextCategorySprites[new_index]->sprite;
    sprite->r = 0;
    sprite->g = 128;
    sprite->b = 128;
}
