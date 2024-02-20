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
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
const int trigPin = 1; 
const int echoPin = 2; 
long duration;
int distance;
const int numReadings = 10; // Number of readings to calculate moving average
int readings[numReadings]; // Array to store individual readings
int readIndex = 0; // Index of the current reading
int total = 0; // Running total
int average = 0; // Moving average

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "8a645cd7-c78a-467d-8627-acde6685cf23"
#define CHARACTERISTIC_UUID "9ad53e1b-d2b3-4905-b005-23976f604d2f"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// TODO: add DSP algorithm functions here
void calculateMovingAverage(int newReading) {
    // Subtract the last reading
    total = total - readings[readIndex];
    // Read from the sensor
    readings[readIndex] = newReading;
    // Add the reading to the total
    total = total + readings[readIndex];
    // Advance to the next position in the array
    readIndex = readIndex + 1;

    // If we're at the end of the array...
    if (readIndex >= numReadings) {
        // ...wrap around to the beginning
        readIndex = 0;
    }

    // Calculate the average
    average = total / numReadings;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    // Initialize the HC-SR04 sensor pins
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    // Initialize all the readings to 0:
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }

    // TODO: name your device to avoid conflictions
    BLEDevice::init("XIAO_ESP32S3_Yuwei");
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
    pCharacteristic->setValue("Hello World");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;

    // TODO: use your defined DSP algorithm to process the readings
    calculateMovingAverage(distance);

    Serial.print("Raw Distance: ");
    Serial.print(distance);
    Serial.print(" cm, Denoised: ");
    Serial.print(average);
    Serial.println(" cm");

    if (deviceConnected) {
        if (average < 30) {
            // Only send data if average distance is less than 30 cm
            String valueToSend = "Distance: " + String(average) + " cm";
            pCharacteristic->setValue(valueToSend.c_str());
            pCharacteristic->notify();
            Serial.print("Notify value: ");
            Serial.println(valueToSend);
        }
    }


    
    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
        pCharacteristic->setValue("Connect Device name: XIAO_ESP32S3_Yuwei");
        pCharacteristic->notify();
        Serial.println("Notify value: Successfully connected XIAO_ESP32_Cath");
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}