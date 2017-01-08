/*
 * APA102C.c
 *
 * Created: 7/21/2015 14:51:49
 *  Author: Joshua
 */ 
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "APA102C.h"

#include "Pins.h"

#define set(port,pin) (port |= pin) // set port pin
#define clear(port,pin) (port &= (~pin)) // clear port pin
#define bit_val(byte,bit) (byte & (1 << bit)) // test for bit set

// TODO: We can make this so much faster with bitshifting

//bit bangs an SPI signal to the specified pins of the given data

inline void sendBit( uint8_t data){
	if(data){
		set(LEDPORT, LEDDAT);
	}else{
		clear(LEDPORT, LEDDAT);
	}
    set(LEDPORT,LEDCLK);
    clear(LEDPORT,LEDCLK);
}

//bit bangs an SPI signal to the specified pins of the given data
void sendByte( uint8_t data){
	sendBit( bit_val(data,7));
	sendBit( bit_val(data,6));
	sendBit( bit_val(data,5));
	sendBit( bit_val(data,4));
	sendBit( bit_val(data,3));
	sendBit( bit_val(data,2));
	sendBit( bit_val(data,1));
	sendBit( bit_val(data,0));
}


void sendRGB( uint8_t r, uint8_t g, uint8_t b  ) {
    
    
    // First send the "Start of frame" to so the pixel is reset and ready to recieve 
	sendByte( 0x00);
	sendByte( 0x00);
	sendByte( 0x00);
	sendByte( 0x00);
    
	//Data
	sendByte(0xE1);//Set brightness to current to minimum TODO: Add setBrightness function (0xE1...0xFF)
	sendByte(b);     // BLUE
	sendByte(g);     // GREEN
	sendByte(r);     // RED    
    
    // Note that no "end of frame is needed for one pixel:
    // https://cpldcpu.com/2014/11/30/understanding-the-apa102-superled/
}    

// TODO: No need to check the portset really
// TODO: No need to disable interrupts since the APA102 is clock based

//bit bangs an SPI signal to the specified pins that generates the specified color 
//	formatted for the APA102, provided as a byte array of R,G,B
void sendColor( uint8_t dummy_clk, uint8_t dummy_dat, uint8_t color[3]) {

	//Disable interrupts
	cli();
	//Start Frame
	sendByte( 0x00);
	sendByte( 0x00);
	sendByte( 0x00);
	sendByte( 0x00);
	//Data
	sendByte( 0xE1);//Set brightness to current to minimum TODO: Add setBrightness function (0xE1...0xFF)
	sendByte( color[2]);     // BLUE
	sendByte( color[1]);     // GREEN
	sendByte( color[0]);     // RED
	//Re-enable interrupts
	sei();
}

void sendColorShort(  uint8_t color[3] ) {
    sendColor( 0 , 0 , color);
}
