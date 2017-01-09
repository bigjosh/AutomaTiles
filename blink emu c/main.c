#include "blink.h"
#include "color.h"
#include<time.h>

extern int timer;

void delay(unsigned int mseconds) {
    clock_t goal = mseconds + clock();
    while (goal > clock());
}

int main(int argc, char **argv) {
    timer = clock();
    printf(" Blink Emulator: \n");
    //blink(2000); ok
    // For fadeTo manual tests
    // Positive increments
    //rgb from = {170, 255, 0}; rgb to = {0, 255, 255};  // Hue 80 to 180
    rgb from = {255, 0, 0}; rgb to = {255, 85, 0};  // Hue 300 to 20
    // Negative increments
    //rgb from = {0, 255, 255}; rgb to = {170, 255, 0};  // Hue 180 to 80
    //rgb from = {255, 85, 0}; rgb to = {255, 0, 255};  // Hue 20 to 300
    setColor(from.r, from.g, from.b);
    //pulse(10432);
    fadeTo(to.r, to.g, to.b, 1000);
    while(1){ 
        updateLed();
        delay(30);
    }
}