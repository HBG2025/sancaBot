// =============================================================================
// SancaBot Free (V1.0)
// Plataforma : ESP32-C3 Super Mini
//
// ── HARDWARE ──────────────────────────────────────────────────────────────────
//   Servo ESQ (rotação contínua)   →  GPIO 0
//   Servo DIR (rotação contínua)   →  GPIO 1
//   Sensor IR ESQUERDO             →  GPIO 5
//   Sensor IR DIREITO              →  GPIO 6
//   Sensor Ultrassom (HC-SR04)     →  TRIG: GPIO 7  |  ECHO: GPIO 8
//
// ── PROTOCOLO BLE (Write Without Response) ───────────────────────────────────
//   P:esq:dir  → potência direta, −100 a +100 (apenas modo manual)
//   S          → parar imediatamente
//   M:0        → modo MANUAL
//   G:0        → Parar modo autônomo
//   G:1:mode   → Iniciar modo autônomo (3=Linha, 4=Obstáculo)
//   C:spd:dF:dR:turn:tE:tD: → Configuração Geral/Manual
//   L:spd:kp:inv:           → Configuração Linha
//   O:spd:dist:turnDrift:   → Configuração Obstáculo
//
// ── RESPOSTA BLE (Notify) ─────────────────────────────────────────────────────
//   A:OK                → confirmação
//   R:mode:L:R:dist:    → status periódico a 200ms (modo autônomo)
// =============================================================================

//
#include <BLEDevice.h> 
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

// ── PINOS
#define PIN_SERVO_ESQ  0
#define PIN_SERVO_DIR  1
#define PIN_IR_LEFT    5
#define PIN_IR_RIGHT   6
#define PIN_ULTRA_TRIG 7
#define PIN_ULTRA_ECHO 8

// ── CALIBRAÇÃO FÍSICA
const int HW_DRIFT  = 0;

// ── BLE
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ── MODOS
#define MODE_MANUAL    0
#define MODE_LINE      3
#define MODE_OBSTACLE  4

// ── CONFIGURAÇÕES GERAIS (C:)
int overallSpeed    = 80;
int driftFwd        = 0;
int driftRev        = 0;
int turnSpeedFactor = 70;
int trimEsq         = 1500;
int trimDir         = 1500;

// ── CONFIGURAÇÕES LINHA (L:)
int lineSpeed       = 55;
int lineKp          = 60;
int lineInvSens     = 0; // 0=HIGH na linha, 1=LOW na linha
int lineDrift       = 0; // -50 a 50

// ── CONFIGURAÇÕES OBSTÁCULO (O:)
int obsSpeed        = 55;
int obsDist         = 30; // Distância limite em cm
int obsTurnDrift    = 0;  // Tendência de curva ao desviar (-30 a 30)

// ── RUNTIME
volatile int  currentMode  = MODE_MANUAL;
volatile bool autoRunning  = false;

// ── SERVOS
Servo servoEsq, servoDir;
static int lastUsEsq    = -1,   lastUsDir    = -1;
static int currentUsEsq = 1500, currentUsDir = 1500;
int targetUsEsq         = 1500;
int targetUsDir         = 1500;

// ── WATCHDOG & TIMERS
const unsigned long STOP_TIMEOUT_MS = 500;
unsigned long lastMoveCmd = 0;
bool motorsStopped = true;

const unsigned long STATUS_INTERVAL_MS = 200;
unsigned long lastStatusMs = 0;

const unsigned long TICK_MS = 20; // 50 Hz
unsigned long lastTick = 0;

// ── BLE STATE
BLECharacteristic *pChar   = nullptr;
bool deviceConnected = false;
bool needsReconnect  = false;

// ── SENSORES
volatile int currentUltraDist = 999;

// =============================================================================
// FUNÇÕES DE SENSORES
// =============================================================================
int readIR(int pin) {
    int raw = digitalRead(pin);
    return (lineInvSens == 0) ? raw : !raw;
}

void readUltrasonic() {
    digitalWrite(PIN_ULTRA_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRA_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRA_TRIG, LOW);
    
    // Timeout de 15000us (~2.5 metros). Se não retornar, dá 0.
    long duration = pulseIn(PIN_ULTRA_ECHO, HIGH, 15000);
    if (duration == 0) {
        currentUltraDist = 999;
    } else {
        currentUltraDist = duration / 58; // Converter para cm
    }
}

