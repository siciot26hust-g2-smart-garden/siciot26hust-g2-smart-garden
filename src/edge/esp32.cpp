#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Preferences.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// SMART GARDEN: Pi AUTO chính, ESP32 AUTO dự phòng.
// Chỉ loop() được sửa trạng thái actuator. Network task chỉ trao đổi qua queue.
// Đọc README.md trước khi nối Node-RED/Web: command cần session + sequence.
#define WIFI_SSID "Samsung_campus_wifi"
#define WIFI_PASSWORD "bachkhoa@123"
#define MQTT_BROKER "10.218.85.76"
#define MQTT_PORT 1883
#define CLIENT_ID "ESP32_Garden"
// Có thể điền tài khoản broker. ACL phải phân quyền Pi và Web (xem README).
#define MQTT_USER ""
#define MQTT_PASSWORD ""

// Giữ nguyên các ngưỡng/cách kiểm tra bơm mà nhóm yêu cầu.
constexpr float WARNING_LEVEL = 10.0f;
constexpr float EMPTY_LEVEL = 13.0f;
constexpr uint32_t MAX_PUMP_TIME = 15000;
constexpr float SOIL_START = 30.0f;
constexpr float SOIL_STOP = 60.0f;
constexpr int SOIL_DRY_VALUE = 3200;
constexpr int SOIL_WET_VALUE = 1400;
constexpr int SERVO_CLOSE_ANGLE = 0;
constexpr int SERVO_OPEN_ANGLE = 180;

constexpr uint32_t ACTIVE_PUBLISH_MS = 3000;
constexpr uint32_t IDLE_PUBLISH_MS = 180000;
constexpr uint32_t PI_STOP_WAIT_MS = ACTIVE_PUBLISH_MS * 5 / 2; // 7.5 s
constexpr uint32_t PI_ACTIVE_LOSS_MS = 30000;
constexpr uint32_t PI_IDLE_LOSS_MS = PI_ACTIVE_LOSS_MS * 2;
constexpr uint32_t PI_RECOVERY_MS = 10000;
constexpr uint32_t HEARTBEAT_MAX_GAP_MS = 10000; // Pi gửi mỗi 5 s
constexpr uint32_t SENSOR_MS = 1000;
constexpr uint32_t DHT_MS = 3000;
constexpr uint32_t ACTIVE_WATER_MS = 1000;
constexpr uint32_t IDLE_WATER_MS = 180000;
constexpr uint32_t SONAR_GAP_MS = 70;
constexpr uint32_t WATER_FRESH_MS = 1500;
constexpr uint32_t COMMAND_MAX_AGE_MS = 5000;

#define DHT_PIN 4
#define DHT_TYPE DHT11
#define SOIL_PIN 34
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RELAY_PUMP 25
#define SERVO_PIN 27

enum ServoState { SERVO_CLOSE, SERVO_OPEN };
enum WaterState { WATER_UNKNOWN, WATER_NORMAL, WATER_WARNING, WATER_EMPTY };
enum Controller { WAIT_PI, PI_MODE, ESP32_FAILOVER };
enum StartSource { START_NONE, START_MANUAL, START_PI, START_LOCAL };

struct ConnectionEvent { bool connected; char session[17]; uint32_t at; };
struct Command {
    char topic[64]; char payload[96]; char session[17]; uint32_t at;
};
struct StateSnapshot {
    char session[17]; uint32_t revision; bool pump; bool autoEnabled;
    bool fault; ServoState servo; WaterState water; char mode[24];
    char reason[64];
};
struct TelemetrySnapshot {
    char session[17]; uint32_t seq; uint32_t at;
    float temp; float hum; float soil; float water;
    uint32_t soilAge; uint32_t dhtAge; uint32_t waterAge;
};
struct Message { char session[17]; char topic[64]; char payload[192]; };

// Explicit prototypes keep Arduino's sketch preprocessing compatible with custom types.
bool elapsed(uint32_t now, uint32_t then, uint32_t period);
const char* waterName(WaterState value);
const char* modeName();
void queueMessage(const char* topic, const char* payload);
void publishAlert(const char* reason);
void stopPump();
void tripPump(const char* reason);
void triggerServo(bool open);

