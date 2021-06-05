
/******************************************************************************/
/*                                                                            */
/*               FrequencyCounter -- Frequency Counter Module                 */
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

  This module implements a frequency counter class using Timer0.  It works as 
  a traditional frequency counter by counting the number of pulses that occur
  on an input pin for a fixed amount of time or optionally can measure the 
  period of the input signal.  It is started by calling the ".mode" member
  to start and set the mode and gate time and then calling the ".read" member
  to get the latest frequency count that the module counted.  There is also 
  a function to see if a frequency count period has passed and the frequency 
  count is updated called ".available".  The 'system' timer is moved to either 
  timer 1 or timer 3 and that timer is also used for the gate timer. 

  In the frequency counter mode, the gate time can be specified as one of 5 
  gate times (10mS, 100mS, 1S, 10S & 100S) or a gate signal can be supplied
  via an external input pin.  In the period measure mode, the input period can 
  be measured and will be converted to a frequency.  This is useful for 
  measuring lower frequency signals with more resolution quicker than the 
  traditional frequency counter mode.  When in the period measure mode, a 
  single input pulse can be measured or 10 or 100 input pulses can be averaged. 

  The functions of this class are as follows (sbyte is 'char' or 'int8_t'):

  sbyte FrequencyCounter::mode(sbyte GateTime) 
  // Starts or stops the frequency counter function and sets the gate time.
  // "GateTime" is one of the following: 
  //   -1= Return current frequency counter gate time, 0= Freq counter off, 
  //   1= 1 Sec gate time, 2= 10mS gate time,    3= 100mS gate time, 
  //   4= 10 Sec gate time 5= 100 Sec gate time, 6= Ext Gate (low going).
  //   7= Period mode (1 period), 8= Period mode (average 10 periods)
  //   9= Period mode (average 100 periods)
  // GateTime values 6..9 are available only if compile option is enabled.
  // Function returns the current gate time or -1 if error.     
  
  sbyte FrequencyCounter::mode(void) 
  // Returns the current gate mode/time. (0..9)

  char *FrequencyCounter::read(char *St,  bool Wait)
  // Reads the value of the frequency counter and returns a string of the 
  // value, corrected for gatetime or period averaging. 
  // "St" is a string buffer in which this function will create a string that 
  //   is the frequency read.  It should be big enough to hold the frequency 
  //   read (15 characters?).
  // "Wait" is non-zero to wait for the next (a "fresh") frequency count, or 
  //   0 to return the last frequency read.
  // The function returns a string of the frequency read or NULL. 
  // This function returns a string instead of an integer or floating point 
  // value so that floating point library functions are not required and 
  // gate times that are over one second and period measurements return the 
  // fractional part of the frequency read. 
  // Be aware that if "Wait" is true then this function will not return until 
  // the gate time has passed and a "fresh" frequency count is available.  
  // This could be up to 100 seconds.
  // When in a period measure mode, the period measured is converted to a 
  // frequency and that value is returned.  In this period measure mode, if the 
  // frequency is too high then '999999' is returned and if the frequency is 
  // too low (or 0Hz) or the software times out  then '0.00000' is returned.  
  // The input signal must provide 2 (a complete wave), 11 or 101 transitions  
  // within the timeout period or '0.00000' is returned.  The timeout period 
  // is configurable and defaults to 5 seconds.

  long FrequencyCounter::read(bool Wait)
  // Read the frequency counter and return the value as an unsigned long. 
  // Function returns the raw frequency read. IT IS NOT SCALED FOR GATE 
  // TIME OR NUMBER OF AVERAGES!!  It is presumed a higher level function will 
  // do this or use the string version of this function for a corrected value. 
  // 'Wait' is non-zero to wait for the next (a "fresh") frequency count, or 
  // 0 to return the last frequency read.

  byte FrequencyCounter::available(void)
  // Returns a non-zero (true) value if a new frequency count is available
  // or zero if not.   This function returns immediately and can be used 
  // instead of calling FrequencyCounter::read with "Wait" true to check to 
  // see if a "fresh" count value is available. 

  While not part of the class, another function is externally available if 
  needed. 

  void FreqCtrGateISR(void)
  // This function implements the "gating" function of the frequency counter.
  // It should be called each time a "gate time" has finished. It moves the 
  // collected count to a holding register and restart the counting for the 
  // new "gate time".  This function should be called every 10 milliseconds 
  // (preferably by an accurate timer) when FreqCtrGate (mode) is not zero. 
  // This is normally done in the ISR of one of the timers in the micro. 
  
  While the basic user interface is via a class, only a single instance should 
  be declared as this module uses specific hardware resources. 
  
  Input to the frequency counter is on Arduino Digital pin 6 [PD7] for 
  Pro Micro (atmega32u4) and is assumed to be a low going train of pulses 
  to count.  Obviously the input signal must be an appropriate TTL level to 
  be counted. Input amplifiers to amplify lower level signal to this TTL level
  (if needed) are beyond the scope of this work. 
   
  This module requires a "gating" source that is typically a different timer 
  that occurs at a fixed time rate.  If not using the supplied "systimer" 
  module then the function "FreqCtrGateISR" needs to be called at the proper 
  periodic rate.  The accuracy of this module is directly affected by the 
  accuracy of this gate timing source!
  
  This module is designed for the Arduino Pro Micro module that uses an Atmel 
  ATMega32U4, and uses Timer 0 for the counting input. It would be ideal to 
  use another timer for this function, but unfortunately the ext clock pin 
  for Timer1 (AtMEGA32U4 pin 26) is not routed to the modules connector and 
  only Timer0 and Timer1 have an external clock input, so no alternative 
  exists other than to use Timer0.  As is known, Timer0 is used for the 
  Arduino system timing functions (delay, millisec, microsec, etc.), so the 
  only alternative is to move these functions to some other timer.  This is 
  done by reworking the stock wiring.c module that is part of the Arduino 
  system to use a different timer for this purpose.  Then this "system" timer 
  is expanded to also use it for the gate timer for the frequency counter 
  function.   
  
  Obviously, when using an alternate timer for the "system" timer function 
  and frequency counter gating function, the timer will not be available for 
  PWM functions or any other "built in" library functions that normally depend 
  on Timer0 or the timer selected for the "system" timer in the Arduino software.
  
  By simply including "SysTimer.cpp" in the build, the functions normally 
  implemented by Timer0 (delay, millisec, microsec, etc.) are moved to use 
  this alternate timer (configurable to Timer1 or Timer3), and a hook to the 
  Timer's ISR is accessable, allowing this timer to be used for the frequency 
  counter function.  

  NOTE: It might be best to use Timer1 for the gate, because it has the 
  highest interrupt priority (even above Timer0), but either Timer1 or Timer3
  is able to be used.   If using USB, then the count returned may not be as 
  precise as when using other or no external communications methods.  This is 
  because the USB interrupts have a higher priority than the Timer1 / Timer3 
  timer that is used for the gate timer.  A possible alternative would be to 
  set up a timer for the gate time and output it on a pin and then use a pin 
  change or external interrupt as the gate timer source.  (Pin change and 
  external interrupts have a higher priority than the USB interrupts).
  Also note that at frequencies above about 2MHz the count returned might be 
  short a count or two.  This is because of the necessity of clearing the 
  counter, turning it off and then back on which takes a couple of CPU cycles. 

  This frequency counter module can also be set up to use external gating. 
  To do so, define 'FCEXTERN' as non-zero and include the module 
  PCInterrupt.cpp in this sketch.  Then by setting 'FrequencyCounter::mode' to 
  6, the Arduino pin defined by 'FCEXTGATEMSK' will be used as the gate input.
  This is configured to Arduino Digital pin 9 [PB5] in the supplied code, but 
  can be changed to a number of other pins.  When activated, a low on this 
  pin turns on the gate, and when hi the gate is turned off.  Including the 
  external gate function is optional.
  
  Using the external gate function and another timer set up to work 
  autonomously and then output its signal on some other pin, and then 
  connecting this pin to the Ext Gate input might be a way to get around the 
  USB problem  mentioned above that might affect the count, because the 
  external gate inputs are all higher in priority than the USB interrupt are.

  With the Arduino Pro Micro modules available during development the system 
  was able to reliably count to frequencies of up to about 8MHz (1/2 of 
  processor clock) in the frequency counter mode and about 20KHz in the period 
  mode with averaging of 10 or 100 or 10KHz in the period mode with averaging 
  of 1. 

  If it is desired to "trim" the Pro Micro's 16MHz oscillator to count or 
  generate frequencies that are more accurate, a Knowles Voltronics JR400 
  trimmer capacitor (8-40pF) (or similar) can be used to replace the C2 
  capacitor on the module.  (You could also replace C2 with a 10pF cap and use 
  a JR200 (4.5-20pF) for a more stable adjustment with less range).  This 
  trimmer capacitor can be placed on top of the 32U4 chip and tiny wires 
  (wire wrap?) used to connect the two connections of it to the Pro micro 
  module.  Connect the rotor of the trimmer cap to GND (the '-' side of C19 is 
  a good place) and the other side of the trimmer cap to the non-GND side of C2.  
  If the rotor side of the trimmer cap is connected to the C2 connection, the 
  oscillator will not be as stable. Once wired, glue the trimmer cap to the top
  of the 32U4 chip.  Then set the frequency generator of the chip this is 
  programmed on to 4MHz and using a reference frequency counter, tune the 
  trimmer capacitor to exactly 4MHz. If frequency counter only is programmed, 
  inject a 4MHz signal into the frequency counter input and tune to exactly 4MHz. 

  The repository also includes a documentation file that provides more details 
  on the use of this library and also covers a companion frequency generator 
  library.

