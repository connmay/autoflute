#include "setup.h"
#include "Config.h"
#include <Arduino.h>


void timer_setup( void ) {
  pmc_enable_periph_clk (TC_INTERFACE_ID + 0*3 + 1) ; // clock the TC0 channel 1

  TcChannel * t = &(TC0->TC_CHANNEL)[1] ;    // pointer to TC0 registers for its channel 1
  t->TC_CCR = TC_CCR_CLKDIS ;  // disable internal clocking while setup regs
  t->TC_IDR = 0xFFFFFFFF ;     // disable interrupts
  t->TC_SR ;                   // read int status reg to clear pending

  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |   // use TCLK1 (prescale by 2, = 42MHz)
              TC_CMR_WAVE |                  // waveform mode
              TC_CMR_WAVSEL_UP_RC |          // count-up PWM using RC as threshold
              TC_CMR_EEVT_XC0 |     // Set external events from XC0 (this setup TIOB as output)
              TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR |
              TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR ;
  t->TC_RC =  Config::ADC_TIMER_OVERFLOW_CNT;     // counter resets on RC, so sets period in terms of 42MHz clock 
  t->TC_RA =  Config::ADC_TIMER_RA;     // roughly square wave
  t->TC_CMR = (t->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET ;  // set clear and set from RA and RC compares
  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG ;  // re-enable local clocking and switch to hardware trigger source.
}
void ADC_setup( void ) {
  NVIC_EnableIRQ (ADC_IRQn); // enable ADC interrupt vector

  ADC->ADC_IDR = 0xFFFFFFFF ; // disable interrupts
  ADC->ADC_IER = 0x80 ;        // enable AD7 End-Of-Conv interrupt (Arduino pin A0)
  ADC->ADC_CHDR = 0xFFFF ;      // disable all channels
  ADC->ADC_CHER = 0x80 ;         // enable just A0
  ADC->ADC_CGR = 0x15555555 ;   // All gains set to x1
  ADC->ADC_COR = 0x00000000 ; // All offsets off

  ADC->ADC_MR = (ADC->ADC_MR & 0xFFFFFFF0) | 2<<1 | ADC_MR_TRGEN ;  // 2 = trig source TIO from TC1
  
}