void enterFailover(const char* reason);
WaterState getWaterState(float distance);
void setWater(float distance, uint32_t now);
float getDistance();
float median(float* samples, uint8_t count);
void startSonarBurst(uint32_t now);
bool startSourceAllowed(StartSource source);
void finishPumpStart(uint32_t now);
void requestPumpStart(StartSource source, uint32_t now);
void checkPumpOperation(float waterLevel);
void serviceSonar(uint32_t now);
void observeSoil(float value, uint32_t now);
void readSensors(uint32_t now);
void serviceControl(uint32_t now);
bool parseUint(const char* text, uint32_t& out);
void ack(const char* topic, uint32_t seq, const char* result);
void handleCommand(const Command& command, uint32_t now);
void drainNetwork(uint32_t now);
void publishSnapshots(uint32_t now);
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool sendState(PubSubClient& mqtt, const StateSnapshot& s);
void numberOrNull(float value, char* buffer, size_t size);
bool sendTelemetry(PubSubClient& mqtt, const TelemetrySnapshot& t);
void networkTask(void*);

QueueHandle_t connectionQueue, commandQueue, stateQueue, telemetryQueue, messageQueue;
DHT dht(DHT_PIN, DHT_TYPE);
Preferences preferences;

// Tất cả biến dưới đây thuộc control loop, không dùng trực tiếp ở network task.
bool isPumping = false;
uint32_t pumpStartTime = 0;
ServoState servoState = SERVO_CLOSE;
WaterState waterState = WATER_UNKNOWN;
Controller controller = WAIT_PI;
bool autoEnabled = true;
bool pumpFault = false;
char faultReason[64] = "NONE";
char session[17] = "";
bool brokerConnected = false;
bool stateDirty = true, telemetryDue = true;
uint32_t bootAt = 0, piWaitSince = 0, stateRevision = 0, telemetrySeq = 0;
uint32_t lastTelemetryAt = 0, lastIssuedTelemetryAt = 0;
uint32_t lastSensorAt = 0, lastDhtAt = 0, lastWaterAt = 0;
float soilPercent = NAN, temperature = NAN, humidity = NAN, waterDistance = -1;
bool soilRead = false, dhtRead = false, waterRead = false;
float distanceHistory[3] = {};
int distanceCount = 0;

bool heartbeatSeen = false;
uint32_t lastHeartbeatAt = 0, heartbeatCounter = 0;
uint32_t stableSince = 0;
uint8_t stableHeartbeatCount = 0;
// Monotonic sequence riêng cho từng topic; tránh lệnh cũ/duplicate.
uint32_t lastCommandSeq[7] = {};
bool wetTimerActive = false;
uint32_t wetSince = 0;
uint8_t wetSamples = 0;
StartSource pendingStart = START_NONE;
bool sonarBurst = false;
uint8_t sonarCount = 0, validSonarCount = 0;
float sonarSamples[5] = {};
uint32_t lastPingAt = 0;

bool elapsed(uint32_t now, uint32_t then, uint32_t period) {
    return uint32_t(now - then) >= period; // millis wraparound
}

const char* waterName(WaterState value) {
    switch (value) {
        case WATER_NORMAL: return "NORMAL";
        case WATER_WARNING: return "WARNING";
        case WATER_EMPTY: return "EMPTY";
        default: return "UNKNOWN";
    }
}

const char* modeName() {
    if (!autoEnabled) return "MANUAL";
    if (controller == PI_MODE) return "PI";
    if (controller == ESP32_FAILOVER) return "ESP32_FAILOVER";
    return "PI_WAIT";
}

void queueMessage(const char* topic, const char* payload) {
    Message m = {};
    snprintf(m.session, sizeof(m.session), "%s", session);
    snprintf(m.topic, sizeof(m.topic), "%s", topic);
    snprintf(m.payload, sizeof(m.payload), "%s", payload);
    if (brokerConnected && xQueueSend(messageQueue, &m, 0) != pdTRUE)
        Serial.println("MQTT event queue full; local control continues.");
}

void publishAlert(const char* reason) {
    Serial.print("[ALERT] "); Serial.println(reason);
    queueMessage("garden/alert", reason);
}

void stopPump() {
    pendingStart = START_NONE;
    if (isPumping) {
        digitalWrite(RELAY_PUMP, HIGH);
        isPumping = false;
        stateDirty = true;
        telemetryDue = true;
        Serial.println(">>> PUMP OFF");
    }
    distanceCount = 0;
    wetSamples = 0;
    wetTimerActive = false;
}

void tripPump(const char* reason) {
    stopPump();
    pumpFault = true; // khóa restart đến khi người dùng reset rõ ràng
    snprintf(faultReason, sizeof(faultReason), "%s", reason);
    stateDirty = true;
    publishAlert(reason);
}

uint32_t currentServoDuty = 1638;
uint32_t targetServoDuty = 1638;
uint32_t lastServoMove = 0;

