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
unsigned long idleTime = 120000L;

static void chrCB(BLERemoteCharacteristic *remoteChr, uint8_t *pData, size_t length, bool isNotify)
{
    float temp = (pData[0] | (pData[1] << 8)) * 0.01;
    float humi = pData[2];
    float voltage = (pData[3] | (pData[4] << 8)) * 0.001;
    Serial.printf("Temperature: %.1f C ; Humidity: %.1f %% ; Voltage: %.3f V\n", temp, humi, voltage);
    datad = true;
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
    datad = false;
    Serial.println("Stablishing communications with temp sensor:");
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
    unsigned int seconds = idleTime / 1000;
    delay(idleTime);
    connected = false;
    Serial.printf("Disconnected. wait %d seconds for next connection...\n", seconds);
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
}

void loop()
{
    if (doConnect && !connected)
    {
        connected = connectToMiTemp();
    }
    if (datad)
    {
        disconnectToMiTemp();
    }
}