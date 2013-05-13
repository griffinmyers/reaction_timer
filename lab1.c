//Reaction time tester developed by Will Myers and Guo Jie Chin 2012, Cornell University
//ECE4760

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"
#include "lcd_lib.h"

// eeprom define address
#define eeprom_true 0
#define eeprom_data 1

void initialize(void);
void init_lcd(void);
void check_button(void); 

//flash storage for LCD strings
const int8_t LCD_ready[] PROGMEM = "Ready!\0";
const int8_t LCD_cheat[] PROGMEM = "Cheat!\0";
const int8_t LCD_yourtime[] PROGMEM = "Your Time:\0";
const int8_t LCD_besttime[] PROGMEM = "Best Time:\0";
const int8_t LCD_tooslow[] PROGMEM = "Too Slow!\0";

// LCD Buffer is used to display the numerical results
int8_t lcd_buffer[17];

volatile int wait_time;		// value used to count the generated random time [1,2]s
unsigned char state;        // state variable for the main state machine
unsigned char push;   		// 1 if program should register a push, reset to 0 after next task
unsigned char active;		// state variable for button debouncing
unsigned char buzz;			// 1 if buzzer should buzz, 0 otherwise
unsigned char timing;		// 1 if machine is currently timing, 0 otherwise
uint16_t reaction_time;		// current reaction time
uint16_t save_reaction;		// used to capture and save reaction time at earliest moment
uint16_t best_reaction;		// the best reaction time, loaded and written to EEPROM

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

//timer 0 compare ISR
ISR (TIMER0_COMPA_vect)
{    
	check_button();  					// poll button every 1ms
  	if (wait_time>0) --wait_time;		
  	reaction_time++;				
	if(buzz == 1) PORTB = PORTB ^ 0xf0; // Upper 4 bits drive buzzers, lower drive LED
}


int main(void)
{
    initialize();
	init_lcd();
	
	// only initialize EEProm if you have to...
	if (eeprom_read_byte((uint8_t*)eeprom_true) != 'T'){ 
  		eeprom_write_word((uint16_t*)eeprom_data,1000);
		eeprom_write_byte((uint8_t*)eeprom_true,'T');
 	}
	

    while(1)
    {
		// if we are currently timing and the user exceeds 1s, drop into Too Slow state
		if(timing == 1 && reaction_time > 1000) state = 5;

		
		if(state == 0){ 					// the ready state, wont enter state 1
			CopyStringtoLCD(LCD_ready,0,1);	// until button push
			state = 1;
		}
		else if(state == 2){ 				// activate LED/Buzzer begin timing			
			if(wait_time == 0)				// once we've waited random time...
            {
				buzz = 1;					// activate buzzer
                PORTB = 0x00; 				// set all LEDs on
                reaction_time = 0;			// begin counting reaction time every 1ms
                state = 3;					// next state: reaction time
				timing = 1;					// we are timing!
	
            }    
        }  
		else if(state == 5){				// state to enter if user is too slow
				timing = 0;					// stop timing
				state = 4;					// next state: reset cycle
				buzz = 0;					// buzzer off
				PORTB = 0xff; 				// set all LEDs off
				LCDclr();
				CopyStringtoLCD(LCD_tooslow,0,0);
		}    
        
        if(push != 0) //if button pressed...
        {
			switch (state) {
				case 1: // generate wait
					wait_time = (1000 + (rand() % 1001)); //wait time in msec, between 1 and 2 seconds
					state = 2; 				// next state is waiting state
					LCDclr();
					push = 0;  				// turn button off
				  	break;
				case 2: // cheat- shouldn't come back into this loop (push button) until wait_time has expired
					LCDclr();
					CopyStringtoLCD(LCD_cheat,0,1);
					state = 4;				// next state: reset cycle
					push = 0;				// turn button off
					timing = 0;				// turn timing off
					break;
				case 3:	// we got a reaction press! display results & store
					save_reaction = reaction_time;	// first record time
					timing = 0;						// turn timing off

					// update LCD with results!
					LCDclr();
					CopyStringtoLCD(LCD_yourtime,0,1);
					sprintf(lcd_buffer,"%-i",save_reaction);
					LCDGotoXY(11,1);
					LCDstring(lcd_buffer,strlen(lcd_buffer));
					
					// get current best reaction from eeprom
					best_reaction = eeprom_read_word((uint16_t*)eeprom_data);	

					// if we've got a new high score, store it
					if(best_reaction > save_reaction){
						best_reaction = save_reaction;
						eeprom_write_word((uint16_t*)eeprom_data,best_reaction);
					}

					// more LCD results!
					CopyStringtoLCD(LCD_besttime,0,0);
					sprintf(lcd_buffer,"%-i",best_reaction);
					LCDGotoXY(11,0);
					LCDstring(lcd_buffer,strlen(lcd_buffer));
					
					state = 4;			// next state: reset cycle
					push = 0;			// turn button off
					buzz = 0;			// no more buzzing
					PORTB = 0xff; 		// set all LEDs off
					break;
				case 4: // reset cycle
					state = 0;			// next state: ready
					LCDclr();			// clear LCD
					push = 0;			// turn button off
					break;
				default:
					//something went wrong
				break;
			}    
        }   
    }


}
 
void check_button(void) 
{
    // detect A.7 button push and
    // generate the message for task 1

    push = ~PINA & 0x80;

	// if we got a down push before an up push reset push to 0
	if(push != 0 && active == 0){
		push = 0;	
	} 
	// if we got a down push after an up push, deactivate button, push gets sent to main
	else if(push != 0 && active == 1){
		active = 0;
	}
	// if we got an up push and the system was deactivated, reactivate the button
	else if(push == 0 && active == 0){
		active = 1;	
	} 

}

void init_lcd(void){
	LCDinit();
	LCDcursorOFF();
	LCDclr();
}

void initialize(void)
{
    
    //set up the ports
    DDRB=0xff;                      // PORT B is an output
    PORTB=0xff;						// Initialize all LEDs off
    DDRA=0x00;                      // PORT A is an input 
           
    //set up timer 0 for 1 mSec ticks
    TIMSK0 = 2;                     // turn on timer 0 cmp match ISR 
    OCR0A = 250;                    // set the compare reg to 250 time ticks
    TCCR0A = 0b00000010;            // turn on clear-on-match
    TCCR0B = 0b00000011;            // clock prescalar to 64

     
    //init some values
    state = 0;
	active = 1;
	buzz = 0;
	timing = 0;
  
    //init the UART -- uart_init() is in uart.c
    uart_init();
    stdout = stdin = stderr = &uart_str;
      
    //crank up the ISRs
    sei();

}
