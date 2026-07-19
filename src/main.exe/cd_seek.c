#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/*
 * cd_seek (0x8005f634) — classic fseek(handle, offset, whence) over the
 * proven `FILE *` (shared with cd_close/cd_getsize/cd_tell/cd_init/cd_open/
 * cd_read): whence 0=SEEK_SET (absolute), 1=SEEK_CUR (relative to f->pos),
 * 2=SEEK_END (relative to f->finfo.size); clamps the new position into
 * [0, f->finfo.size] before storing it back to f->pos. A NULL handle
 * reports via puts() and returns -1 (see cd_getsize's header for the
 * guard-clause polarity note).
 *
 * Ghidra couldn't resolve the calling convention (all 3 args show as
 * "in_REG" reads) and, worse, invents a read of an "unset register $v1" for
 * an invalid `whence` — that's not a decompiler bug: the raw asm's dispatch
 * really falls through to the merge point leaving the new-position register
 * uninitialized when whence isn't 0/1/2 (genuine UB in the original, no
 * default case).
 *
 * NOT a `switch`: a plain `switch (whence) { case 0/1/2: ... }` always
 * compiles (regardless of source case order — verified) to a balanced
 * compare TREE that range-splits the post-root sub-case pair with an extra
 * `slti` (e.g. root test ==1, then `slti whence,2` to separate {0} from
 * {2}) — one instruction too many and the wrong shape. The target instead
 * tests 1, then 0, then 2 sequentially, each branch TARGETING its body
 * (not falling into it), with bodies laid out 0,2,1 (case 1/SEEK_CUR last,
 * falling straight into the merge code with no trailing jump) — that's
 * explicit `if (cond) goto label;` chains: each `goto` compiles as a bare
 * test-and-branch-to-target with nothing to fall through into, so the
 * label bodies can be placed in ANY order (here, source order) independent
 * of the test sequence, unlike an inline `if/else if` (whose `then` body is
 * always inline right after its own test).
 */

typedef struct FILE FILE;

struct FILE
{
    CdlFILE finfo;   /* 0x0 */
    s32 flagUse;     /* 0x18 */
    s32 pos;         /* 0x1C */
};

extern int puts(char *s);
extern char D_80014A48[]; /* "cd_seek:invalid handle" — lives in this TU's
                            * unsplit data blob (splat auto-symbol), same
                            * pattern as AfsInit's D_80014944. */

int cd_seek(FILE *f, int offset, int whence)
{
    s32 ret;
    u32 size;

    if (f == 0) {
        puts(D_80014A48);
        return -1;
    }
    if (whence == 1)
        goto do_cur;
    if (whence == 0)
        goto do_set;
    if (whence == 2)
        goto do_end;
    goto merge;
do_set:
    ret = offset;
    goto merge;
do_end:
    ret = f->finfo.size + offset;
    goto merge;
do_cur:
    ret = f->pos + offset;
merge:
    size = f->finfo.size;
    if (size < ret) {
        ret = size;
    } else if (ret < 0) {
        ret = 0;
    }
    f->pos = ret;
    return 0;
}
