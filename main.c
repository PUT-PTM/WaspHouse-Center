#include "stm32f4xx_conf.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "misc.h"
#include "stm32f4xx_tim.h"
#include "protocol.h"
char tabJSON[55];

//SD
#include "stm32f4xx_spi.h"
#include "ff.h"
#include "stdio.h"
#include "cstring.h"
#include "string.h"
#include "sd_lib.h"
#include "stdio.h"
void initSD(void);
FATFS fatfs;
FIL fs, fd;
FRESULT fr;
UINT bw, br;

void GPIOInit(void);
void Delay_us(volatile uint32_t delay);
void USART3Init(void);
void USART3_IRQHandler(void);
void USART_Send(volatile char *c);
void button();
void updatePortPin(JSON json, GPIO_TypeDef *port, uint16_t pin);
void sendJSON(JSON json, int ID);
void startWIFI();
void doOrder(JSON json);
void JSONToCharArray2(JSON structure);
void redefineJSON(JSON *json);
void createTab();

JSON device;
JSON request[4];
JSON receivedData[32];
const int SIZE_WIFI_BUFFOR=1024;
const int MAX_ALLOWED_TEMPERATURE = 35;
const int MAX_ALLOWED_HUMIDITY = 50;
char tabWifiBuffor[1024];
char tabWifiResponse[10]={' '};
char jsonBuffer[55];
int receivedJSON = 0;
int channel = 0;
int i = 0;
int read = 0;
int receivedLength = 0;
int numberOfJSONs = 0;
int last;

void printMagicTable(){
	printf("\nWifi: %s", tabWifiBuffor);
	for(int clear=0;clear<512;clear++){
		tabWifiBuffor[clear]=0;
	}
	printf("Wifi response: %s\n", tabWifiResponse);
}

int main() {
	SystemInit();

	device.systemID = 100;  // ID Centrum
	device.roomID = 100;	// centrum znajduje siê w piwnicy --> 100.
	device.order = 100;		// domyslny order
	device.value = 100;		// domyslny

	GPIOInit();
	USART3Init();
	button();
	startWIFI();
	timer4();

	//SD
	initSD();
	f_mount(0, &fatfs);
	fr = f_open(&fs, "DATA.bin", FA_CREATE_ALWAYS | FA_WRITE);

	fr = f_write(&fs, "Dane - WaspHause:\n", 18, &bw);
	for(int i=0; i<100; i++){
		fr = f_write(&fs, "{przykladowy JSON}\n", 19, &bw);
	}

	while(1) {
		//if(receivedJSONs >= 0 && last != receivedJSONs) {
		//	sendJSON(request[receivedJSONs / 2], receivedJSONs % 2);
		//
		//	if(receivedJSONs / 2 == 4 && receivedJSONs % 2 == 1) {
		//		receivedJSONs = -1;
		//		last = -1;
		//	}
		//}
		//last = receivedJSONs;
	}
}


void GPIOInit() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void button() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef  GPIO_InitStructure2;
	GPIO_InitStructure2.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure2.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure2);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 1999;
	TIM_TimeBaseStructure.TIM_Prescaler = 8399;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode =  TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure2;
	NVIC_InitStructure2.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure2.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure2.NVIC_IRQChannelSubPriority = 0x00;
	NVIC_InitStructure2.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure2);

	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	SYSCFG_EXTILineConfig(GPIOA, GPIO_Pin_0);
}

void timer4() {
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure3;
	TIM_TimeBaseStructure3.TIM_Period = 23999;
	TIM_TimeBaseStructure3.TIM_Prescaler = 8399*6;
	TIM_TimeBaseStructure3.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure3.TIM_CounterMode = TIM_CounterMode_Up ;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure3);
	TIM_Cmd(TIM4, ENABLE);
	NVIC_InitTypeDef NVIC_InitStructure3;
	NVIC_InitStructure3.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure3.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure3.NVIC_IRQChannelSubPriority = 0x00;
	NVIC_InitStructure3.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure3);
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
}

void Delay_us(volatile uint32_t delay) {
	delay*=24;
	while(delay--);
}

