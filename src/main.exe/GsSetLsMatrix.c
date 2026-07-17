#include "common.h"
#include "main.exe.h"

/*
 * GsSetLsMatrix (0x80065100) — the stock PsyQ libgs entry: load a local-screen
 * matrix into the GTE by handing the same MATRIX to both SetRotMatrix (the
 * rotation part) and SetTransMatrix (the translation part). `mp` is cached in a
 * callee-saved register across both calls — a plain parameter read twice needs
 * no separate temp (cookbook's cached-pointer rule, same shape as
 * LoadTIMpackAndFree).
 */
extern void SetRotMatrix(MATRIX *m);
extern void SetTransMatrix(MATRIX *m);

void GsSetLsMatrix(MATRIX *mp)
{
    SetRotMatrix(mp);
    SetTransMatrix(mp);
}
