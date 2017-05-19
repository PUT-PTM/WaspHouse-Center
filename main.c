/**
 * Grzegorz Osak
 * Szymon Zieliï¿½ski
 */
#include "stm32f4xx_conf.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "misc.h"
#include "stm32f4xx_tim.h"
#include <stdio.h>
#include <inttypes.h>

char buffer;
uint16_t rec;
int x=99;
volatile char *recieve;
char tab[32];
int tab_licznik=0;
int recieve_position=0;
//resend message
int msg_flag=0;
int i=0;

void GPIOInit(void);
void Delay_us(volatile uint32_t delay);
void startServer(void);
void USART3Init(void);
void USART3_IRQHandler(void);
void USART_Send(volatile char *c);
void przycisk();

				/* MAIN START */
int main(void)
{
	SystemInit();
	GPIOInit();
	USART3Init();
	przycisk();

	//GPIO_SetBits(GPIOD, GPIO_Pin_12);		//Low state activates the relay
	//GPIO_SetBits(GPIOD, GPIO_Pin_13);		//			-||-
	unsigned int i=0;
	startServer();


	while(1){
	}
}
				/* MAIN END */

void GPIOInit(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void przycisk(void){

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	//deklaracja przycisku na plytce
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


		// ustawienie trybu pracy priorytetów przerwañ
		//przerwanie diody
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

		NVIC_InitTypeDef NVIC_InitStructure;
		// numer przerwania
		NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
		// priorytet g³ówny
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
		// subpriorytet
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
		// uruchom dany kana³
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		// zapisz wype³nion¹ strukturê do rejestrów
		NVIC_Init(&NVIC_InitStructure);

		// wyczyszczenie przerwania od timera 3 (wyst¹pi³o przy konfiguracji timera)
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		// zezwolenie na przerwania od przepe³nienia dla timera 3
		TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);



		//przerwanie przycisku
		NVIC_InitTypeDef NVIC_InitStructure2;
		// numer przerwania
		NVIC_InitStructure2.NVIC_IRQChannel = EXTI0_IRQn;
		// priorytet g³ówny
		NVIC_InitStructure2.NVIC_IRQChannelPreemptionPriority = 0x00;
		// subpriorytet
		NVIC_InitStructure2.NVIC_IRQChannelSubPriority = 0x00;
		// uruchom dany kana³
		NVIC_InitStructure2.NVIC_IRQChannelCmd = ENABLE;
		// zapisz wype³nion¹ strukturê do rejestrów
		NVIC_Init(&NVIC_InitStructure2);

		EXTI_InitTypeDef EXTI_InitStructure;
		// wybór numeru aktualnie konfigurowanej linii przerwañ
		EXTI_InitStructure.EXTI_Line = EXTI_Line0;
		// wybór trybu - przerwanie b¹dŸ zdarzenie
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
		// wybór zbocza, na które zareaguje przerwanie
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
		// uruchom dan¹ liniê przerwañ
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		// zapisz strukturê konfiguracyjn¹ przerwañ zewnêtrznych do rejestrów
		EXTI_Init(&EXTI_InitStructure);

		// pod³¹czenie danego pinu portu do kontrolera przerwañ
		SYSCFG_EXTILineConfig(GPIOA, GPIO_Pin_0);

}

void Delay_us(volatile uint32_t delay)
{
	delay*=24;
	while(delay--);
}

