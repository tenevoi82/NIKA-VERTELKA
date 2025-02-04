//   rtc.adjust(DateTime(2023, 1, 23, 19, 53, 0));

#include <Arduino.h>
#include <SPI.h>
// #include <Wire.h>
#include <RTClib.h>
#include <SoftwareSerial.h>  // Подключение библиотеки Software Serial
SoftwareSerial swSerial(D5, D6);  // RX, TX // Назначение задействованных дискретных каналов

#define WIFI_SSID "tenevoi"
#define WIFI_PASS "dimadima"

bool zeroPosition = false;
//TwoWire tw;
RTC_DS3231 rtc;

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <GyverDBFile.h>
#include <LittleFS.h>
GyverDBFile db(&LittleFS, "/settings.db");

#include <SettingsGyver.h>
SettingsGyver sett("NIKA - Вертелка", &db);

DB_KEYS(
    kk,
    disableall,
    timeOn,
    timeOff,
    speed,
    angle,
    enabled,
    rotation,
    softstop);

struct Data {
    DateTime rtcTime;
    uint32_t unixtime;
    float temp;
    bool zeropoint = false;
};

Data data;

int freq = 0;
void build(sets::Builder& b) {
    b.DateTime("Время на устр.", &data.unixtime);
    b.LabelFloat("Температура",data.temp,2, sets::Colors::Blue);
    if (b.Switch(kk::disableall, "Выключить")) {
        sett.reload();
    }
    (db[kk::disableall].toBool()) ? analogWrite(12, 0) : analogWrite(12, 1);
    if (!db[kk::disableall].toBool()) {
        {
            sets::Group g1(b, "Управление");
            b.Switch(kk::rotation, "Вращение против часовой");
            if (b.Slider(kk::speed, "Скорость вращения", 0.1, 2, 0.1, " об/мин")) {
                freq = 1000 * db[kk::speed].toFloat();
                analogWriteFreq(freq);
            }
            if (b.Switch(kk::enabled, "Расписание вкл/выкл"))
                b.reload();  // перезагрузить вебморду по клику на свитч
        }
        {
            if (db[kk::enabled]) {
                sets::Group g2(b, "Расписание");
                b.Time(kk::timeOn, "Время включения");
                b.Time(kk::timeOff, "Время выключения");
                b.Slider(kk::angle, "Угол остановки", 0, 360, 0.5, "°");
                b.Switch(kk::softstop, "Плавная остановка");
            }
        }
    }
}

void update(sets::Updater& u) {
}

void setup() {
    //delay(1000);
    // TwoWire tw;
    // tw.setClock(200000);
    // tw.begin();
    
    
    Serial.begin(74880);
        // initializing the rtc
        //delay(1000);
        digitalWrite(D1,HIGH);
        delay(10);
        digitalWrite(D1,LOW);
        delay(10);
        
    while(!rtc.begin()) {
        Serial.println("Couldn't find RTC!");
        delay(1000);
    }
    swSerial.begin(9600);  // Инициализация программного последовательного порта
    // pinMode(14, INPUT_PULLUP);
    // analogWriteResolution(4);
    // pinMode(12, OUTPUT);
    // pinMode(13, OUTPUT);

    // ======== WIFI ========
    // STA
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint8_t tries = 20;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (!--tries) break;
    }
    Serial.println();
    Serial.print("Connected: ");
    Serial.println(WiFi.localIP());

    // ======== DATABASE ========
#ifdef ESP32
    LittleFS.begin(true);
#else
    LittleFS.begin();
#endif
    db.begin();
    db.init(kk::disableall, false);
    db.init(kk::timeOn, 60);
    db.init(kk::timeOff, 60);
    db.init(kk::speed, (float)1);
    db.init(kk::angle, 180);
    db.init(kk::enabled, false);
    db.init(kk::rotation, false);
    db.init(kk::softstop, true);
    db.dump(Serial);
    freq = 1000 * db[kk::speed].toFloat();
    if (!db[kk::disableall].toBool()) analogWriteFreq(freq);

    // ======== SETTINGS ========
    sett.config.sliderTout = 1000;
    sett.config.requestTout = 1000;
    sett.config.updateTout = 500;
    sett.begin();
    sett.onBuild(build);
    sett.onUpdate(update);
}

void printTime() {
    static unsigned long m = millis();
    if (millis() - m > 5000) {
        data.rtcTime = rtc.now();
        data.unixtime = data.rtcTime.unixtime();
        sett.reload();
        char timeText[128];
        sprintf(timeText, "%02d:%02d:%02d %02d/%02d/%02d", data.rtcTime.hour(), data.rtcTime.minute(), data.rtcTime.second(),
                data.rtcTime.day(), data.rtcTime.month(), data.rtcTime.year());
        Serial.println(timeText);
        Serial.println(data.unixtime);
        Serial.println(rtc.getTemperature());
        m = millis();
    }
}

#pragma pack(push, 1)
struct SerialData {
    uint8_t magic = '$';
    uint16_t Size;
    uint16_t data[6] = {0, 0, 0, 0, 0, 0};
    uint8_t crc;
};
#pragma pack(pop)

// функция для расчёта crc
uint8_t crc8_bytes(uint8_t* buffer, uint8_t size) {
    byte crc = 0;
    for (byte i = 0; i < size; i++) {
        byte data = buffer[i];
        for (int j = 8; j > 0; j--) {
            crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
            data >>= 1;
        }
    }
    return crc;
};

int checkMB(byte* d, int s) {
    for (int i = 0; i < s; i++) {
        if (d[i] == '$')
            return i;
    }
    return -1;
}

void loop() {
    SerialData d;

    data.unixtime = data.rtcTime.unixtime();
    data.temp =rtc.getTemperature();
    sett.tick();
    printTime();
    if (millis() % 1000 == 0) {
        // char timeText[128];
        //  int speed = 1000 * db[kk::speed].toFloat();
        //  sprintf(timeText, "%f , frec = %d", db[kk::speed].toFloat(), speed);
        //  analogWriteFreq(speed);
        //  Serial.println(timeText);
    }
    byte buffer[128] = {
        0,
    };
    int size;
    size = Serial.available();
    while (size > 0) {
        Serial.read(buffer, size);
        swSerial.write(buffer, size);
        size = Serial.available();
    }
    size = swSerial.available();
    while (size > 0) {
        swSerial.read(buffer, size);
        if (checkMB(buffer, size) != -1) {
            Serial.print("Got MB on pozition");
            Serial.println(checkMB(buffer, size));
        }
        Serial.write(buffer, size);
        size = swSerial.available();
    }
}