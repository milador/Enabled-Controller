//Enabled-Controller Software
//Enabled_Controller_Software
//Written by: Milad Hajihassan
//Version 1.0.1 (12/9/2020)
//Github Link: https://github.com/milador/Enabled-Controller/

#include "Joystick.h"
#include <StopWatch.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>

//Use FlashStorage library for M0 Boards and EEPROM for Atmega32U4 boards
#if defined(ARDUINO_SAMD_FEATHER_M0)
#include <FlashStorage.h>
#elif defined(__AVR_Atmega32U4__)
#include <EEPROM.h>
#endif


//Can be changed based on the needs of the users 
#define FIXED_SPEED_LEVEL 6
#define FIXED_DELAY 30
#define JOYSTICK_DEADZONE 20
#define SWITCH_REACTION_TIME 50                                       //Minimum time for each switch action ( level 10 : 1x50 =50ms , level 1 : 10x50=500ms )
#define SWITCH_MODE_CHANGE_TIME 2000                                  //How long to hold switch 4 to change mode 

#define LED_BRIGHTNESS 150                                             //The mode led color brightness which is always on ( Use a low value to decrease power usage )
#define LED_ACTION_BRIGHTNESS 150                                      //The action led color brightness which can be a higher value than LED_BRIGHTNESS


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

//Define Joystick pins
#define JOYSTICK_X1_PIN A4
#define JOYSTICK_Y1_PIN A5  
#define JOYSTICK_X2_PIN A2
#define JOYSTICK_Y2_PIN A3  

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

//Declare joystick variables 
int joystickX1;
int joystickY1;
int joystickX2;
int joystickY2;


//Stopwatches array used to time switch presses
StopWatch switchDTimeWatcher[1];


//Declare Switch variables for settings 
int switchConfigured;
int switchReactionTime;
int switchReactionLevel;
int switchMode;


//Declare Flash storage variables 
#if defined(ARDUINO_SAMD_FEATHER_M0)
FlashStorage(switchConfiguredFlash, int);
FlashStorage(switchReactionLevelFlash, int);
FlashStorage(switchModeFlash, int);
#endif


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
  String switchButtonName;
  uint8_t switchMode1ButtonNumber;
  uint8_t switchMode2ButtonNumber;
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
    {1,"A",1,5,5},
    {2,"B",2,6,3},
    {3,"C",3,7,1},
    {4,"D",4,8,6}
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
    {1,"Mode 1",8},
    {2,"Mode 2",2},
    {4,"Settings",4}
};


//Setup NeoPixel LED
Adafruit_NeoPixel ledPixels = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_DotStar dotStar(1, DOTSTAR_DATA_PIN, DOTSTAR_CLOCK_PIN, DOTSTAR_BRG);

//Defining the joystick REPORT_ID and profile type
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 8, 0,
  true, true, false, 
  false, false, false,
  false, false, 
  false, false, false);   

void setup() {

  ledPixels.begin();                                                           //Start NeoPixel
  Serial.begin(115200);                                                        //Start Serial
  switchSetup();
  delay(5);
  joystickSetup();
  delay(1000);
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
  static int previousSwDState;             //Previous status of switch four
  
  //Update status of switch inputs
  switchAState = digitalRead(SWITCH_A_PIN);
  switchBState = digitalRead(SWITCH_B_PIN);
  switchCState = digitalRead(SWITCH_C_PIN);
  switchDState = digitalRead(SWITCH_D_PIN);
  switchUpState = digitalRead(SWITCH_UP_PIN);
  switchRightState = digitalRead(SWITCH_RIGHT_PIN);
  switchDownState = digitalRead(SWITCH_DOWN_PIN);
  switchLeftState = digitalRead(SWITCH_LEFT_PIN);


  //Update status of joystick inputs
  joystickX1 = analogRead(JOYSTICK_X1_PIN);
  joystickY1 = analogRead(JOYSTICK_Y1_PIN);    
  timePressed = timeNotPressed  = 0;       //reset time counters
  if (!ctr) {                              //Set previous status of switch four 
    previousSwDState = HIGH;  
    ctr++;
  }
  //Check if switch 4 is pressed to change switch mode
  if (switchDState == LOW && previousSwDState == HIGH) {
     if (switchDState == LOW) { 
      previousSwDState = LOW; 
     }
     switchDTimeWatcher[0].stop();                                //Reset and start the timer         
     switchDTimeWatcher[0].reset();                                                                        
     switchDTimeWatcher[0].start(); 
  }
  // Switch four was released
  if (switchDState == HIGH && previousSwDState == LOW) {
    previousSwDState = HIGH;
    timePressed = switchDTimeWatcher[0].elapsed();                //Calculate the time that switch one was pressed 
    switchDTimeWatcher[0].stop();                                 //Stop the single action (dot/dash) timer and reset
    switchDTimeWatcher[0].reset();
    
    //Perform action if the switch has been hold active for specified time
    if (timePressed >= SWITCH_MODE_CHANGE_TIME){
      changeSwitchMode();                                                                
    } else if(switchMode==1 || switchMode==2) {
      joystickAction(switchMode,switchAState,switchBState,switchCState,LOW,switchUpState,switchRightState,switchDownState,switchLeftState,joystickX1,joystickY1);
      delay(5);
    }
  }
  
  //Perform joystick actions based on the mode
    switch (switchMode) {
      case 1:
        joystickAction(switchMode,switchAState,switchBState,switchCState,HIGH,switchUpState,switchRightState,switchDownState,switchLeftState,joystickX1,joystickY1);    //Switch mode
        break;
      case 2:
        joystickAction(switchMode,switchAState,switchBState,switchCState,HIGH,switchUpState,switchRightState,switchDownState,switchLeftState,joystickX1,joystickY1);    //Switch mode
        break;
      case 3:
        settingsAction(switchAState,switchBState);                                          //Settings mode
        break;
  };
  dotStar.show(); 
  ledPixels.show(); 
  delay(5);
}

