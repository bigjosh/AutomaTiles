/*
 * APA102C.h
 *
 * Custom Library for writing values to the APA102C RGB LED
 *
 * Created: 7/21/2015 15:11:37
 *  Author: Joshua
 */ 


#ifndef APA102C_H_
#define APA102C_H_

void sendColor( uint8_t dummy_clk, uint8_t dummy_dat, uint8_t color[3]);

void sendColorShort( uint8_t color[3]);

void sendRGB( uint8_t r, uint8_t g, uint8_t b  );

#endif /* APA102C_H_ */