
/** ************************************************************************
 * File Name: Enabled_Controller_Mouse_Mover_Software.ino 
 * Title: Enabled Controller Mouse Mover Software
 * Developed by: Milad Hajihassan
 * Version Number: 1.2 (06/5/2023)
 * Github Link: https://github.com/milador/Enabled_Controller
 ***************************************************************************/

#include "Mouse.h"
#include <StopWatch.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#include <FlashStorage.h>


//Can be changed based on the needs of the users
#define MOUSE_MOVER_XY 15                                             //Mouse move 
#define SWITCH_REACTION_TIME 50                                       //Minimum time for each switch action ( level 10 : 1x50 =50ms , level 1 : 10x50=500ms )
#define SWITCH_MODE_CHANGE_TIME 2000                                  //How long to hold switch 4 to change mode 

#define LED_BRIGHTNESS 150                                             //The mode led color brightness which is always on ( Use a low value to decrease power usage )
#define LED_ACTION_BRIGHTNESS 150                                      //The action led color brightness which can be a higher value than LED_BRIGHTNESS

//Define Mouse Actions
#define MOUSE_NO_ACTION 0 
#define MOUSE_MOVER_X 1
#define MOUSE_MOVER_Y 2
#define MOUSE_MOVER_LEFT_CLICK 3
#define MOUSE_MOVER_RIGHT_CLICK 4
#define MOUSE_MOVER_Y_UP 5
#define MOUSE_MOVER_X_RIGHT 6
#define MOUSE_MOVER_Y_DOWN 7
#define MOUSE_MOVER_X_LEFT 8

//Define Switch pins
#define LED_PIN 5
#define DOTSTAR_DATA_PIN    41
#define DOTSTAR_CLOCK_PIN   40

#define SWITCH_A_PIN 10
#define SWITCH_B_PIN 11
#define SWITCH_C_PIN 12
#define SWITCH_D_PIN 13

#define SWITCH_UP_PIN 7
#define SWITCH_RIGHT_PIN 9
#define SWITCH_DOWN_PIN A1
#define SWITCH_LEFT_PIN A0

// Variable Declaration

//Declare switch state variables for each switch
int switchAState;
int switchBState;
int switchCState;
int switchDState;

int switchUpState;
int switchRightState;
int switchDownState;
int switchLeftState;

bool previousSwitchAState = HIGH;
bool previousSwitchBState = HIGH;
bool previousSwitchCState = HIGH;
bool previousSwitchDState = HIGH;

bool previousSwitchUpState = HIGH;
bool previousSwitchRightState = HIGH;
bool previousSwitchDownState = HIGH;
bool previousSwitchLeftState = HIGH;

bool moverState[9] = {true,true,true,true,true,true,true,true};

//Stopwatches array used to time switch presses
StopWatch timeWatcher[3];
StopWatch switchDTimeWatcher[1];

//Declare Switch variables for settings 
int switchConfigured;
int switchReactionTime;
int switchReactionLevel;
int switchMode;

int morseReactionTime;

FlashStorage(switchConfiguredFlash, int);
FlashStorage(switchReactionLevelFlash, int);
FlashStorage(switchModeFlash, int);


//RGB LED Color code structure 

struct rgbColorCode {
    int r;    // red value 0 to 255
    int g;   // green value
    int b;   // blue value
 };

//Color structure 
typedef struct { 
  uint8_t colorNumber;
  String colorName;
  rgbColorCode colorCode;
} colorStruct;

 //Mode structure 
typedef struct { 
  uint8_t modeNumber;
  String modeName;
  uint8_t modeColorNumber;
} modeStruct;


 //Switch structure 
typedef struct { 
  uint8_t switchNumber;
  String switchName;
  uint8_t switchMouse;
  uint8_t switchColorNumber;
} switchStruct;


 //Settings Action structure 
typedef struct { 
  uint8_t settingsActionNumber;
  String settingsActionName;
  uint8_t settingsActionColorNumber;
} settingsActionStruct;

