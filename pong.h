// Pong game module (2-player)
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bluepad32.h>

extern Adafruit_ST7789 tft;
extern void playPaddleHit();
extern void playWallHit();
extern void playScoreSound();
extern void drawGameBorder(Adafruit_GFX &display);
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const uint8_t DPAD_UP;
extern const uint8_t DPAD_DOWN;

class PongGame : public Game
{
public:
  void setControllers(ControllerPtr p1, ControllerPtr p2) override
  {
    ctrls[0] = p1;
    ctrls[1] = p2;
  }

  void init() override
  {
    paddle1Y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    paddle2Y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    prevPaddle1Y = paddle1Y;
    prevPaddle2Y = paddle2Y;

    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    prevBallX = ballX;
    prevBallY = ballY;

    score1 = 0;
    score2 = 0;

    resetBall();
    drawScores();

    // draw initial paddles
    tft.fillRect(10, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(0));
    tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(1));
  }

  bool update() override
  {
    // Check for return to menu (home button)
    if (controller1 && controller1->isConnected() && controller1->miscBack())
      return false;

    if (controller2 && controller2->isConnected() && controller2->miscBack())
      return false;

    // Player 1
    if (ctrls[0] && ctrls[0]->isConnected())
    {
      uint8_t dpad = ctrls[0]->dpad();
      if ((dpad & DPAD_UP) || (ctrls[0]->axisY() < -200))
      {
        if (paddle1Y > 2)
          paddle1Y -= PADDLE_SPEED;
      }
      if ((dpad & DPAD_DOWN) || (ctrls[0]->axisY() > 200))
      {
        if (paddle1Y < SCREEN_HEIGHT - PADDLE_HEIGHT - 2)
          paddle1Y += PADDLE_SPEED;
      }

      uint32_t now = millis();
      if (now - lastSpeedChangeTime > 150)
      {
        uint16_t btns = ctrls[0]->buttons();
        if (btns & 0x04)
        {
          ballSpeedMultiplier = max(0.3f, ballSpeedMultiplier - 0.1f);
          lastSpeedChangeTime = now;
        }
        if (btns & 0x08)
        {
          ballSpeedMultiplier = min(2.0f, ballSpeedMultiplier + 0.1f);
          lastSpeedChangeTime = now;
        }
      }
    }

    // Player 2
    if (ctrls[1] && ctrls[1]->isConnected())
    {
      uint8_t dpad = ctrls[1]->dpad();
      if ((dpad & DPAD_UP) || (ctrls[1]->axisY() < -200))
      {
        if (paddle2Y > 2)
          paddle2Y -= PADDLE_SPEED;
      }
      if ((dpad & DPAD_DOWN) || (ctrls[1]->axisY() > 200))
      {
        if (paddle2Y < SCREEN_HEIGHT - PADDLE_HEIGHT - 2)
          paddle2Y += PADDLE_SPEED;
      }

      uint32_t now = millis();
      if (now - lastSpeedChangeTime > 150)
      {
        uint16_t btns = ctrls[1]->buttons();
        if (btns & 0x04)
        {
          ballSpeedMultiplier = max(0.3f, ballSpeedMultiplier - 0.1f);
          lastSpeedChangeTime = now;
        }
        if (btns & 0x08)
        {
          ballSpeedMultiplier = min(2.0f, ballSpeedMultiplier + 0.1f);
          lastSpeedChangeTime = now;
        }
      }
    }

    // Ball physics
    ballX += ballXSpeed * ballSpeedMultiplier;
    ballY += ballYSpeed * ballSpeedMultiplier;

    if (ballY <= 0 || ballY >= SCREEN_HEIGHT - BALL_SIZE)
    {
      ballYSpeed = -ballYSpeed;
      playWallHit();
    }

    // p1 collision
    if (ballX <= 10 + PADDLE_WIDTH && ballX >= 10)
    {
      if (ballY + BALL_SIZE >= paddle1Y && ballY <= paddle1Y + PADDLE_HEIGHT)
      {
        ballXSpeed = -ballXSpeed * 1.05;
        ballX = 10 + PADDLE_WIDTH + 1;
        playPaddleHit();
      }
    }

    // p2 collision
    if (ballX + BALL_SIZE >= SCREEN_WIDTH - 10 - PADDLE_WIDTH && ballX + BALL_SIZE <= SCREEN_WIDTH - 10)
    {
      if (ballY + BALL_SIZE >= paddle2Y && ballY <= paddle2Y + PADDLE_HEIGHT)
      {
        ballXSpeed = -ballXSpeed * 1.05;
        ballX = SCREEN_WIDTH - 10 - PADDLE_WIDTH - BALL_SIZE - 1;
        playPaddleHit();
      }
    }

    if (ballX < 0)
    {
      score2++;
      playScoreSound();
      resetBall();
    }
    else if (ballX > SCREEN_WIDTH)
    {
      score1++;
      playScoreSound();
      resetBall();
    }

    // Render
    if (paddle1Y != prevPaddle1Y)
    {
      tft.fillRect(10, prevPaddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST77XX_BLACK);
      tft.fillRect(10, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(0));
      prevPaddle1Y = paddle1Y;
    }
    else
    {
      tft.fillRect(10, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(0));
    }

    if (paddle2Y != prevPaddle2Y)
    {
      tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, prevPaddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST77XX_BLACK);
      tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(1));
      prevPaddle2Y = paddle2Y;
    }
    else
    {
      tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, getPaddleColor(1));
    }

    tft.fillRect(prevBallX, prevBallY, BALL_SIZE, BALL_SIZE, ST77XX_BLACK);
    tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST77XX_WHITE);
    prevBallX = ballX;
    prevBallY = ballY;

    // Speed multiplier display
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

    return true;
  }

  const char *getName() override { return "Pong"; }

private:
  // constants
  static const int PADDLE_WIDTH = 6;
  static const int PADDLE_HEIGHT = 30;
  static const int BALL_SIZE = 8;
  static const int PADDLE_SPEED = 4;

  // controllers
  ControllerPtr ctrls[2] = {nullptr, nullptr};

  // paddles
  int paddle1Y;
  int paddle2Y;
  int prevPaddle1Y;
  int prevPaddle2Y;

  // ball
  float ballX, ballY;
  float prevBallX, prevBallY;
  float ballXSpeed, ballYSpeed;

  int score1 = 0;
  int score2 = 0;

  float ballSpeedMultiplier = 1.0f;
  uint32_t lastSpeedChangeTime = 0;

  void drawScores()
  {
    tft.fillRect(40, 2, 80, 12, ST77XX_BLACK);
    tft.setCursor(50, 2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.print(score1);
    tft.setCursor(100, 2);
    tft.print(score2);
  }

  void resetBall()
  {
    tft.fillRect(prevBallX - 1, prevBallY - 1, BALL_SIZE + 2, BALL_SIZE + 2, ST77XX_BLACK);
    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    ballXSpeed = (random(0, 2) == 0) ? 2.0f : -2.0f;
    ballYSpeed = random(-15, 16) / 10.0f;
    drawScores();
  }
};
