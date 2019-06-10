/* Includes */
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"

#define DELAY 48000000

#define RED GPIO_Pin_14 	// PORT D
#define GREEN GPIO_Pin_12	// PORT D
#define BLUE GPIO_Pin_15	// PORT D
#define ORANGE GPIO_Pin_13	// PORT D

volatile int parking_space = 30;
volatile char sparking_space[50];
volatile int counter = 0;

#define BUTTON GPIO_Pin_0	// PORT A
//#define BUTTON2 GPIO_Pin_1;

GPIO_InitTypeDef GPIO_LED;
GPIO_InitTypeDef GPIO_BUTTON;

// Pattern: R – RG – RGB – GB – B – RB
enum Patt {IDLE, WAIT_PASSWORD, RIGHT_PASS, WRONG_PASS,STOP, NONE};
int state = NONE;

volatile char isr_flag_0 = 0;
volatile char password_1 = 0;
volatile char password_2 = 0;
//volatile char loop ;

void InitializeIO_2(){

	//Enable the clock on GPIO D
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_LED.GPIO_Pin = GREEN | RED | BLUE | ORANGE; //PORT D?
	GPIO_LED.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_LED.GPIO_OType = GPIO_OType_PP; //push/pull mode
	GPIO_LED.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOD, &GPIO_LED);

	// Turn Off LEDs
	GPIO_WriteBit(GPIOD, RED | GREEN | BLUE| ORANGE, Bit_RESET);

	//Enable the clock on GPIO A
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_BUTTON.GPIO_Pin = BUTTON;
	GPIO_BUTTON.GPIO_Mode = GPIO_Mode_IN;
	GPIO_BUTTON.GPIO_OType = GPIO_OType_PP; //push/pull mode
	GPIO_BUTTON.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_BUTTON);


}

void configure_PA0(void){

//	GPIO_InitTypeDef GPIO_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

//	Enable clock for SysCFG
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

//	tell system you are using PA0 for exti_line0
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

	EXTI_InitStruct.EXTI_Line = EXTI_Line0;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;

	EXTI_Init(&EXTI_InitStruct);

//	Add IRQ vector to NVIC

	NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;

	NVIC_Init(&NVIC_InitStruct);

}

//set interrupt handler
//handle PA0 interrupt
void EXTI0_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line0) != RESET){

		isr_flag_0 = 1;


		//clear flag
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

void config_UART()
{
	GPIO_InitTypeDef GPIO_Struct;
	USART_InitTypeDef UART_Struct;

	// Enable clock for GPIOA
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// Enable clock for USART 2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// Connect PA2 to USART2_TX
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	// Connect PA3 to USART2_RX
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	// Initialize GPIOA
	GPIO_Struct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Struct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Struct.GPIO_OType = GPIO_OType_PP;
	GPIO_Struct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Struct.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOA, &GPIO_Struct);

	// Initialize UART
	UART_Struct.USART_BaudRate = 9600;
	UART_Struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	UART_Struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	UART_Struct.USART_Parity = USART_Parity_No;
	UART_Struct.USART_StopBits = USART_StopBits_1;
	UART_Struct.USART_WordLength = USART_WordLength_8b;

	USART_Init(USART2, &UART_Struct);

	// Enable USART 2
	USART_Cmd(USART2, ENABLE);
}

void USART_PutChar(char c)
{
	 while( !USART_GetFlagStatus(USART2, USART_FLAG_TXE) );
	 USART_SendData(USART2, c);
}

void USART_PutString(char *s)
{
	// Send a string
	while(*s)
	{
		USART_PutChar(*s++);
	}
}

//void send_char_data(int c)
//{
//	char s[2];
//	s[0] = c%10;
//	s[1] = c/10;
//	USART_PutString(&s);
//}


int main(void){
	int i;


	// Initialize GPIO
	//*******************
	InitializeIO_2();

	configure_PA0();

	config_UART();

		// Send "Hello World!" to PC


	while(1){

		if( isr_flag_0 == 1 ){

			 //Reset flag
			isr_flag_0 = 0;
			password_1 = 1;
			password_2 = 1;
//			for(i=0; i<1800000; i++); // Poor Implementation of a Software DeBounce
			state = IDLE;
			GPIO_WriteBit(GPIOD, RED | GREEN | BLUE | ORANGE, Bit_RESET);
		}

		// Pattern: R – RG – RGB – GB – B – RB
		switch(state){
			case IDLE:
				USART_PutString("WELCOME!\n");
				GPIO_WriteBit(GPIOD, RED, Bit_SET);
//				GPIO_WriteBit(GPIOD, GREEN | BLUE | ORANGE, Bit_RESET);
				state = WAIT_PASSWORD;
				break;

			case WAIT_PASSWORD:
//				password_1 = 1;
//				password_2 = 1;
				GPIO_WriteBit(GPIOD, ORANGE, Bit_SET);
				GPIO_WriteBit(GPIOD, RED | GREEN | BLUE, Bit_RESET);
				USART_PutString("Checking Authorization!\n");
				if((password_1== 1)&&(password_2== 1))
					{
					state = RIGHT_PASS;
					}
				else
				{
					state = WRONG_PASS;
				}
				break;


			case RIGHT_PASS:


//				GPIO_WriteBit(GPIOD, RED | GREEN | BLUE, Bit_SET);
				GPIO_WriteBit(GPIOD, RED | BLUE | ORANGE, Bit_RESET);
				GPIO_WriteBit(GPIOD, GREEN, Bit_SET);
				USART_PutString("Approved\n");
				counter = counter +1;

				state = STOP;
				break;

			case WRONG_PASS:
				if((password_1== 1)&&(password_2== 1)){
					state = RIGHT_PASS;
//					GPIO_WriteBit(GPIOD, RED | GREEN , Bit_RESET);
//					GPIO_WriteBit(GPIOD, GREEN, Bit_SET);
				}
				else{

					GPIO_WriteBit(GPIOD, BLUE, Bit_SET);
					GPIO_WriteBit(GPIOD, RED | GREEN| ORANGE , Bit_RESET);
					USART_PutString("Restricted!\n");
					state = STOP;
				}
				break;

			case STOP:

				parking_space = parking_space - counter;
				GPIO_WriteBit(GPIOD, RED, Bit_SET);
				GPIO_WriteBit(GPIOD, ORANGE| BLUE | GREEN , Bit_RESET);
				sprintf(sparking_space, "%i\n\r",parking_space);
				USART_PutString(sparking_space);
				state = NONE;

				break;



			default:
				GPIO_WriteBit(GPIOD, RED | GREEN | BLUE| ORANGE, Bit_RESET);
				break;
		}
		for(i=0; i<DELAY; i++);
	}

}

/*
 * Callback used by stm32f4_discovery_audio_codec.c.
 * Refer to stm32f4_discovery_audio_codec.h for more info.
 */
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size){
  /* TODO, implement your code here */
  return;
}

/*
 * Callback used by stm324xg_eval_audio_codec.c.
 * Refer to stm324xg_eval_audio_codec.h for more info.
 */
uint16_t EVAL_AUDIO_GetSampleCallBack(void){
  /* TODO, implement your code here */
  return -1;
}
