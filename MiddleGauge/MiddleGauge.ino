// DISPLAY
#include <TFT_eSPI.h> // Подключаем библиотеку TFT_eSPI
TFT_eSPI tft; // Создаем объект для работы с дисплеем

// TOUCH
#define TOUCH_IRQ 36
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK 25
#define TOUCH_CS 33
#include <XPT2046_Touchscreen.h> // Библиотека для работы с тачскрином
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touchScreen(TOUCH_CS, TOUCH_IRQ);

// PORT EXPANDER MCP23017
#include <SPI.h>
#include <Wire.h>
#include <MCP23017.h>
#define MCP_ADDRESS 0x20 // (A2/A1/A0 = LOW)
#define SDA_PIN 22
#define SCL_PIN 27
#define RESET_PIN 21
MCP23017 portExpander = MCP23017(MCP_ADDRESS);

// Wi-Fi для связки с соседними приборами
#include <FS.h>
using namespace fs;
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Toyota MR-S Yatt Gambella (Control panel)";
const char* password = "12345678";

WebServer server(80);


// UI
int width = 240;
int height = 240;
int offset_y = 40;

// CAR SYSTEM STATE
bool is_ac_on = false;
int  power_state = 0;

// PORT EXPANDER PINS
#define GROUP_AIR_CONTROL           A
#define PIN_AIR_FLOW_POWER_STATE_1  1
#define PIN_AIR_FLOW_POWER_STATE_2  2
#define PIN_AIR_FLOW_POWER_STATE_3  3
#define PIN_AIR_FLOW_POWER_STATE_4  4
#define PIN_AC                      5
#define PIN_INPUT_1                 6
#define PIN_INPUT_2                 7
#define PIN_INPUT_3                 8
 
#define GROUP_CAR_CONTROL           B
#define PIN_POWER_WINDOW_LEFT_UP    1
#define PIN_POWER_WINDOW_LEFT_DOWN  2
#define PIN_POWER_WINDOW_RIGHT_UP   3
#define PIN_POWER_WINDOW_RIGHT_DOWN 4
#define PIN_HYDRAULICS_FRONT_UP     5
#define PIN_HYDRAULICS_FRONT_DOWN   6
#define PIN_HYDRAULICS_REAR_UP      7
#define PIN_HYDRAULICS_REAR_DOWN    8

// TOUCH STATE
bool wasTouched = false;
unsigned long lastTouchTime = 0;
const unsigned long debounceDelay = 200; // Минимальный интервал между касаниями экрана (в миллисекундах)

// IMAGES
// OFF
const uint16_t off_0[] = {
  #include "off_0.txt"
};
const uint16_t off_1[] = {
  #include "off_1.txt"
};
const uint16_t off_2[] = {
  #include "off_2.txt"
};
const uint16_t off_3[] = {
  #include "off_3.txt"
};
const uint16_t off_4[] = {
  #include "off_4.txt"
};

// ON
const uint16_t on_0[] = {
  #include "on_0.txt"
};
const uint16_t on_1[] = {
  #include "on_1.txt"
};
const uint16_t on_2[] = {
  #include "on_2.txt"
};
const uint16_t on_3[] = {
  #include "on_3.txt"
};
const uint16_t on_4[] = {
  #include "on_4.txt"
};


// SETTING EVERYTHING UP
void setup() {
  setupSerial();
  setupTouch();
  setupScreen();
  setupPortExpander();
  setupServer();
}

void setupSerial() {
  Serial.begin(115200);
}

void setupTouch() {
  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touchScreen.begin(touchSPI);
  touchScreen.setRotation(1); // Изменение ориентации тачскрина на портретную
}

void setupScreen() {
  tft.init(); // Инициализация дисплея
  tft.setRotation(0); // Устанавливаем вертикальную ориентацию дисплея
  tft.fillScreen(TFT_BLACK);
}

