
/** ************************************************************************
 * File Name: Enabled_Controller_USB_Software.ino 
 * Title: Enabled Controller USB Software
 * Developed by: Milad Hajihassan
 * Version Number: 1.2 (21/5/2021)
 * Github Link: https://github.com/milador/Enabled_Controller
 ***************************************************************************/

#include "Mouse.h"
#include "Keyboard.h"
#include <StopWatch.h>
#include "EasyMorse.h"
#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#include <FlashStorage.h>


//Can be changed based on the needs of the users
#define JOYSTICK_ENABLED false                                        //Enable Joystick 
#define JOYSTICK_DEADZONE 20                                          //Joystick deadzone
#define JOYSTICK_NUMBER 2                                             //A1 = 1 , A2 = 2
#define MORSE_TIMEOUT 1000                                            //Maximum timeout (1000ms =1s)
#define MORSE_REACTION_TIME 10                                        //Minimum time for a dot or dash switch action ( level 10 : (1.5^1)x10 =15ms , level 1 : (1.5^10)x10=570ms )
#define MOUSE_MOVE_MULTI 2                                            //Morse mouse move multiplier 
#define SWITCH_REACTION_TIME 50                                       //Minimum time for each switch action ( level 10 : 1x50 =50ms , level 1 : 10x50=500ms )
#define SWITCH_MODE_CHANGE_TIME 2000                                  //How long to hold switch 4 to change mode 
#define SWITCH_MAC_PROFILE false                                      //Windows,Android,iOS=false MacOS=true 

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

EasyMorse morse;

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
StopWatch switchDTimeWatcher[1];

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
    {1,"DOT",'a',KEY_F1,MOUSE_LEFT,5,1},                       //{1=dot,"DOT",'a','F1',Left Click,5=blue,1=1xMORSE_REACTION}
    {2,"DASH",'b',KEY_F2,MOUSE_RIGHT,6,3},                     //{2=dash,"DASH",'b','F2',Right Click,6=red,3=3xMORSE_REACTION}
    {3,"C",'c',KEY_F3,0,1,1},                                  //{3,"C",'c','F3',No Click,1=green,1=1xMORSE_REACTION}
    {4,"D",'d',KEY_F4,0,3,1},                                  //{4,"D",'d','F4',No Click,3=yellow,1=1xMORSE_REACTION}
    {5,"UP",KEY_UP_ARROW,KEY_F5,0,4,1},                        //{5,"UP",'UP','F5',No Click,4=orange,1=1xMORSE_REACTION}
    {6,"RIGHT",KEY_RIGHT_ARROW,KEY_F6,0,4,1},                  //{6,"RIGHT",'RIGHT','F6',No Click,4=orange,1=1xMORSE_REACTION}
    {7,"DOWN",KEY_DOWN_ARROW,KEY_F7,0,4,1},                    //{7,"DOWN",'DOWN','F7',No Click,4=orange,1=1xMORSE_REACTION}
    {8,"LEFT",KEY_LEFT_ARROW,KEY_F8,0,4,1},                    //{8,"LEFT",'LEFT','F8',No Click,4=orange,1=1xMORSE_REACTION}
    {9,"ANALOG",' ',' ',0,4,1}                                   //{9,"ANALOG",'NONE','F9',No Click,4=orange,1=1xMORSE_REACTION}
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
Adafruit_NeoPixel ledPixels = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_DotStar dotStar(1, DOTSTAR_DATA_PIN, DOTSTAR_CLOCK_PIN, DOTSTAR_BRG);


void setup() {

  ledPixels.begin();                                                           //Start NeoPixel
  Serial.begin(115200);                                                        //Start Serial
  Keyboard.begin();                                                            //Starts keyboard emulation
  Mouse.begin();                                                               //Starts mouse emulation
  initJoystick();
  delay(1000);
  switchSetup();                                                               //Setup switch
  delay(5);
  initLedFeedback();                                                          //Led will blink in a color to show the current mode 
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
  pinMode(LED_PIN, OUTPUT);                                                      


};

