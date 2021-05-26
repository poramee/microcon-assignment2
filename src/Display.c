#include "Display.h"
#include "ILI9341_GFX.h"
#include "ILI9341_STM32_Driver.h"
#include "ILI9341_Touchscreen.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* VARIABLES  ---------------------------------------------*/
extern TIM_HandleTypeDef htim5;

extern DeviceParams* deviceParamsPtr;
extern DisplayParams* displayParamsPtr;

const uint8_t CHAR_WIDTH = 6;
const uint8_t CHAR_HEIGHT = 8;

Rect targetRect[20];

uint8_t prevDrawHour;
uint8_t prevDrawMin;
uint8_t prevDrawSec;

int prevIsIdle = 1;

int prevCurrentSong = -2; // -2 = initial state, -1 = inactive

/* COLOR PALETTE  -------------------------------------*/
struct {
	uint16_t foreground;
	uint16_t background;
	uint16_t accent;
	uint16_t alert;
	uint16_t warning;
	uint16_t inactive;
	uint16_t active;
	uint16_t editable;
}Color = {
.foreground = 0x0000,
.background = 0xFFFF,
.accent = 0x04FF,
.alert = 0xF80B,
.warning = 0xFE40,
.inactive = 0x9CF3,
.active = 0x172D,
.editable = 0xC77F
};



/* LOGICS  --------------------------------------------*/
void updateScreen(){
	ILI9341_Fill_Screen(Color.background);
	displayParamsPtr -> forceUpdateScreen = 1;
}
void updateScreenComplete(){
	displayParamsPtr -> forceUpdateScreen = 0;
}

void changePage(Screen screen){
	displayParamsPtr -> prevScreen = displayParamsPtr -> currentScreen;
	displayParamsPtr -> currentScreen = screen;
	updateScreen();
}

int checkTouchPadTarget(Rect targetArea[], int arraySize){
	if(displayParamsPtr -> lassPressDuration < 200) return -1;
	if(TP_Touchpad_Pressed()){	
		uint16_t position_array[2];
		if(TP_Read_Coordinates(position_array) == TOUCHPAD_DATA_OK){
			//char str[10];
			//sprintf(str,"%d %d",position_array[0], position_array[1]);
			//UART_Log(str);
			for(int i = 0;i < arraySize;++i){
				uint16_t x0 = 240 - targetArea[i].y;
				uint16_t y0 = targetArea[i].x;
				uint16_t x1 = x0 - targetArea[i].height;
				uint16_t y1 = y0 + targetArea[i].width;
				if(x0 >= position_array[0] && x1 <= position_array[0] && y0 <= position_array[1] && y1 >= position_array[1]){
					displayParamsPtr -> lassPressDuration = 0;
					return i;	
				}
			}
		}
	}
	return -1;
}

void setBrightness(uint8_t newBrightness){
	if(newBrightness > displayParamsPtr -> brightness){
		float increment = (newBrightness - (displayParamsPtr -> brightness))/20.0;
		for(int i = 0;i < 20;++i){
			htim5.Instance -> CCR4 = (1000 - 1) * ((displayParamsPtr -> brightness) + (increment*i))/100.0;
			HAL_Delay(10);
		}
	}
	else if(newBrightness < displayParamsPtr -> brightness){
		float increment = ((displayParamsPtr -> brightness) - newBrightness)/20.0;
		for(int i = 0;i < 20;++i){
			htim5.Instance -> CCR4 = (uint32_t)((1000 - 1) * ((displayParamsPtr -> brightness) - (increment*i))/100.0);
			HAL_Delay(10);
		}
	}
	displayParamsPtr -> brightness = newBrightness;
	htim5.Instance -> CCR4 = (uint32_t)((1000 - 1) * (float)(displayParamsPtr -> brightness)/100.0);
}

uint8_t isClockUpdated(uint8_t resolution){ // 1 = hour, 2 = hour + min, 3 = hour + min + sec
	return (prevDrawHour != deviceParamsPtr -> currentTime.hour && resolution == 1) || (prevDrawMin != deviceParamsPtr -> currentTime.min && resolution == 2) || (prevDrawSec != deviceParamsPtr -> currentTime.sec && resolution == 3);
}

int isWaking(){
	int isWaking = prevIsIdle == 1 && deviceParamsPtr -> isIdle == 0;
	if(isWaking) prevIsIdle = deviceParamsPtr -> isIdle;
	return isWaking;
}

