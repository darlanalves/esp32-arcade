// Arkanoid (2-player variant) module
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

namespace Arkanoid {
  // Paddles (horizontal at bottom)
  const int PADDLE_WIDTH = 32;
  const int PADDLE_HEIGHT = 6;
  const int PADDLE_SPEED = 4;
  const int PADDLE_Y = SCREEN_HEIGHT - PADDLE_HEIGHT - 2;

  // Ball
  const int BALL_SIZE = 4;

  // Bricks - each brick has a type (0=none, 1=red, 2=yellow, 3=cyan)
  const int MAX_BRICK_ROWS = 22;
  const int INITIAL_BRICK_ROWS = 6;
  const int BRICK_COLS = 10;
  const uint32_t NEW_ROW_INTERVAL_MS = 10000;
  int brickW;
  int brickH = 8;
  uint8_t bricks[MAX_BRICK_ROWS][BRICK_COLS]; // 0=empty, 1-3=brick types
  int brickRows = INITIAL_BRICK_ROWS;
  uint32_t lastNewRowTime = 0;

  int paddle1X, paddle2X;
  int prevPaddle1X, prevPaddle2X;

  float ballX, ballY;
  float prevBallX, prevBallY;
  float ballXSpeed, ballYSpeed;

  int level = 1;
  int bricksRemaining = 0;
  int lastHitBy = -1; // 0 left, 1 right

  float ballSpeedMultiplier = 1.0f;
  uint32_t lastSpeedChangeTime = 0;

  uint16_t getBrickColor(uint8_t type) {
    switch(type) {
      case 1: return ST77XX_RED;     // Red = normal
      case 2: return ST77XX_YELLOW;  // Yellow = extra ball
      case 3: return ST77XX_CYAN;    // Cyan = extra life
      default: return ST77XX_BLACK;
    }
  }

  uint8_t newBrickType() {
    if (random(100) < (20 + level * 5)) return 3; // Cyan
    if (random(100) < 30) return 2; // Yellow
    return 1; // Red
  }

  void addTopRow() {
    if (brickRows == MAX_BRICK_ROWS) {
      for (int c = 0; c < BRICK_COLS; c++) {
        if (bricks[brickRows - 1][c]) bricksRemaining--;
      }
    } else {
      brickRows++;
    }

    for (int r = brickRows - 1; r > 0; r--) {
      for (int c = 0; c < BRICK_COLS; c++) {
        bricks[r][c] = bricks[r - 1][c];
      }
    }

    for (int c = 0; c < BRICK_COLS; c++) {
      bricks[0][c] = newBrickType();
      bricksRemaining++;
    }
  }

  void resetLevel() {
    bricksRemaining = 0;
    brickRows = 0;
    for (int r = 0; r < MAX_BRICK_ROWS; r++) {
      for (int c = 0; c < BRICK_COLS; c++) {
        bricks[r][c] = 0;
      }
    }
    for (int r = 0; r < INITIAL_BRICK_ROWS; r++) addTopRow();
  }

  void drawBricks() {
    int brickAreaX = (SCREEN_WIDTH - (BRICK_COLS * brickW)) / 2;
    tft.fillRect(brickAreaX, 8, BRICK_COLS * brickW, MAX_BRICK_ROWS * (brickH + 2), ST77XX_BLACK);
    for (int r = 0; r < brickRows; r++) {
      for (int c = 0; c < BRICK_COLS; c++) {
        int x = (SCREEN_WIDTH - (BRICK_COLS * brickW)) / 2 + c * brickW;
        int y = 8 + r * (brickH + 2);
        if (bricks[r][c]) {
          tft.fillRect(x, y, brickW-2, brickH, getBrickColor(bricks[r][c]));
        } else {
          tft.fillRect(x, y, brickW-2, brickH, ST77XX_BLACK);
        }
      }
    }
  }
}

void Arkanoid_init(ControllerPtr myControllers[]) {
  Arkanoid::level = 1;
  Arkanoid::paddle1X = 0;
  Arkanoid::paddle2X = SCREEN_WIDTH / 2;
  Arkanoid::prevPaddle1X = Arkanoid::paddle1X;
  Arkanoid::prevPaddle2X = Arkanoid::paddle2X;

  Arkanoid::ballX = SCREEN_WIDTH / 2;
  Arkanoid::ballY = SCREEN_HEIGHT / 2;
  Arkanoid::prevBallX = Arkanoid::ballX;
  Arkanoid::prevBallY = Arkanoid::ballY;
  Arkanoid::ballXSpeed = (random(0,2)==0)?2.0:-2.0;
  Arkanoid::ballYSpeed = random(-15,16)/10.0;

  Arkanoid::brickW = (SCREEN_WIDTH - 20) / Arkanoid::BRICK_COLS;
  Arkanoid::resetLevel();
  Arkanoid::lastNewRowTime = millis();

  tft.fillScreen(ST77XX_BLACK);
  drawGameBorder(tft);
  Arkanoid::drawBricks();
  tft.fillRect(Arkanoid::paddle1X, Arkanoid::PADDLE_Y, Arkanoid::PADDLE_WIDTH, Arkanoid::PADDLE_HEIGHT, getPaddleColor(0));
  tft.fillRect(Arkanoid::paddle2X, Arkanoid::PADDLE_Y, Arkanoid::PADDLE_WIDTH, Arkanoid::PADDLE_HEIGHT, getPaddleColor(1));
}