*/

/* 
Revision log: 
  1.0.0    2-5-21    REG   
    Initial implementation

*/


/******************************************************************************/
/*                        User configurable options                           */
/******************************************************************************/


// Allow Ext Gate mode?       (adds 244 flash bytes )
#define FCEXTERN              1             // 1= ext gate mode enabled

// Arduino pin number to use for external gate
#define FCEXTGATEMSK          PCINTMASK9    // PB5 isr index (Arduino Digital 9)

// Allow period measure mode? (adds 1030 flash bytes)
#define FCPERIOD              1             // 1= Period measure mode enabled

#define PERIODTIMOUT          5000          // for period measure it must 
                                            // occur within this many mS

#define FCPRESCALER           1             // If there is a prescaler, put prescale value here

// Enable this for debug messages.
//#define FRQCTRDEBUG           1             // For debug... show debug messages. 


/******************************************************************************/
/*                                                                            */
/*                     Private FreqCtr variables / functions                  */
/*                                                                            */
/******************************************************************************/

#include <arduino.h>
#include "FrequencyCounter.h"
#include "systimer.h"     // access to SysTimerIntFunc
#if FCEXTERN
#include "PCInterrupt.h"  // access to PCChangeIntFunc (for ext gate)
#endif

#if !(defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__))
#error "This module (FrequencyCounter.cpp) only supports ATMega32U4/16U4"
#endif

