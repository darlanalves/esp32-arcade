// Arkanoid (2-player variant) module
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

// Paddles (left/right like Pong but bricks in the center/top)
const int PADDLE_WIDTH = 4;
const int PADDLE_HEIGHT = 24;
const int PADDLE_SPEED = 4;

// Ball
const int BALL_SIZE = 4;

// Bricks
const int BRICK_ROWS = 4;
const int BRICK_COLS = 10;
int brickW;
int brickH = 8;
bool bricks[BRICK_ROWS][BRICK_COLS];

int paddle1Y;
int paddle2Y;
int prevPaddle1Y, prevPaddle2Y;

float ballX, ballY;
float prevBallX, prevBallY;
float ballXSpeed, ballYSpeed;

int score1 = 0;
int score2 = 0;
int lastHitBy = -1; // 0 left, 1 right

void drawBricks() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      int x = (SCREEN_WIDTH - (BRICK_COLS * brickW)) / 2 + c * brickW;
      int y = 8 + r * (brickH + 2);
      if (bricks[r][c]) tft.fillRect(x, y, brickW-2, brickH, ST7735_RED);
      else tft.fillRect(x, y, brickW-2, brickH, ST7735_BLACK);
    }
  }
}

void Arkanoid_init() {
  paddle1Y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
  paddle2Y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
  prevPaddle1Y = paddle1Y;
  prevPaddle2Y = paddle2Y;

  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  prevBallX = ballX;
  prevBallY = ballY;
  ballXSpeed = (random(0,2)==0)?2.0:-2.0;
  ballYSpeed = random(-15,16)/10.0;

  brickW = (SCREEN_WIDTH - 20) / BRICK_COLS;
  for (int r=0;r<BRICK_ROWS;r++) for (int c=0;c<BRICK_COLS;c++) bricks[r][c]=true;

  score1 = 0; score2 = 0; lastHitBy = -1;

  tft.fillScreen(ST7735_BLACK);
  drawBricks();
  tft.fillRect(10, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
  tft.fillRect(SCREEN_WIDTH - 10 - PADDLE_WIDTH, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ST7735_WHITE);
}

void Arkanoid_update() {
  // paddles
  if (myControllers[0] && myControllers[0]->isConnected()) {
    uint8_t d = myControllers[0]->dpad();
    if ((d & DPAD_UP) || myControllers[0]->axisY() < -200) { if (paddle1Y>2) paddle1Y -= PADDLE_SPEED; }
    if ((d & DPAD_DOWN) || myControllers[0]->axisY() > 200) { if (paddle1Y < SCREEN_HEIGHT-PADDLE_HEIGHT-2) paddle1Y += PADDLE_SPEED; }
  }
  if (myControllers[1] && myControllers[1]->isConnected()) {
    uint8_t d = myControllers[1]->dpad();
    if ((d & DPAD_UP) || myControllers[1]->axisY() < -200) { if (paddle2Y>2) paddle2Y -= PADDLE_SPEED; }
    if ((d & DPAD_DOWN) || myControllers[1]->axisY() > 200) { if (paddle2Y < SCREEN_HEIGHT-PADDLE_HEIGHT-2) paddle2Y += PADDLE_SPEED; }
  }

  // ball physics
  ballX += ballXSpeed;
  ballY += ballYSpeed;

  // wall collision
  if (ballY <= 0 || ballY >= SCREEN_HEIGHT - BALL_SIZE) { ballYSpeed = -ballYSpeed; playWallHit(); }

  // paddle collisions (credit lastHitBy)
  if (ballX <= 10 + PADDLE_WIDTH && ballX >= 10) {
    if (ballY + BALL_SIZE >= paddle1Y && ballY <= paddle1Y + PADDLE_HEIGHT) {
      ballXSpeed = -ballXSpeed; ballX = 10 + PADDLE_WIDTH + 1; playPaddleHit(); lastHitBy = 0;
    }
  }
  if (ballX + BALL_SIZE >= SCREEN_WIDTH - 10 - PADDLE_WIDTH && ballX + BALL_SIZE <= SCREEN_WIDTH - 10) {
    if (ballY + BALL_SIZE >= paddle2Y && ballY <= paddle2Y + PADDLE_HEIGHT) {
      ballXSpeed = -ballXSpeed; ballX = SCREEN_WIDTH - 10 - PADDLE_WIDTH - BALL_SIZE - 1; playPaddleHit(); lastHitBy = 1;
    }
  }

  // brick collision
  for (int r=0;r<BRICK_ROWS;r++){
    for (int c=0;c<BRICK_COLS;c++){
      if (!bricks[r][c]) continue;
      int x = (SCREEN_WIDTH - (BRICK_COLS * brickW)) / 2 + c * brickW;
      int y = 8 + r * (brickH + 2);
      if (ballX + BALL_SIZE > x && ballX < x + brickW-2 && ballY + BALL_SIZE > y && ballY < y + brickH) {
        bricks[r][c] = false;
        drawBricks();
        ballYSpeed = -ballYSpeed;
        playWallHit();
        if (lastHitBy == 0) score1 += 1; else if (lastHitBy == 1) score2 += 1;
      }
    }
  }

  // out of bounds -> score like Pong
  if (ballX < 0) { score2++; playScoreSound(); ballX = SCREEN_WIDTH/2; ballY = SCREEN_HEIGHT/2; }
  else if (ballX > SCREEN_WIDTH) { score1++; playScoreSound(); ballX = SCREEN_WIDTH/2; ballY = SCREEN_HEIGHT/2; }

  // render paddles
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

  // render ball
  tft.fillRect(prevBallX, prevBallY, BALL_SIZE, BALL_SIZE, ST7735_BLACK);
  tft.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, ST7735_WHITE);
  prevBallX = ballX; prevBallY = ballY;

  // optional score display
  tft.setCursor(50, 2); tft.setTextColor(ST7735_WHITE); tft.setTextSize(1); tft.print(score1);
  tft.setCursor(100, 2); tft.print(score2);
}
