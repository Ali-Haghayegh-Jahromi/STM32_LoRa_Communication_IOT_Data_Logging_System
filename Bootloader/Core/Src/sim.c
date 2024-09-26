/*
 * sim.c
 */


#include "sim.h"

static uint32_t timeOut;

void simreset(void)
{
	HAL_IWDG_Refresh(&hiwdg);
	HAL_GPIO_WritePin(reset_GPIO_Port,	reset_Pin,	GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(reset_GPIO_Port,	reset_Pin,	GPIO_PIN_SET);
	HAL_Delay(5000);
	
}

void uart_sendstring_device(void* string)
{
	HAL_UART_Transmit_DMA(device_uart,	string,	strlen((char*)string));
}

void uart_sendstring_pc(void* string)
{
	//printf((char*)string,	250);
}
	
void uartflush(void)
{
	memset(gMainBuf,	0,	gNewPos);
	memset(gRxBuf,		0,	RxBuf_SIZE);
	memset(gTxBuf,		0,	TxBuf_SIZE);
	
	gOldPos	=	0;
	gNewPos	=	0;
}


int waitfor(void* string)
{
	HAL_IWDG_Refresh(&hiwdg);
	
	if(strstr((char*)gMainBuf,	(char*)string)	==	NULL)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int simstart(void)
{

//	HAL_GPIO_WritePin(simcard_GPIO_Port,simcard_Pin,GPIO_PIN_RESET);
	simreset();
	uart_sendstring_pc("AT:\r\n");
	uart_sendstring_device("AT\r\n");
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("OK")	!=	0)
		{
			break;
		}
		
		if(HAL_GetTick()-timeOut	>	2000)
		{		
			uart_sendstring_pc("AT no answer\r\n");
			lcd_put_cur(1, 0);
			lcd_send_string("SimMod not found");
			return 0;
		}	
	}
	
	lcd_put_cur(1, 0);
	lcd_send_string("Sim	Module found");
	
	uart_sendstring_pc("AT IS OK\r\n");
	uartflush();
	HAL_Delay(5500);

	uart_sendstring_pc("Echo:\r\n");
	uart_sendstring_device("ATE 1\r\n");
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("OK")	!=	0)
		{
			break;
		}
	}
	HAL_Delay(300);
	uartflush();
	uart_sendstring_pc("Echo OK\r\n");

	uart_sendstring_pc("SIM:\r\n");
	uart_sendstring_device("AT+CPIN?\r\n");
	
	timeOut	=	HAL_GetTick();
	
	while(1)
	{
		if(waitfor("+CPIN: READY")	!=	0)
		{
			break;
		}
		
		if(waitfor("ERROR")	!=	0)
		{	
			lcd_put_cur(1, 0);
			lcd_send_string("SimCard N Found ");
			uart_sendstring_pc("SIMCARD IS NOT IN\r\n");
			return 0;
		}
		
		if(HAL_GetTick()-timeOut>2000)
		{	
			lcd_put_cur(1, 0);
			lcd_send_string("SimCard N Found ");
			uart_sendstring_pc("no simcard reply\r\n");
			return 0;
		}
	}	
	
	lcd_put_cur(1, 0);
	lcd_send_string("Sim Card Found  ");
	uart_sendstring_pc("SIMCARD IS IN\r\n");
	
	uartflush();
	HAL_Delay(300);
	
	uart_sendstring_pc("REG:\r\n");
	uart_sendstring_device("AT+CREG?\r\n");
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("+CREG: 0,1")	!=	0)
		{
			break;
		}
		
		if(waitfor("+CREG: 0,2")	!=	0)
		{		
			break;
		}	
		
		if(waitfor("ERROR")	!=	0)
		{	
			lcd_put_cur(1, 0);
			lcd_send_string("Sim Reg Not Ok  ");
			
			uart_sendstring_pc("REG IS NOT OK\r\n");
			return 0;
		}
		
		if(HAL_GetTick()-timeOut	>	2000)
		{		
			lcd_put_cur(1, 0);
			lcd_send_string("Sim Reg Not Ok  ");
			
			uart_sendstring_pc("no simcard reply\r\n");
			return 0;
		}
	}
	
	lcd_put_cur(1, 0);
	lcd_send_string("Sim Reg Is Ok   ");
	uart_sendstring_pc("REG IS OK\r\n");
	HAL_Delay(300);
	uartflush();
	
	uart_sendstring_device("AT+CIPSHUT\r\n");
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("SHUT OK")	!=	0)
		{
			break;
		}
		
		if(waitfor("ERROR")	!=	0)
		{	
			uart_sendstring_pc("cip shut NOT OK\r\n");
			break;
		}
		if(HAL_GetTick()-timeOut	>	5000)
		{		
			uart_sendstring_pc("no cip shut reply\r\n");
			return 0;
		}
	}

	lcd_put_cur(1, 0);
	lcd_send_string("Sim CIPHUT Ok   ");
	
	uart_sendstring_pc("cip shut ok\r\n");	
	uartflush();
	HAL_Delay(300);

	uart_sendstring_device("AT+CIPMUX=0\r\n");
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("OK")	!=	0)
		{
			break;
		}
		
		if(waitfor("ERROR")	!=	0)
		{		
						
			lcd_put_cur(1, 0);
			lcd_send_string("Sim CIPMUX Nt Ok");
			
			uart_sendstring_pc("CIPMUX IS NOT OK\r\n");
			break;
		}
		
		if(HAL_GetTick()-timeOut	>	5000)
		{		
						
			lcd_put_cur(1, 0);
			lcd_send_string("Sim CIPMUX Nt Ok");
			uart_sendstring_pc("no CIPMUX reply\r\n");
			return 0;
		}
	}

	lcd_put_cur(1, 0);
	lcd_send_string("Sim CIPMUX Is Ok");
	
	uart_sendstring_pc("cip mux=0\r\n");	
	uartflush();
	HAL_Delay(300);
	
	uart_sendstring_pc("GPRS:\r\n");
	uart_sendstring_device("AT+CGATT?\r\n");
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("+CGATT: 1")	!=	0)
		{
			break;
		}
		
		if(waitfor("ERROR")	!=	0)
		{
			lcd_put_cur(1, 0);
			lcd_send_string("Sim GPRS Not Ok ");
			
			uart_sendstring_pc("GPRS IS NOT OK\r\n");
			return 0;
		}
		
		if(waitfor("+CGATT: 0")	!=	0)
		{		
			break;
		}	
		
		if(waitfor("+CGATT: 2")	!=	0)
		{		
			lcd_put_cur(1, 0);
			lcd_send_string("Sim GPRS Not Ok ");
			uart_sendstring_pc("GPRS IS NOT OK\r\n");
			return 0;
		}
		
		if(HAL_GetTick()-timeOut	>	10000)
		{		
			uart_sendstring_pc("no GPRS reply\r\n");
			return 0;
		}
	}
	lcd_put_cur(1, 0);
	lcd_send_string("Sim GPRS Is Ok  ");
	uart_sendstring_pc("GPRS IS OK\r\n");
	uartflush();
	HAL_Delay(300);

	
	uart_sendstring_pc("APN:\r\n");
	uart_sendstring_device("AT+CSTT=\"CMNET""\"\r\n");	
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("OK")	!=	0)
		{
			break;
		}		
		
		if(waitfor("ERROR")	!=	0)
		{		
			lcd_put_cur(1, 0);
			lcd_send_string("Sim APN Not Ok  ");
			uart_sendstring_pc("APN IS NOT OK\r\n");
			return 0;
		}
		
		if(HAL_GetTick()-timeOut	>	10000)
		{		
			lcd_put_cur(1, 0);
			lcd_send_string("Sim APN Not Ok  ");
			uart_sendstring_pc("no APN reply\r\n");
			return 0;
		}
	}
	
	lcd_put_cur(1, 0);
	lcd_send_string("Sim APN Is Ok   ");
	uart_sendstring_pc("APN IS OK\r\n");
	HAL_Delay(5000);
	uartflush();

	
	uart_sendstring_pc("DATA:\r\n");
	uart_sendstring_device("AT+CIICR\r\n");
	timeOut	=	HAL_GetTick();
	while(1)
	{
		if(waitfor("OK")!=0)
		{
			break;
		}
		
		if(waitfor("ERROR")!=0)
		{
			lcd_put_cur(1, 0);
			lcd_send_string("Sim DATA Not Ok ");
			uart_sendstring_pc("DATA IS NOT OK\r\n");
			return 0;
		}
		if(HAL_GetTick()-timeOut>30000)
		{	
			lcd_put_cur(1, 0);
			lcd_send_string("Sim DATA Not Ok ");
			uart_sendstring_pc("no DATA reply\r\n");
			return 0;
		}
	}
	uart_sendstring_pc("DATA IS OK\r\n");
	
	lcd_put_cur(1, 0);
	lcd_send_string("Sim DATA Is Ok  ");
	
	uart_sendstring_device("AT+CIFSR\r\n");
	HAL_Delay(200);
	uart_sendstring_pc(gMainBuf);
	uartflush();

	
	uart_sendstring_device("AT+CIPCLOSE\r\n");
	HAL_Delay(500);
	uartflush();

	return 1;
}

