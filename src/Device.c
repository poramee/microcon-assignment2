#include "Device.h"
#include <stdio.h>
#include <string.h>

#include "fatfs_sd.h"
#include "stdbool.h"
#include "fatfs.h"

/* VARIABLES  ---------------------------------------------*/
extern DeviceParams* deviceParamsPtr;
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;

extern uint32_t count2;
extern uint32_t count3;
extern uint32_t count4;

Time Clear_Time = {0,0,0};


/* MUSIC FUNCTIONS  -----------------------------------------*/


FATFS fs;
FIL fil;
FRESULT fres;

volatile bool end_of_file_reached = false;
volatile bool read_next_chunk = false;
volatile uint16_t* signal_play_buff = NULL;
volatile uint16_t* signal_read_buff = NULL;
volatile uint16_t signal_buff1[4096];
volatile uint16_t signal_buff2[4096];

uint32_t fileSize;
uint32_t headerSizeLeft;
uint16_t compression;
uint16_t channelsNum;
uint32_t sampleRate;
uint32_t bytesPerSecond;
uint16_t bytesPerSample;
uint16_t bitsPerSamplePerChannel;
uint32_t dataSize;
unsigned int bytesRead;
HAL_StatusTypeDef hal_res;

extern I2S_HandleTypeDef hi2s2;

char tmp_txt[100];


// HAL_I2S
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
	if(end_of_file_reached)
		return;

	volatile uint16_t* temp = signal_play_buff;
	signal_play_buff = signal_read_buff;
	signal_read_buff = temp;

	int nsamples = sizeof(signal_buff1) / sizeof(signal_buff1[0]);
	HAL_I2S_Transmit_IT(&hi2s2, (uint16_t*)signal_play_buff, nsamples);
	read_next_chunk = true;
}

// init sd card
void initSDCARD() {
	fres = f_mount(&fs, "", 0);
	if (fres == FR_OK) {
		UART_Log("Micro SD card is mounted successfully!");
	} else if (fres != FR_OK) {
		UART_Log("Micro SD card's mount error!");
	}
}

