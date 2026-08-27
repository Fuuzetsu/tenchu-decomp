#include "common.h"
#include "main.exe.h"

/* return_to_menu_proc_ (0x8004c024) — unconditional thunk for return_to_menu_(); no
 * direct jal callers (indirect-call only). findsimilar ranks this ~0.57
 * against DisposeModel/DisposeOrnament (same prologue+jal+epilogue shape),
 * but the real asm has NO beqz null-guard here — trust the asm over the
 * similarity score. */

extern void return_to_menu_(void);

void return_to_menu_proc_(void)
{
    return_to_menu_();
}
