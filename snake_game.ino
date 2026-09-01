// Snake game module
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bluepad32.h>

extern Adafruit_ST7789 tft;
extern void playPaddleHit();
extern void playWallHit();
extern void playScoreSound();
extern uint16_t getPaddleColor(int playerIndex);
extern void drawGameBorder(Adafruit_GFX& display);
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const uint8_t GAME_DPAD_UP;
extern const uint8_t GAME_DPAD_DOWN;

namespace Snake {
  const int GRID_SIZE = 12;  // pixels per grid cell
  const int GRID_WIDTH = SCREEN_WIDTH / GRID_SIZE;  // 20 cells
  const int GRID_HEIGHT = (SCREEN_HEIGHT - 20) / GRID_SIZE;  // ~21 cells (leaving room for score)

  const int MAX_LENGTH = 100;

  int headX, headY;
  int dirX, dirY;  // Direction (1,0), (-1,0), (0,1), (0,-1)
  int nextDirX, nextDirY;  // Buffered next direction

  int bodyX[MAX_LENGTH], bodyY[MAX_LENGTH];
  int length = 3;

  int foodX, foodY;
  int score = 0;

  float speedMultiplier = 1.0f;
  uint32_t lastSpeedChangeTime = 0;
  uint32_t lastMoveTime = 0;
  int moveDelay = 100;  // milliseconds

  void drawCell(int cellX, int cellY, int inset, uint16_t color) {
    tft.fillRect(cellX * GRID_SIZE + inset, cellY * GRID_SIZE + inset,
                 GRID_SIZE - inset * 2, GRID_SIZE - inset * 2, color);
  }

  void drawHud() {
    tft.fillRect(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, 18, ST77XX_BLACK);
    tft.drawLine(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, SCREEN_HEIGHT - 18, ST77XX_WHITE);
    tft.setCursor(5, SCREEN_HEIGHT - 15);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.print("Score:");
    tft.print(score);
    tft.setCursor(SCREEN_WIDTH - 45, SCREEN_HEIGHT - 15);
    tft.print("S:");
    if (speedMultiplier < 1.0f) tft.setTextColor(ST77XX_CYAN);
    else if (speedMultiplier > 1.0f) tft.setTextColor(ST77XX_RED);
    else tft.setTextColor(ST77XX_WHITE);
    tft.print(speedMultiplier, 1);
  }

  void spawnFood() {
    do {
      foodX = random(GRID_WIDTH);
      foodY = random(GRID_HEIGHT);
    } while (foodX == headX && foodY == headY);  // Don't spawn on head
  }

  bool checkCollisionWithBody() {
    for (int i = 1; i < length; i++) {
      if (bodyX[i] == headX && bodyY[i] == headY) {
        return true;
      }
    }
    return false;
  }
}

void Snake_init(ControllerPtr myControllers[]) {
  Snake::headX = Snake::GRID_WIDTH / 2;
  Snake::headY = Snake::GRID_HEIGHT / 2;
  Snake::dirX = 1;
  Snake::dirY = 0;
  Snake::nextDirX = 1;
  Snake::nextDirY = 0;

  Snake::length = 3;
  for (int i = 0; i < Snake::length; i++) {
    Snake::bodyX[i] = Snake::headX - i;
    Snake::bodyY[i] = Snake::headY;
  }

  Snake::score = 0;
  Snake::speedMultiplier = 1.0f;
  Snake::lastMoveTime = millis();

  Snake::spawnFood();

  tft.fillScreen(ST77XX_BLACK);
  drawGameBorder(tft);
  for (int i = 0; i < Snake::length; i++) {
    Snake::drawCell(Snake::bodyX[i], Snake::bodyY[i], 1,
                    i == 0 ? getPaddleColor(0) : ST77XX_GREEN);
  }
  Snake::drawCell(Snake::foodX, Snake::foodY, 2, ST77XX_RED);
  Snake::drawHud();
}