int isSleeping(){
	int isSleeping = prevIsIdle == 0 && deviceParamsPtr -> isIdle == 1;
	if(isSleeping) prevIsIdle = deviceParamsPtr -> isIdle;
	return isSleeping;
}

void textWrapingSongName(char line1[], char original[]){
	int length = strlen(original);
	if(length <= 13){
		sprintf(line1,"%s",original);
	}
	else{
		sprintf(line1,"%.13s",original);
	}
}

void textWrapingSongArtist(char line1[], char original[]){
	int length = strlen(original);
	if(length <= 13){
		sprintf(line1,"%s",original);
	}
	else{
		sprintf(line1,"%.18s",original);
	}
}

/* COMPONENTS  --------------------------------------------*/
void debugRect(Rect rect){
	ILI9341_Draw_Hollow_Rectangle_Coord(rect.x,rect.y,rect.x + rect.width, rect.y + rect.height, BLUE);
}

void Draw_Triangle(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY, uint16_t color, Rect *triangleTarget){
	
	for(int i = x;i <= x+sizeX;++i){
		for(int j = y;j <= y+sizeY;++j){
			int normalizeX = i - x;
			int normalizeY = j - y;
			//char str[50];
			//sprintf(str,"triangle: %d %d  - %d = %d ",normalizeX,normalizeY, normalizeY,(int)(((float)sizeY/(2*sizeX)) * normalizeX));
			//UART_Log(str);
			if(normalizeY >= (int)(((float)sizeY/(2*sizeX)) * normalizeX) && normalizeY <= (int)(((float)-sizeY/(2*sizeX)) * normalizeX) + sizeY) ILI9341_Draw_Pixel(i,j,color);
		}
	}
	Rect target = {x,y,sizeX,sizeY};
	*triangleTarget = target;
}

void Draw_TriangleFlip(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY, uint16_t color, Rect *triangleTarget){
	
	for(int i = x;i <= x+sizeX;++i){
		for(int j = y;j <= y+sizeY;++j){
			int normalizeX = i - x;
			int normalizeY = j - y;
			//char str[50];
			//sprintf(str,"triangle: %d %d  - %d = %d ",normalizeX,normalizeY, normalizeY,(int)(((float)sizeY/(2*sizeX)) * normalizeX));
			//UART_Log(str);
			// && normalizeY <= (int)(((float)sizeY/(2*sizeX)) * normalizeX) + sizeY
			if(normalizeY >= (int)(((float)-sizeY/(2*sizeX)) * normalizeX + sizeY / 2) && normalizeY <= (int)(((float)sizeY/(2*sizeX)) * normalizeX + sizeY/2)) ILI9341_Draw_Pixel(i,j,color);
		}
	}
	Rect target = {x,y,sizeX,sizeY};
	*triangleTarget = target;
}



void Draw_Custom_Clock_Sec(uint16_t x, uint16_t y,Time time, uint8_t fontSize, uint16_t color, uint16_t bg){
	char timeDisplay[10];
	sprintf(timeDisplay, "%.2d:%.2d:%.2d",time.hour,time.min,time.sec);
	ILI9341_Draw_Text(timeDisplay, x, y, color, fontSize, bg);
}

void Draw_Custom_Clock(uint16_t x, uint16_t y,Time time, uint8_t fontSize, uint16_t color, uint16_t bg){
	char timeDisplay[10];
	sprintf(timeDisplay, "%.2d:%.2d",time.hour,time.min);
	ILI9341_Draw_Text(timeDisplay, x, y, color, fontSize, bg);
}

void Draw_Clock_Sec(uint16_t x,uint16_t y, uint8_t fontSize, uint16_t color, uint16_t bg){
	prevDrawHour = deviceParamsPtr -> currentTime.hour;
	prevDrawMin = deviceParamsPtr -> currentTime.min;
	prevDrawSec = deviceParamsPtr -> currentTime.sec;
	
	Draw_Custom_Clock_Sec(x,y,deviceParamsPtr -> currentTime, fontSize, color,bg);
}

