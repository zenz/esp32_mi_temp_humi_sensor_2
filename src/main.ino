#include <Arduino.h>
#include "BLEDevice.h"

static BLEUUID srvUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
static BLEUUID chrUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");
static BLEAdvertisedDevice *mi_temp;
static BLERemoteCharacteristic *remoteChr;
BLEClient *pClient = BLEDevice::createClient();
static bool doConnect = false;
static bool connected = false;
static bool datad = false;
unsigned long idleTime = 5 * 60 * 1000L;
int ledPin = 22;

static void chrCB(BLERemoteCharacteristic *remoteChr, uint8_t *pData, size_t length, bool isNotify)
{
    float temp = (pData[0] | (pData[1] << 8)) * 0.01;
    float humi = pData[2];
    float voltage = (pData[3] | (pData[4] << 8)) * 0.001;
    Serial.printf("Temperature: %.1f C ; Humidity: %.1f %% ; Voltage: %.3f V\n", temp, humi, voltage);
    datad = true;
    digitalWrite(ledPin, HIGH);
}

class deviceCB : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.getName() != "LYWSD03MMC")
        {
            Serial.print(".");
        }
        else
        {
            Serial.println("\nMIJIA Temp/Humi Sensor 2 Found!");
            BLEDevice::getScan()->stop();
            Serial.println("Stopping scan and connecting to temp sensor");
            mi_temp = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
        }
    }
};

class ClientCB : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
    }

    void onDisconnect(BLEClient *pclient)
    {
        connected = false;
    }
};

bool connectToMiTemp()
{
    digitalWrite(ledPin, LOW);
    datad = false;
    Serial.println("Establishing communications with temp sensor:");
    pClient->setClientCallbacks(new ClientCB());
    pClient->connect(mi_temp);
    Serial.println("    Connected to temp sensor");
    BLERemoteService *pRemoteService = pClient->getService(srvUUID);
    if (pRemoteService == nullptr)
    {
        Serial.println("    Error: Failed to find service");
        pClient->disconnect();
        return false;
    }
    Serial.println("    Service found");
    remoteChr = pRemoteService->getCharacteristic(chrUUID);
    if (remoteChr == nullptr)
    {
        Serial.println("    Failed to find characteristic");
        pClient->disconnect();
        return false;
    }
    Serial.println("    Characteristic found");
    Serial.println("    Setting callback for notify / indicate");
    remoteChr->registerForNotify(chrCB);
    return true;
}

void disconnectToMiTemp()
{
    remoteChr->registerForNotify(NULL, false);
    Serial.println("    Removing callback for notify / indicate");
    pClient->disconnect();
    connected = false;
    unsigned int mins = idleTime / 1000 / 60;
    Serial.printf("Disconnected. wait %d mins for next connection...\n", mins);
    delay(idleTime);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Searching for MIJIA Temp/Humi Sensor 2");
    BLEDevice::init("");
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new deviceCB());
    pBLEScan->setInterval(0x50);
    pBLEScan->setWindow(0x30);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10, false);
    pinMode(ledPin, OUTPUT);
}

void loop()
{
    static unsigned long counter = 0;
    if (doConnect && !connected)
    {
        counter += 1;
        Serial.printf("\n\nThis is the %lu approachs for reading...\n", counter);
        connected = connectToMiTemp();
    }
    if (datad)
    {
        disconnectToMiTemp();
    }
}