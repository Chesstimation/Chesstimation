**Deutsch** | [English](./README.md) |

# Chesstimation

In diesem Projekt wird ein neues Modul für alte Mephisto Schachbretter der Modular, Exclusive und München-Serie entwickelt.
Mein Ziel war es, hier nur die aktuellsten Technologien wie Touch-Display, Akkubetrieb, drahtlose Kommunikation über Bluetooth und USB-C zu verwenden.
Mein Hauptziel war kein neuer Schachcomputer sondern hauptsächlich eine Schnittstelle zu schaffen, um diese alten Bretter mit einem Computer oder einem Smartphone verbinden zu können. Darüber kann man dann sowohl online als auch gegen eine fast unendliche Anzahl verschiedener Schachengines spielen.
Für das Modul werden nur wenige Standard-Hardwarekomponenten benötigt.

## Hauptmerkmale:

* Keine Modifikation am originalen Mephisto Modular/Exclusive/München Brett erforderlich. Einfach die alten Module durch das Modul aus diesem Projekt ersetzen!
* 3,5" Farb-TFT-Display (480x320 Pixel), der verfügbare Platz wird perfekt ausgenutzt.
* moderne, intuitive Bedienung über Touch
* Die aktuelle Stellung der Figuren wird auf dem Display angezeigt.
* Sowohl Weiß als auch Schwarz können von vorne gespielt werden.
* Alle Züge des Gegners werden über die LEDs auf dem Brett angezeigt.
* Stromversorgung über einen eingebauten Akku, der über USB-C geladen werden kann (mit 2200 mAh Lipo >7h Laufzeit, ~4h Ladezeit). 
* Kommunikation wahlweise über USB-C, Bluetooth Classic oder BLE (Bluetooth Low Energy).
* Spielen ganz ohne Kabel ist somit möglich!
* Alle Einstellungen und die Position des Figuren gehen beim Ausschalten des Geräts nicht verloren!
* Das Ein- und Ausschalten dauert weniger als 2 Sekunden.
* Certabo oder Millennium Chesslink kann als Emulation ausgewählt werden, wodurch es mit vielen Schachprogrammen kompatibel ist.
* Durch die große Kompatibilität kann gegen fast sämlich verfügbare Schachsoftware und auch online gespielt werden.
* Folgende Software wurde bislang erfolgreich getestet:
  * "White Pawn" auf iPhone oder Android (https://khadimfall.com/p/white-pawn)
  * "Chess for Android"
  * "Chess Dojo App" auf Android
  * BearChess unter Windows (http://www.solanosoft.com/index.php?page=bearchess)
  * Original Certabo Software unter Windows (https://www.certabo.com/)
* Über Grahams DLLs (https://goneill.co.nz/chess.php) kann eigentlich jede Software verwendet werden, die DGT-Bretter ansteuern kann.

## Aktuelle Einschränkungen:

* Bauernumwandlung nur in Dame.
* Die geschlagene Figur muss erst vom Brett entfernt werden, bevor die schlagende Figur auf das Feld gesetzt wird. 
* Kann nicht gleichzeitig mit anderen Mephisto-Modulen im Brett betrieben werden (macht aber auch keinen Sinn!).

## Kontakt / Spenden:

Falls euch das Projekt gefällt, könnt ihr mich gerne kontaktieren über chesstimation@kabelmail.de

Über eine Spende würde ich mich auch sehr freuen:

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/AndreasPetersik)

## Folgenden Hardware-Komponenten werden benötigt:
1. Wemos LolinD32 (https://de.aliexpress.com/item/32808551116.html)
2. 40-poliger Platinenrandstecker, z.B. https://de.aliexpress.com/item/33035971298.html (ihr müsst 2x20 Pin auswählen)
3. Farbiges Flachbandkabel (~20cm) mit 1.27mm Litzenabstand, 40-polig, z.B. von Aliexpress: https://www.aliexpress.com/item/1005002509747445.html (1.27mm spacing auswählen!)
4. Waveshare 3.5 inch TFT Pico Res Touch display: https://www.waveshare.com/pico-restouch-lcd-3.5.htm, z.B. hier: https://eckstein-shop.de/WaveShare35inchTouchDisplayModuleforRaspberryPiPico2C65KColors2C480C3973202CSPI
5. Micro USB Stecker zum Löten (https://www.aliexpress.com/item/33060931097.html) und USB-C Buchse zum Löten (z.B. https://www.aliexpress.com/item/1005002292881776.html) 
6. Akku mit PH2 Stecker für Lolin D32, z.B.: https://www.aliexpress.com/item/1005001310592871.html (ihr braucht einen extra JST PH 2.0mm Stecker (z.B. https://www.aliexpress.com/item/1005001315857869.html), auf richtige Polung achten! Die Polung auf dem Lolin scheint nicht zu den verfügbaren Akkus zu passen, also Plus- und Minusanschluss müssen getauscht werden!)
7. Mikro-Schiebeschalter (Einbau auf der Unterseite des Moduls) zum Komplettabschalten des Moduls (https://www.aliexpress.com/item/4001202080623.html Typ MSS-22D18 wählen!) 

## Benötigte Werkzeuge und Fähigkeiten:
* entsprechendes Werkzeug zum Löten: Lötkolben, Abisolierzange, Seitenschneider, etc.
  * es müssen 16 Kabel einseitig und 15 weitere Kabel beidseitig angelötet werden.
* ein Schraubstock mit einer Klemmbreite von 6 cm oder mehr, um den Stecker auf das Flachbandkabel pressen zu können
* ein Multimeter ist hilfreich, um die korrekte Polung des Micro-USB zu USB-C Kabels sicher zu stellen.
* ein 3D-Drucker, um das Modulgehäuse zu drucken. Alternativ kann man den 3D-Druck bei einem Dienstleister in Auftrag geben.

## Wie wird der 40-polige Stecker zum Schachbrett mit dem Lolin D32 verbunden?

Wenn man von vorne auf das Schachbrett oder das Modul schaut, gibt es zwei Reihen je 20 Kontakte in dem Stecker womit das Modul mit dem Schachbrett verbunden wird. Die obere Reihe ist mit den ungeraden Nummern von 1 - 39 durchnummeriert, die untere Reihe mit den geraden Nummern von 2 - 40.
Der oben verlinkte Flachbandstecker führt aber nun dazu, dass die Litzen im Flachbandkabel paarweise getauscht erscheinen, also das erste Kabel hat die Nummer 2, dann 1, dann 4,3, 6,5, 8,7 usw. bis. 40,39.

Ich habe dies in unten stehender Tabelle so berücksichtigt, dass man sowohl die 8 Datenleitungen (D0 - D7), als auch die 4 Steuerleitungen die vom Schachbrett kommen, so einfach parallel an den Lolin anschließen kann.

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

## Verbindung vom Touch Display zum Lolin D32

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

## Softwareentwicklungsumgebung

Die Software wurde in C/C++ entwickelt, unter Benutzung folgender Tools:
* Visual Studio Code
* Platform IO

In Platform IO braucht man zwei zusätzliche Bibliotheken:
* LVGL 8.3.4 (you need to use Chesstimation's lv_conf.h)
* TFT eSPI 2.5.2 (you need to use Chesstimation's as User_Setup.h and User_Setup_Select.h)

For flashing the firmware to the Lolin D32 without Platform IO I use ESP Flasher: https://github.com/Jason2866/ESP_Flasher/releases