int	srevercon(void* ip,void* port)
{
	memset((char*)gTxBuf,	0,	strlen((char*)gTxBuf));
	
	strcpy((char*)gTxBuf,	"AT+CIPSTART=\"TCP\",\"");
	strcat((char*)gTxBuf,	ip);
	strcat((char*)gTxBuf,	"\",\"");
	strcat((char*)gTxBuf,	port);
	strcat((char*)gTxBuf,	"""\"\r\n");
	
	uart_sendstring_device(gTxBuf);
	uint32_t t	=	HAL_GetTick();
	while(waitfor("CONNECT OK")	==	0)
	{
		if(waitfor("ALREADY CONNECT")	!=	0)
		{
			uart_sendstring_pc("was connected to SERVER\r\n");
			return 1;
		}
		if(waitfor("CONNECT FAIL")	!=	0)
		{
			uart_sendstring_pc("FAIL, could not connect to SERVER\r\n");
			return 0;
		}
		if(waitfor("ERROR")	!=	0	&&	waitfor("ALREADY CONNECT")	==	0)
		{
			uart_sendstring_pc("ERROR, could not connect to SERVER\r\n");
			return -1;
		}
		if(HAL_GetTick()-t	>	20000)
		{
			uart_sendstring_pc("ERROR,no respond, could not connect to SERVER\r\n");
			return -1;
		}
		HAL_IWDG_Refresh(&hiwdg);
	}
	uart_sendstring_pc("NOW CONNECTED TO SERVER\r\n");
	
	return 1;
}

int	srevercon_udp(void* ip,void* port)
{
	memset((char*)gTxBuf,0,strlen((char*)gTxBuf));
	
	strcpy((char*)gTxBuf,	"AT+CIPSTART=\"UDP\",\"");
	strcat((char*)gTxBuf,	ip);
	strcat((char*)gTxBuf,	"\",\"");
	strcat((char*)gTxBuf,	port);
	strcat((char*)gTxBuf,	"""\"\r\n");
	
	uart_sendstring_device(gTxBuf);
	
	uint32_t t	=	HAL_GetTick();
	
	while(waitfor("CONNECT OK")	==	0)
	{
		if(waitfor("ALREADY CONNECT")	!=	0)
		{
			uart_sendstring_pc("was connected to SERVER\r\n");
			return 1;
		}
		if(waitfor("CONNECT FAIL")	!=	0)
		{
			uart_sendstring_pc("FAIL, could not connect to SERVER\r\n");
			return 0;
		}
	  if(waitfor("ERROR")	!=	0	&&	waitfor("ALREADY CONNECT")	==	0)
		{
			uart_sendstring_pc("ERROR, could not connect to SERVER\r\n");
			return -1;
		}
		if(HAL_GetTick()-t	>	5000)
		{
			uart_sendstring_pc("ERROR,no respond, could not connect to SERVER\r\n");
			return -1;
		}
	}
	uart_sendstring_pc("NOW CONNECTED TO SERVER\r\n");
	
	return 1;
}

