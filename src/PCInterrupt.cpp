/******************************************************************************/
/*                                                                            */
/*           PCInterrupt -- Pin change/external interrupt functions           */
/*                                                                            */
/*                     Copyright (c) 2021  Rick Groome                        */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a    */
/* copy of this software and associated documentation files (the "Software"), */
/* to deal in the Software without restriction, including without limitation  */
/* the rights to use, copy, modify, merge, publish, distribute, sublicense,   */
/* and/or sell copies of the Software, and to permit persons to whom the      */
/* Software is furnished to do so, subject to the following conditions:       */
/*                                                                            */
/* The above copyright notice and this permission notice shall be included in */
/* all copies or substantial portions of the Software.                        */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    */
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        */
/* DEALINGS IN THE SOFTWARE.                                                  */
/*                                                                            */
/*  PROCESSOR:  ATmega32U4       COMPILER: Arduino/GNU C for AVR Vers 1.8.5   */ 
/*  Written by: Rick Groome 2021                                              */ 
/*                                                                            */
/******************************************************************************/

// This module implements a pin change or external interrupt handler and setup
// for same.  It determines which pins changed state and then calls a user 
// defined routine with various bit set, indicating which pins of port B or if 
// the external interrupt changed state.  The user defined function can then 
// use these bits to perform some action. 
// The user routine should then clear the set bits indicating that it has 
// serviced the change. 
// The user should define a function 'PCChangeIntFunc' that will be called 
// when an interrupt occurs, and then enable/disable interrupts via the 
// function 'InitPCInterrupt'.
//

/* 
Revision log: 
  1.0.0    3-2-21    REG   
    Reworked from another file 
    Reworked to use ISR_ALIASOF and weak function instead of a function vector.
*/


#include <arduino.h>
#include "PCInterrupt.h"

static volatile byte LastPINB  = 0;       // Last state of pins

// This is the pins that went low [0] and pins that went high [1]
volatile byte        Changes[2]= {0,0};   

// The following implements a hook into the ISR for the pin change interrupt.  
// If you define 
//   extern "C" void PCChangeIntFunc(byte Changes[])  { <some code> }
// then that function will be called on each pin change interrupt.  If you 
// don't define it, then no code will be generated for it in the ISR. 
extern "C" void __PCChangeISREmpty(byte Changes[]) { }
extern "C" void PCChangeIntFunc(byte Changes[]) __attribute__ ((weak, alias("__PCChangeISREmpty")));


ISR(PCINT0_vect) 
  // This routine called when the interrupt on change occurs on 
  // any of PB1..7.  The routine sets bits in "changes" for each bit that 
  // either transitioned hi or low and has the pin change interrupt enabled.
  // Changes[0] is the bits that just changed to low, Changes[1] are bits that 
  // just went high. 
{
  byte i,NewPINB = (PINB&0x7f)|((PINE&0x40)<<1);
  i = ((LastPINB ^ NewPINB) & PCMSK0);
  Changes[0] |= i & ~NewPINB; 
  Changes[1] |= i & NewPINB; 
  PCChangeIntFunc((byte *)Changes); 
  // NOTE: We could clear bits here instead of in the IntFunc routine.
  LastPINB=NewPINB;            // Save our current state for next time.
}


// Both the Pin change and the external interrupt use the same ISR routine,
// so just point them both to the same routine.  Saves lots of code bytes!!

ISR(INT6_vect, ISR_ALIASOF(PCINT0_vect));
  // This routine called when the external interrupt 6 occurs 


void InitPCInterrupt(byte mask)
  // Initialize the interrupt on pin change interrupt.
{
  if (mask)     // Enable interrupts specified in mask
  {
    byte x; 
    // set Port B pins to input mode w/ pullup
    x=mask&0x7F; DDRB&=~x; PORTB|=x;
    // initialize LastPINB variable
    if (!PCMSK0) LastPINB=(PINB&0x7f)|((PINE&0x40)<<1);
    PCMSK0|=(mask&0x7F);  PCICR=1;
    if (mask & 0x80)
    {
      // set ExtInt.6 to input mode w/ pullup
      DDRE&=~0x40; PORTE|=0x40; 
      EICRB |= 0x10; // sets the interrupt type (any change)
      EIMSK |= 0x40; // activates the interrupt    
    }
  }
  else        // disable interrupts
  { 
    PCICR=PCMSK0=0;
    EIMSK &= ~0x40; 
  }
}

