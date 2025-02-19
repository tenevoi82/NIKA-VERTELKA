//   rtc.adjust(DateTime(2023, 1, 23, 19, 53, 0));

#include <Arduino.h>
#include <SPI.h>
#include <RTClib.h>
#include <SoftwareSerial.h> // Подключение библиотеки Software Serial
#include <ESP8266WiFi.h>
#include <GyverDBFile.h>
#include <LittleFS.h>
#include <SettingsGyver.h>
#include "net.hpp"

#include "web.hpp"
#include "commands.hpp"

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

Commands motor(protocol);


void wifi_setup(){
    Serial.println(
        "========== НАСТРАИВАЕМ WIFI ==========\n\n\n");

    Serial.println(
        "========== РЕЖИМ КЛИЕНТ WIFI ==========");    
    // ======== WIFI ========
    // STA
    WiFi.mode(WIFI_STA);
    WiFi.begin(db[kk::wifissid].c_str(), db[kk::wifipass].c_str());
    WiFi.printDiag(Serial);
    uint8_t tries = 20;
    Serial.println();
    Serial.print("Попытка подключения к wifi сети ");
    Serial.print(db[kk::wifissid].c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (!--tries)
            break;
    }
    if (tries > 0)
    {
        Serial.println("Успех!");
        WiFi.printDiag(Serial);
        Serial.print("Полученный IP: ");
        Serial.println(WiFi.localIP());    
        Serial.println(
            "=======================================");  
    } else{
        Serial.println("Неудачно.");
        Serial.println(
            "========== РЕЖИМ ТОЧКИ ДОСТУПА ========");         
        Serial.println("Создаём свою сеть NIKA VERTELCA");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("NIKA VERTELCA","",6,0,2);
        WiFi.printDiag(Serial);
        IPAddress local_IP(192,168,1,1);
        IPAddress gateway(192,168,1,1);
        IPAddress subnet(255,255,255,0);
        WiFi.softAPConfig(local_IP, gateway, subnet);

        IPAddress apIP = WiFi.softAPIP();
        delay(100);
        Serial.print("AP IP address is: ");
        Serial.println(apIP);        
        Serial.println(
        "======================================="); 

    }
}


void setup()
{
    // питание SoftSerial & Rtc
    pinMode(D5, OUTPUT);
    digitalWrite(D5, HIGH);

    protocol.begin(9600); // Инициализируем SoftwareSerial для обмена данными на скорости 9600 бод
    Serial.begin(74880);

    delay(1000);
    while (!rtc.begin())
    {
        Serial.println("Couldn't find  RTC!");
        digitalWrite(D5, LOW);
        delay(1000);
        digitalWrite(D5, HIGH);
        delay(1000);
    }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

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
    db.init(kk::wifissid, "VERTELKA");
    db.init(kk::wifipass, "123123123");
    // db.dump(Serial);

    wifi_setup();


    freq = 1000 * db[kk::speed].toFloat();

    // ======== SETTINGS ========
    // sett.config.sliderTout = 1000;
    // sett.config.requestTout = 1000;
    // sett.config.updateTout = 500;
    
    // установить инфо о проекте (отображается на вкладке настроек и файлов)
    // void setProjectInfo(const char* name, const char* link = nullptr);
    sett.setProjectInfo("NIKA VERTELKA","http://google.com");
    sett.begin();
    sett.onBuild(build);
    sett.onUpdate(web_update);
}

void loop()
{
    uint16_t d[] = {0, 0, 0, 0, 0, 0};

    // data.unixtime = data.rtcTime.unixtime();
    data.temp = rtc.getTemperature();
    sett.tick();
    static auto f = millis();
    if (millis() - f > 2000)
    {
        //motor.setSpeed(20);
        f = millis();
    }

    // Вызываем метод update() объекта протокола для асинхронной обработки входящих данных,
    // проверки состояния ожидания подтверждения и управления отправкой/приёмом пакетов
    protocol.update();
}