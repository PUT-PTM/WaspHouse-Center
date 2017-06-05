// PATTERN: https://github.com/sapher/stm32-dht11_driver

#include "dht11.h"

int DHT11_init(struct DHT11_Dev* dev, GPIO_TypeDef* port, uint16_t pin) {
	TIM_TimeBaseInitTypeDef TIM_TimBaseStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	dev->port = port;
	dev->pin = pin;

	//Init TIMER2
	TIM_TimBaseStructure.TIM_Period = 84000000 - 1;
	TIM_TimBaseStructure.TIM_Prescaler = 84;
	TIM_TimBaseStructure.TIM_ClockDivision = 0;
	TIM_TimBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimBaseStructure);
	TIM_Cmd(TIM2, ENABLE);

	//Init GPIO DHT11
	GPIO_InitStructure.GPIO_Pin = dev->pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(dev->port, &GPIO_InitStructure);

	return 0;
}

int DHT11_read(struct DHT11_Dev* dev) {

	//Init all
	uint8_t i, j, temp;
	uint8_t data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
	GPIO_InitTypeDef GPIO_InitStructure;

	//!!! First state !!!
	GPIO_InitStructure.GPIO_Pin = dev->pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(dev->port, &GPIO_InitStructure);

	//LOW - min. 18ms
	GPIO_ResetBits(dev->port, dev->pin);

	//waiting - 18ms
	TIM2->CNT = 0;
	while((TIM2->CNT) <= 18000);

	//HIGH - 20-40us
	GPIO_SetBits(dev->port, dev->pin);

	//wait 40us
	TIM2->CNT = 0;
	while((TIM2->CNT) <= 40);
	//!!! End first state !!!

	//Receiving data
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(dev->port, &GPIO_InitStructure);

	TIM2->CNT = 0;
	while(!GPIO_ReadInputDataBit(dev->port, dev->pin)) {
		if(TIM2->CNT > 100)
			return DHT11_ERROR_TIMEOUT;
	}

	TIM2->CNT = 0;
	while(GPIO_ReadInputDataBit(dev->port, dev->pin)) {
		if(TIM2->CNT > 100)
			return DHT11_ERROR_TIMEOUT;
	}

	//Reading 40 bits (8*5)
	for(j = 0; j < 5; ++j) {
		for(i = 0; i < 8; ++i) {

			//LOW - 50us
			while(!GPIO_ReadInputDataBit(dev->port, dev->pin));

			//counting
			TIM_SetCounter(TIM2, 0);

			//HIGH - 26-28us
			while(GPIO_ReadInputDataBit(dev->port, dev->pin));

			//Calculating
			temp = TIM_GetCounter(TIM2);

			data[j] = data[j] << 1;

			if(temp > 40)
				data[j] = data[j]+1;
		}
	}

	//check checksum
	if(data[4] != (data[0] + data[2]))
		return DHT11_ERROR_CHECKSUM;

	//set data
	dev->temparature = data[2];
	dev->humidity = data[0];

	return DHT11_SUCCESS;
}