void triggerServo(bool open) {
    ServoState next = open ? SERVO_OPEN : SERVO_CLOSE;
    if (servoState == next) return;
    targetServoDuty = open ? 7864 : 1638;
    servoState = next;
    stateDirty = true;
    Serial.println(open ? ">>> CURTAIN OPEN" : ">>> CURTAIN CLOSE");
}


void updateServo(uint32_t now) {
    if (currentServoDuty != targetServoDuty) {
        if (now - lastServoMove > 15) {
            if (currentServoDuty < targetServoDuty) {
                currentServoDuty += 50;
                if (currentServoDuty > targetServoDuty) currentServoDuty = targetServoDuty;
            } else {
                currentServoDuty -= 50;
                if (currentServoDuty < targetServoDuty) currentServoDuty = targetServoDuty;
            }
            ledcWrite(2, currentServoDuty);
            lastServoMove = now;
        }
    }
}
void enterFailover(const char* reason) {
    controller = ESP32_FAILOVER;
    // Heartbeat vẫn còn cũng không được tự trả quyền khi control timeout.
    stableHeartbeatCount = 0;
    stableSince = millis();
    pendingStart = START_NONE;
    wetTimerActive = false;
    wetSamples = 0;
    stateDirty = true;
    telemetryDue = true;
    publishAlert(reason);
}

WaterState getWaterState(float distance) {
    if (distance < 0) return WATER_UNKNOWN;
    if (distance >= EMPTY_LEVEL) return WATER_EMPTY;
    if (distance >= WARNING_LEVEL) return WATER_WARNING;
    return WATER_NORMAL;
}

void setWater(float distance, uint32_t now) {
    WaterState next = getWaterState(distance);
    waterDistance = distance;
    lastWaterAt = now;
    waterRead = true;
    if (next != waterState) {
        waterState = next;
        stateDirty = true;
        if (next == WATER_WARNING) publishAlert("WATER_LOW");
    }
}

float getDistance() {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    // Bounded wait 30 ms, không chờ giây/đợi Wi-Fi trong control loop.
    uint32_t duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (!duration) return -1;
    float distance = duration * 0.0343f / 2.0f;
    return (distance > 0 && distance <= 400) ? distance : -1;
}

float median(float* samples, uint8_t count) {
    for (uint8_t i = 1; i < count; ++i) {
        float value = samples[i];
        int j = i - 1;
        while (j >= 0 && samples[j] > value) {
            samples[j + 1] = samples[j]; --j;
        }
        samples[j + 1] = value;
    }
    if (!count) return -1;
    return count % 2 ? samples[count / 2] :
        (samples[count / 2 - 1] + samples[count / 2]) / 2;
}

void startSonarBurst(uint32_t now) {
    if (sonarBurst) return;
    sonarBurst = true;
    sonarCount = validSonarCount = 0;
    lastPingAt = now - SONAR_GAP_MS;
}

bool startSourceAllowed(StartSource source) {
    if (source == START_MANUAL) return !autoEnabled;
    if (source == START_PI) return autoEnabled && controller == PI_MODE && brokerConnected;
    if (source == START_LOCAL) return autoEnabled && controller == ESP32_FAILOVER;
    return false;
}

void finishPumpStart(uint32_t now) {
    if (pendingStart == START_NONE) return;
    StartSource requested = pendingStart;
    pendingStart = START_NONE;
    if (!startSourceAllowed(requested) || pumpFault ||
        waterState == WATER_UNKNOWN || waterState == WATER_EMPTY) {
        stateDirty = true;
        publishAlert("PUMP_START_BLOCKED");
        return;
    }
    if (requested != START_MANUAL && (!isfinite(soilPercent) || soilPercent >= SOIL_STOP)) {
        publishAlert("PUMP_START_BLOCKED_SOIL");
        return;
    }
    digitalWrite(RELAY_PUMP, LOW);
        isPumping = true;
    pumpStartTime = now;
    distanceCount = 0;
    wetTimerActive = false; wetSamples = 0;
    stateDirty = telemetryDue = true;
    Serial.println(">>> PUMP ON");
}

void requestPumpStart(StartSource source, uint32_t now) {
    if (isPumping || pendingStart != START_NONE) return; // không reset MAX_PUMP_TIME
    if (pumpFault) { publishAlert("PUMP_FAULT_LOCKED"); stateDirty = true; return; }
    pendingStart = source;
    if (waterRead && !elapsed(now, lastWaterAt, WATER_FRESH_MS) && !sonarBurst)
        finishPumpStart(now);
    else startSonarBurst(now); // kiểm tra nước mới trước bật relay
}

