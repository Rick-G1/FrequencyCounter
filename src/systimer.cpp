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

// Portions Copyright (c) 2005-2006 David A. Mellis  (see below)

// This module implements either an independent recurring timer that will call 
// the user function specified on each interrupt, or can be used as a "system" 
// timer that calls a user defined function and also implements the delay, 
// millis, micros functions that are normally part of the arduino core.  Using 
// this as a system timer frees up Timer0, that is normally used by the 
// "builtin" functions "delay","millis","micros" and implements them with this 
// timer instead.  
//
// When used as an independent timer, you need to call "StartSysTimer" 
// with a value for the timer counter register (16 bits), a prescale value.  
// You can then call "StopSysTimer" to stop the timer. 
//
// If you define the function "SysTimerIntFunc" then that function will be 
// called as part of the timer interrupt service routine.  This allows a "hook"
// to use the timer for additional functions. 
// The "SysTimerIntFunc" function is the form  "void SysTimerIntFunc(void)"
//
// When used as the system timer, the timer runs at a fixed 1mS (needed for 
// delay,millis,micros) and then calls a user function that is set by 
// "SysTimerIntFunc" (if defined).  
//
// The default is to use this module as an independent timer.  To use as a 
// system timer define "SYSTIMERINCLUDESDELAY" in the header file.
//
// The function "delayMicroseconds" is also included when defining as the system 
// timer so that the default arduino file "wiring.c" is completely replaced.   
// "delayMicroseconds" implements a totally software delay routine.  This code 
// can be removed if not needed to save space.  "delayMicroseconds" is only 
// included so that the wiring.c module doesn't get linked in.
// 
// This code designed for and tested on a ATMega32U4 processor and the Arduino 
// Pro Micro module and for use with either Timer1 or Timer3.  It could 
// probably be used for or reworked for other timers or processors, but that 
// is beyond the scope of this project.  I attempted to maintain some 
// compatibility with the original code but the code is untested on other 
// processors and timers.

/* 
Revision log: 
  0.0.0    3-14-20  REG   
    Initial implementation
    Reworked to use timer3 and add in all the system delay/millis/micros functions
  1.0.0    3-21-21   REG
    Reworked to use either timer 1 or 3 for the system timer. rework of delay.

*/


#include <arduino.h>
#include "systimer.h"

// define (or don't define) this in the header file 
//#dxfine SYSTIMERINCLUDESDELAY  1    // if defined then include millis / delay and other functions ususally in wiring.c

// The following may need to be defined as 0 for some procesors other than 32U4
#define SYSTIMERCOMP   1           // if non-zero use count up autoreloading timer.
                                   // else use timer overflow and reload timer
                                   // Using SYSTIMERCOMP=1 results in a more 
                                   //   accurate timer and delay function

#define SYSTIMERNO     1           // Use this timer as the system timer (1 or 3)

// *****************************************************************************
//   Macros to create tokens for register names / bits for the timers. 
//   These are used in the code so that the timer number used can be changed 
//   just by changing "SYSTIMERNO" to the timer number to use. 
// *****************************************************************************
// PASTETOKENS is used to create token names like TIFR3 from the text "TIFR" and 
// and the SYSTIMERNO number.  This is a two part operation so that the 
// precompiler will actually create a token like "TIFR3" from "TIFR" and "3"
#define PASTER(x,y)       x ## y
#define PASTETOKENS(x,y)  PASTER(x,y)

