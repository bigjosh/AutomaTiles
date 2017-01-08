/*
 * Pins.h
 *
 * Created: 7/15/2015 10:59:51
 *  Author: Joshua
 */ 
#include <avr/io.h>

#ifndef PINS_H_
#define PINS_H_
//Phototransistors are mapped out of order for routing purposes
#define PHOTO0 (1<<PA5)
#define PHOTO1 (1<<PA4)     // Q2
#define PHOTO2 (1<<PA3)     // Q3
#define PHOTO3 (1<<PA0)
#define PHOTO4 (1<<PA1)
#define PHOTO5 (1<<PA2)

// JL: These do NOT appear to be correct. 
//      PHOTO2->Q3
//      PHOTO1->Q2


//IR LED output pin
#define IRPORT (PORTB)
#define IRDDR  (DDRB)
#define IR     (1<<PB2)

//LED clock and data pins
#define LEDDDR (DDRB)
#define LEDPORT (PORTB)
#define LEDCLK (1<<PB0)
#define LEDDAT (1<<PB1)

//Microphone pin
#define MIC (1<<PA7)

//Pushbutton pin
#define BUTTON (1<<PB2)

#define BUTTON_DOWN() (PINB&BUTTON)        // Non-inverted - gets tied to Vcc on press

//Power control pin
#define POWER (1<<PA6)

#endif /* PINS_H_ */