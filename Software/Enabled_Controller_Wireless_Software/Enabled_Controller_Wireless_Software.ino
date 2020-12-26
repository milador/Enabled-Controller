//Enabled_Controller_Wireless_Software Software
//Enabled_Controller_Wireless_Software
//Written by: Milad Hajihassan
//Version 1.0.1 (25/12/2020)
//Github Link: https://github.com/milador/Enabled-Controller/

#include <bluefruit.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>


BLEDis bledis;
BLEHidAdafruit blehid;


//Can be changed based on the needs of the users 
#define JOYSTICK_DEADZONE 20                                          //Joystick deadzone
#define JOYSTICK_NUMBER 2                                             //A1 = 1 , A2 = 2
#define SWITCH_REACTION_TIME 50                                       //Minimum time for each switch action
#define SWITCH_MODE 1                                                 //Only one mode


#define LED_BRIGHTNESS 100                                             //The mode led color brightness which is always on ( Use a low value to decrease power usage )
#define LED_ACTION_BRIGHTNESS 100                                      //The action led color brightness which can be a higher value than LED_BRIGHTNESS


//Define Switch pins
#define LED_PIN_EXT 5
#define LED_PIN_IN  PIN_NEOPIXEL


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

//Previous status of switches
int switchAPrevState = HIGH;          
int switchBPrevState = HIGH;
int switchCPrevState = HIGH;
int switchDPrevState = HIGH;

int switchUpPrevState = HIGH;          
int switchRightPrevState = HIGH;
int switchDownPrevState = HIGH;
int switchLeftPrevState = HIGH;

//Declare joystick variables 
int joystickX;
int joystickY;


//Declare Switch variables for settings 
int switchConfigured;
int switchReactionTime;
int switchReactionLevel;
int switchMode;


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
  uint8_t switchModeButtonNumber;
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
    {1,"A",1,5},
    {2,"B",2,3},
    {3,"C",3,1},
    {4,"D",4,6},
    {5,"M",5,4}
    
};

//Mode properties 
const modeStruct modeProperty[] {
    {1,"Mode 1",8}
};

uint8_t componentsValue;
bool is400Hz;
uint8_t components = 3; 


//Setup NeoPixel LED
Adafruit_NeoPixel ledPixelExt = Adafruit_NeoPixel(1, LED_PIN_EXT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledPixelIn = Adafruit_NeoPixel();


void setup() {

  ledPixelIn.begin();   
  ledPixelExt.begin();                                                           //Start NeoPixel
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
  pinMode(LED_PIN_EXT, OUTPUT);                                                      


};

void loop() {
  
  
  //Perform joystick actions based on the mode
  joystickAction(switchMode);
  ledPixelExt.show(); 
  ledPixelIn.show();
  delay(5);
}

//***DISPLAY FEATURE LIST FUNCTION***//

void displayFeatureList(void) {

  Serial.println(" ");
  Serial.println(" --- ");
  Serial.println("This is the Enabled Controller USB Software");
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

//***SWITCH FEEDBACK FUNCTION***//

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

//***MODE FEEDBACK FUNCTION***//

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
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName("Enabled-Controller");

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();

  blehid.begin();


  // Set callback for set LED from central
  //blehid.setKeyboardLedCallback(set_keyboard_led);
  // Set up and start advertising
  startAdv();


  //Set joystick x,y range
  //Joystick.setXAxisRange(-127, 127);
  //Joystick.setYAxisRange(-127, 127);

}

void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_JOYSTICK);
  
  
  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();
  
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void set_keyboard_led(uint16_t conn_handle, uint8_t led_bitmap)
{
  (void) conn_handle;
  
  // light up Red Led if any bits is set
  if ( led_bitmap )
  {
    ledOn( LED_RED );
  }
  else
  {
    ledOff( LED_RED );
  }
}


//***PERFORM JOYSTICK ACTIONS FUNCTION***//

