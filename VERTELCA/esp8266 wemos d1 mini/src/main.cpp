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

#include "web.hpp"

extern GyverDBFile db;

extern SettingsGyver sett;


#define WIFI_SSID "tenevoi25"
#define WIFI_PASS "dimadima"

RTC_DS3231 rtc;






int freq = 0;


// Создаём объект SoftwareSerial, назначая пины D6 для приема (RX) и D7 для передачи (TX)
SoftwareSerial swSerial(D6, D7); // RX, TX // Назначение задействованных дискретных каналов

// Создаём глобальный объект протокола, передавая ему объект swSerial
SerialProtocol protocol(swSerial);



void setup()
{
    //питание SoftSerial & Rtc
    pinMode(D5,OUTPUT);
    digitalWrite(D5,HIGH);


    protocol.begin(9600); // Инициализируем SoftwareSerial для обмена данными на скорости 9600 бод
    Serial.begin(74880);


    while (!rtc.begin())
    {
        Serial.println("Couldn't find  RTC!");
        delay(1000);
    }
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

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

    // ======== SETTINGS ========
    sett.config.sliderTout = 1000;
    sett.config.requestTout = 1000;
    sett.config.updateTout = 500;
    sett.begin();
    sett.onBuild(build);
    sett.onUpdate(web_update);
}

void printTime()
{
    static unsigned long m = millis();
    if (millis() - m > 5000)
    {
        //data.rtcTime = rtc.now();
        
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
    data.temp = rtc.getTemperature();
    sett.tick();
    static auto f = millis();
    if(millis() - f  > 2000){
        Serial.print("Отправляем днанные длинной ");
        Serial.print(sizeof(d));
        Serial.println(" байт....");
        protocol.sendPacketNonBlocking((uint8_t*)d,sizeof(d),true);
        Serial.println("Отправленно.");
        f = millis();
    }

    // Вызываем метод update() объекта протокола для асинхронной обработки входящих данных,
    // проверки состояния ожидания подтверждения и управления отправкой/приёмом пакетов
    protocol.update();
    //printTime();
}