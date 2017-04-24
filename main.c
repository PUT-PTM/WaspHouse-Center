/**
 * Grzegorz Osak
 * Szymon Zieliñski
 */
#include "stm32f4xx_conf.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "misc.h"

uint16_t buffer;

void GPIOInit(void);
void Delay_us(volatile uint32_t delay);
void startServer(void);
void USART3Init(void);
void USART3_IRQHandler(void);
void USART_Send(volatile char *c);

				/* MAIN START */
int main(void)
{
	SystemInit();
	GPIOInit();
	USART3Init();

	GPIO_SetBits(GPIOD, GPIO_Pin_12);		//Low state activates the relay
	GPIO_SetBits(GPIOD, GPIO_Pin_13);		//			-||-
	unsigned int i=0;
	startServer();

	void USART3_IRQHandler(void);

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
	USART_Send("AT+CWMODE=2\r\n");
	Delay_us(500000);
	USART_Send("AT+CWSAP=\"dzialaj\",\"12345678\",4,2\r\n");
	Delay_us(500000);
	USART_Send("AT+CIPMUX=1\r\n");
	Delay_us(500000);
	USART_Send("AT+CIPSERVER=1,80\r\n");
	Delay_us(500000);
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
    	buffer=USART3->DR;
    	if (buffer == 'G'||buffer == 'g')
    	{
    		GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
    	}
    	if (buffer == 'R'||buffer == 'r')
    	{
    	    GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
    	}
    	if (buffer == 37)
    	{
    		USART_Send("AT+RST\r\n");
    		Delay_us(1000000);
    		startServer();
    	}
    }
}

void USART_Send(volatile char *c)
{
	while(*c)
	{
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		USART_SendData(USART3, *c);
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
		Delay_us(500);
		*c++;
	}
}
