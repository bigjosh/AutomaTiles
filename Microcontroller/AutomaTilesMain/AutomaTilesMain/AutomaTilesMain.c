/*
 * AutomaTilesMain.c
 *
 * Created: 7/14/2015 15:53:13
 *  Author: Joshua
 */ 


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <avr/sleep.h>

#include "Pins.h"
#include "Inits.h"
#include "APA102C.h"

#include <avr/delay.h>

volatile static int16_t holdoff = 2000;//for temporarily preventing click outputs
volatile static uint8_t click = 0;//becomes non-zero when a click is detected
volatile static uint8_t sync = 0;//becomes non-zero when synchronization pulses need to be sent out
volatile static uint8_t state = 0;//current state of tile
volatile static uint32_t timer = 0;//.1 ms timer tick
volatile static uint32_t times[6][4];//ring buffer for holding leading  detection edge times for the phototransistors
volatile static uint8_t timeBuf[6];//ring buffer indices

const uint32_t TIMEOUT = 20;
volatile static uint32_t sleepTimer = 0;
volatile static uint32_t powerDownTimer = 0;
volatile static uint8_t wake = 0;

const uint8_t PULSE_WIDTH = 8;//1/2 bit of manchester encoding, time in ms
volatile static uint8_t progDir = 0;//direction to pay attention to during programming. Set to whichever side put the module into program mode.
volatile static uint8_t comBuf[64];//buffer for holding communicated messages when programming rules (oversized)
volatile static uint16_t bitsRcvd = 0;//tracking number of bits received for retransmission/avoiding overflow
static uint8_t seqNum = 0;//Sequence number used to prevent circular retransmission of data

// init to the default rulestate for automatiles
volatile static uint8_t birthRules[7] = {0,0,1,1,0,0,0}; // if true, should be born
volatile static uint8_t deathRules[7] = {1,1,0,0,1,1,1}; // if true, should die (gotta find a better metaphor, this is too sad)
volatile static uint8_t numStates = 2; // sets the age of an automatile
volatile static uint8_t soundEn = 1; //if true, react to sound

uint8_t colors[][3] = //index corresponds to state
{
	{0x55,0x00,0x00},       // RED=0
	{0x00,0x55,0xAA},
	{0x7F,0x7F,0x00},       // YELLOW=2
	{0xAA,0x55,0x00},
	{0xFF,0x00,0x00},
	{0xAA,0x00,0x55},
	{0x7F,0x00,0x7F},
	{0x55,0x00,0xAA},
	{0x00,0x00,0xFF},       // BLUE=8
	{0x00,0x55,0xAA},      
	{0x00,0x7F,0x7F},
	{0x00,0xAA,0x55},		
	{0x00,0xFF,0x00},      // GREEN=12
	{0x55,0xAA,0x00}
};

uint8_t color_black[] = {0,0,0};

#define COLOR_RED (colors[0])
#define COLOR_YELLOW (colors[2])
#define COLOR_GREEN (colors[12])
#define COLOR_BLUE (colors[8])

#define COLOR_BLACK (color_black)

const uint8_t CYCLE_TIME = 32;
uint8_t cycleTimer = 0;
uint32_t lastTime = 0;
const uint8_t breatheCycle[] = 
{64, 72, 81, 90, 100, 110, 121, 132,
144, 156, 169, 182, 196, 211, 225, 241,
255, 241, 225, 211, 196, 182, 169, 156,
144, 132, 121, 110, 100, 90, 81, 72};
/*{64, 68, 72, 76, 81, 85, 90, 95, 100, 105, 110, 116, 121, 127, 132,	138, 
144, 150, 156, 163, 169, 176, 182, 189, 196, 203, 211, 218, 225, 233, 241, 249,
255, 249, 241, 233, 225, 218, 211, 203, 196, 189, 182, 176, 169, 163, 156, 150,
144, 138, 132, 127, 121, 116, 110, 105, 100, 95, 90, 85, 81, 76, 72, 68};*/

