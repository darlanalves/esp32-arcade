// Main launcher + shared hardware for Pong, Arkanoid, Snake, Asteroids
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Bluepad32.h>

// TFT control pins avoid the ESP32 boot-strapping GPIOs (0, 2, 4, 5, 12, and 15).
#define TFT_CS         32
#define TFT_RST        27
#define TFT_DC         25
#define TFT_BLK_PIN    33
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define BUZZER_PIN 26

// Screen Dimensions (Portrait 240x280 for ST7789V2)
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 280;
const int PLAYFIELD_HEIGHT = SCREEN_HEIGHT - 18;
const int PLAYFIELD_BORDER_WIDTH = 3;

const uint8_t GAME_DPAD_UP = 0x01;
const uint8_t GAME_DPAD_DOWN = 0x02;

void drawGameBorder(Adafruit_GFX& display) {
  for (int offset = 0; offset < PLAYFIELD_BORDER_WIDTH; offset++) {
    display.drawRect(
      offset,
      offset,
      SCREEN_WIDTH - offset * 2,
      PLAYFIELD_HEIGHT - offset * 2,
      ST77XX_WHITE
    );
  }
}

// Bluepad32 Global Controller Pointers (shared)
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Forward declarations for game modules
void Pong_init(ControllerPtr myControllers[]);
bool Pong_update(ControllerPtr myControllers[]);
void Arkanoid_init(ControllerPtr myControllers[]);
bool Arkanoid_update(ControllerPtr myControllers[]);
void Snake_init(ControllerPtr myControllers[]);
bool Snake_update(ControllerPtr myControllers[]);
void Asteroids_init(ControllerPtr myControllers[]);
bool Asteroids_update(ControllerPtr myControllers[]);

// Handle freshly paired or connected gamepads
void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      Serial.printf("Controller connected at index %d\n", i);
      setControllerLED(i);
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

// Controller color mapping: Green, Blue, Cyan, Yellow, Red, Magenta
uint16_t getPaddleColor(int playerIndex) {
  switch(playerIndex % 6) {
    case 0: return ST77XX_GREEN;   // P1
    case 1: return ST77XX_BLUE;    // P2
    case 2: return ST77XX_CYAN;    // P3
    case 3: return ST77XX_YELLOW;  // P4
    case 4: return ST77XX_RED;     // P5
    case 5: return ST77XX_MAGENTA; // P6
    default: return ST77XX_WHITE;
  }
}

void setControllerLED(int playerIndex) {
  if (myControllers[playerIndex] && myControllers[playerIndex]->isConnected()) {
    switch(playerIndex % 6) {
      case 0: myControllers[playerIndex]->setColorLED(0, 255, 0); break;    // Green
      case 1: myControllers[playerIndex]->setColorLED(0, 0, 255); break;    // Blue
      case 2: myControllers[playerIndex]->setColorLED(0, 255, 255); break;  // Cyan
      case 3: myControllers[playerIndex]->setColorLED(255, 255, 0); break;  // Yellow
      case 4: myControllers[playerIndex]->setColorLED(255, 0, 0); break;    // Red
      case 5: myControllers[playerIndex]->setColorLED(255, 0, 255); break;  // Magenta
    }
  }
}

// Menu
const char* MENU_ITEMS[] = { "Pong", "Arkanoid", "Snake", "Asteroids" };
const int MENU_COUNT = 4;
int menuIndex = 0;

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 20);
  tft.print("ESP32 Arcade");

  tft.setTextSize(2);
  for (int i = 0; i < MENU_COUNT; i++) {
    int y = 90 + i * 35;
    if (i == menuIndex) {
      tft.fillRect(10, y-4, SCREEN_WIDTH-20, 28, ST77XX_WHITE);
      tft.setTextColor(ST77XX_BLACK);
      tft.setCursor(25, y);
      tft.print(MENU_ITEMS[i]);
      tft.setTextColor(ST77XX_WHITE);
    } else {
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(25, y);
      tft.print(MENU_ITEMS[i]);
    }
  }
}

bool anyControllerButtonPressed() {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] && myControllers[i]->isConnected()) {
      // Only the four face buttons start a game; the D-pad and sticks navigate.
      if (myControllers[i]->buttons() & 0x0F) return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BLK_PIN, OUTPUT);
  digitalWrite(TFT_BLK_PIN, HIGH);

  // Initialize Display (ST7789V2 240x280)
  tft.init(240, 280);
  tft.setRotation(0);  // Portrait mode for 240x280
  tft.fillScreen(ST77XX_BLACK);

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
        if (d & GAME_DPAD_UP) { menuIndex = max(0, menuIndex - 1); }
        if (d & GAME_DPAD_DOWN) { menuIndex = min(MENU_COUNT - 1, menuIndex + 1); }
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
      tft.fillScreen(ST77XX_BLACK);
      if (selectedGame == 0) {
        Pong_init(myControllers);
      } else if (selectedGame == 1) {
        Arkanoid_init(myControllers);
      } else if (selectedGame == 2) {
        Snake_init(myControllers);
      } else if (selectedGame == 3) {
        Asteroids_init(myControllers);
      }
    }
  } else {
    // Run selected game's update
    bool continueGame = true;
    if (selectedGame == 0) {
      continueGame = Pong_update(myControllers);
    } else if (selectedGame == 1) {
      continueGame = Arkanoid_update(myControllers);
    } else if (selectedGame == 2) {
      continueGame = Snake_update(myControllers);
    } else if (selectedGame == 3) {
      continueGame = Asteroids_update(myControllers);
    }

    if (!continueGame) {
      // Return to menu
      inGame = false;
      menuIndex = selectedGame; // Keep selection
      tft.fillScreen(ST77XX_BLACK);
      drawMenu();
    }
    delay(16); // ~60fps
  }
}
