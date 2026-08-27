#include "common.h"
#include "main.exe.h"

/*
 * cd_read_at_ (0x8005f7d0) — thin forwarding wrapper over the raw
 * sector/byte-offset CD reader cd_read_sectors_(buffer, sector, byteOffset,
 * byteLength): always reads from byte offset 0, and its own third argument
 * is a SECTOR COUNT converted to a byte length by `<< 0xb` (`* 0x800` — the
 * PS1 CD sector size), i.e. "read `count` whole sectors starting at
 * `sector` into `buffer`". cd_read_sectors_ lives below the 0x80060000 PsyQ/CRT
 * boundary (still a game-TU function, just one that calls the BIOS Cd*
 * primitives directly) so it stays in scope; it is otherwise unmatched.
 */
extern void cd_read_sectors_(void *buffer, int sector, int byteOffset, int byteLength);

void cd_read_at_(void *buffer, int sector, int count)
{
    cd_read_sectors_(buffer, sector, 0, count << 0xb);
}
