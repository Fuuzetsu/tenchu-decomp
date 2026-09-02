#include "common.h"
#include "main.exe.h"
#include "item.h"

void set_model_hide_(Humanoid *human, s16 hide)
{
    ModelArchiveType *model;
    s16 last;
    s16 i;

    model = human->model;
    if (model->n > MODEL_PART_BODY_LAST)
    {
        last = MODEL_PART_BODY_LAST;
    }
    else
    {
        last = model->n - 1;
    }
    if (hide != 0)
    {
        HIDE_HUMANOID_BODY_PARTS(model, last, i);
        return;
    }
    SHOW_HUMANOID_BODY_PARTS(model, last, i);
}