void loop() {
  static int ctr;                          //Control variable to set previous status of switches 
  unsigned long timePressed;               //Time that switch one or two are pressed
  unsigned long timeNotPressed;            //Time that switch one or two are not pressed
  static int previousSwitchDState;         //Previous status of switch D
  
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
    previousSwitchDState = HIGH;  
    ctr++;
  }
  //Check if switch D is pressed to change switch mode
  if (switchDState == LOW && previousSwitchDState == HIGH) {
     if (switchDState == LOW) { 
      previousSwitchDState = LOW; 
     }
     switchDTimeWatcher[0].stop();                                //Reset and start the timer         
     switchDTimeWatcher[0].reset();                                                                        
     switchDTimeWatcher[0].start(); 
  }
  // Switch D was released
  if (switchDState == HIGH && previousSwitchDState == LOW) {
    previousSwitchDState = HIGH;
    timePressed = switchDTimeWatcher[0].elapsed();                //Calculate the time that switch one was pressed 
    switchDTimeWatcher[0].stop();                                 //Stop the single action (dot/dash) timer and reset
    switchDTimeWatcher[0].reset();
    //Perform action if the switch has been hold active for specified time
    if (timePressed >= SWITCH_MODE_CHANGE_TIME){
      changeSwitchMode();                                                                
    } else if(switchMode==1) {
      keyboardAction(switchAState,switchBState,switchCState,LOW,switchUpState,switchRightState,switchDownState,switchLeftState);
    } else if(switchMode==4) {
       mouseAction(switchAState,switchBState,switchCState,LOW,switchUpState,switchRightState,switchDownState,switchLeftState,joystickX,joystickY);                        //Mouse mode
    }
  }
  //Serial.println(switchAState);
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
  ledPixels.show(); 
  delay(5);
}

//***DISPLAY FEATURE LIST FUNCTION***//