// =============================================================================
// CONTROLE DOS SERVOS
// =============================================================================
static void rampMotor(int &cur, int target, int neutral) {
    const int RAMP_US = 180;
    bool crossing = (cur < neutral && target > neutral) ||
                    (cur > neutral && target < neutral);
    if (crossing) {
        if (abs(target - cur) <= RAMP_US) cur = target;
        else cur += (target > cur) ? RAMP_US : -RAMP_US;
    } else {
        cur = target;
    }
}

void setMotorSpeeds(int p_esq, int p_dir) {
    const int DEADBAND = 5;
    p_esq = constrain(p_esq + HW_DRIFT, -100, 100);
    p_dir = constrain(p_dir - HW_DRIFT, -100, 100);
    if (abs(p_esq) < DEADBAND) p_esq = 0;
    if (abs(p_dir) < DEADBAND) p_dir = 0;

    targetUsEsq = trimEsq + map(p_esq, -100, 100, -600,  600);
    targetUsDir = trimDir + map(p_dir, -100, 100,  600, -600);
}

void updateMotorsTask() {
    if (motorsStopped) return;
    
    rampMotor(currentUsEsq, targetUsEsq, trimEsq);
    rampMotor(currentUsDir, targetUsDir, trimDir);

    if (currentUsEsq != lastUsEsq) {
        servoEsq.writeMicroseconds(currentUsEsq);
        lastUsEsq = currentUsEsq;
    }
    if (currentUsDir != lastUsDir) {
        servoDir.writeMicroseconds(currentUsDir);
        lastUsDir = currentUsDir;
    }
}

void stopMotors() {
    if (!motorsStopped) {
        targetUsEsq = trimEsq; targetUsDir = trimDir;
        servoEsq.writeMicroseconds(trimEsq);
        servoDir.writeMicroseconds(trimDir);
        currentUsEsq = trimEsq; currentUsDir = trimDir;
        lastUsEsq    = trimEsq; lastUsDir    = trimDir;
        motorsStopped = true;
    }
}

void moveDirect(int e, int d) {
    lastMoveCmd   = millis();
    motorsStopped = false;
    setMotorSpeeds(e, d);
}