void joystickAction(int mode) {

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
  if(JOYSTICK_NUMBER == 1) {
    joystickX = analogRead(JOYSTICK_X1_PIN);
    joystickY = analogRead(JOYSTICK_Y1_PIN);
  } else if (JOYSTICK_NUMBER == 2) {
    joystickX = analogRead(JOYSTICK_X2_PIN);
    joystickY = analogRead(JOYSTICK_Y2_PIN);
  }

    //Perform button actions
    if(!switchAState) {
      switchFeedback(1,mode,switchReactionTime,1);
      //Joystick.pressButton(switchProperty[0].switchModeButtonNumber-1);
      blehid.keyPress('a');
      //enterKeyboard(switchProperty[0].switchButtonName);
    } else if(switchAState && switchAPrevState) {
      //Joystick.releaseButton(switchProperty[0].switchModeButtonNumber-1);
      blehid.keyRelease();
    }
    
    if(!switchBState) {
      switchFeedback(2,mode,switchReactionTime,1);
      //Joystick.pressButton(switchProperty[1].switchModeButtonNumber-1);
    } else if(switchBState && switchBPrevState) {
      //Joystick.releaseButton(switchProperty[1].switchModeButtonNumber-1);
    }
    
    if(!switchCState) {
      switchFeedback(3,mode,switchReactionTime,1);
      //Joystick.pressButton(switchProperty[2].switchModeButtonNumber-1);
    } else if(switchCState && switchDPrevState) {
      //Joystick.releaseButton(switchProperty[2].switchModeButtonNumber-1);
    }
    
    if(!switchDState) {
      switchFeedback(4,mode,switchReactionTime,1);
      //Joystick.pressButton(switchProperty[3].switchModeButtonNumber-1);
    } else if(switchDState && switchDPrevState) {
      //Joystick.releaseButton(switchProperty[3].switchModeButtonNumber-1);
    }

    //Perform d-pad move actions and joystick move
    if(!switchUpState || !switchRightState || !switchDownState || !switchLeftState) {
      switchFeedback(5,mode,switchReactionTime,1);
      if(!switchUpState) {
        //Joystick.setXAxis(0);
        //Joystick.setYAxis(-127); 
      }  
    
      if(!switchRightState) {
        //Joystick.setXAxis(127);
        //Joystick.setYAxis(0); 
      }  
      if(!switchDownState) {
        //Joystick.setXAxis(0);
        //Joystick.setYAxis(127); 
      } 
  
      if(!switchLeftState) {
        //Joystick.setXAxis(-127);
        //Joystick.setYAxis(0); 
      }
    }
    else if((switchUpState && switchUpPrevState) || (switchRightState && switchRightPrevState) || (switchDownState && switchDownPrevState) || (switchLeftState && switchLeftPrevState)) {

      int xx = map(joystickX, 0, 1023, -127, 127);
      int yy = map(joystickY, 0, 1023, -127, 127);
      /*
      if (xx<=JOYSTICK_DEADZONE && xx>=-JOYSTICK_DEADZONE) Joystick.setXAxis(0);  
      else Joystick.setXAxis(xx);  
      
      if (yy<=JOYSTICK_DEADZONE && yy>=-JOYSTICK_DEADZONE) Joystick.setYAxis(0);  
      else Joystick.setYAxis(yy);  
      */
    }

    //Update previous state of buttons 
    switchAPrevState = switchAState;
    switchBPrevState = switchBState;
    switchCPrevState = switchCState;
    switchDPrevState = switchDState;

    switchUpPrevState = switchUpState;
    switchRightPrevState = switchRightState;
    switchDownPrevState = switchDownState;
    switchLeftPrevState = switchLeftState;
    
    delay(100);

}

//***CLEAR JOYSTICK ACTION FUNCTION***//

void joystickClear(){
    //Joystick.setXAxis(0);
    //Joystick.setYAxis(0);  
}

//***SETUP SWITCH MODE FUNCTION***//

void switchSetup() {

    switchMode=SWITCH_MODE;
    switchReactionTime = SWITCH_REACTION_TIME;

    //Serial print settings 
    Serial.print("Switch Mode: ");
    Serial.println(switchMode);


    Serial.print("Reaction Time(ms): ");
    Serial.println(switchReactionTime);
}

//***INIT LED FEEDBACK FUNCTION***//

void initLedFeedback(){
  setLedBlink(2,500,modeProperty[switchMode-1].modeColorNumber,LED_ACTION_BRIGHTNESS);
  delay(5);
  updateLedColor(modeProperty[switchMode-1].modeColorNumber,LED_BRIGHTNESS);
  delay(5);
}
