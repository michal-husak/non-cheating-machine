#include <EEPROM.h>
#include <Bounce2.h>
#include "TM1637.h"

#define CLK 3      
#define DIO 4
TM1637 tm1637(CLK, DIO);

#define BUZZER 2

#define NUM_OF_BUTTONS 5
#define PAUSE_BUTTON_INDEX 0
#define RESTART_BUTTON_INDEX 1
#define UP_BUTTON_INDEX 2
#define DOWN_BUTTON_INDEX 3
#define SAVE_BUTTON_INDEX 4

const uint8_t BUTTON_PINS[NUM_OF_BUTTONS] = {5, 6, 7, 8, 9};

unsigned long timer = millis();
byte workoutLength = 40;
byte pauseLength = 15;
byte storedWorkoutLength = 0;
byte storedPauseLength = 0;
byte workoutCounter = 0;
byte pauseCounter = 0;
bool isWorkout = false;
bool paused = true;

uint8_t previousButtonStates[NUM_OF_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH};
uint8_t buttonStateChanges[NUM_OF_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH};

Bounce * buttonDebouncers = new Bounce[NUM_OF_BUTTONS];

void setup() {

  Serial.begin(9600);

  for (int i = 0; i < NUM_OF_BUTTONS; i++) {
    buttonDebouncers[i].attach(BUTTON_PINS[i], INPUT_PULLUP);
    buttonDebouncers[i].interval(50);
  }

  // reading lengths from memory
  storedWorkoutLength = EEPROM.read(0);
  storedPauseLength = EEPROM.read(1);
  workoutLength = storedWorkoutLength == 0 ? workoutLength : storedWorkoutLength;
  pauseLength = storedPauseLength == 0 ? pauseLength : storedPauseLength;
  workoutCounter = workoutLength;
  pauseCounter = pauseLength;
  
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  
}
 
void loop() {

  displayCurrentTime();
  readButtons();
  resolveInputChanges();
  
  if(!paused) {
    runTimer();
  }
}

void readButtons() {

  for (int i = 0; i < NUM_OF_BUTTONS; i++)  {   
    buttonDebouncers[i].update();
    int currentButtonState = buttonDebouncers[i].read();

    if(previousButtonStates[i] != currentButtonState) {
      if(currentButtonState == LOW) {
        buttonStateChanges[i] = LOW;
      }
    }
    previousButtonStates[i] = currentButtonState;  
  }
  
}

void resolveInputChanges() {
  
  for (int i = 0; i < NUM_OF_BUTTONS; i++)  {   
    if(buttonStateChanges[i] == LOW) {
      functionTriggerSwitch(i);
      buttonStateChanges[i] = HIGH; 
    }
  }
  
}

void runTimer() {
  
  if (millis() - timer >= 1000) {
    
    timer += 1000;
    
    if(isWorkout) {
      workoutCounter--;
      if (workoutCounter == 0) {
        workoutCounter = workoutLength;
        isWorkout = false;
      }
    } else {
      pauseCounter--;
      if (pauseCounter == 0) {
        pauseCounter = pauseLength;
        isWorkout = true;
      }
    }
    warningBeeps();
    displayCurrentTime();
  }
}

void functionTriggerSwitch(int buttonIndex) {
  
  switch(buttonIndex) {
    
    case PAUSE_BUTTON_INDEX:
      togglePause();
      break;
      
    case RESTART_BUTTON_INDEX:
      restart();
      break;

    case UP_BUTTON_INDEX:
      timeChange(1);
      break;
      
    case DOWN_BUTTON_INDEX:
      timeChange(-1);
      break;

    case SAVE_BUTTON_INDEX:
      saveLengths();
      break;
  }
}

void togglePause() {
  if(paused) {
    timer = millis();
  }
  paused = paused ? false : true;
}

void restart() {
  workoutCounter = workoutLength;
  pauseCounter = pauseLength;
  if(paused) {
    displayCurrentTime();
  }
}

void timeChange(int change) {
  if(paused) {
    if(isWorkout) {
      workoutLength += change;
      workoutCounter = workoutLength;
    } else {
      pauseLength += change;
      pauseCounter = pauseLength;
    }
    displayCurrentTime();
  }
}

void saveLengths() {
    if(storedWorkoutLength != workoutLength) {
      EEPROM.write(0, workoutLength);
      storedWorkoutLength = workoutLength;
      Serial.print("workoutLength stored: ");
      Serial.println(workoutLength);
    }
    if(storedPauseLength != pauseLength) {
      EEPROM.write(1, pauseLength);
      storedPauseLength = pauseLength;
      Serial.print("pauseLength stored: ");
      Serial.println(pauseLength);
    }
}

void displayCurrentTime() {
  if(isWorkout) {
    displaySeconds(workoutCounter);
  } else {
    displaySeconds(pauseCounter);
  }
}

// display the number on the most two right digits on the display
void displaySeconds(int seconds) {
  int ones = getDigit(seconds, 1);
  int tenths = getDigit(seconds, 10);
  int hunderts = getDigit(seconds, 100);
  if(hunderts != 0) {
    tm1637.display(1, hunderts);
  }
  tm1637.display(2, tenths);
  tm1637.display(3, ones);
}

// extracts a digit from the integer on specific position - 
// getDigit(2564, 1)    = 4
// getDigit(2564, 10)   = 6
// getDigit(2564, 100)  = 5
// getDigit(2564, 1000) = 2
int getDigit(int number, int decimal) {
  int digit = (number / decimal) % 10;
  return digit;
}

void warningBeeps() {
  if (workoutCounter <= 5 || pauseCounter <= 5) {
    if(workoutCounter == 1 || pauseCounter == 1) {
      tone(BUZZER, 4000, 700);
    } else {
      tone(BUZZER, 3000, 200);
    }
  }
}
