#include "common.h"
#include "text_entry.h"

/* Enter an adjacent range at the end nearest the direction of travel. */
void SelectTrialKanjiRange(TrialKanjiRange *range, short first_column)
{
    TrialKanjiPage.range = range;
    if (first_column)
    {
        TrialKanjiPage.first_visible_code = TrialKanjiPage.code_base = range->first_code;
        TrialKanjiPage.column_index = 0;
    }
    else
    {
        TrialKanjiPage.code_base = range->first_code;
        TrialKanjiPage.column_index = range->column_count - 1;
        TrialKanjiPage.first_visible_code = range->column_starts[TrialKanjiPage.column_index];
    }
}
