#include "Core.h"
#ifndef PARAMS_DEFAULT
#define PARAMS_DEFAULT
DisplayParams DisplayParams_Default = {
.currentScreen=Idle,
.prevScreen=Idle,
.forceUpdateScreen=1,
.brightness=50,
.lassPressDuration = 0,
};

DeviceParams DeviceParams_Default = {
.isIdle=0,
.currentTime={0,0,0},
.Alarm_status=inactive,
.Alarm_time={0,0,0},
.isAlarmSoundPlaying=0,
.Timer_status=inactive,
.Timer_time={0,0,0},
.Music={inactive,"","", 0,0},
.userBrightness=50
};
#endif

#ifndef PARAMS_PTR
#define PARAMS_PTR
DeviceParams* deviceParamsPtr;
DisplayParams* displayParamsPtr;
#endif


void Core_Init(DeviceParams* devicePtr,DisplayParams* displayPtr){
	*devicePtr = DeviceParams_Default;
	*displayPtr = DisplayParams_Default;
	deviceParamsPtr = devicePtr;
	displayParamsPtr = displayPtr;
	Display_Init();
	Device_Init();
}

