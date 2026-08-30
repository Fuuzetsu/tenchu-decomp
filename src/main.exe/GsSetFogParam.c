#include "common.h"
#include "main.exe.h"

/* Declared here because the SDK header spells these as macros. */
extern void SetDQA(long dqa);
extern void SetDQB(long dqb);

void GsSetFogParam(GsFOGPARAM *fog)
{
    SetDQA(fog->dqa);
    SetDQB(fog->dqb);
    SetFarColor(fog->rfc, fog->gfc, fog->bfc);
}
