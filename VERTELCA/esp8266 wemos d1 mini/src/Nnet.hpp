#include <Arduino.h>         // Подключаем базовые функции Arduino
#include <SoftwareSerial.h>    // Подключаем библиотеку SoftwareSerial для программного UART

// Определяем константы протокола
#define HEADER 0xAA          // Заголовок пакета: служит для идентификации начала пакета
#define ACK    0x55          // ACK (подтверждение): указывает на успешный приём
#define NACK   0xFF          // NACK (отрицательное подтверждение): указывает на ошибку (например, неверная CRC)



// Перечисление состояний протокола
enum ProtocolState {
  STATE_IDLE,       // Нет активной передачи, ожидание действий
  STATE_WAIT_ACK    // Пакет отправлен, ожидаем подтверждения (ACK или NACK)
};

// Класс SerialProtocol реализует асинхронный обмен по SoftwareSerial
class SerialProtocol {
public:
  // Конструктор принимает ссылку на SoftwareSerial и инициализирует внутренние переменные
  SerialProtocol(SoftwareSerial &serial)
    : serial(serial), state(STATE_IDLE), ackTimeout(0), rxIndex(0), packetReady(false)
  { }

  // Метод begin() инициализирует SoftwareSerial с заданной скоростью
  void begin(unsigned long baudRate) {
    serial.begin(baudRate);  // Запускаем программный последовательный порт на указанной скорости
  }

  // Метод sendPacketNonBlocking() отправляет пакет и переходит в состояние ожидания подтверждения
  void sendPacketNonBlocking(uint8_t *data, uint8_t length) {
    // Формируем пакет: [HEADER] [LENGTH] [DATA] [CRC]
    uint8_t packet[length + 3];         // Выделяем буфер для пакета
    packet[0] = HEADER;                 // Устанавливаем заголовок пакета
    packet[1] = length;                 // Записываем длину полезной нагрузки
    memcpy(&packet[2], data, length);   // Копируем сами данные в пакет
    // Вычисляем контрольную сумму CRC для всех байтов, кроме последнего, и сохраняем в конце пакета
    packet[length + 2] = crc8(packet, length + 2);

    // Отправляем сформированный пакет по SoftwareSerial
    serial.write(packet, length + 3);

    // Устанавливаем состояние протокола как "ожидание подтверждения"
    state = STATE_WAIT_ACK;
    // Запоминаем время, до которого ждем подтверждения (например, 1000 мс)
    ackTimeout = millis() + 1000;

    // (Опционально можно сохранить пакет для повторной отправки, если ACK не получен)
  }

