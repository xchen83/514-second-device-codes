#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// MQ-4 Sensor
const int mq4Pin = 32; // Example analog pin for MQ-4 sensor

// TCS3200 Color Sensor
const int s0 = 25; // Example pin for S0
const int s1 = 26; // Example pin for S1
const int s2 = 27; // Example pin for S2
const int s3 = 33; // Example pin for S3
const int outPin = 34; // Example pin for OUT

void setupColorSensor() {
    pinMode(s0, OUTPUT);
    pinMode(s1, OUTPUT);
    pinMode(s2, OUTPUT);
    pinMode(s3, OUTPUT);
    pinMode(outPin, INPUT);

    // Set frequency scaling to 20%
    digitalWrite(s0, HIGH);
    digitalWrite(s1, LOW);
}

unsigned long readColorFrequency() {
    unsigned long startTime = millis();
    unsigned long pulseCount = 0;
    while (millis() - startTime < 100) { // Measure frequency for 100 milliseconds
        pulseCount += pulseIn(outPin, LOW);
    }
    return pulseCount;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    pinMode(mq4Pin, INPUT); // MQ-4 sensor setup
    setupColorSensor(); // TCS3200 sensor setup

    BLEDevice::init("ESP32_Sensor_Server");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    int mq4Reading = analogRead(mq4Pin); // Read the MQ-4 sensor

    // Read colors from the TCS3200
    digitalWrite(s2, LOW);
    digitalWrite(s3, LOW); // Set to red filter
    unsigned long redFrequency = readColorFrequency();

    digitalWrite(s2, HIGH);
    digitalWrite(s3, HIGH); // Set to green filter
    unsigned long greenFrequency = readColorFrequency();

    digitalWrite(s2, LOW);
    digitalWrite(s3, HIGH); // Set to blue filter
    unsigned long blueFrequency = readColorFrequency();

    if (deviceConnected) {
        // Send sensor data over BLE
        String valueToSend = "Methane: " + String(mq4Reading) + 
                             ", Red: " + String(redFrequency) +
                             ", Green: " + String(greenFrequency) +
                             ", Blue: " + String(blueFrequency);
        pCharacteristic->setValue(valueToSend.c_str());
        pCharacteristic->notify();
    }

    delay(1000); // Delay between readings
}