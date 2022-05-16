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

#include "mephisto.h"
#include <driver/rtc_io.h>

void Mephisto::initPorts()
{
  rtc_gpio_hold_dis(gpio_num_t(ROW_LE));
  rtc_gpio_hold_dis(gpio_num_t(LDC_LE));
  rtc_gpio_hold_dis(gpio_num_t(LDC_EN));
  rtc_gpio_hold_dis(gpio_num_t(CB_EN));

  pinMode(ROW_LE, OUTPUT);
  pinMode(LDC_LE, OUTPUT);
  pinMode(LDC_EN, OUTPUT);
  pinMode(CB_EN, OUTPUT);

  digitalWrite(CB_EN, HIGH); // only low to read out reed switches!
  digitalWrite(LDC_EN, LOW);

  digitalWrite(ROW_LE, LOW);
  digitalWrite(LDC_LE, LOW);
}

// readRow: byte row can only have values from 0 to 7
byte Mephisto::readRow(byte row)
{
  byte rowResult;

  // 1st STEP: Select row to read:
  digitalWrite(ROW_LE, HIGH);
  for (byte i = 0; i < 8; i++)
  {
    pinMode(bytePort[i], OUTPUT);
    if (i == row)
    {
      digitalWrite(bytePort[i], LOW);
    }
    else
    {
      digitalWrite(bytePort[i], HIGH);
    }
  }
  delayMicroseconds(LATCH_WAIT);
  digitalWrite(ROW_LE, LOW);
  delayMicroseconds(LATCH_WAIT);

  // 2nd STEP: Set all columns to LOW:
  digitalWrite(LDC_LE, HIGH);
  for (byte i = 0; i < 8; i++)
  {
    digitalWrite(bytePort[i], LOW);
  }
  delayMicroseconds(LATCH_WAIT); 
  digitalWrite(LDC_LE, LOW);
  delayMicroseconds(LATCH_WAIT); 

  // 3rd STEP: Read out read switches of selected row:
  digitalWrite(CB_EN, LOW);     // Enable 
  digitalWrite(LDC_EN, HIGH);   // Disable LED output Latch
  delayMicroseconds(LATCH_WAIT); // Mandatory to work!

  rowResult = 0;
  for (byte i = 0; i < 8; i++)
  {
    pinMode(bytePort[i], INPUT);
    rowResult += (digitalRead(bytePort[i])) << i;
  }
  delayMicroseconds(LATCH_WAIT); // Mandatory to work!
  digitalWrite(CB_EN, HIGH);
  digitalWrite(LDC_EN, LOW);

  return ~rowResult;
}

void Mephisto::writeRow(byte row, byte value)
{

  // 1st STEP: Select row to write LED pattern:
  digitalWrite(ROW_LE, HIGH);
  for (byte i = 0; i < 8; i++)
  {
    pinMode(bytePort[i], OUTPUT);
    if (i == row)
    {
      digitalWrite(bytePort[i], LOW);
    }
    else
    {
      digitalWrite(bytePort[i], HIGH);
    }
  }
  delayMicroseconds(LATCH_WAIT);
  digitalWrite(ROW_LE, LOW);
  delayMicroseconds(LATCH_WAIT);


  // 2nd STEP: Write byte into selected row:
  digitalWrite(LDC_LE, HIGH);
  for (byte i = 0; i < 8; i++)
  {
    digitalWrite(bytePort[i], ((value >> i) & 0x1));
  }
  delayMicroseconds(LATCH_WAIT);
  digitalWrite(LDC_LE, LOW);
  delayMicroseconds(LATCH_WAIT);
}