/*  
    Copyright 2021, 2022 Andreas Petersik (andreas.petersik@gmail.com)
    
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

#include <Arduino.h>

#define LATCH_WAIT 10

// Lolin D32 Module:
#define LDC_EN   2    // BLACK  - connects to pin 29 of 40 pin connector
#define CB_EN   15    // WHITE  - connects to pin 30 of 40 pin connector
#define ROW_LE   4    // RED - connects to pin 31 of 40 pin connector
#define LDC_LE   0    // BROWN   - connects to pin 32 of 40 pin connector

class Mephisto {
    public:

//  bytePort describes how D0 (Pin 15 = bytePort[0]) to D7 (Pin 22 = bytePort[7]) of the 40 Pin module connector is connected to the microcontroller

//  byte bytePort[8] = {3, 2, 5, 4, 7, 6, 9, 8};            // For Arduino Uno
    // byte bytePort[8] = {33, 32, 26, 25, 14, 27, 13, 12};    // For Lolin D32
    byte bytePort[8] = {12, 13, 27, 14, 25, 26, 32, 33};    // For Lolin D32

    byte board[8]; // index is row

    // Function prototypes:

    void initPorts();
    byte readRow(byte x);
    void writeRow(byte row, byte value);
};