#include "Wiegand.h"
#include "HX711.h"

/*
 * voltage for leave distance from prox sensor: 1.5v
 */

// Define cat data:
typedef struct {
  float ration;       // Daily food in grams
  float ate;
  unsigned long code; // Wiegand ID code
} cat;

const int numCats = 2;
cat cats[numCats] = {
  { 200, 0, 9663453 }, // cat 0
  { 300, 0, 9409743 }  // cat 1 (need to update this with a real ID number)
};

// State variables:
bool doorOpen = false;
float weight, nextWeight;
unsigned long code;
int catID;    //ID of current cat

// Wiegand variables 
// (needs to be 2 and 3 on UNO since these are the only interruptable pins)
const int wgPinD0 = 2;
int wgPinIntD0 = digitalPinToInterrupt(wgPinD0);
const int wgPinD1 = 3;
int wgPinIntD1 = digitalPinToInterrupt(wgPinD1);
WIEGAND wg;

// HX711 load cell amplifier variables
float calibrationFactor = 200;  // Roughly worked out for 20kg load cell
long zeroFactor = 9688900;      // Just orange platform: 9688900
const int lcPinDout = 5;
const int lcPinClk = 4;
HX711 scale(lcPinDout, lcPinClk);

// Proximity sensor variables
#define pxPin A1
double leaveDist = 10;   // Distance indicating cat has left
int leaveTime = 1000;  // Wait time for closing in ms

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  scale.set_scale(calibrationFactor);
  scale.set_offset(zeroFactor);
  wg.begin(wgPinD0, wgPinIntD0, wgPinD1, wgPinIntD1);

  // Setup proximity input
  pinMode(pxPin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  // two loops, one for each state of the feeder
  while(!doorOpen) {
    if(wg.available()) {
      code = wg.getCode();
      Serial.print("RFID detected: ");
      Serial.println(code);
      for (int i = 0; i < numCats; i++) {
        if (code = cats[i].code) {
          catID = i;
          break;
        }
      }
      if(cats[catID].ate < cats[catID].ration) {
        Serial.println("Opening feeder");
        doorOpen = true;
        weight = scale.get_units();
      }
    }
    // delay(100);
  }
  int timeElapsed = 0;
  while(doorOpen) {
    uint16_t value = analogRead(pxPin);
    double dist = get_IR(value);

    // Check proximity sensor and counter to see if cat has left
    if(dist > leaveDist) {
      if(timeElapsed > leaveTime) {
        Serial.println("Closing door");
        doorOpen = false;
        break;
      }
      timeElapsed = timeElapsed + 100;
    } else {
      timeElapsed = 0;
    }

    // Check weight (for testing)
    nextWeight = scale.get_units();
    if((weight-nextWeight) > 5.0) {
      Serial.print("Weight change: ");
      weight = nextWeight;
      Serial.println(weight);
    }
    
//  Check for other cat:
//   if(wg.available) {
//      code = wg.getCode();
//      if(code != cats[catID].code){
//        Serial.println("Unauthorized cat. Closing door")
//        delay(500);
//        cats[catID].ate = weight - scale.get_units();
//        Serial.print("Cat ate: ");
//        Serial.println(cats[catID].ate);
//        doorOpen = false;
//        break;
//      }
//    }
    delay(100);
  }
}


//return distance (cm)
double get_IR (uint16_t value) {
        if (value < 16)  value = 16;
        return 2076.0 / (value - 11.0);
}
