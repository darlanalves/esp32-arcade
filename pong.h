// Pong game module (2-player)
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

namespace Pong {
  const int PADDLE_WIDTH = 6;
  const int PADDLE_HEIGHT = 30;
  const int BALL_SIZE = 8;
  const int PADDLE_SPEED = 4;

  int paddle1Y;
  int paddle2Y;
  int prevPaddle1Y;
  int prevPaddle2Y;

  float ballX, ballY;
  float prevBallX, prevBallY;
  float ballXSpeed, ballYSpeed;

  int score1 = 0;
  int score2 = 0;

  float ballSpeedMultiplier = 1.0f;
  uint32_t lastSpeedChangeTime = 0;
}

namespace Pong {
  void drawScores() {
    tft.fillRect(40, 2, 80, 12, ST77XX_BLACK);
    tft.setCursor(50, 2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.print(score1);
    tft.setCursor(100, 2);
    tft.print(score2);
  }
}

namespace Pong {
  void resetBall() {
    tft.fillRect(prevBallX - 1, prevBallY - 1, BALL_SIZE + 2, BALL_SIZE + 2, ST77XX_BLACK);
    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    ballXSpeed = (random(0, 2) == 0) ? 2.0 : -2.0;
    ballYSpeed = random(-15, 16) / 10.0;
    drawScores();
  }
}

void Pong_init(ControllerPtr myControllers[]) {
  Pong::paddle1Y = (SCREEN_HEIGHT - Pong::PADDLE_HEIGHT) / 2;
  Pong::paddle2Y = (SCREEN_HEIGHT - Pong::PADDLE_HEIGHT) / 2;
  Pong::prevPaddle1Y = Pong::paddle1Y;
  Pong::prevPaddle2Y = Pong::paddle2Y;

  Pong::ballX = SCREEN_WIDTH / 2;
  Pong::ballY = SCREEN_HEIGHT / 2;
  Pong::prevBallX = Pong::ballX;
  Pong::prevBallY = Pong::ballY;

  Pong::score1 = 0;
  Pong::score2 = 0;

  Pong::resetBall();
  Pong::drawScores();

  // draw initial paddles
  tft.fillRect(10, Pong::paddle1Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, getPaddleColor(0));
  tft.fillRect(SCREEN_WIDTH - 10 - Pong::PADDLE_WIDTH, Pong::paddle2Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, getPaddleColor(1));
}

bool Pong_update(ControllerPtr myControllers[]) {
  // Check for return to menu (home button)
  if (myControllers[0] && myControllers[0]->isConnected() && myControllers[0]->miscButtons()) {
    return false;
  }
  if (myControllers[1] && myControllers[1]->isConnected() && myControllers[1]->miscButtons()) {
    return false;
  }

  // 1. Process Player 1 Controller -> Left Paddle
  if (myControllers[0] && myControllers[0]->isConnected()) {
    uint8_t dpad = myControllers[0]->dpad();
    if ((dpad & GAME_DPAD_UP) || (myControllers[0]->axisY() < -200)) {
      if (Pong::paddle1Y > 2) Pong::paddle1Y -= Pong::PADDLE_SPEED;
    }
    if ((dpad & GAME_DPAD_DOWN) || (myControllers[0]->axisY() > 200)) {
      if (Pong::paddle1Y < SCREEN_HEIGHT - Pong::PADDLE_HEIGHT - 2) Pong::paddle1Y += Pong::PADDLE_SPEED;
    }

    // Ball speed control with L1/R1
    uint32_t now = millis();
    if (now - Pong::lastSpeedChangeTime > 150) {  // Debounce 150ms
      uint16_t btns = myControllers[0]->buttons();
      if (btns & 0x04) {  // L1 button code
        Pong::ballSpeedMultiplier = max(0.3f, Pong::ballSpeedMultiplier - 0.1f);
        Pong::lastSpeedChangeTime = now;
      }
      if (btns & 0x08) {  // R1 button code
        Pong::ballSpeedMultiplier = min(2.0f, Pong::ballSpeedMultiplier + 0.1f);
        Pong::lastSpeedChangeTime = now;
      }
    }
  }

  // 2. Process Player 2 Controller -> Right Paddle
  if (myControllers[1] && myControllers[1]->isConnected()) {
    uint8_t dpad = myControllers[1]->dpad();
    if ((dpad & GAME_DPAD_UP) || (myControllers[1]->axisY() < -200)) {
      if (Pong::paddle2Y > 2) Pong::paddle2Y -= Pong::PADDLE_SPEED;
    }
    if ((dpad & GAME_DPAD_DOWN) || (myControllers[1]->axisY() > 200)) {
      if (Pong::paddle2Y < SCREEN_HEIGHT - Pong::PADDLE_HEIGHT - 2) Pong::paddle2Y += Pong::PADDLE_SPEED;
    }

    // Ball speed control with L1/R1 (P2 controller)
    uint32_t now = millis();
    if (now - Pong::lastSpeedChangeTime > 150) {  // Debounce 150ms
      uint16_t btns = myControllers[1]->buttons();
      if (btns & 0x04) {  // L1 button code
        Pong::ballSpeedMultiplier = max(0.3f, Pong::ballSpeedMultiplier - 0.1f);
        Pong::lastSpeedChangeTime = now;
      }
      if (btns & 0x08) {  // R1 button code
        Pong::ballSpeedMultiplier = min(2.0f, Pong::ballSpeedMultiplier + 0.1f);
        Pong::lastSpeedChangeTime = now;
      }
    }
  }

  // 3. Update Ball Physics
  Pong::ballX += Pong::ballXSpeed * Pong::ballSpeedMultiplier;
  Pong::ballY += Pong::ballYSpeed * Pong::ballSpeedMultiplier;

  // wall hit
  if (Pong::ballY <= 0 || Pong::ballY >= SCREEN_HEIGHT - Pong::BALL_SIZE) {
    Pong::ballYSpeed = -Pong::ballYSpeed;
    playWallHit();
  }

  // p1 collision
  if (Pong::ballX <= 10 + Pong::PADDLE_WIDTH && Pong::ballX >= 10) {
    if (Pong::ballY + Pong::BALL_SIZE >= Pong::paddle1Y && Pong::ballY <= Pong::paddle1Y + Pong::PADDLE_HEIGHT) {
      Pong::ballXSpeed = -Pong::ballXSpeed * 1.05;
      Pong::ballX = 10 + Pong::PADDLE_WIDTH + 1;
      playPaddleHit();
    }
  }

  // p2 collision
  if (Pong::ballX + Pong::BALL_SIZE >= SCREEN_WIDTH - 10 - Pong::PADDLE_WIDTH && Pong::ballX + Pong::BALL_SIZE <= SCREEN_WIDTH - 10) {
    if (Pong::ballY + Pong::BALL_SIZE >= Pong::paddle2Y && Pong::ballY <= Pong::paddle2Y + Pong::PADDLE_HEIGHT) {
      Pong::ballXSpeed = -Pong::ballXSpeed * 1.05;
      Pong::ballX = SCREEN_WIDTH - 10 - Pong::PADDLE_WIDTH - Pong::BALL_SIZE - 1;
      playPaddleHit();
    }
  }

  if (Pong::ballX < 0) {
    Pong::score2++;
    playScoreSound();
    Pong::resetBall();
  } else if (Pong::ballX > SCREEN_WIDTH) {
    Pong::score1++;
    playScoreSound();
    Pong::resetBall();
  }

  // 4. Render Game Objects
  if (Pong::paddle1Y != Pong::prevPaddle1Y) {
    tft.fillRect(10, Pong::prevPaddle1Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, ST77XX_BLACK);
    tft.fillRect(10, Pong::paddle1Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, getPaddleColor(0));
    Pong::prevPaddle1Y = Pong::paddle1Y;
  } else {
    tft.fillRect(10, Pong::paddle1Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, getPaddleColor(0));
  }

  if (Pong::paddle2Y != Pong::prevPaddle2Y) {
    tft.fillRect(SCREEN_WIDTH - 10 - Pong::PADDLE_WIDTH, Pong::prevPaddle2Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, ST77XX_BLACK);
    tft.fillRect(SCREEN_WIDTH - 10 - Pong::PADDLE_WIDTH, Pong::paddle2Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, getPaddleColor(1));
    Pong::prevPaddle2Y = Pong::paddle2Y;
  } else {
    tft.fillRect(SCREEN_WIDTH - 10 - Pong::PADDLE_WIDTH, Pong::paddle2Y, Pong::PADDLE_WIDTH, Pong::PADDLE_HEIGHT, getPaddleColor(1));
  }

  tft.fillRect(Pong::prevBallX, Pong::prevBallY, Pong::BALL_SIZE, Pong::BALL_SIZE, ST77XX_BLACK);
  tft.fillRect(Pong::ballX, Pong::ballY, Pong::BALL_SIZE, Pong::BALL_SIZE, ST77XX_WHITE);
  Pong::prevBallX = Pong::ballX;
  Pong::prevBallY = Pong::ballY;

  // Display speed multiplier in top-left
  tft.fillRect(0, 2, 25, 10, ST77XX_BLACK);
  tft.setCursor(0, 2); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(1);
  tft.print("S:");
  if (Pong::ballSpeedMultiplier < 1.0f) tft.setTextColor(ST77XX_CYAN);
  else if (Pong::ballSpeedMultiplier > 1.0f) tft.setTextColor(ST77XX_RED);
  else tft.setTextColor(ST77XX_WHITE);
  tft.print(Pong::ballSpeedMultiplier, 1);

  return true; // Continue game
}
