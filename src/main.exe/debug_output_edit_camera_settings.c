#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * debug_output_edit_camera_settings (0x8003076c, 0x274 bytes) edits one of
 * four camera SVECTORs with the held pad directions, restores all four
 * vectors when L2+R2 are held (a new L1 press cycles the edited
 * slot), and prints the current values.
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

extern u16 DEBUG_PAD_HELD_;
extern u16 DEBUG_PAD_PRESS_;
extern s16 DEBUG_CAMERA_INDEX_;
extern TCameraPos *DEBUG_CAMERA_BASE_;
extern SVECTOR *DEBUG_CAMERA_SLOTS_[4];
extern char *DEBUG_CAMERA_LABELS_[4];
extern char fmt_camera_edit[];

void debug_output_edit_camera_settings(s16 pad)
{
    SVECTOR *camera;
    char *format;
    s32 marker;
    s32 i;

    DEBUG_PAD_PRESS_ = DEBUG_PAD_HELD_;
    DEBUG_PAD_HELD_ = pad;
    DEBUG_PAD_PRESS_ = DEBUG_PAD_HELD_ & (DEBUG_PAD_HELD_ ^ DEBUG_PAD_PRESS_);

    if (DEBUG_PAD_PRESS_ & PADL1)
    {
        DEBUG_CAMERA_INDEX_++;
        if (DEBUG_CAMERA_INDEX_ >= 4)
        {
            DEBUG_CAMERA_INDEX_ = 0;
        }
    }

    camera = DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_INDEX_];
    if (DEBUG_PAD_HELD_ & PADLup)
    {
        camera->vz -= 50;
    }
    if (DEBUG_PAD_HELD_ & PADLdown)
    {
        camera->vz += 50;
    }
    if (DEBUG_PAD_HELD_ & PADRup)
    {
        camera->vy -= 50;
    }
    if (DEBUG_PAD_HELD_ & PADRdown)
    {
        camera->vy += 50;
    }
    if (DEBUG_PAD_HELD_ & PADRleft)
    {
        camera->vx -= 50;
    }
    if (DEBUG_PAD_HELD_ & PADRright)
    {
        camera->vx += 50;
    }

    if ((DEBUG_PAD_HELD_ & (PADL2 | PADR2)) == (PADL2 | PADR2))
    {
        *DEBUG_CAMERA_BASE_ = CamPosDefault;
    }

    i = 0;
    format = fmt_camera_edit;
    for (; i < 4; i++)
    {
        marker = ' ';
        if (DEBUG_CAMERA_INDEX_ == i)
        {
            marker = '*';
        }
        FntPrint(format, marker, DEBUG_CAMERA_LABELS_[i],
                 DEBUG_CAMERA_SLOTS_[i]->vx, DEBUG_CAMERA_SLOTS_[i]->vy,
                 DEBUG_CAMERA_SLOTS_[i]->vz);
    }
}