void Draw_Clock(uint16_t x,uint16_t y, uint8_t fontSize, uint16_t color, uint16_t bg){
	prevDrawHour = deviceParamsPtr -> currentTime.hour;
	prevDrawMin = deviceParamsPtr -> currentTime.min;
	prevDrawSec = deviceParamsPtr -> currentTime.sec;
	
	Draw_Custom_Clock(x,y,deviceParamsPtr -> currentTime, fontSize, color, bg);
}

void Draw_Button(uint16_t x, uint16_t y, char str[],uint8_t fontSize,uint16_t color, uint16_t bg, Rect *btnTarget){
	const uint16_t strLength = strlen(str);
	const uint16_t sizeX = (CHAR_WIDTH*fontSize) * (strLength + 2);
	const uint16_t sizeY = (CHAR_HEIGHT*fontSize) * 2;
	ILI9341_Draw_Filled_Rectangle_Coord(x,y,x+sizeX,y+sizeY,bg);
	ILI9341_Draw_Text(str, x+(CHAR_WIDTH*fontSize), y+((CHAR_HEIGHT*fontSize)/2.0), color, fontSize, bg);
	Rect returnRect = {x,y,sizeX,sizeY};
	*btnTarget = returnRect;
}

void Draw_BottomNavBar(Rect *alarmTarget, Rect *timerTarget, Rect *stopwatchTarget){
	Draw_Button(10,200,"Alarm", 2, WHITE, Color.accent, alarmTarget);
	if(deviceParamsPtr -> Alarm_status == active){
		ILI9341_Draw_Filled_Rectangle_Coord(alarmTarget -> x + 5,alarmTarget -> y + alarmTarget -> height - 5, alarmTarget -> x + alarmTarget -> width - 5,alarmTarget -> y + alarmTarget -> height, Color.active);
	}
	Draw_Button(alarmTarget -> x + alarmTarget -> width,200,"Timer", 2, WHITE, Color.accent, timerTarget);
	if(deviceParamsPtr -> Timer_status != inactive){
		uint16_t color = Color.active;
		if(deviceParamsPtr -> Timer_status == pause) color = Color.warning;
		ILI9341_Draw_Filled_Rectangle_Coord(timerTarget -> x + 5,timerTarget -> y + timerTarget -> height - 5, timerTarget -> x + timerTarget -> width - 5,timerTarget -> y + timerTarget -> height, color);
	}
	Draw_Button(timerTarget -> x + timerTarget -> width,200,"Stopwatch", 2, WHITE, Color.accent, stopwatchTarget);
	if(deviceParamsPtr -> Stopwatch_status != inactive){
		uint16_t color = Color.active;
		if(deviceParamsPtr -> Stopwatch_status == pause) color = Color.warning;
		ILI9341_Draw_Filled_Rectangle_Coord(stopwatchTarget -> x + 5,stopwatchTarget -> y + stopwatchTarget -> height - 5, stopwatchTarget -> x + stopwatchTarget -> width - 5,stopwatchTarget -> y + stopwatchTarget -> height, color);
	}
}

void Draw_TopBar(char title[],Rect *backBtnTarget){
	Draw_TriangleFlip(10,11,14,14,BLACK,backBtnTarget);
	char str[40];
	sprintf(str,"%.21s",title);
	ILI9341_Draw_Text(str,35,10,Color.foreground, 2, Color.background);
}

void Draw_ToggleSW(uint8_t x,uint8_t y,Status status, uint8_t fontSize,Rect *swTarget){
	const uint16_t sizeX = (CHAR_WIDTH*fontSize) * 7;
	const uint16_t sizeY = (CHAR_HEIGHT*fontSize) * 2;
	ILI9341_Draw_Filled_Rectangle_Coord(x,y,x+sizeX,y+sizeY,status == inactive? Color.inactive:Color.active);
	char str[5];
	if(status == inactive){
		sprintf(str,"%6s ","OFF");
		ILI9341_Draw_Text(str, x, y+((CHAR_HEIGHT*fontSize)/2.0), Color.background, fontSize, Color.inactive);
		ILI9341_Draw_Filled_Rectangle_Coord(x + 5 ,y + (CHAR_HEIGHT*fontSize*0.25),x+(CHAR_WIDTH*fontSize*2),y+(CHAR_HEIGHT*fontSize*1.75),WHITE);
	}
	else{
		sprintf(str," ON");
		ILI9341_Draw_Text(str, x, y+((CHAR_HEIGHT*fontSize)/2.0), Color.background, fontSize, Color.active);
		ILI9341_Draw_Filled_Rectangle_Coord(x + sizeX - (CHAR_WIDTH*fontSize*2) ,y + (CHAR_HEIGHT*fontSize*0.25),x + sizeX - 5,y+(CHAR_HEIGHT*fontSize*1.75),WHITE);
	}
	
	Rect returnRect = {x,y,sizeX,sizeY};
	*swTarget = returnRect;
}