#if FCPERIOD && FCEXTERN
#define FCEXTNO               6             // this is the value for ext clock mode
#define FCPRDNO               7             // this is the value for period mode
#define FCMODEMAX             9             // this is the max value of mode  
#elif FCEXTERN
#define FCEXTNO               6             // this is the value for ext clock mode
#define FCMODEMAX             6             // this is the max value of mode  
#elif FCPERIOD
#define FCPRDNO               6             // this is the value for period mode
#define FCMODEMAX             8             // this is the max value of mode  
#else
#define FCMODEMAX             5             // this is the max value of mode  
#endif

#if FRQCTRDEBUG
#define printfROM(fmt, ...)       printf_P(PSTR(fmt),##__VA_ARGS__)
#endif 


// These variables and functions are hardware dependent and assume the use of 
// the systimer and PCInterrupt modules. 

// private variables
volatile byte                 _FreqCtrReady = 0;  // true after each new update. false after reading.
volatile static unsigned long fcResult = 0;       // the "raw" value returned to the user. (not scaled)
volatile static unsigned long fcOVF = 0;          // upper bits of freq counter value being accumulated
volatile static unsigned int  fcprescaler= 0;     // The gate time (prescaler) counter 
static unsigned int           fcprescalInit = 0;  // The gate time (prescaler) initialization value 
                                                  //  (10's of mS) (0,1,10,100,1000)
static sbyte                  fcGateTime=0;       // saved selected gate time

#if FCPERIOD
static byte                   PrdCnt=0;           // Averaging for period measure
#else
#define PrdCnt                0                   // dummy value if if no period measure
#endif


