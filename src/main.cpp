/*  
    Copyright 2021, 2022, 2023, 2024, 2025 Andreas Petersik (andreas.petersik@gmail.com)
    
    This file is part of the Chesstimation Project.

    Chesstimation is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Chesstimation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Chesstimation.  If not, see <https://www.gnu.org/licenses/>.
*/

#define VERSION     "Chesstimation 1.7.1"
#define ABOUT_TEXT  "\nby Dr. Andreas Petersik\nandreas.petersik@gmail.com\n\nbuilt: Oct 30th, 2025"
// #define BOARD_TEST

#include <Arduino.h>
#include <SPIFFS.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>
#include <BluetoothSerial.h>
#include "lv_i18n.h"

#include <BLEDevice.h>
#include <BLE2902.h>
#include "BLEHIDDevice.h"

// For emulations:
#include <thread>
#include <chrono>

// my includes:

#include <mephisto.h>
#include <board.h>

#include "SPI.h"
#include "TFT_eSPI.h"

#include "../lvgl/src/lvgl.h"

#define LOLIN_D32
#define LED_TIME 300
#define MIN_BRIGHTNESS 190

#define TOUCH_PANEL_IRQ_PIN   GPIO_NUM_34   // The idea is to check if there is a signal change on this pin for waking ESP32 from sleep! Works! Pin is high when no touch, low when touch!

#define BOARD_SETUP_FILE  "/board_setup"
#define SQUARE_SIZE         40

// #define PEGASUS

Mephisto mephisto;
Board chessBoard;
BluetoothSerial SerialBT;

char modeBleAdvertisedName[] = "MILLENNIUM CHESS";

int millBLEinitialized = 0;
int physicalConformity = 0;

enum connectionType {USB, BT, BLE} connection;  // Should be same order as in UI
enum languageType {EN, ES, DE} language;        // Should be same order as in UI

byte readRawRow[8];
byte led_buffer[8];
byte mephistoLED[8][8];
byte eeprom[5]={0,20,3,20,15};
byte LED_startup_sequence[64] = {0,1,2,3,4,5,6,7,15,23,31,39,47,55,63,62,61,60,59,58,57,56,48,40,32,24,16,8, 9,10,11,12,13,14,22,30,38,46,54,53,52,51,50,49,41,33,25,17, 18,19,20,21,29,37,45,44,43,42,34,26, 27,28,36,35};
byte oldBoard[64];
int brightness = 255;

uint16_t calibrationData[5];

//BLE objects
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLEService *pService;

SemaphoreHandle_t sendBLEsemaphore = NULL;

char incomingMessage[170];
std::string replyString;

TFT_eSPI tft = TFT_eSPI();

