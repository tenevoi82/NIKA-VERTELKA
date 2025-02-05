#include <Arduino.h>        // Подключаем основную библиотеку Arduino, содержащую базовые функции (setup, loop, millis и пр.)
#include <SoftwareSerial.h> // Подключаем библиотеку SoftwareSerial для создания программного UART на произвольных пинах

// Определяем константы для протокола обмена данными
#define HEADER 0xAA // Определяем заголовок пакета (начальный байт), равный 0xAA
#define ACK 0x55    // Определяем байт подтверждения (ACK), равный 0x55
#define NACK 0xFF   // Определяем байт отрицательного подтверждения (NACK), равный 0xFF

// Определяем флаг пакета: если установлен, то пакет требует подтверждения (ACK)
#define FLAG_NEEDS_ACK 0x01 // Флаг, обозначающий, что пакет требует подтверждения (1 байт флагов)

// Перечисление возможных состояний протокола
enum ProtocolState
{
    STATE_IDLE,    // Состояние ожидания: нет активной передачи, система простаивает
    STATE_WAIT_ACK // Состояние ожидания подтверждения: пакет отправлен, ждём ACK или NACK
};

// Объявляем класс SerialProtocol для организации асинхронного обмена данными
class SerialProtocol
{
public:
    // Конструктор класса, который принимает ссылку на объект SoftwareSerial
    SerialProtocol(SoftwareSerial &serial)
        : serial(serial),    // Инициализируем ссылку на SoftwareSerial
          state(STATE_IDLE), // Устанавливаем начальное состояние как IDLE (ожидание)
          ackTimeout(0),     // Инициализируем таймаут ожидания подтверждения нулевым значением
          rxIndex(0),        // Сбрасываем индекс буфера приёма (начинаем с 0)
          packetReady(false) // Устанавливаем, что пакет ещё не готов для обработки
    {
    }

    unsigned long ackTimeout_ms = 1000; // Время (в миллисекундах) до которого ожидаем подтверждения (ACK)

    // Метод begin() инициализирует SoftwareSerial на заданной скорости (baud rate)
    void begin(unsigned long baudRate)
    {
        serial.begin(baudRate); // Запускаем SoftwareSerial на указанной скорости
    }

    // Немедленная (неблокирующая) отправка пакета с возможностью установки флага, требующего подтверждения
    // data - указатель на массив с полезными данными
    // length - длина полезных данных
    // needsAck - булевское значение: true, если пакет требует подтверждения, false если не требует
    void sendPacketNonBlocking(uint8_t *data, uint8_t length, bool needsAck)
    {
        // Формируем пакет: размер пакета равен длине полезных данных + 4 байта для HEADER, LENGTH, FLAGS и CRC
        uint8_t packet[length + 4];                   // Создаём массив нужного размера для пакета
        packet[0] = HEADER;                           // Записываем заголовок пакета (HEADER) в первый байт
        packet[1] = length;                           // Записываем длину полезной нагрузки во второй байт
        packet[2] = needsAck ? FLAG_NEEDS_ACK : 0x00; // Записываем флаг: если пакет требует подтверждения, записываем FLAG_NEEDS_ACK, иначе 0x00
        packet[1] = length; // Записываем длину полезных данных во второй байт массива
        // Записываем флаг: если пакет требует подтверждения, устанавливаем FLAG_NEEDS_ACK, иначе записываем 0x00
        packet[2] = needsAck ? FLAG_NEEDS_ACK : 0x00;        
        memcpy(&packet[3], data, length);             // Копируем полезные данные в пакет, начиная с 4-го байта (индекс 3)
        // Вычисляем контрольную сумму CRC для всех байтов, кроме последнего, и записываем её в последний байт пакета
        packet[length + 3] = crc8(packet, length + 3);

        // Отправляем сформированный пакет по SoftwareSerial
        serial.write(packet, length + 4);

        // Если пакет требует подтверждения, переходим в состояние ожидания подтверждения
        if (needsAck)
        {
            state = STATE_WAIT_ACK;       // Устанавливаем состояние протокола как WAIT_ACK
            ackTimeout = millis() + ackTimeout_ms; // Устанавливаем таймаут ожидания подтверждения на ackTimeout_ms миллисекунд 
        }
        // Если пакет не требует подтверждения, состояние остаётся IDLE
    }

