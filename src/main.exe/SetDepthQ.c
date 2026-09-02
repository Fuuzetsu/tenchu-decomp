#include "common.h"
#include "main.exe.h"
#include "gte.h"

void SetDepthQ(s32 dqa, s32 dqb)
{
    gte_ldDQA(dqa);
    gte_ldDQB(dqb);
}
