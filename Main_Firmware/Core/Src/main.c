/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <String.h>
#include <Stdio.h>
#include <Stdbool.h>
#include <Stdlib.h>

#include "lcd.h"
#include "sim.h"
#include "LoRa.h"
#include "struct.h"
#include "List.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
FATFS fs;  // file system
FIL fil1; // File
FIL fil2; // File
FRESULT fresult;  // result
UINT br;

LoRa myLoRa;	//lora object

RTC_TimeTypeDef time;	//RTC
RTC_DateTypeDef date;

static dict_t **gDict;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg;

RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_sdio;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
uint8_t 				gRxBuf[RxBuf_SIZE];			//UART Handle
uint8_t 				gMainBuf[MainBuf_SIZE];	
uint8_t 				gTxBuf[TxBuf_SIZE];
uint32_t 				gOldPos;
uint32_t 				gNewPos;

static bool 		gSimFlag	=	false;	//Sim Start Flags
static bool 		gLoraRecFlag	=	false;	//LoRa Recieved Flags

static uint8_t 	gSec;	//Real Time
static uint8_t 	gMinu;
static uint8_t 	gHour;
static uint8_t 	gDay;
static uint8_t 	gMon;
static uint8_t 	gYear;

static char 	gLoraReadStr[200];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_IWDG_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_RTC_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
FRESULT mount_SD (void)
{
	return f_mount(&fs, "a", 1);
}

FRESULT unmount_SD (void)
{
	return f_mount(NULL, "a", 1);
}

FRESULT open_File(char* name,int i)
{
	if(i	==	1)
	{
		return f_open(&fil1,	name,	FA_WRITE	|	FA_READ	|	FA_OPEN_EXISTING);
	}
	else
	{
		return f_open(&fil2,	name,	FA_WRITE	|	FA_READ	|	FA_OPEN_EXISTING);
	}
}


FRESULT close_File(int i)
{
	if(i	==	1)
	{
		return f_close(&fil1);
	}
	else
	{
		return f_close(&fil2);
	}
}

FRESULT read_File(void* data,int j,int i)
{
	if(i	==	1)
	{
		return f_read(&fil1, data, j,&br);
	}
	else
	{
		return f_read(&fil2, data, j,&br);
	}
}

FRESULT create_File(char* name,int i)
{
	if(i==1)
	{
		return f_open(&fil1, name,	FA_CREATE_ALWAYS	|	FA_WRITE	|	FA_READ);
	}
	else
	{
		return f_open(&fil2, name,	FA_CREATE_ALWAYS	|	FA_WRITE	|	FA_READ);
	}
}

void gets_File(void* buff,int i)
{
	if(i	==	1)
	{
		f_gets((char*)buff,	100,	&fil1);
		return;
	}
	else
	{
		f_gets((char*)buff,	100,	&fil2);
		return;
	}
}

FRESULT write_File (void *data,int j,int i)
{
	if(i	==	1)
	{
		fresult = f_write(&fil1, data, j,	&br);
		if(fresult	==	FR_OK)
		{
			return f_sync(&fil1);
		}
	}
	else
	{
		fresult = f_write(&fil2, data, j,	&br);
		if(fresult	==	FR_OK)
		{
			return f_sync(&fil2);
		}
	}
	return FR_DISK_ERR;
}

FRESULT set_point_File(DWORD ofs,int i)
{
	if(i	==	1)
	{
		if((f_lseek(&fil1,ofs))	==	FR_OK)
		{
			return	f_sync(&fil1);
		}
	}
	else
	{
		if((f_lseek(&fil2,ofs))	==	FR_OK)
		{
			return	f_sync(&fil2);
		}
	}
	return FR_DISK_ERR;
}

int get_point_File(int i)
{
	if(i	==	1)
	{
		return (&fil1)->fptr;
	}
	else
	{
		return (&fil2)->fptr;
	}
}

FRESULT truncate_File(DWORD ofs,int i)
{
	if(i	==	1)
	{
		return 	f_truncate(&fil1);
	}
	else
	{
		return	f_truncate(&fil2);
	}
}
int get_eof_File(int i)
{
	if(i	==	1)
	{
		int js	=	0;
		while((f_eof(&fil1))	==	0)
		{
			set_point_File((++js),	1);
		}
		return js;
	}	
	else
	{
		int js=0;
		while((f_eof(&fil2))	==	0)
		{
			set_point_File((++js),2);
		}
		return js;
	}
}