//Color properties 
const colorStruct colorProperty[] {
    {1,"Green",{0,50,0}},
    {2,"Pink",{50,00,20}},
    {3,"Yellow",{50,50,0}},    
    {4,"Orange",{50,20,0}},
    {5,"Blue",{0,0,50}},
    {6,"Red",{50,0,0}},
    {7,"Purple",{50,0,50}},
    {8,"Teal",{0,128,128}}       
};

//Switch properties 
const switchStruct switchProperty[] {
    {1,"Move-Y",MOUSE_MOVER_Y,5},                             
    {2,"Move-X",MOUSE_MOVER_X,6},                           
    {3,"Click-Left",MOUSE_MOVER_LEFT_CLICK,1},                             
    {4,"Click-Right",MOUSE_MOVER_RIGHT_CLICK,3},
    {5,"Move-Y-Up",MOUSE_MOVER_Y_UP,5},                             
    {6,"Move-X-Right",MOUSE_MOVER_X_RIGHT,6},                           
    {7,"Move-Y-Down",MOUSE_MOVER_Y_DOWN,5},                             
    {8,"Move-X-Left",MOUSE_MOVER_X_LEFT,6}                         
};



//Settings Action properties 
const settingsActionStruct settingsProperty[] {
    {1,"Increase Reaction",5},                             //{1=Increase Reaction,5=blue}
    {2,"Decrease Reaction",6},                             //{2=Decrease Reaction,6=red}
    {3,"Max Reaction",1},                                  //{3=Max Reaction,1=green}
    {4,"Min Reaction",1}                                   //{4=Min Reaction,1=green}
};

//Mode properties 
const modeStruct modeProperty[] {
    {1,"Mouse Mover",8},
    {2,"Settings",4}
};

//Setup NeoPixel LED
Adafruit_NeoPixel ledPixels = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_DotStar dotStar(1, DOTSTAR_DATA_PIN, DOTSTAR_CLOCK_PIN, DOTSTAR_BRG);


void setup() {

  ledPixels.begin();                                                           //Start NeoPixel
  Serial.begin(115200);                                                        //Start Serial
  Mouse.begin();                                                               //Starts mouse emulation
  delay(1000);
  switchSetup();                                                               //Setup switch
  delay(5);
  initLedFeedback();                                                          //Led will blink in a color to show the current mode 
  delay(5);
  displayFeatureList();
  
  //Initialize the switch pins as inputs
  pinMode(SWITCH_A_PIN, INPUT_PULLUP);    
  pinMode(SWITCH_B_PIN, INPUT_PULLUP);   
  pinMode(SWITCH_C_PIN, INPUT_PULLUP);    
  pinMode(SWITCH_D_PIN, INPUT_PULLUP); 
   
  pinMode(SWITCH_UP_PIN, INPUT_PULLUP);    
  pinMode(SWITCH_RIGHT_PIN, INPUT_PULLUP);   
  pinMode(SWITCH_DOWN_PIN, INPUT_PULLUP);    
  pinMode(SWITCH_LEFT_PIN, INPUT_PULLUP); 

  //Initialize the LED pin as an output
  pinMode(LED_PIN, OUTPUT);                                                      


};