// System timer is 'a'
#define TCNTa             PASTETOKENS(TCNT,SYSTIMERNO)
#define TCNTaL            PASTETOKENS(TCNT,PASTETOKENS(SYSTIMERNO,L)) 
#define TCNTaH            PASTETOKENS(TCNT,PASTETOKENS(SYSTIMERNO,H)) 
#define TCCRaA            PASTETOKENS(PASTETOKENS(TCCR,SYSTIMERNO),A) 
#define TCCRaB            PASTETOKENS(PASTETOKENS(TCCR,SYSTIMERNO),B) 
#define TCCRa             PASTETOKENS(TCCR,SYSTIMERNO) 
#define WGMa2             PASTETOKENS(PASTETOKENS(WGM,SYSTIMERNO),2)   
#define OCRaA             PASTETOKENS(PASTETOKENS(OCR,SYSTIMERNO),A)   
#define TIMSKa            PASTETOKENS(TIMSK,SYSTIMERNO)
#define OCIEaA            PASTETOKENS(PASTETOKENS(OCIE,SYSTIMERNO),A)   
#define TOIEa             PASTETOKENS(TOIE,SYSTIMERNO)       
#define TIFRa             PASTETOKENS(TIFR,SYSTIMERNO)
#define OCFaA             PASTETOKENS(PASTETOKENS(OCF,SYSTIMERNO),A) 
#define TOVa              PASTETOKENS(TOV,SYSTIMERNO)
#define TIMERa_OVF_vect   PASTETOKENS(PASTETOKENS(TIMER,SYSTIMERNO),_OVF_vect) 
#define TIMERa_COMPA_vect PASTETOKENS(PASTETOKENS(TIMER,SYSTIMERNO),_COMPA_vect) 

// The following implements a hook into the ISR for the system timer.  
// If you define  extern "C" void SysTimerIntFunc(void)  { <some code> }
// then that function will be called on each timer ISR.  If you don't define it, 
// then no code will be generated for it in the interrupt. 
extern "C" void __SysTimerISREmpty(void) { }
extern "C" void SysTimerIntFunc(void) __attribute__ ((weak, alias("__SysTimerISREmpty")));

#if !SYSTIMERCOMP 
static int Timera_counter;         // value to reload timer with (when in overflow mode)
#endif

// NOTE: These are coded for Arduino ProMicro/Leonardo with ATMega32u4.  
// They probably need to be reworked if using on another processor.

#if !SYSTIMERINCLUDESDELAY 
void StartSysTimer(unsigned int count, byte divisor)
#else
static void StartSysTimerLocal(unsigned int count, byte divisor)
#endif
  // Start a recurring timer interrupt on the "system" Timer (SYSTIMERNO). 
  // Count is the interrupt frequency, divisor is the divisor to use -- 
  // 0=timer off, 1=/1,2=/8,3=/64,4=/256,5=/1024,6=/ext fall,7=/ext rise
  // Interrupt rate is F_OSC/divisor/count. 
  //   Examples (divisor=/4) 100Hz=625, 50Hz=1250, 2Hz=31250
  // If "SysTimerIntFunc" is defined elsewhere it is called during the ISR 
  // routine for this timer. 
  // Can be coded as "StartSysTimer(F_CPU/256/interuptfreq, 4);" 
  // Can also be coded as "StartSysTimer(SYSTIMERSPEED(0.01,2))" (sets timer to 10mS)
{
  noInterrupts();                  // disable all interrupts
#if defined(TCCRaB) && defined(CS11) && defined(CS10)
  TCCRaA = 0;  
  TCCRaB = (divisor&7);            // Prescaler.. 0=timer off, 1=/1,2=/8,3=/64,4=/256,5=/1024,6=/ext fall,7=/ext rise
#elif defined(TCCRa) && defined(CS11) && defined(CS10)
  TCCRa  = (divisor&7);            // Prescaler.. 0=timer off, 1=/1,2=/8,3=/64,4=/256,5=/1024,6=/ext fall,7=/ext rise
#endif
#if SYSTIMERCOMP
  // auto reloading timer
  TCNTa = 0;
  // Dont forget to subtract 1 from the count loaded into OCRaA !!
  OCRaA = count-1;
#if defined(TCCRaB) && defined(WGMa2)
  TCCRaB |= (1 << WGMa2);          // CTC mode
#endif
  TIMSKa |= (1 << OCIEaA);         // enable timer compare interrupt
#else
  // Set Timera_counter to the correct value for our interrupt interval
  TCNTa=Timera_counter= -count;   // set our reload value / preload Timer 
  TIMSKa |= (1 << TOIEa);         // enable timer overflow interrupt
#endif
  interrupts();                   // enable all interrupts
}


