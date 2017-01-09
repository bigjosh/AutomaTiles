// Color conversion
// color.h
// http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both

#ifndef BLINK_H_
#define BLINK_H_

#include <stdio.h>
#include <stdint.h>

// This are defined at Arduino.h
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))

void setColor(const uint8_t r, const uint8_t g, const uint8_t b);

void fadeTo(const uint8_t r, const uint8_t g, const uint8_t b, const uint16_t ms);
//void fadeTo(const Color c, uint8_t ms);
//void fadeTo(const Color c, uint8_t ms);

void blink(const uint16_t ms);
//void blink(int ms, Color c); // defaults to on/off of this color
//void blink(int ms, Color[n] c); // send array of colors to blink between
//void blink(int ms, int min, int max); // low and high levels for blinking and the time between them
//void blink(int ms, Color c, int min, int max); // low and high levels for blinking and the time between them

void pulse(const uint16_t ms); // phase
//void pulse(int ms, int min, int max); // phase w/ low and high brightness
//void pulse(int ms, Colors[n] c); // phased pulse between colors (depends on fadeTo)

extern int timer;

void updateLed(void);  // Refreshing

#endif /* BLINK_H_ */