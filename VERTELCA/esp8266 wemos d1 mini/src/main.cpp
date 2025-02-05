//   rtc.adjust(DateTime(2023, 1, 23, 19, 53, 0));

#include <Arduino.h>
#include <SPI.h>
#include <RTClib.h>
#include <SoftwareSerial.h> // Подключение библиотеки Software Serial
#include <ESP8266WiFi.h>
#include <GyverDBFile.h>
#include <LittleFS.h>
#include <SettingsGyver.h>
#include "net2.hpp"

GyverDBFile db(&LittleFS, "/settings.db");

SettingsGyver sett("NIKA - Вертелка", &db);

SoftwareSerial swSerial(D5, D6); // RX, TX // Назначение задействованных дискретных каналов

#define WIFI_SSID "tenevoi"
#define WIFI_PASS "dimadima"

RTC_DS3231 rtc;

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

struct Data
{
    DateTime rtcTime;
    uint32_t unixtime;
    float temp;
    bool zeropoint = false;
};

Data data;

int freq = 0;
void build(sets::Builder &b)
{
    b.DateTime("Время на устр.", &data.unixtime);
    b.LabelFloat("Температура", data.temp, 2, sets::Colors::Blue);
    if (b.Switch(kk::disableall, "Выключить"))
    {
        sett.reload();
    }
    (db[kk::disableall].toBool()) ? analogWrite(12, 0) : analogWrite(12, 1);
    if (!db[kk::disableall].toBool())
    {
        {
            sets::Group g1(b, "Управление");
            b.Switch(kk::rotation, "Вращение против часовой");
            if (b.Slider(kk::speed, "Скорость вращения", 0.1, 2, 0.1, " об/мин"))
            {
                freq = 1000 * db[kk::speed].toFloat();
                analogWriteFreq(freq);
            }
            if (b.Switch(kk::enabled, "Расписание вкл/выкл"))
                b.reload(); // перезагрузить вебморду по клику на свитч
        }
        {
            if (db[kk::enabled])
            {
                sets::Group g2(b, "Расписание");
                b.Time(kk::timeOn, "Время включения");
                b.Time(kk::timeOff, "Время выключения");
                b.Slider(kk::angle, "Угол остановки", 0, 360, 0.5, "°");
                b.Switch(kk::softstop, "Плавная остановка");
            }
        }
    }
}

// void update(sets::Updater& u) {
// }

// Создаём объект SoftwareSerial, назначая пины 5 для приема (RX) и 4 для передачи (TX)
// SoftwareSerial softSerial(5, 4);

// Создаём глобальный объект протокола, передавая ему объект softSerial
SerialProtocol protocol(swSerial);

void setup()
{

    // delay(1000);
    //  TwoWire tw;
    //  tw.setClock(200000);
    //  tw.begin();

    protocol.begin(9600); // Инициализируем SoftwareSerial для обмена данными на скорости 9600 бод
    Serial.begin(74880);
    // while (!rtc.begin())
    // {
    //     Serial.println("Couldn't find  RTC!");
    //     delay(1000);
    // }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // swSerial.begin(9600);  // Инициализация программного последовательного порта

    // ======== WIFI ========
    // STA
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint8_t tries = 20;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (!--tries)
            break;
    }
    Serial.println();
    Serial.print("Connected: ");
    Serial.println(WiFi.localIP());

    // ======== DATABASE ========
    LittleFS.begin();
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
    if (!db[kk::disableall].toBool())
        analogWriteFreq(freq);

    // ======== SETTINGS ========
    sett.config.sliderTout = 1000;
    sett.config.requestTout = 1000;
    sett.config.updateTout = 500;
    sett.begin();
    sett.onBuild(build);
    // sett.onUpdate(update);
}

void printTime()
{
    static unsigned long m = millis();
    if (millis() - m > 5000)
    {
        //data.rtcTime = rtc.now();
        data.unixtime = data.rtcTime.unixtime();
        sett.reload();
        char timeText[128];
        // sprintf(timeText, "%02d:%02d:%02d %02d/%02d/%02d", data.rtcTime.hour(), data.rtcTime.minute(), data.rtcTime.second(),
        //         data.rtcTime.day(), data.rtcTime.month(), data.rtcTime.year());
        // Serial.println(timeText);
        // Serial.println(data.unixtime);
        // Serial.println(rtc.getTemperature());
        m = millis();
    }
}


void loop()
{
    uint16_t d [] = {0, 0, 0, 0, 0, 0};



    //data.unixtime = data.rtcTime.unixtime();
    //data.temp = rtc.getTemperature();
    sett.tick();
    static auto f = millis();
    if(millis() - f  > 5000){
        Serial.print("Отправляем днанные длинной ");
        Serial.print(sizeof(d));
        Serial.println(" байт....");
        protocol.ackTimeout_ms = 3000;
        protocol.sendPacketNonBlocking((uint8_t*)d,sizeof(d),true);
        Serial.println("Отправленно.");
        f = millis();
    }

    // Вызываем метод update() объекта протокола для асинхронной обработки входящих данных,
    // проверки состояния ожидания подтверждения и управления отправкой/приёмом пакетов
    protocol.update();
    //printTime();
}