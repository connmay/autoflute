#pragma once

#include <Arduino.h>
#include <stdint.h>


char * mic_begin( uint8_t const port );                     // 0 for A0
char * const updateSamples( void );                       // returns pointer to where new samples will be stored if successful
char * const getSamples( uint8_t * const amplitudePtr );  // returns pointer to array of data samples, NULL on failure
char * const mic_update( void );