// Giữ nguyên thuật toán 3 mẫu liên tiếp chênh lệch <= 1 cm.
// Thêm khóa lỗi để AUTO không bật lại bơm ngay sau khi bị ngắt bảo vệ.
void checkPumpOperation(float waterLevel) {
    if (!isPumping || waterLevel < 0) return;
    distanceHistory[0] = distanceHistory[1];
    distanceHistory[1] = distanceHistory[2];
    distanceHistory[2] = waterLevel;
    if (distanceCount < 3) ++distanceCount;
    /* if (distanceCount >= 3 && fabsf(distanceHistory[0] - distanceHistory[1]) <= 1.0f &&
        fabsf(distanceHistory[1] - distanceHistory[2]) <= 1.0f)
        tripPump("PUMP_NO_WATER_CHANGE"); */
}

void serviceSonar(uint32_t now) {
    uint32_t period = isPumping ? ACTIVE_WATER_MS : IDLE_WATER_MS;
    if (!sonarBurst && (!waterRead || pendingStart != START_NONE || elapsed(now, lastWaterAt, period)))
        startSonarBurst(now);
    if (!sonarBurst || !elapsed(now, lastPingAt, SONAR_GAP_MS)) return;
    float raw = getDistance();
    now = millis(); // refresh sau pulseIn
    lastPingAt = now;
    ++sonarCount;
    if (raw >= 0) sonarSamples[validSonarCount++] = raw;
    // Không để median che dấu một mẫu cạn/lỗi khi bơm đang chạy.
    if (isPumping && (raw < 0 || raw >= EMPTY_LEVEL)) {
        setWater(raw, now);
        tripPump(raw < 0 ? "WATER_SENSOR_ERROR" : "WATER_EMPTY");
    }
    if (sonarCount < 5) return;
    sonarBurst = false;
    // Ít nhất 3 mẫu hợp lệ trong 5; bỏ -1, dùng median của mẫu hợp lệ.
    setWater(validSonarCount >= 3 ? median(sonarSamples, validSonarCount) : -1, now);
    if (isPumping) {
        if (waterState == WATER_UNKNOWN) tripPump("WATER_SENSOR_ERROR");
        else if (waterState == WATER_EMPTY) tripPump("WATER_EMPTY");
        else checkPumpOperation(waterDistance);
    }
    finishPumpStart(now);
}

void observeSoil(float value, uint32_t now) {
    soilPercent = value;
    soilRead = true; lastSensorAt = now;
    if (autoEnabled && controller != ESP32_FAILOVER && isPumping && isfinite(value) && value >= SOIL_STOP) {
        if (wetSamples < 2) ++wetSamples;
        if (wetSamples >= 2 && !wetTimerActive) {
            wetTimerActive = true;
            wetSince = now;
            telemetryDue = true; // báo ngay khi xác nhận ngưỡng, không chờ slot 3 s
        }
    } else if (!wetTimerActive) wetSamples = 0;
    // Sau khi timer đã bắt đầu, không reset vì một mẫu nhiễu rơi xuống ngưỡng.
}

void readSensors(uint32_t now) {
    if (!soilRead || elapsed(now, lastSensorAt, SENSOR_MS)) {
        int raw = analogRead(SOIL_PIN);
        float value = float(SOIL_DRY_VALUE - raw) / (SOIL_DRY_VALUE - SOIL_WET_VALUE) * 100;
        observeSoil(constrain(value, 0.0f, 100.0f), now);
    }
    if (!dhtRead || elapsed(now, lastDhtAt, DHT_MS)) {
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
        dhtRead = true; lastDhtAt = millis();
    }
}

void serviceControl(uint32_t now) {
    // Bảo vệ này chạy cả AUTO và MANUAL, không phụ thuộc MQTT/heartbeat.
    if (isPumping && elapsed(now, pumpStartTime, MAX_PUMP_TIME)) tripPump("MAX_PUMP_TIME");
    if (autoEnabled && controller != ESP32_FAILOVER && isPumping && wetTimerActive &&
        elapsed(now, wetSince, PI_STOP_WAIT_MS)) {
        stopPump();
        enterFailover("PUMP_CONTROL_TIMEOUT");
    }
    if (!autoEnabled) return;
    uint32_t timeout = isPumping ? PI_ACTIVE_LOSS_MS : PI_IDLE_LOSS_MS;
    if (controller != ESP32_FAILOVER &&
        (elapsed(now, heartbeatSeen ? lastHeartbeatAt : bootAt, timeout) ||
         (controller == WAIT_PI && elapsed(now, piWaitSince, timeout))))
        enterFailover("PI_HEARTBEAT_TIMEOUT");
    if (controller != ESP32_FAILOVER) return;
    if (isfinite(soilPercent)) {
        if (isPumping && soilPercent >= SOIL_STOP) stopPump();
        else if (!isPumping && !pumpFault && soilPercent < SOIL_START)
            requestPumpStart(START_LOCAL, now);
    }
    if (isfinite(temperature)) {
        if (temperature > 32) triggerServo(true);
        else if (temperature < 30) triggerServo(false);
    }
}

