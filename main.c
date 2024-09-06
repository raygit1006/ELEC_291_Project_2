	#include "../Common/Include/stm32l051xx.h"
#include <stdio.h>
#include <stdlib.h>
#include "../Common/Include/serial.h"
#include "UART2.h"

#define SYSCLK 32000000L
#define DEF_F 15000L
#define F_CPU 32000000L

volatile int PWM_Counter = 0;
volatile unsigned char pwm1=255, pwm2=255;

// LQFP32 pinout
//             ----------
//       VDD -|1       32|- VSS
//      PC14 -|2       31|- BOOT0
//      PC15 -|3       30|- PB7
//      NRST -|4       29|- PB6
//      VDDA -|5       28|- PB5
//       PA0 -|6       27|- PB4
//       PA1 -|7       26|- PB3
//       PA2 -|8       25|- PA15 (Used for RXD of UART2, connects to TXD of JDY40)
//       PA3 -|9       24|- PA14 (Used for TXD of UART2, connects to RXD of JDY40)
//       PA4 -|10      23|- PA13 (Used for SET of JDY40)
//       PA5 -|11      22|- PA12
//       PA6 -|12      21|- PA11
//       PA7 -|13      20|- PA10 (Reserved for RXD of UART1)
//       PB0 -|14      19|- PA9  (Reserved for TXD of UART1)
//       PB1 -|15      18|- PA8  (pushbutton)
//       VSS -|16      17|- VDD
//             ----------


// Uses SysTick to delay <us> micro-seconds. 
void wait_1ms(void)
{
	// For SysTick info check the STM32l0xxx Cortex-M0 programming manual.
	SysTick->LOAD = (F_CPU/1000L) - 1;  // set reload register, counter rolls over from zero, hence -1
	SysTick->VAL = 0; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while((SysTick->CTRL & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	SysTick->CTRL = 0x00; // Disable Systick counter
}

void delayms(int len)
{
	while(len--) wait_1ms();
}

void delay(int dly)
{
	while( dly--);
}
void Delay_us(unsigned char us)
{
	// For SysTick info check the STM32L0xxx Cortex-M0 programming manual page 85.
	SysTick->LOAD = (F_CPU/(1000000L/us)) - 1;  // set reload register, counter rolls over from zero, hence -1
	SysTick->VAL = 0; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while((SysTick->CTRL & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	SysTick->CTRL = 0x00; // Disable Systick counter
}

void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) Delay_us(250);
}

void TIM2_Handler(void) 
{
	TIM2->SR &= ~BIT0; // clear update interrupt flag
	PWM_Counter++;
	
	if(pwm1>PWM_Counter)
	{
		GPIOA->ODR |= BIT11;
	}
	else
	{
		GPIOA->ODR &= ~BIT11;
	}
	
	if(pwm2>PWM_Counter)
	{
		GPIOA->ODR |= BIT12;
	}
	else
	{
		GPIOA->ODR &= ~BIT12;
	}
	
	if (PWM_Counter > 255) // THe period is 20ms
	{
		PWM_Counter=0;
		GPIOA->ODR |= (BIT11|BIT12);
	}   
}

void Hardware_Init(void)
{
	GPIOA->OSPEEDR=0xffffffff; // All pins of port A configured for very high speed! Page 201 of RM0451

	RCC->IOPENR |= BIT0; // peripheral clock enable for port A

    GPIOA->MODER = (GPIOA->MODER & ~(BIT27|BIT26)) | BIT26; // Make pin PA13 output (page 200 of RM0451, two bits used to configure: bit0=1, bit1=0))
	GPIOA->ODR |= BIT13; // 'set' pin to 1 is normal operation mode.

	GPIOA->MODER &= ~(BIT16 | BIT17); // Make pin PA8 input
	// Activate pull up for pin PA8:
	GPIOA->PUPDR |= BIT16; 
	GPIOA->PUPDR &= ~(BIT17);
	/////////////////////////////////////////////////////////////////////

}


void PWM_Init(void)
{
	// Set up output pins
	RCC->IOPENR |= BIT0; // peripheral clock enable for port A
    GPIOA->MODER = (GPIOA->MODER & ~(BIT22|BIT23)) | BIT22; // Make pin PA11 output (page 200 of RM0451, two bits used to configure: bit0=1, bit1=0)
	GPIOA->OTYPER &= ~BIT11; // Push-pull
    GPIOA->MODER = (GPIOA->MODER & ~(BIT24|BIT25)) | BIT24; // Make pin PA12 output (page 200 of RM0451, two bits used to configure: bit0=1, bit1=0)
	GPIOA->OTYPER &= ~BIT12; // Push-pull
	
    GPIOA->MODER = (GPIOA->MODER & ~(BIT21|BIT20)) | BIT20; // Make pin PA11 output (page 200 of RM0451, two bits used to configure: bit0=1, bit1=0)
    GPIOA->MODER = (GPIOA->MODER & ~(BIT19|BIT18)) | BIT18; // Make pin PA12 output (page 200 of RM0451, two bits used to configure: bit0=1, bit1=0)

	

	// Set up timer
	RCC->APB1ENR |= BIT0;  // turn on clock for timer2 (UM: page 177)
	TIM2->ARR = F_CPU/DEF_F-1;
	NVIC->ISER[0] |= BIT15; // enable timer 2 interrupts in the NVIC
	TIM2->CR1 |= BIT4;      // Downcounting    
	TIM2->CR1 |= BIT7;      // ARPE enable    
	TIM2->DIER |= BIT0;     // enable update event (reload event) interrupt 
	TIM2->CR1 |= BIT0;      // enable counting    
	
	__enable_irq();
}


#define PIN_PERIOD (GPIOA->IDR&BIT8)

// GetPeriod() seems to work fine for frequencies between 300Hz and 600kHz.
// 'n' is used to measure the time of 'n' periods; this increases accuracy.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	SysTick->LOAD = 0xffffff;  // 24-bit counter set to check for signal present
	SysTick->VAL = 0xffffff; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(SysTick->CTRL & BIT16) return 0;
	}
	SysTick->CTRL = 0x00; // Disable Systick counter

	SysTick->LOAD = 0xffffff;  // 24-bit counter set to check for signal present
	SysTick->VAL = 0xffffff; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(SysTick->CTRL & BIT16) return 0;
	}
	SysTick->CTRL = 0x00; // Disable Systick counter
	
	SysTick->LOAD = 0xffffff;  // 24-bit counter reset
	SysTick->VAL = 0xffffff; // load the SysTick counter to initial value
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(SysTick->CTRL & BIT16) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(SysTick->CTRL & BIT16) return 0;
		}
	}
	SysTick->CTRL = 0x00; // Disable Systick counter

	return 0xffffff-SysTick->VAL;
}



