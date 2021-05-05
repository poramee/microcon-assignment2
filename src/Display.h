#ifndef DEVICE_H
#define DEVICE_H
#include "Device.h"
#endif
typedef enum{
	Idle,
	Home,
	Music,
	Clock,
	Alarm,
	Stopwatch,
	Timer,
	Settings,
	AlarmPopup,
	TimerPopup
} Screen;


typedef struct{
	Screen currentScreen;
	Screen prevScreen;
	uint8_t forceUpdateScreen;
	uint8_t brightness;
} DisplayParams;

typedef struct{
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
} Rect;


void Display_Init(void );
void displayScreen(void );
