#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "ble_uart.h"
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
unsigned long gnssBytes = 0;
unsigned long lastGnssMs = 0;
unsigned long bleNmeaNotifyCalls = 0;
unsigned long lastNmeaNotifyMs = 0;
BleUartBatch nmeaBatch;
uint8_t gnssSample[64];
size_t gnssSampleLength = 0;
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
  if (config::kSerialDebug) {
    Serial.printf("[debug] %s,nmea=%lu,gnss_rx_bytes=%lu,ble_nmea_notify_calls=%lu\n",
                  status, stats.sentences, gnssBytes, bleNmeaNotifyCalls);
    if (gnssBytes == 0) {
      Serial.println("[gnss] No UART bytes received. Check power, GND, TX1 -> GPIO16, and baud rate.");
    } else {
      Serial.printf("[gnss] last_rx_age_ms=%lu,sample_bytes=%u\n",
                    millis() - lastGnssMs, static_cast<unsigned int>(gnssSampleLength));
      if (gnssSampleLength > 0) {
        Serial.print("[gnss hex] ");
        for (size_t i = 0; i < gnssSampleLength; ++i) Serial.printf("%02X ", gnssSample[i]);
        Serial.print("\n[gnss text] ");
        for (size_t i = 0; i < gnssSampleLength; ++i) {
          const uint8_t byte = gnssSample[i];
          Serial.write(byte >= 32 && byte <= 126 ? byte : '.');
        }
        Serial.println();
      }
    }
    gnssSampleLength = 0;
  }
}
}

void setup() {
  Serial.begin(config::kConsoleBaud);
  gnss.begin(config::kGnssBaud, SERIAL_8N1, config::kGnssRxPin, config::kGnssTxPin);
  if (config::kSerialDebug) {
    Serial.printf("[gnss] UART1 baud=%lu,8N1,rx_gpio=%d,tx_gpio=%d\n",
                  static_cast<unsigned long>(config::kGnssBaud),
                  config::kGnssRxPin, config::kGnssTxPin);
  }
  startBle();
  Serial.printf("GNSS BLE rover started; free heap=%lu bytes\n", static_cast<unsigned long>(ESP.getFreeHeap()));
}

void loop() {
  auto notifyNmea = [](const uint8_t* data, size_t length) {
    txCharacteristic->setValue(data, length);
    txCharacteristic->notify();
    lastNmeaNotifyMs = millis();
    if (config::kSerialDebug) ++bleNmeaNotifyCalls;
  };
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
      if (config::kSerialDebug) {
        ++gnssBytes;
        lastGnssMs = millis();
        if (gnssSampleLength < sizeof(gnssSample)) {
          gnssSample[gnssSampleLength++] = static_cast<uint8_t>(value);
        }
      }
      portENTER_CRITICAL(&sharedMux);
      consumeNmeaByte(byte, nmeaStats);
      const bool connected = bleConnected;
      portEXIT_CRITICAL(&sharedMux);
      if (connected && txCharacteristic != nullptr) {
        nmeaBatch.append(static_cast<uint8_t>(value), notifyNmea);
      } else {
        nmeaBatch.clear();
      }
    }
  }
  portENTER_CRITICAL(&sharedMux);
  const bool connected = bleConnected;
  portEXIT_CRITICAL(&sharedMux);
  const unsigned long now = millis();
  if (!connected) {
    nmeaBatch.clear();
  } else if (now - lastNmeaNotifyMs >= 20) {
    nmeaBatch.flush(notifyNmea);
  }
  if (now - lastStatusMs >= 1000) {
    lastStatusMs = now;
    publishStatus();
  }
  delay(1);
}