// Không cho vượt ngưỡng 32-bit / dấu / whitespace / garbage.
bool parseUint(const char* text, uint32_t& out) {
    if (!text || !*text) return false;
    uint64_t value = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        value = value * 10 + (*p - '0');
        if (value > UINT32_MAX) return false;
    }
    out = uint32_t(value); return true;
}

void ack(const char* topic, uint32_t seq, const char* result) {
    char payload[192];
    snprintf(payload, sizeof(payload),
        "{\"topic\":\"%s\",\"seq\":%lu,\"result\":\"%s\",\"pump\":\"%s\"}",
        topic, (unsigned long)seq, result, isPumping ? "ON" : "OFF");
    queueMessage("garden/state/ack", payload);
    stateDirty = true;
}

void handleCommand(const Command& command, uint32_t now) {
    if (!brokerConnected || strcmp(command.session, session) != 0 ||
        elapsed(now, command.at, COMMAND_MAX_AGE_MS)) return;
    char data[96]; snprintf(data, sizeof(data), "%s", command.payload);
    char* first = strchr(data, '|');
    if (!first) { ack(command.topic, 0, "INVALID_FORMAT"); return; }
    *first++ = '\0';
    if (strcmp(data, session) != 0) { ack(command.topic, 0, "STALE_SESSION"); return; }
    char* value = strchr(first, '|');
    if (value) *value++ = '\0';
    uint32_t sequence;
    if (!parseUint(first, sequence) || !sequence) { ack(command.topic, 0, "INVALID_SEQUENCE"); return; }

    if (!strcmp(command.topic, "garden/pi/heartbeat")) {
        if (value || sequence <= heartbeatCounter) return;
        if (!heartbeatSeen || elapsed(command.at, lastHeartbeatAt, HEARTBEAT_MAX_GAP_MS + 1) ||
            stableHeartbeatCount == 0) {
            stableSince = command.at; stableHeartbeatCount = 1;
        } else if (stableHeartbeatCount < 255) ++stableHeartbeatCount;
        heartbeatSeen = true; heartbeatCounter = sequence; lastHeartbeatAt = command.at;
        return;
    }
    if (!value || !*value || strchr(value, '|')) { ack(command.topic, sequence, "INVALID_FORMAT"); return; }
    int index = -1;
    if (!strcmp(command.topic, "garden/control/auto")) index = 0;
    else if (!strcmp(command.topic, "garden/control/pump")) index = 1;
    else if (!strcmp(command.topic, "garden/control/servo")) index = 2;
    else if (!strcmp(command.topic, "garden/pi/pump")) index = 3;
    else if (!strcmp(command.topic, "garden/pi/servo")) index = 4;
    else if (!strcmp(command.topic, "garden/pi/ready")) index = 5;
    else if (!strcmp(command.topic, "garden/control/reset")) index = 6;
    if (index < 0) return;
    if (sequence <= lastCommandSeq[index]) { ack(command.topic, sequence, "DUPLICATE"); return; }
    lastCommandSeq[index] = sequence;

    if (!strcmp(command.topic, "garden/control/auto")) {
        if (strcmp(value, "ON") && strcmp(value, "OFF")) { ack(command.topic, sequence, "INVALID_VALUE"); return; }
        bool enable = !strcmp(value, "ON");
        if (enable != autoEnabled) {
            stopPump(); // đổi chủ điều khiển không để bơm cũ chạy tiếp
            autoEnabled = enable;
            preferences.putBool("auto", enable);
            controller = WAIT_PI;
            piWaitSince = now;
            stableHeartbeatCount = 0;
            if (enable) { bootAt = now; heartbeatSeen = false; }
            stateDirty = telemetryDue = true;
        }
        ack(command.topic, sequence, "APPLIED"); return;
    }
    if (!strcmp(command.topic, "garden/control/reset")) {
        if (strcmp(value, "RESET")) { ack(command.topic, sequence, "INVALID_VALUE"); return; }
        if (autoEnabled || isPumping) { ack(command.topic, sequence, "MANUAL_OFF_REQUIRED"); return; }
        pumpFault = false; snprintf(faultReason, sizeof(faultReason), "NONE");
        ack(command.topic, sequence, "APPLIED"); return;
    }
    if (!strcmp(command.topic, "garden/pi/ready")) {
        uint32_t confirmedTelemetry;
        if (!parseUint(value, confirmedTelemetry) || !telemetrySeq || confirmedTelemetry != telemetrySeq ||
            elapsed(now, lastIssuedTelemetryAt, HEARTBEAT_MAX_GAP_MS + 1) || !heartbeatSeen ||
            elapsed(now, lastHeartbeatAt, HEARTBEAT_MAX_GAP_MS + 1) || stableHeartbeatCount < 3 ||
            !elapsed(now, stableSince, PI_RECOVERY_MS)) {
            // Gửi mẫu mới để Pi có thể thử lại ready với seq hiện tại.
            telemetryDue = true;
            ack(command.topic, sequence, "NOT_READY"); return;
        }
        if (!autoEnabled) { ack(command.topic, sequence, "AUTO_OFF"); return; }
        controller = PI_MODE; pendingStart = START_NONE;
        // Không xóa timer đang chờ Pi OFF nếu ready bị gửi lại khi PI đã nắm quyền.
        ack(command.topic, sequence, "PI_CONTROL_GRANTED"); return;
    }
    bool piCommand = index == 3 || index == 4;
    bool pumpCommand = index == 1 || index == 3;
    if ((pumpCommand && strcmp(value, "ON") && strcmp(value, "OFF")) ||
        (!pumpCommand && strcmp(value, "OPEN") && strcmp(value, "CLOSE"))) {
        ack(command.topic, sequence, "INVALID_VALUE"); return;
    }
    if (piCommand && (!autoEnabled || controller != PI_MODE)) { ack(command.topic, sequence, "PI_NOT_OWNER"); return; }
    if (!piCommand && autoEnabled) { ack(command.topic, sequence, "AUTO_ON_MANUAL_BLOCKED"); return; }
    if (pumpCommand) {
        if (!strcmp(value, "OFF")) { stopPump(); ack(command.topic, sequence, "APPLIED"); }
        else if (pumpFault) ack(command.topic, sequence, "PUMP_FAULT_LOCKED");
        else if (piCommand && (!isfinite(soilPercent) || soilPercent >= SOIL_STOP))
            ack(command.topic, sequence, "SOIL_STOP_THRESHOLD");
        else {
            requestPumpStart(piCommand ? START_PI : START_MANUAL, now);
            ack(command.topic, sequence, isPumping ? "APPLIED" : "CHECK_STATE");
        }
    } else {
        triggerServo(!strcmp(value, "OPEN")); ack(command.topic, sequence, "APPLIED");
    }
}