#if defined(_SYSTIMER_H)
extern "C" void SysTimerIntFunc(void) 
  // This function is called by the system timer interrupt routine when
  // the timer times out each millisecond.  This timing is used to call the 
  // frequency counter gating function "FreqCtrGateISR" every 10mS.  
  // If this function is defined, it replaces the "weak" definition in the
  // systimer module. 
{ 
#define FCTIME   10             // 10mS
  static byte fcprescale = FCTIME;
  // Do frequency counter gate function every FCTIME mS (10 mS)
  if (!--fcprescale)  
  { 
    FreqCtrGateISR(); 
    fcprescale=FCTIME;   
  }
#if FRQCTRDEBUG
#define LED2        21          // LED to blink to show we're seeing the ISR.
#define LED2TIME    250
  static unsigned int dbgPS = LED2TIME; 
  if (!--dbgPS) { dbgPS=LED2TIME;  digitalWrite(LED2, !digitalRead(LED2)); } 
#endif
}
#endif


#if FCEXTERN
extern "C" void PCChangeIntFunc(byte Changes[])
  // If the external gate time transitioned, start or stop the count
  // This function is normally called via the pin change or external interrupt.
  // Changes is an array[2] of byte that has bits set for each pin that changed 
  // state.  Changes[0]=bits that just went low, Changes[1]=bits that just went high
  // If this function is defined, it replaces the "weak" definition in the
  // PCInterrupt module. 
{  
  byte svTCCR, svTCNT;

  if (fcGateTime==FCEXTNO) 
  {
    if (Changes[0]&FCEXTGATEMSK)       // if PinX went low
    {
      // Start the counting 
      // ext clock--falling edge, reset overflow counter 
      TCNT0=0; TCCR0B=6; fcOVF=0; 
      Changes[0]&=~FCEXTGATEMSK;       // reset the changes bit
    }
    if (Changes[1]&FCEXTGATEMSK)       // if PinX went hi
    {
      // finish the counting and move the results to fcResult
      svTCCR=TCCR0B; TCCR0B=0; 
      fcResult = (fcOVF << 8) + (unsigned long) TCNT0;  
      fcOVF=0; // reset overflow counter
      // Note: We use TCCR0B<>0 to indicate it's an 'active' cycle 
      //       (a full gate period) (not the first gate cycle after we turned it on)
      if (svTCCR) _FreqCtrReady=1;
      Changes[1]=~FCEXTGATEMSK;        // reset the changes bit
    }
  } 
}
#endif


ISR(TIMER0_OVF_vect) {
  // Frequency counter counting interrupt service routine. 
  // Increment the overflow counter when we run out of counts in the hardware
  // register (TCNT0). This happens every 256 counts.  (Freq counter mode)
  // In period measure mode the timer is loaded with FF so that on the first 
  // count, it generates an interrupt. Reload timer with FF for next cycle and 
  // then subtract current 'micros()' from saved 'micros' to come up with time. 
#if FCPERIOD  
  if (fcGateTime >= FCPRDNO)     // if period mode
  {
    {
      unsigned long SavMicros;
      SavMicros=micros();                 // save current uS count
      //TCCR0B^=1;                        // now look for the other edge
      TCNT0=-PrdCnt;                      // reload counter
      if (fcOVF)                          // if we had a valid start transition
      { 
        fcResult=SavMicros-fcOVF;         // save new result
        _FreqCtrReady=1;                  // show ready
      }
      fcprescaler=fcprescalInit;          // restart the timeout timer
      fcOVF=SavMicros;                    // save micros for next time
    }  
  }
  else                          // ordinary frequency counter.
#endif  // FCPERIOD
    fcOVF++; 
}


