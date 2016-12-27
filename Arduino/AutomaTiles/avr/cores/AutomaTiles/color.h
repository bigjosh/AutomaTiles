// Color conversion
// color.h
// http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both

#ifndef COLOR_H_
#define COLOR_H_

#include <stdint.h>

typedef struct {
    double r;       // percent
    double g;       // percent
    double b;       // percent
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // percent
    double v;       // percent
} hsv;

static hsv rgb2hsv(rgb in);
static rgb hsv2rgb(hsv in);

void interpolateRGBColor(uint8_t *result, uint8_t from[3], uint8_t to[3], float percent);
void getRGBfromHSV(uint8_t *result, hsv hsvColor);
hsv getHSVfromRGB(uint8_t rgbColor[3]);

#endif /* COLOR_H_ */
