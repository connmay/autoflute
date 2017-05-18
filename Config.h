//easily change values which are used in multiple locations of code by just chaning it once here

namespace Config {
    unsigned long const SERIAL_RATE = 115200;           // [baud]
    unsigned long const ADC_TIMER_OVERFLOW_CNT = 2100; //42MHz/2.1k = 20 kHz Sample_rate
     unsigned long const ADC_TIMER_RA = ADC_TIMER_OVERFLOW_CNT/2; //honestly not sure what this is used for, check atmel uC data sheet if curious
    uint_least16_t const SAMPLE_RATE = 42000000/ADC_TIMER_OVERFLOW_CNT;    // 
    uint_least16_t const WINDOW_SIZE = 500;             
    
    // values below are calculated
    uint_least16_t const LAG_MIN = 1;                       // [samples] (not seconds!), 10 samples gives a +/-5% error rate (before interpolation)
    uint_least16_t const LAG_MAX = WINDOW_SIZE/2;         // [samples] (not seconds!), at least two waveforms 
    float const FREQ_MIN = SAMPLE_RATE / LAG_MAX;  
    float const FREQ_MAX = SAMPLE_RATE / LAG_MIN; // 96 Hz @ 9615 S/s       // 1602 Hz @ 9615 S/s and 6 min lag
    
    uint8_t const MVN_AVG1_SIZE = 8;
    uint32_t const rangeSmall = 60;
    uint32_t const rangeFine = 10;
    uint32_t const rangeFineFine = 30;
}

