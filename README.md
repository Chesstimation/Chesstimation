# Open Mephisto

This project uses Arduino / ESP32 hardware to connect an old Hegener+Glaser Mephisto Modular chess computer (Modular, Exclusive or München Board) to a computer.
To be able to use existing software on the (Windows-) Computer, a Certabo or a Millennium (Chesslink) board can be simulated.

# Following hardware components are required:
1. Wemos LolinD32 or similar (sufficient number of GPIO pins is required, 12 pins for Mephisto board control + approx. 8 pins for touch display)
2. 40-pin edge card connector (Sullins Connector Solutions EBC20DRAS or EDAC Inc. 395-040-558-301)
3. 3.5 inch Waveshare TFT Arduino display (fits perfectly into one Mephisto module slot!)
4. optionally lipo battery with plug for Lolin D32

# How to connect the 40-pin plug to the Mephisto board?

If you look at the chessboard or module from the front, the upper row of contacts is numbered from left to right from 1 to 39, the lower row from 2 to 40.
However, there seem to be connectors where the odd (top) and even (bottom) contacts are swapped!

Only 14 pins are required to be able to light the LEDs and read out the reed switches from the chessboard:
* D0-D7 are pins 15 to 22
* LDC_EN is pin 29
* CB_EN  is pin 30
* ROW_LE is pin 31
* LDC_LE is pin 32

* VCC is pin 1 and pin 2 (both 5V and 3.3V are working for me, ESP32 CANNOT BE DRIVEN WITH 5V!)
* GND is pin 39 and pin 40

# Software development environment

Software development is done in C++ using:
* Visual Studio Code
* Platform IO