// play wav file
int initWavFile(const char* fname) {
	sprintf(tmp_txt, "Openning %s...", fname);
	UART_Log(tmp_txt);
	fres = f_open(&fil, fname, FA_READ);
	if (fres == FR_OK) {
		UART_Log("f_open() Complete.");
	}
	else {
		UART_Log("f_open() Fail.");
		return -1;
	}
	
	UART_Log("File opened, reading...");
	
	uint8_t header[44];
	fres = f_read(&fil, header, sizeof(header), &bytesRead);
	if (fres != FR_OK) {
		UART_Log("f_read() failed.");
		f_close(&fil);
		return -2;
	}
	
	if (memcmp((const char*)header, "RIFF", 4) != 0) {
		sprintf(tmp_txt, "Wrong WAV signature at offset 0: 0x%02X 0x%02X 0x%02X 0x%02X", header[0], header[1], header[2], header[3]);
		UART_Log(tmp_txt);
		f_close(&fil);
		return -3;
	}
	
	if (memcmp((const char*)header + 8, "WAVEfmt ", 8) != 0) {
		UART_Log("Wrong WAV signature at offset 8!");
		f_close(&fil);
		return -4;
	}
	
	if (memcmp((const char*)header + 36, "data", 4) != 0) {
		UART_Log("Wrong WAV signature at offset 36!");
		f_close(&fil);
		return -5;
	}
	
	fileSize = 8 + (header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24));
	headerSizeLeft = header[16] | (header[17] << 8) | (header[18] << 16) | (header[19] << 24);
  compression = header[20] | (header[21] << 8);
  channelsNum = header[22] | (header[23] << 8);
  sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
  bytesPerSecond = header[28] | (header[29] << 8) | (header[30] << 16) | (header[31] << 24);
  bytesPerSample = header[32] | (header[33] << 8);
  bitsPerSamplePerChannel = header[34] | (header[35] << 8);
  dataSize = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);
	
	UART_Log("--- WAV header ---");
	sprintf(tmp_txt, "File size: %u", fileSize);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Header size left: %u", headerSizeLeft);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Compression (1 = no compression): %d", compression);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Channels num: %d", channelsNum);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Sample rate: %d", sampleRate);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Bytes per second: %d", bytesPerSecond);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Bytes per sample: %d", bytesPerSample);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Bits per sample per channel: %d", bitsPerSamplePerChannel);
	UART_Log(tmp_txt);
	sprintf(tmp_txt, "Data size: %d", dataSize);
	UART_Log(tmp_txt);
	UART_Log("------------------");
	
	if(headerSizeLeft != 16) {
		UART_Log("Wrong `headerSizeLeft` value, 16 expected");
		f_close(&fil);
		return -6;
	}

	if(compression != 1) {
		UART_Log("Wrong `compression` value, 1 expected");
		f_close(&fil);
		return -7;
	}

	if(channelsNum != 2) {
		UART_Log("Wrong `channelsNum` value, 2 expected");
		f_close(&fil);
		return -8;
	}

	if((sampleRate != 44100) || (bytesPerSample != 4) || (bitsPerSamplePerChannel != 16) || (bytesPerSecond != 44100*2*2) || (dataSize < sizeof(signal_buff1) + sizeof(signal_buff2))) {
		UART_Log("Wrong file format, 16 bit file with sample rate 44100 expected");
		f_close(&fil);
		return -9;
	}
	
	fres = f_read(&fil, (uint8_t*)signal_buff1, sizeof(signal_buff1), &bytesRead);
	if(fres != FR_OK) {
		sprintf(tmp_txt, "f_read() failed, res = %d", fres);
		UART_Log(tmp_txt);
		f_close(&fil);
		return -10;
	}
	
	fres = f_read(&fil, (uint8_t*)signal_buff2, sizeof(signal_buff2), &bytesRead);
	if(fres != FR_OK) {
		sprintf(tmp_txt, "f_read() failed, res = %d", fres);
		UART_Log(tmp_txt);
		f_close(&fil);
		return -11;
	}
	
	read_next_chunk = false;
  end_of_file_reached = false;
  signal_play_buff = signal_buff1;
  signal_read_buff = signal_buff2;
	
	int nsamples = sizeof(signal_buff1) / sizeof(signal_buff1[0]);

	hal_res = HAL_I2S_Transmit_IT(&hi2s2, (uint16_t*)signal_buff1, nsamples);
	
	if(hal_res != HAL_OK) {
		sprintf(tmp_txt, "I2S - HAL_I2S_Transmit failed, hal_res = %d!", hal_res);
		UART_Log(tmp_txt);
		f_close(&fil);
		return -12;
	}
	
	return 0;
}

int playMusic() {
	if (dataSize >= sizeof(signal_buff1)) {
		if(!read_next_chunk) {
			return 0;
		}
		
		read_next_chunk = false;
		
		fres = f_read(&fil, (uint8_t*)signal_read_buff, sizeof(signal_buff1), &bytesRead);
		if(fres != FR_OK) {
			sprintf(tmp_txt, "f_read() failed, res = %d\r\n", fres);
			UART_Log(tmp_txt);
			f_close(&fil);
			return -13;
		}
		
		dataSize -= sizeof(signal_buff1);
	}
	else if(!end_of_file_reached) {
		end_of_file_reached = true;
		fres = f_close(&fil);
		if(fres == FR_OK) {
			UART_Log("end music and close file complete.");
			if(deviceParamsPtr -> Music.currentSong < deviceParamsPtr -> Music.totalSongs - 1) Music_Next();
			else Music_Stop();
		}
		else if(fres != FR_OK) {
			sprintf(tmp_txt, "f_close() failed, res = %d\r\n", fres);
			Music_Stop();
			UART_Log(tmp_txt);
			return -14;
		}
	}
}



