// Arkanoid (2-player variant) module
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bluepad32.h>
#include "game.h"

extern Adafruit_ST7789 tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

class ArkanoidGame : public Game
{
public:
  // Public API
  void init()
  {
    level = 1;
    paddle1X = 0;
    paddle2X = SCREEN_WIDTH / 2;
    prevPaddle1X = paddle1X;
    prevPaddle2X = paddle2X;

    // compute runtime dimensions
    PADDLE_Y = SCREEN_HEIGHT - PADDLE_HEIGHT - 2;

    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    prevBallX = ballX;
    prevBallY = ballY;
    ballXSpeed = (random(0, 2) == 0) ? 2.0f : -2.0f;
    ballYSpeed = random(-15, 16) / 10.0f;

    brickW = (SCREEN_WIDTH - 20) / BRICK_COLS;
    resetLevel();
    lastNewRowTime = millis();

    tft.fillScreen(ST77XX_BLACK);
    drawGameBorder();
    drawBricks();
    tft.fillRect(paddle1X, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(0));
    tft.fillRect(paddle2X, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(1));
  }

  bool update()
  {
    // Check for return to menu (home button)
    if (controller1 && controller1->isConnected() && controller1->miscBack())
      return false;

    if (controller2 && controller2->isConnected() && controller2->miscBack())
      return false;

    // Paddles - horizontal movement
    if (controller1 && controller1->isConnected())
    {
      int axis = controller1->axisX();
      if (axis < -200)
      {
        if (paddle1X > 0)
          paddle1X -= PADDLE_SPEED;
      }
      if (axis > 200)
      {
        if (paddle1X < SCREEN_WIDTH / 2 - PADDLE_WIDTH)
          paddle1X += PADDLE_SPEED;
      }

      // Ball speed control with L1/R1
      uint32_t now = millis();
      if (now - lastSpeedChangeTime > 150)
      { // Debounce 150ms
        uint16_t btns = controller1->buttons();
        if (btns & 0x04)
        { // L1 button code
          ballSpeedMultiplier = max(0.3f, ballSpeedMultiplier - 0.1f);
          lastSpeedChangeTime = now;
        }
        if (btns & 0x08)
        { // R1 button code
          ballSpeedMultiplier = min(2.0f, ballSpeedMultiplier + 0.1f);
          lastSpeedChangeTime = now;
        }
      }
    }

    if (controller2 && controller2->isConnected())
    {
      int axis = controller2->axisX();
      if (axis < -200)
      {
        if (paddle2X > SCREEN_WIDTH / 2)
          paddle2X -= PADDLE_SPEED;
      }
      if (axis > 200)
      {
        if (paddle2X < SCREEN_WIDTH - PADDLE_WIDTH)
          paddle2X += PADDLE_SPEED;
      }

      // Ball speed control with L1/R1 (P2 controller)
      uint32_t now = millis();
      if (now - lastSpeedChangeTime > 150)
      { // Debounce 150ms
        uint16_t btns = controller2->buttons();
        if (btns & 0x04)
        { // L1 button code
          ballSpeedMultiplier = max(0.3f, ballSpeedMultiplier - 0.1f);
          lastSpeedChangeTime = now;
        }
        if (btns & 0x08)
        { // R1 button code
          ballSpeedMultiplier = min(2.0f, ballSpeedMultiplier + 0.1f);
          lastSpeedChangeTime = now;
        }
      }
    }

    // ball physics - apply speed multiplier
    ballX += ballXSpeed * ballSpeedMultiplier;
    ballY += ballYSpeed * ballSpeedMultiplier;

    // wall collision (left/right)
    if (ballX <= 0 || ballX >= SCREEN_WIDTH - BALL_SIZE)
    {
      ballXSpeed = -ballXSpeed;
      playWallHit();
    }

    // top wall collision
    if (ballY <= 0)
    {
      ballYSpeed = -ballYSpeed;
      playWallHit();
    }

    // paddle collisions (credit lastHitBy) - horizontal paddles at bottom
    if (ballY + BALL_SIZE >= PADDLE_Y && ballY <= PADDLE_Y + PADDLE_HEIGHT)
    {
      // Paddle 1 (left half)
      if (ballX + BALL_SIZE >= paddle1X && ballX <= paddle1X + PADDLE_WIDTH)
      {
        ballYSpeed = -ballYSpeed;
        ballY = PADDLE_Y - BALL_SIZE;
        playPaddleHit();
        controller1->setRumble(255, 100);
        lastHitBy = 0;
      }
      // Paddle 2 (right half)
      if (ballX + BALL_SIZE >= paddle2X && ballX <= paddle2X + PADDLE_WIDTH)
      {
        ballYSpeed = -ballYSpeed;
        ballY = PADDLE_Y - BALL_SIZE;
        playPaddleHit();
        controller2->setRumble(255, 100);
        lastHitBy = 1;
      }
    }

    // brick collision
    for (int r = 0; r < brickRows; r++)
    {
      for (int c = 0; c < BRICK_COLS; c++)
      {
        if (!bricks[r][c])
          continue;
        int x = (SCREEN_WIDTH - (BRICK_COLS * brickW)) / 2 + c * brickW;
        int y = 8 + r * (brickH + 2);
        if (ballX + BALL_SIZE > x && ballX < x + brickW - 2 && ballY + BALL_SIZE > y && ballY < y + brickH)
        {
          uint8_t brickType = bricks[r][c];
          bricks[r][c] = 0;
          bricksRemaining--;
          drawBricks();
          ballYSpeed = -ballYSpeed;
          playWallHit();

          // Handle rewards based on brick type
          if (brickType == 2)
            playScoreSound(); // Extra ball reward sound
          if (brickType == 3)
            playScoreSound(); // Extra life reward sound
        }
      }
    }

    if (millis() - lastNewRowTime >= NEW_ROW_INTERVAL_MS)
    {
      addTopRow();
      lastNewRowTime = millis();
      drawBricks();
    }

    // Check if level complete
    if (bricksRemaining <= 0)
    {
      level++;
      resetLevel();
      ballX = SCREEN_WIDTH / 2;
      ballY = SCREEN_HEIGHT / 2;
      ballXSpeed = (random(0, 2) == 0) ? 2.0f : -2.0f;
      ballYSpeed = random(-15, 16) / 10.0f;
      tft.fillScreen(ST77XX_BLACK);
      drawBricks();
      playScoreSound();
    }

    // out of bounds (bottom) -> reset ball
    if (ballY > SCREEN_HEIGHT)
    {
      ballX = SCREEN_WIDTH / 2;
      ballY = SCREEN_HEIGHT / 2;
    }

    // render paddles
    if (paddle1X != prevPaddle1X)
    {
      tft.fillRect(prevPaddle1X, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST77XX_BLACK);
      tft.fillRect(paddle1X, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(0));
      prevPaddle1X = paddle1X;
    }
    if (paddle2X != prevPaddle2X)
    {
      tft.fillRect(prevPaddle2X, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST77XX_BLACK);
      tft.fillRect(paddle2X, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(1));
      prevPaddle2X = paddle2X;
    }

    // render ball
    tft.fillRect(prevBallX, prevBallY, BALL_SIZE, BALL_SIZE, ST77XX_BLACK);
    tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST77XX_WHITE);
    prevBallX = ballX;
    prevBallY = ballY;

