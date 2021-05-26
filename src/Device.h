#ifndef MAIN_H
#define MAIN_H
#include "main.h"
#include "mpu6050.h"
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
	char songName[100];
	char songArtist[100];
	int currentSong;
	int totalSongs;
} MusicStatus;


typedef struct{
	int isIdle;
	
	Time currentTime;
	
	Status Alarm_status;
	Time Alarm_time;
	
	Status Timer_status;
	Time Timer_time;
	
	Status Stopwatch_status;
	Time Stopwatch_time;
	
	MusicStatus Music;
	
	uint8_t userBrightness;
	
	MPU6050_t Accelerometer;
}DeviceParams;

void Device_Init(void );

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

void Music_FunctionLoop(void );
void Music_Load(void );
void Music_Play(void );
void Music_Pause(void );
void Music_Stop(void );
void Music_Next(void );
void Music_Prev(void );

void Accelerometer_Read(void );
void checkAutoSleepWake(void );
