/*  
    Copyright 2021 Andreas Petersik (andreas.petersik@gmail.com)
    
    This file is part of the Open Mephisto Project.

    Open Mephisto is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Open Mephisto is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Open Mephisto.  If not, see <https://www.gnu.org/licenses/>.
*/

#define VERSION     "Open Mephisto 1.02"
#define ABOUT_TEXT  "\nby Dr. Andreas Petersik\nandreas.petersik@gmail.com\n\nbuilt: Jan 1st, 2022"

#include <Arduino.h>
#include <SPIFFS.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>
#include <BluetoothSerial.h>

#include <BLEDevice.h>
#include <BLE2902.h>

// my includes:

#include <mephisto.h>
#include <board.h>

#include "SPI.h"
#include "TFT_eSPI.h"

#include "../lvgl/src/lvgl.h"

#define LOLIN_D32
#define LED_TIME 300

#define TOUCH_PANEL_IRQ_PIN   GPIO_NUM_34   // The idea is to check if there is a signal change on this pin for waking ESP32 from sleep! Works! Pin is high when no touch, low when touch!

#define BOARD_SETUP_FILE  "/board_setup"
#define SQUARE_SIZE         40

Mephisto mephisto;
Board chessBoard;
BluetoothSerial SerialBT;

enum connectionType {USB, BT, BLE} connection;

byte readRawRow[8];
byte led_buffer[8];
byte mephistoLED[8][8];
byte eeprom[5]={0,20,3,20,15};
byte oldBoard[64];
int brightness = 255;

uint16_t calibrationData[5];

//BLE objects
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLEService *pService;

char incomingMessage[170];
std::string replyString;

TFT_eSPI tft = TFT_eSPI();

#define DISP_BUF_SIZE (480 * 40)
static lv_disp_draw_buf_t disp_buf; //LVGL stuff;
static lv_color_t buf[DISP_BUF_SIZE];

static lv_style_t fMediumStyle;
static lv_style_t fLargeStyle;
static lv_style_t fExtraLargeStyle;
static lv_style_t light_square;
static lv_style_t dark_square;

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

lv_disp_drv_t disp_drv;
lv_indev_drv_t indev_drv;

lv_obj_t *settingsScreen, *settingsBtn, *btn2, *screenMain, *liftedPiecesLbl, *liftedPiecesStringLbl, *debugLbl, *chessBoardCanvas, *chessBoardLbl, *batteryLbl;
lv_obj_t *labelA1, *exitSettingsBtn, *offBtn, *certaboCalibCB, *restartBtn, *certaboCB, *chesslinkCB, *usbCB, *btCB, *bleCB, *flippedCB;
lv_obj_t *square[64], *dummy1Btn, *calibrateBtn, *object, *brightnessSlider;
lv_obj_t *wp[8], *bp[8], *wk, *bk, *wn1, *bn1, *wn2, *bn2, *wb1, *bb1, *wb2, *bb2, *wr1, *br1, *wr2, *br2, *wq1, *bq1, *wq2, *bq2;

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
  object = lv_msgbox_create(screenMain, VERSION, ABOUT_TEXT, NULL, true);

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

  if ((lv_obj_get_state(bleCB) & LV_STATE_CHECKED) == 1 && chessBoard.emulation == 1)
  {
    saveConnection = BLE;
  }
  if ((lv_obj_get_state(btCB) & LV_STATE_CHECKED) == 1)
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
    f.close();
  }
}

void loadBoardSettings(void)
{

  byte tempBoardSetup[64];
  uint16_t tempPiecesLifted[32];
  uint8_t tempInt8[1];

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
  if (chessBoard.emulation == 1)
  {
    debugPrint("Clear Message to be sent: ");
  }
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
    debugPrint("Final Message to be sent: ");
    debugPrintln(codedMessage.c_str());
    pTxCharacteristic->setValue(codedMessage);
    pTxCharacteristic->notify();
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
    lv_obj_add_state(certaboCB, LV_STATE_CHECKED);
    lv_obj_clear_state(chesslinkCB, LV_STATE_CHECKED);  
  }
  if(chessBoard.emulation==1) 
  {
    lv_obj_clear_state(certaboCB, LV_STATE_CHECKED);
    lv_obj_add_state(chesslinkCB, LV_STATE_CHECKED);  
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

// You need to declare handles for connection and disconnection events

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    connection = BLE;
    updateSettingsScreen();
    // initSerialPortCommunication();
    debugPrintln("BLE DEVICE CONNECTED");
    //your stuff for handling new connections
  };

  void onDisconnect(BLEServer *pServer)
  {
    debugPrintln("BLE DEVICE DISCONNECTED");
    updateSettingsScreen();
    // initSerialPortCommunication();
    //your stuff for handling closed connections

    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
  }
};

