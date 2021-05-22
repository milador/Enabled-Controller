#include <bluefruit.h>
#include "Enabled_Controller_Config.h"

BLEDis bledis;
BLEHidAdafruit blehid;


void setSwitchLed(uint16_t connectionHandle, uint8_t ledBitmap)
{
  (void) connectionHandle;
  
  // light up Red Led if any bits is set
  if ( ledBitmap )
  {
    ledOn( LED_RED );
  }
  else
  {
    ledOff( LED_RED );
  }
}
//***START BLUETOOTH ADVERTISING***//

void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLUEFRUIT_DEVICE_HID);

  
  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();
  
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

//***SETUP BLUETOOTH FUNCTION***//

void bluetoothSetup(){
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName(BLUEFRUIT_DEVICE_NAME);

  // Configure and Start Device Information Service
  bledis.setManufacturer(BLUEFRUIT_DEVICE_MANUFACTURER);
  bledis.setModel(BLUEFRUIT_DEVICE_MODEL);
  bledis.begin();

  blehid.begin();

  // Set callback for set LED from central
  blehid.setKeyboardLedCallback(setSwitchLed);
  // Set up and start advertising
  startAdv();
  
}


//***ENTER KEYBOARD ACTIONS FUNCTION***//

void enterKeyboard(uint8_t modifierCode, uint8_t keyCode) {
  uint8_t keyCodePointer[6] = {keyCode};
  blehid.keyboardReport(modifierCode, keyCodePointer);
}

//***ENTER KEYBOARD CHAR ACTIONS FUNCTION***//

void enterKeyboardChar(char charachter) {
  blehid.keyPress(charachter);
}

//***CLEAR KEYBOARD FUNCTION***//

void clearKeyboard() {
  blehid.keyRelease();
}


//***PERFORM MOUSE ACTIONS FUNCTION***//

void enterMouse(int button,int xValue,int yValue) {
    switch (button) {
      case 0:
        break;
      case 1:
        blehid.mouseButtonPress(MOUSE_BUTTON_LEFT);
        delay(50);
        blehid.mouseButtonRelease();
        break;
      case 2:
        blehid.mouseButtonPress(MOUSE_BUTTON_RIGHT);
        delay(50);
        blehid.mouseButtonRelease();
        break;
      case 3:
        blehid.mouseButtonPress(MOUSE_BUTTON_LEFT);
        delay(50);
        blehid.mouseButtonRelease();
        blehid.mouseButtonPress(MOUSE_BUTTON_LEFT);
        delay(50);
        blehid.mouseButtonRelease();
        break;        
      case 4:
        blehid.mouseButtonPress(MOUSE_BUTTON_RIGHT);
        delay(50);
        blehid.mouseButtonRelease();
        blehid.mouseButtonPress(MOUSE_BUTTON_RIGHT);
        delay(50);
        blehid.mouseButtonRelease();
        break;
      case 5:             
        blehid.mouseButtonPress(MOUSE_BUTTON_LEFT);
        delay(200);
        blehid.mouseButtonRelease();
        break;
      case 6:
        blehid.mouseButtonPress(MOUSE_BUTTON_RIGHT);
        delay(200);
        blehid.mouseButtonRelease();
        break;                           
  };
  
  
  blehid.mouseMove(xValue, yValue);
  
}

//***CLEAR MOUSE FUNCTION***//

void clearMouse() {
  blehid.mouseButtonRelease();
}
