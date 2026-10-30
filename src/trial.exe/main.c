#include "common.h"
#include "trial.exe.h"
#include "../main.exe/adt.h"
#include "../main.exe/effect.h"
#include "../main.exe/filesystem.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/item.h"
#include "../main.exe/misc.h"
#include "../main.exe/sound.h"
#include <psxsdk/libgpu.h>

int main(void)
{
    AdtPadRead = GetRealPad;
    ResetCallback();
    InitPadControl();
    SystemFlag = 0;
    InitFileSystem(READ_SOURCE_CDROM);
    CdaStatus.flag = CDA_FLAG_ACTIVE;
    InitGraphicsSystem();
    InitAccessInfo();
    ControlScheme = ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->control_scheme;
    InitConflict();
    InitEffect();
    InitializeItem();
    InitializeInfoView();
    InitMisc();
    InitSoundEffect();
    InitializeImage();
    SetupSoundEffect(RIKIMARU_0, TRIAL_MENU_SOUND_SET);
    RunMissionEditor();
    ClearTrialPages();
    SetDispMask(0);
    exec_process_(PROCESS_MENU);
}