const uint8_t dark[3] = {0x00, 0x00, 0x00};
const uint8_t recieveColor[3] = {0x00, 0x7F, 0x00};
const uint8_t transmitColor[3] = {0xAA, 0x55, 0x00};
	
uint8_t bright(uint8_t val){
	if(val >= 127){
		return 255;
	}
	if(val == 0){
		return 4;
	}
	return val<<1;
}

enum MODE
{
	sleep,
	running,
	recieving,
	transmitting
} mode;
	
static void getStates(uint8_t * result);
static void parseBuffer();





/////// <Test mode code>

// Search for a 2ms sqaure 
// use pixel to indicate status
// BLACK: No signal at all (below detection threshold)
// GREEN: Receiving a good square
// BLUE : Runt received (on shorter than 1ms)
// RED  : Jumbo received (on longer than 1ms, probably saturated detector)

// REMEBER: Phototransistors pull LOW when the see light

#define MIN_WIDTH_US (1750U) 
#define MAX_WIDTH_US (2250U)

// Compare the time to a value
#define TEST_TIMER_GT(us)  ( ((uint8_t) TIMER1_VAL) > ( ((uint8_t) us)/8U)  )       // macro because of all the unsigned casts


// Map the faces 1-6 (represented by blink flashes) to the PORT bit they are each connected to

const uint8_t modeToPin[] = {
    0,          // TX
    _BV(PA1),  
    _BV(PA2),
    _BV(PA5),
    _BV(PA4),
    _BV(PA3),
    _BV(PA0),
    };

