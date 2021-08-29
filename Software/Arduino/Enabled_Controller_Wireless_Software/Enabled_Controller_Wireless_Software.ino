/** ************************************************************************
 * File Name: Enabled_Controller_Wireless_Software.ino 
 * Title: Enabled Controller Wireless Software
 * Developed by: Milad Hajihassan
 * Version Number: 1.1 (21/5/2021)
 * Github Link: https://github.com/milador/Enabled_Controller
 ***************************************************************************/


#include <StopWatch.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <ArduinoJson.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "EasyMorseBlue.h"
#include "Enabled_Controller_Hid.h"

using namespace Adafruit_LittleFS_Namespace;

#define SETTINGS_FILE    "/settings.txt"
#define SETTINGS_JSON    "{\"configured\":0,\"mode\":1,\"reaction-level\":4}"

//Can be changed based on the needs of the users 
#define JOYSTICK_ENABLED false                                        //Enable Joystick 
#define JOYSTICK_DEADZONE 20                                          //Joystick deadzone
#define JOYSTICK_NUMBER 2                                             //A1 = 1 , A2 = 2
#define MORSE_TIMEOUT 1000                                            //Maximum timeout (1000ms =1s)
#define MORSE_REACTION_TIME 10                                        //Minimum time for a dot or dash switch action ( level 10 : (1.5^1)x10 =15ms , level 1 : (1.5^10)x10=570ms )
#define MOUSE_MOVE_MULTI 2                                            //Morse mouse move multiplier 
#define SWITCH_REACTION_TIME 50                                       //Minimum time for each switch action ( level 10 : 1x50 =50ms , level 1 : 10x50=500ms )
#define SWITCH_MODE_CHANGE_TIME 2000                                  //How long to hold switch D to change mode 
#define SWITCH_MAC_PROFILE false                                      //Windows,Android,iOS=false MacOS=true 

#define LED_BRIGHTNESS 150                                             //The default led color brightness which is always on
#define LED_LP_BRIGHTNESS 4                                            //The low power mode led color brightness
#define LED_ACTION_BRIGHTNESS 150                                      //The action led color brightness which can be a higher value than LED_BRIGHTNESS

//Define Switch pins
#define LED_PIN_EXT 5
#define LED_PIN_IN  PIN_NEOPIXEL 

//Define Switch pins
#define LED_PIN 12
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

//Define Mouse Actions
#define MOUSE_NO_ACTION 0 
#define MOUSE_LEFT_CLICK 1
#define MOUSE_RIGHT_CLICK 2
#define MOUSE_DOUBLE_LEFT_CLICK 3
#define MOUSE_DOUBLE_RIGHT_CLICK 4
#define MOUSE_LEFT_HOLD 5
#define MOUSE_RIGHT_HOLD 6

EasyMorseBlue morse;

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
int joystickX;
int joystickY;
int joystickCenterX;
int joystickCenterY;

//Stopwatches array used to time switch presses
StopWatch timeWatcher[3];
StopWatch switch4TimeWatcher[1];

//Initialize FileSystem
File file(InternalFS);


//Initialize time variables for morse code
unsigned msMin = MS_MIN_DD;
unsigned msMax = MS_MAX_DD;
unsigned msEnd = MS_END;
unsigned msClear = MS_CL;

//Declare Switch variables for settings 
int switchConfigured;
int switchReactionTime;
int switchReactionLevel;
int switchMode;