void drainNetwork(uint32_t now) {
    ConnectionEvent event;
    if (xQueueReceive(connectionQueue, &event, 0) == pdTRUE) {
        bool changedSession = strcmp(session, event.session) != 0;
        brokerConnected = event.connected;
        if (changedSession) {
            snprintf(session, sizeof(session), "%s", event.session);
            // Giữ timestamp heartbeat cũ để reconnect không reset đồng hồ mất Pi.
            heartbeatCounter = 0;
            stableHeartbeatCount = 0;
            memset(lastCommandSeq, 0, sizeof(lastCommandSeq));
            // Reconnect không tự trao lại quyền cho Pi và không xóa timer 7.5 s.
            if (controller == PI_MODE) { controller = WAIT_PI; piWaitSince = now; }
            pendingStart = START_NONE;
        }
        stateDirty = telemetryDue = true;
    }
    Command command;
    for (int i = 0; i < 16 && xQueueReceive(commandQueue, &command, 0) == pdTRUE; ++i)
        handleCommand(command, millis());
    (void)now;
}

void publishSnapshots(uint32_t now) {
    if (stateDirty) {
        StateSnapshot s = {};
        snprintf(s.session, sizeof(s.session), "%s", session);
        s.revision = ++stateRevision;
        s.pump = isPumping; s.autoEnabled = autoEnabled; s.servo = servoState;
        s.water = waterState; s.fault = pumpFault;
        snprintf(s.mode, sizeof(s.mode), "%s", modeName());
        snprintf(s.reason, sizeof(s.reason), "%s", faultReason);
        xQueueOverwrite(stateQueue, &s); stateDirty = false;
    }
    uint32_t period = isPumping ? ACTIVE_PUBLISH_MS : IDLE_PUBLISH_MS;
    if (telemetryDue || elapsed(now, lastTelemetryAt, period)) {
        TelemetrySnapshot t = {};
        snprintf(t.session, sizeof(t.session), "%s", session);
        t.seq = ++telemetrySeq; t.at = now;
        t.temp = temperature; t.hum = humidity; t.soil = soilPercent; t.water = waterDistance;
        t.soilAge = soilRead ? uint32_t(now - lastSensorAt) : UINT32_MAX;
        t.dhtAge = dhtRead ? uint32_t(now - lastDhtAt) : UINT32_MAX;
        t.waterAge = waterRead ? uint32_t(now - lastWaterAt) : UINT32_MAX;
        xQueueOverwrite(telemetryQueue, &t);
        lastIssuedTelemetryAt = lastTelemetryAt = now; telemetryDue = false;
    }
}