void testMode(void) {

    // wait for a low to sync on

    /*    
    _delay_us(200);
    
    if (TEST_TIMER_GT(1000)) {
        sendColorShort(COLOR_GREEN);
    } else {
        sendColorShort(COLOR_RED);    
    }
    return;        
    
    */
        
    cli();
    uint8_t mode=0;     // Mode is which LED we are looking at , or 0=transmit
    uint8_t state=0;
    
    static uint8_t xmitonoff = 0;

    
    while (1) {
        
        if (BUTTON_DOWN()) {
            
            sendColorShort(COLOR_YELLOW);
            
            _delay_ms(50);  // Debounce
            
            while (BUTTON_DOWN());      // Wait for release
            
            sendColorShort(COLOR_BLACK);
            _delay_ms(500);
            
            mode++;
            
            if (mode==7) {
                
                mode = 0;           // Back to xmit mode
                xmitonoff = 1;      // Quick visual feedback....
                //sendColorShort(COLOR_GREEN);        // Indicate (and give them a sec to let go of button)
                //_delay_ms(1000);    
                            
            } else {
                
                // Note that we can't get here if theIR was on because it is shorted to the button in this design!
                //irOff();      // Are we switching from xmis mode? Either to act than think. 
                
                // Indicate which LED we are watching
                
                for(uint8_t i=0; i< mode; i++ ) {
                    sendColorShort(COLOR_BLUE);
                    _delay_ms(500);
                    sendColorShort(COLOR_BLACK);
                    _delay_ms(500);
                }
                                
            }
            
        }                                    
            
        if (mode==0) { // Xmit mode!
                        
            if (!xmitonoff) {                // This is hoops becase we can not detect a button push while ir is on becuase IR and Button are shortted on board
                irOn();
                sendColorShort(COLOR_GREEN);
                _delay_ms(200);
                irOff();                
                sendColorShort(COLOR_BLACK);
            } else {
                _delay_ms(100);
            }
            
            xmitonoff++;
            
            if (xmitonoff>=8) xmitonoff=0;
            
        } else {
            
            uint8_t photomask = modeToPin[mode];
            
        
            uint8_t newState = (PINA & photomask);            //1=SIGNAL, 0=Dark
        
            if (newState!=state) {
                        
                state=newState;
            
                if (state) {
                    sendColorShort(COLOR_RED);
                } else {
                    sendColorShort(COLOR_BLACK);
                }
            
                _delay_ms(100);         // Make brief states visible
            
            }
        
        
        
         }        
     }     
     /*

        sendColorShort( COLOR_BLACK);     // Start with pixel off while we search
                
        // INitial pulse is special - We don't check if it is too short because we might have jumpped in in the middle...
                   
                   while (1)  {
        TIMER1_CLR;
        irOn();                       
        while (!TEST_TIMER_GT(2000));
        TIMER1_CLR;
        irOff();        
        while (!TEST_TIMER_GT(500));
        
                   }        
        
        while ((!(PINA & PHOTO4)) && !TEST_TIMER_GT(MAX_WIDTH_US) );     // While signal ON, but don't wait too long
    
        if ( TEST_TIMER_GT(MAX_WIDTH_US) ) {            // ON too long
            sendColorShort(COLOR_YELLOW);
            return;
        }
        
            sendColorShort(COLOR_BLUE);
            
            return;
        
        

        // Ok, we are low now waiting for a pos edge - LED is black

        while ( (PINA & PHOTO4) );       // Wait for an up edge (we can wait forever, we are black now

        // Bingo! We have a pos edge - start the stopwatch!
    
        TIMER1_CLR;         
        while ((!(PINA & PHOTO4)) && !TEST_TIMER_GT(MAX_WIDTH_US) );     // While signal ON, but don't wait too long
    
        if ( TEST_TIMER_GT(MAX_WIDTH_US) ) {            // ON too long
            sendColorShort(COLOR_RED);
            return;
        }
        
        if ( !TEST_TIMER_GT( MIN_WIDTH_US) ) {          // On too short
            sendColorShort(COLOR_BLUE);
            return;        
        }
        
        //Ok, pulse looks good - show fleeting aproval!
        
        sendColorShort(COLOR_GREEN);
        
        // Loop back and look for the next weel formed pulse!
        
        
    }        
    
    */
    /*
    uint8_t runtflag=0;
    uint8_t jumboflag=0;
    uint8_t goldieflag=0;
   
          
    while (1) {
        
        
        
        while (countdown && !(PINA & PHOTO4));  // wait for some signal to get us started. No signal at all is no error, so shows as black. 
        
        if (!0)
            
                    sendRGB(0,0,0);                    
                    // Let black go away quickly
                }
                
            }                
            
        for( uint8_t i=0; i<3 ;i++) {       // take 3 samples at 500ms  intervals
            
            if (!PINA & PHOTO4)) {          // This should be high!
                sendRGB(255,0,0);           // Indicate runt
                currentColorCountdown=255
            }
            
            
        _delay_ms(1);           // This should put us square in the middle of the pulse 
            
        };       
        
        // Ok, if we get here then we have a signal detected        
        // We don't know where in the timer cycle we are, but we know that a 2ms pulse must be sampled 2 frames in a row. 
        
        
        
        countdown=6;
        
        uint8_t pulseflag=0;
        
        while( countdown && PINA & PHOTO4) {
            pulseflag=1;                            // Did we even see anything?
        }            
        
        
        if (!pulseflag) {       // We saw nothing at all 
            sendRGB(0,0,0);     // Go black
            
        }
        
        switch (countdown) {
            
            case 6: 
            case 5: sendRGB(255,0,0); break;        // Pulse too short or not at all 
            
            
        }
        
        if (countdown==4 || countdown==3) {
            sendColor(0,255,0);
            } else {
            sendColor(0,0,255);
        }
        
        while( countdown && !(PINA & PHOTO4));

        if (countdown==4 || countdown==3) {
            sendColor(0,255,0);
            } else {
            sendColor(0,0,255);
        }
        
    }
    
    */
    
}

inline void irOn() {        
    PORTB |= IR;            // Set port high first or else will will short if the button happens to be pressed! This will enable the pull-up which will just fight with the pull-down for 1us
    DDRB |= IR;             // We always keep the IR control pin in input mode becuase it happens to be hardwired to the button so we never want to turn it on unessisarily.   
}

inline void irOff() {
    DDRB &= ~ IR;
    PORTB &= ~ IR;    
}


/////// </test mode code>

//volatile uint8_t countdown=0; // USed to time bits. Updated in timer ISR


