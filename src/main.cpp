#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "config.h"
#include "nmea.h"
#include "rtcm.h"

namespace {
HardwareSerial gnss(1);
BLECharacteristic* txCharacteristic = nullptr;
NmeaStats nmeaStats;
bool bleConnected = false;
unsigned long rtcmBytes = 0;
unsigned long lastStatusMs = 0;

struct GnssWriter {
  size_t write(const uint8_t* bytes, size_t size) { return gnss.write(bytes, size); }
};

class ServerCallbacks final : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer*) override { bleConnected = true; }
  void onDisconnect(BLEServer*) override {
    bleConnected = false;
    BLEDevice::startAdvertising();
  }
};

class RtcmCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    const String value = characteristic->getValue();
    GnssWriter writer;
    if (forwardRtcm(reinterpret_cast<const uint8_t*>(value.c_str()), value.length(), writer)) {
      rtcmBytes += value.length();
    }
  }
};

void startBle() {
  BLEDevice::init(config::kDeviceName);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService* service = server->createService(config::kServiceUuid);
  BLECharacteristic* rx = service->createCharacteristic(
      config::kUartRxUuid, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(new RtcmCallbacks());
  txCharacteristic = service->createCharacteristic(
      config::kUartTxUuid, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  txCharacteristic->addDescriptor(new BLE2902());
  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(config::kServiceUuid);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void publishStatus() {
  char status[128];
  snprintf(status, sizeof(status), "connected=%u,fix=%lu,gga=%lu,checksum_errors=%lu,rtcm_bytes=%lu",
           bleConnected ? 1U : 0U, nmeaStats.fixQuality, nmeaStats.ggaSentences,
           nmeaStats.checksumErrors, rtcmBytes);
  txCharacteristic->setValue(reinterpret_cast<uint8_t*>(status), strlen(status));
  if (bleConnected) txCharacteristic->notify();
}
}

void setup() {
  Serial.begin(config::kConsoleBaud);
  gnss.begin(config::kGnssBaud, SERIAL_8N1, config::kGnssRxPin, config::kGnssTxPin);
  startBle();
  Serial.printf("GNSS BLE rover started; free heap=%lu bytes\n", static_cast<unsigned long>(ESP.getFreeHeap()));
}

void loop() {
  while (gnss.available() > 0) {
    const int value = gnss.read();
    if (value >= 0) {
      char byte = static_cast<char>(value);
      consumeNmeaByte(byte, nmeaStats);
      if (bleConnected && txCharacteristic != nullptr) {
        txCharacteristic->setValue(reinterpret_cast<uint8_t*>(&byte), 1);
        txCharacteristic->notify();
      }
    }
  }
  const unsigned long now = millis();
  if (now - lastStatusMs >= 1000) {
    lastStatusMs = now;
    publishStatus();
  }
  delay(1);
}