/* INIT FUNCTION  ------------------------------------------*/
void Device_Init(){
	UART_Log("INIT");
	while (MPU6050_Init(&hi2c1) == 1);
	initSDCARD();
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
		if(deviceParamsPtr -> currentTime.hour >= 24) deviceParamsPtr -> currentTime.hour = 0;
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


// Music
// Private Functions
void Music_Mute(){
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
}
void Music_UnMute(){
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
}

// Public Functions
void Music_FunctionLoop(){
	if(deviceParamsPtr -> Music.status == active || deviceParamsPtr -> isAlarmSoundPlaying) playMusic();
}

void Music_LoadSongNames(){
	UART_Log("Loading Song Name List");
       
	// FIL file;
	FRESULT open = f_open(&fil,"detail.txt", FA_READ);
	if(open == FR_OK) UART_Log("Read OK");
	
	char buffer[100];
	int songToLoad = deviceParamsPtr -> Music.currentSong;
	int lineCnt = 0;
	while(f_gets(buffer,sizeof(buffer),&fil)){
		if(lineCnt == 0){
			sscanf(buffer,"%d", &deviceParamsPtr -> Music.totalSongs);
		}
		else if(lineCnt - 1 == songToLoad){
			char *token = buffer;
			token = strtok(token,"$");
			sprintf(deviceParamsPtr -> Music.songName,"%s", token);
			
			token = strtok(NULL,"\n");
			sprintf(deviceParamsPtr -> Music.songArtist,"%s", token);
			
			UART_Log(deviceParamsPtr -> Music.songName);
			UART_Log(deviceParamsPtr -> Music.songArtist);
			
			break;
		}
		++lineCnt;
	}
}
void AlarmSound_Play(){
	if(deviceParamsPtr -> isAlarmSoundPlaying) return;
	deviceParamsPtr -> isAlarmSoundPlaying = 1;
	
	Music_Stop();
	Music_UnMute();
	char songNameToLoad[20];
	sprintf(songNameToLoad, "alarm.wav");
	initWavFile(songNameToLoad);
	
}

void AlarmSound_Stop(){
	Music_Mute();
	deviceParamsPtr -> isAlarmSoundPlaying = 0;
	f_close(&fil);
	signal_play_buff = NULL;
	signal_read_buff = NULL;
	for(int i = 0;i < 4096;++i){
		signal_buff1[i] = signal_buff2[i] = 0;
	}
	
}

void Music_Load(){
	Music_LoadSongNames();
	char songNameToLoad[100];
	sprintf(songNameToLoad, "%d.wav", deviceParamsPtr -> Music.currentSong);
	initWavFile(songNameToLoad);
}

void Music_Play(){
	Music_UnMute();
	
	if(deviceParamsPtr -> Music.status == inactive) Music_Load();
	deviceParamsPtr -> Music.status = active;
}

void Music_Pause(){
	Music_Mute();
	
	deviceParamsPtr -> Music.status = pause;
}

void Music_Stop(){
	Music_Mute();
	
	f_close(&fil);
	signal_play_buff = NULL;
	signal_read_buff = NULL;
	for(int i = 0;i < 4096;++i){
		signal_buff1[i] = signal_buff2[i] = 0;
	}
	
	deviceParamsPtr -> Music.status = inactive;
}

void Music_Next(){
	Music_Stop();
	if(deviceParamsPtr -> Music.currentSong < deviceParamsPtr -> Music.totalSongs - 1) deviceParamsPtr -> Music.currentSong++;
	Music_Play();
}

void Music_Prev(){
	Music_Stop();
	if(deviceParamsPtr -> Music.currentSong > 0) deviceParamsPtr -> Music.currentSong--;
	Music_Play();
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
		//UART_Log("Wake");
		deviceParamsPtr -> isIdle = 0;
	}
	else if(!(deviceParamsPtr -> isIdle) && deviceParamsPtr -> Accelerometer.KalmanAngleX <= 30){
		//UART_Log("Sleep");
		deviceParamsPtr -> isIdle = 1;
	}
	// deviceParamsPtr -> isIdle = 0; // Test Only
}