int main(void)
{
    
    
    /*
    // TODO: Remove! Only for testing!    
    // Turn on power for the boost converter
    
    DDRA|= _BV(PORTA6);        
    //PORTA |= _BV(PORTA6);      // Drive MOFSET gate high, turn off mosfet
        
     PORTA &= ~_BV(PORTA6);      // Drive MOFSET gate low, turn on mosfet (redunant, we can take this out since default state is low)


    // Enable output connected to IR LEDs
    
    DDRB|= _BV(PORTB2);         

    // Do nothing but blink the IR LEDs on and off!!!
        
    while (1) {
                      
        PORTB |= _BV(PORTB2);
        _delay_ms(500);
        PORTB &= ~_BV(PORTB2);
        _delay_ms(500);
        
    }
    
    // </remove>
    
    */
    
    
	//Initialization routines
	initIO();
    
    _delay_ms(100);     // Allow boost to warm up TODO: Is this unnecessary? How long?
    sendColor(LEDCLK,LEDDAT,COLOR_BLUE);

    _delay_ms(1000);

    sendRGB(0,0,0);
        
        // TODO: Why is this INTing even withough sei()? Is there one hitting someplace in a routine?
    //initTimer0();       // Establish 1ms interrupts on TIM0_COMPA_vect)   
                        // Currently this also starts transmitting a 2ms square wave
                        
    //sei();              // Let the ISR actually execute
                        
                        
    //initTimer1();       // Start the timer we use for stopwatching the incoming pulses 

    // BLUE on boot just we know if we get reset (maybe from under voltage or glitch)    
    
    
    while (1) {
        testMode();
        _delay_ms(300);     // Leave LED on long enough to see it...        
        sendColorShort(COLOR_BLACK);
        _delay_ms(300);     // Leave LED on long enough to see it...
        
    }        
       
	sei();
	//initAD();         // Not wanted now there is no mic
	initTimer();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	//Set up timing ring buffers
	for(uint8_t i = 0; i<6; i++){
		timeBuf[i]=0;
	}
	
	mode = running;
    
    
	
    while(1)

    {	
		if(mode==sleep){
			disAD();
			//
			DDRB &= ~IR;//Set direction in
			PORTB &= ~IR;//Set pin tristated
			sendColor(LEDCLK, LEDDAT, dark);
			PORTA |= POWER;//Set LED and Mic power pin high (off)
			wake = 0;
			while(!wake){
				cli();
				uint32_t diff = timer-powerDownTimer;
				sei();
				if(diff>500){
					sleep_cpu();
					wake = 1;
				}
			}
			holdoff=500;
			PORTA &= ~POWER;
			
			uint8_t done = 0;
			cli();
			uint32_t startTime = timer;
			sei();
			while(!done){
				cli();
				uint32_t time = timer;
				sei();
				if(time-startTime>250){
					done=1;
				}
			}
			
			done = 0;
			cli();
			startTime = timer;
			sei();
			DDRB |= IR;//Set direction out
			PORTB |= IR;//Set pin on
			sendColor(LEDCLK, LEDDAT, transmitColor);
			while(!done){
				cli();
				uint32_t time = timer;
				sei();
				if(time-startTime>500){
					done=1;					
				}				
			}
			enAD();
			powerDownTimer = timer;
			sleepTimer = timer;
			mode = running;
		}else if(mode==running){
			if(click){
				uint8_t neighborStates[6];
				getStates(neighborStates);
				uint8_t clickColor[] = {bright(colors[state][0]),bright(colors[state][1]),bright(colors[state][2])}; 
				sendColor(LEDCLK, LEDDAT, clickColor);
				uint8_t numOn = 0;

				sync = 3;//request sync pulse be sent at next possible opportunity (set to 4 for logistical reasons)

				for (uint8_t i = 0; i< 6; i++)
				{
					//at the moments specific state detection is a bit iffy, 
					//so mapping any received state>1 to a state of 1 works for now
					if(neighborStates[i]>0){
						numOn++;
					}	
				}
				//Logic for what to do to the state based on number of on neighbors
				if(state == 0) {
					if(birthRules[numOn]) {
						state=1;
					}
				}
				else {
					if(deathRules[numOn]) {
						state = (state + 1) % numStates;
					}
				}
				//End logic

				//prevent another click from being detected for a bit
				holdoff = 100;
				click = 0;
			}
		
			//periodically update LED once every 0x3F = 64 ms (fast enough to feel responsive)
			cli();
			if(!(timer & 0x3F)){
				if(timer!=lastTime){
					lastTime=timer;
					cycleTimer++;
					cycleTimer %= CYCLE_TIME;
					uint8_t outColor[3] = {(colors[state][0]*breatheCycle[cycleTimer])/255, 
										   (colors[state][1]*breatheCycle[cycleTimer])/255,
										   (colors[state][2]*breatheCycle[cycleTimer])/255};
					sendColor(LEDCLK, LEDDAT, outColor);
				}
			}
			sei();
			//check if we enter sleep mode
			cli();//temporarilly prevent interrupts to protect timer
			if(timer-sleepTimer>1000*TIMEOUT){
				mode = sleep;
			}
			sei();
		}else if(mode==recieving){
			//disable A/D
			disAD();
			//set photo transistor interrupt to only trigger on specific direction
			setDir(progDir);
			//set recieving color
			sendColor(LEDCLK, LEDDAT, recieveColor);	
			//record time entering the mode for timeout
			cli();
			uint32_t modeStart = timer;
			sei();
			while(mode==recieving){//stay in this mode until instructed to leave or timeout
				cli();
				uint32_t diff = timer-modeStart;
				sei();
				if(diff>3000){//been in mode 1 for more than 5 seconds
					mode = transmitting;					
				}
			}
		}else if(mode==transmitting){
			//disable Phototransistor Interrupt
			setDirNone();
			//set LED to output
			DDRB |= IR;//Set direction out
			//send 5 pulses
			uint32_t startTime = timer;
			if(bitsRcvd>=8 && comBuf[0]!=seqNum){
				for(int i=0; i<5; i++){
					while(timer==startTime){
						PORTB &= ~IR;
					}
					startTime = timer;
					while(timer==startTime){
						PORTB |= IR;
					}
					startTime = timer;
				}
				parseBuffer();
			}else{
				bitsRcvd = 0;
			}
			
			startTime = timer;
			sendColor(LEDCLK, LEDDAT, transmitColor);//update color while waiting			
			while(timer<startTime+20);//pause for mode change
			startTime = timer;
			uint16_t timeDiff;
			uint16_t bitNum;
			while(bitsRcvd>0){
				timeDiff = (timer-startTime)/PULSE_WIDTH;
				bitNum = timeDiff/2;
				if(timeDiff%2==0){//first half
					if(comBuf[bitNum/8]&(1<<bitNum%8)){//bit high
						PORTB &=  ~IR;
					}else{//bit low
						PORTB |=  IR;
					}
				}else{//second half
					if(comBuf[bitNum/8]&(1<<bitNum%8)){//bit high
						PORTB |=  IR;
						}else{//bit low
						PORTB &=  ~IR;
					}
				}
				if(bitNum>=bitsRcvd){
					bitsRcvd = 0;
				}				
			}
			//while(timer<startTime+2000);//pause for effect
			
			//done transmitting
			//re-enable A/D
			enAD();
			//re-enable all phototransistors
			setDirAll();
			state = 0;
			mode = running;

		}
	}
}

