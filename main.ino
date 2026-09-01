// Main launcher + shared hardware for Pong and Arkanoid
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Bluepad32.h>

// TFT Pin Definitions
#define TFT_CS         15
#define TFT_RST         4
#define TFT_DC          2
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define BUZZER_PIN 26

// Screen Dimensions (Landscape 160x128)
const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 128;

#define DPAD_UP    0x01
#define DPAD_DOWN  0x02

// Bluepad32 Global Controller Pointers (shared)
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Forward declarations for game modules
void Pong_init();
void Pong_update();
void Arkanoid_init();
void Arkanoid_update();

// Handle freshly paired or connected gamepads
void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      Serial.printf("Controller connected at index %d\n", i);
      ctl->setColorLED(0, 255, 0);
      break;
    }
  }
}

// Handle disconnected gamepads
void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      Serial.printf("Controller disconnected from index %d\n", i);
      break;
    }
  }
}

// Simple sound helpers used by games
void playPaddleHit() { tone(BUZZER_PIN, 800, 50); }
void playWallHit()   { tone(BUZZER_PIN, 500, 50); }
void playScoreSound(){ tone(BUZZER_PIN, 200, 250); }

// Menu
const char* MENU_ITEMS[] = { "Pong", "Arkanoid" };
const int MENU_COUNT = 2;
int menuIndex = 0;

void drawMenu() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 10);
  tft.print("Select Game");

  tft.setTextSize(1);
  for (int i = 0; i < MENU_COUNT; i++) {
    int y = 40 + i * 18;
    if (i == menuIndex) {
      tft.fillRect(8, y-2, SCREEN_WIDTH-16, 16, ST7735_WHITE);
      tft.setTextColor(ST7735_BLACK);
      tft.setCursor(12, y);
      tft.print(MENU_ITEMS[i]);
      tft.setTextColor(ST7735_WHITE);
    } else {
      tft.setCursor(12, y);
      tft.print(MENU_ITEMS[i]);
    }
  }
}

bool anyControllerButtonPressed() {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] && myControllers[i]->isConnected()) {
      // use buttons() if available; fallback to axis/trigger activity
      if (myControllers[i]->buttons()) return true;
      if (abs(myControllers[i]->axisX()) > 200) return true;
      if (abs(myControllers[i]->axisY()) > 200) return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(25, OUTPUT);
  digitalWrite(25, HIGH);

  // Initialize Display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  randomSeed(analogRead(0));

  // Initialize Bluepad32 system
  BP32.setup(&onConnectedController, &onDisconnectedController);
}

bool inGame = false;
int selectedGame = 0;

void loop() {
  BP32.update();

  if (!inGame) {
    static int lastMenuIndex = -1;
    // read dpad from any controller to move selection
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
      if (myControllers[i] && myControllers[i]->isConnected()) {
        uint8_t d = myControllers[i]->dpad();
        if (d & DPAD_UP) { menuIndex = max(0, menuIndex - 1); }
        if (d & DPAD_DOWN) { menuIndex = min(MENU_COUNT - 1, menuIndex + 1); }
      }
    }

    if (menuIndex != lastMenuIndex) {
      drawMenu();
      lastMenuIndex = menuIndex;
    }

    // start game when any non-dpad button or axis is pressed
    if (anyControllerButtonPressed()) {
      selectedGame = menuIndex;
      inGame = true;
      tft.fillScreen(ST7735_BLACK);
      if (selectedGame == 0) Pong_init(); else Arkanoid_init();
    }
  } else {
    // Run selected game's update
    if (selectedGame == 0) Pong_update(); else Arkanoid_update();
    delay(16); // ~60fps
  }
}
