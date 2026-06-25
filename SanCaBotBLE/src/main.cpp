// SancaBot / ESP32-C3 SuperMini
// PlatformIO + VSCode
// Control del LED embarcado por BLE
// LED embarcado: GPIO 8
//
// Nombre BLE: SancaBot_LED
//
// Comandos BLE aceptados:
//   ON   -> encender LED
//   OFF  -> apagar LED
//   1    -> encender LED
//   0    -> apagar LED
//   T    -> alternar LED
//
// Nota importante:
// En la placa usada, el LED embarcado funciona con lógica invertida:
//   LOW  -> LED encendido
//   HIGH -> LED apagado

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// -----------------------------------------------------
// LED embarcado del SancaBot / ESP32-C3 SuperMini
// -----------------------------------------------------
#define LED_PIN 8

// -----------------------------------------------------
// BLE
// -----------------------------------------------------
#define BLE_DEVICE_NAME "SancaBot_LED"

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic = nullptr;

bool deviceConnected = false;
bool ledState = false;

// -----------------------------------------------------
// Funciones del LED
// -----------------------------------------------------
void applyLedState() {
  // LED activo en bajo:
  // ledState = true  -> LOW  -> encendido
  // ledState = false -> HIGH -> apagado
  digitalWrite(LED_PIN, ledState ? LOW : HIGH);
}

void sendStatus() {
  if (!deviceConnected || pCharacteristic == nullptr) {
    return;
  }

  if (ledState) {
    pCharacteristic->setValue("LED:ON");
  } else {
    pCharacteristic->setValue("LED:OFF");
  }

  pCharacteristic->notify();
}

// -----------------------------------------------------
// Procesamiento de comandos BLE
// -----------------------------------------------------
void processCommand(String command) {
  command.trim();
  command.toUpperCase();

  Serial.print("Comando recibido: ");
  Serial.println(command);

  if (command == "ON" || command == "1") {
    ledState = true;
    applyLedState();
    Serial.println("LED encendido");
    sendStatus();
  }
  else if (command == "OFF" || command == "0") {
    ledState = false;
    applyLedState();
    Serial.println("LED apagado");
    sendStatus();
  }
  else if (command == "T") {
    ledState = !ledState;
    applyLedState();
    Serial.println("LED alternado");
    sendStatus();
  }
  else {
    Serial.println("Comando no reconocido");
  }
}

// -----------------------------------------------------
// Callbacks BLE
// -----------------------------------------------------
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Dispositivo BLE conectado");
    sendStatus();
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Dispositivo BLE desconectado");

    delay(300);
    BLEDevice::startAdvertising();
    Serial.println("Anunciando BLE nuevamente");
  }
};

class LedCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string raw = pCharacteristic->getValue();
    String value = String(raw.c_str());

    if (value.length() == 0) {
      return;
    }

    processCommand(value);
  }
};

// -----------------------------------------------------
// Setup
// -----------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("[BOOT] SancaBot LED BLE");

  pinMode(LED_PIN, OUTPUT);

  ledState = false;
  applyLedState();

  BLEDevice::init(BLE_DEVICE_NAME);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ     |
    BLECharacteristic::PROPERTY_WRITE    |
    BLECharacteristic::PROPERTY_WRITE_NR |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->setCallbacks(new LedCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("LED:OFF");

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  BLEDevice::startAdvertising();

  Serial.println("BLE listo.");
  Serial.print("Nombre BLE: ");
  Serial.println(BLE_DEVICE_NAME);
  Serial.println("Comandos: ON, OFF, 1, 0, T");
}

// -----------------------------------------------------
// Loop
// -----------------------------------------------------
void loop() {
  delay(100);
}