  // Метод update() вызывается в основном цикле и выполняет:
  // - Чтение входящих данных
  // - Обработка полученных пакетов
  // - Управление состоянием ожидания подтверждения
  void update() {
    // Читаем все доступные байты из SoftwareSerial
    while (serial.available() > 0) {
      uint8_t byte = serial.read();  // Читаем один байт

      // Если это первый байт пакета и он не равен HEADER, пропускаем его
      if (rxIndex == 0 && byte != HEADER) {
        continue;  // Переходим к следующему байту
      }

      // Сохраняем байт в буфер входящих данных
      rxBuffer[rxIndex++] = byte;

      // Если получено как минимум 2 байта, то rxBuffer[1] содержит длину данных
      // Проверяем, достигнут ли полный размер пакета: HEADER (1) + LENGTH (1) + DATA + CRC (1)
      if (rxIndex > 2 && rxIndex == rxBuffer[1] + 3) {
        packetReady = true;  // Полный пакет получен, устанавливаем флаг
      }
    }

    // Если получен полный пакет, обрабатываем его
    if (packetReady) {
      packetReady = false;                  // Сбрасываем флаг, чтобы принять следующий пакет
      uint8_t payloadLength = rxBuffer[1];    // Извлекаем длину полезной нагрузки из пакета
      uint8_t receivedCrc = rxBuffer[payloadLength + 2]; // Извлекаем полученную CRC из последнего байта
      // Вычисляем контрольную сумму для принятого пакета (без CRC)
      uint8_t calculatedCrc = crc8(rxBuffer, payloadLength + 2);

      // Если вычисленная и полученная CRC совпадают, пакет корректен
      if (receivedCrc == calculatedCrc) {
        // Если пакет содержит ровно 1 байт полезной нагрузки и этот байт равен ACK...
        if (payloadLength == 1 && rxBuffer[2] == ACK) {
          // Если мы находимся в состоянии ожидания подтверждения, то подтверждение получено
          if (state == STATE_WAIT_ACK) {
            Serial.println("ACK received."); // Выводим сообщение об успешном подтверждении
            state = STATE_IDLE;              // Сбрасываем состояние – больше не ждём подтверждения
          }
        }
        // Если пакет содержит 1 байт и равен NACK, обрабатываем отрицательное подтверждение
        else if (payloadLength == 1 && rxBuffer[2] == NACK) {
          if (state == STATE_WAIT_ACK) {
            Serial.println("NACK received."); // Сообщаем об ошибке получения
            state = STATE_IDLE;               // Сбрасываем состояние (возможно, можно повторить отправку)
          }
        }
        // Если полученный пакет не является подтверждением, считаем его командой или данными
        else {
          Serial.println("Received command packet.");  // Выводим сообщение о получении команды
          // Здесь можно добавить обработку полученной команды/данных

          // После обработки команды отправляем ACK подтверждение
          // Для этого вызываем наш метод отправки, который переведёт состояние в ожидание ACK
          uint8_t ackPayload[] = { ACK };
          sendPacketNonBlocking(ackPayload, 1);
          // Если требуется отправка ответа, её также можно выполнить здесь или позже
        }
      }
      // Если CRC не совпадает – произошла ошибка при передаче
      else {
        Serial.println("CRC error in received packet.");
      }

      // Сбрасываем индекс приёма для начала приёма следующего пакета
      rxIndex = 0;
    }

    // Если мы находимся в состоянии ожидания подтверждения, проверяем, не истёк ли таймаут
    if (state == STATE_WAIT_ACK && millis() > ackTimeout) {
      // Если время ожидания вышло, сообщаем об этом и сбрасываем состояние
      Serial.println("Timeout waiting for ACK.");
      state = STATE_IDLE;
      // Здесь можно реализовать повторную отправку пакета, если это необходимо
    }
  }

private:
  SoftwareSerial &serial;  // Ссылка на объект SoftwareSerial для обмена данными
  ProtocolState state;     // Текущее состояние протокола (IDLE или WAIT_ACK)
  unsigned long ackTimeout;  // Время, до которого мы ожидаем подтверждения

  // Буфер для приёма входящих данных и связанные переменные
  uint8_t rxBuffer[32];    // Массив для хранения входящих байтов
  uint8_t rxIndex;         // Текущий индекс в буфере приёма
  bool packetReady;        // Флаг, указывающий, что получен полный пакет

  // Функция вычисления CRC-8 для проверки целостности данных
  uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;        // Инициализируем CRC нулём
    while (len--) {         // Обрабатываем каждый байт массива data
      uint8_t inbyte = *data++;   // Берём текущий байт и переходим к следующему
      for (uint8_t i = 8; i; i--) {  // Обрабатываем 8 бит этого байта
        uint8_t mix = (crc ^ inbyte) & 0x01;  // Определяем, отличаются ли младшие биты crc и inbyte
        crc >>= 1;               // Сдвигаем crc вправо на 1 бит
        if (mix) crc ^= 0x8C;    // Если mix равен 1, выполняем XOR с полиномом 0x8C
        inbyte >>= 1;            // Сдвигаем inbyte вправо на 1 бит
      }
    }
    return crc;  // Возвращаем вычисленное значение CRC
  }
};


// // Создаём объект SoftwareSerial на пинах 5 (RX) и 4 (TX)
// SoftwareSerial softSerial(5, 4);

// // Создаём глобальный объект протокола, используя ранее созданный softSerial
// SerialProtocol protocol(softSerial);

// void setup() {
//   Serial.begin(115200);  // Инициализируем аппаратный Serial для вывода отладочной информации
//   protocol.begin(9600);  // Инициализируем SoftwareSerial для обмена по протоколу на скорости 9600 бод
// }

// void loop() {
//   // Вызываем метод update() для асинхронной обработки входящих данных
//   // и управления состоянием протокола (например, проверка таймаута ожидания ACK)
//   protocol.update();

//   // Здесь можно выполнять и другие задачи, не блокируя основной цикл
// }