    // Метод update() должен вызываться в основном цикле (loop) для:
    // - Чтения входящих данных из SoftwareSerial
    // - Формирования пакета из полученных байтов
    // - Обработки полученного пакета (подтверждения или команд)
    // - Управления состоянием ожидания подтверждения (проверка таймаута)
    void update()
    {
        // Считываем все доступные байты из SoftwareSerial
        while (serial.available() > 0)
        {                                 // Пока доступны байты для чтения
            uint8_t byte = serial.read(); // Читаем один байт из SoftwareSerial
            if (rxIndex == 0 && byte != HEADER)
            {             // Если это первый байт пакета и он не равен HEADER
                continue; // Пропускаем этот байт и переходим к следующему
            }
            rxBuffer[rxIndex++] = byte; // Сохраняем полученный байт в буфер и увеличиваем индекс

            // Если получено не менее 4-х байт (минимальный размер пакета) и индекс равен ожидаемому размеру пакета:
            // Размер пакета = HEADER (1) + LENGTH (1) + FLAGS (1) + DATA (length) + CRC (1) = length + 4
            if (rxIndex > 3 && rxIndex == rxBuffer[1] + 4)
            {
                packetReady = true; // Устанавливаем флаг, что пакет полностью получен
            }
        }

        // Если получен полный пакет, переходим к его обработке
        if (packetReady)
        {
            packetReady = false;                        // Сбрасываем флаг готовности пакета, чтобы принять следующий пакет
            uint8_t length = rxBuffer[1];               // Извлекаем длину полезной нагрузки из второго байта пакета
            uint8_t flags = rxBuffer[2];                // Извлекаем байт флагов (третий байт пакета)
            uint8_t receivedCrc = rxBuffer[length + 3]; // Извлекаем полученную CRC из последнего байта пакета
            // Вычисляем контрольную сумму для полученных данных (HEADER, LENGTH, FLAGS, DATA)
            uint8_t calculatedCrc = crc8(rxBuffer, length + 3);

            // Сравниваем полученную CRC с вычисленной для проверки целостности пакета
            if (receivedCrc == calculatedCrc)
            {
                // Если текущее состояние WAIT_ACK и пакет является подтверждением (один байт полезной нагрузки равный ACK)
                if (state == STATE_WAIT_ACK && length == 1 && rxBuffer[3] == ACK)
                {
                    Serial.println("ACK received."); // Выводим сообщение, что получено подтверждение
                    state = STATE_IDLE;              // Сбрасываем состояние, так как подтверждение получено
                }
                // Если текущее состояние WAIT_ACK и пакет является подтверждением NACK (один байт полезной нагрузки равный NACK)
                else if (state == STATE_WAIT_ACK && length == 1 && rxBuffer[3] == NACK)
                {
                    Serial.println("NACK received."); // Выводим сообщение, что получено отрицательное подтверждение
                    state = STATE_IDLE;               // Сбрасываем состояние (можно добавить логику повторной отправки, если нужно)
                }
                // Если пакет не является подтверждением, проверяем наличие флага, требующего подтверждения
                else if (flags & FLAG_NEEDS_ACK)
                {
                    // Если флаг установлен, значит полученный пакет требует отправки подтверждения
                    Serial.println("Received command packet requiring ACK."); // Выводим сообщение о получении команды с требованием подтверждения
                    uint8_t ackPayload[] = {ACK};                             // Формируем полезную нагрузку для ACK (один байт, равный ACK)
                    // Отправляем подтверждение, используя асинхронную отправку (неблокирующая)
                    sendPacketNonBlocking(ackPayload, 1, false);
                }
                // Если пакет не требует подтверждения и не является ACK/NACK, обрабатываем его как обычную команду или данные
                else
                {
                    Serial.println("Received command packet."); // Выводим сообщение о получении команды без требования подтверждения
                                                                // Здесь можно добавить обработку полученной команды или данных
                }
            }
            else
            {
                // Если контрольные суммы не совпадают, выводим сообщение об ошибке CRC
                Serial.println("CRC error in received packet.");
            }
            // После обработки пакета сбрасываем индекс для приёма, чтобы начать формирование нового пакета
            rxIndex = 0;
        }

        // Если находимся в состоянии ожидания подтверждения, проверяем, не истёк ли таймаут
        if (state == STATE_WAIT_ACK && millis() > ackTimeout)
        {
            Serial.println("Timeout waiting for ACK."); // Выводим сообщение, что время ожидания подтверждения истекло
            state = STATE_IDLE;                         // Сбрасываем состояние, так как ожидание завершено
                                                        // При необходимости можно добавить логику повторной отправки пакета здесь
        }
    }

private:
    SoftwareSerial &serial;   // Ссылка на объект SoftwareSerial для обмена данными
    ProtocolState state;      // Переменная, хранящая текущее состояние протокола (IDLE или WAIT_ACK)
    unsigned long ackTimeout; // Время (в миллисекундах) до которого ожидаем подтверждения (ACK)
    uint8_t rxBuffer[32];     // Буфер для хранения входящих байтов данных
    uint8_t rxIndex;          // Индекс текущей позиции в буфере приёма
    bool packetReady;         // Флаг, указывающий, что получен полный пакет и он готов к обработке

    // Метод crc8() вычисляет CRC-8 для входящего массива данных
    // data - указатель на массив данных, len - длина данных, для которых вычисляется контрольная сумма
    uint8_t crc8(const uint8_t *data, uint8_t len)
    {
        uint8_t crc = 0; // Инициализируем переменную для CRC нулём
        while (len--)
        {                             // Пока есть байты в массиве (len уменьшается до 0)
            uint8_t inbyte = *data++; // Читаем текущий байт из массива и переходим к следующему
            for (uint8_t i = 8; i; i--)
            { // Обрабатываем каждый бит текущего байта (8 битов)
                // Вычисляем mix: младший бит результата XOR между текущим CRC и inbyte
                uint8_t mix = (crc ^ inbyte) & 0x01;
                crc >>= 1; // Сдвигаем CRC вправо на 1 бит
                if (mix)
                    crc ^= 0x8C; // Если mix равен 1, выполняем операцию XOR с полиномом 0x8C
                inbyte >>= 1;    // Сдвигаем текущий байт вправо на 1 бит
            }
        }
        return crc; // Возвращаем вычисленное значение CRC
    }
};

// // Создаём объект SoftwareSerial, назначая пины 5 для приема (RX) и 4 для передачи (TX)
// SoftwareSerial softSerial(5, 4);

// // Создаём глобальный объект протокола, передавая ему объект softSerial
// SerialProtocol protocol(softSerial);

// void setup() {
//   Serial.begin(115200);        // Инициализируем аппаратный последовательный порт для вывода отладочной информации
//   protocol.begin(9600);        // Инициализируем SoftwareSerial для обмена данными на скорости 9600 бод
// }

// void loop() {
//   // Вызываем метод update() объекта протокола для асинхронной обработки входящих данных,
//   // проверки состояния ожидания подтверждения и управления отправкой/приёмом пакетов
//   protocol.update();

//   // Здесь можно выполнять другие задачи, так как основной цикл не блокируется
// }