void Draw_TopButton(char title[],uint8_t isActive,Rect *btnTarget){
	ILI9341_Draw_Text(title,(CHAR_WIDTH*2) * (26 - strlen(title)),10,isActive? Color.accent: Color.inactive, 2, Color.background);
	Rect returnRect = {(CHAR_WIDTH*2) * (26 - strlen(title)),0,(CHAR_WIDTH*2) * (24),10 + (CHAR_HEIGHT*2)};
	*btnTarget = returnRect;
}

void Draw_PauseButton(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY,uint16_t thickness, uint16_t color, Rect* target){
	ILI9341_Draw_Filled_Rectangle_Coord(x,y,x + thickness,y + sizeY,color);
	ILI9341_Draw_Filled_Rectangle_Coord(x + sizeX - thickness,y,x + sizeX,y + sizeY,color);
	
	Rect temp = {x,y,sizeX,sizeY};
	*target = temp;
}

void Draw_NextSongButton(uint16_t x, uint16_t y,uint16_t color, Rect *target){
	Draw_Triangle(x, y, 30, 30, color, target);
	ILI9341_Draw_Filled_Rectangle_Coord(x + 30,y,x + 33,y + 30,color);
}

void Draw_PrevSongButton(uint16_t x, uint16_t y,uint16_t color, Rect *target){
	Draw_TriangleFlip(x + 3, y, 30, 30, color, target);
	ILI9341_Draw_Filled_Rectangle_Coord(x,y,x + 3,y + 30,color);
}
void Draw_CurrentMusicTopBar(){
	if(deviceParamsPtr -> Music.status == active && (prevCurrentSong != deviceParamsPtr -> Music.currentSong || displayParamsPtr -> forceUpdateScreen)){
		Draw_Triangle(10,10,20,20,Color.active,NULL);
		char str[40];
		sprintf(str,"%.21s",deviceParamsPtr -> Music.songName);
		ILI9341_Draw_Text(str,45,12,Color.foreground, 2, Color.background);
		prevCurrentSong = deviceParamsPtr -> Music.currentSong;
	}
	if((deviceParamsPtr -> Music.status == inactive || deviceParamsPtr -> Music.status == pause) && (prevCurrentSong != -1 || displayParamsPtr -> forceUpdateScreen)){
		Draw_Triangle(10,10,20,20,Color.background,NULL);
		char str[40];
		sprintf(str,"%.21s",deviceParamsPtr -> Music.songName);
		ILI9341_Draw_Text(str,45,12,Color.background, 2, Color.background);
		prevCurrentSong = -1;
	}
}



/* PAGES  -------------------------------------------------*/

void Idle_Page(){
	setBrightness(10);
	if(isClockUpdated(2) || displayParamsPtr -> forceUpdateScreen){
		Draw_Clock(75,90, 6, Color.foreground, Color.background);
	}
	Draw_CurrentMusicTopBar(); // Dynamically Update
	updateScreenComplete();
}

void Home_Page(){
	if(displayParamsPtr -> forceUpdateScreen){
		setBrightness(deviceParamsPtr -> userBrightness);
		Draw_Button(20,120,"MUSIC", 3, WHITE, Color.accent, &targetRect[0]);
		Draw_Button(175,120,"CLOCK", 3, WHITE, Color.accent, &targetRect[1]);
		Draw_Button(180,200,"SETTINGS", 2, WHITE, 0x0000, &targetRect[2]);
	}
	
	Draw_CurrentMusicTopBar(); // Dynamically Update
	
	if(isClockUpdated(2) || displayParamsPtr -> forceUpdateScreen){
		Draw_Clock(70,30, 6, Color.foreground, Color.background);
	}
	
	updateScreenComplete();
	
	const int touchOnTarget = checkTouchPadTarget(targetRect, 3);
	switch(touchOnTarget){
		case 0:
			changePage(Music);
			break;
		case 1:
			changePage(Clock);
			break;
		case 2:
			changePage(Settings);
			break;
		default:
			break;
	}
}

