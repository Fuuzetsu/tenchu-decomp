#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"

/*
 * update_something_for_each_visible_enemy_ (0x80029368, 0x104 bytes) — per
 * visible enemy: pick a DrawTMDmode from a parallel s16 table, draw its
 * model archive (id = -i, per DrawAfterimage/DrawOrnament's existing
 * per-item-index convention), then draw any active weapon ornaments and
 * afterimage effects it currently has armed. Same "Humanoid control" TU as
 * is_character_state_present_on_stage_.c/GetDirection.c; called from CVArun
 * and main (a per-frame render pass, not think-handler logic).
 *
 * `i` is a genuine s16 (matches m2c): re-widened via sll+sra at the top of
 * EVERY iteration for the index*4/index*2 address math and the negated
 * DrawModelArchive arg — unlike is_character_state_present_on_stage_'s s32
 * counter, a real s16 can't cache a wider extended copy across iterations.
 * VISIBLE_ENEMIES_ is reloaded fresh at the bottom test too (not hoisted
 * into a preheader register like Humans there) because this loop contains
 * calls — the compiler can't prove they don't alias the gp global, so it
 * can't cache it across them. The last if's `i+1` is computed eagerly in
 * its own delay slot (reused verbatim from the `for` increment, which must
 * run whether or not this final DrawAfterimage fires) and then recomputed
 * once more right after the call (the call clobbers the delay-slot copy).
 *
 * Humanoid's `model` (0x58) and `some_afterimage_1_0xa4`/
 * `_2_0xa8` (0xa4/0xa8) fields were corrected here (see game_types.h) —
 * `model` was an unverified `char_state_related_camera_things *` guess
 * (nothing used it), and the afterimage pair were 8 separate u8 bytes; this
 * function proves all three access widths against Ghidra's own independent
 * `Humanoid` struct (reference/ghidra_types.h) AND item.h's cross-TU
 * Humanoid.model, both agreeing at the same offsets.
 */
extern s16 VISIBLE_ENEMIES_;
extern Humanoid *VISIBLE_CHARACTERS_ON_STAGE_[];
extern s16 D_800BE768[];
extern short DrawModelArchive(ModelArchiveType *mad, long gap);
extern short DrawOrnament(OrnamentType *objp);

void update_something_for_each_visible_enemy_(void)
{
    s16 i;
    Humanoid *cs;

    for (i = 0; i < VISIBLE_ENEMIES_; i++)
    {
        cs = VISIBLE_CHARACTERS_ON_STAGE_[i];
        DrawTMDmode = D_800BE768[i];
        DrawModelArchive(cs->model, -i);
        if (cs->weapon[0] != 0)
        {
            DrawOrnament(cs->weapon[0]);
        }
        if (cs->weapon[1] != 0)
        {
            DrawOrnament(cs->weapon[1]);
        }
        if (cs->illusion[0] != 0)
        {
            DrawAfterimage(cs->illusion[0], 1);
        }
        if (cs->illusion[1] != 0)
        {
            DrawAfterimage(cs->illusion[1], 1);
        }
    }
}
