# Arbeitsjournalournal

## Tag 1

**Ziel:** Recherche
**Gearbeitete Stunden:** 7

Ich habe mich intensiv in den HX711 eingearbeitet, Schema analysiert,
Recherchen durchgeführt, ein GitHub-Repository erstellt und
strukturiert, den Temperatursensor recherchiert und Datenblätter
abgelegt. Zusätzlich habe ich Grundlagen zu Wägezellen erarbeitet, ein
3D-Grobkonzept erstellt, das Inhaltsverzeichnis aufgebaut und mit der
Projektplanung gestartet.

**Ziel:** erreicht

------------------------------------------------------------------------

## Tag 2

**Ziel:** Grundstruktur & Meilensteine
**Gearbeitete Stunden:** 3

Ich habe die Dokumentation weitergeführt, Zeitplan überarbeitet,
Einführung in Markdown-Files erhalten und GitHub aktualisiert. Alle
Dateien wurden in MD konvertiert und das Repository sauber strukturiert.
Meilensteine wurden definiert und der Aufbau eines professionellen Repos
recherchiert.

**Ziel:** erreicht

------------------------------------------------------------------------

## Tag 3

**Ziel:** OLED-Display erfolgreich ansteuern
**Gearbeitete Stunden:** 6

Ich habe die Projektdokumentation in Markdown umgewandelt und weiter daran gearbeitet sowie das README erstellt. Nachdem ich mit Alex besprochen hatte, wie eine klare und gut strukturierte README aufgebaut sein sollte, habe ich diese überarbeitet und verbessert. Beim OLED-Display stellte ich fest, dass auf der Rückseite die Adresse 0x78 angegeben war, das Display jedoch nicht funktionierte. Zur Überprüfung nutzte ich einen I²C-Scanner, der die korrekte Adresse bestätigte. Da das Display mit dem ursprünglichen SSD1306-Code nur fehlerhafte Zeichen ausgab, vermutete ich eine falsche Display-Bibliothek. Nach Installation der U8g2-Library und mehreren Tests stellte ich fest, dass das Modul einen SH1106-Controller besitzt. Nach Anpassung des Codes und der verwendeten Library konnte ich das Display erfolgreich in Betrieb nehmen und die Erkenntnisse dokumentieren.

**Ziel:** erreicht

------------------------------------------------------------------------

## Tag 4

**Ziel:** Temp-Sensor erfolgreich ansteuern
**Gearbeitete Stunden:** 5

Ich habe das Datenblatt des DS18B20 studiert und gelernt, dass der Sensor über das OneWire-Protokoll kommuniziert und dadurch nur eine einzige Datenleitung für die Datenübertragung benötigt. Der Sensor liefert digitale Temperaturwerte, die sich direkt auslesen lassen, ohne dass eine zusätzliche Analogverarbeitung nötig ist. Anschliessend habe ich den Code geschrieben und den DS18B20 erfolgreich zum Laufen gebracht.

**Ziel:** erreicht

------------------------------------------------------------------------