#if !SYSTIMERINCLUDESDELAY
void StopSysTimer(void)
  // Stop the timer  (only if we don't include the delay functions)
{
  noInterrupts();                 // disable all interrupts
#if defined(TCCRaB) 
  TCCRaB &= ~7;                   // timer off
#elif defined(TCCRa) 
  TCCRa &= ~7;                    // timer off
#endif
#if SYSTIMERCOMP
  TIMSKa &= ~(1 << OCIEaA);       // turn off timer interrupts
#else  
  TIMSKa &= ~(1 << TOIEa);        // turn off timer interrupts
#endif
  interrupts();                   // enable all interrupts
}


#if !SYSTIMERCOMP
ISR(TIMERa_OVF_vect)              // interrupt service routine 
#else 
ISR(TIMERa_COMPA_vect)            // interrupt service routine 
#endif
{
#if !SYSTIMERCOMP
  // The next line should be  "TCNTa += Timera_counter", but then the timer 
  // runs too slow. So just reload the TCNT register with the count (FF06)(-250)
  TCNTa = Timera_counter;         // reload timer (reload value plus timer residual)
#endif
  SysTimerIntFunc();              // call the user defined function (if defined)
}
#endif  // !SYSTIMERINCLUDESDELAY


#if SYSTIMERINCLUDESDELAY
/******************************************************************************/
/*       Redesigned wiring.c functions for delay/millis/micros that use       */  
/*                       a different timer (than timer 0)                     */
/******************************************************************************/
/*

  This file is an extensive rework of the stock "wiring.c" file that normally 
  is part of the Arduino/GNU environment.  Functions like delaymicroseconds, 
  delay, millis, and init and micros are very similar to the original code, 
  but in this module use a different timer and allow for a hook to a user 
  routine in the timer's interrupt routine.
  
  The original wiring.c code has this header:
  
  wiring.c - Partial implementation of the Wiring API for the ATmega8.
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

*/

#include "wiring_private.h"

#define TIMERCOUNTSPERSEC   1000    // Timer interrupt rate (0.001)
#define TIMERPSVALUE        3       // Value for timer prescale regisister (/64)

//#define TIMERISRRATE      0.001   // (not used) interrupt rate for this timer
//Note: Use integer value for TIMERCOUNTSPERSEC -- not ((int)(1.0/TIMERISRRATE)) 
//      (Compiler turns it into 999, NOT 1000)

volatile static unsigned long milliseconds=0; // system milliseconds count 
                                              //(used by delay,millis,micros)

#if !SYSTIMERCOMP             
ISR(TIMERa_OVF_vect)          // interrupt service routine (once per mS)
#else
ISR(TIMERa_COMPA_vect)        // interrupt service routine (once per mS) 
#endif
{
#if !SYSTIMERCOMP
  // The next line should be  "TCNTa += Timera_counter", but then the timer 
  // runs too slow. So just reload the TCNT register with the count (FF06)(-250)
  TCNTa = Timera_counter;     // reload timer (reload value plus timer residual)
#endif  
  milliseconds++;
  SysTimerIntFunc();           // call the user defined function (if defined)
}


unsigned long millis() 
  // Return the number of milliseconds since we started running.  
  // (Rolls over about every 50 days)
{
  // disable interrupts while we read timer0_millis or we might get an
  // inconsistent value (e.g. in the middle of a write to timer0_millis)
  uint8_t oldSREG = SREG; unsigned long t;
  cli();  t = milliseconds;  SREG = oldSREG;
  return t; 
}


