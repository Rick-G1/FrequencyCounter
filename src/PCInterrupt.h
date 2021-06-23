/******************************************************************************/
/*                                                                            */
/*                   E M B E D D E D   C O N T R O L L E R                    */
/*                                                                            */
/*                     PC Interrupt (pin change interrupts)                   */
/*                                                                            */
/*                    C I R C U I T   C H E C K,  I N C .                     */
/*                                                                            */   
/*         Copyright (c) 2014  Circuit Check, Inc.   All rights reserved.     */
/*                                                                            */
/*                      CONFIDENTIAL AND PROPRIETARY                          */
/*                                                                            */
/*    THIS WORK CONTAINS VALUABLE, CONFIDENTIAL AND PROPRIETARY INFORMATION   */
/*    OF THE AUTHOR.   DISCLOSURE,  USE OR PRODUCTION WITHOUT THE WRITTEN     */
/*    AUTHORIZATION OF THE AUTHOR IS STRICTLY PROHIBITED.    THIS WORK BY     */
/*    THE AUTHOR IS PROTECTED BY THE LAWS OF THE UNITED STATES AND OTHER      */
/*    COUNTRIES.                                                              */
/*                                                                            */
/*    PROCESSOR:  Atmel ATmega32U4         COMPILER: Arduino / GNU C for AVR  */
/*                                                                            */
/*    Written by: Rick Groome 2014                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#ifndef _PCINTERRUPT_H
#define _PCINTERRUPT_H

#include <arduino.h>

// Pin numbers for each bit position (e.g. arduino pin number)
// Use these constants for 'mask' values below in the class.
#define PCINTMASK8  0x10	/* Digital 8  [PB4] */ 
#define PCINTMASK9  0x20	/* Digital 9  [PB5] */ 
#define PCINTMASK10 0x40	/* Digital 10 [PB6] */ 
#define PCINTMASK14 0x01	/* Digital 14 [PB0] */ 
#define PCINTMASK15 0x02	/* Digital 15 [PB1] */ 
#define PCINTMASK16 0x04	/* Digital 16 [PB2] */ 
#define PCINTMASK17 0x08	/* Digital 17 [PB3] */ 
#define PCINTMASK7  0x80	/* Digital 7  [PE6] (use this instead of PB7) */ 

// Use rising/falling/change to determine if a Pin Change interrupt occured.
// e.g      if (PCH.rising(PCINTMASK10)) { dosomething } 
// Then don't forget to call PCH.clear(PCINTMASK10);  

class PCInterrupt
{
  public:
    boolean falling(byte mask);  // Return true if falling interrupt detected on mask bit
    boolean rising(byte mask);   // Return true if rising interrupt detected on mask bit
    boolean change(byte mask);   // Return true if any change detected on mask bit
    void    clear(byte mask);    // Clear the interrupt bits in mask
    void    enable(byte mask);   // Enable interrupts on pins in mask
    void    disable(byte mask);  // Disable interrupts on pins in mask
};

extern PCInterrupt PCH;   // This is the one (and only) instance of this class
// You can rename this to whatever you like, if desired. 

// If you define this function, then it will be called as part of 
// the pin change interrupt service routine
extern "C" { void PCChangeIntFunc(byte Changes[]); }

#endif  //_PCINTERRUPT_H


