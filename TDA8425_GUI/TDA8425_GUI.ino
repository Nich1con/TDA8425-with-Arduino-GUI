#define EB_HOLD 300

/*
  Подключаем библиотеки
*/
#include <EEPROM.h> 
#include <EncButton.h>
#include <GyverOLED.h>
#include <Wire.h>

#define EE_DATA_ADDR 1 // Адрес хранения настроек
#define EE_KEY_ADDR 0  // Адрес Хранения ключа ЕЕПРОМ
#define EE_KEY 0xBE    // Ключ

GyverOLED<SSD1306_128x64> oled(0x3C); // Создаем обьекты
EncButton<EB_CALLBACK, 2, 3, 4> enc;

// Структура с переменными - параметрами звука
struct {               
  byte volume = 0;
  byte treble = 0;
  byte bass = 0;
  bool input = false;
  bool mute = false;
} data;

bool selected = false; // Флаг для меню
byte menu_pointer = 0; // Указатель меню

bool save_requested = false; // Флаг запроса ЕЕПРОМ
uint32_t request_time = 0;   // Время последнего запроса

#define INPUT1_NAME "AUX INPUT" // Названия для селектора
#define INPUT2_NAME "BLUETOOTH"

void setup() {
  Wire.begin();				
  Wire.setClock(100000);
  
  oled.init();
  oled.clear();
  oled.home();
  oled.update();
  
  enc.attach(LEFT_HANDLER, encHandler_left);    // Аттачим коллбеки для энкодера
  enc.attach(RIGHT_HANDLER, encHandler_right);
  enc.attach(CLICK_HANDLER, encHandler_clicks);
  enc.attach(HOLDED_HANDLER, encHandler_hold);

  if (EEPROM.read(EE_KEY_ADDR) != EE_KEY) {     // Если ключ в еепром не совпадает 
    EEPROM.put(EE_DATA_ADDR, data);				// Это - первый запуск, записываем дефолт
    EEPROM.write(EE_KEY_ADDR, EE_KEY);			// Пишем ключ
  }

  EEPROM.get(EE_DATA_ADDR, data);				// Читаем еепром настройки
  TDA8425_volume(data.volume);					// Загружаем настройки
  TDA8425_treble(data.treble);
  TDA8425_bass(data.bass);
  TDA8425_control(data.input, data.mute);

  drawMenu();									// Рисуем первоначальное меню
}

void loop() {
  enc.tick();	// Тикер энкодера

  if (save_requested and millis() - request_time > 5000) { // Если запись запрошена давно (>5 сек)
    save_requested = false;				// Сбрасываем флаг
    EEPROM.put(EE_DATA_ADDR, data);		// Пишем настройки
  }
}

void update(uint8_t num) {				// Апдейт 
  switch (num) {
    case 0: TDA8425_volume(data.volume); break; // Выгружаем в TDA8425 то, что обновилось
    case 1: TDA8425_treble(data.treble); break;
    case 2: TDA8425_bass(data.bass); break;
    case 3: TDA8425_control(data.input, data.mute); break;
  }
  save_requested = true;		// Запрашиваем запись
  request_time = millis();		// Пишем время запроса
}

void encHandler_left(void) {    // Поворот влево
  uint8_t inc = (enc.isFast() ? 10 : 5); // Если поворот быстрый - меняем значение сильнее
  if (selected) {
    switch (menu_pointer) {
      case 0: data.volume = constrain(data.volume - inc, 0, 100); break; // Крутим выбраный параметр
      case 1: data.treble = constrain(data.treble - inc, 0, 100); break;
      case 2: data.bass = constrain(data.bass - inc, 0, 100); break;
      case 3: data.input = !data.input; break;
    }
    update(menu_pointer); // Выгружаем новые данные
  } else {
    menu_pointer = constrain(menu_pointer - 1, 0, 3); // Или двигаем указатель меню
  }
  drawMenu(); // Обновляем картинку
}