void Clock_Page(){
	if(displayParamsPtr -> forceUpdateScreen){
		Draw_TopBar("CLOCK",&targetRect[0]);
		Draw_BottomNavBar(&targetRect[1], &targetRect[2], &targetRect[3]);
	}
	if(isClockUpdated(3) || displayParamsPtr -> forceUpdateScreen) Draw_Clock_Sec(40,90,5, Color.foreground, Color.background);
	
	updateScreenComplete();
	
	const int touchOnTarget = checkTouchPadTarget(targetRect, 4);
	switch(touchOnTarget){
		case 0:
			changePage(Home);
			break;
		case 1:
			changePage(Alarm);
			break;
		case 2:
			changePage(Timer);
			break;
		case 3:
			changePage(Stopwatch);
			break;
		default:
			break;
	}
}

uint8_t alarmEdit = 0;
void Alarm_Page(){
	if(displayParamsPtr -> forceUpdateScreen){
		Draw_TopBar("ALARM", &targetRect[0]);
		Draw_TopButton(alarmEdit == 0?"EDIT":"DONE",1,&targetRect[2]);
		uint16_t clockColor = Color.foreground;
		if(!alarmEdit && deviceParamsPtr -> Alarm_status == inactive) clockColor = Color.inactive;
		Draw_Custom_Clock(20,95,deviceParamsPtr -> Alarm_time,4,clockColor, alarmEdit? Color.editable:Color.background);
		Draw_ToggleSW(225,95,deviceParamsPtr -> Alarm_status, 2,&targetRect[1]);
	}
	
	updateScreenComplete();

	int touchOnTarget;
	if(alarmEdit){
		Rect adjustHour = {.x = 20,.y = 95, .width = CHAR_WIDTH*4*2, .height = CHAR_HEIGHT*4};
		Rect adjustMin = {.x = 20 + CHAR_WIDTH*4*3,.y = 95, .width = CHAR_WIDTH*4*2, .height = CHAR_HEIGHT*4};
		targetRect[3] = adjustHour;
		targetRect[4] = adjustMin;
		touchOnTarget = checkTouchPadTarget(targetRect, 5);
	}
	else touchOnTarget = checkTouchPadTarget(targetRect, 3);
	
	switch(touchOnTarget){
		case 0:
			alarmEdit = 0;
			changePage(Clock);
			break;
		case 1:
			if(deviceParamsPtr -> Alarm_status == inactive) deviceParamsPtr -> Alarm_status = active;
			else deviceParamsPtr -> Alarm_status = inactive;
			updateScreen();
			break;
		case 2:
			alarmEdit = !alarmEdit;
			updateScreen();
			break;
		case 3:
			deviceParamsPtr -> Alarm_time.hour++;
			deviceParamsPtr -> Alarm_time.hour %= 24;
			updateScreen();
			break;
		case 4:
			deviceParamsPtr -> Alarm_time.min++;
			deviceParamsPtr -> Alarm_time.min %= 60;
			updateScreen();
			break;
		default:
			break;
	}
}

