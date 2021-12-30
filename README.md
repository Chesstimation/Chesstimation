# Open Mephisto

This project uses ESP32 hardware to connect an old Hegener+Glaser Mephisto Modular chess computer (Modular, Exclusive or München Board) to a computer or a smart phone.

## Main Features:

* No modification to original Mephisto Modular/Exclusive/München board required. Just replace the old modules with the new one!
* 3.5" Color-TFT (480x320 Pixels), available space is perfectly used.
* Current position is shown on the display.
* Both White and Black can be played from the front.
* All opponent's moves are displayed via the board's LEDs.
* Power supply via built-in battery that can be charged via USB-C (with 2200 mAh lipo >7h running time, ~4h charging time). 
* Communication with PC via USB-C or Bluetooth Classic, to iPhone via BLE (Bluetooth Low Energy).
* Playing completely without cables is possible!
* All settings and the board position are not lost when the device is switched off!
* Switching on and off takes less than 2 seconds.
* Certabo or Millennium Chesslink can be selected as emulation, making it compatible with many chess programs:
  * "White Pawn" on iPhone (https://khadimfall.com/p/white-pawn)
  * BearChess (http://www.solanosoft.com/index.php?page=bearchess)
  * original Certabo Software (https://www.certabo.com/)
  * Lucas Chess (https://lucaschess.pythonanywhere.com/index?lang=en)
  * and via Graham's DLLs (https://goneill.co.nz/chess.php) actually all software that can handle DGT boards.

## Limitations:

* Pawn promotion currently to queen only.
* Captured piece needs to be removed from board before capturing piece is put to the square. 
* Must not be combined with other modules or power supplies.

## Donation:

If you like this project, I would be happy to receive a donation. 

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/AndreasPetersik)

## Following hardware components are required:
1. Wemos LolinD32 (https://de.aliexpress.com/item/32808551116.html)
2. 40-pin edge card connector, e.g. https://de.aliexpress.com/item/33035971298.html (you need to select 2x20 Pin)
3. Rainbow Ribbon Cable (~30cm) with 1.27mm spacing, e.g. from Aliexpress: https://de.aliexpress.com/item/1005003002882947.html (you need to select 1.27mm spacing!)
4. 3.5 inch Waveshare TFT Pico Res Touch display: https://www.waveshare.com/pico-restouch-lcd-3.5.htm (fits perfectly into one Mephisto module slot!)
5. optionally lipo battery with plug for Lolin D32, e.g.: https://www.ebay.de/itm/324239268082?hash=item4b7e2a98f2:g:IXQAAOSwRJVhug4l (you need an extra JST PH 2.0mm plug for this battery)

## How to connect the 40-pin plug to the Mephisto board?

If you look at the chessboard or module from the front, the upper row of contacts is numbered from left to right from 1 to 39, the lower row from 2 to 40.
With the ribbon cable connector as linked above the wires are numbered a bit different, from left to right: 2,1, 4,3, 6,5, 8,7, ... 40,39.
This has already been considered in below table and you will be able to connect the 8 data wires just in parallel to the Lolin D32.

 <table>
  <tr>
    <th>40-pin Connector Pin</th>
    <th>LolinD32 Pin</th>
  </tr>
  <tr>
    <td>1+2</td>
    <td>3.3V</td>
  </tr>
  <tr>
    <td>39+40</td>
    <td>GND</td>
  </tr>
  <tr>
    <td>15 / D0</td>
    <td>GPIO 33</td>
  </tr>
  <tr>
    <td>16 / D1</td>
    <td>GPIO 32</td>
  </tr>
  <tr>
    <td>17 / D2</td>
    <td>GPIO 26</td>
  </tr>
  <tr>
    <td>18 / D3</td>
    <td>GPIO 25</td>
  </tr>
  <tr>
    <td>19 / D4</td>
    <td>GPIO 14</td>
  </tr>
  <tr>
    <td>20 / D5</td>
    <td>GPIO 27</td>
  </tr>
  <tr>
    <td>21 / D6</td>
    <td>GPIO 13</td>
  </tr>
  <tr>
    <td>22 / D7</td>
    <td>GPIO 12</td>
  </tr>
  <tr>
    <td>29 / LDC_EN</td>
    <td>GPIO 0</td>
  </tr>
  <tr>
    <td>30 / CB_EN</td>
    <td>GPIO 4</td>
  </tr>
  <tr>
    <td>31 / ROW_LE</td>
    <td>GPIO 15</td>
  </tr>
  <tr>
    <td>32 / LDC_LE</td>
    <td>GPIO 2</td>
  </tr>
</table> 

## How to connect the 3.5" display to Lolin D32

 <table>
  <tr>
    <th>TFT Display Pin</th>
    <th>LolinD32 Pin</th>
  </tr>
  <tr>
    <td>VSYS</td>
    <td>3.3V</td>
  </tr>
  <tr>
    <td>GND</td>
    <td>GND</td>
  </tr>
  <tr>
    <td>LCD_DC</td>
    <td>GPIO 17</td>
  </tr>
  <tr>
    <td>LCD_CS</td>
    <td>SS/GPIO 5</td>
  </tr>
  <tr>
    <td>CLK</td>
    <td>SCK/GPIO 18</td>
  </tr>
  <tr>
    <td>MOSI</td>
    <td>MOSI/GPIO 23</td>
  </tr>
  <tr>
    <td>MISO</td>
    <td>MISO/GPIO 19</td>
  </tr>
  <tr>
    <td>LCD_BL</td>
    <td>GPIO 16</td>
  </tr>
  <tr>
    <td>LCD_RST</td>
    <td>RESET</td>
  </tr>
  <tr>
    <td>TP_CS</td>
    <td>GPIO 21</td>
  </tr>
  <tr>
    <td>TP_IRQ</td>
    <td>GPIO 34</td>
  </tr>
</table> 

## Software development environment

Software development is done in C++ using:
* Visual Studio Code
* Platform IO

In Platform IO you need to install 2 additional libraries for this project:
* LVGL 8.1 (you need to use Open Mephisto's lv_conf.h)
* TFT eSPI 2.4.11 (you need to use chess-module-display.h as User_Setup.h)

For flashing the firmware to the Lolin D32 without Platform IO I use ESP Flasher: https://github.com/Jason2866/ESP_Flasher/releases