// Color conversion
// color.h
// http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both

#ifndef COLOR_H_
#define COLOR_H_

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))

#include <stdio.h>
#include <stdint.h>

// We use this to macros to tranform from our smaller 127 hue wheel to a more common 360 hue wheel
#define wheel(d) (d*127/360) // transform a from 360 degrees wheel hue value to a 127 degrees hue wheel 
#define wheelTo360(d) (d*360/127) // transform a from 360 degrees wheel hue value to a 127 degrees hue wheel 

typedef struct {
    uint8_t r;       // percent
    uint8_t g;       // percent
    uint8_t b;       // percent
} rgb;

typedef struct {
	// hue needs to have sign to avoid overflows when the difference between one of RGB is negative
    int8_t h;       // angle in degrees
    uint8_t s;       // percent
    uint8_t v;       // percent
} hsv;

hsv rgb2hsv(rgb in);
rgb hsv2rgb(hsv in);

#endif /* COLOR_H_ */