/* Receives two bytes containing birth rules and death rules
 * Each byte contains 0 or 1 in each position to 
 * represent true or false if born or killed off with n neighbors
 */
static void setRules(uint8_t br, uint8_t dr) {
	birthRules[0] = br&(1<<0);
	birthRules[1] = br&(1<<1);
	birthRules[2] = br&(1<<2);
	birthRules[3] = br&(1<<3);
	birthRules[4] = br&(1<<4);
	birthRules[5] = br&(1<<5);
	birthRules[6] = br&(1<<6);
	
	deathRules[0] = dr&(1<<0);
	deathRules[1] = dr&(1<<1);
	deathRules[2] = dr&(1<<2);
	deathRules[3] = dr&(1<<3);
	deathRules[4] = dr&(1<<4);
	deathRules[5] = dr&(1<<5);
	deathRules[6] = dr&(1<<6);
}

/* Set the number of states each automatile will go through
 * this is effectively an age... i.e. after this many die states, 
 * it will actually die 
 */
static void setNumStates(uint8_t num) {
	numStates = num;
}

static void setColors(volatile uint8_t* buf){
	colors[0][0] = buf[0];
	colors[0][1] = buf[1];
	colors[0][2] = buf[2];
	colors[1][0] = buf[3];
	colors[1][1] = buf[4];
	colors[1][2] = buf[5];
}