void loop() {
  static int ctr;                          //Control variable to set previous status of switches 
  unsigned long timePressed;               //Time that switch one or two are pressed
  unsigned long timeNotPressed;            //Time that switch one or two are not pressed
  
  //Update status of switch inputs
  switchAState = digitalRead(SWITCH_A_PIN);
  switchBState = digitalRead(SWITCH_B_PIN);
  switchCState = digitalRead(SWITCH_C_PIN);
  switchDState = digitalRead(SWITCH_D_PIN);
  switchUpState = digitalRead(SWITCH_UP_PIN);
  switchRightState = digitalRead(SWITCH_RIGHT_PIN);
  switchDownState = digitalRead(SWITCH_DOWN_PIN);
  switchLeftState = digitalRead(SWITCH_LEFT_PIN);
  
  timePressed = timeNotPressed  = 0;       //reset time counters
    if (!ctr) {                              //Set previous status of switch four 
      previousSwitchDState = HIGH;  
      ctr++;
    }
    
    if (switchAState == LOW){                                          // Switch A was pressed
      if (previousSwitchAState == LOW) { 
        mouseMoverAction(switchProperty[0].switchMouse,moverState[0],MOUSE_MOVER_XY,MOUSE_MOVER_XY);
      }
      previousSwitchAState = LOW;
    }
    else if (switchAState == HIGH){                                      // Switch A was released
      if (previousSwitchAState == LOW) { 
          if (switchMode == 2) { settingsAction(switchAState,HIGH); }
          else { moverState[0]=!moverState[0]; }
      }
      previousSwitchAState = HIGH;
    }
  
    if (switchBState == LOW){                                            // Switch B was pressed
      if (previousSwitchBState == LOW) { 
        mouseMoverAction(switchProperty[1].switchMouse,moverState[1],MOUSE_MOVER_XY,MOUSE_MOVER_XY);      
      }
      previousSwitchBState = LOW;
    }
    else if (switchBState == HIGH){                                    //Switch B was released
      if (previousSwitchBState == LOW) { 
          if (switchMode == 2) { settingsAction(HIGH,switchBState); }
          else { moverState[1]=!moverState[1]; }    
      }
      previousSwitchBState = HIGH;
    }
  
    if (switchCState == LOW){                                          // Switch C was pressed
      if (previousSwitchCState == LOW) { 
        moverState[2]=true;
        mouseMoverAction(switchProperty[2].switchMouse,moverState[2],MOUSE_MOVER_XY,MOUSE_MOVER_XY);
      }
      previousSwitchCState = LOW;
    }
    else if (switchCState == HIGH){                                    // Switch C was released
  
      if (previousSwitchCState == LOW) { 
        moverState[2]=false;
        mouseMoverAction(switchProperty[2].switchMouse,moverState[2],MOUSE_MOVER_XY,MOUSE_MOVER_XY);
      } 
      previousSwitchCState = HIGH;
    }
  
    if (switchDState == LOW){                                          // Switch D was pressed
      if (previousSwitchDState == HIGH) { 
        switchDTimeWatcher[0].stop();                                   //Reset and start the timer         
        switchDTimeWatcher[0].reset();                                                                        
        switchDTimeWatcher[0].start(); 
        mouseMoverAction(switchProperty[3].switchMouse,false,MOUSE_MOVER_XY,MOUSE_MOVER_XY);      
      }
      previousSwitchDState = LOW;
    }
    else if (switchDState == HIGH){                                     // Switch D was released
      if (previousSwitchDState == LOW) { 
        timePressed = switchDTimeWatcher[0].elapsed();                //Calculate the time that switch one was pressed 
        switchDTimeWatcher[0].stop();                                 //Stop the single action (dot/dash) timer and reset
        switchDTimeWatcher[0].reset();
        //Perform action if the switch has been hold active for specified time
        if (timePressed >= SWITCH_MODE_CHANGE_TIME){
          changeSwitchMode();   
                                                                      
        }
        else {
          moverState[3]=!moverState[3];
        }
        timePressed = 0; 
      }
      previousSwitchDState = HIGH;
    } 
    if (switchUpState == LOW){                                          // Switch Up was pressed
      if (previousSwitchUpState == LOW) { 
        mouseMoverAction(switchProperty[4].switchMouse,false,MOUSE_MOVER_XY,MOUSE_MOVER_XY);
      }
      previousSwitchUpState = LOW;
    }
    else if (switchUpState == HIGH){                                      // Switch Up was released
      previousSwitchUpState = HIGH;
    }
    if (switchRightState == LOW){                                          // Switch Right was pressed
      if (previousSwitchRightState == LOW) { 
        mouseMoverAction(switchProperty[5].switchMouse,false,MOUSE_MOVER_XY,MOUSE_MOVER_XY);
      }
      previousSwitchRightState = LOW;
    }
    else if (switchRightState == HIGH){                                    // Switch Right was released
      previousSwitchRightState = HIGH;
    }
    if (switchDownState == LOW){                                          // Switch Down was pressed
      if (previousSwitchDownState == LOW) { 
        mouseMoverAction(switchProperty[6].switchMouse,false,MOUSE_MOVER_XY,MOUSE_MOVER_XY);
      }
      previousSwitchDownState = LOW;
    }
    else if (switchDownState == HIGH){                                    // Switch Down was released
      previousSwitchDownState = HIGH;
    }
    if (switchLeftState == LOW){                                          // Switch Left was pressed
      if (previousSwitchLeftState == LOW) { 
        mouseMoverAction(switchProperty[7].switchMouse,false,MOUSE_MOVER_XY,MOUSE_MOVER_XY);
      }
      previousSwitchLeftState = LOW;
    }
    else if (switchLeftState == HIGH){                                    // Switch Left was released
      previousSwitchLeftState = HIGH;
    }

  ledPixels.show(); 
  delay(5);
}