void displayFeatureList(void) {

  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("Enabled Controller USB firmware");
  Serial.println(" ");
  Serial.println("VERSION: 1.2 (21 May 2021)");
  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("Features: Adaptive switch, Morse Keyboard, Morse Mouse, and Mouse");
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
      //Serial.println("Switch A");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[0].switchMacChar) : Keyboard.press(switchProperty[0].switchChar);
    } else if(!switch2) {
      switchFeedback(2,switchMode,switchReactionTime,1);
      //Serial.println("Switch B");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[1].switchMacChar) : Keyboard.press(switchProperty[1].switchChar);
    } else if(!switch3) {
      switchFeedback(3,switchMode,switchReactionTime,1);
      //Serial.println("Switch C");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[2].switchMacChar) : Keyboard.press(switchProperty[2].switchChar);
    } else if(!switch4) {
      switchFeedback(4,switchMode,switchReactionTime,1);
      //Serial.println("Switch D");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[3].switchMacChar) : Keyboard.press(switchProperty[3].switchChar);
    } else if(!switch5) {
      switchFeedback(5,switchMode,switchReactionTime,1);
      //Serial.println("Switch UP");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[4].switchMacChar) : Keyboard.press(switchProperty[4].switchChar);
    } else if(!switch6) {
      switchFeedback(6,switchMode,switchReactionTime,1);
      //Serial.println("Switch RIGHT");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[5].switchMacChar) : Keyboard.press(switchProperty[5].switchChar);
    } else if(!switch7) {
      switchFeedback(7,switchMode,switchReactionTime,1);
      //Serial.println("Switch DOWN");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[6].switchMacChar) : Keyboard.press(switchProperty[6].switchChar);
    } else if(!switch8) {
      switchFeedback(8,switchMode,switchReactionTime,1);
      //Serial.println("Switch LEFT");
      (SWITCH_MAC_PROFILE) ? Keyboard.press(switchProperty[7].switchMacChar) : Keyboard.press(switchProperty[7].switchChar);
    }
    else
    {
      Keyboard.releaseAll();
    }
    delay(5);

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
  int i, j;
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
   
   backspaceDone = 0;                                                                           //Backspace is not entered 
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
    if(mode==1) Keyboard.write(8);                                                               //Press Backspace key
    backspaceDone = 1;                                                                          //Set Backspace done already
    isShown = 1;
    for (i=1; i<3; i++) { timeWatcher[i].stop(); timeWatcher[i].reset(); }                      //Stop and reset end of character
 }

   //End the character if nothing pressed for defined time and nothing is shown already 
   timeNotPressed = timeWatcher[1].elapsed();
    if (timeNotPressed >= msMax && timeNotPressed >= msEnd && isShown == 0 && backspaceDone == 0) {
      //Serial.println(morse.getCharNum()); 
      if(mode==1) {
       Keyboard.write(morse.getCharAscii());                                                  //Enter keyboard key based on ascii code if it's in morse keyboard mode
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


//***PERFORM MOUSE ACTIONS FUNCTION***//

void enterMouse(int button,int xValue,int yValue) {
    switch (button) {
      case 0:
        break;
      case 1:
        if (!Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.press(MOUSE_LEFT);
          delay(50);
          Mouse.release(MOUSE_LEFT);
        } 
        else if (Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.release(MOUSE_LEFT);
        }    
        break;
      case 2:
        if (!Mouse.isPressed(MOUSE_RIGHT)) {
          Mouse.press(MOUSE_RIGHT);
          delay(50);
          Mouse.release(MOUSE_RIGHT);
        }
        else if (Mouse.isPressed(MOUSE_RIGHT)) {
          Mouse.release(MOUSE_RIGHT);
        }   
        break;
      case 3:
        if (!Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.press(MOUSE_LEFT);
          delay(50);
          Mouse.release(MOUSE_LEFT);
          delay(50);
          Mouse.press(MOUSE_LEFT);
          delay(50);
          Mouse.release(MOUSE_LEFT);                    
        }
        else if (Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.release(MOUSE_LEFT);
        }   
        break;        
      case 4:
        if (!Mouse.isPressed(MOUSE_RIGHT)) {
          Mouse.press(MOUSE_RIGHT);
          delay(50);
          Mouse.release(MOUSE_RIGHT);
          delay(50);
          Mouse.press(MOUSE_RIGHT);
          delay(50);
          Mouse.release(MOUSE_RIGHT);                    
        }
        else if (Mouse.isPressed(MOUSE_RIGHT)) {
          Mouse.release(MOUSE_RIGHT);
        }   
        break;
      case 5:             
        if (!Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.press(MOUSE_LEFT);
        } 
        else if (Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.release(MOUSE_LEFT);
        }    
        break;
      case 6:
        if (!Mouse.isPressed(MOUSE_RIGHT)) {
          Mouse.press(MOUSE_RIGHT);
        } 
        else if (Mouse.isPressed(MOUSE_RIGHT)) {
          Mouse.release(MOUSE_RIGHT);
        }    
        break;                           
  };

  Mouse.move(xValue*MOUSE_MOVE_MULTI, yValue*MOUSE_MOVE_MULTI, 0);

}

//***PERFORM MOUSE ACTIONS FUNCTION***//

void clearMouse() {
  Mouse.move(0, 0, 0);
  Mouse.release();
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
    //setLedBlink(6,100,3,LED_ACTION_BRIGHTNESS);
    settingsFeedback(3,switchMode,100,6);
    switchReactionLevel = 10;
  } else {
    //setLedBlink(switchReactionLevel,100,4,LED_ACTION_BRIGHTNESS);
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
    morseReactionTime = (pow(1.5,(11-switchReactionLevel))*MORSE_REACTION_TIME);
    delay(25);
  } 
  Serial.print("Reaction level: ");
  Serial.println(switchReactionLevel);
  Serial.print("Reaction Time(ms): ");
  Serial.print(switchReactionTime);
  Serial.print("-");
  Serial.println(morseReactionTime);  
  
  switchReactionLevelFlash.write(switchReactionLevel);
  delay(25);
}
