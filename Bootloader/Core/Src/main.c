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
#include <Stdint.h>

#include "sim.h"
#include "lcd.h"
#include "zcrypt.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
FATFS fs;  // Fatfs
FIL fil1; 
FIL fil2; 
FILINFO fno;
FRESULT fresult;
UINT b;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg;

SD_HandleTypeDef hsd;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
static FLASH_EraseInitTypeDef EraseInitStruct;	//flash working
uint64_t gAddress	=	0x0800C800;
	
uint8_t gTxBuf		[TxBuf_SIZE];	//USART-SIM
uint8_t gRxBuf		[RxBuf_SIZE];
uint8_t gMainBuf	[MainBuf_SIZE];

uint16_t gOldPos;
uint16_t gNewPos;

char gName[4]={0,0,0,0};	//version name

static uint32_t gTimerError	=	0; //

static uint8_t gCounterOk		=	0;
static uint8_t gCounterBad	=	0;


static uint16_t gLinesCounter	=	0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_IWDG_Init(void);
static void MX_DMA_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_USART2_UART_Init(void);
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
	if(i==1)
	{
		return f_open(&fil1,name,FA_WRITE|FA_READ|FA_OPEN_EXISTING);
	}
	else
	{
		return f_open(&fil2,name,FA_WRITE|FA_READ|FA_OPEN_EXISTING);
	}
}


FRESULT close_File(int i)
{
	if(i==1)
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
	if(i==1)
	{
		return f_read(&fil1, data, j,&b);
	}
	else
	{
		return f_read(&fil2, data, j,&b);
	}
}

FRESULT create_File(char* name,int i)
{
	if(i==1)
	{
		return f_open(&fil1, name,FA_CREATE_ALWAYS|FA_WRITE|FA_READ);
	}
	else
	{
		return f_open(&fil2, name,FA_CREATE_ALWAYS|FA_WRITE|FA_READ);
	}
}

void gets_File(void* buff,int i)
{
	if(i==1)
	{
		f_gets((char*)buff,100,&fil1);
		return;
	}
	if(i==2)
	{
		f_gets((char*)buff,100,&fil2);
		return;
	}
}

FRESULT write_File (void *data,int j,int i)
{
	if(i==1)
	{
		fresult = f_write(&fil1, data, j, &b);
		if(fresult==FR_OK)
		{
			
			return f_sync(&fil1);
		}
		else 
		{
			return fresult;
		}
	}
	else
	{
		fresult = f_write(&fil2, data, j, &b);
		if(fresult==FR_OK)
		{
			
			return f_sync(&fil2);
		}
		else 
		{
			return fresult;
		}
	}
}

FRESULT set_point_File(DWORD ofs,int i)
{
	if(i==1)
	{
		return f_lseek(&fil1,ofs);
	}
	else
	{
		return f_lseek(&fil2,ofs);
	}
}

int get_point_File(int i)
{
	if(i==1)
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
	if(i==1)
	{
		return f_truncate(&fil1);
	}
	else
	{
		return f_truncate(&fil2);
	}
}
int get_eof_File(int i)
{
	if(i==1)
	{
		int po=get_point_File(1);
		int js=po;
		while(f_eof(&fil1)==0)
		{
			set_point_File(++js,1);
		}
		set_point_File(po,1);
		return js;
	}	
	else
	{
		int po=get_point_File(2);
		int js=po;
		while(f_eof(&fil2)==0)
		{
			set_point_File(++js,2);
		}
		set_point_File(po,2);
		return js;
	}
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
			uint16_t datatocopy = MainBuf_SIZE-gOldPos;  // find out how much space is left in the main buffer
			memcpy ((uint8_t *)gMainBuf+gOldPos,gRxBuf, datatocopy);  // copy data in that remaining space

			gOldPos = 0;  // point to the start of the buffer
			memcpy ((uint8_t *)gMainBuf, (uint8_t *)gRxBuf+datatocopy, (Size-datatocopy));  // copy the remaining data
			gNewPos = (Size-datatocopy);  // update the position
		}

		/* if the current position + new data size is less than the main buffer
		 * we will simply copy the data into the buffer and update the position
		 */
		else
		{
			memcpy ((uint8_t *)gMainBuf+gOldPos,gRxBuf, Size);
			gNewPos = Size+gOldPos;
		}


		/* start the DMA again */
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2,(uint8_t *)gRxBuf, RxBuf_SIZE);
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
	}
}

