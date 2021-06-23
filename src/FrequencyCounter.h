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

// See .cpp file for a description of this module and it's functions.

#ifndef _FREQCTR_H
#define _FREQCTR_H

#define sbyte int8_t  // also char

class FrequencyCounter
{
  public:
    sbyte mode(sbyte Resolution);
      // Starts or stops the frequency counter function and sets the gate time.
      // "GateTime" is one of the following: 
      //   -1= Return current frequency counter gate time, 0= Freq counter off, 
      //   1= 1 Sec gate time, 2= 10mS gate time,    3= 100mS gate time, 
      //   4= 10 Sec gate time 5= 100 Sec gate time, 6= Ext Gate (low going).
      //   7= Period mode (1 period), 8= Period mode (average 10 periods)
      //   9= Period mode (average 100 periods)
      // GateTime values 6..9 are available only if compile option is enabled.
      // Function returns the current gate time or -1 if error.     

    sbyte mode(void);
      // Returns the current gate mode/time. (0..9)

    byte available(void);
      // Returns true after each new update. False after reading the value.

    char *read(char *St,  bool Wait);
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

    unsigned long read(bool Wait);
      // Read the frequency counter and return the value as an unsigned long. 
      // Function returns the raw frequency read. IT IS NOT SCALED FOR GATE 
      // OR NUMBER OF AVERAGES!!  It is presumed a higher level function will 
      // do this or use the string version of this function for a corrected value. 
      // 'Wait' is non-zero to wait for the next (a "fresh") frequency count, or 
      // 0 to return the  last frequency read.
};


/******************************************************************************/
/*                     Externally available support functions                 */
/******************************************************************************/

extern void FreqCtrGateISR(void);
  // This function implements the "gating" function of the frequency counter.
  // A "gate time" has finished. Move collected count to a holding register 
  // and restart the counting for the new "gate time".  
  // This function should be called every 10 milliseconds  (preferably by an 
  // accurate timer) when FCGateTime is not zero. 
  // This is normally done in the ISR of one of the timers in the micro. 


/******************************************************************************/
/*                        User configurable options                           */
/*     Changing these options controls which features the module contains     */
/******************************************************************************/

// Does this module take over the SysTimerIntFunc function ?
#define FCINCLUDESYSTIMERLINK 1             // define as non-zero to take over function

// Allow Ext Gate mode?       (adds 244 flash bytes )
#define FCEXTERN              1             // 1= ext gate mode enabled

// Arduino pin number to use for external gate
#define FCEXTGATEMSK          PCINTMASK9    // PB5 isr index (Arduino Digital 9)

// Allow period measure mode? (adds 1030 flash bytes)
#define FCPERIOD              1             // 1= Period measure mode enabled

// for period measure it must occur within this many mS
#define PERIODTIMOUT          5000          

// If there is a prescaler, put prescale value here
#define FCPRESCALER           1             

// Enable this for debug messages.
//#define FRQCTRDEBUG           1             // For debug... show debug messages. 


#endif    // _FREQCTR_H

