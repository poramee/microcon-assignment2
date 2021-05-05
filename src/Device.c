#include "Device.h"
#include <string.h>

/* VARIABLES  ---------------------------------------------*/
extern DeviceParams* deviceParamsPtr;
extern UART_HandleTypeDef huart3;

extern uint32_t count4;

Time Clear_Time = {0,0,0};

/* FUNCTIONS  ---------------------------------------------*/
void UART_Log(char str[]){
	HAL_UART_Transmit(&huart3,(uint8_t*) str,strlen(str),1000);
	HAL_UART_Transmit(&huart3,(uint8_t*) "\n\r",4,1000);
}
void checkMainClock(uint32_t *counter){
	if(*counter >= 1000){
		*counter = *counter - 1000;
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
#include <stdio.h>

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

void updateTimer(uint32_t counter){
	if(deviceParamsPtr -> Timer_status == active){
		deviceParamsPtr -> Timer_time.hour = counter/3600000;
		deviceParamsPtr -> Timer_time.min = (counter/60000)-(counter/3600000*60);
		deviceParamsPtr -> Timer_time.sec =(counter/1000)-(counter/60000*60);
		
		char str[20];
		sprintf(str,"%d",counter);
		UART_Log(str);
		if(counter <= 0){
			UART_Log("CLEAR");
			deviceParamsPtr -> Timer_time = Clear_Time;
			deviceParamsPtr -> Timer_status = done;
		}
	}
}