void flash_lock(void)
{
	FLASH_OBProgramInitTypeDef OptionsBytesStruct;
  HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);
  FLASH_OBProgramInitTypeDef FLASH_RDP;
	HAL_FLASH_Unlock();

	HAL_FLASH_OB_Unlock();
	if(OptionsBytesStruct.RDPLevel == OB_RDP_LEVEL1)
	{
		goto aa;
	}
	
	FLASH_RDP.RDPLevel = OB_RDP_LEVEL1;

	FLASH_RDP.OptionType = OPTIONBYTE_RDP;

	HAL_FLASHEx_OBProgram(&FLASH_RDP); 
	
	HAL_FLASH_OB_Launch();
	aa:
	HAL_FLASH_OB_Lock();
	
	HAL_FLASH_Lock();
}

void flash_update(char* st)
{
	char a0[2]={0},a1[2]={0},a2[50]={0};
	int len=0;
	uint64_t tem=0x00;
	
	a0[0]=st[1];//data len
	a0[1]=st[2];
	a1[0]=st[7];//data type
	a1[1]=st[8];

	for(int i=0;i<32;i++)
	{
		a2[i]=st[i+9];//data
	}

	len=0;
	
	if(a0[0]>='0'&&a0[0]<='9')
	{
		len+=16*16*16*(a0[0]-'0');
	}
	else if(a0[0]>='A'&&a0[0]<='F')
	{
		len+=16*16*16*(10+a0[0]-'A');
	}

	if(a0[0]>='0'&&a0[0]<='9')
	{
		len+=16*16*16*(a0[0]-'0');
	}
	else if(a0[0]>='A'&&a0[0]<='F')
	{
		len+=16*16*16*(10+a0[0]-'A');
	}
					
	for(int q=0;q<8;q++)
	{
		if((strstr(a1,"00")!=NULL) && (len>0))
		{
			tem=0x00;
			
			if(a2[4*q+3]>='0'&&a2[4*q+3]<='9')
			{
				tem+=16*16*(a2[4*q+3]-'0');
			}
			else if(a2[4*q+3]>='A'&&a2[4*q+3]<='F')
			{
				tem+=16*16*(10+a2[4*q+3]-'A');
			}
			
			if(a2[4*q+2]>='0'&&a2[4*q+2]<='9')
			{
				tem+=16*16*16*(a2[4*q+2]-'0');
			}
			else if(a2[4*q+2]>='A'&&a2[4*q+2]<='F')
			{
				tem+=16*16*16*(10+a2[4*q+2]-'A');
			}
			
			if(a2[4*q+1]>='0'&&a2[4*q+1]<='9')
			{
				tem+=(a2[4*q+1]-'0');
			}
			else if(a2[4*q+1]>='A'&&a2[4*q+1]<='F')
			{
				tem+=(10+a2[4*q+1]-'A');
			}
			
			if(a2[4*q]>='0'&&a2[4*q]<='9')
			{
				tem+=16*(a2[4*q]-'0');
			}
			else if(a2[4*q]>='A'&&a2[4*q]<='F')
			{
				tem+=16*(10+a2[4*q]-'A');
			}
			
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,gAddress,(uint64_t)tem);
			gAddress+=2;
			HAL_IWDG_Refresh(&hiwdg);
		}
		else
		{
			break;
		}
	}
}
void decrypt(char *data1,char *data2,int j)
{

	#define padlen 4
	uint8_t data[100]={0};
	memcpy(data, data1, 100); /* to not mess with original data, this will be sent to encrypt function */
	uint8_t iv[AES_IV_SIZE] = "hellofucker1065o";               /* or any random filled buffer */
	uint8_t key[AES_KEYLEN] = "hellofucker1065o";      /* fill with your key */

    uint8_t buff[100] = {0};          /* large enough to be filled with data + padding (maximum 16 bytes) */
    memcpy(buff, data, sizeof(data)); /* to not mess with original data, this will be sent to encrypt function */
    AES_ctx_t ctx;
    AES_init_ctx_iv(&ctx, key, iv); /* every time using a encrypt or decrypt, creadfromsd must be initialized */
    AES_CBC_decrypt_buffer(&ctx, buff, sizeof(data) + padlen);
		if(j==1||j==2)
		{
			printf("After Decrypt : ");
		}
    for (size_t i = 0; i < sizeof(data) + padlen; ++i)
    {
			if(j==1)
			{
				printf("[0x%02X]", buff[i]);
			}
			else if(j==2)
			{
				printf("%c", buff[i]);
			}
			else
			{
				break;	
			}
    }
		if(j==1||j==2)
		{
			printf("\r\n");
		}

//    int32_t data_len = pkcs7_data_length(buff, sizeof(data) + padlen, AES_BLOCKLEN);
		int data_len=44;
		
    if (data_len > 0)
    {
        uint8_t original[100] = {0}; /* large enough */
        memcpy(original, buff, data_len);
				if(j==1||j==2)
				{
					printf("Original Data : ");
				}
        for (size_t i = 0; i < data_len; ++i)
        {
					if(j==1)
					{
						printf("[0x%02X]", original[i]);
					}
					else if(j==2)
					{
						printf("%c", original[i]);
					}
					else
					{
						break;	
					}
        }
				if(j==1||j==2)
				{
					printf("\r\n");
				}
				strcpy(data2,(char*)original);
    }
    else
    {
			if(j==1||j==2)
			{
				printf("received buffer is not correctly padded.\r\n");
			}
    }
}