unsigned long micros() 
  // Return the number of microseconds since we started running. 
  // (rolls over about every 70min)
  // This is done by returning the number of milliseconds (*1000) plus the 
  // value in the timer counter register * uS/count
{
  uint8_t oldSREG = SREG; unsigned int ctr; unsigned long m; 
#if SYSTIMERCOMP
  // ints off, read milliseconds variable and timer(ctr) 
  cli();  m = milliseconds;  ctr = TCNTa;   
  // If timer cleared after reading milliseconds then add 1 to our copy of 
  // milliseconds (any additional counts still remain in ctr)
  // (8 is some 'cushion' because timer is still counting during reading milliseconds)
  if (ctr<8 && (TIFRa & (1<<OCFaA))) { m++; }
  SREG = oldSREG;         // interrupts back on (if they were)
#else
  // NOTE: if using overflow mode (not comparemode) then should have set ctr to 
  // "Timera_counter" instead of 0  then adjust ctr downward to 0 
  // ( Ctr=-Timera_counter+ctr  ) before adding adding it to m*1000... 
  cli();  m = milliseconds;  ctr = TCNTa;  // ints off, read milliseconds and timer
  SREG = oldSREG;                          // interrupts back on (if they were)
  //
  // Since ctr counts from FF06 up, even if we roll over, the count is still 
  // correct for determining the partial number of uS counts beyond the "m" value.  
  // Do NOT add 1 to m like we do in the SYSTIMERCOMP version (ctr already includes it)
  // Timera_counter is initialized to -250 (FF06).
  ctr = ctr-(unsigned)Timera_counter;      // same as (-Timera_counter)+ctr; 
#endif 
  // m = The number of mS*1000 plus the partial value in the timer register 
  // (will be 0...249) times the number of uS per timer count.   
  // Note: Multiply 0..249 times 4 (for prescale of 3)
  // (1.0/(F_CPU/64)*1000000) is number of uS per timer count (4)
  m=(m*TIMERCOUNTSPERSEC) + (ctr*(unsigned int)(1.0/(F_CPU/TIM5PS(TIMERPSVALUE))*1000000));
  return m; 
}


void delay(unsigned long ms)
  // Delay specified number of milliseconds. 'ms' is the number of mS to delay. 
  // Call 'yield' while delaying.
{
#if 10          // Quicker... Save about 130 bytes
  unsigned long start = millis();
  while (ms>0 && (millis()-start) < ms) { yield(); }
#else           // do it per the original code
  unsigned long start = micros();
  while (ms > 0) {
    yield();
    while ( ms > 0 && (micros() - start) >= 1000) {
      ms--;
      start += 1000;
    }
  }
#endif
}


