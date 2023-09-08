#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "bme280.h"
#include <ArduinoJson.h>

// global variables
const char* ssid = "504";
const char* password = "cardenas16V";
const char* mqtt_server = "192.168.1.106"; // Dirección IP de tu servidor MQTT
const char* mqtt_topic = "temperatura"; // Tema MQTT al que se va a publicar
time_t initialTime = 0; // Variable global para almacenar la hora inicial
StaticJsonDocument<200> jsonDocument;


WiFiClient espClient;
PubSubClient client(espClient);

// Prototipos de funciones
void setup_bme280(); // Configura el sensor BME280
void read_BME280(void *parameter); // Lee la temperatura del sensor BME280
void setup_wifi(); // Configura la conexión WiFi
void setup_time(); // Configura la hora del sistema
String get_time(); // Obtiene la hora del sistema


void setup() {
  Serial.begin(115200);
  setup_bme280();
  setup_wifi();
  setup_time();

  // Configuración del cliente MQTT
  client.setServer(mqtt_server, 1883);

  // Crear tarea para leer la temperatura
  xTaskCreatePinnedToCore(
    read_BME280,
    "ReadTemperatureTask",
    4096,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
  // Si no hay conexión al servidor MQTT, intenta reconectarse
  if (!client.connected()) {
    // Conexión al broker MQTT
    if (client.connect("ESP32_Client")) {
      Serial.println("Conexión MQTT exitosa");
    } else {
      Serial.print("Fallo en la conexión MQTT, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos...");
      delay(5000);
    }
  }
  client.loop();
}

void setup_wifi() {
  delay(10);
  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conexión WiFi exitosa");
  Serial.println("Dirección IP obtenida: ");
  Serial.println(WiFi.localIP());
}


void read_BME280(void *parameter) {
  (void)parameter;

  for (;;) {
    // Calcula la hora actual basada en el reloj interno y la hora inicial
    time_t now = initialTime + (millis() / 1000);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // Lee la temperatura del sensor BME280
    temperature_bme280 = bme.readTemperature();
    humidity_bme280 = bme.readHumidity();
    pressure_bme280 = bme.readPressure() / 100.0F;
    altitude_bme280 = bme.readAltitude(SEALEVELPRESSURE_HPA);

    // Agrupa los datos en un JSON, formatea la temperatura a 2 decimales
    jsonDocument["temperatura"] = String(temperature_bme280, 2);
    jsonDocument["timestamp"] = strftime_buf;

    // Serializa el JSON en una cadena de texto
    String buffer;
    serializeJson(jsonDocument, buffer);
    
    Serial.print("Mensaje = ");
    Serial.print(buffer);
    Serial.print("Time = ");
    Serial.println(strftime_buf);

    // Publica la temperatura en el tema MQTT y la hora en JSON
    if (client.connected()) {
      client.publish(mqtt_topic, buffer.c_str());
    } else {
      Serial.println("Fallo al publicar el mensaje");
    }

    // clear jsonDocument
    jsonDocument.clear();
    //clear buffer
    buffer = "";
    
    vTaskDelay(pdMS_TO_TICKS(5000)); // Espera 5 segundos antes de la siguiente lectura
  }
}


// get date and time from ntp server utc - 6
void setup_time() {
  configTime(-6 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  initialTime = now; // Almacena la hora inicial una vez que se sincroniza
  Serial.println("");
  Serial.println("Time has been synced");
}

// format time to string yyyy-mm-dd hh:mm:ss and return string
String get_time() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(strftime_buf);
}