/* Uses the current state of the times ring buffer to determine the states of neighboring tiles
 * For each side, to have a non-zero state, a pulse must have been received in the last 100 ms and two of the
 * last three timing spaces must be equal.
 * 
 * State is communicated as a period for the pulses. Differences are calculated between pulses and if a consistent
 * difference is found, that translates directly to a state
 * Accuracy is traded for number of states (i.e. 5 states can be communicated reliably, while 10 with less robustness)
*/
static void getStates(uint8_t * result){
	cli();//Disable interrupts to safely grab consistent timer value
	uint32_t curTime = timer;
	uint32_t diffs[3];
	for(uint8_t i = 0; i < 6; i++){
		if((curTime-times[i][timeBuf[i]])>100){//More than .1 sec since last pulse, too long
			result[i] = 0;
		}else{//received pulses recently
			uint8_t buf = timeBuf[i];//All bit-masking is to ensure the numbers are between 0 and 3
			diffs[0] = times[i][buf] - times[i][(buf-1)&0x03];
			diffs[1] = times[i][(buf-1)&0x03] - times[i][(buf-2)&0x03];
			diffs[2] = times[i][(buf-2)&0x03] - times[i][(buf-3)&0x03];
			if(diffs[0]>100 || diffs[1]>100 || diffs[2] > 100){//Not enough pulses recently
				result[i] = 0;
			}else{//received enough pulses recently
				//rounding 
				diffs[0] >>= 3;
				diffs[1] >>= 3;
				diffs[2] >>= 3;
				//checking if any two of the differences are equal and using a value from the equal pair
				if(diffs[0] == diffs[1] || diffs[0] == diffs[2]){
					result[i] = (uint8_t) diffs[0];
				}else if(diffs[1] == diffs[2]){
					result[i] = (uint8_t) diffs[1];
				}else{//too much variation
					result[i] = 0;
				}
			}
		}
	}	
	sei();//Re-enable interrupts
}

//parse received data buffer and update values
//1 -> next 2 bytes are rules
//2 -> next byte is number of states
//3 -> next 6 bytes are colors
//4 -> disable mic
//5 -> enable mic
static void parseBuffer(){
	seqNum = comBuf[0];
	int i = 1;
	uint8_t numBytes = bitsRcvd/8;
	while(i<numBytes){
		if(comBuf[i]==1){
			if(i+2<numBytes){
				setRules(comBuf[i+1],comBuf[i+2]); 
			}
			i+=3;
		}else if(comBuf[i]==2){
			if(i+1<numBytes){
				setNumStates(comBuf[i+1]);
			}
			i+=2;
		}else if(comBuf[i]==3){
			if(i+6<numBytes){
				setColors(comBuf+i+1);
			}
			i+=7;
		}else if(comBuf[i]==4){
			soundEn = 0;
			i++;
		}else if(comBuf[i]==5){
			soundEn = 1;
			i++;
		}else{
			i++;
		}
	}
}

//Timer interrupt occurs every 1 ms
//Increments timer and controls IR LEDs to keep their timing consistent

// Current code generates a 2ms square on IR LEDS as a test pattern

