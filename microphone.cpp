#include "microphone.h"
#include "Config.h"
#define SCHAR_MAX 127
#define SCHAR_MIN -128

char volatile isSamples = 0;
char volatile *isS = &isSamples;

struct filevariables_t {
  uint8_t     analogPort;
  uint8_t     prescaler;
  char *  samples;  // updated by ISR
} static fv;  // file variables scope variables

 
struct amplitudeRange_t {
  char min;
  char max;
};

struct isr_t {
  uint_least16_t ii;
  amplitudeRange_t range;
};

isr_t volatile _isr = {  // data updated while interrupt enabled
  0, //_isr.ii = 0
  {SCHAR_MAX, SCHAR_MIN} //_isr.min = SCHAR_MAX, _isr.max = SCHAR_MAX
};
//SCHAR_MAX = Maximal value which can be stored in a signed char variable.
//SCHAR_MIN = Minimal value which can be stored in a signed char variable.

#if SHOW_SAMPLES
void _plotSamples( char * const    samples, sampleCnt_t const  nrOfSamples ) {        // pointer to 8-bit data samples [in] number of data samples [in]
  for ( uint32_t ii = 0; ii < nrOfSamples; ii++ ) {
    int16_t d = (int16_t)samples[ii] - SCHAR_MIN;
    while ( d > 0 ) {
      Serial.print ( " " );
      d = d - 4;
    }
    Serial.println( "*" );
    //Serial.println ( samples[ii], DEC );
  }
}
#endif

char * mic_begin( uint8_t const port ) {
  fv.analogPort = port; //assigns input port to the file variable
  fv.samples = new char[Config::WINDOW_SIZE]; //creates a new array of sample_t type (char, 8bit) and assigns fv.samples to the pointer address to the first byte of this block
  mic_update();
  return fv.samples;
}

char * const mic_update( void ) {
  ::isr_t volatile * const isr = &_isr; //creates a pointer to
  isr->ii = 0; //same as _isr.ii = 0;
  isr->range.min = SCHAR_MAX; //same as _isr.range.min = SCHAR_MAX
  isr->range.max = SCHAR_MIN; //same as _isr.range.max = SCHAR_MIN

  ADC_MR_TRGEN_EN; //enable hardware triggers (i.e. the timer) to cause the ADC_ISR to run

  return 0;
}

char * const getSamples( uint8_t * const amplitudePtr ) {
  // wait until all samples are available

  while ( isSamples == 0 ) {
  }
#if SHOW_SAMPLES
  _plotSamples ( fv.samples, SAMPLE_COUNT );
#endif
  ::isr_t volatile * const isr = &_isr;

  bool clipping = (isr->range.max == SCHAR_MIN) || (isr->range.max == SCHAR_MAX);
  uint8_t amplitude = (int16_t)isr->range.max - isr->range.min; // top-top [0..255]

  *amplitudePtr = amplitude / 2; // range [0..127]
  *isS = 0;
  return fv.samples; 
}

void ADC_Handler(void) {
  if (ADC->ADC_ISR & ADC_ISR_EOC7) {
    ::isr_t volatile * const isr = &_isr;
    char const s = (char)(*(ADC->ADC_CDR + 7) >> 4); 

    // remove voltage bias by changing range from [0..255] to [-128..127]

    if ( isr->ii == 0 ) {
      isr->range.min = SCHAR_MAX;
      isr->range.max = SCHAR_MIN;
    }
    if ( s < isr->range.min ) { //if current sample is lower in amplitude than the previous range.min, update range.min to current sample
      isr->range.min = s;
    }
    if ( s > isr->range.max ) { //if current sample is higher in amplitude than the previous range.max, update range.max to current sample
      isr->range.max = s;
    }
    if ( isr->ii < Config::WINDOW_SIZE ) { //if current sample number is smaller than the max windows size, increment the current sample number
      fv.samples[isr->ii++] = s;
    }
    else {
      ADC_MR_TRGEN_DIS; //Hardware trigger selected by TRGSEL field is disabled.
      *isS = 1;// else we're done sampling, disable ADC and wait for next call of mic_update();
    }
  }
}