uint8_t editTimer = 0;
uint8_t timerSubpage = 0;
Time prevTimer = {0,0,0};
void Timer_Page(){
	uint16_t timerReady = deviceParamsPtr -> Timer_time.hour > 0 || deviceParamsPtr -> Timer_time.min > 0 || deviceParamsPtr -> Timer_time.sec > 0;
	if(displayParamsPtr -> forceUpdateScreen){
		Draw_TopBar("TIMER", &targetRect[0]);
		
		if(deviceParamsPtr -> Timer_status == inactive){
			Draw_TopButton(editTimer?"DONE":"SET",1, &targetRect[1]);
			Draw_Button(40,150,"   START   ",3,WHITE,timerReady && !editTimer?Color.active:Color.inactive,&targetRect[2]);
			if(editTimer){
				Rect adjustHour = {.x = 40,.y = 90, .width = CHAR_WIDTH*5*2, .height = CHAR_HEIGHT*5};
				Rect adjustMin = {.x = 40 + CHAR_WIDTH*5*3,.y = 90, .width = CHAR_WIDTH*5*2, .height = CHAR_HEIGHT*5};
				Rect adjustSec = {.x = 40 + CHAR_WIDTH*5*6,.y = 90, .width = CHAR_WIDTH*5*2, .height = CHAR_HEIGHT*5};
				targetRect[3] = adjustHour;
				targetRect[4] = adjustMin;
				targetRect[5] = adjustSec;
				timerSubpage = 1;
			}
			else timerSubpage = 0;
		}
		else if(deviceParamsPtr -> Timer_status == active){
			Draw_Button(40,150,"RESET",2,WHITE,Color.accent,&targetRect[2]);
			Draw_Button(196,150,"PAUSE",2,WHITE,Color.warning,&targetRect[3]);
			timerSubpage = 2;
		}
		else if(deviceParamsPtr -> Timer_status == pause){
			Draw_Button(40,150,"RESET",2,WHITE,Color.accent,&targetRect[2]);
			Draw_Button(184 ,150,"RESUME",2,WHITE,Color.active,&targetRect[3]);
			timerSubpage = 3;
		}
		else if(deviceParamsPtr -> Timer_status == done){
			Draw_Button(40,150,"   RESET   ",3,WHITE,Color.accent,&targetRect[2]);
			timerSubpage = 4;
		}
	}
	
	if( prevTimer.hour != deviceParamsPtr -> Timer_time.hour ||
			prevTimer.min != deviceParamsPtr -> Timer_time.min ||
			prevTimer.sec != deviceParamsPtr -> Timer_time.sec ||
			displayParamsPtr -> forceUpdateScreen){
					Draw_Custom_Clock_Sec(40,70,deviceParamsPtr -> Timer_time,5, Color.foreground, editTimer? Color.editable:Color.background);
		}

	updateScreenComplete();
	
	int touchOnTarget = checkTouchPadTarget(targetRect,6);
	
	if(touchOnTarget == 0){
		changePage(Clock);
	}
	else{
		if(timerSubpage == 0){
			switch(touchOnTarget){
				case 1:
					editTimer = !editTimer;
					updateScreen();
					break;
				case 2:
					if(timerReady) Timer_Start();
					updateScreen();
					break;
				default:
					break;
			}
		}
		else if(timerSubpage == 1){
			switch(touchOnTarget){
			case 1:
				editTimer = !editTimer;
				updateScreen();
				break;
			case 3:
				deviceParamsPtr -> Timer_time.hour++;
				deviceParamsPtr -> Timer_time.hour %= 99;
				updateScreen();
				break;
			case 4:
				deviceParamsPtr -> Timer_time.min++;
				deviceParamsPtr -> Timer_time.min %= 60;
				updateScreen();
				break;
			case 5:
				deviceParamsPtr -> Timer_time.sec++;
				deviceParamsPtr -> Timer_time.sec %= 60;
				updateScreen();
				break;
			default:
				break;
				
			}
		}
		else if(timerSubpage == 2){
			switch(touchOnTarget){
				case 2:
					Timer_Reset();
					updateScreen();
					break;
				case 3:
					Timer_Pause();
					updateScreen();
				default:
					break;
			}
		}
		else if(timerSubpage == 3){
			switch(touchOnTarget){
				case 2:
					Timer_Reset();
					updateScreen();
					break;
				case 3:
					Timer_Start();
					updateScreen();
					break;
				default:
					break;
			}
		}
		else if(timerSubpage == 4){
			switch(touchOnTarget){
				case 2:
					Timer_Reset();
					updateScreen();
					break;
				default:
					break;
			}
		}
	}
}

