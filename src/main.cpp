#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp_mac.h>
#if defined(CONFIG_NIMBLE_ENABLED)
#include <os/os_mbuf.h>
#endif

#include "ble_uart.h"
#include "ble_name.h"
#include "config.h"
#include "nmea.h"
#include "oled.h"
#include "rover_status.h"
#include "rtcm.h"

namespace {
HardwareSerial gnss(1);
BLECharacteristic* txCharacteristic = nullptr;
NmeaStats nmeaStats;
bool haveGga = false;
uint32_t lastGgaMs = 0;
uint32_t lastGstMs = 0;
bool bleConnected = false;
unsigned long rtcmBytes = 0;
uint32_t lastCorrectionMs = 0;
unsigned long lastStatusMs = 0;
unsigned long gnssBytes = 0;
unsigned long lastGnssMs = 0;
unsigned long bleNmeaNotifyCalls = 0;
unsigned long lastNmeaNotifyMs = 0;
bool lastNotifyAccepted = false;
TaskHandle_t notifyTask = nullptr;
unsigned long bleNotifyErrors = 0;
uint32_t lastBleNotifyError = 0;
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
    if (value.length() > 0 &&
        rtcmBuffer.enqueue(reinterpret_cast<const uint8_t*>(value.c_str()), value.length())) {
      rtcmBytes += value.length();
      lastCorrectionMs = millis();
    }
    portEXIT_CRITICAL(&sharedMux);
  }
};

class TxCallbacks final : public BLECharacteristicCallbacks {
 public:
  void onStatus(BLECharacteristic*, Status status, uint32_t code) override {
    // Only the synchronous callback reports whether notify() accepted this chunk.
    if (xTaskGetCurrentTaskHandle() != notifyTask) return;
    lastNotifyAccepted = status == Status::SUCCESS_NOTIFY;
    if (!lastNotifyAccepted) {
      ++bleNotifyErrors;
      lastBleNotifyError = code;
    }
  }
};

void startBle() {
  uint8_t mac[6];
  ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_BT));
  char deviceName[kBleDeviceNameSize];
  formatBleDeviceName(deviceName, sizeof(deviceName), mac);
  BLEDevice::init(deviceName);
  if (config::kSerialDebug) Serial.printf("[ble] name=%s\n", deviceName);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService* service = server->createService(config::kServiceUuid);
  BLECharacteristic* rx = service->createCharacteristic(
      config::kUartRxUuid, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(new RtcmCallbacks());
  txCharacteristic = service->createCharacteristic(
      config::kUartTxUuid, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  txCharacteristic->setCallbacks(new TxCallbacks());
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
    Serial.printf("[ble tx] errors=%lu,last_code=%lu\n", bleNotifyErrors,
                  static_cast<unsigned long>(lastBleNotifyError));
#if defined(CONFIG_NIMBLE_ENABLED)
    Serial.printf("[ble buffers] free=%d,heap=%lu\n", os_msys_num_free(),
                  static_cast<unsigned long>(ESP.getFreeHeap()));
#endif
    if (gnssBytes == 0) {
      Serial.printf("[gnss] No UART bytes received. Check power, GND, TX1 -> GPIO%d, and baud rate.\n",
                    config::kGnssRxPin);
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
#if ARDUINO_USB_CDC_ON_BOOT && ARDUINO_USB_MODE
  // An attached USB host may stop reading; diagnostics must not block forwarding.
  Serial.setTxTimeoutMs(0);
#endif
  notifyTask = xTaskGetCurrentTaskHandle();
  gnss.setRxBufferSize(4096);
  gnss.begin(config::kGnssBaud, SERIAL_8N1, config::kGnssRxPin, config::kGnssTxPin);
  if (config::kSerialDebug) {
    Serial.printf("[gnss] receiver=%s,UART1 baud=%lu,8N1,rx_gpio=%d,tx_gpio=%d\n",
                  config::kReceiverName,
                  static_cast<unsigned long>(config::kGnssBaud),
                  config::kGnssRxPin, config::kGnssTxPin);
  }
  auto waitForGnss = [](uint32_t ms) {
    const uint32_t start = millis();
    while (millis() - start < ms) {
      while (gnss.available() > 0) {
        const int byte = gnss.read();
        if (byte >= 0 && config::kSerialDebug) Serial.write(static_cast<uint8_t>(byte));
      }
      delay(1);
    }
  };
  const bool configured = configureGnss(gnss, waitForGnss);
  if (config::kLc29hDa) {
    waitForGnss(250);
    Serial.println(configured ? "[gnss] Startup commands sent (not acknowledged)"
                              : "[gnss] Startup command write failed. Check receiver configuration.");
  }
  startBle();
#ifdef ENABLE_OLED
  startOled();
#endif
  Serial.printf("GNSS BLE rover started; free heap=%lu bytes\n", static_cast<unsigned long>(ESP.getFreeHeap()));
}

void loop() {
  auto notifyNmea = [](const uint8_t* data, size_t length) {
    if (millis() - lastNmeaNotifyMs < 5) return false;
    lastNotifyAccepted = false;
    txCharacteristic->setValue(data, length);
    txCharacteristic->notify();
    lastNmeaNotifyMs = millis();
    if (config::kSerialDebug) ++bleNmeaNotifyCalls;
    return lastNotifyAccepted;
  };
  uint8_t chunk[config::kRtcmChunkSize];
  size_t chunkLength;
  portENTER_CRITICAL(&sharedMux);
  chunkLength = rtcmBuffer.drain(sizeof(chunk), chunk);
  portEXIT_CRITICAL(&sharedMux);
  if (chunkLength > 0) gnss.write(chunk, chunkLength);
  while (gnss.available() > 0) {
    if (nmeaBatch.full() &&
        (millis() - lastNmeaNotifyMs < 5 || !nmeaBatch.flush(notifyNmea))) break;
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
      const unsigned long previousGstSentences = nmeaStats.gstSentences;
      if (consumeNmeaByte(byte, nmeaStats)) {
        haveGga = true;
        lastGgaMs = millis();
      }
      if (nmeaStats.gstSentences != previousGstSentences) lastGstMs = millis();
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
#ifdef ENABLE_OLED
  const uint32_t correctionMs = lastCorrectionMs;
#endif
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
#ifdef ENABLE_OLED
    const bool staleCorrections = correctionDataStale(correctionMs, now, config::kCorrectionStaleMs);
    updateOled(fixStatusLabel(haveGga, lastGgaMs, now, nmeaStats.fixQuality,
                              staleCorrections), connected,
               gstAccuracyFresh(nmeaStats.gstSentences > 0, lastGstMs, now),
               nmeaStats.horizontalAccuracy, nmeaStats.verticalAccuracy);
#endif
  }
  delay(1);
}
