#include "common.h"
#include "main.exe.h"

/*
 * init_score_stats (0x8004e964) — placeholder name (not from Ghidra/Ghidra
 * export; this function isn't in the export at all, no decomp.me reference
 * either — described purely from the raw .s + m2c). Populates an end-of-
 * stage score/stats record from six per-run u8 counters, each a plain gp-
 * relative global (`lbu` — genuinely byte-sized; a wider source type would
 * need a halfword load even under a narrowing `(u8)` cast, per the
 * matching-cookbook narrowing-store rule), then a GameClock tick count
 * clamped to 0x1A5C2 (the only absolute/non-gp global here — GameClock is
 * defined in the think/DoInfoViewProc TU, see item.h's gp note). Returns
 * the same pointer it was given (arg0 survives in $v0 via the jr delay
 * slot, no separate load).
 * gp: StageBosses/StageEnemies/Findenemies/Murders/Criticals/FriendHits
 * added via `tools/gpsyms.py init_score_stats --write`.
 */
typedef struct
{
    u8 bosses;
    u8 enemies;
    u8 hidden_finds;
    u8 murders;
    u8 criticals;
    u8 friendly_hits;
    u8 pad[2];
    s32 clock;
} ScoreStats;

extern u8 StageBosses;
extern u8 StageEnemies;
extern u8 Findenemies;
extern u8 Murders;
extern u8 Criticals;
extern u8 FriendHits;
extern s32 GameClock;

ScoreStats *init_score_stats(ScoreStats *stats)
{
    s32 clock;

    stats->bosses = StageBosses;
    stats->enemies = StageEnemies;
    stats->hidden_finds = Findenemies;
    stats->murders = Murders;
    stats->criticals = Criticals;
    stats->friendly_hits = FriendHits;
    clock = GameClock;
    if (clock > 0x1A5C2)
    {
        clock = 0x1A5C2;
    }
    stats->clock = clock;
    return stats;
}

// triage: TRIVIAL — 29 insns, 0 callees
// likely-relevant cookbook sections:
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// extern u8 Criticals;
// extern u8 Findenemies;
// extern u8 FriendHits;
// extern s32 GameClock;
// extern u8 Murders;
// extern u8 StageBosses;
// extern u8 StageEnemies;
//
// void *init_score_stats(void *arg0) {
//     s32 var_v1;
//
//     arg0->unk0 = (u8) StageBosses;
//     arg0->unk1 = (u8) StageEnemies;
//     arg0->unk2 = (u8) Findenemies;
//     arg0->unk3 = (u8) Murders;
//     arg0->unk4 = (u8) Criticals;
//     arg0->unk5 = (u8) FriendHits;
//     var_v1 = GameClock;
//     if (var_v1 > 0x1A5C2) {
//         var_v1 = 0x1A5C2;
//     }
//     arg0->unk8 = var_v1;
//     return arg0;
// }
