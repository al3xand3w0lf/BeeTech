&nbsp; BeetechV2\_Scale.ino (Hauptdatei)



&nbsp; - Hardware-Beschreibung aktualisiert: ESP32-S3 Dev Board, ST7567 LCD

&nbsp; - Alle Pin-Definitionen auf ESP32-S3 Pin Mapping umgestellt

&nbsp; - SPI-Bus explizit initialisiert: SPI.begin(12, 13, 11, 10) - geteilt zwischen SD-Karte und LCD

&nbsp; - I2C mit neuen Pins: Wire.begin(8, 9)

&nbsp; - Display-Objekt von U8G2\_SH1106\_128X64\_NONAME\_F\_HW\_I2C oled → U8G2\_ST7567\_ENH\_DG128064I\_F\_4W\_HW\_SPI lcd

&nbsp; - Alle oled\_\* Aufrufe → lcd\_\*

&nbsp; - Extra-Pins als #define angelegt: SD\_CARD\_DETECT (46), USER\_BUTTON\_PIN (0)



&nbsp; SH1106\_OLED.ino → ST7567\_LCD.ino (umbenannt + umgeschrieben)



&nbsp; - Alle Funktionen von oled\_\* → lcd\_\* umbenannt

&nbsp; - I2C-spezifischen Init-Code entfernt (setI2CAddress)

&nbsp; - Backlight-Steuerung hinzugefügt (lcd\_backlightOn/Off, active LOW)

&nbsp; - lcd\_init() setzt jetzt Backlight-Pin und setContrast(200)

&nbsp; - lcd\_off/on schaltet jetzt auch Backlight mit



&nbsp; Nicht geändert (bleiben wie sie sind)



&nbsp; - HX711\_Scale.ino - nutzt nur HX711\_DOUT\_PIN/HX711\_CLK\_PIN aus der Hauptdatei

&nbsp; - DS18B20\_Temperature.ino - nutzt DS18B20\_PIN aus der Hauptdatei

&nbsp; - SdCard.ino - nutzt SD\_CS\_PIN aus der Hauptdatei

&nbsp; - Database.ino, Terminal.ino - keine Display/Pin-Abhängigkeiten