/*Set to your screen resolution*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DISP_BUF_SIZE (320 * 96)
// #define DISP_BUF_SIZE (320 * 60) // old size used for LVGL 8.x
// #define DISP_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 6 * (LV_COLOR_DEPTH / 8)) // leads to not working White Pawn
// lv_color_t *dispBuf;//[DISP_BUF_SIZE]; 
void *dispBuf;//[DISP_BUF_SIZE]; 

lv_display_t * disp;

lv_style_t fMediumStyle;
lv_style_t fLargeStyle;
lv_style_t fExtraLargeStyle;
lv_style_t light_square;
lv_style_t dark_square;

LV_FONT_DECLARE(montserrat_umlaute20);
LV_FONT_DECLARE(montserrat_umlaute22);
LV_IMG_DECLARE(WP40);
LV_IMG_DECLARE(BP40);
LV_IMG_DECLARE(WK40);
LV_IMG_DECLARE(BK40);
LV_IMG_DECLARE(WB40);
LV_IMG_DECLARE(BB40);
LV_IMG_DECLARE(WQ40);
LV_IMG_DECLARE(BQ40);
LV_IMG_DECLARE(WN40);
LV_IMG_DECLARE(BN40);
LV_IMG_DECLARE(WR40);
LV_IMG_DECLARE(BR40);

lv_indev_t * indev;

lv_obj_t *settingsScreen, *settingsBtn, *btn2, *screenMain, *liftedPiecesLbl, *liftedPiecesStringLbl, *debugLbl, *chessBoardCanvas, *chessBoardLbl, *batteryLbl;
lv_obj_t *labelA1, *exitSettingsBtn, *offBtn, *certaboCalibCB, *restartBtn, *certaboCB, *chesslinkCB, *languageLbl, *flippedCB, *pegasusCB, *testCB;
lv_obj_t *square[64], *dummy1Btn, *calibrateBtn, *object, *brightnessSlider, *connectionLbl, *commLbl, *emulatorsBtn, *langBtn, *langLbl;
lv_obj_t *ui_settings_obj, *offBtnLbl, *newGameLbl, *brightLbl, *backLbl, *settingsLbl, *emulationLbl, *langDd, *connectionDd;
lv_obj_t *promotionBtnW, *promotionBtnImageW, *promotionBtnB, *promotionBtnImageB;
lv_obj_t *wp[8], *bp[8], *wk, *bk, *wn1, *bn1, *wn2, *bn2, *wb1, *bb1, *wb2, *bb2, *wr1, *br1, *wr2, *br2, *wq1, *bq1, *wq2, *bq2;

void displayLEDstartUpSequence()
{
  for(int i=0; i<64; i++)
  {
    mephisto.writeRow(getRowFromBoardIndex(LED_startup_sequence[i]), 0x1 << getColFromBoardIndex(LED_startup_sequence[i]));
    delay(48);
  }
}

void assembleIncomingChesslinkMessage(char readChar)
{
  switch (readChar)
  {
  case 'R':
  case 'I':
  case 'L':
  case 'V':
  case 'W':
  case 'S':
  case 'X':
    incomingMessage[0] = readChar;
    incomingMessage[1] = 0;
    break;
  default:
    byte position = strlen(incomingMessage);
    incomingMessage[position] = readChar;
    incomingMessage[position + 1] = 0;
    break;
  }
}

void displayAboutBox()
{
  object = lv_msgbox_create(screenMain);

  lv_msgbox_add_title(object, VERSION);
  lv_msgbox_add_text(object, ABOUT_TEXT);
  lv_msgbox_add_close_button(object);

  lv_obj_add_style(object, &fLargeStyle, 0);
  lv_obj_set_size(object, 368, 186);
  lv_obj_center(object);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

byte debugPrint(const char *message)
{
  if(connection != USB) 
  {
    Serial.print(message);
    return 1;
  }
  if(connection != BT) 
  {
    SerialBT.print(message);
    return 1;
  }
  return 0;
}

void startTouchCalibration()
{
    // object = lv_msgbox_create(NULL, "Touch Calibration", "Touch the corners with the white line\nin counter clockwise order", NULL, true);

    // lv_obj_add_style(object, &fLargeStyle, 0);
    // lv_obj_set_size(object, 480, 320);
    // lv_obj_center(object);
    // lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

    tft.calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
    tft.fillRect(480-16, 320-16, 16, 16, TFT_RED);

    lv_obj_invalidate(settingsScreen);

    // lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
}

byte debugPrintln(const char *message)
{
  if(connection != USB) 
  {
    Serial.println(message);
    return 1;
  }
  if(connection != BT) 
  {
    SerialBT.println(message);
    return 1;
  }
  return 0;
}

void saveBoardSettings(void)
{
  connectionType saveConnection = USB;

  if (!SPIFFS.begin())
  {
    SPIFFS.format();
    SPIFFS.begin();
  }

  if ((connection == BLE) && (chessBoard.emulation != 0))
  {
    saveConnection = BLE;
  }
  if (connection == BT)
  {
    saveConnection = BT;
  }

  // store data
  File f = SPIFFS.open(BOARD_SETUP_FILE, "w");
  if (f)
  {
    f.write((const byte *)chessBoard.piece, 64);
    f.write((const byte *)chessBoard.piecesLifted, 64);
    f.write(chessBoard.liftedIdx);
    f.write(chessBoard.emulation);
    f.write(chessBoard.flipped);
    f.write(saveConnection);
    f.write(brightness);
    f.write(language);
    f.close();
  }
}

void loadBoardSettings(void)
{

  byte tempBoardSetup[64];
  uint16_t tempPiecesLifted[32];
  uint8_t tempInt8[1];
  physicalConformity = 1;

  if (!SPIFFS.begin())
  {
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if file exists
  if (SPIFFS.exists(BOARD_SETUP_FILE))
  {
    File f = SPIFFS.open(BOARD_SETUP_FILE, "r");
    if (f)
    {
      if (f.readBytes((char *)tempBoardSetup, 64) == 64)
      {
        for (int i = 0; i < 64; i++)
        {
          chessBoard.piece[i] = tempBoardSetup[i];
        }
      }
      if (f.readBytes((char *)tempPiecesLifted, 64) == 64)
      {
        for (int i = 0; i < 32; i++)
        {
          chessBoard.piecesLifted[i] = tempPiecesLifted[i];
        }
      }
      if (f.readBytes((char *)tempInt8, 1) == 1)
      {
        chessBoard.liftedIdx = tempInt8[0];
      }
      if (f.readBytes((char *)tempInt8, 1) == 1)
      {
        chessBoard.emulation = tempInt8[0];
      }
      if (f.readBytes((char *)tempInt8, 1) == 1)
      {
        chessBoard.flipped = tempInt8[0];
      }
      if (f.readBytes((char *)tempInt8, 1) == 1)
      {
        connection = (connectionType)tempInt8[0];
      }
      if (f.readBytes((char *)tempInt8, 1) == 1)
      {
        if(tempInt8[0] < MIN_BRIGHTNESS)
        {
          brightness = MIN_BRIGHTNESS;
        }
        else
        {
          brightness = tempInt8[0];
        }
      }
      if (f.readBytes((char *)tempInt8, 1) == 1)
      {
        language = (languageType)tempInt8[0];
      }
      f.close();
    }
  }
}

void updateMephistoLEDs(byte mephistoLED[8][8]) {
  for(byte row=0; row<8; row++) {
    for(byte col=0; col<8; col++) {
      mephistoLED[col][row]=chessBoard.milleniumLEDs[col][row] & chessBoard.milleniumLEDs[col+1][row]
        & chessBoard.milleniumLEDs[col][row+1] & chessBoard.milleniumLEDs[col+1][row+1];
    }
  }
}

byte getOddParity(byte value)
{
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  return (~value) & 1;
}

void sendMessageToChessBoard(const char *message)
{
  std::string codedMessage = message;
  char blockCode[3];

  debugPrintln("Message to be sent from Board to Application: ");
  debugPrintln(message);

  if (chessBoard.emulation == 1)
  {
    sprintf(blockCode, "%02X", chessBoard.calcBlockPar(message));
    codedMessage += blockCode;
  }

  if (connection == BLE)
  {

    for (int i = 0; i < codedMessage.length(); i++)
    {
      if (getOddParity(codedMessage[i]))
      {
        codedMessage[i] += 128;
      }
    }
    // debugPrint("Final Message to be sent: ");
    // debugPrintln(codedMessage.c_str());

    if (xSemaphoreTake(sendBLEsemaphore, portMAX_DELAY) == pdTRUE)
    {
      for (int i = 0; i < codedMessage.length(); i += 8)
      {
        pTxCharacteristic->setValue(codedMessage.substr(i, 8));
        pTxCharacteristic->notify();
      }
      xSemaphoreGive(sendBLEsemaphore);
    }
    else
    {
      debugPrintln("*** Semaphore could not be taken - ERROR sending message ***");
    }
  }
  if (connection == BT)
  {
    SerialBT.print(codedMessage.c_str());
    SerialBT.flush();
  }
  if (connection == USB)
  {
    Serial.print(codedMessage.c_str());
    Serial.flush();
  }
}

void updateSettingsScreen()
{
  if(chessBoard.emulation==0) 
  {
#ifdef PEGASUS
    lv_obj_clear_state(pegasusCB, LV_STATE_CHECKED);  
#endif
    lv_obj_add_state(certaboCB, LV_STATE_CHECKED);
    lv_obj_clear_state(chesslinkCB, LV_STATE_CHECKED);  
  }
  if(chessBoard.emulation==1) 
  {
#ifdef PEGASUS
    lv_obj_clear_state(pegasusCB, LV_STATE_CHECKED);  
#endif
    lv_obj_clear_state(certaboCB, LV_STATE_CHECKED);
    lv_obj_add_state(chesslinkCB, LV_STATE_CHECKED);  
  }
  if(chessBoard.emulation==2) 
  {
    lv_obj_clear_state(certaboCB, LV_STATE_CHECKED);
    lv_obj_clear_state(chesslinkCB, LV_STATE_CHECKED);
#ifdef PEGASUS
    lv_obj_add_state(pegasusCB, LV_STATE_CHECKED);  
#endif
  }
  if(chessBoard.flipped) 
  {
    lv_obj_add_state(flippedCB, LV_STATE_CHECKED);
  }
  if(!chessBoard.flipped) 
  {
    lv_obj_clear_state(flippedCB, LV_STATE_CHECKED);
  }
}

class MyServerCallbacksPegasus : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    connection = BLE;
    updateSettingsScreen();
    debugPrintln("Pegasus Emulation: BLE DEVICE CONNECTED");
  };

  void onDisconnect(BLEServer *pServer)
  {
    debugPrintln("Pegasus Emulation: BLE DEVICE DISCONNECTED");
    updateSettingsScreen();
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
  }
};

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    connection = BLE;
    updateSettingsScreen();
    debugPrintln("Millennium Emulation: BLE DEVICE CONNECTED");
  };

  void onDisconnect(BLEServer *pServer)
  {
    debugPrintln("Millennium Emulation: BLE DEVICE DISCONNECTED");
    millBLEinitialized = 0;
    updateSettingsScreen();
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
  }
};

void sendChesslinkAnswer(char *incomingMessage)
{
  if (strlen(incomingMessage) == 3 && (strcmp(incomingMessage, "V56") == 0))
  {
    debugPrint("Detected valid incoming Version Request Message V: ");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard("v0017");  // identify as Millennium Exclusive board: v0.23 in WhitePawn (00.00 - 00.FF)
    return;
  }
  if (strlen(incomingMessage) == 5 && incomingMessage[0] == 'R')
  {
    debugPrint("Detected incoming Read EEPROM Message R: ");
    debugPrintln(incomingMessage);
    incomingMessage[0] = 'r';
    char twoDigits[3];
    sprintf(twoDigits, "%02X", eeprom[incomingMessage[2] - '0']);
    debugPrint("EEPROM Pos requested: ");
    debugPrint(String(incomingMessage[2] - '0').c_str());
    debugPrint(" Content to be sent: ");
    debugPrintln(twoDigits);
    incomingMessage[3] = twoDigits[0];
    incomingMessage[4] = twoDigits[1];
    sendMessageToChessBoard(incomingMessage);
    return;
  }
  if (strlen(incomingMessage) == 7 && incomingMessage[0] == 'W')
  {
    debugPrint("Detected incoming Write EEPROM Message W: ");
    debugPrintln(incomingMessage);
    incomingMessage[0] = 'w';

    char twoDigits[3];
    twoDigits[0] = incomingMessage[3];
    twoDigits[1] = incomingMessage[4];
    twoDigits[2] = 0;
    eeprom[incomingMessage[2] - '0'] = strtol(twoDigits, NULL, 16);

    debugPrint("Write to EEPROM Pos: ");
    debugPrint(String(incomingMessage[2] - '0').c_str());
    debugPrint(" Value: ");
    debugPrintln(String(eeprom[incomingMessage[2] - '0']).c_str());

    incomingMessage[5] = 0;
    sendMessageToChessBoard(incomingMessage);
    return;
  }
  if (strlen(incomingMessage) == 3 && (strcmp(incomingMessage, "S53") == 0))
  {
    debugPrint("Detected valid incoming Status request Message S: ");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard(chessBoard.boardMessage);
    return;
  }
  if (strlen(incomingMessage) == 167 && incomingMessage[0] == 'L')
  {
    debugPrint("Detected incoming set LED Message L: ");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard("l");
    // Does not help to get the LEDs work with the Chess Link App:
    // chessBoard.generateSerialBoardMessage();
    // sendMessageToChessBoard(chessBoard.boardMessage);
    // sendMessageToChessBoard(chessBoard.boardMessage);
    // sendMessageToChessBoard(chessBoard.boardMessage);

    incomingMessage[165] = 0;
    chessBoard.updateMilleniumLEDs((&incomingMessage[1]));

    // Print DEBUG LED pattern:
    debugPrintln("DEBUG Output Millennium LED Pattern:");
    for (byte row = 0; row < 9; row++)
    {
      for (byte col = 0; col < 9; col++)
      {
        sprintf(incomingMessage, "%02X", chessBoard.milleniumLEDs[col][row]); // ATTENTION incomingMessage is used here for Debug Output!
        debugPrint(incomingMessage);
        debugPrint(" ");
      }
      debugPrintln("");
    }
    debugPrintln("");
    updateMephistoLEDs(mephistoLED);
    millBLEinitialized = 1;
    return;
  }
  if (strlen(incomingMessage) == 3 && incomingMessage[0] == 'I')
  {
    debugPrint("Detected incoming Message I: ");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard("iFF\n");  // Don't understand I-Message!
    return;
  }
  if (strlen(incomingMessage) == 3 && (strcmp(incomingMessage, "X58") == 0))
  {
    debugPrint("Detected valid incoming Message X: ");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard("x");
    chessBoard.extinguishMilleniumLEDs();
    updateMephistoLEDs(mephistoLED);
    millBLEinitialized = 1;
    return;
  }
}

class MyCallbacksChesslink : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    // char tempString[64];
    // sprintf(tempString, "received msg-length: %i", rxValue.length());
    // debugPrintln(tempString);
    // your stuff to process incoming data
    char readChar;

    if (rxValue.length() > 0)
    {
      for (int i = 0; i < rxValue.length(); i++)
      {
        readChar = (rxValue[i] & 127); // remove odd parity

        assembleIncomingChesslinkMessage(readChar);
      }
      sendChesslinkAnswer(incomingMessage);
    }
  }
};

class MyCallbacksPegasus : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    // your stuff to process incoming data
    char readChar;

    if (rxValue.length() > 0)
    {
      for (int i = 0; i < rxValue.length(); i++)
      {
        readChar = rxValue[i];

        debugPrintln(""+readChar);
      }
      // sendChesslinkAnswer(incomingMessage);
    }
  }
};

void initBleServicePegasus()
{
  //Bluetooth BLE initialization for mode B boards
  //esp_log_level_set("*", ESP_LOG_VERBOSE);

  //Register and initialize BLE Transparent UART Mode.
  BLEDevice::deinit(true);
  BLEDevice::init("DGT Pegasus");
  BLEDevice::setMTU(517);
  // BLEDevice::setMTU(192);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacksPegasus());

  // Create the BLE Service for Transparent UART Mode.
  pService = pServer->createService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E"); // Nordic BLE Service
  // pService = pServer->createService("49535343-fe7d-4ae5-8fa9-9fafd205e455");

  // Create a BLE Characteristic for TX data
  pTxCharacteristic = pService->createCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLECharacteristic::PROPERTY_NOTIFY); // Nordic TX Characteristic
  // pTxCharacteristic = pService->createCharacteristic("49535343-1e4d-4bd9-ba61-23c647249616", BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Create a BLE Characteristic for RX data
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLECharacteristic::PROPERTY_WRITE); // Nordic RX Characteristic
  // BLECharacteristic *pRxCharacteristic = pService->createCharacteristic("49535343-8841-43f4-a8d4-ecbe34729bb3", BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacksPegasus());

  // Start the service
  pService->start();
  // Advertise the service
  pServer->getAdvertising()->start();

}

void initBleServiceChesslink()
{
  //Bluetooth BLE initialization for mode B boards
  //esp_log_level_set("*", ESP_LOG_VERBOSE);

  //Register and initialize BLE Transparent UART Mode.
  BLEDevice::deinit(true);
  BLEDevice::init(modeBleAdvertisedName);
  BLEDevice::setMTU(185);
  // BLEDevice::setMTU(517);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service for Transparent UART Mode.
  pService = pServer->createService("49535343-fe7d-4ae5-8fa9-9fafd205e455");
  // pService = pServer->createService("94f39d29-7d6d-437d-973b-fba39e49d4ee");

  // Create a BLE Characteristic for TX data
  pTxCharacteristic = pService->createCharacteristic("49535343-1e4d-4bd9-ba61-23c647249616", BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Create a BLE Characteristic for RX data
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic("49535343-8841-43f4-a8d4-ecbe34729bb3", BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacksChesslink());

  // Start the service
  pService->start();
  // Advertise the service
  pServer->getAdvertising()->start();

   pServer->getAdvertising()->setAppearance(GENERIC_HID);

  pServer->getAdvertising()->addServiceUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455");

  pServer->getAdvertising()->setScanResponse(true);

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  oAdvertisementData.setShortName(modeBleAdvertisedName);
  oAdvertisementData.setName(modeBleAdvertisedName);
  oAdvertisementData.setAppearance(GENERIC_HID);

  pServer->getAdvertising()->setAdvertisementData(oAdvertisementData);

  pServer->getAdvertising()->start();
  
   //semaphore to handle data over BLE
  sendBLEsemaphore = xSemaphoreCreateMutex();
  xSemaphoreGive(sendBLEsemaphore);
}

void initSerialPortCommunication(void)
{
  if (connection != USB)
  {
    // Serial Port is used for debugging!
    // Serial.end();
    Serial.begin(115200);
  }
  else if (connection != BT)
  {
    // SerialBT.end();
    SerialBT.begin("Chesstimation DEBUG");
  }
  if (chessBoard.emulation == 0)
  {
    if (connection == USB)
    {
      // Serial.end();
      Serial.begin(38400);
    }
    if(connection == BT)
    {
      // SerialBT.end();
      SerialBT.begin("Certabo");
    }
  }
  if (chessBoard.emulation == 1)
  {
    if (connection == USB)
    {
      // Serial.end();
      Serial.begin(38400, SERIAL_7O1);
      // Serial.begin(38400); // ESP32 S3, SERIAL_7O1);
    }
    if(connection == BT)
    {
      // SerialBT.end();
      SerialBT.begin("MILLENNIUM CHESS BT");
    }
    if(connection == BLE)
    {
      initBleServiceChesslink();
    }
  }

  // Update UI:
  if(connection == USB) 
  {
    lv_label_set_text(connectionLbl, LV_SYMBOL_USB);
  }
  else
  {
    lv_label_set_text(connectionLbl, LV_SYMBOL_BLUETOOTH);
  }
  lv_dropdown_set_selected(connectionDd, connection);
}

void resetOldBoard()
{
  for (int i = 0; i < 64; i++)
  {
    oldBoard[i]=EMP;
  }
  // Hide additional queens, otherwise they will re-appear!
  lv_obj_add_flag(wq2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(bq2, LV_OBJ_FLAG_HIDDEN);

}

void updatePiecesOnBoard()
{
  // Check if update for chess board display is needed:

  byte certPiece;
  byte updated = 0;

  for (int i = 0; i < 64; i++)
  {
    
    certPiece = chessBoard.piece[i];
    
    if (oldBoard[i] != certPiece)
    {
      // Check for Black Pawns (these have binary pattern 11 at the end (see board.h))
      if ((certPiece & 0b00001111) == 0b00000011)
      {
        int pawnIndex = (certPiece >> 4) & 0x0f;
        // Security check to ensure that array is addressed correctly:
        if (pawnIndex < 8) {
          lv_obj_set_pos(bp[pawnIndex], getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
          lv_obj_clear_flag(bp[pawnIndex], LV_OBJ_FLAG_HIDDEN);
        }
      }
      // Check for White Pawns (these have binary pattern 10 at the end (see board.h))
      if ((certPiece & 0b00001111) == 0b00000010)
      {
        int pawnIndex = (certPiece >> 4) & 0x0f;
        // Security check to ensure that array is addressed correctly:
        if (pawnIndex < 8) {
          lv_obj_set_pos(wp[pawnIndex], getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
          lv_obj_clear_flag(wp[pawnIndex], LV_OBJ_FLAG_HIDDEN);
        }
      }
      if (certPiece == WK1)
      {
        lv_obj_set_pos(wk, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wk, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BK1)
      {
        lv_obj_set_pos(bk, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bk, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WN1)
      {
        lv_obj_set_pos(wn1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wn1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BN1)
      {
        lv_obj_set_pos(bn1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bn1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WN2)
      {
        lv_obj_set_pos(wn2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wn2, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BN2)
      {
        lv_obj_set_pos(bn2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bn2, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WB1)
      {
        lv_obj_set_pos(wb1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wb1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BB1)
      {
        lv_obj_set_pos(bb1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bb1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WB2)
      {
        lv_obj_set_pos(wb2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wb2, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BB2)
      {
        lv_obj_set_pos(bb2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bb2, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WR1)
      {
        lv_obj_set_pos(wr1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wr1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BR1)
      {
        lv_obj_set_pos(br1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(br1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WR2)
      {
        lv_obj_set_pos(wr2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wr2, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BR2)
      {
        lv_obj_set_pos(br2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(br2, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WQ1)
      {
        lv_obj_set_pos(wq1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wq1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BQ1)
      {
        lv_obj_set_pos(bq1, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bq1, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == WQ2)
      {
        lv_obj_set_pos(wq2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wq2, LV_OBJ_FLAG_HIDDEN);
      }
      if (certPiece == BQ2)
      {
        lv_obj_set_pos(bq2, getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bq2, LV_OBJ_FLAG_HIDDEN);
      }

      // Piece was lifted:

        if ((oldBoard[i] & 0b00001111) == 0b00000011) // Black Pawn 
        {
            int pawnIndex = (oldBoard[i] >> 4) & 0x0f;
            if (pawnIndex < 8) {
                lv_obj_add_flag(bp[pawnIndex], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if ((oldBoard[i] & 0b00001111) == 0b00000010) // White Pawn 
        {
            int pawnIndex = (oldBoard[i] >> 4) & 0x0f;
            if (pawnIndex < 8) {
                lv_obj_add_flag(wp[pawnIndex], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (oldBoard[i] == WK1)
        {
          lv_obj_add_flag(wk, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BK1)
        {
          lv_obj_add_flag(bk, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WN1)
        {
          lv_obj_add_flag(wn1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BN1)
        {
          lv_obj_add_flag(bn1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WN2)
        {
          lv_obj_add_flag(wn2, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BN2)
        {
          lv_obj_add_flag(bn2, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WB1)
        {
          lv_obj_add_flag(wb1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BB1)
        {
          lv_obj_add_flag(bb1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WB2)
        {
          lv_obj_add_flag(wb2, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BB2)
        {
          lv_obj_add_flag(bb2, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WR1)
        {
          lv_obj_add_flag(wr1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BR1)
        {
          lv_obj_add_flag(br1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WR2)
        {
          lv_obj_add_flag(wr2, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BR2)
        {
          lv_obj_add_flag(br2, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WQ1)
        {
          lv_obj_add_flag(wq1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BQ1)
        {
          lv_obj_add_flag(bq1, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == WQ2)
        {
          lv_obj_add_flag(wq2, LV_OBJ_FLAG_HIDDEN);
        }
        if (oldBoard[i] == BQ2)
        {
          lv_obj_add_flag(bq2, LV_OBJ_FLAG_HIDDEN);
        }
      
      oldBoard[i] = certPiece;
      updated++;
    }
  }
  if (updated < 2)
  {
    // Put comment before next line to enable debug output via debugLbl
    lv_label_set_text(debugLbl, "");
  }
  else if (updated != 32)
  {
    lv_label_set_text_fmt(debugLbl, "Updated: %i", updated);
  }
}

static void slider_event_cb(lv_event_t *e)
{
  lv_obj_t *slider = (lv_obj_t*)lv_event_get_target(e);
  brightness = lv_slider_get_value(slider);
  ledcWrite(0, (uint8_t)brightness);
  // lv_label_set_text_fmt(debugLbl, "Brightness: %i", brightness);
}

void switchOff(void)
{
  saveBoardSettings();

  tft.writecommand(0x10); // TFT Display Sleep mode on
  // tft.writecommand(0x28);       // TFT Display Off

  ledcDetachPin(TFT_BL);

  // rtc_gpio_isolate(gpio_num_t(ROW_LE));
  // rtc_gpio_isolate(gpio_num_t(LDC_LE));

  // rtc_gpio_hold_en(gpio_num_t(LDC_EN));
  // rtc_gpio_hold_en(gpio_num_t(CB_EN));

  // rtc_gpio_isolate(gpio_num_t(LDC_EN));
  // rtc_gpio_isolate(gpio_num_t(CB_EN));

  // pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW); // Switch off backlight, somehow does not work with ILI9488 Display???

  // rtc_gpio_hold_en(gpio_num_t(GPIO_NUM_16));

  esp_sleep_enable_ext0_wakeup(TOUCH_PANEL_IRQ_PIN, LOW);

  displayLEDstartUpSequence();
  // Switch all LEDs off before going into sleep mode:
  for(int i = 0; i<8; i++)
  {
    mephisto.writeRow(i, 0);
  }
    
  BLEDevice::deinit(false);
  esp_light_sleep_start();
  // esp_sleep_enable_timer_wakeup(10);
  // esp_deep_sleep_start();

// After Wakeup:

  lv_screen_load(screenMain);
  loadBoardSettings();
  initSerialPortCommunication();
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, brightness);
  tft.writecommand(0x11);       // TFT Sleep Off

  // ESP.restart();

}

void updatePromotionButton()
{
  switch (chessBoard.promotionPieceW)
  {
  case 'Q':
    lv_image_set_src(promotionBtnImageW, &WQ40);
    break;
  case 'N':
    lv_image_set_src(promotionBtnImageW, &WN40);
    break;
  case 'R':
    lv_image_set_src(promotionBtnImageW, &WR40);
    break;
  case 'B':
    lv_image_set_src(promotionBtnImageW, &WB40);
    break;
  }
  switch (chessBoard.promotionPieceB)
  {
  case 'Q':
    lv_image_set_src(promotionBtnImageB, &BQ40);
    break;
  case 'N':
    lv_image_set_src(promotionBtnImageB, &BN40);
    break;
  case 'R':
    lv_image_set_src(promotionBtnImageB, &BR40);
    break;
  case 'B':
    lv_image_set_src(promotionBtnImageB, &BB40);
    break;
  }
}

void updateUI_language()
{
  if(language == ES) {
    lv_i18n_set_locale("ES");
    // lv_label_set_text(langLbl, "ES");
  } else if(language == DE) {
    lv_i18n_set_locale("DE");
    // lv_label_set_text(langLbl, "ES");
  } else {
    lv_i18n_set_locale("EN");
    // lv_label_set_text(langLbl, "EN");
  }

  lv_label_set_text(liftedPiecesStringLbl, lv_i18n_get_text("UI_READY"));

  lv_label_set_text(settingsLbl, lv_i18n_get_text("UI_SETTINGS"));
  lv_label_set_text(ui_settings_obj, lv_i18n_get_text("UI_SETTINGS"));
  lv_label_set_text(emulationLbl, lv_i18n_get_text("UI_EMULATION"));
  lv_checkbox_set_text(certaboCalibCB, lv_i18n_get_text("UI_W_QUEENS"));
  lv_checkbox_set_text(flippedCB, lv_i18n_get_text("UI_FLIP_BOARD"));

  lv_label_set_text(commLbl, lv_i18n_get_text("UI_CONNECTION"));
  lv_label_set_text(languageLbl, lv_i18n_get_text("UI_LANGUAGE"));
  lv_label_set_text(offBtnLbl, lv_i18n_get_text("UI_SWITCH_OFF"));
  lv_label_set_text(newGameLbl, lv_i18n_get_text("UI_NEW_GAME"));
  lv_label_set_text(brightLbl, lv_i18n_get_text("UI_BRIGHTNESS"));
  lv_label_set_text(backLbl, lv_i18n_get_text("UI_BACK"));

}

static void event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = (lv_obj_t*)lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED)
  {
    // lv_label_set_text_fmt(debugLbl, "event-clicked");

    int clickedBoard = 0;
    for (int i = 0; i < 64; i++)
    {
      if (obj == square[i])
      {
        clickedBoard++;
      }
    }
    if (clickedBoard > 0)
    {
      displayAboutBox();
    }
    if (obj == calibrateBtn)
    {
      startTouchCalibration();
    }

    if (obj == offBtn)
    {
      switchOff();
    }
    if (obj == settingsBtn)
    {
      lv_screen_load(settingsScreen);
    }
    /*
    if (obj == emulatorsBtn)
    {
      create_Mephisto_MM_Screen();
      lv_scr_load(mm_screen);
      startMephistoEmulation();
    }
    */
    if (obj == restartBtn)
    {
      chessBoard.startPosition(lv_obj_get_state(certaboCalibCB) & LV_STATE_CHECKED);
      resetOldBoard();
      updatePiecesOnBoard();
      chessBoard.promotionPieceW = chessBoard.promotionPieceB = 'Q';
      updatePromotionButton();

      physicalConformity = (lv_obj_get_state(certaboCalibCB) & LV_STATE_CHECKED); // Physical conformity not checked when using calibration Queens in Certabo Emulation!

      lv_screen_load(screenMain);
    }
    else if (obj == exitSettingsBtn)
    {
      lv_scr_load(screenMain);
      updatePromotionButton();
    }
    // Promotion Button Event:
    if (obj == promotionBtnW)
    {
      if(chessBoard.promotionPieceW == 'Q')
      {
        chessBoard.promotionPieceW = 'N';
      }
      else if(chessBoard.promotionPieceW == 'N')
      {
        chessBoard.promotionPieceW = 'R';
      }
      else if(chessBoard.promotionPieceW == 'R')
      {
        chessBoard.promotionPieceW = 'B';
      }
      else if(chessBoard.promotionPieceW == 'B')
      {
        chessBoard.promotionPieceW = 'Q';
      }
      updatePromotionButton();
    }
    if (obj == promotionBtnB)
    {
      if (chessBoard.promotionPieceB == 'Q')
      {
        chessBoard.promotionPieceB = 'N';
      }
      else if (chessBoard.promotionPieceB == 'N')
      {
        chessBoard.promotionPieceB = 'R';
      }
      else if (chessBoard.promotionPieceB == 'R')
      {
        chessBoard.promotionPieceB = 'B';
      }
      else if (chessBoard.promotionPieceB == 'B')
      {
        chessBoard.promotionPieceB = 'Q';
      }
      updatePromotionButton();
    }
  }
  else if (code == LV_EVENT_VALUE_CHANGED)
  {
    if (obj == connectionDd)
    {
      connectionType tempConnection = (connectionType)lv_dropdown_get_selected(connectionDd);

      if (tempConnection == BLE)
      {
        if((lv_obj_get_state(certaboCB) & LV_STATE_CHECKED) != 1)
        {
          connection = tempConnection;
        }
        else
        {
          lv_dropdown_set_selected(connectionDd, (uint16_t)(connection));
        }
      }
      else
      {
        connection = tempConnection;
      }
    }
    if (obj == langDd)
    {
      char buf[3];
      lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
      debugPrintln(buf);
      if (strcmp("DE", buf) == 0)
      {
        language = DE;
        debugPrintln("Selected Language DE");
        updateUI_language();
      }
      else if (strcmp("ES", buf) == 0)
      {
        language = ES;
        debugPrintln("Selected Language ES");
        updateUI_language();
      }
      else
      {
        language = EN;
        debugPrintln("Selected Language EN");
        updateUI_language();
      }
    }

    if (obj == certaboCB)
    {
      if ((lv_obj_get_state(certaboCB) & LV_STATE_CHECKED) == 1)
      {
        chessBoard.emulation = 0;
      }
    }
#ifdef PEGASUS
    if (obj == pegasusCB)
    {
      if ((lv_obj_get_state(pegasusCB) & LV_STATE_CHECKED) == 1)
      {
        chessBoard.emulation = 2;
      }
    }
#endif
    if (obj == chesslinkCB)
    {
      if ((lv_obj_get_state(chesslinkCB) & LV_STATE_CHECKED) == 1)
      {
        chessBoard.emulation = 1;
      }
    }
    if (obj == flippedCB)
    {
      chessBoard.flipped = lv_obj_get_state(flippedCB) & LV_STATE_CHECKED;
    }
    updateSettingsScreen();
  }
}