/* Delay for the given number of microseconds.  Assumes a 1, 8, 12, 16, 20 or 24 MHz clock. */
void delayMicroseconds(unsigned int us)
{
  // call = 4 cycles + 2 to 4 cycles to init us(2 for constant delay, 4 for variable)
  // calling avrlib's delay_us() function with low values (e.g. 1 or
  // 2 microseconds) gives delays longer than desired.
  //delay_us(us);
#if F_CPU >= 24000000L
  // for the 24 MHz clock for the aventurous ones, trying to overclock
  // zero delay fix
  if (!us) return; //  = 3 cycles, (4 when true)
  // the following loop takes a 1/6 of a microsecond (4 cycles)
  // per iteration, so execute it six times for each microsecond of
  // delay requested.
  us *= 6; // x6 us, = 7 cycles
  // account for the time taken in the preceeding commands.
  // we just burned 22 (24) cycles above, remove 5, (5*4=20)
  // us is at least 6 so we can substract 5
  us -= 5; //=2 cycles
#elif F_CPU >= 20000000L
  // for the 20 MHz clock on rare Arduino boards
  // for a one-microsecond delay, simply return.  the overhead
  // of the function call takes 18 (20) cycles, which is 1us
  __asm__ __volatile__ (
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop"); //just waiting 4 cycles
  if (us <= 1) return; //  = 3 cycles, (4 when true)
  // the following loop takes a 1/5 of a microsecond (4 cycles)
  // per iteration, so execute it five times for each microsecond of
  // delay requested.
  us = (us << 2) + us; // x5 us, = 7 cycles
  // account for the time taken in the preceeding commands.
  // we just burned 26 (28) cycles above, remove 7, (7*4=28)
  // us is at least 10 so we can substract 7
  us -= 7; // 2 cycles
#elif F_CPU >= 16000000L
  // for the 16 MHz clock on most Arduino boards
  // for a one-microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 1us
  if (us <= 1) return; //  = 3 cycles, (4 when true)
  // the following loop takes 1/4 of a microsecond (4 cycles)
  // per iteration, so execute it four times for each microsecond of
  // delay requested.
  us <<= 2; // x4 us, = 4 cycles
  // account for the time taken in the preceeding commands.
  // we just burned 19 (21) cycles above, remove 5, (5*4=20)
  // us is at least 8 so we can substract 5
  us -= 5; // = 2 cycles,
#elif F_CPU >= 12000000L
  // for the 12 MHz clock if somebody is working with USB
  // for a 1 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 1.5us
  if (us <= 1) return; //  = 3 cycles, (4 when true)
  // the following loop takes 1/3 of a microsecond (4 cycles)
  // per iteration, so execute it three times for each microsecond of
  // delay requested.
  us = (us << 1) + us; // x3 us, = 5 cycles
  // account for the time taken in the preceeding commands.
  // we just burned 20 (22) cycles above, remove 5, (5*4=20)
  // us is at least 6 so we can substract 5
  us -= 5; //2 cycles
#elif F_CPU >= 8000000L
  // for the 8 MHz internal clock
  // for a 1 and 2 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 2us
  if (us <= 2) return; //  = 3 cycles, (4 when true)
  // the following loop takes 1/2 of a microsecond (4 cycles)
  // per iteration, so execute it twice for each microsecond of
  // delay requested.
  us <<= 1; //x2 us, = 2 cycles
  // account for the time taken in the preceeding commands.
  // we just burned 17 (19) cycles above, remove 4, (4*4=16)
  // us is at least 6 so we can substract 4
  us -= 4; // = 2 cycles
#else
  // for the 1 MHz internal clock (default settings for common Atmega microcontrollers)
  // the overhead of the function calls is 14 (16) cycles
  if (us <= 16) return; //= 3 cycles, (4 when true)
  if (us <= 25) return; //= 3 cycles, (4 when true), (must be at least 25 if we want to substract 22)
  // compensate for the time taken by the preceeding and next commands (about 22 cycles)
  us -= 22; // = 2 cycles
  // the following loop takes 4 microseconds (4 cycles)
  // per iteration, so execute it us/4 times
  // us is at least 4, divided by 4 gives us 1 (no zero delay bug)
  us >>= 2; // us div 4, = 4 cycles
#endif
  // busy wait
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
  // return = 4 cycles
}


// You must define 'init' if you define delay (or millis/micros) so that GCC 
// doesn't need the library module "wiring.c" that contains these functions and 
// init. (GCC either includes the entire module with init and delay or doesn't 
// include any of it)

void init(void)
// This is called before "setup" by main or some functions won't work there!  
// THIS VERSION DOES NOT HAVE ALL OF THE PWM TIMERS SETUP LIKE THE DEFAULT 
// VERSION IN WIRING.C.  
// PWM FUNCTIONS FOR THE TIMER SELECTED ABOVE (SYSTIMERNO) WILL NOT WORK !!!!
{
  sei();
  
#if 10    // if zero then skip initializing the other timers
  // Because init() is the first thing run, just set values in the timer 
  // registers instead of setting lots of bits like in the original code. 
  // This saves LOTS of code space.  (22 vs 60 bytes in timer 4 case)

  // We do not initialize Timer0 because, presumably by using this module, 
  // Timer0 is being used for some other purpose...

// ***************************  Timer1 setup  **********************************

  // timers 1 and 2 are used for phase-correct hardware pwm
  // this is better for motors as it ensures an even waveform
  // note, however, that fast pwm mode can achieve a frequency of up
  // 8 MHz (with a 16 MHz clock) at 50% duty cycle

#if (SYSTIMERNO!=1) && defined(TCCR1B) && defined(CS11) && defined(CS10)
  //TCCR1B = 0;
  //// set timer 1 prescale factor to 64
  //sbi(TCCR1B, CS11);
#if F_CPU >= 8000000L
  //sbi(TCCR1B, CS10);
#endif
  TCCR1B=(1<<CS11) | ((F_CPU >= 8000000L)?(1<<CS10):0);
#elif (SYSTIMERNO!=1) && defined(TCCR1) && defined(CS11) && defined(CS10)
  //sbi(TCCR1, CS11);
#if F_CPU >= 8000000L
  //sbi(TCCR1, CS10);
#endif
  TCCR1=(1<<CS11) | ((F_CPU >= 8000000L)?(1<<CS10):0);
#endif
  // put timer 1 in 8-bit phase correct pwm mode
#if (SYSTIMERNO!=1) && defined(TCCR1A) && defined(WGM10)
  sbi(TCCR1A, WGM10);
#endif

// ***************************  Timer2 setup  **********************************
  // set timer 2 prescale factor to 64
#if (SYSTIMERNO!=2) && defined(TCCR2) && defined(CS22)
  //sbi(TCCR2, CS22);
  TCCR2=(1<<CS22) | ((defined(WGM20))?(1<<WGM20):0);
#elif (SYSTIMERNO!=2) && defined(TCCR2B) && defined(CS22)
  sbi(TCCR2B, CS22);
//#else
  // Timer 2 not finished (may not be present on this CPU)
#endif
  // configure timer 2 for phase correct pwm (8-bit)
#if (SYSTIMERNO!=2) && defined(TCCR2) && defined(WGM20)
  //sbi(TCCR2, WGM20);
#elif (SYSTIMERNO!=2) && defined(TCCR2A) && defined(WGM20)
  sbi(TCCR2A, WGM20);
//#else
  // Timer 2 not finished (may not be present on this CPU)
#endif

// ***************************  Timer3 setup  **********************************

#if (SYSTIMERNO!=3) && defined(TCCR3B) && defined(CS31) && defined(WGM30)
  //sbi(TCCR3B, CS31);    // set timer 3 prescale factor to 64
  //sbi(TCCR3B, CS30);
  //sbi(TCCR3A, WGM30);   // put timer 3 in 8-bit phase correct pwm mode
  TCCR3B=(1<<CS31)|(1<<CS30);  TCCR3A=(1<<WGM30);
#endif

// ***************************  Timer4 setup  **********************************

/* beginning of timer4 block for 32U4 and similar */
#if (SYSTIMERNO!=4) && defined(TCCR4A) && defined(TCCR4B) && defined(TCCR4D) 
  //sbi(TCCR4B, CS42);    // set timer4 prescale factor to 64
  //sbi(TCCR4B, CS41);
  //sbi(TCCR4B, CS40);
  //sbi(TCCR4D, WGM40);   // put timer 4 in phase- and frequency-correct PWM mode 
  //sbi(TCCR4A, PWM4A);   // enable PWM mode for comparator OCR4A
  //sbi(TCCR4C, PWM4D);   // enable PWM mode for comparator OCR4D
  TCCR4B=(1<<CS42)|(1<<CS41)|(1<<CS40);  
  TCCR4D=(1<<WGM40);  TCCR4A=(1<<PWM4A);  TCCR4C=(1<<PWM4D);  
#else /* beginning of timer4 block for ATMEGA1280 and ATMEGA2560 */
#if (SYSTIMERNO!=4) && defined(TCCR4B) && defined(CS41) && defined(WGM40)
  //sbi(TCCR4B, CS41);    // set timer 4 prescale factor to 64
  //sbi(TCCR4B, CS40);
  //sbi(TCCR4A, WGM40);   // put timer 4 in 8-bit phase correct pwm mode
  TCCR4B=(1<<CS41)|(1<<CS40);  TCCR4A=(1<<WGM40);
#endif
#endif /* end timer4 block for ATMEGA1280/2560 and similar */ 

// ***************************  Timer5 setup  **********************************

#if (SYSTIMERNO!=5) && defined(TCCR5B) && defined(CS51) && defined(WGM50)
  //sbi(TCCR5B, CS51);    // set timer 5 prescale factor to 64
  //sbi(TCCR5B, CS50);
  //sbi(TCCR5A, WGM50);   // put timer 5 in 8-bit phase correct pwm mode
  TCCR5B=(1<<CS51)|(1<<CS50);  TCCR5A=(1<<WGM50);
#endif

#endif    // Skip other timers

// Now start up the system timer to 'TIMERCOUNTSPERSEC' rate
  StartSysTimerLocal(SYSTIMERSPEED((1.0/TIMERCOUNTSPERSEC),TIMERPSVALUE));

// *******************************  ADC setup **********************************
//
//  Modified version of the A/D setup code... Assume we're just starting out.
//  Just blatently set ADCSRA to a value instead of twiddling bits. Saves 20+ bytes 

#if defined(ADCSRA)
  // set a2d prescaler so we are inside the desired 50-200 KHz range.
  #if F_CPU >= 16000000 // 16 MHz / 128 = 125 KHz
    ADCSRA=(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
  #elif F_CPU >= 8000000 // 8 MHz / 64 = 125 KHz
    ADCSRA=(1<<ADPS2)|(1<<ADPS1);
    //cbi(ADCSRA, ADPS0);
  #elif F_CPU >= 4000000 // 4 MHz / 32 = 125 KHz
    ADCSRA=(1<<ADPS2)|(1<<ADPS0);
    //cbi(ADCSRA, ADPS1);
  #elif F_CPU >= 2000000 // 2 MHz / 16 = 125 KHz
    ADCSRA=(1<<ADPS2);
    //cbi(ADCSRA, ADPS1);
    //cbi(ADCSRA, ADPS0);
  #elif F_CPU >= 1000000 // 1 MHz / 8 = 125 KHz
    ADCSRA=(1<<ADPS1)|(1<<ADPS0);
    //cbi(ADCSRA, ADPS2);
  #else // 128 kHz / 2 = 64 KHz -> This is the closest you can get, the prescaler is 2
    ADCSRA=(1<<ADPS0);
    //cbi(ADCSRA, ADPS2);
    //cbi(ADCSRA, ADPS1);
  #endif
  // enable a2d conversions
  sbi(ADCSRA, ADEN);
#endif

//
// This was the original version of the A/D setup code..... 
//
#if defined(ORIGINAL_ADCSRA)
  // set a2d prescaler so we are inside the desired 50-200 KHz range.
  #if F_CPU >= 16000000 // 16 MHz / 128 = 125 KHz
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #elif F_CPU >= 8000000 // 8 MHz / 64 = 125 KHz
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
  #elif F_CPU >= 4000000 // 4 MHz / 32 = 125 KHz
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #elif F_CPU >= 2000000 // 2 MHz / 16 = 125 KHz
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
  #elif F_CPU >= 1000000 // 1 MHz / 8 = 125 KHz
    cbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #else // 128 kHz / 2 = 64 KHz -> This is the closest you can get, the prescaler is 2
    cbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #endif
  // enable a2d conversions
  sbi(ADCSRA, ADEN);
#endif
  // the bootloader connects pins 0 and 1 to the USART; disconnect them
  // here so they can be used as normal digital i/o; they will be
  // reconnected in Serial.begin()
#if defined(UCSRB)
  UCSRB = 0;
#elif defined(UCSR0B)
  UCSR0B = 0;
#endif
}

#endif    // SYSTIMERINCLUDESDELAY