void setupPortExpander() {
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!portExpander.Init()) {
    Serial.println("MCP23017 Not connected!");
    //while (1) {}
  }

  portExpander.setPortMode(0b00011111, A);  // Port A: 1-5 are OUTPUT, 6-8 are INPUT
  portExpander.setPortPullUp(0b11100000, A); // All A INPUT pins are PULLED UP
  
  portExpander.setPortMode(0b11111111, B);  // Port B: all pins are OUTPUT

  pinMode(RESET_PIN, OUTPUT);
}

void setupServer() {
    // Создаем точку доступа
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);
  
  // Регистрируем обработчик на корневой URL
  server.on("/", handleRoot);
  server.begin();
}


void resetPortExpander () {
  digitalWrite(RESET_PIN, LOW);
  delay(5 + 5 * power_state);
  digitalWrite(RESET_PIN, HIGH);
  updatePins();
}


// UPDATING EVERYTHING
void loop() {
  //checkPorts();
  checkTouch();
  checkServer();
}

// CHECKING PORTS
// void checkPorts() {
//   uint8_t portStatus = portExpander.getPort(A);
//   uint8_t pinStatus = portExpander.getPin(5, A);

//   String temp = "Port A Status = " + String(portStatus, BIN);
//   temp = "Pin 5 Status = " + String(pinStatus);
// }

// CHECKING TOUCH
void checkTouch() {
  bool isTouched = touchScreen.tirqTouched() && touchScreen.touched();
  unsigned long currentTime = millis();

  if (isTouched && !wasTouched && (currentTime - lastTouchTime > debounceDelay)) { // Если касание только что произошло и прошло достаточное время
    checkTouchPoint();
    lastTouchTime = currentTime; // Обновляем время последнего касания
  }

  wasTouched = isTouched; // Обновляем состояние предыдущего касания
}

// CHECKING SERVER
void checkServer() {
  server.handleClient();
}

// DOING ACTIONS
void checkTouchPoint() {
    TS_Point p = touchScreen.getPoint();

    if (p.x < 2700) { // Выше линии
      changePowerState(p.y < 2000 ? 1 : -1);
    } else { // Ниже линии
      switchAC();
    }
}

// ACTIONS

void changePowerState(int amount) {
  power_state = (power_state + amount + 5) % 5;
  updateScreen();
  resetPortExpander();
}

void switchAC() {
  is_ac_on = !is_ac_on;
  updateScreen();
  resetPortExpander();
}

// COMMUNICATION

void handleRoot() {
  String message = "Data received: ";
  message += server.arg("data");
  server.send(200, "text/plain", message);
}

// REDRAW

void updateScreen() {
  switch (power_state) {
    case 0:
      draw(is_ac_on ? on_0 : off_0);
      break;
    case 1:
      draw(is_ac_on ? on_1 : off_1);
      break;
    case 2:
      draw(is_ac_on ? on_2 : off_2);
      break;
    case 3:
      draw(is_ac_on ? on_3 : off_3);
      break;
    case 4:
      draw(is_ac_on ? on_4 : off_4);
      break;
  }
}

void draw(const uint16_t* image) {
  tft.pushImage(offset_y, 0, width, height, image);
}

void updatePins () {
  if (is_ac_on)
    portExpander.setPinX(PIN_AC, GROUP_AIR_CONTROL, OUTPUT, HIGH);

  // В любом случае, если кондишка включена, должен быть выставлен первый пин
  if (power_state > 0)
    portExpander.setPinX(PIN_AIR_FLOW_POWER_STATE_1, GROUP_AIR_CONTROL, OUTPUT, HIGH);

  if (power_state == 2)
    portExpander.setPinX(PIN_AIR_FLOW_POWER_STATE_2, GROUP_AIR_CONTROL, OUTPUT, HIGH);

  if (power_state == 3)
    portExpander.setPinX(PIN_AIR_FLOW_POWER_STATE_3, GROUP_AIR_CONTROL, OUTPUT, HIGH);

  if (power_state == 4)
    portExpander.setPinX(PIN_AIR_FLOW_POWER_STATE_4, GROUP_AIR_CONTROL, OUTPUT, HIGH);
}
