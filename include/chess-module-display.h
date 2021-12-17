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

// For ESP32
// See SetupX_Template.h for all options available

// 3.5inch Touch LCD Shield for Arduino:

#define PicoResTouchLCD_35      // Newer Display
// #define ArduinoTouchShield35    // Older Display

#define RPI_DISPLAY_TYPE

#ifdef ArduinoTouchShield35
#define ILI9486_DRIVER
#define TFT_INVERSION_OFF   
#define TOUCH_CS 22     // Chip select pin (T_CS) of touch screen
#endif

#ifdef PicoResTouchLCD_35
#define ILI9488_DRIVER
#define TFT_INVERSION_ON   
#define TOUCH_CS 21    // Chip select pin (T_CS) of touch screen
#endif

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5  // Chip select control pin
#define TFT_DC   17  // Data Command control pin
#define TFT_BL   16 // Backlight control pin // added by Andreas Petersik
#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

// #define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// #define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
// #define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// #define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
// #define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
// #define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
// #define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// #define SMOOTH_FONT

#define SPI_FREQUENCY  20000000 // Some displays will operate at higher frequencies

#define SPI_TOUCH_FREQUENCY  2500000
