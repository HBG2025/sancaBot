#include <Arduino.h>
#include <ESP32Servo.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// =====================================================
// Pines de servos - SancaBot
// =====================================================
const int PIN_SERVO_LEFT  = 0;   // Servo izquierdo
const int PIN_SERVO_RIGHT = 1;   // Servo derecho

// =====================================================
// Parámetros de servos de rotación continua
// =====================================================
const int SERVO_STOP_US = 1500;
const int SERVO_MIN_US  = 1000;
const int SERVO_MAX_US  = 2000;

// Ajustes de movimiento
const int LEFT_FORWARD_US   = 1700;
const int RIGHT_FORWARD_US  = 1300;

const int LEFT_BACKWARD_US  = 1300;
const int RIGHT_BACKWARD_US = 1700;

const int LEFT_TURN_US      = 1300;
const int RIGHT_TURN_US     = 1300;

const int RIGHT_TURN_LEFT_US  = 1700;
const int LEFT_TURN_LEFT_US   = 1700;

// Seguridad: si no llega comando, detener
const unsigned long MOVEMENT_TIMEOUT_MS = 700;

// =====================================================
// BLE
// =====================================================
#define BLE_DEVICE_NAME "ESP32C3-SancaBot"

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fae-cec4c61e9afb"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

bool deviceConnected = false;
unsigned long lastCommandTime = 0;

// =====================================================
// Objetos Servo
// =====================================================
Servo servoLeft;
Servo servoRight;

// =====================================================
// Funciones de movimiento
// =====================================================
void stopRobot() {
  servoLeft.writeMicroseconds(SERVO_STOP_US);
  servoRight.writeMicroseconds(SERVO_STOP_US);
  Serial.println("STOP");
}

void moveForward() {
  servoLeft.writeMicroseconds(LEFT_FORWARD_US);
  servoRight.writeMicroseconds(RIGHT_FORWARD_US);
  Serial.println("FORWARD");
}

void moveBackward() {
  servoLeft.writeMicroseconds(LEFT_BACKWARD_US);
  servoRight.writeMicroseconds(RIGHT_BACKWARD_US);
  Serial.println("BACKWARD");
}

void turnLeft() {
  servoLeft.writeMicroseconds(LEFT_TURN_LEFT_US);
  servoRight.writeMicroseconds(RIGHT_TURN_LEFT_US);
  Serial.println("LEFT");
}

void turnRight() {
  servoLeft.writeMicroseconds(LEFT_TURN_US);
  servoRight.writeMicroseconds(RIGHT_TURN_US);
  Serial.println("RIGHT");
}

// =====================================================
// Procesamiento de comandos BLE
// =====================================================
void processCommand(String command) {
  command.trim();
  command.toUpperCase();

  if (command.length() == 0) {
    return;
  }

  char c = command.charAt(0);

  Serial.print("Comando recibido: ");
  Serial.println(c);

  lastCommandTime = millis();

  switch (c) {
    case 'F':
      moveForward();
      break;

    case 'B':
      moveBackward();
      break;

    case 'L':
      turnLeft();
      break;

    case 'R':
      turnRight();
      break;

    case 'S':
      stopRobot();
      break;

    default:
      Serial.println("Comando no reconocido");
      stopRobot();
      break;
  }
}

// =====================================================
// Callbacks BLE
// =====================================================
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Dispositivo conectado");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Dispositivo desconectado");

    stopRobot();

    delay(300);
    BLEDevice::startAdvertising();
    Serial.println("BLE anunciando nuevamente");
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = String(pCharacteristic->getValue().c_str());

    if (value.length() > 0) {
      processCommand(value);
    }
  }
};

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Iniciando SancaBot BLE...");

  // Servos
  servoLeft.setPeriodHertz(50);
  servoRight.setPeriodHertz(50);

  servoLeft.attach(PIN_SERVO_LEFT, SERVO_MIN_US, SERVO_MAX_US);
  servoRight.attach(PIN_SERVO_RIGHT, SERVO_MIN_US, SERVO_MAX_US);

  stopRobot();

  // BLE
  BLEDevice::init(BLE_DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->setCallbacks(new CommandCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("SancaBot BLE listo");

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  BLEDevice::startAdvertising();

  Serial.println("BLE listo.");
  Serial.println("Busca el dispositivo: ESP32C3-SancaBot");
  Serial.println("Comandos:");
  Serial.println("F = adelante");
  Serial.println("B = atras");
  Serial.println("L = izquierda");
  Serial.println("R = derecha");
  Serial.println("S = parar");
}

// =====================================================
// Loop
// =====================================================
void loop() {
  if (deviceConnected) {
    if ((millis() - lastCommandTime) > MOVEMENT_TIMEOUT_MS) {
      stopRobot();
      lastCommandTime = millis();
    }
  }

  delay(20);
}