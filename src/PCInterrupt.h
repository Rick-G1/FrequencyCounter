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

#ifndef _PCINTERRUPT_H
#define _PCINTERRUPT_H

#include <arduino.h>

// Pin numbers for each bit position (e.g. Arduino pin number)
// NOTE: This is for ATMega16/32U4 only.  
//       It will need to be reworked for other processors.
#define PCINTMASK8  0x10  /* PB4  PinChange  Digital pin 8  */ 
#define PCINTMASK9  0x20  /* PB5  PinChange  Digital pin 9  */ 
#define PCINTMASK10 0x40  /* PB6  PinChange  Digital pin 10 */ 
#define PCINTMASK14 0x08  /* PB3  PinChange  Digital pin 14 */ 
#define PCINTMASK15 0x02  /* PB1  PinChange  Digital pin 15 */ 
#define PCINTMASK16 0x04  /* PB2  PinChange  Digital pin 16 */ 
#define PCINTMASK7  0x80  /* PE6  Ext Int 6  Digital pin 7  */ 
//#define PCINTMASKPB0 0x01 /* PB0  PinChange  Digital pin NONE  */ 

extern volatile byte Changes[2];

extern void InitPCInterrupt(byte mask);
  // Initialize the interrupt on pin change interrupt.

// If you define this function, then it will be called as part of the 
// pin change ISR routine.
extern "C" { extern void PCChangeIntFunc(byte Changes[]); }

#endif  //_PCINTERRUPT_H