void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(display);
}

void my_input_read(lv_indev_t* drv, lv_indev_data_t* data)
{
   uint16_t touchX, touchY;

   bool touched = tft.getTouch( &touchX, &touchY, 600 );

   if( !touched )
   {
      data->state = LV_INDEV_STATE_REL;
   }
   else
   {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touchX;
      data->point.y = touchY;

      // debugPrint( "Data x " );
      // debugPrintln( touchX );

      // debugPrint( "Data y " );
      // debugPrintln( touchY );
   }
}

void initLVGL()
{
  tft.begin();

  tft.setRotation(1);

// Test for brightness control: did not work...
  // tft.writecommand(0x53);
  // tft.writedata(0xFF);
  // tft.writecommand(0x51);
  // tft.writedata(20);


  lv_init();

  /*Initialize the display*/
  
  disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  dispBuf = heap_caps_malloc(DISP_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, dispBuf, NULL, DISP_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

  /*Initialize the input device driver*/

  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_input_read);
}

void createSettingsScreen()
{
  // Settings dialog:

  settingsScreen = lv_obj_create(NULL);

  /*Create a container with ROW flex direction*/
  lv_obj_t *cont_header = lv_obj_create(settingsScreen);
  lv_obj_set_size(cont_header, 480, 45);
  lv_obj_set_style_radius(cont_header, 0, 0);
  lv_obj_align(cont_header, LV_ALIGN_TOP_MID, 0, 0);
  // lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
  lv_obj_clear_flag(cont_header, LV_OBJ_FLAG_SCROLLABLE);


  /*Create a container with COLUMN flex direction*/
  lv_obj_t *content = lv_obj_create(settingsScreen);
  lv_obj_set_size(content, 480, 275);
  lv_obj_set_style_radius(content, 0, 0);
  lv_obj_align_to(content, cont_header, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN_WRAP);
  lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_SPACE_AROUND);
  // lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *label;

  ui_settings_obj = lv_label_create(cont_header);
  lv_obj_center(ui_settings_obj);
  lv_obj_add_style(ui_settings_obj, &fExtraLargeStyle, 0);

  emulationLbl = lv_label_create(content);
  lv_obj_add_style(emulationLbl, &fLargeStyle, 0);

  chesslinkCB = lv_checkbox_create(content);
  lv_checkbox_set_text(chesslinkCB, "Millennium/Chesslink");
  lv_obj_add_event_cb(chesslinkCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(chesslinkCB, &fMediumStyle, 0);

#ifdef PEGASUS
  pegasusCB = lv_checkbox_create(content);
  lv_checkbox_set_text(pegasusCB, "DGT Pegasus");
  lv_obj_add_event_cb(pegasusCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(pegasusCB, &fMediumStyle, 0);
#endif

  /*Create a container for Certabo Settings */
  object = lv_obj_create(content);
  lv_obj_set_size(object, 280, 30);   // Size of the left column in the Settings Dialog
  lv_obj_set_style_radius(object, 0, 0);
  lv_obj_set_style_border_width(object, 0, 0);
  lv_obj_set_flex_flow(object, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_left(object, 0, 0);
  lv_obj_set_style_pad_top(object, 0, 0);
  lv_obj_align_to(object, content, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

  certaboCB = lv_checkbox_create(object);
  lv_checkbox_set_text(certaboCB, "Certabo");
  lv_obj_add_event_cb(certaboCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(certaboCB, &fMediumStyle, 0);

  certaboCalibCB = lv_checkbox_create(object);
  lv_obj_add_event_cb(certaboCalibCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(certaboCalibCB, &fMediumStyle, 0);

  flippedCB = lv_checkbox_create(content);
  lv_obj_add_event_cb(flippedCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_grid_cell(flippedCB, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_align(flippedCB, LV_ALIGN_LEFT_MID );

#ifndef PEGASUS
  // lv_obj_set_style_pad_top(flippedCB, 16, 0);
  lv_obj_set_style_pad_top(flippedCB, 1, 0);
#endif
  lv_obj_add_style(flippedCB, &fMediumStyle, 0);

  object = lv_obj_create(content);  // Panel for Communication Label and Dropdown
  lv_obj_set_size(object, 275, 42);
  lv_obj_set_style_pad_all(object, 0, 0);
  lv_obj_set_style_radius(object, 0, 0);
  lv_obj_set_style_border_width(object, 0, 0);
  // lv_obj_set_style_pad_left(object, 0, 0);
  // lv_obj_set_style_pad_top(object, 0, 0);
  lv_obj_align_to(object, content, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

  // ui_connection_obj = lv_label_create(content);
  commLbl = lv_label_create(object);
  lv_obj_set_style_pad_all(commLbl, 0, 0);
  lv_obj_add_style(commLbl, &fLargeStyle, 0);
  lv_obj_set_align(commLbl, LV_ALIGN_LEFT_MID );

  connectionDd = lv_dropdown_create(object);
  lv_dropdown_set_options_static(connectionDd, "USB\nBT\nBLE");
  lv_obj_set_align(connectionDd, LV_ALIGN_RIGHT_MID );
  lv_obj_set_style_pad_all(connectionDd, 4, 0);
  lv_obj_set_size(connectionDd, 80, LV_SIZE_CONTENT);  
  lv_obj_add_event_cb(connectionDd, event_handler, LV_EVENT_ALL, NULL);
  // lv_obj_add_style(connectionDd, &fMediumStyle, 0);

  object = lv_obj_create(content);  // Panel for Language Label and Dropdown
  lv_obj_set_size(object, 275, 42);
  lv_obj_set_style_pad_all(object, 0, 0);
  lv_obj_set_style_radius(object, 0, 0);
  lv_obj_set_style_border_width(object, 0, 0);
  // lv_obj_set_style_pad_left(object, 0, 0);
  // lv_obj_set_style_pad_top(object, 0, 0);
  lv_obj_align_to(object, content, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

  languageLbl = lv_label_create(object);
  lv_obj_set_style_pad_all(languageLbl, 0, 0);
  lv_obj_add_style(languageLbl, &fLargeStyle, 0);
  lv_obj_set_align(languageLbl, LV_ALIGN_LEFT_MID );

  langDd = lv_dropdown_create(object);
  lv_dropdown_set_options_static(langDd, "EN\nES\nDE");
  lv_obj_set_align(langDd, LV_ALIGN_RIGHT_MID );
  if(language==DE) lv_dropdown_set_selected(langDd, 2);
  else if(language==ES) lv_dropdown_set_selected(langDd, 1);

  lv_obj_add_event_cb(langDd, event_handler, LV_EVENT_ALL, NULL);
  // lv_obj_set_style_pad_left(langDd, 2, LV_PART_MAIN| LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(langDd, 4, 0);
  lv_obj_set_size(langDd, 68, LV_SIZE_CONTENT);
  // lv_obj_add_style(langDd, &fMediumStyle, 0);

  updateSettingsScreen();

  offBtn = lv_btn_create(content);
  offBtnLbl = lv_label_create(offBtn);
  lv_obj_add_event_cb(offBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(offBtn, 160, 35);
  lv_obj_center(offBtnLbl);
  lv_obj_add_style(offBtnLbl, &fMediumStyle, 0);

  restartBtn = lv_btn_create(content);
  newGameLbl = lv_label_create(restartBtn);
  lv_obj_add_event_cb(restartBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(restartBtn, 160, 35);
  lv_obj_center(newGameLbl);
  lv_obj_add_style(newGameLbl, &fMediumStyle, 0);

  /*Create a container for Brightness Slider */
  object = lv_obj_create(content);
  lv_obj_set_size(object, 160, 55);
  lv_obj_set_style_radius(object, 0, 0);
  lv_obj_set_style_border_width(object, 0, 0);
  lv_obj_set_flex_flow(object, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_left(object, 10, 0);
  lv_obj_set_style_pad_right(object, 10, 0);
  lv_obj_set_style_pad_top(object, 0, 0);
  lv_obj_set_style_pad_bottom(object, 15, 0);
  lv_obj_align_to(object, content, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

  brightLbl = lv_label_create(object);
  lv_obj_add_style(brightLbl, &fMediumStyle, 0);
  // lv_obj_set_size(brightLbl, 160, 10);
  // lv_obj_center(brightLbl);

  lv_obj_t *brightnessSlider = lv_slider_create(object);
  lv_obj_set_size(brightnessSlider, 140, 10);
  // lv_obj_set_style_pad_left(brightnessSlider, 5, 0);

#ifdef PicoResTouchLCD_35
  lv_slider_set_range(brightnessSlider, 155, 255);
#else
  lv_slider_set_range(brightnessSlider, 5, 255);
#endif
  lv_slider_set_value(brightnessSlider, brightness, LV_ANIM_ON);
  lv_obj_add_event_cb(brightnessSlider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

#ifdef BOARD_TEST
  testCB = lv_checkbox_create(content);
  lv_checkbox_set_text(testCB, "BOARD TEST");
  lv_obj_add_event_cb(testCB, event_handler, LV_EVENT_ALL, NULL);
#endif

  // calibrateBtn = lv_btn_create(content);
  // label = lv_label_create(calibrateBtn);
  // lv_label_set_text(label, "Calibrate Touch");
  // lv_obj_add_event_cb(calibrateBtn, event_handler, LV_EVENT_ALL, NULL);
  // lv_obj_set_size(calibrateBtn, 180, 35);
  // lv_obj_center(label);
  // lv_obj_add_style(label, &fMediumStyle, 0);

  exitSettingsBtn = lv_btn_create(content);
  backLbl = lv_label_create(exitSettingsBtn);
  lv_obj_add_event_cb(exitSettingsBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(exitSettingsBtn, 160, 35);
  lv_obj_center(backLbl);
  lv_obj_add_style(backLbl, &fMediumStyle, 0);

}

void createUI()
{
  screenMain = lv_obj_create(NULL);

  lv_style_init(&fMediumStyle); // Font Size 20
  // lv_style_set_text_font(&fMediumStyle, &lv_font_montserrat_20);
  lv_style_set_text_font(&fMediumStyle, &montserrat_umlaute20);
    
  lv_style_init(&fLargeStyle); // Font Size 22
  // lv_style_set_text_font(&fLargeStyle, &lv_font_montserrat_22); 
  lv_style_set_text_font(&fLargeStyle, &montserrat_umlaute22);  
    
  lv_style_init(&fExtraLargeStyle); // Font Size 28
  lv_style_set_text_font(&fExtraLargeStyle, &lv_font_montserrat_28);
    
  object = lv_label_create(screenMain);
  lv_label_set_text(object, "Chesstimation");
  lv_obj_set_style_text_align(object, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_size(object, 151, 55);
  lv_obj_set_pos(object, 327, 35);
  // lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
  lv_obj_add_style(object, &fMediumStyle, 0);  
  
  #ifdef LOLIN_D32
  batteryLbl = lv_label_create(screenMain);
  lv_obj_set_style_text_align(batteryLbl, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_size(batteryLbl, 33, 20);
  lv_obj_set_pos(batteryLbl, 446, 0);
  lv_obj_add_style(batteryLbl, &fLargeStyle, 0);  
  #endif
  
  liftedPiecesStringLbl = lv_label_create(screenMain);
  // lv_label_set_text(liftedPiecesStringLbl, lv_i18n_get_text("UI_READY"));
  lv_obj_set_style_text_align(liftedPiecesStringLbl, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_size(liftedPiecesStringLbl, 135, 160);
  lv_obj_set_pos(liftedPiecesStringLbl, 330, 100);
  lv_obj_add_style(liftedPiecesStringLbl, &fMediumStyle, 0);    // was f28Style    
  
  debugLbl = lv_label_create(screenMain);
  lv_label_set_text(debugLbl, "");
  lv_obj_set_style_text_align(debugLbl, LV_TEXT_ALIGN_RIGHT, 0);
  lv_label_set_long_mode(debugLbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_size(debugLbl, 146, 25);
  lv_obj_set_pos(debugLbl, 327, 236);
  lv_obj_add_style(debugLbl, &fMediumStyle, 0);  

#ifdef LEGACY_EMULATION  
  emulatorsBtn = lv_btn_create(screenMain);
  lv_obj_add_event_cb(emulatorsBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(emulatorsBtn, 120, 40);
  lv_obj_set_pos(emulatorsBtn, 340, 200);

  lv_obj_t * label2 = lv_label_create(emulatorsBtn);
  lv_label_set_text(label2, "Emulator");
  lv_obj_set_align(label2, LV_ALIGN_CENTER);
  lv_obj_add_style(emulatorsBtn, &fMediumStyle, 0);
#endif

  promotionBtnW = lv_btn_create(screenMain);
  // lv_obj_add_flag(promotionBtnW, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(promotionBtnW, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(promotionBtnW, 56, 56);
  promotionBtnImageW = lv_image_create(promotionBtnW);
  lv_image_set_src(promotionBtnImageW, &WQ40);   
  lv_obj_center(promotionBtnImageW); 
  lv_obj_set_pos(promotionBtnW, 338, 174);

  promotionBtnB = lv_btn_create(screenMain);
  // lv_obj_add_flag(promotionBtnB, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(promotionBtnB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(promotionBtnB, 56, 56);
  promotionBtnImageB = lv_image_create(promotionBtnB);
  lv_image_set_src(promotionBtnImageB, &BQ40);   
  lv_obj_center(promotionBtnImageB); 
  lv_obj_set_pos(promotionBtnB, 406, 174);

  settingsBtn = lv_btn_create(screenMain);
  lv_obj_add_event_cb(settingsBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(settingsBtn, 150, 40);
  lv_obj_set_pos(settingsBtn, 325, 264);

  settingsLbl = lv_label_create(settingsBtn);
  lv_obj_set_align(settingsLbl, LV_ALIGN_CENTER);
  lv_obj_add_style(settingsBtn, &fMediumStyle, 0);

  // // labelA1 = lv_label_create(screenMain);
  // // lv_label_set_text(labelA1, LV_SYMBOL_BATTERY_FULL);
  // // lv_obj_set_pos(labelA1, 200, 200);
  // // lv_obj_set_size(labelA1, 40, 40);

  lv_style_init(&dark_square);
  lv_style_set_bg_color(&dark_square, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
  lv_style_set_radius(&dark_square, 0);
  lv_style_set_bg_opa(&dark_square, LV_OPA_COVER);
  lv_style_set_border_width(&dark_square, 0);

  lv_style_init(&light_square);
  lv_style_set_bg_color(&light_square, lv_color_white());
  lv_style_set_radius(&light_square, 0);
  lv_style_set_bg_opa(&light_square, LV_OPA_COVER);
  lv_style_set_border_width(&light_square, 0);

  connectionLbl = lv_label_create(screenMain);
  lv_obj_set_style_text_align(connectionLbl, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_size(connectionLbl, 29, 25);
  lv_obj_set_pos(connectionLbl, 442, 68);
  lv_obj_add_style(connectionLbl, &fLargeStyle, 0);  

  // Create Chessboard

  int i;

  for(i = 0; i<64; i++)
  {
    square[i] = lv_obj_create(screenMain);
    lv_obj_set_size(square[i], SQUARE_SIZE, SQUARE_SIZE);
    lv_obj_set_pos(square[i], getColFromBoardIndex(i) * SQUARE_SIZE, (getRowFromBoardIndex(i)) * SQUARE_SIZE);
    lv_obj_add_event_cb(square[i], event_handler, LV_EVENT_ALL, NULL);
        
    if ((0xAA55AA55 >> i) & 1)  // taken from chess programming wiki to find out which one is a light and dark square!
    { 
        lv_obj_add_style(square[i], &light_square, 0);
        
    }
    else
    {
        lv_obj_add_style(square[i], &dark_square, 0);
    }
  }


 
  for (int i = 0; i < 8; i++)
  {
    wp[i] = lv_img_create(screenMain);
    lv_img_set_src(wp[i], &WP40);
    lv_obj_add_flag(wp[i], LV_OBJ_FLAG_HIDDEN);

    bp[i] = lv_img_create(screenMain);
    lv_img_set_src(bp[i], &BP40);
    lv_obj_add_flag(bp[i], LV_OBJ_FLAG_HIDDEN);
  }
  wk = lv_img_create(screenMain);
  lv_img_set_src(wk, &WK40);
  lv_obj_add_flag(wk, LV_OBJ_FLAG_HIDDEN);

  bk = lv_img_create(screenMain);
  lv_img_set_src(bk, &BK40);
  lv_obj_add_flag(bk, LV_OBJ_FLAG_HIDDEN);

  wn1 = lv_img_create(screenMain);
  lv_img_set_src(wn1, &WN40);
  lv_obj_add_flag(wn1, LV_OBJ_FLAG_HIDDEN);

  wn2 = lv_img_create(screenMain);
  lv_img_set_src(wn2, &WN40);
  lv_obj_add_flag(wn2, LV_OBJ_FLAG_HIDDEN);

  bn1 = lv_img_create(screenMain);
  lv_img_set_src(bn1, &BN40);
  lv_obj_add_flag(bn1, LV_OBJ_FLAG_HIDDEN);
  
  bn2 = lv_img_create(screenMain);
  lv_img_set_src(bn2, &BN40);
  lv_obj_add_flag(bn2, LV_OBJ_FLAG_HIDDEN);
  
  wb1 = lv_img_create(screenMain);
  lv_img_set_src(wb1, &WB40);
  lv_obj_add_flag(wb1, LV_OBJ_FLAG_HIDDEN);

  wb2 = lv_img_create(screenMain);
  lv_img_set_src(wb2, &WB40);
  lv_obj_add_flag(wb2, LV_OBJ_FLAG_HIDDEN);

  bb1 = lv_img_create(screenMain);
  lv_img_set_src(bb1, &BB40);
  lv_obj_add_flag(bb1, LV_OBJ_FLAG_HIDDEN);
  
  bb2 = lv_img_create(screenMain);
  lv_img_set_src(bb2, &BB40);
  lv_obj_add_flag(bb2, LV_OBJ_FLAG_HIDDEN);
  
  wr1 = lv_img_create(screenMain);
  lv_img_set_src(wr1, &WR40);
  lv_obj_add_flag(wr1, LV_OBJ_FLAG_HIDDEN);

  wr2 = lv_img_create(screenMain);
  lv_img_set_src(wr2, &WR40);
  lv_obj_add_flag(wr2, LV_OBJ_FLAG_HIDDEN);

  br1 = lv_img_create(screenMain);
  lv_img_set_src(br1, &BR40);
  lv_obj_add_flag(br1, LV_OBJ_FLAG_HIDDEN);
  
  br2 = lv_img_create(screenMain);
  lv_img_set_src(br2, &BR40);
  lv_obj_add_flag(br2, LV_OBJ_FLAG_HIDDEN);
  
  wq1 = lv_img_create(screenMain);
  lv_img_set_src(wq1, &WQ40);
  lv_obj_add_flag(wq1, LV_OBJ_FLAG_HIDDEN);

  wq2 = lv_img_create(screenMain);
  lv_img_set_src(wq2, &WQ40);
  lv_obj_add_flag(wq2, LV_OBJ_FLAG_HIDDEN);

  bq1 = lv_img_create(screenMain);
  lv_img_set_src(bq1, &BQ40);
  lv_obj_add_flag(bq1, LV_OBJ_FLAG_HIDDEN);
  
  bq2 = lv_img_create(screenMain);
  lv_img_set_src(bq2, &BQ40);
  lv_obj_add_flag(bq2, LV_OBJ_FLAG_HIDDEN);
  
  resetOldBoard();
  updatePiecesOnBoard();

}

void setup()
{
/*
  pinMode(POWER_SAVE_PIN, OUTPUT);
  gpio_hold_dis(POWER_SAVE_PIN);
  digitalWrite(POWER_SAVE_PIN, HIGH);
*/

  pinMode(TFT_BL, OUTPUT);
  // digitalWrite(TFT_BL, HIGH);    
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);

  gpio_hold_dis((gpio_num_t)TFT_BL); 

  // digitalWrite(GPIO_NUM_16, LOW);    
/*
  gpio_hold_dis(GPIO_NUM_17);
  gpio_hold_dis(GPIO_NUM_18);
  gpio_hold_dis(GPIO_NUM_19);
  gpio_hold_dis(GPIO_NUM_23);
  gpio_hold_dis(GPIO_NUM_5);
  gpio_hold_dis(GPIO_NUM_21);
  gpio_hold_dis(GPIO_NUM_0);
  gpio_hold_dis(GPIO_NUM_4);
  gpio_hold_dis(GPIO_NUM_2);
*/

  mephisto.initPorts();
  // displayLEDstartUpSequence();

  chessBoard.startPosition(0);
  connection = BLE;
  // ledcWrite(0, 255);
  loadBoardSettings();
  ledcWrite(0, brightness);
  // if(chessBoard.emulation == 0 && connection == BLE)
  //   connection = USB;

  for (int i = 0; i < 8; i++)
  {
    chessBoard.lastRawRow[i] = mephisto.readRow(i);
  }

  chessBoard.generateSerialBoardMessage();
  // chessBoard.copyPieceSetupToRaw(chessBoard.lastRawRow);

  initLVGL();

  lv_i18n_init(lv_i18n_language_pack);

  createUI();

  createSettingsScreen();

  updateUI_language();
  
  initSerialPortCommunication();
  lv_screen_load(screenMain);
 
  chessBoard.updateLiftedPiecesString();
 
  // chessBoard.printDebugMessage();
}

void loop()
{
  byte rows = 0;
  byte setBack = 0;
  byte lifted = 0;
  static byte writeToIndex = 0;

#ifdef LOLIN_D32
  static long long oldMillis=-10000;
  static long long oldMessageMillis=-10000;
  unsigned long actMillis;
  actMillis = millis();
  static unsigned long lastTickMillis = 0;
  
    // LVGL Tick Interface, this is required since LVGL 9.x
  unsigned int tickPeriod = actMillis - lastTickMillis;
  lv_tick_inc(tickPeriod);
  lastTickMillis = actMillis;

  if(actMillis-oldMillis>10000) {
    float voltage = analogRead(35)/587.5;
    // char batMessage[80]="";
    // sprintf(batMessage, "%.2fV - %1.0f min", voltage, actMillis/60000.0);
    // lv_label_set_text(debugLbl, batMessage);
    // debugPrintln(batMessage);
    if(voltage > 3.79)
    {
      lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_FULL);
    }
    else if(voltage > 3.56)
    {
      lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_3);
    }
    else if(voltage > 3.46)
    {
      lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_2);
    }
    else if(voltage > 3.35)
    {
      lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_1);
    }
    else
    {
      lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_EMPTY);
    }
    oldMillis=actMillis;
  }
#endif  

  lv_task_handler();

#ifdef BOARD_TEST
  if ((lv_obj_get_state(testCB) & LV_STATE_CHECKED) == 1)
  {
    static int i = 0;
    {
      mephisto.writeRow(i, 0xff);
      delay(500);
      lv_task_handler();

      mephisto.writeRow(i, 0x00);
      delay(100);
      lv_task_handler();
      int value = mephisto.readRow(i);
      if(1)//value != 0)
      {
        mephisto.writeRow(i, value);
        delay(500);
      }
    }
    i++;
    if (i > 7)
      i = 0;
    return;
  }
#endif

  if (chessBoard.emulation == 0) // Certabo!
  {
    if (connection == USB)
    {
      while (Serial.available())
      {
        led_buffer[writeToIndex] = Serial.read();

        writeToIndex++;
        if (writeToIndex > 7)
        {
          writeToIndex = 0;
          Serial.println("L");
          Serial.flush();
        }
      };
    }
    if (connection == BT)
    {
      while (SerialBT.available())
      {
        led_buffer[writeToIndex] = SerialBT.read();

        writeToIndex++;
        if (writeToIndex > 7)
        {
          writeToIndex = 0;
          SerialBT.println("L");
          SerialBT.flush();
        }
      };
    }
  }

  if ((chessBoard.emulation == 1) && connection != BLE) // Millennium Chesslink
  {
    char readChar;
    if (connection == USB)
    {
      if (Serial.available() > 0)
      {
        for (int i = 0; i < Serial.available(); i++)
        {
          Serial.readBytes(&readChar, 1);

          assembleIncomingChesslinkMessage(readChar);
        }
        sendChesslinkAnswer(incomingMessage);
      }
    }
    if (connection == BT)
    {
      if (SerialBT.available() > 0)
      {
        for (int i = 0; i < SerialBT.available(); i++)
        {
          SerialBT.readBytes(&readChar, 1);

          assembleIncomingChesslinkMessage(readChar);
        }
        sendChesslinkAnswer(incomingMessage);
      }
    }
  }

  if(chessBoard.emulation==1) // Millennium Chesslink
  {
    // Translate MephistoLEDs into led_buffer:

    for (byte row = 0; row < 8; row++)
    {
      byte rowValue = 0;
      for (byte col = 0; col < 8; col++)
      {
        byte led = 0;
        if (mephistoLED[7 - col][row] > 0)
          led = 1;
        rowValue <<= 1;
        rowValue += led;
      }
      led_buffer[row] = rowValue;
    }
  }

  // Read row status from board:
  for(int i=0; i<8; i++) {
    readRawRow[i] = mephisto.readRow(i);
  }

  // Check physical conformity (this is only done when a new game is started):
  if (!physicalConformity)
  {
    byte piecesPhys, piecesMem;
    char debugMessage[80] = "";
    rows = 0;

    for (int row = 0; row < 8; row++)
    {
      piecesMem = 0;
      for (int i = 0; i < 8; i++)
      {
        piecesMem <<= 1;
        if (chessBoard.piece[toBoardIndex(row, i)] != 0)
        {
          piecesMem++;
        }
      }

      piecesPhys = readRawRow[row];

      sprintf(debugMessage, "piecesPhysical: %x", piecesPhys);
      debugPrintln(debugMessage);
      sprintf(debugMessage, "piecesMemory: %x", piecesMem);
      debugPrintln(debugMessage);

      if (piecesMem != piecesPhys)
      {
        physicalConformity = 0;
        rows++;
        lv_label_set_text_fmt(debugLbl, lv_i18n_get_text("UI_CHECK_PIECES"));
        led_buffer[7-row] = piecesMem>piecesPhys?piecesMem-piecesPhys:piecesPhys-piecesMem;

        // mephisto.writeRow(row, piecesMem>piecesPhys?piecesMem-piecesPhys:piecesPhys-piecesMem);
        // delay(LED_TIME);
      }
    }
    if (rows == 0)
    {
      physicalConformity = 1;
      lv_label_set_text_fmt(debugLbl, "");
      led_buffer[0] = led_buffer[1] = led_buffer[2] = led_buffer[3] = led_buffer[4] = led_buffer[5] = led_buffer[6] = led_buffer[7] = 0; 
    }
    for (int i = 0; i < 8; i++)
    {
      mephisto.writeRow(7 - i, led_buffer[i]);
      if (led_buffer[i] != 0 && rows != 0)
      {
        delay(LED_TIME / rows);
        lv_task_handler();
      }
    }
    return;
  }

  // First step: Display LEDs with the content from the buffer.
  // 1st calculate buffer time:
  rows = 0;
  for (int i = 0; i < 8; i++)
  {
    if (led_buffer[i] != 0)
      rows++;
  }

  for (int i = 0; i < 8; i++)
  {
    // bit wise reverse taken from: http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
    byte ledValue = chessBoard.flipped ? (led_buffer[7 - i] * 0x0202020202ULL & 0x010884422010ULL) % 1023 : led_buffer[i];
    mephisto.writeRow(7 - i, ledValue);
    if (ledValue != 0)
      delay(LED_TIME / rows);
  }

  setBack = 0;
  lifted = 0;

  // Check every single field for lifted piece:
  for(int i=0; i<8; i++) { // for each row
    for(int col = 0; col<8; col++) { // for each column
    int diff = (bitRead(chessBoard.lastRawRow[i], col) - bitRead(readRawRow[i], col));
      if(diff>0) {
        chessBoard.liftPieceFrom(toBoardIndex(i, col));
        lifted++;
        lv_label_set_text_fmt(debugLbl, "Lifted row: %i", getRowFromBoardIndex((chessBoard.piecesLifted[chessBoard.liftedIdx - 1] >> 8)));
      }
    }
  }

  // Check every single field for set back piece:
  // for each row
  for (int i = 0; i < 8; i++)
  { 
    // for each column
    for (int col = 0; col < 8; col++)
    { 
      int diff = (bitRead(chessBoard.lastRawRow[i], col) - bitRead(readRawRow[i], col));
      if (diff < 0)
      {
        chessBoard.setPieceBackTo(toBoardIndex(i, col));
        setBack++;
      }
    }
  }

  // A figure was lifted or set back:
  if (lifted + setBack > 0)
  {
    chessBoard.updateLiftedPiecesString();
    if (chessBoard.liftedIdx == 0)
    {
      lv_label_set_text(debugLbl, "");
      lv_label_set_text(liftedPiecesStringLbl, "");
    }
    else
    {
      lv_label_set_text_fmt(debugLbl, "Lifted: %i", chessBoard.liftedIdx);
      lv_label_set_text(liftedPiecesStringLbl, chessBoard.liftedPiecesDisplayString);
    }
    updatePiecesOnBoard();
  }

  for (int row = 0; row < 8; row++)
  {
    chessBoard.lastRawRow[row] = readRawRow[row];
  }

  // Check if promotion button neeed to be enabled:
  int pawnsW = 0;
  int pawnsB = 0;
  for (int col = 0; col < 8; col++)
  {
    if (chessBoard.flipped)
    {
      if (chessBoard.isBlackPawn(chessBoard.piece[toBoardIndex(6, col)]))
        pawnsB++;
      if (chessBoard.isWhitePawn(chessBoard.piece[toBoardIndex(1, col)]))
        pawnsW++;
    }
    else
    {
      if (chessBoard.isWhitePawn(chessBoard.piece[toBoardIndex(6, col)]))
        pawnsW++;
      if (chessBoard.isBlackPawn(chessBoard.piece[toBoardIndex(1, col)]))
        pawnsB++;
    }
  }
  // TODO: Remove debugging:
  // if(pawnsW!=0 || pawnsB!=0)
  //   lv_label_set_text_fmt(debugLbl, "W: %i, B: %i", pawnsW, pawnsB);
  
  if (pawnsW && lv_obj_has_flag(promotionBtnW, LV_OBJ_FLAG_HIDDEN))
  {
    lv_obj_remove_flag(promotionBtnW, LV_OBJ_FLAG_HIDDEN);
  }
  else if (!pawnsW && !lv_obj_has_flag(promotionBtnW, LV_OBJ_FLAG_HIDDEN))
  {
    lv_obj_add_flag(promotionBtnW, LV_OBJ_FLAG_HIDDEN);
  }
  if (pawnsB && lv_obj_has_flag(promotionBtnB, LV_OBJ_FLAG_HIDDEN))
  {
    lv_obj_remove_flag(promotionBtnB, LV_OBJ_FLAG_HIDDEN);
  }
  else if (!pawnsB && !lv_obj_has_flag(promotionBtnB, LV_OBJ_FLAG_HIDDEN))
  {
    lv_obj_add_flag(promotionBtnB, LV_OBJ_FLAG_HIDDEN);
  }

  // After the whole board is read and any change is regarded, the Serial Board message is re-generated:
  chessBoard.generateSerialBoardMessage();
  // if(connection == BLE && chessBoard.emulation==1)
  // {
  //   chessBoard.boardMessage[65]=0;
  // }

  if (lifted > 0 || setBack > 0 || chessBoard.emulation == 0 || ((actMillis-oldMessageMillis>eeprom[3]*4) && eeprom[2]==2)) // Certabo boards sends position even when no change happend
  { 
    sendMessageToChessBoard(chessBoard.boardMessage);
    oldMessageMillis = actMillis;
  }
  if (chessBoard.emulation == 0 && rows == 0)
  {
    // delay(100);
  }
  writeToIndex = 0;
}