void encHandler_right(void) { // см. поворот влево
  uint8_t inc = (enc.isFast() ? 10 : 5);
  if (selected) {
    switch (menu_pointer) {
      case 0: data.volume = constrain(data.volume + inc, 0, 100); break;
      case 1: data.treble = constrain(data.treble + inc, 0, 100); break;
      case 2: data.bass = constrain(data.bass + inc, 0, 100); break;
      case 3: data.input = !data.input; break;
    }
    update(menu_pointer);
  } else {
    menu_pointer = constrain(menu_pointer + 1, 0, 3);
  }
  drawMenu();
}

void encHandler_clicks(void) { // клик переключает свойство меню
  selected = !selected;
  drawMenu();
}

void encHandler_hold(void) {  // удержание переключает mute
  data.mute = !data.mute;
  drawMenu();
  update(3);
}

void drawMenu(void) {
  oled.clear();    // Очистка буфера
  oled.home();
  oled.setScale(1);

  if (data.mute) {  // Вывод MITE при наличии
    oled.setCursor(50, 0);
    oled.print(F("MUTE"));
  } else {
    oled.setCursor(28, 0);
    oled.print(F("VOLUME: "));
    printValue(data.volume);
  }

  oled.setCursor(28, 2); // Вывод параметров
  oled.print(F("TREBLE: "));
  printValue(data.treble);

  oled.setCursor(34, 4);
  oled.print(F("BASS: "));
  printValue(data.bass);

  oled.setScale(2);  // Вывод входа большим шрифтом
  oled.setCursor(10, 6);
  if (data.input) oled.print(F(INPUT1_NAME));
  else oled.print(F(INPUT2_NAME));

  oled.rect(0, 8, map(data.volume, 0, 100, 0, 127), 14, OLED_FILL); // Столбики
  oled.rect(0, 24, map(data.treble, 0, 100, 0, 127), 30, OLED_FILL);
  oled.rect(0, 40, map(data.bass, 0, 100, 0, 127), 46, OLED_FILL);

  oled.line(0, 14, 127, 14); // Прочая графика - линии
  oled.line(0, 30, 127, 30);
  oled.line(0, 46, 127, 46);

  if (selected) {     // переключение указателя влево/вправо и тд
    oled.setCursor(114, menu_pointer * 2);
    oled.setScale(menu_pointer > 2 ? 2 : 1);
    oled.print(menu_pointer > 2 ? F("<") : F("<<"));
  } else {
    oled.setCursor(0, menu_pointer * 2);
    oled.setScale(menu_pointer > 2 ? 2 : 1);
    oled.print(menu_pointer > 2 ? F(">") : F(">>"));
  }

  oled.update();
}

void printValue(uint8_t data) { // Вывод числа с атрибутами min/max
  if (data > 99) oled.print(F("MAX"));
  else if (data < 1) oled.print(F("MIN"));
  else {
    oled.print(data);
    oled.print(F("%"));
  }
}


bool TDA8425_volume(uint8_t data) { // Загрузка громкости
  Wire.beginTransmission(0x41);		// Адрес TDA8425
  Wire.write(0);					// Адрес регистра
  Wire.write(0b11000000 | data >> 2); // Данные для громкости L и R
  Wire.write(0b11000000 | data >> 2); // 6 бит из 8ми
  return (bool)!Wire.endTransmission(); // Заканчиваем передачу
}

bool TDA8425_bass(uint8_t data) {  // Аналогично для тембра, но 4 бита
  Wire.beginTransmission(0x41);
  Wire.write(2);
  Wire.write(0b11110000 | data >> 4);
  return (bool)!Wire.endTransmission();
}

bool TDA8425_treble(uint8_t data) {
  Wire.beginTransmission(0x41);
  Wire.write(3);
  Wire.write(0b11110000 | data >> 4);
  return (bool)!Wire.endTransmission();
}

bool TDA8425_control(bool in, bool mute) { // Управление mute и селектором
  byte reg = 0b11001110;  // linear stereo mode
  if (in) reg |= 1 << 0;  // Если надо - селектор входов
  if (mute) reg |= 1 << 5; // И мьют
  Wire.beginTransmission(0x41); // как обычно
  Wire.write(0x08);
  Wire.write(reg);
  return (bool)!Wire.endTransmission();
}