    // Display level in top-right
    tft.fillRect(140, 2, 20, 10, ST77XX_BLACK);
    tft.setCursor(140, 2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.print("L");
    tft.print(level);

    // Display speed multiplier
    tft.fillRect(0, 2, 25, 10, ST77XX_BLACK);
    tft.setCursor(0, 2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.print("S:");
    if (ballSpeedMultiplier < 1.0f)
      tft.setTextColor(ST77XX_CYAN);
    else if (ballSpeedMultiplier > 1.0f)
      tft.setTextColor(ST77XX_RED);
    else
      tft.setTextColor(ST77XX_WHITE);
    tft.print(ballSpeedMultiplier, 1);

    return true; // Continue game
  }

  // Allow changing controllers without reinitializing game state
  void setControllers(ControllerPtr p1, ControllerPtr p2)
  {
    controller1 = p1;
    controller2 = p2;
  }

  const char *getName()
  {
    return "Arkanoid";
  }

private:
  // Paddles (horizontal at bottom)
  static const int PADDLE_WIDTH = 32;
  static const int PADDLE_HEIGHT = 6;
  static const int PADDLE_SPEED = 4;
  int PADDLE_Y = 0; // computed in init() because SCREEN_HEIGHT is not a compile-time constant

  // Ball
  static const int BALL_SIZE = 4;

  // Bricks - each brick has a type (0=none, 1=red, 2=yellow, 3=cyan)
  static const int MAX_BRICK_ROWS = 22;
  static const int INITIAL_BRICK_ROWS = 6;
  static const int BRICK_COLS = 10;
  static const uint32_t NEW_ROW_INTERVAL_MS = 10000;

  int brickW;
  int brickH = 8;
  uint8_t bricks[MAX_BRICK_ROWS][BRICK_COLS]; // 0=empty, 1-3=brick types
  int brickRows = INITIAL_BRICK_ROWS;
  uint32_t lastNewRowTime = 0;

  ControllerPtr controller1 = nullptr;
  ControllerPtr controller2 = nullptr;

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

  uint16_t getBrickColor(uint8_t type)
  {
    switch (type)
    {
    case 1:
      return ST77XX_RED; // Red = normal
    case 2:
      return ST77XX_YELLOW; // Yellow = extra ball
    case 3:
      return ST77XX_CYAN; // Cyan = extra life
    default:
      return ST77XX_BLACK;
    }
  }

  uint8_t newBrickType()
  {
    if (random(100) < (20 + level * 5))
      return 3; // Cyan
    if (random(100) < 30)
      return 2; // Yellow
    return 1;   // Red
  }

  void addTopRow()
  {
    if (brickRows == MAX_BRICK_ROWS)
    {
      for (int c = 0; c < BRICK_COLS; c++)
      {
        if (bricks[brickRows - 1][c])
          bricksRemaining--;
      }
    }
    else
    {
      brickRows++;
    }

    for (int r = brickRows - 1; r > 0; r--)
    {
      for (int c = 0; c < BRICK_COLS; c++)
      {
        bricks[r][c] = bricks[r - 1][c];
      }
    }

    for (int c = 0; c < BRICK_COLS; c++)
    {
      bricks[0][c] = newBrickType();
      bricksRemaining++;
    }
  }

  void resetLevel()
  {
    bricksRemaining = 0;
    brickRows = 0;
    for (int r = 0; r < MAX_BRICK_ROWS; r++)
    {
      for (int c = 0; c < BRICK_COLS; c++)
      {
        bricks[r][c] = 0;
      }
    }
    for (int r = 0; r < INITIAL_BRICK_ROWS; r++)
      addTopRow();
  }

  void drawBricks()
  {
    int brickAreaX = (SCREEN_WIDTH - (BRICK_COLS * brickW)) / 2;
    tft.fillRect(brickAreaX, 8, BRICK_COLS * brickW, MAX_BRICK_ROWS * (brickH + 2), ST77XX_BLACK);
    for (int r = 0; r < brickRows; r++)
    {
      for (int c = 0; c < BRICK_COLS; c++)
      {
        int x = (SCREEN_WIDTH - (BRICK_COLS * brickW)) / 2 + c * brickW;
        int y = 8 + r * (brickH + 2);
        if (bricks[r][c])
        {
          tft.fillRect(x, y, brickW - 2, brickH, getBrickColor(bricks[r][c]));
        }
        else
        {
          tft.fillRect(x, y, brickW - 2, brickH, ST77XX_BLACK);
        }
      }
    }
  }
};