uint8_t StopwatchSubpage=0;
Time prevStopwatch = {0,0,0};
void Stopwatch_Page(){
	if(displayParamsPtr -> forceUpdateScreen){
		Draw_TopBar("STOPWATCH", &targetRect[0]);
		
		if(deviceParamsPtr -> Stopwatch_status == inactive){
			Draw_Button(40,150,"   START   ",3,WHITE,Color.active,&targetRect[1]);
			StopwatchSubpage = 0;
		}
		else if(deviceParamsPtr -> Stopwatch_status == active){
			Draw_Button(40,150,"RESET",2,WHITE,Color.accent,&targetRect[1]);
			Draw_Button(196,150,"PAUSE",2,WHITE,Color.warning,&targetRect[2]);
			StopwatchSubpage = 1;
		}
		else if(deviceParamsPtr -> Stopwatch_status == pause){
			Draw_Button(40,150,"RESET",2,WHITE,Color.accent,&targetRect[1]);
			Draw_Button(184 ,150,"RESUME",2,WHITE,Color.active,&targetRect[2]);
			StopwatchSubpage = 2;
		}
	}
	
	if( prevStopwatch.hour != deviceParamsPtr -> Stopwatch_time.hour ||
			prevStopwatch.min != deviceParamsPtr -> Stopwatch_time.min ||
			prevStopwatch.sec != deviceParamsPtr -> Stopwatch_time.sec ||
			displayParamsPtr -> forceUpdateScreen){
					Draw_Custom_Clock_Sec(40,70,deviceParamsPtr -> Stopwatch_time,5, Color.foreground, Color.background);
	}

	updateScreenComplete();
	int touchOnTarget = checkTouchPadTarget(targetRect,3);
	
	
	if(touchOnTarget == 0){
		changePage(Clock);
	}
	else{
		if(StopwatchSubpage == 0){
			switch(touchOnTarget){
				case 1:
					Stopwatch_Start();
					updateScreen();
					break;
				default:
					break;
			}
		}
		else if(StopwatchSubpage == 1){
			switch(touchOnTarget){
				case 1:
					Stopwatch_Reset();
					updateScreen();
					break;
				case 2:
					Stopwatch_Pause();
					updateScreen();
				default:
					break;
			}
		}
		else if(StopwatchSubpage == 2){
			switch(touchOnTarget){
				case 1:
					Stopwatch_Reset();
					updateScreen();
					break;
				case 2:
					Stopwatch_Start();
					updateScreen();
					break;
				default:
					break;
			}
		}
	}
}

void Settings_Page(){
	if(displayParamsPtr -> forceUpdateScreen){
		Draw_TopBar("SETTINGS", &targetRect[0]);
		
		// TIME
		ILI9341_Draw_Text("TIME",10,54,Color.foreground,2,Color.background);
		Draw_Clock(200,50,3,Color.foreground,Color.editable);
		Rect adjustClockHour = {200,50,36,24};
		Rect adjustClockMin = {254,50,36,24};
		
		// BRIGHTNESS
		ILI9341_Draw_Text("BRIGHTNESS",10,100,Color.foreground,2,Color.background);
		char brStr[5];
		sprintf(brStr,"%3d %%", deviceParamsPtr -> userBrightness);
		ILI9341_Draw_Text(brStr,200,96,Color.foreground,3,Color.editable);
		Rect adjustBrightness = {200,96,90,24};
		
		targetRect[1] = adjustClockHour;
		targetRect[2] = adjustClockMin;
		targetRect[3] = adjustBrightness;
	}
	if(isClockUpdated(2)) Draw_Clock(200,50,3,Color.foreground,Color.editable);
	
	updateScreenComplete();
	
	switch(checkTouchPadTarget(targetRect, 4)){
		case 0:
			changePage(Home);
			break;
		case 1:
			deviceParamsPtr -> currentTime.hour++;
			deviceParamsPtr -> currentTime.hour %= 24;
			updateScreen();
			break;
		case 2:
			deviceParamsPtr -> currentTime.min++;
			deviceParamsPtr -> currentTime.min %= 60;
			updateScreen();
			break;
		case 3:
			deviceParamsPtr -> userBrightness += 10;
			if(deviceParamsPtr -> userBrightness > 100) deviceParamsPtr -> userBrightness = 10;
			setBrightness(deviceParamsPtr -> userBrightness);
			updateScreen();
			break;
		default:
			break;
	}
					
	
}

