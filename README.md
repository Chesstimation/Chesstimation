**English** | [Deutsch](./README_de.md) |

# Chesstimation

This project uses ESP32 hardware to connect an old Hegener+Glaser Mephisto Modular chess computer (Modular, Exclusive or München Board) to a computer or a smart phone.

## Main Features:

* No modification to original Mephisto Modular/Exclusive/München board required. Just replace the old modules with the new one!
* 3.5" Color-TFT (480x320 Pixels), available space is perfectly used.
* Modern, intuitive touch control
* Current position is shown on the display.
* Both White and Black can be played from the front.
* All opponent's moves are displayed via the board's LEDs.
* Power supply via built-in battery that can be charged via USB-C (with 2200 mAh lipo >7h running time, ~4h charging time). 
* Communication with PC via USB-C, Bluetooth Classic or BLE (Bluetooth Low Energy).
* Playing completely without cables is possible!
* All settings and the board position are not lost when the device is switched off!
* Switching on and off takes less than 2 seconds.
* Certabo or Millennium Chesslink can be selected as emulation, making it compatible with many chess programs:
  * "White Pawn" on iPhone and Android (https://khadimfall.com/p/white-pawn)
  * "Chess for Android"
  * "Chess Dojo App" for Android
  * BearChess (http://www.solanosoft.com/index.php?page=bearchess)
  * original Certabo Software (https://www.certabo.com/)
  * Lucas Chess (https://lucaschess.pythonanywhere.com/index?lang=en)
  * and via Graham's DLLs (https://goneill.co.nz/chess.php) actually all software that can handle DGT boards.
* Pawn underpromotion selection when reaching second last row

## Limitations:

* Captured piece needs to be removed from board before capturing piece is put to the square. 
* Must not be combined with other modules or power supplies.

## Contact & Donation:

If you like this project you may contact me via chesstimation@kabelmail.de

I would also be happy to receive a donation. 

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/AndreasPetersik)

## Following hardware components are required:
1. Wemos LolinD32 (https://de.aliexpress.com/item/32808551116.html)
2. 40-pin edge card connector, e.g. https://de.aliexpress.com/item/33035971298.html (you need to select 2x20 Pin)
3. Rainbow Ribbon Cable (~30cm) with 1.27mm spacing, e.g. from Aliexpress: https://www.aliexpress.com/item/1005002509747445.html (you need to select 1.27mm spacing and 40 pin!)
4. Waveshare 3.5 inch TFT Pico Res Touch display: https://www.waveshare.com/pico-restouch-lcd-3.5.htm, e.g available here: https://eckstein-shop.de/WaveShare35inchTouchDisplayModuleforRaspberryPiPico2C65KColors2C480C3973202CSPI
5. Micro USB soldering plug (https://www.aliexpress.com/item/33060931097.html) and USB-C female soldering connector (e.g. https://www.aliexpress.com/item/1005002292881776.html) 
6. optionally lipo battery with plug for Lolin D32, e.g.: https://www.aliexpress.com/item/1005001310592871.html (you need an extra JST PH 2.0mm plug for this battery, e.g. https://www.aliexpress.com/item/1005001315857869.html Double-check polarity of plug with + and - signs on PCB of Lolin!) 
7. Micro-Switch (built into bottom of module) to switch off module completely (https://www.aliexpress.com/item/4001202080623.html Choose MSS-22D18 type!) 

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
    <td>BAT</td>
  </tr>
  <tr>
    <td>39+40</td>
    <td>GND</td>
  </tr>
  <tr>
    <td>15 / D0</td>
    <td>GPIO 12</td>
  </tr>
  <tr>
    <td>16 / D1</td>
    <td>GPIO 13</td>
  </tr>
  <tr>
    <td>17 / D2</td>
    <td>GPIO 27</td>
  </tr>
  <tr>
    <td>18 / D3</td>
    <td>GPIO 14</td>
  </tr>
  <tr>
    <td>19 / D4</td>
    <td>GPIO 25</td>
  </tr>
  <tr>
    <td>20 / D5</td>
    <td>GPIO 26</td>
  </tr>
  <tr>
    <td>21 / D6</td>
    <td>GPIO 32</td>
  </tr>
  <tr>
    <td>22 / D7</td>
    <td>GPIO 33</td>
  </tr>
  <tr>
    <td>29 / LDC_LE</td>
    <td>GPIO 2</td>
  </tr>
  <tr>
    <td>30 / ROW_LE</td>
    <td>GPIO 15</td>
  </tr>
  <tr>
    <td>31 / CB_EN</td>
    <td>GPIO 4</td>
  </tr>
  <tr>
    <td>32 / LDC_EN</td>
    <td>GPIO 0</td>
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
    <td>BAT</td>
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
    <td>RST(RESET)</td>
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
* LVGL 8.3.4 (you need to use Chesstimation's lv_conf.h)
* TFT eSPI 2.5.2 (you need to use Chesstimation's as User_Setup.h and User_Setup_Select.h)

For flashing the firmware to the Lolin D32 without Platform IO I use ESP Flasher: https://github.com/Jason2866/ESP_Flasher/releases