void FreqCtrGateISR(void)
  // This function implements the "gating" function of the frequency counter.
  // A "gate time" has finished. Move collected count to a holding register 
  // and restart the counting for the new "gate time".  
  // This function should be called every 10 milliseconds  (preferably by an 
  // accurate timer) when FCGateTime is not zero. 
  // This is normally done in the ISR of one of the timers in the micro. 
{
  byte svTCCR,svTCNT; 

  // If it's gate time.     Note: fcprescaler will be 0 if counter is off
  if (fcprescaler && !--fcprescaler)      
  {
#if FCPERIOD
    if (fcGateTime >= FCPRDNO)
    {
#if PERIODTIMOUT
      // For period measurement this is a timeout.  If we don't get 
      // enough transitions for a period measure within a certain amount of 
      // time, then restart the timer and report no input frequency found.
      if (!_FreqCtrReady)
      {
        TCNT0=-PrdCnt;                    // reload counter
        TIFR0 |= (1 << TOV0);             // reset a possible int that might have happened
        fcOVF=0;                          // show we don't have valid start transition
        fcResult=1;  _FreqCtrReady=1;     // set result to 1, show ready
      }
#endif      
    }
    else
#endif    // FCPERIOD
    {
      // Turn off counter and get value, reset TCNT0, turn counter back on.
      // Note: We use TCCR0B<>0 to indicate it's an 'active' cycle 
      //       (not the first gate cycle after we turned it on)
      noInterrupts(); // USB seems to interfere less if we shut interrupts off
      svTCCR=TCCR0B; TCCR0B=0; svTCNT=TCNT0; 
      TCNT0=0; TCCR0B=6;                        // ext clock--falling edge 
      interrupts(); 
      fcResult = (fcOVF << 8) + (unsigned long) svTCNT;  
      fcOVF=0; // reset overflow counter
      // Note: We use TCCR0B<>0 to indicate it's an 'active' cycle 
      //       (a full gate period) (not the first gate cycle after we turned it on)
      if (svTCCR) _FreqCtrReady=1;
    } 
    fcprescaler=fcprescalInit;          // reinit the prescaler
  }     // if (fcprescaler && !--fcprescaler)       
}


/******************************************************************************/
/*                                                                            */
/*                   FrequencyCounter Class functions                         */
/*                        (Main user interface)                               */
/*                                                                            */
/******************************************************************************/

byte FrequencyCounter::available(void) { return _FreqCtrReady; }
  // Returns true after each new update. False after reading the value.


sbyte FrequencyCounter::mode(void)     { return fcGateTime;  }
  // Returns the current gate mode/time. (0..9)


sbyte FrequencyCounter::mode(sbyte GateTime)
  // Starts or stops the frequency counter function and sets the gate time.
  // "GateTime" is one of the following: 
  //   -1= Return current frequency counter gate time, 0= Freq counter off, 
  //   1= 1 Sec gate time, 2= 10mS gate time,    3= 100mS gate time, 
  //   4= 10 Sec gate time 5= 100 Sec gate time, 6= Ext Gate (low going).
  //   7= Period mode (1 period), 8= Period mode (average 10 periods)
  //   9= Period mode (average 100 periods)
  // GateTime values 6..9 are available only if compile option is enabled.
  // Function returns the current gate time or -1 if error.     
{
  unsigned int t=0; byte i,j, svGateTime=fcGateTime;

  if (GateTime < 0) goto GetGate;
  if (GateTime > FCMODEMAX) return -1;  
  fcGateTime=GateTime;
#if FCEXTERN
  if (GateTime==FCEXTNO) GateTime=1;
#endif  // FCEXTERN
  // Convert gate time to sequential times 0=off, 1=10mS, 2=100mS, 3=1S, 4=10S,5=100s
  if (GateTime && GateTime<4)  if (!--GateTime) GateTime=3; 
  // Create prescale value:  0=off, 1=10mS, 10=100mS, 100=1S, 1000=10S, 10000=100s
  if (GateTime) for (i=1,t=1; i<GateTime; i++)  t*=10; 
#if FCPERIOD
  if (fcGateTime >= FCPRDNO) 
  {   
    // Determine value for PrdCnt (1, 10 or 100 averages)
    for (j=(GateTime-FCPRDNO),PrdCnt=1,i=0; i<j; i++) PrdCnt*=10; 
    t=(PERIODTIMOUT/10);      // set timeout value
  }
#endif
  fcprescaler=fcprescalInit=t;  
  if (fcprescaler)            // if freq counter is on
  {
    fcprescaler=1;            // give us some time to set up (2), set gate time
    pinMode(6, INPUT_PULLUP); // Timer 0 Clock input is always on D6 (ProMicro)
    // Note: Don't turn on extinput on TCCRB (TCCR0B=6).. Let the ISR do it. 
    //       This prevents user from reading a partial frequency count.
    //       See FreqCtrGateISR
#if FCPERIOD
    if (fcGateTime >= FCPRDNO)
    {
      // Set timer 0 to max count so it rolls over on one external transition 
      TCCR0A=0; TCNT0=-PrdCnt;  
      fcOVF=0;   
      // Turn on the counter
      TCCR0B = 6; 
      TIFR0  |= (1 << TOV0);    // reset any residual int
      TIMSK0 |= (1 << TOIE0);   // enable timer overflow interrupt
    }
    else
#endif    // FCPERIOD
    {
      TCCR0A = 0;   TCCR0B = 0; // Prescaler.. 0=timer off, 1=/1,2=/8,3=/64,4=/256,5=/1024,6=/ext fall,7=/ext rise
      TCNT0 = 0;
      TIMSK0 |= (1 << TOIE0);   // enable timer overflow interrupt
#if FCEXTERN
      if (fcGateTime==FCEXTNO) { InitPCInterrupt(FCEXTGATEMSK); fcprescaler=0; }
#endif
    }
    interrupts();
  }
  else                          // Turn freq counter off
  {
    //fcprescalInit=fcprescaler=0;  // Shut off the counting (already done above)
    TCCR0A = 0;   TCCR0B = 0;   // Prescaler.. 0=timer off, 1=/1,2=/8,3=/64,4=/256,5=/1024,6=/ext fall,7=/ext rise
    TIMSK0 &= ~(1 << TOIE0);    // disable timer overflow interrupt
#if FCEXTERN
    if (svGateTime==FCEXTNO)  InitPCInterrupt(0);
#endif
  }
  _FreqCtrReady=0; fcResult=0;
GetGate:
#if FRQCTRDEBUG
  pinMode(LED2,OUTPUT);         //
  printfROM("InGateTim=%d CvtGateTim=%u Prescale=%u PSInit=%u PrdCnt=%u\n",fcGateTime,GateTime,fcprescaler,fcprescalInit,PrdCnt);
#endif
  return fcGateTime;
}

