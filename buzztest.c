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


int main(void){


	//set up timer 2 for square wave output
	OCR2A = 3000; 				
	TCCR2B = 0; 	  				//count at full rate	
	// set to toggle OC2A, clear on match
	TCCR2A = (1<<COM2A0) | (1<<WGM21) ;
	DDRD = (1<<PIND7) ;				//PORT D.7 is OC2A


while(1){}


}