void startWIFI(void) {
	Delay_us(500000);
	read = 1;
	USART_Send("AT+RST\r\n");
	checkWifiResponseOKOrERROR();
	read = 0;
	USART_Send("AT+CIPMUX=1\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CWMODE=3\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CWSAP=\"CENTER\",\"\",4,0\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CWJAP=\"Wasp_Wi-Fi\",\"75757575\"\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CIPSERVER=1,80\r\n");
	checkWifiResponseOKOrERROR();
	USART_Send("AT+CIFSR\r\n");
	checkWifiResponseOKOrERROR();
	GPIO_SetBits(GPIOD, GPIO_Pin_15);
}

void USART3Init(void) {
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

void USART3_IRQHandler(void) {
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
		char sign = USART3->DR;

		if(sign == '\n'||sign == '\r')	sign = '\\';
		for(int i=1;i<10;i++)
		    tabWifiResponse[i-1]=tabWifiResponse[i];
		tabWifiResponse[9]=sign;

		if((read == 0) && ((((sign >= 32) && (sign <= 126)) || (sign == '\n')) || (sign == '\r'))) {
			if(sign == '\n')	sign = '|';
			if(sign == '\r')	sign = '\\';

			int n = strlen(tabWifiBuffor);

			if(n < SIZE_WIFI_BUFFOR){
				tabWifiBuffor[n] = sign;
			} else{
				for(int k=0; k<SIZE_WIFI_BUFFOR; k++){
					tabWifiBuffor[k]=0;
				}
			}

			if(USART3->DR == '{') {
				receivedJSON++;
				i = 0;
			}

			if(receivedJSON > 0) {
				jsonBuffer[i] = USART3->DR;
				i++;
			}

			if(USART3->DR == '}') {
				numberOfJSONs++;
				i = 0;
				receivedJSON = 0;
				printf("\njson: %s", jsonBuffer);
				printf("  %s  ", "kk");
				JSON json;
				charArrayToJSON(&json, &jsonBuffer);
				doOrder(json);
			}
		}
    }
}

void updatePortPin(JSON json, GPIO_TypeDef *port, uint16_t pin) {
	int ID;
	switch(json.value) {
	case 100: GPIO_SetBits(port, pin); break;
	case 101: GPIO_ResetBits(port, pin); break;
	case 102: GPIO_ToggleBits(port, pin); break;
	case 103:
		receivedData[receivedLength] = json;
		JSONToCharArray(json);
		fr = f_write(&fs, tabJSON, strlen(tabJSON), &bw);
		receivedLength++;
	break;
	}
}


void doOrder(JSON json) {
	if(json.roomID < 0) {
		JSONToCharArray(json);
		sendJSON(json, json.systemID - 100);
		return;
	}

	switch(json.order) {
	case 301: // niebieska dioda
		updatePortPin(json, GPIOD, GPIO_Pin_15);
		break;

	case 302: // czerwona dioda
		updatePortPin(json, GPIOD, GPIO_Pin_14);
		break;

	case 303: // pomarañczowa dioda
		updatePortPin(json, GPIOD, GPIO_Pin_13);
		break;

	case 304: // zielona dioda
		updatePortPin(json, GPIOD, GPIO_Pin_12);
		break;

	case 760: // odes³anie danych na serwer
		for(int i = 0; i < receivedLength; i++) {
			sendJSON(receivedData[i], 2);
		} receivedLength = 0;
		int length = 6;
		char buffer[32];
		int n = sprintf(buffer, "AT+CIPSEND=%d,%d\r\n", 2, length);
		USART_Send(buffer);
		USART_Send("STOP\r\n");
	break;

	case 800: // przypisanie nowego ID
		json.roomID = device.roomID;
		json.systemID = device.systemID;
		json.value = 100 + channel;
		sendJSON(json, channel);
		channel++;
		break;
	}

	if(json.order == 900 && json.roomID > 0) { // wlacz awaryjna klime od temperatury
		if(json.value - 500 > MAX_ALLOWED_TEMPERATURE) {
			JSON x;
			x.roomID = device.roomID;
			x.order = 304;
			x.value = 100;
			x.systemID = json.systemID;
			sendJSON(x, x.systemID - 100);
		} else {
			JSON x;
			x.roomID = device.roomID;
			x.order = 304;
			x.value = 101;
			x.systemID = json.systemID;
			sendJSON(x, x.systemID - 100);
		}
	}

	if(json.order == 901 && json.roomID > 0) { // wlacz awaryjna klime od wilgotnosci
		printf(" HUMIDITY %d    XX", json.value - 500);
		if(json.value - 500 > MAX_ALLOWED_HUMIDITY) {
			JSON x;
			x.roomID = device.roomID;
			x.order = 304;
			x.value = 100;
			x.systemID = json.systemID;
			sendJSON(x, x.systemID - 100);
		} else {
			JSON x;
			x.roomID = device.roomID;
			x.order = 304;
			x.value = 101;
			x.systemID = json.systemID;
			sendJSON(x, x.systemID - 100);
		}
	}

	if(json.order >= 900) {
		receivedData[receivedLength] = json;
		JSONToCharArray(json);
		fr = f_write(&fs, tabJSON, strlen(tabJSON), &bw);
		receivedLength++;
	}
}

