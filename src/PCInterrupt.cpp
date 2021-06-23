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

/* 
   
  This module/class implements a pin change or external interrupt handler and 
  the setup for same.  It determines which pins have changed state and then 
  sets bits in a variable that can either be checked via polled logic in some 
  mainline program or via a user written function that is called during the 
  actual pin change interrupt routine.  The pins include port B, bits 0 to 6, 
  and external interrupt 6. 
  
  When using in the polled mode, three functions 'rising','falling' and 
  'change', can be used in mainline code to determine if the pin just 
  changed state to a 1 (rising), a 0 (falling), or any change (change). 

  If the user defines a function named 'PCChangeIntFunc', then that function 
  will be called when a pin change interrupt occurs.  When using this user 
  function that is called during the interrupt, a variable is passed to the 
  function that can be examined to indicate which bits have changed state, 
  or the function can also use the rising/falling/change methods as mentioned 
  previously to determine which bits have changed. 
  
  In either case, when the user code has acted on the changed pins it should 
  then call the member function 'clear' to clear the bits in the modules 
  "changed bits" variable so it will be ready for the next time. 

  Pin change interupts are enabled/disabled via the enable and disable methods.

  In all cases where a 'mask' variable is used, the user should use the 
  'PCINTMASKxx' constants found in the header file.  You can enable/disable 
  multiple pin change bits by OR'ing the mask constants to the enable/disable 
  functions as in  "PCH.enable(PCINTMASK7|PCINTMASK8|PCINTMASK9)".  This same 
  OR'ing technique can be used in the other functions also to check for or 
  clear multiple pins.

  The class 'PCH' is instantised in this module.  To use the member functions, 
  use PCH as the variable as in PCH.rising(PCINTMASK8).  Do not instantise 
  another variable of this class as this is a hardware based class.

*/

/* 
Revision log: 
  1.0.0    3-2-21    REG   
    Reworked from another file 
    Reworked to use ISR_ALIASOF and weak function instead of a function vector.
  1.0.1    6-20-21   REG   
    Reworked to use a class interface and added disable and change functions

*/

#include <arduino.h>
#include "pcinterrupt.h"

// This is the last state of the PC change pins
volatile byte LastPINB = 0;
// This is the pins that went low(falling) [0] and pins that went high(rising) [1]
volatile byte Changes[2]= {0,0};   

// This is the one (and only) instance of this class
// if you don't define it here, you can call it whatever name you like
// PCInterrupt PCH;   

// The following implements a hook into the ISR for the pin change interrupt.  
// If you define 
//   extern "C" void PCChangeIntFunc(byte Changes[])  { <some code> }
// then that function will be called on each pin change interrupt.  If you 
// don't define it, then no code will be generated for it in the ISR. 
extern "C" void __PCChangeISREmpty(byte Changes[] __attribute__((unused))) { }
extern "C" void PCChangeIntFunc(byte Changes[]) __attribute__ ((weak, alias("__PCChangeISREmpty")));

ISR(PCINT0_vect) 
{
  // This routine called when the interrupt on change occurs on 
  // any of PB0..7.  The routine checks for each bit that changed,
  // and if it's now zero (falling) then call the routine specified.
  byte i, NewPINB;

  NewPINB = (PINB&0x7f)|((PINE&0x40)<<1); // Read the IO ports
  // i= changes to the bits that are enabled
  i = ((LastPINB ^ NewPINB) & ((PCMSK0 &0x7f)|((EIMSK<<1)&0x80)));
  Changes[0] |= i & ~NewPINB;             // pins that just went low
  Changes[1]  |= i & NewPINB;             // pins that just went high
  PCChangeIntFunc((byte *)Changes);
  LastPINB=NewPINB;                       // Save  current state for next time.
}

// Both the pin change and the external interrupt use the same ISR routine,
//   so just point them both to the same routine.  Saves lots of code bytes!!
ISR(INT6_vect, ISR_ALIASOF(PCINT0_vect));
  // This routine called when the external interrupt 6 occurs 

// These three functions return true if the pin change event occured. 
boolean PCInterrupt::falling(byte mask)  { return(Changes[0] & mask); } 
boolean PCInterrupt::rising(byte mask)   { return(Changes[1] & mask); } 
boolean PCInterrupt::change(byte mask)   { return((Changes[0]|Changes[1]) & mask); } 

void    PCInterrupt::clear(byte mask)
  // Clear the bits specified by 'mask' in the Changes array.
{
  noInterrupts();
  mask=~mask; Changes[0]&=mask; Changes[1]&=mask;
  interrupts();
}

void    PCInterrupt::enable(byte mask)   
  // Enable the interrupt on pin change interrupt for the pins specified in 'mask'.
  // (Set selected pin change pins to input mode with pull up)
{ 
  if (mask)       // Enable interrupts specified in mask 
  {
    byte x;
    // set Port B pins to input mode w/ pullup
    x=mask&0x7F; DDRB&=~x; PORTB|=x;
    if (mask & 0x80)
    {
      // set ExtInt.6 to input mode w/ pullup
      DDRE&=~0x40; PORTE|=0x40; 
      EICRB |= 0x10; // sets the interrupt type (any change)
      EIMSK |= 0x40; // activates the interrupt    
    }
    // initialize LastPINB variable
    LastPINB=(PINB&0x7f)|((PINE&0x40)<<1);
    // setup the Pin change hardware
    if (x) { PCMSK0|=x;  PCICR=1; }
    interrupts();
  } 
}

void    PCInterrupt::disable(byte mask)  
  // Disable the interrupt on pin change interrupt for the pins specified in 'mask'.
  // (Do not alter the direction of the pin change IO pins [leave as input w/PUP])
{ 
  if (mask)       // Disable interrupts specified in mask 
  {
    if (mask&0x7F) { PCMSK0&=(mask&0x7f); if (!PCMSK0) PCICR=0; }
    if (mask&0x80) EIMSK &= ~((mask & 0x80)>>1); 
    Changes[0]&=~mask; Changes[1]&=~mask; 
  }
}


