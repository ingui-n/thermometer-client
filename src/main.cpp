#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoJson.h>
#include <env.h>

OneWire oneWire(THERMOMETER_PIN);
DallasTemperature sensors(&oneWire);

uint8_t broadcastAddress[] = RECEIVE_MAC;

void OnDataSent(uint8_t *mac_addr, const uint8_t sendStatus) {
    Serial.print("Last Packet Send Status: ");
    Serial.println(sendStatus == 0 ? "Delivery success" : "Delivery fail");
}

void sendTemperature(const double temperature) {
    JsonDocument doc;
    String jsonData;

    doc["device"] = DEVICE_NAME;
    doc["password"] = ESP_NOW_PASSWORD;
    doc["temperature"] = temperature;

    serializeJson(doc, jsonData);
    esp_now_send(broadcastAddress, (uint8_t *) jsonData.c_str(), static_cast<int>(jsonData.length()));
}

void setup() {
    Serial.flush();
    Serial.begin(SERIAL_BAUDRATE);
    sensors.begin();

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, nullptr, 0);

    Serial.println("Setup complete");
}

void loop() {
    const unsigned long time = millis();
    static unsigned long lastTime = 0;

    if (time - lastTime > 60000) {
        sensors.requestTemperatures();
        const double temperatureC = sensors.getTempCByIndex(0) + TEMPERATURE_CALIBRATION;

        Serial.println("Temperature: " + String(temperatureC));

        if (temperatureC > -50 && temperatureC < 100) {
            sendTemperature(temperatureC);
        } else {
            Serial.println("Temperature too low - measurement error");
        }

        lastTime = time;
    }
}
