#define EB_HOLD 300

#include <EEPROM.h>
#include <EncButton.h>
#include <GyverOLED.h>
#include <Wire.h>

#define EE_DATA_ADDR 1
#define EE_KEY_ADDR 0
#define EE_KEY 0xBE

GyverOLED<SSD1306_128x64> oled(0x3C);
EncButton<EB_CALLBACK, 2, 3, 4> enc;

struct {
  byte volume = 0;
  byte treble = 0;
  byte bass = 0;
  bool input = false;
  bool mute = false;
} data;

bool selected = false;
byte menu_pointer = 0;

bool save_requested = false;
uint32_t request_time = 0;

#define INPUT1_NAME "AUX INPUT"
#define INPUT2_NAME "BLUETOOTH"

void setup() {
  Wire.begin();
  Wire.setClock(100000);
  
  oled.init();
  oled.clear();
  oled.home();
  oled.update();
  
  enc.attach(LEFT_HANDLER, encHandler_left);
  enc.attach(RIGHT_HANDLER, encHandler_right);
  enc.attach(CLICK_HANDLER, encHandler_clicks);
  enc.attach(HOLDED_HANDLER, encHandler_hold);

  if (EEPROM.read(EE_KEY_ADDR) != EE_KEY) {
    EEPROM.put(EE_DATA_ADDR, data);
    EEPROM.write(EE_KEY_ADDR, EE_KEY);
  }

  EEPROM.get(EE_DATA_ADDR, data);
  TDA8425_volume(data.volume);
  TDA8425_treble(data.treble);
  TDA8425_bass(data.bass);
  TDA8425_control(data.input, data.mute);

  drawMenu();
}

void loop() {
  enc.tick();

  if (save_requested and millis() - request_time > 5000) {
    save_requested = false;
    EEPROM.put(EE_DATA_ADDR, data);
  }
}

void update(uint8_t num) {
  switch (num) {
    case 0: TDA8425_volume(data.volume); break;
    case 1: TDA8425_treble(data.treble); break;
    case 2: TDA8425_bass(data.bass); break;
    case 3: TDA8425_control(data.input, data.mute); break;
  }
  save_requested = true;
  request_time = millis();
}

void encHandler_left(void) {
  uint8_t inc = (enc.isFast() ? 10 : 5);
  if (selected) {
    switch (menu_pointer) {
      case 0: data.volume = constrain(data.volume - inc, 0, 100); break;
      case 1: data.treble = constrain(data.treble - inc, 0, 100); break;
      case 2: data.bass = constrain(data.bass - inc, 0, 100); break;
      case 3: data.input = !data.input; break;
    }
    update(menu_pointer);
  } else {
    menu_pointer = constrain(menu_pointer + 1, 0, 3);
  }
  drawMenu();
}

void encHandler_right(void) {
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
    menu_pointer = constrain(menu_pointer - 1, 0, 3);
  }
  drawMenu();
}

void encHandler_clicks(void) {
  selected = !selected;
  drawMenu();
}

void encHandler_hold(void) {
  data.mute = !data.mute;
  drawMenu();
  update(3);
}

void drawMenu(void) {
  oled.clear();
  oled.home();
  oled.setScale(1);

  if (data.mute) {
    oled.setCursor(50, 0);
    oled.print(F("MUTE"));
  } else {
    oled.setCursor(28, 0);
    oled.print(F("VOLUME: "));
    printValue(data.volume);
  }

  oled.setCursor(28, 2);
  oled.print(F("TREBLE: "));
  printValue(data.treble);

  oled.setCursor(34, 4);
  oled.print(F("BASS: "));
  printValue(data.bass);

  oled.setScale(2);
  oled.setCursor(10, 6);
  if (data.input) oled.print(F(INPUT1_NAME));
  else oled.print(F(INPUT2_NAME));

  oled.rect(0, 8, map(data.volume, 0, 100, 0, 127), 14, OLED_FILL);
  oled.rect(0, 24, map(data.treble, 0, 100, 0, 127), 30, OLED_FILL);
  oled.rect(0, 40, map(data.bass, 0, 100, 0, 127), 46, OLED_FILL);

  oled.line(0, 14, 127, 14);
  oled.line(0, 30, 127, 30);
  oled.line(0, 46, 127, 46);

  if (selected) {
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

void printValue(uint8_t data) {
  if (data > 99) oled.print(F("MAX"));
  else if (data < 1) oled.print(F("MIN"));
  else {
    oled.print(data);
    oled.print(F("%"));
  }
}


bool TDA8425_volume(uint8_t data) {
  Wire.beginTransmission(0x41);
  Wire.write(0);
  Wire.write(0b11000000 | data >> 2);
  Wire.write(0b11000000 | data >> 2);
  return (bool)!Wire.endTransmission();
}

bool TDA8425_bass(uint8_t data) {
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

bool TDA8425_control(bool in, bool mute) {
  byte reg = 0b11001110;
  if (in) reg |= 1 << 0;
  if (mute) reg |= 1 << 5;
  Wire.beginTransmission(0x41);
  Wire.write(0x08);
  Wire.write(reg);
  return (bool)!Wire.endTransmission();
}