void redefineJSON(JSON *json) {
	json->systemID = device.systemID;
	json->roomID = device.roomID;
}

int dadaad = 0;
void TIM3_IRQHandler(void) {
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		if(GPIO_ReadInputData(GPIO_Pin_0)) {
			JSON json;

			if(dadaad == 0) {
				//int k = numberOfJSON;
				json.roomID = 100;
				json.systemID = 100;
				json.order = 904;
				json.value = 100;
				sendJSON(json, json.systemID - 100);
			}

			if(dadaad > 0) {
				json.order = 900;
				json.roomID = 100;
				json.systemID = 101;
				json.value = 100;
				sendJSON(json, json.systemID - 100);
			}
			dadaad++;
		}
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        TIM_Cmd(TIM3, DISABLE);
 		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

void EXTI0_IRQHandler(void) {
	if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
		TIM_Cmd(TIM3, ENABLE);
	}
}

void JSONToCharArray2(JSON structure) {
	for(int i=0;i<55;i++) tabJSON[i]=0;
    int len = sprintf(tabJSON, "{\"order\":%d,\"systemID\":%d,\"roomID\":%d,\"value\":%d}\r\n", structure.order, structure.systemID, structure.roomID, structure.value);
}

void sendJSON(JSON json, int ID) {
	JSONToCharArray2(json);
	int length = strlen(tabJSON);
	char buffer[32];
	int n = sprintf(buffer, "AT+CIPSEND=%d,%d\r\n", ID, length);
	USART_Send(buffer);
	USART_Send(tabJSON);
}

void USART_Send(volatile char *c) {
	if(read == 0){
		printMagicTable();
	}

	int length = strlen(c);
	for(int i=0; i<length; i++) {
		USART_SendData(USART3, c[i]);
		Delay_us(500);
	}
}

void checkWifiResponseOKOrERROR() {
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
}

void initSD(void){
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // zegar dla portu GPIO z którego wykorzystane zostan¹ piny do SPI (MOSI, MISO, SCK)
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE); // zegar dla portu GPIO z którego wykorzystany zostanie pin do SPI (CS)
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); // zegar dla modu³u SPI2
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure2;

	GPIO_InitStructure2.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure2.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure2.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure2);


	//inicjalizacja pinów wykorzystywanych do SPI
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	// SPI SCK, MISO, MOSI
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// konfigurcja pinu CS
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	// deselecy – ustawienie pinu w stan wysoki
	GPIO_SetBits(GPIOE, GPIO_Pin_3);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2); // SCK
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2); // MISO
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2); // MOSI

	// konfiguracja SPI w trybie MASTER
	SPI_InitTypeDef SPI_InitStructure;
	SPI_I2S_DeInit(SPI2);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_Init(SPI2, &SPI_InitStructure);

	// Uruchomienie SPI2
	SPI_Cmd(SPI2, ENABLE);

	// Ustawienie pinu CS w stan niski
	GPIO_ResetBits(GPIOE, GPIO_Pin_3);

	for(i=0; i<10; i++) {
		SPI_I2S_SendData(SPI2, 0xFF);
		while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	}

	send_cmd(CMD0, 0);
}

void TIM4_IRQHandler(void) {
    if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
    	numberOfJSONs = 0;
    	last = -1;
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}

void createTab() {
	request[0].order = 900;
	request[0].roomID = device.roomID;
	request[0].systemID = device.systemID;
	request[0].value = 100;

	request[1].order = 901;
	request[1].roomID = device.roomID;
	request[1].systemID = device.systemID;
	request[1].value = 100;

	request[2].order = 902;
	request[2].roomID = device.roomID;
	request[2].systemID = device.systemID;
	request[2].value = 100;

	request[3].order = 903;
	request[3].roomID = device.roomID;
	request[3].systemID = device.systemID;
	request[3].value = 100;
}
