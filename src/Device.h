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
	Status status;
	int currentSong;
	int totalSongs;
} MusicStatus;


typedef struct{
	Time currentTime;
	
	Status Alarm_status;
	Time Alarm_time;
	
	Status Timer_status;
	Time Timer_time;
	
	Status Stopwatch_status;
	Time Stopwatch_time;
	
	MusicStatus Music;
	
	uint8_t userBrightness;
}DeviceParams;

void checkMainClock(void );
void checkAlarmClock(void );
void UART_Log(char str[]);
void Timer_Start(void );
void Timer_Pause(void );
void Timer_Reset(void );
void updateTimer(void );

void Stopwatch_Start(void );
void Stopwatch_Pause(void );
void Stopwatch_Reset(void );
void updateStopwatch(void );


void Music_Load(void );
void Music_Play(void );
void Music_Pause(void );
void Music_Stop(void );
void Music_Next(void );
void Music_Prev(void );

int Accelerometer_Read(void );
