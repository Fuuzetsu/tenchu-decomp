#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

extern RECT BriefingVramRect[];

extern void *valloc(u32 size);
extern void vfree(void *p);
extern void ResetInventory(void);
extern void BriefingAndInventorySelectionScreen(void);

void DoBriefingAndInventorySelection(void)
{
    RECT r;
    u_long *p;

    r = BriefingVramRect[0];
    p = (u_long *)valloc(0x20000);
    StoreImage2(&r, p);
    ResetInventory();
    BriefingAndInventorySelectionScreen();
    LoadImage2(&r, p);
    vfree(p);
}