void startServer(void)
{
	Delay_us(500000);
	USART_Send("AT+RST\r\n");
	Delay_us(500000);
	USART_Send("AT+CWMODE=3\r\n");
	Delay_us(500000);
	//GPIO_SetBits(GPIOD, GPIO_Pin_13);
	USART_Send("AT+CWSAP=\"xxx\",\"\",4,0\r\n");
	Delay_us(5000000);
	USART_Send("AT+CWJAP=\"testowa\",\"abcabcabc\"\r\n");
	Delay_us(3000000);
	//GPIO_SetBits(GPIOD, GPIO_Pin_14);
	USART_Send("AT+CIPMUX=1\r\n");
	Delay_us(500000);
	//GPIO_SetBits(GPIOD, GPIO_Pin_15);
	USART_Send("AT+CIPSERVER=1,80\r\n");
	Delay_us(500000);
	USART_Send("AT+CIPSTO=2700\r\n");
	Delay_us(500000);
	GPIO_SetBits(GPIOD, GPIO_Pin_15);
	sendToServer("DUPA");
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

    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
    	rec=USART3->DR;
    	buffer = rec & 0xFF;
    	//printf("%" PRIu16 " \n",rec);
    	printf("%c ",buffer);

    	if(buffer=='+'){
    		msg_flag=1;
    	}
    	if(buffer=='I'&&msg_flag==1){
    		msg_flag=2;
    	}
    	if(buffer=='P'&&msg_flag==2){
    	    msg_flag=3;
    	}
    	if(buffer=='D'&&msg_flag==3){
    	    msg_flag=4;
    	}
    	if(msg_flag==4){

    		tab[tab_licznik]=buffer;
    		tab_licznik++;
    		if(tab_licznik>31){
    			msg_flag=0;
    			tab_licznik=0;
    			//sendMessageToServer(tab);
    		}
    	}
    	/*tab[tab_licznik]=buffer;
    	tab_licznik++;
    	if(tab[tab_licznik]=='\n'&&!tab_licznik){
    		recieve=tab;
    		tab_licznik=0;
    		for(i=0;i<32;i++){
    			if(tab[i]=='+')
    				sendToServer();
    		}
    	}*/

    	if (buffer == 'G'||buffer == 'g')
    	{
    		GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
    	}
    	if (recieve=="ERROR")
    	{
    	    GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
    	}

    }
    return buffer;
}

void USART_Send(volatile char *c)
{
	printf("%s\n",c);
	while(*c)
	{
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		USART_SendData(USART3, *c);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
		Delay_us(500);
		*c++;
	}
}

void USART_Send_32(volatile char *c)
{
	int licznik=0;
	printf("%s\n",c);
	while(*c)
	{
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		USART_SendData(USART3, *c);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
		Delay_us(500);
		licznik++;
		*c++;
	}
	licznik+=2;

	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	USART_SendData(USART3, '\r');
	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
	Delay_us(500);

	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	USART_SendData(USART3, '\n');
	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
	Delay_us(500);

	for(i=licznik;i<=32;i++){
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		USART_SendData(USART3, ' ');
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
		Delay_us(500);
	}
}

void TIM3_IRQHandler(void) {
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		if(GPIO_ReadInputData(GPIO_Pin_0))

			setConnectionToServer();
			sendMessageToServer("ABCDE");
			dropConnectionWithServer();

			GPIO_ToggleBits(GPIOD,GPIO_Pin_12);
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

        TIM_Cmd(TIM3, DISABLE);
        // wyzerowanie flagi wyzwolonego przerwania
 		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

void EXTI0_IRQHandler(void)
{

        if(EXTI_GetITStatus(EXTI_Line0) != RESET)
        {
        	TIM_Cmd(TIM3, ENABLE);
   	   	}

}

void sendToServer(volatile char *c){
    //wysylanie_danych
	USART_Send("AT+CIFSR\r\n");
	Delay_us(500000);
	/*USART_Send("AT+CIPSTART=1,\"TCP\",\"192.168.43.197\",280\r\n");
	Delay_us(2000000);
	USART_Send("AT+CIPSEND=1,32\r\n");
	Delay_us(500000);*/
	setConnectionToServer();
	sendMessageToServer("AAA");
	dropConnectionWithServer();
	/*USART_Send_32(recieve);
	//while(recieve!="SEND OK");
	Delay_us(500000);*/
	//while(recieve!="SEND OK\r\n");
	//USART_Send("AT+CIPCLOSE=1\r\n");
	//Delay_us(500000);

}
void setConnectionToServer(){
	USART_Send("AT+CIPSTART=1,\"TCP\",\"192.168.43.197\",280\r\n");
	Delay_us(50000);
}

void sendMessageToServer(volatile char *message){
	USART_Send("AT+CIPSEND=1,32\r\n");
	Delay_us(50000);
	USART_Send_32(message);
	Delay_us(300000);
}

void dropConnectionWithServer(){
	USART_Send("AT+CIPCLOSE=1\r\n");
	Delay_us(50000);
}




