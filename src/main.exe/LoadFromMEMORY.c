#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromMEMORY(unsigned char *filename);
 *     FILEIO.C:247, 45 src lines, frame 64 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       unsigned char * filename
 *     reg   $s1       unsigned long * vmp
 *     reg   $s2       unsigned long * data
 *     stack sp+16     unsigned char [24] name
 *     reg   $s0       short i
 *     reg   $a2       short j
 *     reg   $s4       unsigned char * filename
 *     reg   $s2       int fd
 *     reg   $s3       int size
 *     reg   $s1       unsigned long * data
 * END PSX.SYM */

extern void AdtMessageBox(char *fmt, ...);
extern char msg_memory_load_is_disabled[]; /* *** memory load is disabled now *** */

u_long *LoadFromMEMORY(u8 *filename)
{
    AdtMessageBox(msg_memory_load_is_disabled);
    return 0;
}