void sendChesslinkAnswer(char *incomingMessage)
{
  if (strlen(incomingMessage) == 3 && (strcmp(incomingMessage, "V56") == 0))
  {
    debugPrintln("Detected valid Version Request Message V");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard("v0103");
    return;
  }
  if (strlen(incomingMessage) == 5 && incomingMessage[0] == 'R')
  {
    debugPrintln("Detected Read EEPROM Message R:");
    debugPrintln(incomingMessage);
    incomingMessage[0] = 'r';
    char twoDigits[3];
    sprintf(twoDigits, "%02X", eeprom[incomingMessage[2] - '0']);
    debugPrint("EEPROM Pos requested: ");
    debugPrintln(String(incomingMessage[2] - '0').c_str());
    debugPrint("Content to be sent: ");
    debugPrintln(twoDigits);
    incomingMessage[3] = twoDigits[0];
    incomingMessage[4] = twoDigits[1];
    sendMessageToChessBoard(incomingMessage);
    return;
  }
  if (strlen(incomingMessage) == 7 && incomingMessage[0] == 'W')
  {
    debugPrintln("Detected message W:");
    debugPrintln(incomingMessage);
    debugPrintln("Detected Write EEPROM Message W");
    incomingMessage[0] = 'w';

    char twoDigits[3];
    twoDigits[0] = incomingMessage[3];
    twoDigits[1] = incomingMessage[4];
    twoDigits[2] = 0;
    eeprom[incomingMessage[2] - '0'] = strtol(twoDigits, NULL, 16);

    debugPrint("Eeprom Pos requested: ");
    debugPrintln(String(incomingMessage[2] - '0').c_str());
    debugPrint("Value read: ");
    debugPrintln(String(eeprom[incomingMessage[2] - '0']).c_str());

    incomingMessage[5] = 0;
    sendMessageToChessBoard(incomingMessage);
    return;
  }
  if (strlen(incomingMessage) == 3 && incomingMessage[0] == 'S')
  {
    debugPrintln("Detected Status request Message S:");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard(chessBoard.boardMessage);
    return;
  }
  if (strlen(incomingMessage) == 167 && incomingMessage[0] == 'L')
  {
    debugPrintln("Detected set LED Message L:");
    debugPrintln(incomingMessage);

    incomingMessage[165] = 0;
    chessBoard.updateMilleniumLEDs((&incomingMessage[1]));

    // Print DEBUG LED pattern:
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
    updateMephistoLEDs(mephistoLED);
    sendMessageToChessBoard("l");
    return;
  }
  if (incomingMessage[0] == 'I')
  {
    debugPrintln("Detected Message I:");
    debugPrintln(incomingMessage);
    sendMessageToChessBoard("i00");
    return;
  }
  if (strlen(incomingMessage) == 3 && incomingMessage[0] == 'X')
  {
    debugPrintln("Detected Message X:");
    debugPrintln(incomingMessage);
    chessBoard.extinguishMilleniumLEDs();
    updateMephistoLEDs(mephistoLED);
    sendMessageToChessBoard("x");
    return;
  }
}

class MyCallbacks : public BLECharacteristicCallbacks
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
        readChar = (rxValue[i] & 127); // remove odd parity

        assembleIncomingChesslinkMessage(readChar);
      }
      sendChesslinkAnswer(incomingMessage);
    }
  }
};

