/*Autoflute code v. 

#include "setup.h" //function declarations for the adc_setup() and timer_setup(), used to configure sample frequency 
#include "Config.h" //definition of global variables, use to easily modify variables which are used in more than one location 
#include "microphone.h" //
#include "frequency.h"

/******************************/
/* Motor driver pins */
const int Mode1 = 16; 
const int Mode2 = 22;
const int nSleep = 18; 
const int Phase = 19;
const int Enable = 2;
const int nFault = 14;
/************************/
/************************/
/* Motor command states*/
bool phase = 0;
bool isEnable = false;
/************************/
/************************/
/* Digital pin assignments for endstops*/
const int stopRight = 6;
const int stopLeft = 7;
/************************/
/************************/
/*Mode control booleans*/
bool agent = false;
bool avgCheck = false;
bool delayCheck = false;
bool isFreqMet = false;
/************************/
/************************/
/*Frequency variables*/
double freq_int = 0; //current detected frequency
double inputFreq = 0; // setpoint
int16_t diff = 0; //difference between setpoint and current frequency
/************************/
/************************/
/*Moving average variables*/
float freqAvg1[Config::MVN_AVG1_SIZE];
float avgHolder1 = 0;
float freq_sum;
int j = 0;
/************************/
/************************/

char* inputBuffer = new char[20]; 

char * globalSamples; //pointer to the memory location where the samples are stored

//Analog A0 pin as microphone input
enum analogInPins {
  MIC_PORT = 0
};

void setup() {
  /*Defines I/O mode of all digital pins*/
  pinMode (Mode1, OUTPUT);
  pinMode (Mode2, OUTPUT);
  pinMode (Phase, OUTPUT);
  pinMode (Enable, OUTPUT);
  pinMode (nSleep, OUTPUT);
  pinMode(stopRight, INPUT_PULLUP);
  pinMode(stopLeft,  INPUT_PULLUP);
  digitalWrite (nSleep, HIGH);

  /*Defines the interrupt function to be used when the carriage hits the end stops
   * see the following page for more info: https://www.arduino.cc/en/Reference/attachInterrupt */
   
  attachInterrupt(stopRight, rightEndStop, FALLING );
  attachInterrupt(stopLeft, leftEndStop, FALLING );
  
  Serial.begin(Config::SERIAL_RATE); //begin serial communication at a baud rate specified by the config.h header 
  
  digitalWrite(Enable, LOW); //ensure the motor doesn't move initially

  /*Setup functions to configure timer and analog to digital converter, see setup.cpp for more details */
  timer_setup();
  ADC_setup();
  
  globalSamples = mic_begin( MIC_PORT ); //function to begin the first data collection

  for (int i = 0; i < Config::MVN_AVG1_SIZE; i++) {
    freqAvg1[i] = 0;
  }
}
void loop() {
  uint8_t amplitude; //variable to check if there was distortion of the signal due to the signal being out of range 
  globalSamples = getSamples( &amplitude ); //fetch samples stored by the mic_update function

  float freq = calculateFreq( globalSamples ); //calculate the frequency stored in the globalSamples array using the auto-correlation algorithim

  (void)mic_update(); //samples no longer needed, update the sample array with new samples

  freq_int = (int16_t)(freq - 5.5);                     //shift the frequency by 5.5 Hz because it was found that it was consistently over-shooting the actual frequency
  if (delayCheck == false) delay(15);                   //always need some delay to make sure the samples in the buffer are overwritten correctly and to smooth everything out
  inputFreq = getNote(inputFreq);                       //determines which note is desired, see getNote function below for more details
  if (freq_int < 300 || freq_int > 1200) freq_int = 0;  //force detected frequency to 0 if out of range of notes producible by flute

  diff = inputFreq - freq_int; //calculate difference between desired and detected frequency 
  if (diff < 0) phase = 1; //
  else phase = 0;
  diff = abs(diff); //already have information about direction, take absolute value to simplify case structures
  if (freq_int != 0) avgHolder1 = mvn_avg1(diff); //store a moving average of the data, used to determine when it switches to 'fine adjustement' mode
  
  if (agent == false) {    //big adjustement mode, always enters at least once (agent set to false each time a new frequency is inputted
    delayCheck = false; 
    if (diff > Config::rangeSmall && freq_int != 0 && inputFreq != 0) isEnable = 1;
    else if (freq_int != 0 && inputFreq != 0) {
      isEnable = 0;
      if (avgHolder1 < Config::rangeSmall) { //switch to fine adjustement mode if avgHolder is smaller than the rangeSmall variable specified in the config file
        agent = true;
      }
    }
    else isEnable = 0; //if no frequency is detected or the input frequency is 0, don't make any movement

    //COMMANDS SENT TO MOTOR
    digitalWrite(Phase, phase); 
    digitalWrite(Enable, isEnable);

    //Used to ensure that the motor moves a little bit in the direction required, helpful in situations when the next input frequency is really close to the current
    if (agent == true){
      digitalWrite(Phase, phase);
      digitalWrite(Enable, HIGH);
      delayMicroseconds(17000);
      avgHolder1 = 1.1*Config::rangeFine; //reset avgHolder1 to a value
    }
  }
  //Fine adjustment mode
  if (agent == true) {
    delayCheck = true; //turns off 'global' delay in main loop, delay occurs naturally instead from delaying Microseconds for sudo-PWM
    
    if (isFreqMet == true) digitalWrite(Enable, LOW);
    else if ((avgHolder1 > Config::rangeFine) && (freq_int != 0)) {
      digitalWrite(Phase, phase);
      digitalWrite(Enable, HIGH);
      delayMicroseconds(8000);
      digitalWrite(Enable, LOW);
      delayMicroseconds(1000);
    }
    else if ((avgHolder1 <= Config::rangeFineFine)) { //prevents further movement if moving average below a certain thershold
      isFreqMet = true; //
    }
  }
}


