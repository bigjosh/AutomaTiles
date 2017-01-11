#include "color.h"
#include <ncurses.h>

int main(int argc, char **argv) {

    initscr();
    noecho();
    curs_set(FALSE);
    //printf("//// Color lib to ints //// \n");
    
    uint8_t red = 16, green = 0, blue = 255;

    rgb in = {red, green, blue};
    hsv out = rgb2hsv(in);
    printf("r = %d, g = %d, b = %d\n", red, green, blue);
    printf("h = %d, s = %d, v = %d\n", out.h*360/127, out.s, out.v);
    rgb rgbRet = hsv2rgb(out);
	printf("r = %d, g = %d, b = %d\n", rgbRet.r, rgbRet.g, rgbRet.b);

    //rgb hsv2rgb();
    // allColorsRGBtoHSVTest();
    // allColorsHSVtoRGBTest();
}
