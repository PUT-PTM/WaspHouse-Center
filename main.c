#include "stm32f4xx_conf.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "misc.h"
#include <stdio.h>
#include "STM32F4xx.h"


void LedInit(void);
void Delay_us(volatile uint32_t delay);
void startWaspCenter(void);
void USART3Init(void);
void USART3_IRQHandler(void);
void USART_Send(volatile char *c);


void connectHomeNetwork(char* name, char* password);
void StartConnectonWithServer(char* ip, int port);
void CloseConnectonWithServer(void);
void SendToWaspCenter(volatile char* message);

const int SIZE_WIFI_BUFFOR = 1024;
char tabWifiBuffor[1024];
char tabWifiResponse[10]={' '};

int read = 0;	//	0-czyta komunikaty	1-nie czyta komunikatów od wifi

//test test test
void printfMagicTable(){

	printf("\nWifi:%s",tabWifiBuffor);

	//clear tab
	for(int k=0; k<512; k++){
		tabWifiBuffor[k]=0;
	}

	printf("\ntab:%s",tabWifiResponse);
}

//const DATA
const char* NAME= "WaspCenter";

const char* NAME_WASPCENTER_AP = "wasphouse";
const char* PASSWORD_WASPCENTER_AP = "admin123";
const int PORT_WASPCENTER_SERVER = 280;

const char* NAME_HOME_NETWORK = "test";
const char* PASSWORD_HOME_NETWORK = "admin123";

const char* IP_SERVER = "192.168.43.129";
const int PORT_SERVER = 280;

//JSON
char received[512];
int receivingJSON = 0;
int i = 0;
int newJSON = 0;


int main (void) {

	SystemInit();

	LedInit();

	//WIFI INIT
	USART3Init();

	//tutaj powinna byæ konfiguracja hotspot
	//i oczekiwanie na podanie danych

	startWaspCenter();

	connectHomeNetwork(NAME_HOME_NETWORK, PASSWORD_HOME_NETWORK);

	StartConnectonWithServer(IP_SERVER, PORT_SERVER);

	char buffer[512];
	int n = sprintf(buffer,"%s\r\n",NAME);
	SendToServer(buffer);

	while(1){

		if(newJSON == 1){
			newJSON = 0;

			char buffer[512];
			int n = sprintf(buffer, "%s\r\n", received);
			SendToServer(buffer);
			for(int i=0;i<512;i++)
				received[i]=0;
		}

	}

	SendToServer("{EXIT}\r\n");
	CloseConnectonWithServer();
	checkWifiResponseOKOrERROR();
}

void LedInit(void){
	/* GPIOD Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitTypeDef  GPIO_InitStructure;
	/* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void Delay_us(volatile uint32_t delay)
{
	delay*=24;
	while(delay--);
}

void connectHomeNetwork(char* name, char* password)
{
	char buffer[512];
	int n = sprintf(buffer,"AT+CWJAP=\"%s\",\"%s\"\r\n",name,password);

	USART_Send(buffer);
	checkWifiResponseOKOrERROR();
}

void startWaspCenter(void)
{
	USART_Send("AT+CWMODE=3\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CIPMUX=1\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CIPMODE=1\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CWSAP=\"xxx\",\"\",4,0\r\n");
	checkWifiResponseOKOrERROR();

	char buffor[512];
	int n = sprintf(buffor,"AT+CIPSERVER=1,%d\r\n",PORT_WASPCENTER_SERVER);
	USART_Send(buffor);
	checkWifiResponseOKOrERROR();
	//Delay_us(200000);
}

void USART3Init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(USART3_IRQn);

	USART_Cmd(USART3, ENABLE);
}

void USART3_IRQHandler(void)
{
    if((USART_GetITStatus(USART3, USART_IT_RXNE) != RESET))
    {

    	char sign = USART3->DR;

    	if((read == 0) && ((((sign >= 32) && (sign <= 126)) || (sign == '\n')) || (sign == '\r'))){

    		if(sign == '\n')	sign = '|';
    		if(sign == '\r')	sign = '\\';

    		for(int i=1;i<10;i++)
				tabWifiResponse[i-1]=tabWifiResponse[i];
			tabWifiResponse[9]=sign;

    		//test test test
    		int n = strlen(tabWifiBuffor);

			if(n < SIZE_WIFI_BUFFOR){
				tabWifiBuffor[n] = sign;
			}else{
				for(int k=0; k<SIZE_WIFI_BUFFOR; k++){
					tabWifiBuffor[k]=0;
				}
			}

			if(sign == '{') {
				i = 0;
				receivingJSON++;
			}

			if(receivingJSON > 0) {
				received[i] = sign;
				i++;
			}

			if(sign == '}') {
				receivingJSON--;

				if(receivingJSON == 0) {
					// tu odebra³eœ jsona, znajduj¹cego siê w received
					newJSON = 1;
				}
			}

    	}
    }
}

void USART_Send(volatile char *c)
{
	//test test test
	if(read == 0){
		printfMagicTable();
	}

	int length = strlen(c);

	for(int i=0; i<length; i++){
		USART_SendData(USART3, c[i]);
		Delay_us(500);
	}

}

void StartConnectonWithServer(char* ip, int port){
	Delay_us(200000);
	char buffer[512];
	int n = sprintf(buffer,"AT+CIPSTART=0,\"TCP\",\"%s\",%d\r\n",ip,port);

	USART_Send(buffer);
	checkWifiResponseOKOrERROR();
	//Delay_us(2000000);

	//test test test
	GPIO_SetBits(GPIOD,GPIO_Pin_12);
	printfMagicTable();
}

void CloseConnectonWithServer(void){
	USART_Send("AT+CIPCLOSE\r\n");
	checkWifiResponseOKOrERROR();

	//test test test
	GPIO_ResetBits(GPIOD,GPIO_Pin_12);
	printfMagicTable();
}

//send to waspcenter
void SendToServer(volatile char* message)
{
	int length = strlen(message);

	char buffer[512];
	int n = sprintf(buffer, "AT+CIPSEND=0,%d\r\n", length);
	USART_Send(buffer);
	checkWifiResponseOKOrERROR();

	USART_Send(message);

	checkWifiResponseOKOrERROR();

	//test test test
	printfMagicTable();
}

void checkWifiResponseOKOrERROR(){
	int flag=1;
	while(flag){
		for(int i=0;i<9;i++){
			if(tabWifiResponse[i]=='O'&&tabWifiResponse[i+1]=='K')
				flag=0;
		}
		for(int i=0;i<6;i++){
			if(tabWifiResponse[i]=='E'&&tabWifiResponse[i+1]=='R'&&tabWifiResponse[i+2]=='R'&&tabWifiResponse[i+3]=='O'&&tabWifiResponse[i+4]=='R')
				flag=0;
		}
	}
	for(int i=0;i<10;i++){
		tabWifiResponse[i]=' ';
	}
}