bool Snake_update(ControllerPtr myControllers[]) {
  // Check for return to menu (home button)
  if (myControllers[0] && myControllers[0]->isConnected() && myControllers[0]->miscButtons()) {
    return false;
  }

  // Input handling
  if (myControllers[0] && myControllers[0]->isConnected()) {
    int axisX = myControllers[0]->axisX();
    int axisY = myControllers[0]->axisY();
    uint8_t dpad = myControllers[0]->dpad();

    // D-pad or analog stick
    if ((dpad & GAME_DPAD_UP) || axisY < -200) {
      Snake::nextDirX = 0;
      Snake::nextDirY = -1;
    }
    if ((dpad & GAME_DPAD_DOWN) || axisY > 200) {
      Snake::nextDirX = 0;
      Snake::nextDirY = 1;
    }
    if (axisX < -200) {
      Snake::nextDirX = -1;
      Snake::nextDirY = 0;
    }
    if (axisX > 200) {
      Snake::nextDirX = 1;
      Snake::nextDirY = 0;
    }

    // Speed control with L1/R1
    uint32_t now = millis();
    if (now - Snake::lastSpeedChangeTime > 150) {
      uint16_t btns = myControllers[0]->buttons();
      if (btns & 0x04) {  // L1 button
        Snake::speedMultiplier = max(0.3f, Snake::speedMultiplier - 0.1f);
        Snake::lastSpeedChangeTime = now;
        Snake::drawHud();
      }
      if (btns & 0x08) {  // R1 button
        Snake::speedMultiplier = min(2.0f, Snake::speedMultiplier + 0.1f);
        Snake::lastSpeedChangeTime = now;
        Snake::drawHud();
      }
    }
  }

  // Game update
  uint32_t now = millis();
  int currentDelay = (int)(Snake::moveDelay / Snake::speedMultiplier);

  if (now - Snake::lastMoveTime > currentDelay) {
    Snake::lastMoveTime = now;
    int tailX = Snake::bodyX[Snake::length - 1];
    int tailY = Snake::bodyY[Snake::length - 1];

    // Prevent 180-degree turns
    if (!(Snake::dirX != 0 && Snake::nextDirX != 0 && (Snake::dirX + Snake::nextDirX == 0))) {
      if (!(Snake::dirY != 0 && Snake::nextDirY != 0 && (Snake::dirY + Snake::nextDirY == 0))) {
        Snake::dirX = Snake::nextDirX;
        Snake::dirY = Snake::nextDirY;
      }
    }

    // Move head
    Snake::headX += Snake::dirX;
    Snake::headY += Snake::dirY;

    // Wall collision
    if (Snake::headX < 0 || Snake::headX >= Snake::GRID_WIDTH ||
        Snake::headY < 0 || Snake::headY >= Snake::GRID_HEIGHT) {
      playWallHit();
      return false;  // Game over
    }

    // Self collision
    if (Snake::checkCollisionWithBody()) {
      playWallHit();
      return false;  // Game over
    }

    bool ateFood = Snake::headX == Snake::foodX && Snake::headY == Snake::foodY;

    // Shift body
    for (int i = Snake::length - 1; i > 0; i--) {
      Snake::bodyX[i] = Snake::bodyX[i - 1];
      Snake::bodyY[i] = Snake::bodyY[i - 1];
    }
    Snake::bodyX[0] = Snake::headX;
    Snake::bodyY[0] = Snake::headY;

    // Food collision
    if (ateFood) {
      if (Snake::length < Snake::MAX_LENGTH) {
        Snake::bodyX[Snake::length] = tailX;
        Snake::bodyY[Snake::length] = tailY;
        Snake::length++;
      }
      Snake::score += 10;
      playScoreSound();
      Snake::spawnFood();
    }

    if (!ateFood) Snake::drawCell(tailX, tailY, 1, ST77XX_BLACK);
    Snake::drawCell(Snake::bodyX[0], Snake::bodyY[0], 1, getPaddleColor(0));
    if (Snake::length > 1) Snake::drawCell(Snake::bodyX[1], Snake::bodyY[1], 1, ST77XX_GREEN);
    if (ateFood) {
      Snake::drawCell(Snake::foodX, Snake::foodY, 2, ST77XX_RED);
      Snake::drawHud();
    }
  }

  return true;
}