int simsend(void* string,	int i)
{
	uart_sendstring_device("AT+CIPSEND\r\n");
	char temp[1];
	temp[0]=0x1A;
	uint32_t t = HAL_GetTick();
	while(strstr((char*)gMainBuf,">")	==	NULL)
	{
		if(HAL_GetTick()-t	>	3000)
		{
			break;
//			memset((char*)gTxBuf,0,strlen((char*)gTxBuf));
//			strcpy((char*)gTxBuf,"shitsim\r\n");
//			strcat((char*)gTxBuf,(char*)temp);
//			uart_sendstring_device(gTxBuf);
//			return 0;
		}
		if(strstr((char*)gMainBuf,"ERROR")	!=	NULL)
		{
			uart_sendstring_pc("at+cipsend time error\r\n");
			return 0;
		}
	}
	if(i	==	0)
	{
		memset((char*)gTxBuf,	0,	strlen((char*)gTxBuf));
		
		strcpy((char*)gTxBuf,	string);
		strcat((char*)gTxBuf,	temp);
		
		uart_sendstring_device(gTxBuf);
		return 1;
	}
		if(i	!=	0)
	{
		memset((char*)gTxBuf,	0,	strlen((char*)gTxBuf));
		
		strcpy((char*)gTxBuf,	string);
		strcat((char*)gTxBuf,	temp);
		
		HAL_UART_Transmit_DMA(device_uart,	gTxBuf,	i);
		return 1;
	}
	return 0;
}