#if FCPERIOD
// These two function are only needed if period measurments are enabled.

byte  Log10I(unsigned long Val)
  // Get the log10 of Val (integer part only)
{
  byte i=0; 
  while (Val>1) { i++; Val/=10; }
  return i;
}


char *DPStr(char *st, unsigned long Val, byte DP)
  // Convert 'val' to a fractional part of a number string. Return st;
  // DP is the number of digits past the decimal point
  // Example:  DPStr(st,12345,2) will return ".45"
{
  st[0]=0; if (DP) 
  {
    sprintf_P(st,PSTR("%09lu"),Val); 
    strcpy(st+1,st+(strlen(st)-DP)); st[0]='.';
  }  
  return st; 
}
#endif


char *FrequencyCounter::read(char *St,  bool Wait)
  // Reads the value of the frequency counter and returns a string of the 
  // value, corrected for gatetime or period averaging. 
  // "St" is a string buffer in which this function will create a string that 
  //   is the frequency read.  It should be big enough to hold the frequency 
  //   read. (15 characters?)
  // "Wait" is non-zero to wait for the next (a "fresh") frequency count, or 
  //   0 to return the last frequency read.
  // The function returns a string of the frequency read or NULL. 
  // This function returns a string instead of an integer or floating point 
  // value so that floating point library functions are not required and 
  // gate times that are over one second and period measurements return the 
  // fractional part of the frequency read. 
  // Be aware that if "Wait" is true then this function will not return until 
  // the gate time has passed and a "fresh" frequency count is available.  
  // This could be up to 100 seconds.
  // When in a period measure mode, the period measured is converted to a 
  // frequency and that value is returned.  In this period measure mode, if the 
  // frequency is too high then '999999' is returned and if the frequency is 
  // too low (or 0Hz) or the software times out  then '0.00000' is returned.  
  // The input signal must provide 2 (a complete wave), 11 or 101 transitions  
  // within the timeout period or '0.00000' is returned.  The timeout period 
  // is configurable and defaults to 5 seconds.
{
  char dp; unsigned long scale; unsigned long Val; 

  if (!St) return St;               // if no place to put result, return NULL;
  // Wait if requested.  (only if counter is on and wait is true)
  while (fcGateTime && Wait && !_FreqCtrReady) { yield(); }   
  noInterrupts();  Val = fcResult;  interrupts();   // Get the frequency read
#if  FCPERIOD
  if (fcGateTime >= FCPRDNO)  
  {
#if FRQCTRDEBUG
    printfROM("Cnt=%lu (%lX)  ",Val,Val);
#endif
    dp=5;  scale=100000;                  // Set #dp's and scale
    // If the period is ready and large enough to not overrun an unsigned long
    if (_FreqCtrReady && Val>(24*PrdCnt)) 
    { 
      Val=(100000000000ULL*PrdCnt)/Val;   // Convert period to frequency
    }
    else
    { 
      // If timeout show '0.00000' (no transitions on input)  
      // else show '999999' (input frequency too fast)
      if (Val==1) *((byte *)&Val)=0;  else { dp=0; scale=1; Val=999999; }
    }
  }
  else
#endif   // FCPERIOD 
#if FCPERIOD
  {     // else regular count mode
    scale=fcprescalInit/100;   dp=Log10I(fcprescalInit)-2; 
    if (dp<0) { scale=1; while (dp<0) { Val*=10; dp++; } }
    //Note: this is longer -->  if (dp<0) { scale=1; Val*=(100/fcprescalInit); dp=0; }
  }
#if FCPRESCALER && (FCPRESCALER != 1)
    Val*=FCPRESCALER;               // multiply Val by the prescaler
#endif
  // At this point 'dp' is #digits after dec pt.  scale is the scaler value.
  // Val/scale is integer part. Val%scale is fract part
  // Print the integer part 
  sprintf_P(St,PSTR("%lu"),Val/scale);   
  // Print the fractional part
  // Would like to do sprintf_P(St,PSTR("%0*lu"),dp,Val%scale)
  // but GCC doesn't understand the '*' to get the width from parameters.
  // In the next line it should be 'Val%scale', but we just cut off the front
  // so it doesn't matter.  Also the "if (dp)" is eliminated by compiler, 
  // because the function already handles it.
  if (dp) DPStr(St+strlen(St),Val,dp); 
  //if (dp) sprintf_P(St+strlen(St),(dp>1)?PSTR(".%02d"):PSTR(".%d"), (unsigned)Val%scale);
#else     // shorter code.. Use with no period measure functionality
    byte fp; 
    dp=0;
    //if (fcprescalInit)          // include if no divide by 0 is allowed
    {
#if FCPRESCALER && (FCPRESCALER != 1)
    Val*=FCPRESCALER;               // multiply Val by the prescaler
#endif
      if (fcprescalInit<=100)     // integer value only (off, 10mS, 100mS, 1S)
      {
        // For 10mS,100mS multiply Val times 10 or 100 so its the actual 
        // frequency.  NOTE: fcprescalInit is 0 when ctr is off, but code 
        // doesn't seem to care about the div 0.
        Val*=(100/fcprescalInit); 
      }
      else                        // integer and fraction (10s, 100s)
      { 
        // Calculate the fractional part and the integer part 
        dp=fcprescalInit/100;  fp=Val%dp; Val/=dp;
      }
    }
    sprintf_P(St,PSTR("%lu"),Val);   // print the integer part
    // If there's a fractional part, add it
    if (dp) sprintf_P(St+strlen(St),(dp>10)?PSTR(".%02d"):PSTR(".%d"),fp);
#endif   // shorter code with no period mode
  _FreqCtrReady=0;                  // Show we've read this value 
  return St;                        // return the freq ctr string
}  


unsigned long FrequencyCounter::read(bool Wait)
  // Read the frequency counter and return the value as an unsigned long. 
  // Function returns the raw frequency read. IT IS NOT SCALED FOR GATE 
  // OR NUMBER OF AVERAGES!!  It is presumed a higher level function will 
  // do this or use the string version of this function for a corrected value. 
  // 'Wait' is non-zero to wait for the next (a "fresh") frequency count, or 
  // 0 to return the  last frequency read.
{
  unsigned long Val;
  // Wait if requested.  (only if counter is on and wait is true)
  while (fcGateTime && Wait && !_FreqCtrReady) { yield(); }   
  _FreqCtrReady=0;                  // Show we've read this value 
  noInterrupts(); Val=fcResult; interrupts();
#if FCPRESCALER && (FCPRESCALER != 1)
    Val*=FCPRESCALER;               // multiply Val by the prescaler
#endif
  return Val;
}