//***DISPLAY FEATURE LIST FUNCTION***//

void displayFeatureList(void) {

  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("Enabled Controller Mini Mouse Mover firmware");
  Serial.println(" ");
  Serial.println("VERSION: 1.2 (26 May 2023)");
  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("Features: Mouse Mover");
  Serial.println(" --- ");
  Serial.println(" ");

}

//***RGB LED FUNCTION***//

void setLedBlink(int numBlinks, int delayBlinks, int ledColor,uint8_t ledBrightness) {
  if (numBlinks < 0) numBlinks *= -1;

      for (int i = 0; i < numBlinks; i++) {
        dotStar.setPixelColor(0, dotStar.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
        dotStar.setBrightness(ledBrightness);
        dotStar.show(); 
        ledPixels.setPixelColor(0, ledPixels.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
        ledPixels.setBrightness(ledBrightness);
        ledPixels.show(); 
        delay(delayBlinks);
        dotStar.setPixelColor(0, dotStar.Color(0,0,0));
        ledPixels.setBrightness(ledBrightness);
        dotStar.show();   
        ledPixels.setPixelColor(0, ledPixels.Color(0,0,0));
        ledPixels.setBrightness(ledBrightness);
        ledPixels.show(); 
        delay(delayBlinks);
      }
}

//***UPDATE RGB LED COLOR FUNCTION***//

void updateLedColor(int ledColor, uint8_t ledBrightness) {
    dotStar.setPixelColor(0, dotStar.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
    dotStar.setBrightness(ledBrightness);
    dotStar.show();
    ledPixels.setPixelColor(0, ledPixels.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
    ledPixels.setBrightness(ledBrightness);
    ledPixels.show();
}

//***GET RGB LED COLOR FUNCTION***//

uint32_t getLedColor(int ledModeNumber) {

  int colorNumber= modeProperty[ledModeNumber-1].modeColorNumber-1;
  
  //return (dotStar.Color(colorProperty[colorNumber].colorCode.g,colorProperty[colorNumber].colorCode.r,colorProperty[colorNumber].colorCode.b));
  return (ledPixels.Color(colorProperty[colorNumber].colorCode.g,colorProperty[colorNumber].colorCode.r,colorProperty[colorNumber].colorCode.b));
}

//***GET RGB LED BRIGHTNESS FUNCTION***//

uint8_t getLedBrightness() {
  return (ledPixels.getBrightness());
}

//***SET RGB LED COLOR FUNCTION***//

void setLedColor (uint32_t ledColor, uint8_t ledBrightness){
  dotStar.setPixelColor(0, ledColor);
  dotStar.setBrightness(ledBrightness);
  dotStar.show(); 
    
  ledPixels.setPixelColor(0, ledColor);
  ledPixels.setBrightness(ledBrightness);
  ledPixels.show(); 

}

//***SET RGB LED BRIGHTNESS FUNCTION***//

void setLedBrightness(uint8_t ledBrightness) {
  ledPixels.setBrightness(ledBrightness);
  ledPixels.show();
}

//***CLEAR RGB LED FUNCTION***//

void ledClear() {
  dotStar.setPixelColor(0, dotStar.Color(0,0,0));
  dotStar.show(); 
  ledPixels.setPixelColor(0, ledPixels.Color(0,0,0));
  ledPixels.show(); 
}

//***SWITCH FEEDBACK FUNCTION***//

void switchFeedback(int switchNumber,int modeNumber,int delayTime, int blinkNumber =1)
{
  //Get previous led color and brightness 
  uint32_t previousColor = getLedColor(modeNumber);
  uint8_t previousBrightness = getLedBrightness();
 
  //updateLedColor(switchProperty[switchNumber-1].switchColorNumber,LED_ACTION_BRIGHTNESS);
  //delay(MORSE_REACTION);
  setLedBlink(blinkNumber,delayTime,switchProperty[switchNumber-1].switchColorNumber,LED_ACTION_BRIGHTNESS);
  delay(5);

  //Set previous led color and brightness 
  setLedColor(previousColor,previousBrightness);
  
}

void settingsFeedback(int settingsNumber,int modeNumber,int delayTime, int blinkNumber =1)
{
  //Get previous led color and brightness 
  uint32_t previousColor = getLedColor(modeNumber);
  uint8_t previousBrightness = getLedBrightness();
 
  setLedBlink(blinkNumber,delayTime,settingsProperty[settingsNumber-1].settingsActionColorNumber,LED_ACTION_BRIGHTNESS);
  delay(5);

  //Set previous led color and brightness 
  setLedColor(previousColor,previousBrightness);
  
}

void modeFeedback(int modeNumber,int delayTime, int blinkNumber =1)
{

   //Get new led color and brightness 
  uint32_t newColor = getLedColor(modeNumber);
  uint8_t newBrightness = getLedBrightness();
  
  setLedBlink(blinkNumber,delayTime,modeProperty[modeNumber-1].modeColorNumber,LED_ACTION_BRIGHTNESS);
  delay(5);

  //Set new led color and brightness 
  setLedColor(newColor,newBrightness);
  
}

//***SETUP SWITCH MODE FUNCTION***//

void switchSetup() {
  //Check if it's first time running the code
  switchConfigured = switchConfiguredFlash.read();
  delay(5);
  
  if (switchConfigured==0) {
    //Define default settings if it's first time running the code
    switchReactionLevel=10;
    switchMode=1;
    switchConfigured=1;

    //Write default settings to flash storage 
    switchReactionLevelFlash.write(switchReactionLevel);
    switchModeFlash.write(switchMode);
    switchConfiguredFlash.write(switchConfigured);
    delay(5);
      
  } else {
    //Load settings from flash storage if it's not the first time running the code
    switchReactionLevel=switchReactionLevelFlash.read();
    switchMode=switchModeFlash.read();
    delay(5);
  }  

    //Serial print settings 
    Serial.print("Switch Mode: ");
    Serial.println(switchMode);

    Serial.print("Switch Reaction Level: ");
    Serial.println(switchReactionLevel);
    Serial.print("Reaction Time(ms): ");
    Serial.println(switchReactionTime); 
    //Calculate switch delay based on switchReactionLevel
    switchReactionTime = ((11-switchReactionLevel)*SWITCH_REACTION_TIME);
}

void initLedFeedback(){
  setLedBlink(2,500,modeProperty[switchMode-1].modeColorNumber,LED_ACTION_BRIGHTNESS);
  delay(5);
  updateLedColor(modeProperty[switchMode-1].modeColorNumber,LED_BRIGHTNESS);
  delay(5);
}

//***PERFORM MOUSE MOVER ACTIONS FUNCTION***//

void mouseMoverAction(int action,bool state, int xValue,int yValue) {
    switch (action) {
      case 0:
        break;
      case 1:
        if (state) {
          Mouse.move(xValue, 0, 0);
          delay(10);
        } 
        else if (!state) {
          Mouse.move(-xValue, 0, 0);
          delay(10);
        }    
        break;      
      case 2:
        if (state) {
          Mouse.move(0, -yValue, 0);
          delay(10);
        } 
        else if (!state) {
          Mouse.move(0, yValue, 0);
          delay(10);
        }    
        break; 
      case 3:
        if (state){
          if (!Mouse.isPressed(MOUSE_LEFT)) {
            Mouse.press(MOUSE_LEFT);
            delay(300);
            Mouse.release(MOUSE_LEFT);
          }
        } 
        else{
          if (Mouse.isPressed(MOUSE_LEFT)) {
            Mouse.release(MOUSE_LEFT);
          }
        }
        break;
      case 4:
          if (!Mouse.isPressed(MOUSE_RIGHT)) {
            Mouse.press(MOUSE_RIGHT);
            delay(150);
            Mouse.release(MOUSE_RIGHT);
          }    
          break;
      case 5:
          Mouse.move(0, -yValue, 0);
          delay(10);
          break;
      case 6:
          Mouse.move(xValue, 0, 0);
          delay(10);
          break;
      case 7:
          Mouse.move(0, yValue, 0);
          delay(10);
          break;
      case 8:
          Mouse.move(-xValue, 0, 0);
          delay(10);
          break;
  };

}



void changeSwitchMode(){
    //Update switch mode varia
    switchMode++;
    if (switchMode == (sizeof (modeProperty) / sizeof (modeProperty[0]))+1) {
      switchMode=1;
    } 
    else {
    }

    modeFeedback(switchMode,500,2);

    //Serial print switch mode
    Serial.print("Switch Mode: ");
    Serial.println(switchMode);
    
    //Save switch mode in flash storage
    switchModeFlash.write(switchMode);
    delay(25);
}

//***CONFIGURATION MODE ACTIONS FUNCTION***//

void settingsAction(int switch1,int switch2) {
  if(switch1==LOW) {
    decreaseReactionLevel();
  }
  if(switch2==LOW) {
    increaseReactionLevel();
  }
}

//***INCREASE SWITCH REACTION LEVEL FUNCTION***//

void increaseReactionLevel(void) {
  switchReactionLevel++;
  if (switchReactionLevel == 11) {
    settingsFeedback(3,switchMode,100,6);
    switchReactionLevel = 10;
  } else {
    settingsFeedback(1,switchMode,100,6);
    switchReactionTime = ((11-switchReactionLevel)*SWITCH_REACTION_TIME);
    delay(25);
  }
  Serial.print("Reaction level: ");
  Serial.println(switchReactionLevel);
  Serial.print("Reaction Time(ms): ");
  Serial.println(switchReactionTime);
  switchReactionLevelFlash.write(switchReactionLevel);
  delay(25);
}

//***DECREASE SWITCH REACTION LEVEL FUNCTION***//

void decreaseReactionLevel(void) {
  switchReactionLevel--;
  if (switchReactionLevel == 0) {
    //setLedBlink(6,100,3,LED_ACTION_BRIGHTNESS);
    settingsFeedback(4,switchMode,100,6);
    switchReactionLevel = 1; 
  } else {
    //setLedBlink(switchReactionLevel,100,5,LED_ACTION_BRIGHTNESS);
    settingsFeedback(2,switchMode,100,6);
    switchReactionTime = ((11-switchReactionLevel)*SWITCH_REACTION_TIME);
    delay(25);
  } 
  Serial.print("Reaction level: ");
  Serial.println(switchReactionLevel);
  Serial.print("Reaction Time(ms): ");
  Serial.println(switchReactionTime);
  
  switchReactionLevelFlash.write(switchReactionLevel);
  delay(25);
}
