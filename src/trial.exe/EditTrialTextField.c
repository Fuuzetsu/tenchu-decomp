#include "common.h"
#include "mission_menu.h"

/* The settings form seeds TrialTextEntry before opening the keyboard. */
void EditTrialTextField(short mode, char *text, int size)
{
    ShowTrialTextEntry(mode);
    strncpy(text, TrialTextEntry, size);
    memset(TrialTextEntry, 0, sizeof(TrialTextEntry));
}