void ntptotime(uint32_t epoch)
{
	uint16_t	ntp_hour;
	uint16_t 	ntp_minute;
	uint16_t 	ntp_second;
	uint16_t 	ntp_date;
	uint16_t 	ntp_month;
	uint16_t 	ntp_year;

	unsigned char monthDays[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	//Thu=4, Fri=5, Sat=6, Sun=0, Mon=1, Tue=2, Wed=3
	unsigned char	leap_days;
	unsigned short temp_days;
	unsigned int days_since_epoch, day_of_year; 
   //---------------------------- Input and Calculations -------------------------------------
	leap_days	=	0; 
	// Add or substract time zone here. 
	epoch				+=	16200 ; //GMT +5:30 = +16,200 seconds 
	epoch				-=	2208988800;
	ntp_second	 = 	epoch	%	60;
	epoch 			/= 	60;
	ntp_minute 	 = 	epoch	%	60;
	epoch 			/= 	60;
	ntp_hour 		 = 	epoch	%	24;
	epoch 			/= 	24;
		
	days_since_epoch = epoch;      //number of days since epoch
	
	ntp_year = 1970	+	(days_since_epoch	/	365); // ball parking year, may not be accurate!

	int i;
	for (i	=	1972; i	<	ntp_year; i	+=	4)      // Calculating number of leap days since epoch/1970
	{
		 if((((i%4)	==	0) && ((i%100)	!=	0)) || ((i%400)	==	0))
		 {			 
			 leap_days++;
		 }
	}
					
	ntp_year 		= 1970	+	((days_since_epoch - leap_days)	/	365); // Calculating accurate current year by (days_since_epoch - extra leap days)
	day_of_year = ((days_since_epoch - leap_days)	%	365)	+	1;

 
	if((((ntp_year%4)	==	0) && ((ntp_year%100)	!=	0)) || ((ntp_year%400)	==	0))  
	{
		monthDays[1]	=	29;     //February = 29 days for leap years
	}
	else 
	{
		monthDays[1]	=	28; 		//February = 28 days for non-leap years 
	}
 
	temp_days	=	0;
 
	for (ntp_month=0 ; ntp_month <= 11 ; ntp_month++) //calculating current Month
	{
		 if (day_of_year <= temp_days) 
		 {
			 break; 
		 }
			 
		 temp_days = temp_days + monthDays[ntp_month];
	}
	
	temp_days = temp_days - monthDays[ntp_month-1]; //calculating current Date
	ntp_date = day_of_year - temp_days;
	
	if(ntp_month	>=	9)
	{
		if(temp_days	>=	22)
		{
			ntp_hour--;
		}
	}
	else if(ntp_month	<=	3)
	{
		if(temp_days	<	22)
		{
			ntp_hour--;
		}
	}
	time.Seconds	=	ntp_second;
	time.Minutes	=	ntp_minute;
	time.Hours		=	ntp_hour;
	date.Month		=	ntp_month;
	date.Date			=	ntp_date;
	date.Year			=	ntp_year	-	2000;
	
	HAL_RTC_SetTime(&hrtc,	&time,	RTC_FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc,	&date,	RTC_FORMAT_BIN);
	//uartflush();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART2)
	{
		gOldPos = gNewPos;  // Update the last position before copying new data

		/* If the data in large and it is about to exceed the buffer size, we have to route it to the start of the buffer
		 * This is to maintain the circular buffer
		 * The old data in the main buffer will be overlapped
		 */
		if (gOldPos+Size > MainBuf_SIZE)  // If the current position + new data size is greater than the main buffer
		{
			uint16_t datatocopy = MainBuf_SIZE	-	gOldPos;  // find out how much space is left in the main buffer
			memcpy ((uint8_t *)gMainBuf+gOldPos, gRxBuf, datatocopy);  // copy data in that remaining space

			gOldPos = 0;  // point to the start of the buffer
			memcpy ((uint8_t *)gMainBuf, (uint8_t *)gRxBuf+datatocopy, (Size-datatocopy));  // copy the remaining data
			gNewPos = (Size-datatocopy);  // update the position
		}

		/* if the current position + new data size is less than the main buffer
		 * we will simply copy the data into the buffer and update the position
		 */
		else
		{
			memcpy (((uint8_t *)gMainBuf+gOldPos), gRxBuf, Size);
			gNewPos = Size	+	gOldPos;
		}


		/* start the DMA again */
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t *) gRxBuf,	RxBuf_SIZE);
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
	}
}
int ind(char* a1,	char* a2)
{
	char *found;
	
	found	=	strstr(a1,	a2);
	
	if(found)
	{
		return (int)(found-a1);
	}
	else
	{
		return -1;
	}
 }
