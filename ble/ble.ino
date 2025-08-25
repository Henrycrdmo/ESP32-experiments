// CODE MADE BY HENRY MORAIS

#include <BLEDevice.h>   // Core BLE functions: init, device setup
#include <BLEServer.h>   // Allows the ESP32 to act as a GATT server
#include <BLEUtils.h>    // Utility functions for BLE, like working with UUIDs
#include <BLE2902.h>     // Descriptor type needed so clients can enable "Notify"

// ----- Global variables and objects ----- 
BLECharacteristic *pCharacteristicTX;   // Pointer to the TX characteristic (ESP32 → Phone, Notify)
BLECharacteristic *pCharacteristicRX;   // Pointer to the RX characteristic (Phone → ESP32, Write)
bool deviceConnected = false;           // Tracks whether a client is connected
int txValue = 0;                        // Example variable for data (not strictly used here)

// ----- UUIDs for service and characteristics ----- 
// These UUIDs follow the "Nordic UART Service (NUS)" convention, commonly used for BLE UART examples.
#define SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART-like service
#define CHARAC_UUID_TX    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // TX characteristic (Notify)
#define CHARAC_UUID_RX    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // RX characteristic (Write)

// ----- Server callbacks class ----- 
// Called automatically when a client (phone) connects/disconnects
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;   // Set flag when phone connects
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;  // Clear flag when phone disconnects
  }
};

// ----- Characteristic callbacks class ----- 
// Handles events on the RX characteristic (when phone sends data to ESP32)
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // getValue() returns an Arduino String in ESP32 Arduino core v3.x
    String rxValue = pCharacteristic->getValue();  

    if (rxValue.length() > 0) {
      // Print what we received from the phone
      Serial.print("Received from phone: ");
      Serial.println(rxValue);

      // Optional: echo it back to the phone using TX Notify
      pCharacteristicTX->setValue(rxValue);  // Load value into TX characteristic
      pCharacteristicTX->notify();           // Send Notify event to phone
    }
  }
};

void setup(){
  // Start USB serial output at 115200 baud (make sure Serial Monitor matches this)
  Serial.begin(115200);

  // Initialise the BLE stack with a device name (shows up in LightBlue as "ESP32-C3")
  BLEDevice::init("ESP32-C3");

  // Create the BLE server and attach the connection callbacks
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create a new BLE service (UART-like)
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // ---- TX Characteristic (ESP32 → Phone, Notify only) ----
  pCharacteristicTX = pService->createCharacteristic(
    CHARAC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY   // Can only notify
  );
  pCharacteristicTX->addDescriptor(new BLE2902()); // Allows client to subscribe to notifications

  // ---- RX Characteristic (Phone → ESP32, Write only) ----
  pCharacteristicRX = pService->createCharacteristic(
    CHARAC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE    // Client can write to it
  );
  pCharacteristicRX->setCallbacks(new MyCallbacks()); // Attach our write handler

  // Start the service so it becomes visible
  pService->start();

  // Start advertising so the phone can find this ESP32-C3
  pServer->getAdvertising()->start();
  Serial.println("Waiting for client connection...");
}

void loop(){
  // Only run if a BLE client is connected
  if (deviceConnected) {
    // Check if something was typed into the Serial Monitor
    if (Serial.available()) {
      // Read input up to newline (blocking until timeout or newline)
      String msg = Serial.readStringUntil('\n');

      // Send the message to the phone via TX Notify
      pCharacteristicTX->setValue(msg.c_str()); // Load into characteristic
      pCharacteristicTX->notify();              // Send Notify
      Serial.println("Sent to phone: " + msg);  // Confirm locally
    }
  }
}
