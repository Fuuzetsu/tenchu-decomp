#include "common.h"
#include "main.exe.h"

extern AreaMapType *LoadAreaMap(AreaMapType *adr);

AreaMapType *load_balma_area_map_(AreaMapType *adr)
{
    NodeIndexType *cur;
    AreaMapType *newmap;

    cur = (NodeIndexType *)GlobalAreaMap;
    newmap = LoadAreaMap(adr);
    GlobalAreaMap = (AreaMapType *)cur;
    FieldIndex = cur;
    BalmaAreaMap = newmap;
    FieldArea = (AreaNodeType *)cur->index;
    return newmap;
}