void LoraInit()
{
	myLoRa = newLoRa();
	
	myLoRa.hSPIx                 = &hspi1;
	myLoRa.CS_port               = NSS_GPIO_Port;
	myLoRa.CS_pin                = NSS_Pin;
	myLoRa.reset_port            = lorarst_GPIO_Port;
	myLoRa.reset_pin             = lorarst_Pin;
	myLoRa.DIO0_port						 = DIO0_GPIO_Port;
	myLoRa.DIO0_pin							 = DIO0_Pin;
	
	myLoRa.frequency             = 433;							  // default = 433 MHz
	myLoRa.spredingFactor        = SF_7;							// default = SF_7
	myLoRa.bandWidth			       = BW_125KHz;				  // default = BW_125KHz
	myLoRa.crcRate				       = CR_4_5;						// default = CR_4_5
	myLoRa.power					       = POWER_20db;				// default = 20db
	myLoRa.overCurrentProtection = 100; 							// default = 100 mA
	myLoRa.preamble				       = 8;		  					// default = 8;
	
	while(1)
	{
		LoRa_reset(&myLoRa);
		int test	=	LoRa_init(&myLoRa);
		if(test	==	200)
		{
			uart_sendstring_pc("LORA OK\r\n");
			HAL_IWDG_Refresh(&hiwdg);
			break;
		}
		if(test	==	404)
		{
			uart_sendstring_pc("LORA NOT FOUND\r\n");
			HAL_IWDG_Refresh(&hiwdg);
			HAL_Delay(500);
		}
		if(test	==	503)
		{
			uart_sendstring_pc("LORA UNAVAILABLE\r\n");
			HAL_IWDG_Refresh(&hiwdg);
			HAL_Delay(500);
		}
	}
	LoRa_startReceiving(&myLoRa);
}
bool NTPInit(void)
{
	uint8_t Tried	=	0;
	Tried++;
	
	for(int i = Tried ; i<6 ; i++)
	{
		uartflush();
		
		if(i	==	0)
		{
			if((srevercon_udp("0.ir.pool.ntp.org","123"))	==	1)
			{
				break;
			}
		}
		if(i	==	3)
		{
			if((srevercon_udp("0.de.pool.ntp.org","123"))	==	1)
			{
				break;
			}
		}
		if(i	==	1)
		{
			if((srevercon_udp("1.ir.pool.ntp.org","123"))	==	1)
			{
				break;
			}
		}
		if(i	==	4)
		{
			if((srevercon_udp("1.de.pool.ntp.org","123"))	==	1)
			{
				break;
			}
		}
		if(i	==	2)
		{
			if((srevercon_udp("2.ir.pool.ntp.org","123"))	==	1)
			{
				break;
			}
		}
		if(i	==	5)
		{
			if((srevercon_udp("2.de.pool.ntp.org","123"))	==	1)
			{
				break;
			}
		}
	}
	
	uartflush();
	
	for(int i	=	0;	i	<	10;	i++)
	{
		uint8_t ntpreq[49];
		
		ntpreq[0]		=	0x4B;
		ntpreq[48]	=	0;
		
		for(int i	=	1;	i	<	48;	i++)
		{
			ntpreq[i]	=	0x01;
		}
		int	y	=	simsend(ntpreq,	48);
		if(y	==	1)
		{
			break;
		}
	}
	
	uint64_t timerReg;
	
	timerReg = HAL_GetTick();
	
	while((waitfor("SEND OK\r\n"))	!=	1)
	{
		if((HAL_GetTick()	-	timerReg)	>	5000)
		{
			uartflush();
			return false;
		}
	}
	uartflush();
	HAL_Delay(1000);
	timerReg = HAL_GetTick();
	while(1)
	{
		if((HAL_GetTick()	-	timerReg)	>	3000)
		{
			uartflush();
			return false;
		}
		if(gMainBuf[47]	!=	0)
		{
			uint8_t 	ntpreply[4];
			uint32_t 	epo; //RTC & NTP
			
			for(int i=0;	i<4;	i++)
			{
				ntpreply[i]	=	gMainBuf[i+40];
			}
			
			struct_unpack((char*)ntpreply,	"!i",	&epo);
			
			ntptotime(epo);
			
			HAL_RTC_GetTime(&hrtc,	&time,	RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc,	&date,	RTC_FORMAT_BIN);
						
			gSec		=	time.Seconds;
			gMinu		=	time.Minutes;
			gHour		=	time.Hours;
			gDay		=	date.Date;
			gMon		=	date.Month;
			gYear		=	date.Year;
			
			break;
		}
	}
	if((gYear	==	21)	||	(gYear	==	22)	||	(gYear	==	23)	||	(gYear	==	24))
	{
		return true;
	}
	return false;
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin	==	lorairq_Pin)
	{
		gLoraRecFlag	=	true;
	}
}
void MainInit(void)
{
	HAL_IWDG_Refresh(&hiwdg);
	
	lcd_init();
	HAL_Delay(100);
	lcd_init();
	HAL_Delay(100);
	lcd_clear();
	HAL_Delay(100);
	lcd_send_string("    Main APP    ");	
	HAL_Delay(1000);
	
	uart_sendstring_pc("MAIN PROG\n");

	HAL_IWDG_Refresh(&hiwdg);

	if(mount_SD()!=	FR_OK)
	{
		lcd_send_string("No SD Card Found");
		HAL_Delay(2000);
		lcd_clear();
	}	
	
	lcd_send_string("SD Card Found");
	HAL_Delay(2000);
	lcd_clear();

	while((open_File("slaveid.txt",	1))	!=	FR_OK)
	{
		lcd_put_cur(0, 0);
		lcd_send_string(" slaveid.txt NF ");	
		HAL_Delay(100);
	}	
	
	lcd_put_cur(0, 0);
	lcd_send_string("slaveid.tx Found");	
	HAL_Delay(1000);

	char conString[100];
	char slaveId	[3];
	
	lcd_put_cur(0, 0);
	lcd_send_string("Slave Id Reading");	
	HAL_Delay(1000);

	while(1)
	{
		gets_File(conString,	1);
		
		if((strlen(conString))	==	0)
		{
			break;
		}
		slaveId[0]	=	conString[0];
		slaveId[1]	=	conString[1];
		slaveId[2]	=	conString[2];
		dictAddItem(gDict,	slaveId,	1);
	}
	
	close_File(1);
	
	lcd_put_cur(0, 0);
	lcd_send_string("  Reading Done  ");	
	HAL_Delay(1000);

	lcd_clear();
	HAL_Delay(100);
	
	lcd_put_cur(0, 0);
	lcd_send_string("  Sim Starting  ");
	
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t *) gRxBuf, RxBuf_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

	for(int i	=	0;	i	<	1;	i++)
	{
		if((simstart())	==	1)
		{
			gSimFlag	=	true;
			break;
		}
		HAL_Delay(1500);
		lcd_put_cur(1, 0);
		lcd_send_string("     Again      ");
		HAL_Delay(1000);
	}	

	if(gSimFlag	==	true)
	{
		lcd_put_cur(1, 0);
		lcd_send_string("  Sim Started!  ");
		HAL_Delay(1500);
		
		lcd_clear();
		lcd_put_cur(0, 0);
		lcd_send_string(" NTP RT Request ");
		
		int Try	=	2,	Tried	=	0;
		
		
		while((NTPInit())	!=	true)
		{
			if((Tried++)	>=	Try)
			{
				break;
			}
			
		lcd_put_cur(1, 0);
		lcd_send_string("     Again      ");
		HAL_Delay(1000);
		}
	}

	uartflush();

	LoraInit();

	LoRa_startReceiving(&myLoRa);

	HAL_RTC_GetTime(&hrtc,	&time,	RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc,	&date,	RTC_FORMAT_BIN);

	gSec		=	time.Seconds;
	gMinu		=	time.Minutes;
	gHour		=	time.Hours;
	gDay		=	date.Date;
	gMon		=	date.Month;
	gYear		=	date.Year;
	
	char lcdtemp[16];
	sprintf(lcdtemp,	"20%02d - %02d - %02d  ",	gYear,	gMon,	gDay);
	lcd_put_cur(0, 0);
	lcd_send_string(lcdtemp);
	sprintf(lcdtemp,	"  %02d : %02d : %02d  ",	gHour,	gMinu,	gSec);
	lcd_put_cur(1, 0);
	lcd_send_string(lcdtemp);
	HAL_Delay(1500);
	lcd_clear();
	HAL_Delay(200);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_IWDG_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_RTC_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	gDict = dictAlloc();
	
	MainInit();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		if(gLoraRecFlag	==	true)
		{
			LoRa_receive(&myLoRa,	(uint8_t*)gLoraReadStr,	200);
			
			gLoraRecFlag	=	false;
			
			char	loraRecTempSt[4];
			
			loraRecTempSt[0]	=	gLoraReadStr[0];
			loraRecTempSt[1]	=	gLoraReadStr[1];
			loraRecTempSt[2]	=	gLoraReadStr[2];
			loraRecTempSt[3]	=	0;
			
			char lcdtemp[16];
			sprintf(lcdtemp,	"    LoRa:%c%c%c    ",	loraRecTempSt[0],	loraRecTempSt[1],	loraRecTempSt[2]);
			lcd_clear();
			HAL_Delay(50);
			lcd_put_cur(0,0);
			lcd_send_string(lcdtemp);

			if((dictGetItem(*gDict, loraRecTempSt))	==	1)
			{
				lcd_put_cur(1,0);
				lcd_send_string(" Valid Slave Id ");
				HAL_GPIO_TogglePin(led_GPIO_Port,led_Pin);
				LoRa_transmit(&myLoRa,"Ok\n",3,100);
			}
			else
			{
				lcd_put_cur(1,0);
				lcd_send_string("Invalid Slave Id");
				LoRa_transmit(&myLoRa,loraRecTempSt,3,100);
			}
		}
		HAL_IWDG_Refresh(&hiwdg);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV128;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 1;
  DateToUpdate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 7;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
  /* DMA2_Channel4_5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel4_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, led_Pin|NSS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(simrst_GPIO_Port, simrst_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(lorarst_GPIO_Port, lorarst_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 PE4 PE5
                           PE6 PE7 PE8 PE9
                           PE10 PE11 PE12 PE13
                           PE14 PE15 PE0 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PC13 PC1 PC2 PC3
                           PC4 PC6 PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 PF2 PF3
                           PF4 PF5 PF6 PF7
                           PF8 PF9 PF10 PF11
                           PF12 PF13 PF14 PF15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : led_Pin */
  GPIO_InitStruct.Pin = led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA4 PA8 PA9
                           PA10 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_4|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : simrst_Pin */
  GPIO_InitStruct.Pin = simrst_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(simrst_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : NSS_Pin */
  GPIO_InitStruct.Pin = NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB11 PB12 PB13 PB14
                           PB15 PB5 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PG0 PG1 PG2 PG3
                           PG4 PG5 PG6 PG7
                           PG8 PG9 PG10 PG11
                           PG12 PG13 PG14 PG15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD12 PD13 PD14 PD15
                           PD0 PD4 PD6 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_0|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : lorarst_Pin */
  GPIO_InitStruct.Pin = lorarst_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(lorarst_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DIO0_Pin */
  GPIO_InitStruct.Pin = DIO0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(DIO0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : lorairq_Pin */
  GPIO_InitStruct.Pin = lorairq_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(lorairq_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
