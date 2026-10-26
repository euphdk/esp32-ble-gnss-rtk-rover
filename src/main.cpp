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
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;
RtcmBuffer<config::kRtcmBufferSize> rtcmBuffer;

class ServerCallbacks final : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer*) override {
    portENTER_CRITICAL(&sharedMux);
    bleConnected = true;
    portEXIT_CRITICAL(&sharedMux);
  }
  void onDisconnect(BLEServer*) override {
    portENTER_CRITICAL(&sharedMux);
    bleConnected = false;
    portEXIT_CRITICAL(&sharedMux);
    BLEDevice::startAdvertising();
  }
};

class RtcmCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    const String value = characteristic->getValue();
    portENTER_CRITICAL(&sharedMux);
    if (rtcmBuffer.enqueue(reinterpret_cast<const uint8_t*>(value.c_str()), value.length()))
      rtcmBytes += value.length();
    portEXIT_CRITICAL(&sharedMux);
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
  NmeaStats stats;
  unsigned long forwardedBytes;
  bool connected;
  portENTER_CRITICAL(&sharedMux);
  stats = nmeaStats;
  forwardedBytes = rtcmBytes;
  connected = bleConnected;
  portEXIT_CRITICAL(&sharedMux);
  snprintf(status, sizeof(status), "connected=%u,fix=%lu,gga=%lu,checksum_errors=%lu,rtcm_bytes=%lu",
           connected ? 1U : 0U, stats.fixQuality, stats.ggaSentences,
           stats.checksumErrors, forwardedBytes);
  txCharacteristic->setValue(reinterpret_cast<uint8_t*>(status), strlen(status));
  if (connected) txCharacteristic->notify();
}
}

void setup() {
  Serial.begin(config::kConsoleBaud);
  gnss.begin(config::kGnssBaud, SERIAL_8N1, config::kGnssRxPin, config::kGnssTxPin);
  startBle();
  Serial.printf("GNSS BLE rover started; free heap=%lu bytes\n", static_cast<unsigned long>(ESP.getFreeHeap()));
}

void loop() {
  uint8_t chunk[config::kRtcmChunkSize];
  size_t chunkLength;
  portENTER_CRITICAL(&sharedMux);
  chunkLength = rtcmBuffer.drain(sizeof(chunk), chunk);
  portEXIT_CRITICAL(&sharedMux);
  if (chunkLength > 0) gnss.write(chunk, chunkLength);
  while (gnss.available() > 0) {
    const int value = gnss.read();
    if (value >= 0) {
      char byte = static_cast<char>(value);
      portENTER_CRITICAL(&sharedMux);
      consumeNmeaByte(byte, nmeaStats);
      const bool connected = bleConnected;
      portEXIT_CRITICAL(&sharedMux);
      if (connected && txCharacteristic != nullptr) {
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