void SendATCommand (char * s)
{
	char buff[40];
	printf("Command: %s", s);
	GPIOA->ODR &= ~(BIT13); // 'set' pin to 0 is 'AT' mode.
	waitms(10);
	eputs2(s);
	egets2(buff, sizeof(buff)-1);
	GPIOA->ODR |= BIT13; // 'set' pin to 1 is normal operation mode.
	waitms(10);
	printf("Response: %s", buff);
}

int main(void)
{
	char buff[80];
    int cnt=0;
    long int count;
	float T, f;
	
	GPIOA->ODR |= BIT10;
	GPIOA->ODR |= BIT9;
	//GPIOA->ODR &= ~BIT10;
	//GPIOA->ODR &= ~BIT9;

	Hardware_Init();
	PWM_Init();
	initUART2(9600);
	
	waitms(1000); // Give putty some time to start.
	printf("\r\nJDY-40 test\r\n");

	// We should select an unique device ID.  The device ID can be a hex
	// number from 0x0000 to 0xFFFF.  In this case is set to 0xABBA
	 SendATCommand("AT+DVIDABBA\r\n");

	// To check configuration
	SendATCommand("AT+VER\r\n");
	SendATCommand("AT+BAUD\r\n");
	SendATCommand("AT+RFID4848\r\n");
	SendATCommand("AT+DVID4848\r\n");
	SendATCommand("AT+RFC\r\n");
	SendATCommand("AT+POWE\r\n");
	SendATCommand("AT+CLSS\r\n");
	
	printf("\r\nPress and hold a push-button attached to PA8 (pin 18) to transmit.\r\n");
	
	cnt=0;
	while(1)
	{
		//if((GPIOA->IDR&BIT8)==0)
		//{
		//	sprintf(buff, " %s\r\n","Shabi");
		//	eputs2(buff);
		//	//printf("Caonima");
		//	waitms(200);
		//}
		if(ReceivedBytes2()>0) // Something has arrived
		{    
			
			egets2(buff, sizeof(buff)-1);
			printf("RXshoudaola: %s", buff);

		}
		//----------------------------------------------------------Decoder
		decoder(buff);
		
		//----------------------------------------------------------frequency
		count=GetPeriod(100);
		if(count>0)
		{
			T=count/(F_CPU*100.0); // Since we have the time of 100 periods, we need to divide by 100
			f=1.0/T;
			printf("f=%.2fHz, count=%d            \r", f, count);
			freq (f);
		}
		else
		{
			printf("NO SIGNAL                     \r");
		}
		fflush(stdout); // GCC printf wants a \n in order to send something.  If \n is not present, we fflush(stdout)
		waitms(200);

	}
}


