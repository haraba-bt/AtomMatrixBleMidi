#define M5STACK_MPU6886

#include "M5Atom.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
test


// Bluetooth device name
#define DEVICE_NAME "ATOM_Matrix"

// BLE MIDI GATT service and characteristic
#define MIDI_SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define MIDI_CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

// BLE GATT server
BLEServer *pServer;

// BLE GATT characteristic
BLECharacteristic *pCharacteristic;

// BLE connection state
bool isConnected = false;

// Send MIDI Packet
uint8_t midiPacket[] = {
  0x80,  // header
  0x80,  // timestamp
  0x00,  // status
  0x3c,  // 60
  0x00   // velocity
};

extern const unsigned char AtomImageData[375 + 2];

uint8_t DisBuff[2 + 5 * 5 * 3];

void setBuff(uint8_t Rdata, uint8_t Gdata, uint8_t Bdata)
{
  DisBuff[0] = 0x05;
  DisBuff[1] = 0x05;
  for (int i = 0; i < 25; i++)
  {
    DisBuff[2 + i * 3 + 0] = Rdata;
    DisBuff[2 + i * 3 + 1] = Gdata;
    DisBuff[2 + i * 3 + 2] = Bdata;
  }
}

// BLE GATT Serveri callbacks
class cbServer: public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
      isConnected = true;
      M5.dis.drawpix(0, 0x00f000);
    };

    void onDisconnect(BLEServer *pServer) {
      isConnected = false;
      M5.dis.drawpix(0, 0xf00000);
    }
};

void setup() {
  // Initialize M5Stack
  M5.begin(true, false, true);
  delay(10);
  setBuff(0x00, 0x00, 0x00);
  M5.dis.displaybuff(DisBuff);

  // BLE Initialize
  BLEDevice::init(DEVICE_NAME);

  // Create BLE GATT server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new cbServer());
  BLEDevice::setEncryptionLevel((esp_ble_sec_act_t)ESP_LE_AUTH_REQ_SC_BOND);

  // Create BLE GATT service
  BLEService* pService = pServer->createService(BLEUUID(MIDI_SERVICE_UUID));

  // Create BLE GATT characteristic
  pCharacteristic = pService->createCharacteristic(
                      BLEUUID(MIDI_CHARACTERISTIC_UUID),
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE_NR
                    );
  pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  // Set CCCD
  pCharacteristic->addDescriptor(new BLE2902());

  // Start BLE GATT service
  pService->start();

  // Start BLE advertising
  pServer->getAdvertising()->addServiceUUID(MIDI_SERVICE_UUID);
  pServer->getAdvertising()->start();

  M5.dis.drawpix(0, 0xf00000);
}

void loop() {
  M5.update();

  if (!isConnected) return;

  if (M5.Btn.wasPressed()) {
    midiPacket[2] = 0x90; // Note down
    midiPacket[3] = 0x3C; //
    midiPacket[4] = 127;  // Velocity
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
    setBuff(0xff, 0x00, 0x00);
    M5.dis.displaybuff(DisBuff);
  }

  if (M5.Btn.wasReleased()) {
    midiPacket[2] = 0x80; // Note up
    midiPacket[3] = 0x3C; //
    midiPacket[4] = 0;    // Velocity
    pCharacteristic->setValue(midiPacket, 5);
    pCharacteristic->notify();
    setBuff(0x00, 0x00, 0x00);
    M5.dis.displaybuff(DisBuff);
  }
}
