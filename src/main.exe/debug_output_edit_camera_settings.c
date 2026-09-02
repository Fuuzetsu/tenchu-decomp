#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

extern u16 DEBUG_PAD_HELD_;
extern u16 DEBUG_PAD_PRESS_;
extern s16 DEBUG_CAMERA_INDEX_;
extern TCameraPos *DEBUG_CAMERA_BASE_;
extern SVECTOR *DEBUG_CAMERA_SLOTS_[N_DEBUG_CAMERA_SLOTS];
extern char *DEBUG_CAMERA_LABELS_[N_DEBUG_CAMERA_SLOTS];
extern char fmt_camera_edit[];

void debug_output_edit_camera_settings(s16 pad)
{
    enum
    {
        CAMERA_EDIT_STEP = 50
    };
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
        if (DEBUG_CAMERA_INDEX_ >= N_DEBUG_CAMERA_SLOTS)
        {
            DEBUG_CAMERA_INDEX_ = 0;
        }
    }

    camera = DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_INDEX_];
    if (DEBUG_PAD_HELD_ & PADLup)
    {
        camera->vz -= CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADLdown)
    {
        camera->vz += CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRup)
    {
        camera->vy -= CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRdown)
    {
        camera->vy += CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRleft)
    {
        camera->vx -= CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRright)
    {
        camera->vx += CAMERA_EDIT_STEP;
    }

    if ((DEBUG_PAD_HELD_ & (PADL2 | PADR2)) == (PADL2 | PADR2))
    {
        *DEBUG_CAMERA_BASE_ = CamPosDefault;
    }

    i = 0;
    format = fmt_camera_edit;
    for (; i < N_DEBUG_CAMERA_SLOTS; i++)
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
