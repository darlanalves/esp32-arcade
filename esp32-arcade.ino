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
const int PLAYFIELD_BORDER_WIDTH = 5;

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
ControllerPtr P1;
ControllerPtr P2;

// Forward declarations for game modules
void Pong_init(ControllerPtr myControllers[]);
bool Pong_update(ControllerPtr myControllers[]);
void Arkanoid_init(ControllerPtr myControllers[]);
bool Arkanoid_update(ControllerPtr myControllers[]);
void Snake_init(ControllerPtr myControllers[]);
bool Snake_update();
void Asteroids_init(ControllerPtr myControllers[]);
bool Asteroids_update(ControllerPtr myControllers[]);

// Handle freshly paired or connected gamepads
void onConnectedController(ControllerPtr ctl) {
  if (P1 == nullptr) {
    P1 = ctl;
    Serial.printf("Player 1 connected");
    setControllerLED(ctl, 0);
  }

  if (P2 == nullptr && ctl != P1) {
    P2 = ctl;
    Serial.printf("Player 2 connected");
    setControllerLED(ctl, 1);
  }
}

// Handle disconnected gamepads
void onDisconnectedController(ControllerPtr ctl) {
  if (P1->getIndex() == ctl->getIndex()) {
    P1 = nullptr;
    Serial.printf("Player 1 disconnected");
  }

  if (P2->getIndex() == ctl->getIndex()) {
    P2 = nullptr;
    Serial.printf("Player 2 disconnected");
  }
}

// Simple sound helpers used by games
void playPaddleHit() { tone(BUZZER_PIN, 800, 50); }
void playWallHit()   { tone(BUZZER_PIN, 500, 50); }
void playScoreSound(){ tone(BUZZER_PIN, 200, 250); }

// Controller color mapping: Green, Blue, Cyan, Yellow, Red, Magenta
uint16_t getPaddleColor(int playerIndex) {
  switch(playerIndex) {
    case 0: return ST77XX_GREEN;   // P1
    case 1: return ST77XX_BLUE;    // P2
  }
}

void setControllerLED(ControllerPtr ctl) {
  if (ctl->isConnected()) {
    playerIndex = ctl->getIndex();

    if(playerIndex == 0) {
      ctl->setColorLED(0, 255, 0); break;    // Green
    }

    if(playerIndex == 1) {
      ctl->setColorLED(0, 0, 255); break;    // Blue
    }
  }
}

// Menu
const char* MENU_ITEMS[] = { "Pong", "Arkanoid", "Snake", "Asteroids" };
const int MENU_COUNT = 4;
int menuIndex = 0;

void drawMenuItem(int itemIndex, bool selected) {
  int y = 90 + itemIndex * 35;
  tft.fillRect(10, y - 4, SCREEN_WIDTH - 20, 28,
               selected ? ST77XX_WHITE : ST77XX_BLACK);
  // round corners
  tft.drawPixel(10, y - 4, selected ? ST77XX_BLACK : ST77XX_WHITE);
  tft.drawPixel(SCREEN_WIDTH - 11, y - 4, selected ? ST77XX_BLACK : ST77XX_WHITE);
  tft.drawPixel(10, y + 23, selected ? ST77XX_BLACK : ST77XX_WHITE);
  tft.drawPixel(SCREEN_WIDTH - 11, y + 23, selected ? ST77XX_BLACK : ST77XX_WHITE);

  tft.setTextSize(2);
  tft.setTextColor(selected ? ST77XX_BLACK : ST77XX_WHITE);
  tft.setCursor(25, y);
  tft.print(MENU_ITEMS[itemIndex]);
}

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 20);
  tft.print("ESP32 Arcade");

  tft.setTextSize(2);
  for (int i = 0; i < MENU_COUNT; i++) {
    drawMenuItem(i, i == menuIndex);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BLK_PIN, OUTPUT);
  digitalWrite(TFT_BLK_PIN, HIGH);

  // Initialize Display (ST7789V2 240x280)
  tft.init(240, 280);
  tft.setRotation(90);  // Portrait mode for 240x280
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
    static bool menuDpadHeld = false;
    bool upPressed = false;
    bool downPressed = false;

    // Read every controller once, but advance only on a new D-pad press.
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
      if (myControllers[i] && myControllers[i]->isConnected()) {
        uint8_t d = myControllers[i]->dpad();
        upPressed = upPressed || (d & GAME_DPAD_UP);
        downPressed = downPressed || (d & GAME_DPAD_DOWN);
      }
    }

    if (menuIndex != lastMenuIndex) {
      if (lastMenuIndex < 0) {
        drawMenu();
      } else {
        drawMenuItem(lastMenuIndex, false);
        drawMenuItem(menuIndex, true);
      }
      lastMenuIndex = menuIndex;
    }

    if (!menuDpadHeld) {
      int previousMenuIndex = menuIndex;
      if (upPressed && !downPressed) menuIndex = max(0, menuIndex - 1);
      if (downPressed && !upPressed) menuIndex = min(MENU_COUNT - 1, menuIndex + 1);
      if (menuIndex != previousMenuIndex) {
        drawMenuItem(previousMenuIndex, false);
        drawMenuItem(menuIndex, true);
        lastMenuIndex = menuIndex;
      }
    }
    menuDpadHeld = upPressed || downPressed;

    // start game when any non-dpad button or axis is pressed
    bool startGame = (P1->isConnected() && P1->miscHome()) || (P2->isConnected() && P2->miscHome());

    if (startGame) {
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
      continueGame = Pong_update();
    } else if (selectedGame == 1) {
      continueGame = Arkanoid_update();
    } else if (selectedGame == 2) {
      continueGame = Snake_update();
    } else if (selectedGame == 3) {
      continueGame = Asteroids_update();
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
