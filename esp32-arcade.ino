// Main launcher + shared hardware for Pong, Arkanoid, Snake, Asteroids
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Bluepad32.h>
#include "arkanoid.h"
#include "snake.h"
#include "asteroids.h"
#include "game.h"
#include "pong.h"

// TFT control pins avoid the ESP32 boot-strapping GPIOs (0, 2, 4, 5, 12, and 15).
#define TFT_CS 32
#define TFT_RST 27
#define TFT_DC 25
#define TFT_BLK_PIN 33
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Screen Dimensions (Portrait 240x280 for ST7789V2)
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 280;

// Bluepad32 Global Controller Pointers (shared)
ControllerPtr P1;
ControllerPtr P2;

Game *selectedGame = nullptr;
static Game *availableGames[] = {
    new PongGame(),
    new ArkanoidGame(),
    new SnakeGame(),
    new AsteroidsGame(),
};

static bool inGame = false;
static int selectedGameIndex = 0;
static int menuIndex = 0;
static int lastMenuIndex = -1;
static const int MENU_COUNT = sizeof(availableGames) / sizeof(availableGames[0]);

// Handle freshly paired or connected gamepads
void onConnectedController(ControllerPtr ctl)
{
  if (P1 == nullptr)
  {
    P1 = ctl;
    Serial.printf("Player 1 connected\n");
    ctl->setColorLED(0, 255, 0);
    return;
  }

  if (P2 == nullptr)
  {
    P2 = ctl;
    Serial.printf("Player 2 connected\n");
    ctl->setColorLED(0, 0, 255);
  }

  if (selectedGame != nullptr)
  {
    selectedGame->setControllers(P1, P2);
  }
}

// Handle disconnected gamepads
void onDisconnectedController(ControllerPtr ctl)
{
  if (P1 && P1->index() == ctl->index())
  {
    P1 = nullptr;
    Serial.printf("Player 1 disconnected\n");
  }

  if (P2 && P2->index() == ctl->index())
  {
    P2 = nullptr;
    Serial.printf("Player 2 disconnected\n");
  }

  if (selectedGame != nullptr)
  {
    selectedGame->setControllers(P1, P2);
  }
}

void drawMenuItem(int itemIndex, bool selected)
{
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
  tft.print(availableGames[itemIndex]->getName());
}

void drawMenu()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 20);
  tft.print("ESP32 Arcade");

  tft.setTextSize(2);
  for (int i = 0; i < MENU_COUNT; i++)
  {
    drawMenuItem(i, i == menuIndex);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(TFT_BLK_PIN, OUTPUT);
  digitalWrite(TFT_BLK_PIN, HIGH);

  // Initialize Display (ST7789V2 240x280)
  tft.init(240, 280);
  tft.setRotation(90); // 240x280
  tft.fillScreen(ST77XX_BLACK);

  randomSeed(analogRead(0));

  // Initialize Bluepad32 system
  BP32.setup(&onConnectedController, &onDisconnectedController);
}

void loop()
{
  BP32.update();

  if (!inGame)
  {
    static bool menuDpadHeld = false;
    bool upPressed = false;
    bool downPressed = false;

    // Check connected players (only P1/P2 used)
    if (P1 && P1->isConnected())
    {
      uint8_t d = P1->dpad();
      upPressed = upPressed || (d & DPAD_UP);
      downPressed = downPressed || (d & DPAD_DOWN);
    }
    if (P2 && P2->isConnected())
    {
      uint8_t d = P2->dpad();
      upPressed = upPressed || (d & DPAD_UP);
      downPressed = downPressed || (d & DPAD_DOWN);
    }

    if (menuIndex != lastMenuIndex)
    {
      if (lastMenuIndex < 0)
      {
        drawMenu();
      }
      else
      {
        drawMenuItem(lastMenuIndex, false);
        drawMenuItem(menuIndex, true);
      }
      lastMenuIndex = menuIndex;
    }

    if (!menuDpadHeld)
    {
      int previousMenuIndex = menuIndex;
      if (upPressed && !downPressed)
        menuIndex = max(0, menuIndex - 1);
      if (downPressed && !upPressed)
        menuIndex = min(MENU_COUNT - 1, menuIndex + 1);
      if (menuIndex != previousMenuIndex)
      {
        drawMenuItem(previousMenuIndex, false);
        drawMenuItem(menuIndex, true);
        lastMenuIndex = menuIndex;
      }
    }

    menuDpadHeld = upPressed || downPressed;

    // start game when any non-dpad button or axis is pressed
    bool startGame = (P1 && P1->isConnected() && P1->miscHome()) || (P2 && P2->isConnected() && P2->miscHome());

    if (startGame)
    {
      selectedGameIndex = menuIndex;
      selectedGame = availableGames[menuIndex];
      inGame = true;
      tft.fillScreen(ST77XX_BLACK);

      // Configure controllers & initialize game via Game interface
      selectedGame->setControllers(P1, P2);
      selectedGame->init();
    }
  }
  else
  {
    // Run selected game's update
    bool continueGame = selectedGame->update();

    if (!continueGame)
    {
      // Return to menu
      inGame = false;
      menuIndex = selectedGameIndex;
      selectedGame = nullptr;
      lastMenuIndex = -1;
      tft.fillScreen(ST77XX_BLACK);
      drawMenu();
    }
    delay(16); // ~60fps
  }
}
