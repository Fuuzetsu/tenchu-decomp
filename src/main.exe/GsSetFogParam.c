#include "common.h"
#include "main.exe.h"

/* Private GTE register setters used by this libgs object. */
extern void SetDQA(long dqa);
extern void SetDQB(long dqb);

void GsSetFogParam(GsFOGPARAM *fog)
{
    SetDQA(fog->dqa);
    SetDQB(fog->dqb);
    SetFarColor(fog->rfc, fog->gfc, fog->bfc);
}
