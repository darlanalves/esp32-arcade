// Pong game module (2-player)
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Bluepad32.h>

extern Adafruit_ST7735 tft;
extern ControllerPtr myControllers[];
extern void playPaddleHit();
extern void playWallHit();
extern void playScoreSound();
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const int DPAD_UP;
extern const int DPAD_DOWN;

const int PADDLE_WIDTH = 4;
const int PADDLE_HEIGHT = 24;
const int BALL_SIZE = 4;
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

void drawScores() {
  tft.fillRect(40, 2, 80, 12, ST7735_BLACK);
  tft.setCursor(50, 2);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.print(score1);
  tft.setCursor(100, 2);
  tft.print(score2);
}

void resetBall() {
  tft.fillRect(prevBallX - 1, prevBallY - 1, BALL_SIZE + 2, BALL_SIZE + 2, ST7735_BLACK);
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  ballXSpeed = (random(0, 2) == 0) ? 2.0 : -2.0;
  ballYSpeed = random(-15, 16) / 10.0;
  drawScores();
}

void Pong_init() {
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
  tft.fillRect(10, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
  tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
}

void Pong_update() {
  // 1. Process Controller 1 (Index 0) -> Left Paddle
  if (myControllers[0] && myControllers[0]->isConnected()) {
    uint8_t dpad = myControllers[0]->dpad();
    if ((dpad & DPAD_UP) || (myControllers[0]->axisY() < -200)) {
      if (paddle1Y > 2) paddle1Y -= PADDLE_SPEED;
    }
    if ((dpad & DPAD_DOWN) || (myControllers[0]->axisY() > 200)) {
      if (paddle1Y < SCREEN_HEIGHT - PADDLE_HEIGHT - 2) paddle1Y += PADDLE_SPEED;
    }
  }

  // 2. Process Controller 2 (Index 1) -> Right Paddle
  if (myControllers[1] && myControllers[1]->isConnected()) {
    uint8_t dpad = myControllers[1]->dpad();
    if ((dpad & DPAD_UP) || (myControllers[1]->axisY() < -200)) {
      if (paddle2Y > 2) paddle2Y -= PADDLE_SPEED;
    }
    if ((dpad & DPAD_DOWN) || (myControllers[1]->axisY() > 200)) {
      if (paddle2Y < SCREEN_HEIGHT - PADDLE_HEIGHT - 2) paddle2Y += PADDLE_SPEED;
    }
  }

  // 3. Update Ball Physics
  ballX += ballXSpeed;
  ballY += ballYSpeed;

  // wall hit
  if (ballY <= 0 || ballY >= SCREEN_HEIGHT - BALL_SIZE) {
    ballYSpeed = -ballYSpeed;
    playWallHit();
  }

  // p1 collision
  if (ballX <= 10 + PADDLE_WIDTH && ballX >= 10) {
    if (ballY + BALL_SIZE >= paddle1Y && ballY <= paddle1Y + PADDLE_HEIGHT) {
      ballXSpeed = -ballXSpeed * 1.05;
      ballX = 10 + PADDLE_WIDTH + 1;
      playPaddleHit();
    }
  }

  // p2 collision
  if (ballX + BALL_SIZE >= SCREEN_WIDTH - 10 - PADDLE_WIDTH && ballX + BALL_SIZE <= SCREEN_WIDTH - 10) {
    if (ballY + BALL_SIZE >= paddle2Y && ballY <= paddle2Y + PADDLE_HEIGHT) {
      ballXSpeed = -ballXSpeed * 1.05;
      ballX = SCREEN_WIDTH - 10 - PADDLE_WIDTH - BALL_SIZE - 1;
      playPaddleHit();
    }
  }

  if (ballX < 0) {
    score2++;
    playScoreSound();
    resetBall();
  } else if (ballX > SCREEN_WIDTH) {
    score1++;
    playScoreSound();
    resetBall();
  }

  // 4. Render Game Objects
  if (paddle1Y != prevPaddle1Y) {
    tft.fillRect(10, prevPaddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_BLACK);
    tft.fillRect(10, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
    prevPaddle1Y = paddle1Y;
  } else {
    tft.fillRect(10, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
  }

  if (paddle2Y != prevPaddle2Y) {
    tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, prevPaddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_BLACK);
    tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
    prevPaddle2Y = paddle2Y;
  } else {
    tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
  }

  tft.fillRect(prevBallX, prevBallY, BALL_SIZE, BALL_SIZE, ST7735_BLACK);
  tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST7735_WHITE);
  prevBallX = ballX;
  prevBallY = ballY;
}