void rightEndStop() { //end stop functions, prevents hard collisions with track
  delayMicroseconds(5000);
  while (digitalRead(stopRight) == 0) {
    phase = 0;
    digitalWrite (Phase, phase);
    digitalWrite  (Enable, HIGH);
    delayMicroseconds(15000);
  }
  digitalWrite(Enable, LOW);
}

void leftEndStop() {
  delayMicroseconds(5000);
  while (digitalRead(stopLeft) == 0) {
    phase = 1;
    digitalWrite(Phase, phase);
    digitalWrite (Enable, HIGH);
    delayMicroseconds(10000);
  }
  digitalWrite(Enable, LOW);
}


int16_t mvn_avg1(int16_t newValue) { //moving average function to store in avgHolder1
  freq_sum -= freqAvg1[j];
  freqAvg1[j] = newValue;
  freq_sum += freqAvg1[j];
  j = (j + 1) % Config::MVN_AVG1_SIZE;
  return freq_sum / Config::MVN_AVG1_SIZE;
}
uint8_t readCommandRaw() { //reads command and returns contents
  uint8_t i = 0;
  while (Serial.available()) {
    if (i < 20) {
      inputBuffer[i++] = Serial.read();
      delayMicroseconds(500);
    } else {
      Serial.read();
    }
  }
  return i;
}

double getValue(uint8_t messageLength, uint8_t startSpot) { //used to process value in getNote function
  double val = 0;
  inputBuffer[startSpot + messageLength] = 0;
  return atof(&(inputBuffer[startSpot]));
}

double getNote(double currentFreq) { //enables user to enter in notes from laptop keyboard as if the keys were on a piano. starting point: 'a' = 440 Hz = A4 musical note
  if (Serial.available()) {
    int messageLength = readCommandRaw();
    float frequency;
    switch (inputBuffer[0]) {
      case 'z':
        frequency = getValue(messageLength, 1);
        break;
      case 'a':
        frequency = 440;
        break;
      case 'w':
        frequency = 466;
        break;
      case 's':
        frequency = 494;
        break;
      case 'd':
        frequency = 523;
        break;
      case 'r':
        frequency = 554;
        break;
      case 'f':
        frequency = 587;
        break;
      case 't':
        frequency = 622;
        break;
      case 'g':
        frequency = 659;
        break;
      case 'h':
        frequency = 698;
        break;
      case 'u':
        frequency = 740;
      case 'j':
        frequency = 784;
        break;
      case 'i':
        frequency = 831;
      case 'k':
        frequency = 880;
        break;
      case 'o':
        frequency = 932;
        break;
      case 'l':
        frequency = 988;
        break;
      default:
        frequency = currentFreq;
    }
    agent = false;
    isFreqMet = false;
    avgCheck = false;
    delayCheck = false;
    avgHolder1 = Config::rangeFine;
    return frequency;
  }
}



