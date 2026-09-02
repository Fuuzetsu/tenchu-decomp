#include "common.h"
#include "main.exe.h"

extern TCameraPos *DEBUG_CAMERA_BASE_;
extern SVECTOR *DEBUG_CAMERA_SLOTS_[N_DEBUG_CAMERA_SLOTS];

void initialise_default_player_cameras_(void)
{
    CamPos = CamPosDefault;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_R1] =
        &DEBUG_CAMERA_BASE_->r1;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_R2] =
        &DEBUG_CAMERA_BASE_->r2;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_P1] =
        &DEBUG_CAMERA_BASE_->p1;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_P2] =
        &DEBUG_CAMERA_BASE_->p2;
}
