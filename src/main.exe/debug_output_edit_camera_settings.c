#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * debug_output_edit_camera_settings (0x8003076c, 0x274 bytes) edits one of
 * four camera SVECTORs with the held pad directions, restores all four
 * vectors when L1+R1 are held, and prints the current values.
 *
 * Splat divides the original assembly at the interior
 * `__override__prt_800309b0...` call-site marker.  The first piece falls
 * straight through to the second; this is one ordinary C function, not a
 * jump table or a second entry point.
 *
 * Matching notes:
 *  - The one-frame button state is written through the globals themselves.
 *    The first assignment remains observable to the following global-based
 *    expression and reproduces both target `sh` stores; computing the result
 *    only through locals lets cc1 delete the first store.
 *  - The 32-byte camera reset is one align-2 aggregate assignment, producing
 *    the target's `lwl/lwr` and `swl/swr` block copy.
 *  - `i = 0` deliberately precedes the named format-pointer capture.  Reorg
 *    moves the zero into the reset guard's delay slot, while the format
 *    pointer lives in `$s3` across the four FntPrint calls.
 */

extern u16 BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT;
extern u16 BUTTONS_REGISTERED_FOR_ONE_FRAME_DURING_EXPANDED_DEBUG_OUTPUT;
extern s16 DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX;
extern u8 *CAMERA_PTR_ARRAY_START;
extern SVECTOR *CAMERA_POINTERS[4];
extern char *CAMERA_PROPERTIES[4];
extern char D_80011A50[];

void debug_output_edit_camera_settings(s16 pad)
{
    SVECTOR *camera;
    char *format;
    s32 marker;
    s32 i;

    BUTTONS_REGISTERED_FOR_ONE_FRAME_DURING_EXPANDED_DEBUG_OUTPUT =
        BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT;
    BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT = pad;
    BUTTONS_REGISTERED_FOR_ONE_FRAME_DURING_EXPANDED_DEBUG_OUTPUT =
        BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT &
        (BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT ^
         BUTTONS_REGISTERED_FOR_ONE_FRAME_DURING_EXPANDED_DEBUG_OUTPUT);

    if (BUTTONS_REGISTERED_FOR_ONE_FRAME_DURING_EXPANDED_DEBUG_OUTPUT & 4)
    {
        DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX =
            DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX + 1;
        if (DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX >= 4)
        {
            DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX = 0;
        }
    }

    camera = CAMERA_POINTERS[DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX];
    if (BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT & 0x1000)
    {
        camera->vz -= 0x32;
    }
    if (BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT & 0x4000)
    {
        camera->vz += 0x32;
    }
    if (BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT & 0x10)
    {
        camera->vy -= 0x32;
    }
    if (BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT & 0x40)
    {
        camera->vy += 0x32;
    }
    if (BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT & 0x80)
    {
        camera->vx -= 0x32;
    }
    if (BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT & 0x20)
    {
        camera->vx += 0x32;
    }

    if ((BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT & 3) == 3)
    {
        *(TCameraPos *)CAMERA_PTR_ARRAY_START = D_80011BC0;
    }

    i = 0;
    format = D_80011A50;
    for (; i < 4; i++)
    {
        marker = ' ';
        if (DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX == i)
        {
            marker = '*';
        }
        FntPrint(format, marker, CAMERA_PROPERTIES[i],
                 CAMERA_POINTERS[i]->vx, CAMERA_POINTERS[i]->vy,
                 CAMERA_POINTERS[i]->vz);
    }
}
