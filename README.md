## [Frequency Counter](https://github.com/Rick-G1/FrequencyCounter) library for Arduino and Pro Micro (ATMega32U4)

Available as Arduino library "**FrequencyCounter**"

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

[![Hit Counter](https://hitcounter.pythonanywhere.com/count/tag.svg?url=https%3A%2F%2Fgithub.com%2FRick-G1%2FFrequencyCounter)](https://github.com/brentvollebregt/hit-counter)

Frequency counter library for AVR **ATMega32U4** (and similar) processor using **Timer 0** and Timer 1 or Timer 3 (which also becomes the system timer).

This module implements a frequency counter class using Timer0.  It works as a traditional frequency counter by counting the number of pulses that occur on an input pin for a fixed amount of time or optionally can measure the period of the input signal.  It is started by calling the ".mode" member to start and set the mode and gate time and then calling the ".read" member to get the latest frequency count that the module counted.  There is also a function to see if a frequency count period has passed and the frequency count is updated called ".available".  The 'system' timer is moved to either timer 1 or timer 3 and this timer is also used for the gate timer. 

In the frequency counter mode, the gate time can be specified as one of 5 gate times (10mS, 100mS, 1S, 10S & 100S) or a gate signal can be supplied via an external input pin.  In the period measure mode, the input period can be measured and will be converted to a frequency.  This is useful for measuring lower frequency signals with more resolution quicker than the traditional frequency counter mode.  When in the period measure mode, a single input pulse can be measured or 10 or 100 input pulses can be averaged. 

## Interface
The library is implemented as a class named "`FrequencyCounter`" with 3 member functions (The type sbyte is 'char' or 'int8_t'):

`sbyte `**mode**`(sbyte GateTime)`  Starts or stops the frequency counter function and sets the gate time.  "GateTime" is one of the following: 
 - -1= Return current frequency counter gate time
 - 0= Freq counter off
 - 1= 1 Sec gate time 
 - 2= 10mS gate time 
 - 3= 100mS gate time 
 - 4= 10 Sec gate time
 - 5= 100 Sec gate time
 - 6= Ext Gate (low going)
 - 7= Period mode (1 period)
 - 8= Period mode (average 10 periods)
 - 9= Period mode (average 100 periods)
GateTime values 6..9 are available only if compile option is enabled.
Function returns the current gate time or -1 if error. 
 
`sbyte `**mode**`(void) `  Returns the current gate mode/time. (0..9)
 
`char *`**read**`(char *St,  bool Wait)` Reads the value of the frequency counter and returns a string of the value, corrected for gatetime or period averaging. "St" is a string buffer in which this function will create a string that is the frequency read.  It should be big enough to hold the frequency read (15 characters?).  "Wait" is non-zero to wait for the next (a "fresh") frequency count, or 0 to return the last frequency read.  The function returns a string of the frequency read or NULL. 
This function returns a string instead of an integer or floating point value so that floating point library functions are not required and gate times that are over one second and period measurements return the fractional part of the frequency read.  Be aware that if "Wait" is true then this function will not return until the gate time has passed and a "fresh" frequency count is available.  This could be up to 100 seconds.  When in a period measure mode, the period measured is converted to a frequency and that value is returned.  In this period measure mode, if the frequency is too high then '999999' is returned and if the frequency is too low (or 0Hz) or the software times out  then '0.00000' is returned.  The input signal must provide 2 (a complete wave), 11 or 101 transitions within the timeout period or '0.00000' is returned.  The timeout period is configurable and defaults to 5 seconds.

`long `**read**`(bool Wait)`  Read the frequency counter and return the value as a long.  Function returns the raw frequency read. <b>IT IS NOT SCALED FOR GATE TIME OR NUMBER OF AVERAGES!!</b>  It is presumed a higher level function will do this or use the string version of this function for a corrected value.  'Wait' is non-zero to wait for the next (a "fresh") frequency count, or 0 to return the last frequency read.

`byte `**available**`(void)`  Returns a non-zero (true) value if a new frequency count is available or zero if not.   This function returns immediately and can be used instead of calling FrequencyCounter::read with “Wait” true to check to see if a "fresh" count value is available. 

While not part of the class, another function is externally available if needed.  

`void `**FreqCtrGateISR**`(void)`  This function implements the "gating" function of the frequency counter.  It should be called each time a "gate time" has finished.  It moves the collected count to a holding register and restart the counting for the new "gate time".  This function should be called every 10 milliseconds (preferably by an accurate timer) when FreqCtrGate (mode) is not zero.  This is normally done in the ISR of one of the timers in the micro.  (This function is not needed if using the default library files as published)
 
While the basic user interface is via a class, only a single instance should be declared as this module uses specific hardware resources. 

## Internal Details
Input to the frequency counter is on Arduino Digital pin 6 [PD7] for Pro Micro (ATMega32U4) and is assumed to be a low going train of pulses to count.  Obviously the input signal must be an appropriate TTL level to be counted.  Input amplifiers to amplify lower level signal to this TTL level (if needed) are beyond the scope of this work. 

This module requires a "gating" source that is typically a different timer that occurs at a fixed time rate.  If not using the supplied "systimer" module then the function "FreqCtrGateISR" needs to be called at the proper periodic rate.  The accuracy of this module is directly affected by the accuracy of this gate timing source!

This module is designed for the Arduino Pro Micro module that uses an Atmel ATMega32U4, and uses Timer 0 for the counting input. It would be ideal to use another timer for this function, but unfortunately the ext clock pin for Timer1 (ATMega32U4 pin 26) is not routed to the modules connector and only Timer0 and Timer1 have an external clock input, so no alternative exists other than to use Timer0.  As is known, Timer0 is used for the Arduino system timing functions (delay, millisec, microsec, etc.), so the only alternative is to move these functions to some other timer.  This is done by reworking the stock wiring.c module that is part of the Arduino system to use a different timer for this purpose.  Then this "system" timer is expanded to also use it for the gate timer for the frequency counter function.   

Obviously, when using an alternate timer for the "system" timer function and frequency counter gating function, the timer will not be available for PWM functions or any other "built in" library functions that normally depend on Timer0 or the timer selected for the "system" timer in the Arduino software.

By simply including "SysTimer.cpp" in the build, the functions normally implemented by Timer0 (delay, millisec, microsec, etc.) are moved to use this alternate timer (configurable to Timer1 or Timer3), and a hook to the Timer's ISR is accessable, allowing this timer to be used for the frequency counter function.  

NOTE: It might be best to use Timer1 for the gate, because it has the   highest interrupt priority (even above Timer0), but either Timer1 or Timer3 is able to be used.   If using USB, then the count returned may not be as precise as when using other or no external communications methods.  This is because the USB interrupts have a higher priority than the Timer1 / Timer3 timer that is used for the gate timer.  A possible alternative would be to set up a timer for the gate time and output it on a pin and then use a pin change or external interrupt as the gate timer source.  (Pin change and external interrupts have a higher priority than the USB interrupts).  Also note that at frequencies above about 2MHz the count returned might be short a count or two.  This is because of the necessity of clearing the counter, turning it off and then back on which takes a couple of CPU cycles.  

This frequency counter module can also be set up to use external gating.  To do so, define 'FCEXTERN' as non-zero and include the module PCInterrupt.cpp in this sketch.  Then by setting FrequencyCounter::mode' to 6, the Arduino pin defined by 'FCEXTGATEMSK' will be used as the gate input.  This is configured to Arduino Digital pin 9 [PB5] in the supplied code, but can be changed to a number of other pins.  When activated, a low on this pin turns on the gate, and when hi the gate is turned off.  Including the external gate function is optional.
 
Using the external gate function and another timer set up to work autonomously and then output its signal on some other pin, and then connecting this pin to the Ext Gate input might be a way to get around the USB problem mentioned above that might affect the count, because the external gate inputs are all higher in priority than the USB interrupt are.
 
With the Arduino Pro Micro modules available during development the system was able to reliably count to frequencies of up to about 8MHz (1/2 of processor clock) in the frequency counter mode and about 20KHz in the period mode with averaging of 10 or 100 or 10KHz in the period mode with averaging of 1.
 
If it is desired to "trim" the Pro Micro's 16MHz oscillator to count or generate frequencies that are more accurate, a Knowles Voltronics JR400 trimmer capacitor (8-40pF) (or similar) can be used to replace the C2 capacitor on the module.  (You could also replace C2 with a 10pF cap and use a JR200 (4.5-20pF) for a more stable adjustment with less range).  This trimmer capacitor can be placed on top of the 32U4 chip and tiny wires (wire wrap?) used to connect the two connections of it to the Pro micro module.  Connect the rotor of the trimmer cap to GND (the '-' side of C19 is a good place) and the other side of the trimmer cap to the non-GND side of C2.  If the rotor side of the trimmer cap is connected to the C2 connection, the oscillator will not be as stable. Once wired, glue the trimmer cap to the top of the 32U4 chip.  Then set the frequency generator of the chip this is programmed on to 4MHz and using a reference frequency counter, tune the trimmer capacitor to exactly 4MHz. If frequency counter only is programmed, inject a 4MHz signal into the frequency counter input and tune to exactly 4MHz.

The repository also includes a documentation file that provides more details on the use of this library and also covers a companion frequency generator library.

