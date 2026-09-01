#include "common.h"
#include "main.exe.h"
#include "sound.h"

/*
 * search_id_table_ (0x8004f770, 0x44 bytes) — generic table search: walks a
 * caller-supplied array of 6-byte records (only the first byte, an ID, is
 * read here) looking for one whose ID matches `id`, returning a pointer to
 * the matching record or NULL; the table is terminated by an ID byte of
 * SOUND_TABLE_END (0xFF). Sound TU (address-contiguous with
 * apply_cd_volume_/set_cda_volume_/PlayMusicFormID/SetupSoundEffect/Sound),
 * no direct `jal` callers found —
 * reached indirectly or from an unsplit caller.
 *
 * A `while (cond) {...}` whose initial value isn't a provable constant keeps
 * BOTH the entry test and the rotated bottom test (cookbook Loops); here the
 * two tests are physically the same comparison
 * (`*table != SOUND_TABLE_END`) re-emitted at the entry and at the loop
 * bottom, exactly the classic duplicate_loop_exit_test shape, not two
 * different conditions.
 */

u8 *search_id_table_(u8 *table, s32 id)
{
    while (*table != SOUND_TABLE_END)
    {
        if (id == *table)
            return table;
        table += 6;
    }
    return 0;
}