int morseReactionTime;

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
  uint8_t switchChar;
  uint8_t switchMacChar;
  uint8_t switchMouse;
  uint8_t switchColorNumber;
  uint8_t switchMorseTimeMulti;
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
    {1,"DOT",HID_KEY_A,HID_KEY_F1,MOUSE_LEFT_CLICK,5,1},                             //{1=dot,"DOT",'a','F1',5=blue,1=1xMORSE_REACTION}
    {2,"DASH",HID_KEY_B,HID_KEY_F2,MOUSE_RIGHT_CLICK,6,3},                           //{2=dash,"DASH",'b','F2',6=red,3=3xMORSE_REACTION}
    {3,"C",HID_KEY_C,HID_KEY_F3,MOUSE_DOUBLE_LEFT_CLICK,1,1},                        //{3,"C",'c','F3',1=green,1=1xMORSE_REACTION}
    {4,"D",HID_KEY_D,HID_KEY_F4,MOUSE_DOUBLE_RIGHT_CLICK,3,1},                       //{4,"D",'d','F4',3=yellow,1=1xMORSE_REACTION}
    {5,"UP",HID_KEY_ARROW_UP,HID_KEY_F5,MOUSE_NO_ACTION,4,1},                        //{5,"UP",'UP','F5',4=orange,1=1xMORSE_REACTION}
    {6,"RIGHT",HID_KEY_ARROW_RIGHT,HID_KEY_F6,MOUSE_NO_ACTION,4,1},                  //{6,"RIGHT",'RIGHT','F6',4=orange,1=1xMORSE_REACTION}
    {7,"DOWN",HID_KEY_ARROW_DOWN,HID_KEY_F7,MOUSE_NO_ACTION,4,1},                    //{7,"DOWN",'DOWN','F7',4=orange,1=1xMORSE_REACTION}
    {8,"LEFT",HID_KEY_ARROW_LEFT,HID_KEY_F8,MOUSE_NO_ACTION,4,1},                    //{8,"LEFT",'LEFT','F8',4=orange,1=1xMORSE_REACTION}
    {9,"ANALOG",HID_KEY_NONE,HID_KEY_NONE,MOUSE_NO_ACTION,4,1}                       //{9,"ANALOG",'NONE','F9',4=orange,1=1xMORSE_REACTION}
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
    {1,"Keyboard Switch",8},
    {2,"Morse Keyboard",7},
    {3,"Morse Mouse",2},
    {4,"Mouse",1},
    {5,"Settings",4}
};