// ==== NETWORK TASK: độc quyền WiFiClient/PubSubClient; không viết relay/servo ====
char networkSession[17] = ""; // chỉ network task sử dụng
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (strlen(topic) >= sizeof(Command{}.topic) || length >= sizeof(Command{}.payload)) return;
    if (memchr(payload, '\0', length)) return;
    Command c = {};
    snprintf(c.topic, sizeof(c.topic), "%s", topic);
    memcpy(c.payload, payload, length); c.payload[length] = '\0';
    snprintf(c.session, sizeof(c.session), "%s", networkSession);
    c.at = millis();
    xQueueSend(commandQueue, &c, 0); // queue full => bỏ command, control fail-safe vẫn chạy
}

bool sendState(PubSubClient& mqtt, const StateSnapshot& s) {
    bool ok = true;
    ok &= mqtt.publish("garden/state/pump", s.pump ? "ON" : "OFF", true);
    ok &= mqtt.publish("garden/state/servo", s.servo == SERVO_OPEN ? "OPEN" : "CLOSE", true);
    ok &= mqtt.publish("garden/state/auto", s.autoEnabled ? "ON" : "OFF", true);
    ok &= mqtt.publish("garden/state/mode", s.mode, true);
    ok &= mqtt.publish("garden/state/water", waterName(s.water), true);
    ok &= mqtt.publish("garden/state/pump_fault", s.fault ? s.reason : "NONE", true);
    // Retry cùng snapshot nếu publish session/online ban đầu bị lỗi ghi socket.
    ok &= mqtt.publish("garden/state/session", s.session, true);
    ok &= mqtt.publish("garden/status", "online", true);
    return ok;
}

void numberOrNull(float value, char* buffer, size_t size) {
    if (isfinite(value)) snprintf(buffer, size, "%.2f", value);
    else snprintf(buffer, size, "null");
}

bool sendTelemetry(PubSubClient& mqtt, const TelemetrySnapshot& t) {
    char temp[24], hum[24], soil[24], water[24];
    numberOrNull(t.temp, temp, sizeof(temp)); numberOrNull(t.hum, hum, sizeof(hum));
    numberOrNull(t.soil, soil, sizeof(soil));
    numberOrNull(t.water >= 0 ? t.water : NAN, water, sizeof(water));
    bool ok = true;
    // Từng cảm biến độc lập: lỗi DHT không chặn soil/water.
    ok &= mqtt.publish("garden/sensor/temp", isfinite(t.temp) ? temp : "ERROR");
    ok &= mqtt.publish("garden/sensor/hum", isfinite(t.hum) ? hum : "ERROR");
    ok &= mqtt.publish("garden/sensor/soil", isfinite(t.soil) ? soil : "ERROR");
    ok &= mqtt.publish("garden/sensor/water", t.water >= 0 ? water : "-1");
    char json[512];
    snprintf(json, sizeof(json),
        "{\"session\":\"%s\",\"seq\":%lu,\"uptime_ms\":%lu,\"temp\":%s,\"hum\":%s,"
        "\"soil\":%s,\"water\":%s,\"soil_age_ms\":%lu,\"dht_age_ms\":%lu,\"water_age_ms\":%lu}",
        t.session, (unsigned long)t.seq, (unsigned long)t.at, temp, hum, soil, water,
        (unsigned long)t.soilAge, (unsigned long)t.dhtAge, (unsigned long)t.waterAge);
    ok &= mqtt.publish("garden/telemetry", json); // không retained, dùng seq này cho ready
    return ok;
}