void freq (float f){
    char fre [80];
    if (f <=60870){
       sprintf(fre, " %c\r\n", 'L');
       eputs2(fre);
       printf(".");
       waitms(200);
    }
    if (f>60870||f<=61000){
       sprintf(fre, " %c\r\n", 'M');
       eputs2(fre);
       printf(".");
       waitms(200);
    }
    if (f>61000){
       sprintf(fre, " %c\r\n", 'N');
       eputs2(fre);
       printf(".");
       waitms(200);
    }
}


void decoder(char* buff){
	if (buff[0]=='A'){		//back
		pwm1=255;
		pwm2=255;
		//GPIOA->ODR &= ~BIT10;
		//GPIOA->ODR &= ~BIT9;
		GPIOA->ODR |= BIT10;
		GPIOA->ODR |= BIT9;
		
	}
	if (buff[0]=='B'){		//slow Straight
		pwm1=100;
		pwm2=100;
		//GPIOA->ODR &= ~BIT10;
		//GPIOA->ODR &= ~BIT9;
		GPIOA->ODR |= BIT10;
		GPIOA->ODR |= BIT9;	
	}
	if (buff[0]=='C'){		//Straight
		pwm1=60;
		pwm2=60;
		//GPIOA->ODR &= ~BIT10;
		//GPIOA->ODR &= ~BIT9;
		GPIOA->ODR |= BIT10;
		GPIOA->ODR |= BIT9;	
	}
	if (buff[0]=='D'){		//slow back
		pwm1=200;
		pwm2=200;
		GPIOA->ODR &= ~BIT10;
		GPIOA->ODR &= ~BIT9;
		//GPIOA->ODR |= BIT10;
		//GPIOA->ODR |= BIT9;	
			
	}
	if (buff[0]=='E'){		//back
		pwm1=255;
		pwm2=255;
		GPIOA->ODR &= ~BIT10;
		GPIOA->ODR &= ~BIT9;
		//GPIOA->ODR |= BIT10;
		//GPIOA->ODR |= BIT9;	
	}
	if (buff[0]=='G'){		//Straight
		pwm1=255;
		pwm2=60;
		//GPIOA->ODR &= ~BIT10;
		GPIOA->ODR &= ~BIT9;
		GPIOA->ODR |= BIT10;
		//GPIOA->ODR |= BIT9;	
	}
	if (buff[0]=='F'){		//Straight
		pwm1=60;
		pwm2=255;
		GPIOA->ODR &= ~BIT10;
		//GPIOA->ODR &= ~BIT9;
		//GPIOA->ODR |= BIT10;
		GPIOA->ODR |= BIT9;		
	}
	
	if (buff[0]=='H'){		//zuoqian
		pwm1=60;
		pwm2=130;
		//GPIOA->ODR &= ~BIT10;
		//GPIOA->ODR &= ~BIT9;
		GPIOA->ODR |= BIT10;
		GPIOA->ODR |= BIT9;		
	}
	if (buff[0]=='I'){		//youqian
		pwm1=120;
		pwm2=60;
		//GPIOA->ODR &= ~BIT10;
		//GPIOA->ODR &= ~BIT9;
		GPIOA->ODR |= BIT10;
		GPIOA->ODR |= BIT9;		
	}
	if (buff[0]=='K'){		//Straight
		pwm1=60;
		pwm2=255;
		GPIOA->ODR &= ~BIT10;
		GPIOA->ODR &= ~BIT9;
		//GPIOA->ODR |= BIT10;
		//GPIOA->ODR |= BIT9;		
	}
	if (buff[0]=='J'){		//Straight
		pwm1=255;
		pwm2=60;
		GPIOA->ODR &= ~BIT10;
		GPIOA->ODR &= ~BIT9;
		//GPIOA->ODR |= BIT10;
		//GPIOA->ODR |= BIT9;		
	}
	
		


}