//Setup NeoPixel LED
Adafruit_NeoPixel ledPixelExt = Adafruit_NeoPixel(1, LED_PIN_EXT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledPixelIn = Adafruit_NeoPixel();

void setup() {

  ledPixelIn.begin();   
  ledPixelExt.begin();                                                          //Start NeoPixel
  Serial.begin(115200);                                                         //Start Serial
  delay(500);
  initSettings(SETTINGS_FILE,SETTINGS_JSON);                                    //Setup FileSystem
  delay(5);
  initJoystick();
  delay(250);
  bluetoothSetup();                                                             //Starts bluetooth emulation
  delay(1000);
  switchSetup();                                                               //Setup switch
  delay(5);
  initLedFeedback();                                                           //Led will blink in a color to show the current mode 
  delay(5);
  morseSetup();                                                                //Setup morse
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
  pinMode(LED_PIN_EXT, OUTPUT);   

};

void loop() {
  //Skip if not connected
  if (!Bluefruit.connected()) 
  {
    //Enter low power mode
    waitForEvent();
    setLedBrightness(LED_LP_BRIGHTNESS);
    return;
  }

  setLedBrightness(LED_BRIGHTNESS);
  static int ctr;                          //Control variable to set previous status of switches 
  unsigned long timePressed;               //Time that switch one or two are pressed
  unsigned long timeNotPressed;            //Time that switch one or two are not pressed
  static int previousSw4State;             //Previous status of switch four
  
  //Update status of switch inputs
  switchAState = digitalRead(SWITCH_A_PIN);
  switchBState = digitalRead(SWITCH_B_PIN);
  switchCState = digitalRead(SWITCH_C_PIN);
  switchDState = digitalRead(SWITCH_D_PIN);

  switchUpState = digitalRead(SWITCH_UP_PIN);
  switchRightState = digitalRead(SWITCH_RIGHT_PIN);
  switchDownState = digitalRead(SWITCH_DOWN_PIN);
  switchLeftState = digitalRead(SWITCH_LEFT_PIN);

    //Update joystick values based on available joystick number 
  if(JOYSTICK_ENABLED && JOYSTICK_NUMBER == 1) {
    joystickX = analogRead(JOYSTICK_X1_PIN);
    joystickY = analogRead(JOYSTICK_Y1_PIN);
  } else if (JOYSTICK_ENABLED && JOYSTICK_NUMBER == 2) {
    joystickX = analogRead(JOYSTICK_X2_PIN);
    joystickY = analogRead(JOYSTICK_Y2_PIN);
  }
  
  timePressed = timeNotPressed  = 0;       //reset time counters
  if (!ctr) {                              //Set previous status of switch four 
    previousSw4State = HIGH;  
    ctr++;
  }
  //Check if switch 4 is pressed to change switch mode
  if (switchDState == LOW && previousSw4State == HIGH) {
     if (switchDState == LOW) { 
      previousSw4State = LOW; 
     }
     switch4TimeWatcher[0].stop();                                //Reset and start the timer         
     switch4TimeWatcher[0].reset();                                                                        
     switch4TimeWatcher[0].start(); 
  }
  // Switch four was released
  if (switchDState == HIGH && previousSw4State == LOW) {
    previousSw4State = HIGH;
    timePressed = switch4TimeWatcher[0].elapsed();                //Calculate the time that switch one was pressed 
    switch4TimeWatcher[0].stop();                                 //Stop the single action (dot/dash) timer and reset
    switch4TimeWatcher[0].reset();
    //Perform action if the switch has been hold active for specified time
    if (timePressed >= SWITCH_MODE_CHANGE_TIME){
      changeSwitchMode();                                                                
    } else if(switchMode==1) {
      keyboardAction(switchAState,switchBState,switchCState,LOW,switchUpState,switchRightState,switchDownState,switchLeftState);
    }else if(switchMode==4) {
       mouseAction(switchAState,switchBState,switchCState,LOW,switchUpState,switchRightState,switchDownState,switchLeftState,joystickX,joystickY);                        //Mouse mode
    }
  }
  //Perform actions based on the mode
  switch (switchMode) {
      case 1:
        keyboardAction(switchAState,switchBState,switchCState,HIGH,switchUpState,switchRightState,switchDownState,switchLeftState);                        //Switch mode
        break;
      case 2:
        morseAction(1,switchAState,switchBState);                                           //Keyboard Morse mode
        break;
      case 3:
        morseAction(2,switchAState,switchBState);                                           //Mouse Morse mode
        break;
      case 4:
        mouseAction(switchAState,switchBState,switchCState,HIGH,switchUpState,switchRightState,switchDownState,switchLeftState,joystickX,joystickY);                        //Switch mode
        break;
      case 5:
        settingsAction(switchAState,switchBState);                                          //Settings mode
        break;
  };
  ledPixelIn.show(); 
  ledPixelExt.show(); 
  delay(5);
}

//***DISPLAY FEATURE LIST FUNCTION***//

void displayFeatureList(void) {

  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("This is the Enabled Controller Wireless firmware");
  Serial.println(" ");
  Serial.println("VERSION: 1.1 (May 21, 2021)");
  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("Features: Adaptive switch, Morse Keyboard, Morse Mouse, Mouse");
  Serial.println(" --- ");
  Serial.println(" ");

}

//***RGB LED FUNCTION***//

void setLedBlink(int numBlinks, int delayBlinks, int ledColor,uint8_t ledBrightness) {
  if (numBlinks < 0) numBlinks *= -1;

      for (int i = 0; i < numBlinks; i++) {
        ledPixelIn.setPixelColor(0, ledPixelIn.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
        ledPixelIn.setBrightness(ledBrightness);
        ledPixelIn.show(); 
        
        ledPixelExt.setPixelColor(0, ledPixelExt.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
        ledPixelExt.setBrightness(ledBrightness);
        ledPixelExt.show(); 
        
        delay(delayBlinks);
        
        ledPixelIn.setPixelColor(0, ledPixelIn.Color(0,0,0));
        ledPixelIn.setBrightness(ledBrightness);
        ledPixelIn.show();    
        
        ledPixelExt.setPixelColor(0, ledPixelExt.Color(0,0,0));
        ledPixelExt.setBrightness(ledBrightness);
        ledPixelExt.show(); 
        
        delay(delayBlinks);
      }
}

//***UPDATE RGB LED COLOR FUNCTION***//

void updateLedColor(int ledColor, uint8_t ledBrightness) {
    ledPixelIn.setPixelColor(0, ledPixelIn.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
    ledPixelIn.setBrightness(ledBrightness);
    ledPixelIn.show();
    
    ledPixelExt.setPixelColor(0, ledPixelExt.Color(colorProperty[ledColor-1].colorCode.g,colorProperty[ledColor-1].colorCode.r,colorProperty[ledColor-1].colorCode.b));
    ledPixelExt.setBrightness(ledBrightness);
    ledPixelExt.show();
}

//***GET RGB LED COLOR FUNCTION***//

uint32_t getLedColor(int ledModeNumber) {

  int colorNumber= modeProperty[ledModeNumber-1].modeColorNumber-1;
  
  //return (dotStar.Color(colorProperty[colorNumber].colorCode.g,colorProperty[colorNumber].colorCode.r,colorProperty[colorNumber].colorCode.b));
  return (ledPixelExt.Color(colorProperty[colorNumber].colorCode.g,colorProperty[colorNumber].colorCode.r,colorProperty[colorNumber].colorCode.b));
}

//***GET RGB LED BRIGHTNESS FUNCTION***//

uint8_t getLedBrightness() {
  return (ledPixelExt.getBrightness());
}

//***SET RGB LED COLOR FUNCTION***//

void setLedColor (uint32_t ledColor, uint8_t ledBrightness){
  ledPixelIn.setPixelColor(0, ledColor);
  ledPixelIn.setBrightness(ledBrightness);
  ledPixelIn.show(); 
    
  ledPixelExt.setPixelColor(0, ledColor);
  ledPixelExt.setBrightness(ledBrightness);
  ledPixelExt.show(); 

}

//***SET RGB LED BRIGHTNESS FUNCTION***//

void setLedBrightness(uint8_t ledBrightness) {
  ledPixelIn.setBrightness(ledBrightness);
  ledPixelIn.show();
  
  ledPixelExt.setBrightness(ledBrightness);
  ledPixelExt.show();
}

//***CLEAR RGB LED FUNCTION***//
void ledClear() {
  ledPixelIn.setPixelColor(0, ledPixelIn.Color(0,0,0));
  ledPixelIn.show(); 
   
  ledPixelExt.setPixelColor(0, ledPixelExt.Color(0,0,0));
  ledPixelExt.show(); 
}

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
  switchConfigured = readSettings(SETTINGS_FILE,"configured");
  delay(5);
  
  if (switchConfigured==0) {
    //Define default settings if it's first time running the code
    switchReactionLevel=10;
    switchMode=1;
    switchConfigured=1;

    //Write default settings to flash storage 
    writeSettings(SETTINGS_FILE,"reaction_level",switchReactionLevel);
    delay(1);
    writeSettings(SETTINGS_FILE,"mode",switchMode);
    delay(1);
    writeSettings(SETTINGS_FILE,"configured",switchConfigured);
    
    delay(5);
      
  } else {
    //Load settings from flash storage if it's not the first time running the code
    switchReactionLevel=readSettings(SETTINGS_FILE,"reaction_level");
    delay(1);
    switchMode=readSettings(SETTINGS_FILE,"mode");
    delay(5);
  }  

    //Serial print settings 
    Serial.print("Switch Mode: ");
    Serial.println(switchMode);

    Serial.print("Switch Reaction Level: ");
    Serial.println(switchReactionLevel);
    Serial.print("Reaction Time(ms): ");
    Serial.print(switchReactionTime);
    Serial.print("-");
    Serial.println(morseReactionTime);   
    //Calculate switch delay based on switchReactionLevel
    switchReactionTime = ((11-switchReactionLevel)*SWITCH_REACTION_TIME);
    morseReactionTime = (pow(1.5,(11-switchReactionLevel))*MORSE_REACTION_TIME);
}

//***SETUP MORSE FUNCTION***//

void morseSetup() {
    morse.clear();
    msMin = morseReactionTime;
    msMax = msEnd = msClear = MORSE_TIMEOUT; 

}

void initLedFeedback(){
  setLedBlink(2,500,modeProperty[switchMode-1].modeColorNumber,LED_ACTION_BRIGHTNESS);
  delay(5);
  updateLedColor(modeProperty[switchMode-1].modeColorNumber,LED_BRIGHTNESS);
  delay(5);
}

//***INITIALIZE JOYSTICK INPUTS FUNCTION***//

void initJoystick() {
   //Update joystick values based on available joystick number 
  if(JOYSTICK_ENABLED && JOYSTICK_NUMBER == 1) {
    joystickCenterX = analogRead(JOYSTICK_X1_PIN);
    joystickCenterY = analogRead(JOYSTICK_Y1_PIN);
  } else if (JOYSTICK_ENABLED && JOYSTICK_NUMBER == 2) {
    joystickCenterX = analogRead(JOYSTICK_X2_PIN);
    joystickCenterY = analogRead(JOYSTICK_Y2_PIN);
  }
}


//***ADAPTIVE SWITCH KEYBOARD FUNCTION***//

void keyboardAction(int switch1,int switch2,int switch3,int switch4,int switch5,int switch6,int switch7,int switch8) {
    if(!switch1) {
      switchFeedback(1,switchMode,switchReactionTime,1);
      //Serial.println("a");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[0].switchMacChar) : enterKeyboard(0, switchProperty[0].switchChar);
    } else if(!switch2) {
      switchFeedback(2,switchMode,switchReactionTime,1);
      //Serial.println("b");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[1].switchMacChar) : enterKeyboard(0, switchProperty[1].switchChar);
    } else if(!switch3) {
      switchFeedback(3,switchMode,switchReactionTime,1);
      //Serial.println("c");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[2].switchMacChar) : enterKeyboard(0, switchProperty[2].switchChar);
    } else if(!switch4) {
      switchFeedback(4,switchMode,switchReactionTime,1);
      //Serial.println("d");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[3].switchMacChar) : enterKeyboard(0, switchProperty[3].switchChar);
    } else if(!switch5) {
      switchFeedback(5,switchMode,switchReactionTime,1);
      //Serial.println("Up");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[4].switchMacChar) : enterKeyboard(0, switchProperty[4].switchChar);
    } else if(!switch6) {
      switchFeedback(6,switchMode,switchReactionTime,1);
      //Serial.println("Right");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[5].switchMacChar) : enterKeyboard(0, switchProperty[5].switchChar);
    } else if(!switch7) {
      switchFeedback(7,switchMode,switchReactionTime,1);
      //Serial.println("Down");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[6].switchMacChar) : enterKeyboard(0, switchProperty[6].switchChar);
    }else if(!switch8) {
      switchFeedback(8,switchMode,switchReactionTime,1);
      //Serial.println("Left");
      (SWITCH_MAC_PROFILE) ? enterKeyboard(0, switchProperty[7].switchMacChar) : enterKeyboard(0, switchProperty[7].switchChar);
    }
    else
    {
      clearKeyboard();
    }
    delay(SWITCH_REACTION_TIME);

}



//***ADAPTIVE SWITCH MOUSE FUNCTION***//

void mouseAction(int switch1,int switch2,int switch3,int switch4,int switch5,int switch6,int switch7,int switch8,int analogX, int analogY) {

    int xx = map(analogX, 0, 1023, -127, 127);
    int yy = map(analogY, 0, 1023, -127, 127);
    
    if(!switch1) {
      switchFeedback(1,switchMode,switchReactionTime,1);
      //Serial.println("a");
      enterMouse(switchProperty[0].switchMouse,xx,yy);
    } else if(!switch2) {
      switchFeedback(2,switchMode,switchReactionTime,1);
      //Serial.println("b");
      enterMouse(switchProperty[1].switchMouse,xx,yy);
    } else if(!switch3) {
      switchFeedback(3,switchMode,switchReactionTime,1);
      //Serial.println("c");
      enterMouse(switchProperty[2].switchMouse,xx,yy);
    } else if(!switch4) {
      switchFeedback(4,switchMode,switchReactionTime,1);
      //Serial.println("d");
      enterMouse(switchProperty[3].switchMouse,xx,yy);
    } else if(!switch5) {
      switchFeedback(5,switchMode,switchReactionTime,1);
      //Serial.println("Up");
      enterMouse(switchProperty[4].switchMouse,xx,yy);
    } else if(!switch6) {
      switchFeedback(6,switchMode,switchReactionTime,1);
      //Serial.println("Right");
      enterMouse(switchProperty[5].switchMouse,xx,yy);
    } else if(!switch7) {
      switchFeedback(7,switchMode,switchReactionTime,1);
      //Serial.println("Down");
      enterMouse(switchProperty[6].switchMouse,xx,yy);
    } else if(!switch8) {
      switchFeedback(8,switchMode,switchReactionTime,1);
      //Serial.println("Left");
      enterMouse(switchProperty[7].switchMouse,xx,yy);
    } else if (abs(analogX-joystickCenterX)>=JOYSTICK_DEADZONE || abs(analogY-joystickCenterY)>=JOYSTICK_DEADZONE) {
      if (abs(analogX-joystickCenterX)>=JOYSTICK_DEADZONE && abs(analogY-joystickCenterY)>=JOYSTICK_DEADZONE) { enterMouse(0,xx,yy); }
      else if (abs(analogX-joystickCenterX)>=JOYSTICK_DEADZONE) { enterMouse(0,xx,0); }
      else if (abs(analogY-joystickCenterY)>=JOYSTICK_DEADZONE) { enterMouse(0,0,yy); }
    }
    else
    {
      clearMouse();
    }
    delay(SWITCH_REACTION_TIME);

}


//***MORSE CODE TO MOUSE CONVERT FUNCTION***//

void morseAction(int mode,int switch1,int switch2) {
  int i;
  static int ctr;
  unsigned long timePressed;
  unsigned long timeNotPressed;
  static int previousSwitch1;
  static int previousSwitch2;

  int isShown;                                                                                  //is character shown yet?
  int backspaceDone = 0;                                                                        //is backspace entered? 0 = no, 1=yes

  timePressed = timeNotPressed  = 0; 

  if (!ctr) {
    previousSwitch1 = LOW;  
    previousSwitch2 = LOW;  
    ctr++;
  }
   //Check if dot or dash switch is pressed
   if ((switch1 == LOW && previousSwitch1 == HIGH) || (switch2 == LOW && previousSwitch2 == HIGH)) {
      //Dot
     if (switch1 == LOW) { 
         switchFeedback(1,mode,morseReactionTime,1);                                            //Blink blue once
         previousSwitch1 = LOW;
     } //Dash
     if (switch2 == LOW) {
         switchFeedback(2,mode,morseReactionTime,1);                                            //Blink red once
         previousSwitch2 = LOW;
     }
     isShown = 0;                                                                               //A key is pressed but not Shown yet
     for (i=0; i<3; i++) timeWatcher[i].stop();                                                          //Stop end of char
     
     timeWatcher[0].reset();                                                                             //Reset and start the the timer
     timeWatcher[0].start();                                                                             
   }

 
 // Switch 1 or Dot was released
 if (switch1 == HIGH && previousSwitch1 == LOW) {
   previousSwitch1 = HIGH;
   
   backspaceDone = 0;                                                                                    //Backspace is not entered 
   timePressed = timeWatcher[0].elapsed();                                                               //Calculate the time that key was pressed 
   for (i=0; i<3; i++) timeWatcher[i].stop();                                                            //Stop end of character and reset
   for (i=0; i<3; i++) timeWatcher[i].reset();                                                           
   timeWatcher[1].start();

  //Push dot to morse stack if it's pressed for specified time
    if (timePressed >= msMin && timePressed < msMax) {
      morse.push(1);
      isShown = 0; 
    }
 }

   //Switch 2 or Dash was released
   if (switch2 == HIGH && previousSwitch2 == LOW) {

    previousSwitch2 = HIGH;
    backspaceDone = 0;                                                                          //Backspace is not entered 
    timePressed = timeWatcher[0].elapsed();                                                              //Calculate the time that key was pressed 
    for (i=0; i<3; i++) timeWatcher[i].stop();                                                           //Stop end of character and reset
    for (i=0; i<3; i++) timeWatcher[i].reset();
    timeWatcher[1].start();
   
    //Push dash to morse stack if it's pressed for specified time
    if (timePressed >= msMin && timePressed < msMax) {
      morse.push(2);
      isShown = 0;       
    }
   }

 // Processing the backspace key if button 1 or button 2 are hold and pressed for defined time
  if (timePressed >= msMax && timePressed >= msClear && backspaceDone == 0 &&  mode== 1) {
    previousSwitch1 = HIGH;
    previousSwitch2 = HIGH;
    if(mode==1) {
      enterKeyboard(0,44);                                                                      //Press Backspace key
    }
    backspaceDone = 1;                                                                          //Set Backspace done already
    isShown = 1;
    for (i=1; i<3; i++) { timeWatcher[i].stop(); timeWatcher[i].reset(); }                      //Stop and reset end of character
 }

   //End the character if nothing pressed for defined time and nothing is shown already 
   timeNotPressed = timeWatcher[1].elapsed();
    if (timeNotPressed >= msMax && timeNotPressed >= msEnd && isShown == 0 && backspaceDone == 0) {

      if(mode==1) {
        enterKeyboard(0,morse.getBlueChar());                              //Enter keyboard key
        delay(50);
        clearKeyboard();
      } else if (mode==2) {  
        int* mouseAct;
        mouseAct=morse.getMouse();
        enterMouse((int)mouseAct[0],(int)mouseAct[1],(int)mouseAct[2]);                        //Perform mouse action if it's in morse mouse mode
      }
      
      //Clean up morse code and get ready for next character
      morse.clear();
      isShown = 1;                                                                                //Set variable to is shown                                                                                      
      timeWatcher[1].stop();                                                                               //Stop and reset the timer to form a character
      timeWatcher[1].reset();
  }
  
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

    morseSetup();
    
    //Blink 2 times in modes color 
    //setLedBlink(2,500,modeProperty[switchMode].modeColorNumber,LED_ACTION_BRIGHTNESS);
    modeFeedback(switchMode,500,2);

    //Serial print switch mode
    Serial.print("Switch Mode: ");
    Serial.println(switchMode);

    //Save switch mode in file system 
    writeSettings(SETTINGS_FILE,"mode",switchMode);
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
    morseReactionTime = (pow(1.5,(11-switchReactionLevel))*MORSE_REACTION_TIME);
    delay(25);
  }
  Serial.print("Reaction level: ");
  Serial.println(switchReactionLevel);
  Serial.print("Reaction Time(ms): ");
  Serial.print(switchReactionTime);
  Serial.print("-");
  Serial.println(morseReactionTime);
  writeSettings(SETTINGS_FILE,"reaction_level",switchReactionLevel);        //Save new reaction level
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
    morseReactionTime = (pow(1.5,(11-switchReactionLevel))*MORSE_REACTION_TIME);
    delay(25);
  } 
  Serial.print("Reaction level: ");
  Serial.println(switchReactionLevel);
  Serial.print("Reaction Time(ms): ");
  Serial.print(switchReactionTime);
  Serial.print("-");
  Serial.println(morseReactionTime); 
  writeSettings(SETTINGS_FILE,"reaction_level",switchReactionLevel);        //Save new reaction level 
  delay(25);
}

//***INITIALIZE SETTINGS FILE FUNCTION***//

void initSettings(String fileString,String jsonString){

  const char* fileName = fileString.c_str();
  // Initialize Internal File System
  InternalFS.begin();
  
  file.open(fileName, FILE_O_READ);
  if ( file )
  {
    //Serial.println(SETTINGS_FILE " file exists");
    uint32_t readlen;
    char buffer[64] = { 0 };
    readlen = file.read(buffer, sizeof(buffer));
    delay(5);
    buffer[readlen] = 0;
    file.close();
    delay(5);
  }else
  {
    if( file.open(fileName, FILE_O_WRITE) )
    {
      const char* jsonChar = jsonString.c_str();
      file.write(jsonChar, strlen(jsonChar));
      delay(1);
      file.close();
      delay(1);
    }
  }
}

//***DELETE SETTINGS FILE FUNCTION***//

void deleteSettings(String fileString){
  const char* fileName = fileString.c_str();
  InternalFS.remove(fileName);
  delay(1);  
}

//***FORMAT ALL FILES FUNCTION***//

void formatSettings(){
  InternalFS.format();
  delay(1);
}

//***READ SETTINGS FILE FUNCTION***//

int readSettings(String fileString,String key){
  
  uint32_t readLenght;
  const char* fileName = fileString.c_str();
  
  char buffer[64] = { 0 };
  DynamicJsonDocument doc(1024);
  file.open(fileName, FILE_O_READ);
  delay(1);
  readLenght = file.read(buffer, sizeof(buffer));
  delay(1);
  buffer[readLenght] = 0;
  deserializeJson(doc, String(buffer));
  JsonObject obj = doc.as<JsonObject>();
  int value = obj[key];
  return value;
}

//***WRITE SETTINGS FILE FUNCTION***//

void writeSettings(String fileString,String key,int value){

    uint32_t readLenght;
    const char* fileName = fileString.c_str();
    
    char buffer[64] = { 0 };
    DynamicJsonDocument doc(1024);
    file.open(fileName, FILE_O_READ);
    delay(1);
    
    readLenght = file.read(buffer, sizeof(buffer));
    file.close();
    delay(1);
    
    buffer[readLenght] = 0;
    deserializeJson(doc, String(buffer));
    JsonObject obj = doc.as<JsonObject>();
    
    obj[String(key)] = serialized(String(value));
    String jsonString;
    serializeJson(doc, jsonString);
    const char* jsonChar = jsonString.c_str();
    
    InternalFS.remove(fileName);
    delay(1);
    
    file.open(fileName, FILE_O_WRITE);
    delay(1);
    
    file.write(jsonChar, strlen(jsonChar));
    delay(1);
    file.close();
}
