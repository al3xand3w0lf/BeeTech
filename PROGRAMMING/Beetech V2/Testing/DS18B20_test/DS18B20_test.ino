#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 26  // A0-Pin des Feather V2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  sensors.begin();

  Serial.println("DS18B20 Start...");
}

void loop() {
  sensors.requestTemperatures();                    // Messung starten
  float tempC = sensors.getTempCByIndex(0);         // Ersten Sensor lesen

  Serial.print("Temperatur: ");
  Serial.print(tempC);
  Serial.println(" °C");

  delay(1000);  // 1 Sekunde Pause
}