void initBleService()
{
  //Bluetooth BLE initialization for mode B boards
  //esp_log_level_set("*", ESP_LOG_VERBOSE);

  //Register and initialize BLE Transparent UART Mode.
  BLEDevice::init("MILLENNIUM CHESS");
  BLEDevice::setMTU(517);
  // BLEDevice::setMTU(192);

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
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();
  // Advertise the service
  pServer->getAdvertising()->start();

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
    SerialBT.begin("Open Mephisto DEBUG");
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
    }
    if(connection == BT)
    {
      // SerialBT.end();
      SerialBT.begin("MILLENNIUM CHESS BT");
    }
    if(connection == BLE)
    {
      initBleService();
    }
  }
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
      if ((certPiece & 0b00001111) == 0b00000011)
      {
        lv_obj_set_pos(bp[(certPiece>>4) & 0x0f], getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(bp[(certPiece>>4) & 0x0f], LV_OBJ_FLAG_HIDDEN);
      }
      if ((certPiece & 0b00001111) == 0b00000010)
      {
        lv_obj_set_pos(wp[(certPiece>>4) & 0x0f], getColFromBoardIndex(i) * SQUARE_SIZE, getRowFromBoardIndex(i) * SQUARE_SIZE);
        lv_obj_clear_flag(wp[(certPiece>>4) & 0x0f], LV_OBJ_FLAG_HIDDEN);
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
      // if (certPiece == EMP)
      // {
        if ((oldBoard[i] & 0b00001111) == 0b00000011) // Black Pawn 
        {
          lv_obj_add_flag(bp[(oldBoard[i] >> 4) & 0x0f], LV_OBJ_FLAG_HIDDEN);
        }
        if ((oldBoard[i] & 0b00001111) == 0b00000010) // White Pawn 
        {
          lv_obj_add_flag(wp[(oldBoard[i] >> 4) & 0x0f], LV_OBJ_FLAG_HIDDEN);
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
      // }
      oldBoard[i] = certPiece;
      updated++;
    }
  }
  if (updated < 2)
  {
    lv_label_set_text(debugLbl, "");
  }
  else if (updated != 32)
  {
    lv_label_set_text_fmt(debugLbl, "Updated: %i", updated);
  }
}

static void slider_event_cb(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_target(e);
  brightness = lv_slider_get_value(slider);
  ledcWrite(0, (uint8_t)brightness);
  // lv_label_set_text_fmt(debugLbl, "Brightness: %i", brightness);
}

void switchOff(void)
{
  saveBoardSettings();

  tft.writecommand(0x10); // TFT Display Sleep mode on
  // tft.writecommand(0x28);       // TFT Display Off

  // pinMode(GPIO_NUM_16, OUTPUT);
  ledcDetachPin(TFT_BL);
  digitalWrite(GPIO_NUM_16, LOW); // Switch off backlight, somehow does not work with ILI9488 Display???

  rtc_gpio_isolate(gpio_num_t(ROW_LE));
  rtc_gpio_isolate(gpio_num_t(LDC_LE));
  rtc_gpio_isolate(gpio_num_t(LDC_EN));
  rtc_gpio_isolate(gpio_num_t(CB_EN));

  delay(500);

  esp_sleep_enable_ext0_wakeup(TOUCH_PANEL_IRQ_PIN, LOW);

  gpio_deep_sleep_hold_en();
  esp_deep_sleep_start();
}