void networkTask(void*) {
    WiFiClient client;
    PubSubClient mqtt(client);
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setSocketTimeout(1); mqtt.setKeepAlive(15); mqtt.setBufferSize(768);
    WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // không vòng while chờ Wi-Fi
    uint32_t wifiAttempt = millis(), mqttAttempt = millis() - 5000;
    uint32_t lastStateRevision = 0, lastTelemetrySeq = 0;
    bool reportedConnected = false;
    for (;;) {
        uint32_t now = millis();
        bool newConnection = false;
        if (WiFi.status() != WL_CONNECTED) {
            if (mqtt.connected()) client.stop();
            if (elapsed(now, wifiAttempt, 15000)) { wifiAttempt = now; WiFi.reconnect(); }
        } else if (!mqtt.connected() && elapsed(now, mqttAttempt, 5000)) {
            mqttAttempt = now;
            // connect() có thể block ở task mạng, control loop vẫn được schedule.
            if (mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                             "garden/status", 1, true, "offline")) {
                snprintf(networkSession, sizeof(networkSession), "%08lx%08lx",
                    (unsigned long)esp_random(), (unsigned long)esp_random());
                mqtt.publish("garden/status", "online", true);
                mqtt.publish("garden/state/session", networkSession, true);
                bool subscribed = mqtt.subscribe("garden/control/#", 1);
                subscribed &= mqtt.subscribe("garden/pi/#", 1);
                if (!subscribed) client.stop();
                else newConnection = true;
                lastStateRevision = lastTelemetrySeq = 0;
            }
        }
        bool connected = WiFi.status() == WL_CONNECTED && mqtt.connected();
        if (connected != reportedConnected || newConnection) {
            ConnectionEvent e = {};
            e.connected = connected; e.at = millis();
            snprintf(e.session, sizeof(e.session), "%s", networkSession);
            xQueueOverwrite(connectionQueue, &e);
            reportedConnected = connected;
        }
        if (connected) {
            mqtt.loop();
            StateSnapshot s;
            if (xQueuePeek(stateQueue, &s, 0) == pdTRUE &&
                !strcmp(s.session, networkSession) && s.revision != lastStateRevision && sendState(mqtt, s))
                lastStateRevision = s.revision;
            TelemetrySnapshot t;
            if (xQueuePeek(telemetryQueue, &t, 0) == pdTRUE &&
                !strcmp(t.session, networkSession) && t.seq != lastTelemetrySeq && sendTelemetry(mqtt, t))
                lastTelemetrySeq = t.seq;
            Message message;
            for (int i = 0; i < 4 && xQueueReceive(messageQueue, &message, 0) == pdTRUE; ++i)
                if (!strcmp(message.session, networkSession)) mqtt.publish(message.topic, message.payload);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);
    digitalWrite(RELAY_PUMP, HIGH); pinMode(RELAY_PUMP, OUTPUT_OPEN_DRAIN);
    pinMode(TRIG_PIN, OUTPUT); digitalWrite(TRIG_PIN, LOW); pinMode(ECHO_PIN, INPUT);
    dht.begin();
    ledcSetup(2, 50, 16); ledcAttachPin(SERVO_PIN, 2); ledcWrite(2, 1638);
    preferences.begin("garden", false);
    autoEnabled = true; // AUTO OFF không mất khi restart
    bootAt = millis();
    piWaitSince = bootAt;
    connectionQueue = xQueueCreate(1, sizeof(ConnectionEvent));
    commandQueue = xQueueCreate(16, sizeof(Command));
    stateQueue = xQueueCreate(1, sizeof(StateSnapshot));
    telemetryQueue = xQueueCreate(1, sizeof(TelemetrySnapshot));
    messageQueue = xQueueCreate(32, sizeof(Message));
    if (!connectionQueue || !commandQueue || !stateQueue || !telemetryQueue || !messageQueue ||
        xTaskCreate(networkTask, "garden-network", 8192, nullptr, 1, nullptr) != pdPASS) {
        Serial.println("INIT FAILED: pump held OFF. Restart device.");
        while (true) { digitalWrite(RELAY_PUMP, HIGH); delay(1000); }
    }
    Serial.println("ESP32 SMART GARDEN - Pi primary / ESP32 failover");
}

void loop() {
    // Timer bảo vệ chạy trước cả sensor IO và xử lý command.
    serviceControl(millis());
    drainNetwork(millis());
    readSensors(millis());
    serviceSonar(millis());
    serviceControl(millis());
    updateServo(millis());
    publishSnapshots(millis());
    delay(2);
}
