#include "common.h"
#include "main.exe.h"

void swap_balma_area_map_(void)
{
    NodeIndexType *saved;
    AreaMapType *cur;

    saved = (NodeIndexType *)BalmaAreaMap;
    cur = GlobalAreaMap;
    GlobalAreaMap = (AreaMapType *)saved;
    BalmaAreaMap = cur;
    FieldIndex = saved;
    FieldArea = (AreaNodeType *)saved->index;
}