ISR(TIM0_COMPA_vect){
    
    static uint8_t timerCount=0;
    
    timerCount++;
    
    if (timerCount==4) timerCount=0;
        
    switch( timerCount ) {
        case 0: irOn(); break;
        case 1: break;
        case 2: irOff();
        case 3: break;        
    }
    
    /*
    if (countdown) countdown--;
    
    return;
    */
    
    //PINB |= 1;     // Flick the PB0 line to trigger scope
    //PINB |= 1;     // Flick the PB0 line to trigger scope
    
    return;
    
    
	static uint8_t IRcount = 0;//Tracks cycles for accurate IR LED timing
	static uint8_t sendState = 0;//State currently being sent. only updates on pulse to ensure accurate states are sent
	timer++;
	if(mode==running){
		IRcount++;
		if(IRcount>=(uint8_t)(sendState*8+4)){//State timings are off by 4 from a multiple of 8 to help with detection
			IRcount = 0;
			if(sync==0){
				sendState = state;
			}
		}
		
		if(IRcount==5){ 
			PORTB |= IR;
			DDRB |= IR;
		}else if(IRcount==7&&sync>1){
			PORTB |= IR;
			DDRB |= IR;		
			sync = 1;
		}else if(IRcount==9&&sync==1){
			PORTB |= IR;
			DDRB |= IR;
			sync = 0;
		}else if(sendState==0&&sync>0){//0 case is special
			if((IRcount&0x01)!=0){
				PORTB |= IR;
				DDRB |= IR;			
				sync -= 1;
				}else{
				DDRB &= ~IR;//Set direction in
				PORTB &= ~IR;
			}
		}else{
			DDRB &= ~IR;//Set direction in
			PORTB &= ~IR;//Set pin tristated
		
			if(IRcount<5){
				if(PINB & BUTTON){//Button active high
					if(holdoff==0){
						state = !state;//simple setup for 2 state tile
						sleepTimer = timer;
						powerDownTimer = timer;
					}
					holdoff = 500;//debounce and hold state until released
				}
			}
		}
	}else if(mode == recieving){//recieving data, ensure led off
		DDRB &= ~IR;//Set direction in
		PORTB &= ~IR;//Set pin tristated		
	}
}


//INT0 interrupt triggered when the pushbutton is pressed
ISR(PCINT1_vect){
	wake = 1;
}

//Pin Change 0 interrupt triggered when any of the phototransistors change level
//Checks what pins are newly on and updates their buffers with the current time
ISR(PCINT0_vect){
	static uint8_t prevVals = 0; //stores the previous state so that only what pins are newly on are checked
	static uint8_t pulseCount[6]; //stores counted pulses for various actions
	static uint32_t oldTime = 0; //stored the previous time for data transmission
	uint8_t vals = PINA & 0x3f; //mask out phototransistors
	uint8_t newOn = vals & ~prevVals; //mask out previously on pins
	
	powerDownTimer = timer;
	
	if(mode==running){
		for(uint8_t i = 0; i < 6; i++){
			if(newOn & 1<<i){ //if an element is newly on, 
				if(timer-times[i][timeBuf[i]]<10){//This is a rapid pulse. treat like a click
					pulseCount[i]++;
					if(pulseCount[i]==2){
						if(holdoff==0){
							click = 1;
							wake = 1;
							sleepTimer = timer;
						}
					}
					if(pulseCount[i]>=4){//There have been 4 quick pulses. Enter programming mode.							
						mode = recieving;
						progDir = i;
					}					
				}else{//Normally timed pulse, process normally
					pulseCount[i]=0;
					timeBuf[i]++;
					timeBuf[i] &= 0x03;
					times[i][timeBuf[i]] = timer;
				}
			}
		}
	}else if(mode==recieving){
		if(((prevVals^vals)&(1<<progDir))){//programming pin has changed
			if(timer-oldTime > (3*PULSE_WIDTH)/2){//an edge we care about
				if(timer-oldTime > 4*PULSE_WIDTH){//first bit. use for sync
					bitsRcvd = 0;
					for(int i = 0; i < 64; i++){//zero out buffer
						comBuf[i]=0;
					}					
				}
				oldTime = timer;				
				if(bitsRcvd<64*8){
					uint8_t bit = ((vals&(1<<progDir))>>progDir);
					comBuf[bitsRcvd/8] |= bit<<(bitsRcvd%8);
					bitsRcvd++;
				}				
			}
		}
	}
	prevVals = vals;
}
