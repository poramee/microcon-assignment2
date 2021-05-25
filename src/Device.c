#include "Device.h"
#include <stdio.h>
#include <string.h>

/* VARIABLES  ---------------------------------------------*/
extern DeviceParams* deviceParamsPtr;
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;

extern uint32_t count2;
extern uint32_t count3;
extern uint32_t count4;

Time Clear_Time = {0,0,0};

/* INIT FUNCTION  ------------------------------------------*/


void Device_Init(){
	while (MPU6050_Init(&hi2c1) == 1);
}

/* FUNCTIONS  ---------------------------------------------*/
void UART_Log(char str[]){
	HAL_UART_Transmit(&huart3,(uint8_t*) str,strlen(str),1000);
	HAL_UART_Transmit(&huart3,(uint8_t*) "\n\r",4,1000);
}
void checkMainClock(){
	if(count2 >= 1000){
		count2 = count2 - 1000;
		++deviceParamsPtr -> currentTime.sec;
		if(deviceParamsPtr -> currentTime.sec >= 60){
			deviceParamsPtr -> currentTime.sec = 0;
			++deviceParamsPtr -> currentTime.min;
		}
		if(deviceParamsPtr -> currentTime.min >= 60){
			deviceParamsPtr -> currentTime.min = 0;
			++deviceParamsPtr -> currentTime.hour;
		}
		else if(deviceParamsPtr -> currentTime.hour >= 24) deviceParamsPtr -> currentTime.hour = 0;
	}
}

void checkAlarmClock(){
	if(deviceParamsPtr -> Alarm_status == active){
		int hour, min, hourAlarm, minAlarm;
		hour = deviceParamsPtr -> currentTime.hour;
		min = deviceParamsPtr -> currentTime.min;
		
		hourAlarm = deviceParamsPtr -> Alarm_time.hour;
		minAlarm = deviceParamsPtr -> Alarm_time.min;
		
		if(hour == hourAlarm && min == minAlarm){
			deviceParamsPtr -> Alarm_status = done;
		}
	}
}


void Timer_Start(){
	deviceParamsPtr -> Timer_status = active;
	char str[20];
		sprintf(str,"%2d %2d %2d",deviceParamsPtr -> Timer_time.hour,deviceParamsPtr -> Timer_time.min,deviceParamsPtr -> Timer_time.sec);
		UART_Log(str);
	count4 = ((deviceParamsPtr -> Timer_time.hour)*3600000)+((deviceParamsPtr -> Timer_time.min)*60000)+((deviceParamsPtr -> Timer_time.sec)*1000);
}

void Timer_Pause(){
	deviceParamsPtr -> Timer_status = pause;
}

void Timer_Reset(){
	deviceParamsPtr -> Timer_time = Clear_Time;
	deviceParamsPtr -> Timer_status = inactive;
}

void updateTimer(){
	if(deviceParamsPtr -> Timer_status == active){
		deviceParamsPtr -> Timer_time.hour = count4/3600000;
		deviceParamsPtr -> Timer_time.min = (count4/60000)-(count4/3600000*60);
		deviceParamsPtr -> Timer_time.sec =(count4/1000)-(count4/60000*60);
		
		if(count4 <= 0){
			deviceParamsPtr -> Timer_time = Clear_Time;
			deviceParamsPtr -> Timer_status = done;
		}
	}
}


void Stopwatch_Start(){
	deviceParamsPtr -> Stopwatch_status = active;
	count3 = ((deviceParamsPtr -> Stopwatch_time.hour)*3600000)+((deviceParamsPtr -> Stopwatch_time.min)*60000)+((deviceParamsPtr -> Stopwatch_time.sec)*1000);
}

void Stopwatch_Pause(){
	deviceParamsPtr -> Stopwatch_status = pause;
}

void Stopwatch_Reset(){
	deviceParamsPtr -> Stopwatch_time = Clear_Time;
	deviceParamsPtr -> Stopwatch_status = inactive;
}

void updateStopwatch(){
	//char str[20];
	//sprintf(str,"%d %d %d",deviceParamsPtr -> Stopwatch_time.hour, deviceParamsPtr -> Stopwatch_time.min, deviceParamsPtr -> Stopwatch_time.sec);
	//UART_Log(str);
	if(deviceParamsPtr -> Stopwatch_status == active){
		deviceParamsPtr -> Stopwatch_time.hour = count3/3600000;
		deviceParamsPtr -> Stopwatch_time.min = (count3/60000)-(count3/3600000*60);
		deviceParamsPtr -> Stopwatch_time.sec =(count3/1000)-(count3/60000*60);
	}
}

void Music_Load(){
	sprintf(deviceParamsPtr -> Music.songName,"Hello Test 1234");
}

void Music_Play(){
	// TODO: Play Music Algorithm
	if(deviceParamsPtr -> Music.status == inactive) Music_Load(); // Maybe?
	
	deviceParamsPtr -> Music.status = active;
}

void Music_Pause(){
	// TODO: Pause Music Algorithm
	
	
	deviceParamsPtr -> Music.status = pause;
}

void Music_Stop(){
	// TODO: Stop Music Algorithm
	
	
	deviceParamsPtr -> Music.status = inactive;
}

void Music_Next(){
	
	
}

void Music_Prev(){
	
	
}

void Accelerometer_Read(){
	MPU6050_Read_All(&hi2c1, &(deviceParamsPtr -> Accelerometer));
	char str[50];
	sprintf(str,"%lf", deviceParamsPtr -> Accelerometer.KalmanAngleX);
	// UART_Log(str);
}

void checkAutoSleepWake(){
	Accelerometer_Read();
	if(deviceParamsPtr -> isIdle && deviceParamsPtr -> Accelerometer.KalmanAngleX >= 50){
		UART_Log("Wake");
		deviceParamsPtr -> isIdle = 0;
	}
	else if(!(deviceParamsPtr -> isIdle) && deviceParamsPtr -> Accelerometer.KalmanAngleX <= 30){
		UART_Log("Sleep");
		deviceParamsPtr -> isIdle = 1;
	}
	deviceParamsPtr -> isIdle = 0; // Test Only
}
