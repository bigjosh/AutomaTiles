/*
 * Inits.c
 *
 * Created: 7/15/2015 11:55:59
 *  Author: Joshua
 */ 

#include "Inits.h"


//Set up pin directions, pull up/downs, overrides, pin change interrupts

void initIO(){
	DDRA = POWER; //Set Boost enable pin as output 
	PORTA = 0x00; //No pull ups and POWER is set low // TODO: This is redundant, these are bootup values
	LEDDDR = LEDCLK|LEDDAT; //Set LED signals as outputs
	PORTB = 0x00; //No pull ups and IR LED is low    // TODO: This is redundant, these are bootup values
	PCMSK1 = BUTTON; //Mask out only the button for the pin change interrupt
	PCMSK0 = 0x3F; //Mask out only the phototransistors for the pin change interrupt
	GIMSK = (1<<PCIE1)|(1<<PCIE0);//Enable Pin change interrupt for the button and Pin change interrupts for the phototransistors
	PRR = /*(1<<PRTIM1)|*/ (1<<PRUSI);//Disable unused modules (JML: We are using Timer1 for the moment as a stopwatch)
    DIDR0 |= MIC;           // Disable digital input on the MIC pin to save power since it will be floating.     
}


void setDir(uint8_t dir){//make tile only listen to one phototransistor
	PCMSK0 = 1<<dir;
}

void setDirNone(){
	PCMSK0 = 0;
}

void setDirAll(){//make tile listen to all phototransistors
	PCMSK0 = 0x3F;
}

void initTimer0(){//Set up global .1ms timer used for various protocols
	TCCR0A = (0<<COM0A1)|(0<<COM0A0)//OC0A Disconnected
		|(0<<COM0B1)|(0<<COM0B0)//OC0B Disconnected
		|(1<<WGM01)|(0<<WGM00);//Clear timer on compare match
	TCCR0B = (0<<WGM02)
		|(0<<CS02)|(1<<CS01)|(0<<CS00);//ClkIO/8 (125kHz)
	OCR0A = 125; //125 cycles at 125kHZ = 1ms
	TIMSK0 = (0<<OCIE0B)|(1<<OCIE0A)|(0<<TOIE0);//Only enable OC Match A interrupt
}

void initTimer1(){          // For now use for timing pulses,  runs 1Mhz
    TCCR1B = 0b00000010;    // Clk/8   - This give us 125 ticks per ms, which should be a nice balance and we only need the bottom byte
}

