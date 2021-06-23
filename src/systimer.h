/******************************************************************************/
/*                                                                            */
/*                    SysTimer -- Recurring System Timer                      */
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

// See .cpp file for a description of this module and it's functions.

#ifndef _SYSTIMER_H
#define _SYSTIMER_H

// if defined then include millis/delay and other functions usually in wiring.c
#define SYSTIMERINCLUDESDELAY  1   

#if !SYSTIMERINCLUDESDELAY

void StartSysTimer(unsigned int count, byte divisor);
  // Start a recurring timer interrupt on the "system" Timer (SYSTIMERNO). 
  // Count is the interrupt frequency, divisor is the divisor to use -- 
  // 0=timer off, 1=/1,2=/8,3=/64,4=/256,5=/1024,6=/ext fall,7=/ext rise
  // Interrupt rate is F_OSC/divisor/count. 
  //   Examples (divisor=/4) 100Hz=625, 50Hz=1250, 2Hz=31250
  // If "SysTimerIntFunc" is defined elsewhere it is called during the ISR 
  // routine for this timer. 
  // Can be coded as "StartSysTimer(F_CPU/256/interuptfreq, 4);" 
  // Can also be coded as "StartSysTimer(SYSTIMERSPEED(0.01,2))" (sets timer to 10mS)

void StopSysTimer(void);
  // Stop the timer  (only if we don't include the delay functions)

#else    // SYSTIMERINCLUDESDELAY

/******************************************************************************/
/*       Redesigned wiring.c functions for delay/millis/micros that use       */  
/*                       a different timer (than timer 0)                     */
/******************************************************************************/

// If replacing wiring.c (in default library) these functions are defined 
// in the cpp file

// NOTE: These are coded for Arduino ProMicro/Leonardo with ATMega32u4.  
// They may need to be reworked if using on another processor.

extern unsigned long millis();
// Return the number of milliseconds since we started running.  
// (Rolls over about every 50 days)

extern unsigned long micros(); 
// Return the number of microseconds since we started running. 
// (rolls over about every 70min)

extern void delay(unsigned long ms);
// Delay for the number of ms specified.

extern void init(void); 
// This gets linked in to image and is called before main and replaces the 
// init() in Wiring.c in the default library. 
// You do not need to call it... The arduino sketch 'main' already does so. 
// This version does NOT include all of the normal PWM initialization like 
// the default library wiring.c version does. 

#endif 

// If you define this function, then it will be called as part of the Timer 
// interrupt service routine
extern "C" { extern void SysTimerIntFunc(void); }

/******************************************************************************/
/*               Macros to set the timer speed (count and prescale)           */ 
/******************************************************************************/

// You would normally use the macro SYSTIMERSPEED to set the timer count and 
// prescaler values from the speed you want the timer to run at. 
// "StartSysTimer(SYSTIMERSPEED(0.01,2))"  sets the timer to 10mS

// TIM5PS ... Get the prescaler divisor for P=1..5.  Returns the prescale value..
// This is not normally used by the user of the module
#define TIM5PS(P)       ((P==1)?1:(P==2)?8:(P==3)?64:(P==4)?256:(P==5)?1024:0)

// SYSTIMERCNT ... If the returned CNT>65535 or CNT<3 then error... else return 
//   the count for the F_CPU used with the PS specified
// This is not normally used by the user of the module
#define SYSTIMERCNT(S,PS) ( (1/S)<1?65536:(((F_CPU/TIM5PS(PS)/(1/S))>65535)?65536\
                          :(F_CPU/TIM5PS(PS)/(1/S))<3?65536:(F_CPU/TIM5PS(PS)/(1/S)) ))

// This can be used as the parameters for StartSysTimer... 
// As in StartSysTimer(SYSTIMERSPEED(0.01,2)); 
#define SYSTIMERSPEED(S,P)  SYSTIMERCNT(S,P),P


#endif    // _SYSTIMER_H


