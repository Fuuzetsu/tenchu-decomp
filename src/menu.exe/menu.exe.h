/* MENU.EXE shares the engine types and the persistent handoff with MAIN.EXE. */
#ifndef MENU_EXE_H
#define MENU_EXE_H

#include "../main.exe/main.exe.h"

/* This state survives executable replacement at a fixed protocol address. */
#define MENU_STATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

#endif
