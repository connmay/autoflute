#include "frequency.h"
#include "Config.h"


enum class State {
  findPosSlope = 0,
  findNegSlope,
  secondPeak,
};

float const                 // returns interpolated peak adjustment compared to peak location
  _quadInterpAdj( int32_t const left,   // correlation value left of the peak
                  int32_t const mid,    // correlation value at the peak
          int32_t const right ) // correlation value right of the peak
  {
    double const adj = (double)0.5 * (right - left) / (2. * mid - left - right);
    return adj;
  }

int32_t const _autoCorr( char * const samples, uint_least16_t const lag ) { // (normalized) auto correlation result, pointer to signed 8-bit data samples, [in] lag
  // samples[ii] * samples[ii+lag], results in an int16 term
  // sum += term, results in an int32
  // To keep the sum to an int16, each time the term could be divided by nrOfSamples.
  //   to make the division faster, I would round nrOfSamples up to a 2^n boundary. (2BD)

  int32_t ac = 0;

  for ( uint_least16_t ii = 0; ii < Config::WINDOW_SIZE - lag; ii++ ) {
    ac += ((int32_t)samples[ii] * samples[ii + lag]);
  }
  return ac;
}

float const calculateFreq( char * const  samples ) { // returns frequency found, 0 when not found [out], samples = pointer to signed 8-bit data samples [in]
  float period = 0;

  if ( samples ) {
    // Search between minimum and maximum frequencies (sampleRate/lagMax, sampleRate/lagMin).
    // For 150 samples and a 9615 S/s, this corresponds to [512 .. 3846 Hz]
    uint_least16_t const lagMin = Config::LAG_MIN; // SAMPLE_LAG_MIN;
    uint_least16_t const lagMax = Config::LAG_MAX;

    // determine threshold below we ignore peaks
    int32_t const acMax = _autoCorr( samples, 0 );  // initial peak = measure of the energy in the signal
    int32_t const acThreshold = (float)acMax * 1 / 2;    // empirical value
    int32_t acPrev = acMax;
    State state = State::findPosSlope;   // ensure C++11 is enabled

    for ( uint_least16_t lag = lagMin; (lag < lagMax) && (state != State::secondPeak); lag++ ) {

      // unnormalized autocorrelation for time "lag"
      int32_t ac = _autoCorr( samples, lag );

      // find peak after the initial maximum
      switch ( state ) {
        case State::findPosSlope:
          if ( (ac > acPrev) ) {
            state = State::findNegSlope;
          }
          break;
        case State::findNegSlope:
          if ( ac <= acPrev ) {
            state = State::secondPeak;
              period = lag - 1 + _quadInterpAdj( _autoCorr( samples, lag - 2 ), acPrev, ac );
          }
          break;
      }
      acPrev = ac;
    }
  }
  float const f = Config::SAMPLE_RATE / period;

  return (period > 0 && f < Config::FREQ_MAX && f > Config::FREQ_MIN) ? f : 0;
}

