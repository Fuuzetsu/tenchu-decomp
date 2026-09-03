#include "common.h"
#include "main.exe.h"

/* Declared here because the SDK header spells these as macros. */
void GsSetFogParam(GsFOGPARAM *fog)
{
    SetDQA(fog->dqa);
    SetDQB(fog->dqb);
    SetFarColor(fog->rfc, fog->gfc, fog->bfc);
}