// =============================================================================
// BLE
// =============================================================================
void sendNotify(const char *msg) {
    if (!deviceConnected || !pChar) return;
    pChar->setValue(msg);
    pChar->notify();
}

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pC) {
        //String v = pC->getValue();
        std::string raw = pC->getValue();
        String v = String(raw.c_str());
        if (v.length() == 0) return;
        char cmd = v.charAt(0);

        // P:esq:dir
        if (cmd == 'P') {
            if (currentMode != MODE_MANUAL) return;
            int s1 = v.indexOf(':'), s2 = v.indexOf(':', s1 + 1);
            if (s1 > 0 && s2 > 0) {
                int e = v.substring(s1 + 1, s2).toInt();
                int d = v.substring(s2 + 1).toInt();
                int currentDrift = 0;
                if (abs(e + d) > 10) { // Aplica deriva linear apenas se não estiver girando no próprio eixo
                    currentDrift = ((e + d) > 0) ? driftFwd : driftRev;
                }
                
                if (currentDrift != 0) {
                    float leftMult = 1.0; float rightMult = 1.0;
                    if (currentDrift < 0) leftMult = (100.0 + currentDrift) / 100.0;
                    else rightMult = (100.0 - currentDrift) / 100.0;
                    e = round(e * leftMult);
                    d = round(d * rightMult);
                }
                moveDirect(e, d);
            }
        }
        else if (cmd == 'S') {
            lastMoveCmd = 0;
            stopMotors();
        }
        else if (cmd == 'M') {
            int col = v.indexOf(':');
            if (col > 0) {
                int newMode = v.substring(col + 1).toInt();
                currentMode = newMode; // Se 0, vai para manual. PWA lida com a restrição.
                autoRunning = false;
                lastMoveCmd = 0;
                stopMotors();
            }
        }
        else if (cmd == 'G') {
            int col1 = v.indexOf(':');
            int col2 = v.indexOf(':', col1 + 1);
            if (col1 > 0) {
                int runState = v.substring(col1 + 1, col2 > 0 ? col2 : v.length()).toInt();
                autoRunning = (runState == 1);
                if (autoRunning && col2 > 0) {
                    currentMode = v.substring(col2 + 1).toInt();
                }
                if (!autoRunning) stopMotors();
            }
        }
        else if (cmd == 'C') {
            // C:spd:dF:dR:turn:tE:tD:
            int c[6] = {0,0,0,0,0,0};
            int lastIdx = v.indexOf(':');
            for(int i=0; i<6; i++) {
                int idx = v.indexOf(':', lastIdx + 1);
                if (idx < 0) break;
                c[i] = v.substring(lastIdx + 1, idx).toInt();
                lastIdx = idx;
            }
            overallSpeed = constrain(c[0], 0, 100);
            driftFwd = constrain(c[1], -50, 50);
            driftRev = constrain(c[2], -50, 50);
            turnSpeedFactor = constrain(c[3], 10, 100);
            trimEsq = constrain(c[4], 1400, 1600);
            trimDir = constrain(c[5], 1400, 1600);
            lastUsEsq = -1; lastUsDir = -1;
            currentUsEsq = trimEsq; currentUsDir = trimDir;
            targetUsEsq = trimEsq; targetUsDir = trimDir;
            sendNotify("A:OK");
        }
        else if (cmd == 'L') {
            // L:spd:kp:inv:ldrift:
            int c[4] = {0,0,0,0};
            int lastIdx = v.indexOf(':');
            for(int i=0; i<4; i++) {
                int idx = v.indexOf(':', lastIdx + 1);
                if (idx < 0) break;
                c[i] = v.substring(lastIdx + 1, idx).toInt();
                lastIdx = idx;
            }
            lineSpeed = constrain(c[0], 0, 100);
            lineKp = constrain(c[1], 0, 100);
            lineInvSens = c[2];
            lineDrift = constrain(c[3], -50, 50);
            sendNotify("A:OK");
        }
        else if (cmd == 'O') {
            // O:spd:dist:turnDrift:
            int c[3] = {0,0,0};
            int lastIdx = v.indexOf(':');
            for(int i=0; i<3; i++) {
                int idx = v.indexOf(':', lastIdx + 1);
                if (idx < 0) break;
                c[i] = v.substring(lastIdx + 1, idx).toInt();
                lastIdx = idx;
            }
            obsSpeed = constrain(c[0], 0, 100);
            obsDist = constrain(c[1], 5, 100);
            obsTurnDrift = constrain(c[2], -50, 50);
            sendNotify("A:OK");
        }
    }
};

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer*) {
        deviceConnected = true; needsReconnect = false;
        sendNotify("A:OK");
    }
    void onDisconnect(BLEServer*) {
        deviceConnected = false; needsReconnect = true;
        currentMode = MODE_MANUAL; autoRunning = false; lastMoveCmd = 0;
    }
};

// =============================================================================
// MODOS AUTÔNOMOS
// =============================================================================
void lineFollowTick() {
    int sL = readIR(PIN_IR_LEFT);
    int sR = readIR(PIN_IR_RIGHT);

    int error = 0;
    if      ( sL && !sR) error = -2;
    else if (!sL &&  sR) error = +2;

    int correction = (lineKp * error) / 10;
    int e = constrain(lineSpeed - correction, -100, 100);
    int d = constrain(lineSpeed + correction, -100, 100);

    if (lineDrift != 0) {
        float leftMult = 1.0; float rightMult = 1.0;
        if (lineDrift < 0) leftMult = (100.0 + lineDrift) / 100.0;
        else rightMult = (100.0 - lineDrift) / 100.0;
        e = round(e * leftMult);
        d = round(d * rightMult);
    }
    motorsStopped = false;
    setMotorSpeeds(e, d);
}

// Máquina de estados do desvio de obstáculo
int obsState = 0; // 0=Avança, 1=Parar/Ré, 2=Gira
unsigned long obsStateTimer = 0;