bool Arkanoid_update(ControllerPtr myControllers[]) {
  // Check for return to menu (home button)
  if (myControllers[0] && myControllers[0]->isConnected() && myControllers[0]->miscButtons()) {
    return false;
  }
  if (myControllers[1] && myControllers[1]->isConnected() && myControllers[1]->miscButtons()) {
    return false;
  }

  // Paddles - horizontal movement
  if (myControllers[0] && myControllers[0]->isConnected()) {
    int axis = myControllers[0]->axisX();
    if (axis < -200) { if (Arkanoid::paddle1X > 0) Arkanoid::paddle1X -= Arkanoid::PADDLE_SPEED; }
    if (axis > 200) { if (Arkanoid::paddle1X < SCREEN_WIDTH / 2 - Arkanoid::PADDLE_WIDTH) Arkanoid::paddle1X += Arkanoid::PADDLE_SPEED; }

    // Ball speed control with L1/R1
    uint32_t now = millis();
    if (now - Arkanoid::lastSpeedChangeTime > 150) {  // Debounce 150ms
      uint16_t btns = myControllers[0]->buttons();
      if (btns & 0x04) {  // L1 button code
        Arkanoid::ballSpeedMultiplier = max(0.3f, Arkanoid::ballSpeedMultiplier - 0.1f);
        Arkanoid::lastSpeedChangeTime = now;
      }
      if (btns & 0x08) {  // R1 button code
        Arkanoid::ballSpeedMultiplier = min(2.0f, Arkanoid::ballSpeedMultiplier + 0.1f);
        Arkanoid::lastSpeedChangeTime = now;
      }
    }
  }
  if (myControllers[1] && myControllers[1]->isConnected()) {
    int axis = myControllers[1]->axisX();
    if (axis < -200) { if (Arkanoid::paddle2X > SCREEN_WIDTH / 2) Arkanoid::paddle2X -= Arkanoid::PADDLE_SPEED; }
    if (axis > 200) { if (Arkanoid::paddle2X < SCREEN_WIDTH - Arkanoid::PADDLE_WIDTH) Arkanoid::paddle2X += Arkanoid::PADDLE_SPEED; }

    // Ball speed control with L1/R1 (P2 controller)
    uint32_t now = millis();
    if (now - Arkanoid::lastSpeedChangeTime > 150) {  // Debounce 150ms
      uint16_t btns = myControllers[1]->buttons();
      if (btns & 0x04) {  // L1 button code
        Arkanoid::ballSpeedMultiplier = max(0.3f, Arkanoid::ballSpeedMultiplier - 0.1f);
        Arkanoid::lastSpeedChangeTime = now;
      }
      if (btns & 0x08) {  // R1 button code
        Arkanoid::ballSpeedMultiplier = min(2.0f, Arkanoid::ballSpeedMultiplier + 0.1f);
        Arkanoid::lastSpeedChangeTime = now;
      }
    }
  }

  // ball physics - apply speed multiplier
  Arkanoid::ballX += Arkanoid::ballXSpeed * Arkanoid::ballSpeedMultiplier;
  Arkanoid::ballY += Arkanoid::ballYSpeed * Arkanoid::ballSpeedMultiplier;

  // wall collision (left/right)
  if (Arkanoid::ballX <= 0 || Arkanoid::ballX >= SCREEN_WIDTH - Arkanoid::BALL_SIZE) { Arkanoid::ballXSpeed = -Arkanoid::ballXSpeed; playWallHit(); }

  // top wall collision
  if (Arkanoid::ballY <= 0) { Arkanoid::ballYSpeed = -Arkanoid::ballYSpeed; playWallHit(); }

  // paddle collisions (credit lastHitBy) - horizontal paddles at bottom
  if (Arkanoid::ballY + Arkanoid::BALL_SIZE >= Arkanoid::PADDLE_Y && Arkanoid::ballY <= Arkanoid::PADDLE_Y + Arkanoid::PADDLE_HEIGHT) {
    // Paddle 1 (left half)
    if (Arkanoid::ballX + Arkanoid::BALL_SIZE >= Arkanoid::paddle1X && Arkanoid::ballX <= Arkanoid::paddle1X + Arkanoid::PADDLE_WIDTH) {
      Arkanoid::ballYSpeed = -Arkanoid::ballYSpeed;
      Arkanoid::ballY = Arkanoid::PADDLE_Y - Arkanoid::BALL_SIZE;
      playPaddleHit();
      Arkanoid::lastHitBy = 0;
    }
    // Paddle 2 (right half)
    if (Arkanoid::ballX + Arkanoid::BALL_SIZE >= Arkanoid::paddle2X && Arkanoid::ballX <= Arkanoid::paddle2X + Arkanoid::PADDLE_WIDTH) {
      Arkanoid::ballYSpeed = -Arkanoid::ballYSpeed;
      Arkanoid::ballY = Arkanoid::PADDLE_Y - Arkanoid::BALL_SIZE;
      playPaddleHit();
      Arkanoid::lastHitBy = 1;
    }
  }

  // brick collision
  for (int r=0;r<Arkanoid::brickRows;r++){
    for (int c=0;c<Arkanoid::BRICK_COLS;c++){
      if (!Arkanoid::bricks[r][c]) continue;
      int x = (SCREEN_WIDTH - (Arkanoid::BRICK_COLS * Arkanoid::brickW)) / 2 + c * Arkanoid::brickW;
      int y = 8 + r * (Arkanoid::brickH + 2);
      if (Arkanoid::ballX + Arkanoid::BALL_SIZE > x && Arkanoid::ballX < x + Arkanoid::brickW-2 && Arkanoid::ballY + Arkanoid::BALL_SIZE > y && Arkanoid::ballY < y + Arkanoid::brickH) {
        uint8_t brickType = Arkanoid::bricks[r][c];
        Arkanoid::bricks[r][c] = 0;
        Arkanoid::bricksRemaining--;
        Arkanoid::drawBricks();
        Arkanoid::ballYSpeed = -Arkanoid::ballYSpeed;
        playWallHit();

        // Handle rewards based on brick type
        if (brickType == 2) playScoreSound(); // Extra ball reward sound
        if (brickType == 3) playScoreSound(); // Extra life reward sound
      }
    }
  }

  if (millis() - Arkanoid::lastNewRowTime >= Arkanoid::NEW_ROW_INTERVAL_MS) {
    Arkanoid::addTopRow();
    Arkanoid::lastNewRowTime = millis();
    Arkanoid::drawBricks();
  }

  // Check if level complete
  if (Arkanoid::bricksRemaining <= 0) {
    Arkanoid::level++;
    Arkanoid::resetLevel();
    Arkanoid::ballX = SCREEN_WIDTH / 2;
    Arkanoid::ballY = SCREEN_HEIGHT / 2;
    Arkanoid::ballXSpeed = (random(0,2)==0)?2.0:-2.0;
    Arkanoid::ballYSpeed = random(-15,16)/10.0;
    tft.fillScreen(ST77XX_BLACK);
    Arkanoid::drawBricks();
    playScoreSound();
  }

  // out of bounds (bottom) -> reset ball
  if (Arkanoid::ballY > SCREEN_HEIGHT) {
    Arkanoid::ballX = SCREEN_WIDTH/2;
    Arkanoid::ballY = SCREEN_HEIGHT/2;
  }

  // render paddles
  if (Arkanoid::paddle1X != Arkanoid::prevPaddle1X) {
    tft.fillRect(Arkanoid::prevPaddle1X, Arkanoid::PADDLE_Y, Arkanoid::PADDLE_WIDTH, Arkanoid::PADDLE_HEIGHT, ST77XX_BLACK);
    tft.fillRect(Arkanoid::paddle1X, Arkanoid::PADDLE_Y, Arkanoid::PADDLE_WIDTH, Arkanoid::PADDLE_HEIGHT, getPaddleColor(0));
    Arkanoid::prevPaddle1X = Arkanoid::paddle1X;
  }
  if (Arkanoid::paddle2X != Arkanoid::prevPaddle2X) {
    tft.fillRect(Arkanoid::prevPaddle2X, Arkanoid::PADDLE_Y, Arkanoid::PADDLE_WIDTH, Arkanoid::PADDLE_HEIGHT, ST77XX_BLACK);
    tft.fillRect(Arkanoid::paddle2X, Arkanoid::PADDLE_Y, Arkanoid::PADDLE_WIDTH, Arkanoid::PADDLE_HEIGHT, getPaddleColor(1));
    Arkanoid::prevPaddle2X = Arkanoid::paddle2X;
  }

  // render ball
  tft.fillRect(Arkanoid::prevBallX, Arkanoid::prevBallY, Arkanoid::BALL_SIZE, Arkanoid::BALL_SIZE, ST77XX_BLACK);
  tft.fillRect(Arkanoid::ballX, Arkanoid::ballY, Arkanoid::BALL_SIZE, Arkanoid::BALL_SIZE, ST77XX_WHITE);
  Arkanoid::prevBallX = Arkanoid::ballX; Arkanoid::prevBallY = Arkanoid::ballY;

  // Display level in top-right
  tft.fillRect(140, 2, 20, 10, ST77XX_BLACK);
  tft.setCursor(140, 2); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(1);
  tft.print("L"); tft.print(Arkanoid::level);

  // Display speed multiplier
  tft.fillRect(0, 2, 25, 10, ST77XX_BLACK);
  tft.setCursor(0, 2); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(1);
  tft.print("S:");
  if (Arkanoid::ballSpeedMultiplier < 1.0f) tft.setTextColor(ST77XX_CYAN);
  else if (Arkanoid::ballSpeedMultiplier > 1.0f) tft.setTextColor(ST77XX_RED);
  else tft.setTextColor(ST77XX_WHITE);
  tft.print(Arkanoid::ballSpeedMultiplier, 1);

  return true; // Continue game
}
