#include "common.h"
#include "main.exe.h"

/*
 * DisposeAreaMap (0x8001ab28) — free an area-map object; if none was given,
 * fall back to the cached GlobalAreaMap (clearing the cache) instead. Unlike
 * Ghidra's flattened rendering (which loads GlobalAreaMap unconditionally at
 * entry), the asm only loads it inside the `area == 0` path — a nested if,
 * not a hoisted read (trust the asm's branch shape over Ghidra's SSA-style
 * early temp).
 */
extern void vfree(void *p);
extern void *GlobalAreaMap;

void DisposeAreaMap(void *area)
{
    if (area == 0) {
        void *tmp = GlobalAreaMap;
        if (tmp != 0) {
            area = tmp;
            GlobalAreaMap = 0;
        }
    }
    vfree(area);
}
