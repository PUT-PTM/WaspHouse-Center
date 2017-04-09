/**
 * Grzegorz Osak
 * Szymon Zieliñski
 */

#include "stm32f4xx_conf.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_exti.h"
#include "misc.h"

int main(void) {
	SystemInit();

	/* GPIOD Periph clock enable */
	// wlaczenie taktowania wybranego portu
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	// wlaczenie taktowania wybranego uk³adu USART
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
	//diody
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	//diody
	GPIO_InitTypeDef GPIO_InitStructure1;
	/* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
	GPIO_InitStructure1.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14
			| GPIO_Pin_15;
	GPIO_InitStructure1.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure1.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure1.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure1.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure1);

	// konfiguracja linii Rx i Tx dla USART3 i USART6
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10| GPIO_Pin_11 | GPIO_Pin_6
			| GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	// ustawienie funkcji alternatywnej dla pinów (USART3)
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

	// ustawienie funkcji alternatywnej dla pinów (USART6)
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

	//konfiguracja usart
	USART_InitTypeDef USART_InitStructure;
	// predkosc transmisji (mozliwe standardowe opcje: 9600, 19200, 38400, 57600, 115200, ...)
	USART_InitStructure.USART_BaudRate = 115200;
	// d³ugoœæ s³owa (USART_WordLength_8b lub USART_WordLength_9b)
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	// liczba bitów stopu (USART_StopBits_1, USART_StopBits_0_5, USART_StopBits_2, USART_StopBits_1_5)
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	// sprawdzanie parzystoœci (USART_Parity_No, USART_Parity_Even, USART_Parity_Odd)
	USART_InitStructure.USART_Parity = USART_Parity_No;
	// sprzêtowa kontrola przep³ywu (USART_HardwareFlowControl_None, USART_HardwareFlowControl_RTS, USART_HardwareFlowControl_CTS, USART_HardwareFlowControl_RTS_CTS)
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	// tryb nadawania/odbierania (USART_Mode_Rx, USART_Mode_Rx )
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	// konfiguracja
	USART_Init(USART3, &USART_InitStructure);
	USART_Init(USART6, &USART_InitStructure);
	// wlaczenie ukladu USART
	USART_Cmd(USART3, ENABLE);
	USART_Cmd(USART6, ENABLE);

	//AT
	send_to_wifi('A');
	send_to_wifi('T');
	//AT+CWMODE=3
	send_to_wifi('A');
	send_to_wifi('T');
	send_to_wifi('+');
	send_to_wifi('C');
	send_to_wifi('W');
	send_to_wifi('M');
	send_to_wifi('O');
	send_to_wifi('D');
	send_to_wifi('E');
	send_to_wifi('=');
	send_to_wifi('3');
	for (;;) {
		//AT+CWMODE?
		send_to_wifi('A');
		send_to_wifi('T');
		send_to_wifi('+');
		send_to_wifi('C');
		send_to_wifi('W');
		send_to_wifi('M');
		send_to_wifi('O');
		send_to_wifi('D');
		send_to_wifi('E');
		send_to_wifi('?');

		send_to_computer(getCharWifi());
	}

}

void send_to_computer(uint16_t letter){
	while (USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET);
		// wyslanie danych
		USART_SendData(USART6, letter);
		// czekaj az dane zostana wyslane
		while (USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET);
}

uint16_t getCharWifi(){
	// czekaj na odebranie danych
    while (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET);

   	return USART_ReceiveData(USART3);
}

void send_to_wifi(uint16_t letter){
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		// wyslanie danych
		USART_SendData(USART3, letter);
		// czekaj az dane zostana wyslane
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
}
