#ifndef MAIN_H
#define MAIN_H
#include "main.h"
#endif

typedef enum{
	inactive,
	active,
	pause,
	done
} Status;

typedef struct{
	int hour;
	int min;
	int sec;
} Time;


typedef struct{
	Time currentTime;
	
	Status Alarm_status;
	Time Alarm_time;
	
	Status Timer_status;
	Time Timer_time;
	
	Status Stopwatch_status;
	Time Stopwatch_time;
	
	uint8_t userBrightness;
}DeviceParams;

void checkMainClock(uint32_t *counter);
void checkAlarmClock(void );
void UART_Log(char str[]);
void Timer_Start(void );
void Timer_Pause(void );
void Timer_Reset(void );
void updateTimer(uint32_t counter);

