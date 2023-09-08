#ifndef BME280_H
#define BME280_H
#include  "Adafruit_BME280.h"

// BME280 sensor
Adafruit_BME280 bme;
double SEALEVELPRESSURE_HPA = 1013.25;
float temperature_bme280;
float humidity_bme280;
float pressure_bme280;
float altitude_bme280;

// Function setup bme280 sensor function
void setup_bme280() {
  Serial.begin(115200);
  Serial.println("BME280 Sensor Test");
  bool status;
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

#endif