void deinitEverything(void)
{
	//-- reset peripherals to guarantee flawless start of user application
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	HAL_NVIC_DisableIRQ(SysTick_IRQn); 
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	HAL_I2C_DeInit(&hi2c1);
	HAL_UART_DeInit(&huart2);
	HAL_SD_DeInit(&hsd);
	HAL_DMA_DeInit(&hdma_usart2_rx);
	HAL_DMA_DeInit(&hdma_usart2_tx);
	HAL_RCC_DeInit();
	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
}

void jump_to(const uint32_t addr)
{
	deinitEverything();
	void (*main_application)(void);
	uint32_t msp = *(volatile uint32_t*)addr;
	main_application = (void*)(*(volatile uint32_t*)(addr + 4));
	__set_MSP(msp);
	main_application();
}

int ind(char* a1,char* a2)
{
	char *found;
	found=strstr(a1,a2);
	if(found)
	{
		return (int)(found-a1);
	}
	else
	{
		return -1;
	}
 }

 int check(uint8_t* st)
{
	char Var=0, Checksum=0,Comp=0;
	char tm1[40]={0};	//data without checksum
	char tm2[2]={0};	//checksum
	char tm3[2]={0};	//checksum sum

	for(int p=0;p<40;p++)
	{
		tm1[p]=st[p+1];
	}
	tm2[0]=st[41];
	tm2[1]=st[42];
	if(tm2[1]>='0'&&tm2[1]<='9')
	{
		if((tm2[0]>='0'&&tm2[0]<='9'))
		{
			Var=tm2[1]-'0';
			Var+=16*(tm2[0]-'0');
		}
		else if(tm2[0]>='A'&&tm2[0]<='F')
		{
			Var=tm2[1]-'0';
			Var+=16*(tm2[0]-'A'+10);
		}
		else
		{
			return -1;
		}
	}
	else if(tm2[1]>='A'&&tm2[1]<='F')
	{
		if((tm2[0]>='0'&&tm2[0]<='9'))
		{
			Var=tm2[1]-'A'+10;
			Var+=16*(tm2[0]-'0');
		}
		else if(tm2[0]>='A'&&tm2[0]<='F')
		{
			Var=tm2[1]-'A'+10;
			Var+=16*(tm2[0]-'A'+10);
		}
		else
		{
			return -1;
		}
	}
	for(int p=0;p<20;p++)
	{
		tm3[0]=tm1[2*p];
		tm3[1]=tm1[2*p+1];
		if(tm3[1]>='0'&&tm3[1]<='9')
		{
			if((tm3[0]>='0'&&tm3[0]<='9'))
			{
				Checksum+=tm3[1]-'0';
				Checksum+=16*(tm3[0]-'0');
			}
			else if(tm3[0]>='A'&&tm3[0]<='F')
			{
				Checksum+=tm3[1]-'0';
				Checksum+=16*(tm3[0]-'A'+10);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			if((tm3[0]>='0'&tm3[0]<='9'))
			{
				Checksum+=tm3[1]-'A'+10;
				Checksum+=16*(tm3[0]-'0');
			}
			else if(tm3[0]>='A'&&tm3[0]<='F')
			{
				Checksum+=tm3[1]-'A'+10;
				Checksum+=16*(tm3[0]-'A'+10);
			}
			else
			{
				return -1;
			}
		}
	}
	Comp=-Var;
	if(Checksum==Comp)
	{
		return 1;
	}
	else
	{
		return -1;
	}
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
  MX_IWDG_Init();
  MX_DMA_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	flash_lock();

	uart_sendstring_pc("BOOT PROG\r\n");
		
	lcd_init();
	lcd_init();
	HAL_Delay(100);
	lcd_send_string("BootLoader");
	HAL_Delay(500);	
	lcd_clear();
	HAL_Delay(100);	
	lcd_send_string("BootLoader");
	HAL_Delay(500);
	lcd_clear();
	
	HAL_FLASH_Unlock();
	
	char ve[4]={0};
	
	ve[0]	=*	(uint16_t*)(0x0800c400);
	ve[1]	=*	(uint16_t*)(0x0800c402);
	ve[2]	=*	(uint16_t*)(0x0800c404);
	ve[3]	=*	(uint16_t*)(0x0800c406);
	HAL_FLASH_Lock();
	
	
	if(mount_SD()!=	FR_OK)
	{
		lcd_send_string("No SD Card Found");
		HAL_Delay(2000);
		lcd_clear();
		NVIC_SystemReset();
	}	
	
	lcd_send_string("SD Card Found");
	HAL_Delay(2000);
	lcd_clear();
	
	if(HAL_GPIO_ReadPin(progmode_GPIO_Port,progmode_Pin)	==	GPIO_PIN_SET)
	{
		lcd_send_string("Auto Mode:      ");
		
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, (uint8_t *) gRxBuf, RxBuf_SIZE);
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
		
		fresult	=	open_File("updatea.txt",	1);
		if(fresult	!=	FR_OK)
		{
			fresult=create_File("updatea.txt",	1);
			if(fresult	!=	FR_OK)
			{
				HAL_Delay(100);
				NVIC_SystemReset();
			}
		}

		lcd_put_cur(1, 0);
		lcd_send_string("Sim Starting    ");
		HAL_Delay(1000);
		
		for(int i	=	0	,	y	=	0;	i	<5	;	i++)
		{
			y	=	simstart();
			if(y	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("Sim Started!    ");
				HAL_Delay(1000);
				break;
			}
			
			HAL_Delay(500);
			uartflush();
			
			if(i	==	4)
			{
				if(y	!=	1)
				{
					lcd_put_cur(1, 0);
					lcd_send_string("Going To Func!  ");
					HAL_Delay(1000);
					close_File(1);
					goto Func;
				}
			}
		}
		
		lcd_put_cur(1, 0);
		lcd_send_string("U Server Conning");
		HAL_Delay(1000);
		
		for(int i	=	0	,	y	=	0;	i	<	6;	i++)
		{
			if(i	<	3)
			{
				y	=	srevercon("turk.expressvpn.monster",	"2841");
			}
			else
			{
				y	=	srevercon("213.239.213.207",	"2841");
				//y=srevercon("212.64.214.25",	"2841");
			}

			if(y	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("U Server Conned!");
				HAL_Delay(1000);
				break;
			}
			
			HAL_Delay(1000);
			uartflush();
			if(i	==	5)
			{
				if(y	!=	1)
				{
					lcd_put_cur(1, 0);
					lcd_send_string("U Server Problem");
					HAL_Delay(1000);
					lcd_put_cur(1, 0);
					lcd_send_string("Going To Func!  ");
					HAL_Delay(1000);
					close_File(1);
					goto Func;
				}
			}
		}
		
		gTimerError	=	HAL_GetTick();
		
		while(1)
		{
			if(waitfor("send master id")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("send master id  ");
				
				uartflush();
				simsend("master id",0);
				while(strstr((char*)gMainBuf,"SEND OK")	==	NULL);
				gTimerError	=	HAL_GetTick();
			}	
			
			if(waitfor("send atentication information")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("send atenti info");
				
				uartflush();
				simsend("username:user , password:pass",	0);
				
				while(strstr((char*)gMainBuf,"SEND OK")	==	NULL);
				
				gTimerError	=	HAL_GetTick();
			}		
			
			if(waitfor("software version?")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("softwareversion?");
				
				uartflush();
				char tmep[3];
				sprintf(tmep,	"%c%c%c",	ve[0]+'0',	ve[1]+'0',	ve[2]+'0');
				if(ve[3]	==	1)
				{
					simsend(tmep,0);
					while(strstr((char*)gMainBuf,"SEND OK")	==	NULL);
				}
				else
				{
					simsend("no version",0);
					while(strstr((char*)gMainBuf,"SEND OK")	==	NULL);
				}
				gTimerError	=	HAL_GetTick();
			}
			
			if((HAL_GetTick()-gTimerError)	>	10000)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("Timeout 1       ");
				HAL_Delay(2000);
				lcd_put_cur(1, 0);
				lcd_send_string("Going To Func!  ");
				HAL_Delay(1000);
				close_File(1);
				goto Func;
			}
			
			if(waitfor("dont need update")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("Dont Need Update");
				HAL_Delay(2000);
				goto ass2;
			}	
			
			if(waitfor("CLOSED")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("TCP has Closed 1");
				HAL_Delay(2000);
				lcd_put_cur(1, 0);
				lcd_send_string("Going To Func!  ");
				HAL_Delay(1000);
				close_File(1);
				goto Func;
			}
			
			if(waitfor("version=")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("softwareversion=");
				
				int lx	=	ind((char*)gMainBuf,"version=")+7;

				gName[0]	=	gMainBuf[lx+1];
				gName[1]	=	gMainBuf[lx+2];
				gName[2]	=	gMainBuf[lx+3];

				truncate_File	(0,	1);
				set_point_File(0,	1);
				
				write_File(gName,	3,	1);
				write_File("0",		1,	1);
				write_File("\n",	1,	1);
				
				uartflush();
				simsend("data recived sucsessfully",0);
				while(strstr((char*)gMainBuf,"SEND OK")==NULL);
				
				break;
			}
		}
				
		gTimerError	=	HAL_GetTick();
		
		while(1)
		{
			HAL_Delay(100);
			if(waitfor("CLOSED")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("TCP has Closed 2");
				HAL_Delay(1000);
				lcd_put_cur(1, 0);
				lcd_send_string("Going To Func!  ");
				
				close_File(1);
				goto Func;
			}
			
//			if(HAL_GetTick()-gTimerError>5000)
//			{
//				uartflush();
//				while(strstr((char*)gMainBuf,"Send OK")==NULL)
//				{
//					simsend("no data",0);
//					HAL_Delay(3000);
//				}
//				
//				uartflush();
//				gTimerError=HAL_GetTick();
//			}

			if(waitfor("sending all data sucsessfully")	==	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("Recieve Is Done ");
				HAL_Delay(2000);
				uartflush();
				break;
			}
			
			if((strstr((char*)gMainBuf,"begin")	!=	NULL)	&&	(strstr((char*)gMainBuf,"end")	!=	NULL))
			{
				int flag	=	0;
				
				char tmp2	[MainBuf_SIZE];	//update server
				char tmp3	[200];	//hex line
				char tmp4	[200];	//decrypted hex line
				
				int in	=	ind((char*)gMainBuf,"begin")+5;
				
				for(int l	=	in;	l	<=	strlen((char*)gMainBuf);	l++)
				{
					tmp2[l-in]	=	(char)gMainBuf[l];
				}
				
				while(1)
				{
					static uint16_t l		=	0;
					uint16_t 				len	=	0;
					
					memset(tmp3,	0,	200);
					memset(tmp4,	0,	200);
					
					while(1)
					{
						tmp3[len++]	=	tmp2[l++];
						
						if(strstr(tmp3,	"next\n")	!=	NULL)
						{
							break;
						}
						
						if(strstr(tmp3,	"end")	!=	NULL)
						{
							l	=	0;
							break;
						}
					}
					
					if(strstr(tmp3,	"end")	!=	NULL)
					{
						l	=	0;
						break;
					}	
					if(strstr(tmp3,	"NULL")	==	NULL)
					{
						char tmp5[30];
						memset(tmp5,	0,	30);
						
						int len2	=	strlen(tmp3)	-	5;
						
						for(int l1	=	52;	l1	<	len;	l1++)
						{
							tmp5[l1-52]	=	tmp3[l1];
						}
						
						for(int l2	=	0;	l2	<	strlen(tmp5);	l2++)
						{
							int y	=	0;
							
							if((tmp5[4*l2+2]>='0')	&&	(tmp5[4*l2+2]<='9')	&&	(tmp5[4*l2+1]>='0')	&&	(tmp5[4*l2+1]<='9'))
							{
								
								y	=				tmp5[4*l2+2]-'0';
								y	+=	10*(tmp5[4*l2+1]-'0');

								if((uint8_t)tmp3[y]	==	0xFF)
								{
									tmp3[y]	=	0; 
								}
							}
						}
					}
					
					decrypt(tmp3,	tmp4,	-1);
					
					if(check((uint8_t*)tmp4)	!=	1)
					{
						l	=	0;
						flag	=	-1;
						break;
					}
				}
				
				if(flag	==	0)
				{
					int e	=	strlen((char*)tmp2)	-	3;
					int w	=	get_point_File(1);
					
					while(get_point_File(1)	-	w	<	e)
					{
						fresult = set_point_File(w,	1);
						
						fresult = write_File(tmp2,	e,	1);
						
						HAL_Delay(100);
					}
					
					uartflush();
					simsend("data recived sucsessfully",0);
					
					while(strstr((char*)gMainBuf,"SEND OK")	==	NULL);
					
					char lcdtmp[16];
					sprintf(lcdtmp,	" Good:%02d  Bad:%01d ",	++gCounterOk,	gCounterBad);
					
					lcd_put_cur(1, 0);
					lcd_send_string(lcdtmp);
				}
				else if(flag	==	-1)
				{
					uartflush();
					simsend("data recived bad",0);
					while(strstr((char*)gMainBuf,"SEND OK")	==	NULL);
					
					char lcdtmp[16];
					sprintf(lcdtmp,	" Good:%02d  Bad:%01d ",	gCounterOk,	++gCounterBad);
					lcd_put_cur(1, 0);
					lcd_send_string(lcdtmp);
				}
				gTimerError	=	HAL_GetTick();
			}
		}
		
		lcd_put_cur(1, 0);
		lcd_send_string("Writing to Flash");		
		
		set_point_File(0,	1);
		
		write_File(gName,	3,	1);
		write_File("1"	,	1,	1);
		write_File("\n"	,	1,	1);
		
		automatic:		
		gLinesCounter	=	0;
		
		gTimerError	=	HAL_GetTick();
		
		HAL_FLASH_Unlock();
				
		EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = 0x0800C400;
		EraseInitStruct.NbPages     = 1;
		uint32_t PAGEError;
		HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
		
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C400,	(uint64_t)gName[0]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C402,	(uint64_t)gName[1]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C404,	(uint64_t)gName[2]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C406,	(uint64_t)0);
		
		EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = gAddress;
		EraseInitStruct.NbPages     = 100;
		HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
		
		char	readfromsd			[200];	//decrypt
		char 	writetoflash		[200];
		char 	writetoflash2		[200];
		char 	linefromsd			[200];
		char 	tmp							[30];

		while(1)
		{
			HAL_IWDG_Refresh(&hiwdg);
			
			memset(writetoflash	,	0,	200);
			memset(writetoflash2,	0,	200);
			memset(linefromsd		,	0,	200);	
			memset(tmp					,	0,	30);
			
			while(1)
			{
				memset(readfromsd,	0,	200);
				gTimerError	=	HAL_GetTick();
				while(1)
				{
					char u[2]	=	{0,	0};
					read_File(u,	1,	1);
					strcat(readfromsd,	u);
					
					if((u[0]==0x0A)	||	(u[0]==0x00))
					{
						break;
					}
					
					if(HAL_GetTick()	-	gTimerError	>	100)
					{
						goto ass;
					}
				}
				if(strlen(readfromsd)	==	NULL)
				{
					break;
				}
				
				strcat(linefromsd,	readfromsd);

				if(strstr(linefromsd,"next\n")	!=	NULL)
				{
						break;
				}					
			}		
			
			if(strlen(linefromsd)	<	40)
			{
				goto ass;
			}
			
			if(strstr(linefromsd,"NULL")	==	NULL)
			{
				int len	=	strlen(linefromsd)	-	5;
				
				for(int l	=	52;	l	<	len;	l++)
				{
					tmp[l-52]	=	linefromsd[l];
				}
				
				for(int l	=	0;	l	<	strlen(tmp);	l++)
				{
					int y	=	0;
					
					if(tmp[4*l+2]>='0'	&&	tmp[4*l+2]<='9'	&&	tmp[4*l+1]>='0'	&&	tmp[4*l+1]<='9')
					{
						y	=		tmp[4*l+2]-'0';
						y	+=	10*(tmp[4*l+1]-'0');
						
						if((uint8_t)linefromsd[y]	==	0xFF)
						{
							linefromsd[y]	=	0;
						}
					}
				}
			}
			
			decrypt(linefromsd,	writetoflash,	0);	
			
			if(check((uint8_t*)writetoflash)	!=	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("Error in File  !");	
				printf("error in updatea\n");
				goto manual;
			}
			
			flash_update(writetoflash);
			
			gLinesCounter++;
		}
		ass:
		EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = 0x0800C400;
		EraseInitStruct.NbPages     = 1;
		HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
		
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C400,	(uint64_t)gName[0]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C402,	(uint64_t)gName[1]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C404,	(uint64_t)gName[2]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C406,	(uint64_t)1);
		HAL_FLASH_Lock();
		close_File(1);
		ass2:
		unmount_SD();
		HAL_IWDG_Refresh(&hiwdg);
				
		lcd_put_cur(1, 0);
		lcd_send_string("Flash Write done");
		HAL_Delay(2000);
		lcd_put_cur(1, 0);
		lcd_send_string("Jumping To Flash");	
		HAL_Delay(1000);	
		
		jump_to(0x0800C800);
	}
	
	else if(HAL_GPIO_ReadPin(progmode_GPIO_Port,progmode_Pin)	==	GPIO_PIN_RESET)
	{
		lcd_put_cur(0, 0);
		lcd_send_string("Manual Mode:    ");	
		

		manual:
	
		gLinesCounter=0;
		while(open_File("updatem.txt",1)	!=	FR_OK)
		{
			lcd_put_cur(1, 0);
			lcd_send_string("No updatem.txt !");	
			HAL_Delay(10);
			HAL_NVIC_SystemReset();
		}
					
		lcd_put_cur(1, 0);
		lcd_send_string("File Found      ");	
		
		gets_File(gName,1);
		
		if(gName[3]	!=	'1')
		{
			lcd_put_cur(1, 0);
			lcd_send_string("File is Not OK !");	
			HAL_Delay(2000);
			lcd_put_cur(1, 0);
			lcd_send_string("Going To Func!  ");	
			goto Func;
		}
		
		int ver1	=	0;
		int ver2	=	0;
		
		if(*(uint16_t*)(0x0800C406)	==	1)
		{
			ver1	+=	100*(*(uint16_t*)(0x0800C400));
			ver1	+=	10*	(*(uint16_t*)(0x0800C402));
			ver1	+=			 *(uint16_t*)(0x0800C404);
		}

		ver2	+=	100*(gName[0]-'0');
		ver2	+=	10*	(gName[1]-'0');
		ver2	+=			(gName[2]-'0');
		
		if(ver1	==	ver2)
		{
			lcd_put_cur(0, 0);
			lcd_send_string("Jumping On Flash");
			goto ass2;
		}
		
		lcd_put_cur(1, 0);
		lcd_send_string("File is OK !    ");
		HAL_Delay(2000);
		lcd_put_cur(1, 0);
		lcd_send_string("Writing To Flash");	
		HAL_IWDG_Refresh(&hiwdg);

		HAL_FLASH_Unlock();
		EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = 0x0800C400;
		EraseInitStruct.NbPages     = 1;
		uint32_t PAGEError;
		HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
		
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C400,	(uint64_t)gName[0]);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C402,	(uint64_t)gName[1]);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C404,	(uint64_t)gName[2]);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C406,	(uint64_t)0);
		
		
		EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = gAddress;
		EraseInitStruct.NbPages     = 100;
		HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
		
		char	readfromsd			[200];	//decrypt
		char 	writetoflash		[200];
		char 	writetoflash2		[200];
		char 	linefromsd			[200];
		char 	tmp							[30];	 
		
		while(1)
		{
			HAL_IWDG_Refresh(&hiwdg);
			
			memset(writetoflash,	0,	200);
			memset(tmp,						0,	30);
			memset(writetoflash2,	0,	200);
			memset(linefromsd,		0,		200);		
				
			while(1)
			{
				memset(readfromsd,0,200);
				
				gTimerError	=	HAL_GetTick();
				
				while(1)
				{
					char u[2]	=	{0,	0};
					
					read_File(u,	1,	1);
					strcat(readfromsd,	u);
					
					if((u[0]	==	0x0A)	||	(u[0]	==	0x00))
					{
						break;
					}
					
					if((HAL_GetTick()-gTimerError)	>	100)
					{
						goto as;
					}
				}
				
				if(strlen(readfromsd)	==	NULL)
				{
					break;
				}
				
				strcat(linefromsd,readfromsd);

				if(strstr(linefromsd,"next")	!=	NULL)
				{
	
						break;
				}					
			}
			
			if(strlen(linefromsd)	<	50)
			{
				HAL_Delay(100);
				goto as;
			}
			
			if(strstr(linefromsd,	"NULL")	==	NULL)
			{
				int len	=	strlen(linefromsd)	-	5;
				
				for(int l	=	52;	l	<	len;	l++)
				{
					tmp[l-52]	=	linefromsd[l];
				}
				for(int l	=	0;	l	<	strlen(tmp);	l++)
				{
					int y=0;
					if((tmp[4*l+2]>='0')	&&	(tmp[4*l+2]<='9')	&&	(tmp[4*l+1]>='0')	&&	(tmp[4*l+1]<='9'))
					{
						y	=		tmp[4*l+2]-'0';
						y	+=	10	*	(tmp[4*l+1]-'0');
						
						if((uint8_t)linefromsd[y]	==	0xFF)
						{
							linefromsd[y]=0;
						}
					}
				}
			}
			
			decrypt(linefromsd,writetoflash,0);
			
			if(check((uint8_t*)writetoflash)	!=	1)
			{
				lcd_put_cur(1, 0);
				lcd_send_string("Error in File  !");
				printf("error in updatem\n");
				goto automatic;
			}
			
			flash_update(writetoflash);
			gLinesCounter++;
		}

		
		as:
		EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
		EraseInitStruct.PageAddress = 0x0800C400;
		EraseInitStruct.NbPages     = 1;
		HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
		
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C400,	(uint64_t)gName[0]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C402,	(uint64_t)gName[1]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C404,	(uint64_t)gName[2]-'0');
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,	0x0800C406,	(uint64_t)1);
		HAL_FLASH_Lock();
		close_File(1);
		unmount_SD();
		HAL_IWDG_Refresh(&hiwdg);
		
		lcd_put_cur(1, 0);
		lcd_send_string("Flash Write done");
		HAL_Delay(2000);
		lcd_put_cur(1, 0);
		lcd_send_string("Jumping To Flash");	
		HAL_Delay(1000);

		jump_to(0x0800C800);
	}
	
	Func:
	
	lcd_put_cur(0, 0);
	lcd_send_string("Function Part   ");
	
	HAL_FLASH_Unlock();
	
	int y	=	*(uint16_t*)(0x0800C000);
	
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = 0x0800C000;
	EraseInitStruct.NbPages     = 1;
	uint32_t PAGEError;
	HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
	if(y	==	65535)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,0x0800C000,(uint64_t)0);
	}
	else if(y	!=	65535)
	{
		y++;
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,0x0800C000,(uint64_t)y);
	}		
	HAL_FLASH_Lock();
	
	if(y<5)
	{
		HAL_NVIC_SystemReset();
	}
	else if(y	>=	5)
	{
		int 	ver1	=	0;
		int	 	ver2	=	0;
		int 	ver3	=	0;
		
		char 	fileVer1[100];
		char 	fileVer2[100];
			
		open_File("updatea.txt",1);
		gets_File(fileVer1,1);
		
		open_File("updatem.txt",2);
		gets_File(fileVer2,2);

		if(*(uint16_t*)(0x0800C406)	==	1)
		{
			ver1	+=	100*(*(uint16_t*)(0x0800C400));
			ver1	+=	10*	(*(uint16_t*)(0x0800C402));
			ver1	+=			 *(uint16_t*)(0x0800C404);
		}
		
		if(fileVer1[3]==1	||	fileVer1[3]=='1')
		{
			ver2	+=	100*(fileVer1[0]-'0');
			ver2	+=	10*	(fileVer1[1]-'0');
			ver2	+=			(fileVer1[2]-'0');
		}	
		
		if(fileVer2[3]==1	||	fileVer2[3]=='1')
		{
			ver3	+=	100*(fileVer2[0]-'0');
			ver3	+=	10*	(fileVer2[1]-'0');
			ver3	+=			(fileVer2[2]-'0');
		}
		
		if(ver1	>	ver2)
		{
			if(ver1	>=	ver3)
			{
				close_File(1);
				close_File(2);
				lcd_put_cur(0, 0);
				lcd_send_string("Jumping On Flash");
				goto ass2;
			}
			else if(ver1	<	ver3)
			{
				close_File(1);
				close_File(2);
				
				lcd_put_cur(0, 0);
				lcd_send_string("Func -> Manual  ");
				
				goto manual;
			}
		}
		else if(ver1	<=	ver2)
		{
			if(ver2	>=	ver3)
			{
				close_File(2);
				gName[0]	=	ver2	/	100;
				gName[1]	=	ver2	/	10;
				gName[2]	=	ver2	%	10;
				
				lcd_put_cur(0, 0);
				lcd_send_string("Func -> Auto    ");
				
				goto automatic;
			}
			else if(ver2	<	ver3)
			{
				close_File(1);
				close_File(2);
				
				lcd_put_cur(0, 0);
				lcd_send_string("Func -> Manual  ");
				
				goto manual;			
			}
		}
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		HAL_IWDG_Refresh(&hiwdg);
		HAL_GPIO_TogglePin(led_GPIO_Port,led_Pin);
		HAL_Delay(60);		
		HAL_GPIO_TogglePin(led_GPIO_Port,led_Pin);
		HAL_Delay(1000);		
		HAL_GPIO_TogglePin(led_GPIO_Port,led_Pin);
		HAL_Delay(500);		
		HAL_GPIO_TogglePin(led_GPIO_Port,led_Pin);
		HAL_Delay(800);		
		HAL_GPIO_TogglePin(led_GPIO_Port,led_Pin);
		HAL_Delay(100);		
		HAL_GPIO_TogglePin(led_GPIO_Port,led_Pin);
		HAL_Delay(50);
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

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

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
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(simrst_GPIO_Port, simrst_Pin, GPIO_PIN_RESET);

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

  /*Configure GPIO pins : PC13 PC14 PC15 PC1
                           PC2 PC3 PC4 PC5
                           PC6 PC7 PC9 PC10
                           PC11 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_1
                          |GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11;
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

  /*Configure GPIO pins : PA0 PA4 PA5 PA6
                           PA7 PA8 PA9 PA10
                           PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : simrst_Pin */
  GPIO_InitStruct.Pin = simrst_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(simrst_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB12 PB13 PB14 PB15
                           PB5 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
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

  /*Configure GPIO pin : progmode_Pin */
  GPIO_InitStruct.Pin = progmode_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(progmode_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD12 PD13 PD14 PD15
                           PD0 PD1 PD3 PD4
                           PD5 PD6 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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