//***DISPLAY FEATURE LIST FUNCTION***//

void displayFeatureList(void) {

  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("This is the Enabled Controller firmware");
  Serial.println(" ");
  Serial.println("VERSION: 1.0.1 (12 September 2020)");
  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("Features: Joystick ");
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

void switchFeedback(int switchNumber,int modeNumber,int delayTime, int blinkNumber =1)
{
  //Get previous led color and brightness 
  uint32_t previousColor = getLedColor(modeNumber);
  uint8_t previousBrightness = getLedBrightness();
 
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


//***SETUP SWITCH HID PROFILES FUNCTION (LOAD HID PROFILES)***//

void joystickSetup(){
      //Setup joystick library 
      Joystick.begin(); 

      //Set joystick x,y range
      Joystick.setXAxisRange(-127, 127);
      Joystick.setYAxisRange(-127, 127);

}

//***PERFORM JOYSTICK ACTIONS FUNCTION***//

void joystickAction(int mode, int switchA,int switchB,int switchC,int switchD, int switchUp,int switchRight,int switchDown,int switchLeft,int x,int y) {
  
    if(!switchA) {
      switchFeedback(1,mode,switchReactionTime,1);
      ( mode ==1 ) ? Joystick.pressButton(switchProperty[0].switchMode1ButtonNumber-1) : Joystick.pressButton(switchProperty[0].switchMode2ButtonNumber-1);
    }  else {
      ( mode ==1 ) ? Joystick.releaseButton(switchProperty[0].switchMode1ButtonNumber-1) : Joystick.releaseButton(switchProperty[0].switchMode2ButtonNumber-1);
    }
    
    if(!switchB) {
      switchFeedback(2,mode,switchReactionTime,1);
      ( mode ==1 ) ? Joystick.pressButton(switchProperty[1].switchMode1ButtonNumber-1) : Joystick.pressButton(switchProperty[1].switchMode2ButtonNumber-1);
    }  else {
      ( mode ==1 ) ? Joystick.releaseButton(switchProperty[1].switchMode1ButtonNumber-1) : Joystick.releaseButton(switchProperty[1].switchMode2ButtonNumber-1);
    }
    
    if(!switchC) {
      switchFeedback(3,mode,switchReactionTime,1);
      ( mode ==1 ) ? Joystick.pressButton(switchProperty[2].switchMode1ButtonNumber-1) : Joystick.pressButton(switchProperty[2].switchMode2ButtonNumber-1);
    }  else {
      ( mode ==1 ) ? Joystick.releaseButton(switchProperty[2].switchMode1ButtonNumber-1) : Joystick.releaseButton(switchProperty[2].switchMode2ButtonNumber-1);
    }
    
    if(!switchD) {
      switchFeedback(4,mode,switchReactionTime,1);
      ( mode ==1 ) ? Joystick.pressButton(switchProperty[3].switchMode1ButtonNumber-1) : Joystick.pressButton(switchProperty[3].switchMode2ButtonNumber-1);
    }  else {
      ( mode ==1 ) ? Joystick.releaseButton(switchProperty[3].switchMode1ButtonNumber-1) : Joystick.releaseButton(switchProperty[3].switchMode2ButtonNumber-1);
    }

    if(!switchUp) {
      Joystick.setXAxis(0);
      Joystick.setYAxis(-127); 
    }  else {
      Joystick.setXAxis(0);
      Joystick.setYAxis(0);    
    }

    if(!switchRight) {
      Joystick.setXAxis(127);
      Joystick.setYAxis(0); 
    }  else {
      Joystick.setXAxis(0);
      Joystick.setYAxis(0);    
    }
    
    if(!switchDown) {
      Joystick.setXAxis(0);
      Joystick.setYAxis(127); 
    }  else {
      Joystick.setXAxis(0);
      Joystick.setYAxis(0);    
    }

    if(!switchLeft) {
      Joystick.setXAxis(-127);
      Joystick.setYAxis(0); 
    }  else {
      Joystick.setXAxis(0);
      Joystick.setYAxis(0);    
    }

    
        
    int xx = map(x, 0, 1023, -127, 127);
    int yy = -map(y, 0, 1023, -127, 127);
      Serial.println(yy);

    if (xx<=JOYSTICK_DEADZONE && xx>=-JOYSTICK_DEADZONE) Joystick.setXAxis(0);  
    else Joystick.setXAxis(xx);  
    
    if (yy<=JOYSTICK_DEADZONE && yy>=-JOYSTICK_DEADZONE) Joystick.setYAxis(0);  
    else Joystick.setYAxis(yy);  

    delay(100);

}

//***CLEAR JOYSTICK ACTION FUNCTION***//

void joystickClear(){
    Joystick.setXAxis(0);
    Joystick.setYAxis(0);  
}

//***SETUP SWITCH MODE FUNCTION***//

void switchSetup() {
  //Check if it's first time running the code
  #if defined(ARDUINO_SAMD_FEATHER_M0)
    switchConfigured = switchConfiguredFlash.read();
  #elif defined(__AVR_Atmega32U4__)
    EEPROM.get(22, switchConfigured);
    delay(5);
    if(switchConfigured<0){ switchConfigured = 0; } 
  #endif
  if (switchConfigured==0) {
    //Define default settings if it's first time running the code
    switchReactionLevel=10;
    switchMode=1;
    switchConfigured=1;

    //Write default settings to flash storage 
    #if defined(ARDUINO_SAMD_FEATHER_M0)
      switchReactionLevelFlash.write(switchReactionLevel);
      switchModeFlash.write(switchMode);
      switchConfiguredFlash.write(switchConfigured);
    #elif defined(__AVR_Atmega32U4__)
      EEPROM.put(26, switchReactionLevel);
      delay(5);
      EEPROM.put(24, switchMode);
      delay(5);
      EEPROM.put(22, switchConfigured);
      delay(5);
    #endif
  } else {
    //Load settings from flash storage if it's not the first time running the code
    #if defined(ARDUINO_SAMD_FEATHER_M0)
      switchReactionLevel=switchReactionLevelFlash.read();
      switchMode=switchModeFlash.read();
    #elif defined(__AVR_Atmega32U4__)
      EEPROM.get(26, switchReactionLevel);
      delay(5);
      EEPROM.get(24, switchMode);
      delay(5);
    #endif    
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


//***CHANGE SWITCH MODE FUNCTION***//

void changeSwitchMode(){
    //Update switch mode varia
    switchMode++;
    if (switchMode == (sizeof (modeProperty) / sizeof (modeProperty[0]))+1) {
      switchMode=1;
    } 
    else {
    }
    
    //Blink 2 times in modes color 
    modeFeedback(switchMode,500,2);

    //Serial print switch mode
    Serial.print("Switch Mode: ");
    Serial.println(switchMode);
    
    //Save switch mode in flash storage
    #if defined(ARDUINO_SAMD_FEATHER_M0)
      switchModeFlash.write(switchMode);
    #elif defined(__AVR_Atmega32U4__)
      EEPROM.put(24, switchMode);
      delay(5);
    #endif
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
  #if defined(ARDUINO_SAMD_FEATHER_M0)
    switchReactionLevelFlash.write(switchReactionLevel);
  #elif defined(__AVR_Atmega32U4__)
    EEPROM.put(26, switchReactionLevel);
    delay(5);
  #endif
  delay(25);
}

//***DECREASE SWITCH REACTION LEVEL FUNCTION***//

void decreaseReactionLevel(void) {
  switchReactionLevel--;
  if (switchReactionLevel == 0) {

    settingsFeedback(4,switchMode,100,6);
    switchReactionLevel = 1; 
  } else {
    settingsFeedback(2,switchMode,100,6);
    switchReactionTime = ((11-switchReactionLevel)*SWITCH_REACTION_TIME);
    delay(25);
  } 
  Serial.print("Reaction level: ");
  Serial.println(switchReactionLevel);
  Serial.print("Reaction Time(ms): ");
  Serial.println(switchReactionTime);
  #if defined(ARDUINO_SAMD_FEATHER_M0)
    switchReactionLevelFlash.write(switchReactionLevel);
  #elif defined(__AVR_Atmega32U4__)
    EEPROM.put(26, switchReactionLevel);
    delay(5);
  #endif
  delay(25);
}