void Music_Page(){
	if(displayParamsPtr -> forceUpdateScreen){
		
		Draw_TopBar("MUSIC", &targetRect[0]);
		
		char songNameLine[2][50];
		
		textWrapingSongName(songNameLine[0], deviceParamsPtr -> Music.songName);
		textWrapingSongArtist(songNameLine[1], deviceParamsPtr -> Music.songArtist);
		
		ILI9341_Draw_Text(songNameLine[0], 35, 70, Color.foreground, 3 , Color.background);
		ILI9341_Draw_Text(songNameLine[1], 35, 100, Color.foreground, 2 , Color.background);
		
		//int playButtonX = 106 - (deviceParamsPtr -> Music.status == active? 9: 0);
		//Draw_Button(playButtonX,150,(deviceParamsPtr -> Music.status == active? "PAUSE": "PLAY"), 3, WHITE, Color.accent, &targetRect[1]);
		//Draw_Button(38,158,"<<", 2, WHITE, BLACK, &targetRect[2]);
		//Draw_Button(234,158,">>", 2, WHITE, BLACK, &targetRect[3]);
		
		if(deviceParamsPtr -> Music.status == active) Draw_PauseButton(135,170,50,50,20,Color.accent,&targetRect[1]);
		else Draw_Triangle(135,170,50,50,Color.accent,&targetRect[1]);
		Draw_PrevSongButton(57,180,Color.foreground,&targetRect[2]);
		Draw_NextSongButton(230,180,Color.foreground, &targetRect[3]);
		
		if(deviceParamsPtr -> Music.status == active) prevCurrentSong = deviceParamsPtr -> Music.currentSong;
		else prevCurrentSong = -1;
	}
	
	updateScreenComplete();
	
	switch(checkTouchPadTarget(targetRect, 4)){
		case 0:
			changePage(Home);
			break;
		case 1:
			deviceParamsPtr -> Music.status == active? Music_Pause() : Music_Play();
			updateScreen();
			break;
		case 2:
			Music_Prev();
			updateScreen();
			break;
		case 3:
			Music_Next();
			updateScreen();
			break;
		default:
			if(deviceParamsPtr -> Music.status == inactive && prevCurrentSong != -1){
				prevCurrentSong = -1;
				updateScreen();
			}
			break;
	}
}




/* POP-UP PAGES  -------------------------------------------------*/

void AlarmPopup_Page(){
	if(isClockUpdated(2) || displayParamsPtr -> forceUpdateScreen){
		ILI9341_Fill_Screen(Color.alert);
		Draw_Clock(70,30, 6, WHITE, Color.alert);
		ILI9341_Draw_Text("ALARM",100,100,Color.background,4,Color.alert);
		Draw_Button(106,180,"STOP",3,WHITE,Color.accent,&targetRect[0]);
	}
	
	updateScreenComplete();
	
	switch(checkTouchPadTarget(targetRect, 1)){
		case 0:
			deviceParamsPtr -> Alarm_status = inactive;
			changePage(displayParamsPtr -> prevScreen);
			break;
	}
}

void TimerPopup_Page(){
	if(displayParamsPtr -> forceUpdateScreen){
		ILI9341_Fill_Screen(Color.alert);
		ILI9341_Draw_Text("TIMER",100,100,Color.background,4,Color.alert);
		Draw_Button(106,180,"STOP",3,WHITE,Color.accent,&targetRect[0]);
	}
	
	updateScreenComplete();
	
	switch(checkTouchPadTarget(targetRect, 1)){
		case 0:
			deviceParamsPtr -> Timer_status = inactive;
			changePage(displayParamsPtr -> prevScreen);
			break;
	}
}


/* MAIN  --------------------------------------------------*/

void Display_Init(){
	ILI9341_Init();
	ILI9341_Fill_Screen(Color.background);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_1);
	setBrightness(deviceParamsPtr -> userBrightness);
}



void displayScreen(){
	if(deviceParamsPtr -> Alarm_status == done && displayParamsPtr -> currentScreen != AlarmPopup && displayParamsPtr -> currentScreen != TimerPopup){
		changePage(AlarmPopup);
	}
	if(deviceParamsPtr -> Timer_status == done && displayParamsPtr -> currentScreen != TimerPopup && displayParamsPtr -> currentScreen != AlarmPopup){
		changePage(TimerPopup);
	}
	
	if(isWaking()){
		setBrightness(deviceParamsPtr -> userBrightness);
		changePage(Home);
	}
	else if(isSleeping()) changePage(Idle);
	
	
	switch(displayParamsPtr -> currentScreen){
		case Idle:
			Idle_Page();
			break;
		case Home:
			Home_Page();
			break;
		case Clock:
			Clock_Page();
			break;
		case Alarm:
			Alarm_Page();
			break;
		case Timer:
			Timer_Page();
			break;
		case Stopwatch:
			Stopwatch_Page();
			break;
		case Music:
			Music_Page();
			break;
		case Settings:
			Settings_Page();
			break;
		case AlarmPopup:
			AlarmPopup_Page();
			break;
		case TimerPopup:
			TimerPopup_Page();
			break;
		default:
			break;
	}
}