void obstacleTick() {
    // 1. Lê a distância
    readUltrasonic();
    
    // Máquina de estados
    if (obsState == 0) { // Avança
        if (currentUltraDist <= obsDist) {
            obsState = 1; // Encontrou obstáculo, vai para ré
            obsStateTimer = millis();
        } else {
            // Segue em frente
            int e = obsSpeed;
            int d = obsSpeed;
            if (driftFwd != 0) {
                float lM = 1.0; float rM = 1.0;
                if (driftFwd < 0) lM = (100.0 + driftFwd)/100.0;
                else rM = (100.0 - driftFwd)/100.0;
                e = round(e * lM); d = round(d * rM);
            }
            motorsStopped = false;
            setMotorSpeeds(e, d);
        }
    }
    else if (obsState == 1) { // Ré leve para sair da colisão
        motorsStopped = false;
        setMotorSpeeds(-50, -50); // Dá ré
        if (millis() - obsStateTimer > 400) { // Ré por 400ms
            obsState = 2;
            obsStateTimer = millis();
        }
    }
    else if (obsState == 2) { // Girar
        motorsStopped = false;
        // Gira no eixo dependendo do turnDrift
        int e = obsTurnDrift >= 0 ? 50 : -50;
        int d = obsTurnDrift >= 0 ? -50 : 50;
        setMotorSpeeds(e, d);
        
        // Gira por 400ms ou até a frente ficar bem limpa (Distância > obsDist + 15)
        if (millis() - obsStateTimer > 450 || currentUltraDist > obsDist + 15) {
            obsState = 0; // Volta a avançar
        }
    }
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(800);
    Serial.println("[BOOT] SancaBot Free v1.0");

    pinMode(PIN_IR_LEFT, INPUT);
    pinMode(PIN_IR_RIGHT, INPUT);
    
    pinMode(PIN_ULTRA_TRIG, OUTPUT);
    pinMode(PIN_ULTRA_ECHO, INPUT);
    digitalWrite(PIN_ULTRA_TRIG, LOW);

    servoEsq.setPeriodHertz(50);
    servoEsq.attach(PIN_SERVO_ESQ);
    servoDir.setPeriodHertz(50);
    servoDir.attach(PIN_SERVO_DIR);
    servoEsq.writeMicroseconds(trimEsq);
    servoDir.writeMicroseconds(trimDir);

    uint64_t chipid = ESP.getEfuseMac();
    uint16_t shortId = (uint16_t)(chipid >> 32);
    char bleName[20];
    snprintf(bleName, sizeof(bleName), "SancaBot_%04X", shortId);
    
    Serial.print("BLE Name: ");
    Serial.println(bleName);

    BLEDevice::init(bleName);
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pChar = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ     |
        BLECharacteristic::PROPERTY_WRITE    |
        BLECharacteristic::PROPERTY_WRITE_NR |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pChar->addDescriptor(new BLE2902());
    pChar->setCallbacks(new MyCallbacks());
    pService->start();

    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true);
    pAdv->setMinPreferred(0x06);
    pAdv->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
    unsigned long now = millis();

    if (needsReconnect) {
        needsReconnect = false;
        stopMotors();
        delay(200);
        BLEDevice::startAdvertising();
    }

    if (now - lastTick >= TICK_MS) {
        lastTick = now;
        if (autoRunning && deviceConnected) {
            if (currentMode == MODE_LINE) lineFollowTick();
            else if (currentMode == MODE_OBSTACLE) obstacleTick();
        }
        updateMotorsTask();
    }

    if (currentMode == MODE_MANUAL && deviceConnected &&
        !motorsStopped && (now - lastMoveCmd > STOP_TIMEOUT_MS)) {
        stopMotors();
    }

    // Notificações Bluetooth (Apenas quando Auto está ativo)
    if (autoRunning && deviceConnected && (now - lastStatusMs >= STATUS_INTERVAL_MS)) {
        lastStatusMs = now;
        int sL = readIR(PIN_IR_LEFT);
        int sR = readIR(PIN_IR_RIGHT);
        
        // PWA espera: R:mode:L:R:dist:
        // Distância é lida durante o tick no obstáculo. Mas se estiver na linha, lemos só pra UI!
        if (currentMode == MODE_LINE) readUltrasonic(); 
        
        char buf[32];
        snprintf(buf, sizeof(buf), "R:%d:%d:%d:%d:", currentMode, sL, sR, currentUltraDist);
        sendNotify(buf);
    }

    delay(5);
}