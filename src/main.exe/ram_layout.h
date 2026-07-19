#ifndef TENCHU_RAM_LAYOUT_H
#define TENCHU_RAM_LAYOUT_H

/*
 * One source of truth for fixed MAIN.EXE memory contracts and configurable
 * RAM-budget policy.  tools/ram_layout.py reads the simple literal defines
 * below; linker generators must consume that module rather than repeat these
 * numbers. Canonical extraction/config inputs may still contain shipped
 * addresses as retail oracles; normal-link generators must translate those
 * inputs through this policy before they become runtime definitions.
 *
 * Section-owned objects such as StageChar, CamState, and D_80097D70 are NOT
 * layout constants.  Normal code must name their linker symbols so ordinary
 * growth moves them.  In particular, the 0x80090000 high-half scaffolds in
 * three matching sources do not describe a real "game globals" base.
 */

#define TENCHU_MAIN_LOAD_ADDRESS              0x80011000

#define TENCHU_PERSISTENT_STATE_ADDRESS       0x80010000
#define TENCHU_PERSISTENT_STATE_SIZE          0x00000e70
#define TENCHU_PERSISTENT_RNG_SIZE            0x00000004

#define TENCHU_MEMORY_POOL_FLOOR              0x800dc000
#define TENCHU_MEMORY_POOL_END                0x801fc000
#define TENCHU_BSS_ALIGNMENT                  16
#define TENCHU_MEMORY_POOL_ALIGNMENT          16
#define TENCHU_MEMORY_POOL_MINIMUM_SIZE       0x00000010
#define TENCHU_MEMORY_POOL_HEADER_WORDS       2

#define TENCHU_EXECUTABLE_HANDOFF_ADDRESS     0x80100000
#define TENCHU_EXECUTABLE_HANDOFF_SIZE        0x0000005c
#define TENCHU_INITIAL_STACK_ADDRESS          0x801ffff0
#define TENCHU_CACHED_RAM_END                 0x80200000

#define TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS    0x807f0000
/* Buf16 is part of the PC-link protocol; the Python loader rejects resizing. */
#define TENCHU_PC_MEMORY_HANDSHAKE_SIZE       0x00000010

#define TENCHU_SCRATCHPAD_ADDRESS             0x1f800000
#define TENCHU_SCRATCHPAD_SIZE                0x00000400
#define TENCHU_MMIO_ADDRESS                   0x1f801000
#define TENCHU_MMIO_SIZE                      0x00002000

#define TENCHU_SCRATCHPAD(offset) \
    (TENCHU_SCRATCHPAD_ADDRESS + (offset))

/* Derived fixed-contract values; keep arithmetic visible instead of copying
 * another absolute address into game code. */
#define TENCHU_PERSISTENT_RNG_ADDRESS \
    (TENCHU_PERSISTENT_STATE_ADDRESS + TENCHU_PERSISTENT_STATE_SIZE)
#define TENCHU_MEMORY_POOL_HEADER_BYTES \
    (TENCHU_MEMORY_POOL_HEADER_WORDS * 4)
#define TENCHU_PC_MEMORY_POOL_ADDRESS \
    TENCHU_CACHED_RAM_END
#define TENCHU_PC_MEMORY_POOL_SIZE \
    (TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS - TENCHU_PC_MEMORY_POOL_ADDRESS)
#define TENCHU_PC_MEMORY_PAYLOAD_ADDRESS \
    (TENCHU_PC_MEMORY_POOL_ADDRESS + TENCHU_MEMORY_POOL_HEADER_BYTES)
#define TENCHU_CACHED_RAM_END_PLUS_HEADER \
    TENCHU_PC_MEMORY_PAYLOAD_ADDRESS
#define TENCHU_RETAIL_MEMORY_POOL_CAPACITY \
    (((TENCHU_MEMORY_POOL_END - TENCHU_MEMORY_POOL_FLOOR) / 4) - \
     TENCHU_MEMORY_POOL_HEADER_WORDS)

#endif /* TENCHU_RAM_LAYOUT_H */