static void event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED)
  {
    // lv_label_set_text_fmt(debugLbl, "event-clicked");

    int clickedBoard = 0;
    for (int i = 0; i < 64; i++)
    {
      if (obj == square[i])
      {
        clickedBoard ++;
      }
    }
    if(clickedBoard>0) 
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
      lv_scr_load(settingsScreen);
    }
    if (obj == settingsBtn)
    {
      lv_scr_load(settingsScreen);
    }
    if (obj == restartBtn)
    {
      chessBoard.startPosition(lv_obj_get_state(certaboCalibCB) & LV_STATE_CHECKED);
      resetOldBoard();
      updatePiecesOnBoard();
      lv_scr_load(screenMain);
    }
    else if (obj == exitSettingsBtn)
    {
      lv_scr_load(screenMain);
    }
  }
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    if (obj == usbCB)
    {
      if ((lv_obj_get_state(usbCB) & LV_STATE_CHECKED) == 1)
      {
        lv_obj_clear_state(btCB, LV_STATE_CHECKED);
        lv_obj_clear_state(bleCB, LV_STATE_CHECKED);
      }
    }
    if (obj == bleCB)
    {
      if ((lv_obj_get_state(bleCB) & LV_STATE_CHECKED) == 1)
      {
        lv_obj_clear_state(usbCB, LV_STATE_CHECKED);
        lv_obj_clear_state(btCB, LV_STATE_CHECKED);
      }
    }
    if (obj == btCB)
    {
      if ((lv_obj_get_state(btCB) & LV_STATE_CHECKED) == 1)
      {
        lv_obj_clear_state(usbCB, LV_STATE_CHECKED);
        lv_obj_clear_state(bleCB, LV_STATE_CHECKED);
      }
    }
    if (obj == certaboCB)
    {
      if ((lv_obj_get_state(certaboCB) & LV_STATE_CHECKED) == 1)
      {
        chessBoard.emulation = 0;
      }
    }
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

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data)
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

  lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

  /*Initialize the display*/

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the input device driver*/

  lv_indev_drv_init(&indev_drv);          /*Descriptor of a input device driver*/
  indev_drv.type = LV_INDEV_TYPE_POINTER; /*Touch pad is a pointer-like device*/
  indev_drv.read_cb = my_input_read;      /*Set your driver function*/
  lv_indev_drv_register(&indev_drv);      /*Finally register the driver*/
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

  object = lv_label_create(cont_header);
  lv_label_set_text(object, "Settings");
  lv_obj_center(object);
  lv_obj_add_style(object, &fExtraLargeStyle, 0);   // was f28Style

  object = lv_label_create(content);
  lv_label_set_text(object, "Emulation:");
  lv_obj_add_style(object, &fLargeStyle, 0);   // was f28Style

  /*Create a container for Certabo Settings */
  object = lv_obj_create(content);
  lv_obj_set_size(object, 260, 30);
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
  lv_checkbox_set_text(certaboCalibCB, "w Queens");
  lv_obj_add_event_cb(certaboCalibCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(certaboCalibCB, &fMediumStyle, 0);

  chesslinkCB = lv_checkbox_create(content);
  lv_checkbox_set_text(chesslinkCB, "Millennium/Chesslink");
  lv_obj_add_event_cb(chesslinkCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(chesslinkCB, &fMediumStyle, 0);

  flippedCB = lv_checkbox_create(content);
  lv_checkbox_set_text(flippedCB, "Flip Board");
  lv_obj_add_event_cb(flippedCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_style_pad_top(flippedCB, 16, 0);
  lv_obj_add_style(flippedCB, &fMediumStyle, 0);

  object = lv_label_create(content);
  lv_label_set_text(object, "Connection:");
  lv_obj_set_style_pad_top(object, 12, 0);
  lv_obj_add_style(object, &fLargeStyle, 0);

  /*Create a container for Connection Settings */
  object = lv_obj_create(content);
  lv_obj_set_size(object, 250, 30);
  lv_obj_set_style_radius(object, 0, 0);
  lv_obj_set_style_border_width(object, 0, 0);
  lv_obj_set_flex_flow(object, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_left(object, 0, 0);
  lv_obj_set_style_pad_top(object, 0, 0);
  lv_obj_align_to(object, content, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

  usbCB = lv_checkbox_create(object);
  lv_checkbox_set_text(usbCB, "USB");
  lv_obj_add_event_cb(usbCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(usbCB, &fMediumStyle, 0);

  btCB = lv_checkbox_create(object);
  lv_checkbox_set_text(btCB, "BT");
  lv_obj_add_event_cb(btCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(btCB, &fMediumStyle, 0);

  bleCB = lv_checkbox_create(object);
  lv_checkbox_set_text(bleCB, "BLE");
  lv_obj_add_event_cb(bleCB, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_style(bleCB, &fMediumStyle, 0);

  updateSettingsScreen();

  offBtn = lv_btn_create(content);
  label = lv_label_create(offBtn);
  lv_obj_add_event_cb(offBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_label_set_text(label, "Switch Off");
  lv_obj_set_size(offBtn, 180, 35);
  lv_obj_center(label);
  lv_obj_add_style(label, &fMediumStyle, 0);

  restartBtn = lv_btn_create(content);
  label = lv_label_create(restartBtn);
  lv_label_set_text(label, "Restart");
  lv_obj_add_event_cb(restartBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(restartBtn, 180, 35);
  lv_obj_center(label);
  lv_obj_add_style(label, &fMediumStyle, 0);

  /*Create a container for Brightness Slider */
  object = lv_obj_create(content);
  lv_obj_set_size(object, 180, 55);
  lv_obj_set_style_radius(object, 0, 0);
  lv_obj_set_style_border_width(object, 0, 0);
  lv_obj_set_flex_flow(object, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_left(object, 10, 0);
  lv_obj_set_style_pad_right(object, 10, 0);
  lv_obj_set_style_pad_top(object, 0, 0);
  lv_obj_set_style_pad_bottom(object, 15, 0);
  lv_obj_align_to(object, content, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);

  label = lv_label_create(object);
  // lv_obj_set_size(label, 160, 10);
  // lv_obj_center(label);
  lv_label_set_text(label, "Brightness:");

  lv_obj_t *brightnessSlider = lv_slider_create(object);
  lv_obj_set_size(brightnessSlider, 160, 10);
  // lv_obj_set_style_pad_left(brightnessSlider, 5, 0);

#ifdef PicoResTouchLCD_35
  lv_slider_set_range(brightnessSlider, 155, 255);
#else
  lv_slider_set_range(brightnessSlider, 5, 255);
#endif
  lv_slider_set_value(brightnessSlider, 255, LV_ANIM_ON);
  lv_obj_add_event_cb(brightnessSlider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // calibrateBtn = lv_btn_create(content);
  // label = lv_label_create(calibrateBtn);
  // lv_label_set_text(label, "Calibrate Touch");
  // lv_obj_add_event_cb(calibrateBtn, event_handler, LV_EVENT_ALL, NULL);
  // lv_obj_set_size(calibrateBtn, 180, 35);
  // lv_obj_center(label);
  // lv_obj_add_style(label, &fMediumStyle, 0);

  exitSettingsBtn = lv_btn_create(content);
  label = lv_label_create(exitSettingsBtn);
  lv_obj_add_event_cb(exitSettingsBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_label_set_text(label, "Back");
  lv_obj_set_size(exitSettingsBtn, 180, 35);
  lv_obj_center(label);
  lv_obj_add_style(label, &fMediumStyle, 0);

}

void createUI()
{
  screenMain = lv_obj_create(NULL);

  // lv_style_init(&fSmallStyle);
  // lv_style_set_text_font(&fSmallStyle, &lv_font_montserrat_10);
    
  lv_style_init(&fMediumStyle);
  lv_style_set_text_font(&fMediumStyle, &lv_font_montserrat_18);
    
  lv_style_init(&fLargeStyle);
  lv_style_set_text_font(&fLargeStyle, &lv_font_montserrat_22);  // was 22
    
  lv_style_init(&fExtraLargeStyle);
  lv_style_set_text_font(&fExtraLargeStyle, &lv_font_montserrat_28); // was 28
    
  object = lv_label_create(screenMain);
  lv_label_set_text(object, "Open\nMephisto");
  lv_obj_set_style_text_align(object, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_size(object, 140, 55);
  lv_obj_set_pos(object, 333, 5);
  lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
  lv_obj_add_style(object, &fExtraLargeStyle, 0);  
  
  #ifdef LOLIN_D32
  batteryLbl = lv_label_create(screenMain);
  lv_obj_set_style_text_align(batteryLbl, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_size(batteryLbl, 33, 15);
  lv_obj_set_pos(batteryLbl, 446, 0);
  lv_obj_add_style(batteryLbl, &fLargeStyle, 0);  
  #endif
  
  liftedPiecesStringLbl = lv_label_create(screenMain);
  lv_label_set_text(liftedPiecesStringLbl, "ready");
  lv_obj_set_style_text_align(liftedPiecesStringLbl, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_size(liftedPiecesStringLbl, 135, 160);
  lv_obj_set_pos(liftedPiecesStringLbl, 330, 100);
  lv_obj_add_style(liftedPiecesStringLbl, &fLargeStyle, 0);    // was f28Style    
  
  debugLbl = lv_label_create(screenMain);
  lv_label_set_text(debugLbl, "");
  lv_obj_set_style_text_align(debugLbl, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_size(debugLbl, 135, 40);
  lv_obj_set_pos(debugLbl, 330, 230);
  lv_obj_add_style(debugLbl, &fMediumStyle, 0);  
  
  settingsBtn = lv_btn_create(screenMain);
  lv_obj_add_event_cb(settingsBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_size(settingsBtn, 120, 40);
  lv_obj_set_pos(settingsBtn, 340, 260);

  lv_obj_t * label1 = lv_label_create(settingsBtn);
  lv_label_set_text(label1, "Settings");
  lv_obj_set_align(label1, LV_ALIGN_CENTER);
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

  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);

  pinMode(TFT_BL, OUTPUT);
  gpio_hold_dis((gpio_num_t)TFT_BL); 
  ledcWrite(0, brightness);

  // digitalWrite(TFT_BL, HIGH);    

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

  chessBoard.startPosition(0);
  connection = USB;
  loadBoardSettings();
  // if(chessBoard.emulation == 0 && connection == BLE)
  //   connection = USB;

  mephisto.initPorts();

  for (int i = 0; i < 8; i++)
  {
    chessBoard.lastRawRow[i] = mephisto.readRow(i);
  }

  chessBoard.generateSerialBoardMessage();
  // chessBoard.copyPieceSetupToRaw(chessBoard.lastRawRow);

  initLVGL();

  createUI();

  createSettingsScreen();

  object = lv_label_create(screenMain);
  lv_obj_set_style_text_align(object, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_size(object, 29, 20);
  lv_obj_set_pos(object, 442, 75);
  lv_obj_add_style(object, &fLargeStyle, 0);  

  if(connection == USB) 
  {
    lv_obj_add_state(usbCB, LV_STATE_CHECKED);
    lv_label_set_text(object, LV_SYMBOL_USB);
  }
  if(connection == BT) 
  {
    lv_obj_add_state(btCB, LV_STATE_CHECKED);
    lv_label_set_text(object, LV_SYMBOL_BLUETOOTH);
  }
  if(connection == BLE) 
  {
    lv_obj_add_state(bleCB, LV_STATE_CHECKED);
    lv_label_set_text(object, LV_SYMBOL_BLUETOOTH);
  }

  initSerialPortCommunication();

  lv_scr_load(screenMain);

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
  unsigned long actMillis;
  char batMessage[12]="";
  actMillis = millis();
  if(actMillis-oldMillis>10000) {
    float voltage = analogRead(35)/587.5;
    // sprintf(batMessage, "%.2fV ", voltage);
    // sprintf(batMessage, "");
    if(voltage > 4.15)
    {
      // lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_FULL);
      strcat(batMessage, LV_SYMBOL_BATTERY_FULL);
    }
    else if(voltage > 3.85)
    {
      // lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_3);
      strcat(batMessage, LV_SYMBOL_BATTERY_3);
    }
    else if(voltage > 3.55)
    {
      // lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_2);
      strcat(batMessage, LV_SYMBOL_BATTERY_2);
    }
    else if(voltage > 3.25)
    {
      // lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_1);
      strcat(batMessage, LV_SYMBOL_BATTERY_1);
    }
    else if(voltage <= 3.25)
    {
      // lv_label_set_text(batteryLbl, LV_SYMBOL_BATTERY_EMPTY);
      strcat(batMessage, LV_SYMBOL_BATTERY_EMPTY);
    }
    lv_label_set_text(batteryLbl, batMessage);
    oldMillis=actMillis;
  }
#endif  

  lv_task_handler();

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

  if(chessBoard.emulation==1)
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

  // Read row status from board:
  for(int i=0; i<8; i++) {
    readRawRow[i] = mephisto.readRow(i);
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
  for(int i=0; i<8; i++) { // for each row
    for(int col = 0; col<8; col++) { // for each column
    int diff = (bitRead(chessBoard.lastRawRow[i], col) - bitRead(readRawRow[i], col));
      if(diff<0) {
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

  for (int row=0; row<8; row++) {
      chessBoard.lastRawRow[row]=readRawRow[row];
  }

  // After the whole board is read and any change is regarded, the Serial Board message is re-generated:
  chessBoard.generateSerialBoardMessage();
  if(connection == BLE && chessBoard.emulation==1)
  {
    chessBoard.boardMessage[65]=0;
  }

  if (lifted > 0 || setBack > 0 || chessBoard.emulation == 0) // Certabo boards sends position even when no change happend
  { 
    sendMessageToChessBoard(chessBoard.boardMessage);
  }
  if (chessBoard.emulation == 0 && rows == 0)
  {
    // delay(100);
  }
  writeToIndex = 0;
}