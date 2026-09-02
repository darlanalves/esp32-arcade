// Asteroids game module
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bluepad32.h>
#include <math.h>
#include <string.h>

extern Adafruit_ST7789 tft;
extern void playPaddleHit();
extern void playWallHit();
extern void playScoreSound();
extern uint16_t getPaddleColor(int playerIndex);
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const uint8_t GAME_DPAD_UP;
extern const uint8_t GAME_DPAD_DOWN;

class AsteroidsGame : public Game
{
public:
  void setControllers(ControllerPtr p1, ControllerPtr p2) override
  {
    ctrls[0] = p1;
    ctrls[1] = p2;
  }

  void init() override
  {
    centerX = SCREEN_WIDTH / 2;
    centerY = (SCREEN_HEIGHT - 20) / 2;
    playfieldHeight = SCREEN_HEIGHT - 20;

    shipX = centerX;
    shipY = centerY;
    shipVelX = 0;
    shipVelY = 0;
    shipAngle = 0;
    shipSpeed = 0;

    score = 0;
    lives = 3;
    speedMultiplier = 1.0f;

    for (int i = 0; i < MAX_BULLETS; i++)
      bullets[i].active = false;
    spawnAsteroids(3);

    tft.fillScreen(ST77XX_BLACK);
    drawAsteroidsFrame();
  }

  bool update() override
  {
    // Check for return to menu
    if (ctrls[0] && ctrls[0]->isConnected() && ctrls[0]->miscButtons())
      return false;

    if (ctrls[0] && ctrls[0]->isConnected())
    {
      int axisX = ctrls[0]->axisX();
      int axisY = ctrls[0]->axisY();
      uint16_t btns = ctrls[0]->buttons();

      if (axisX != 0 || axisY != 0)
        shipAngle = atan2(-axisY, axisX) * 180.0 / M_PI + 90;

      if (btns & 0x01 || btns & 0x02)
        shipSpeed = min(MAX_SHIP_SPEED, shipSpeed + 0.15f);
      else
        shipSpeed *= 0.95f;

      if ((btns & 0x04) || (btns & 0x08))
      {
        uint32_t now = millis();
        if (now - lastShotTime > 200)
        {
          for (int i = 0; i < MAX_BULLETS; i++)
          {
            if (!bullets[i].active)
            {
              float rad = shipAngle * M_PI / 180.0;
              bullets[i].x = shipX + cos(rad) * SHIP_SIZE;
              bullets[i].y = shipY - sin(rad) * SHIP_SIZE;
              bullets[i].velX = cos(rad) * 4.0f;
              bullets[i].velY = -sin(rad) * 4.0f;
              bullets[i].active = true;
              playPaddleHit();
              lastShotTime = now;
              break;
            }
          }
        }
      }

      uint32_t now = millis();
      if (now - lastSpeedChangeTime > 150)
      {
        if (btns & 0x10)
        {
          speedMultiplier = max(0.3f, speedMultiplier - 0.1f);
          lastSpeedChangeTime = now;
        }
        if (btns & 0x20)
        {
          speedMultiplier = min(2.0f, speedMultiplier + 0.1f);
          lastSpeedChangeTime = now;
        }
      }
    }

    // Update physics
    float rad = shipAngle * M_PI / 180.0;
    shipVelX = cos(rad) * shipSpeed;
    shipVelY = -sin(rad) * shipSpeed;
    shipX += shipVelX;
    shipY += shipVelY;

    if (shipX < 0)
      shipX = SCREEN_WIDTH;
    if (shipX > SCREEN_WIDTH)
      shipX = 0;
    if (shipY < 0)
      shipY = SCREEN_HEIGHT - 20;
    if (shipY > SCREEN_HEIGHT - 20)
      shipY = 0;

    for (int i = 0; i < MAX_BULLETS; i++)
    {
      if (bullets[i].active)
      {
        bullets[i].x += bullets[i].velX;
        bullets[i].y += bullets[i].velY;
        if (bullets[i].x < 0 || bullets[i].x > SCREEN_WIDTH || bullets[i].y < 0 || bullets[i].y > SCREEN_HEIGHT - 20)
          bullets[i].active = false;
      }
    }

    for (int i = 0; i < asteroidCount; i++)
    {
      if (asteroids[i].active)
      {
        asteroids[i].x += asteroids[i].velX;
        asteroids[i].y += asteroids[i].velY;
        if (asteroids[i].x < 0)
          asteroids[i].x = SCREEN_WIDTH;
        if (asteroids[i].x > SCREEN_WIDTH)
          asteroids[i].x = 0;
        if (asteroids[i].y < 0)
          asteroids[i].y = SCREEN_HEIGHT - 20;
        if (asteroids[i].y > SCREEN_HEIGHT - 20)
          asteroids[i].y = 0;

        for (int j = 0; j < MAX_BULLETS; j++)
        {
          if (bullets[j].active)
          {
            int radius = 10 - asteroids[i].size * 3;
            float dx = bullets[j].x - asteroids[i].x;
            float dy = bullets[j].y - asteroids[i].y;
            if (dx * dx + dy * dy < radius * radius)
            {
              bullets[j].active = false;
              asteroids[i].active = false;
              score += (3 - asteroids[i].size) * 10;
              playScoreSound();
              if (asteroids[i].size < 2 && asteroidCount < MAX_ASTEROIDS - 1)
              {
                for (int k = 0; k < 2; k++)
                {
                  asteroids[asteroidCount].x = asteroids[i].x;
                  asteroids[asteroidCount].y = asteroids[i].y;
                  asteroids[asteroidCount].velX = (random(-100, 100) / 100.0f);
                  asteroids[asteroidCount].velY = (random(-100, 100) / 100.0f);
                  asteroids[asteroidCount].size = asteroids[i].size + 1;
                  asteroids[asteroidCount].active = true;
                  asteroidCount++;
                }
              }
            }
          }
        }
      }
    }

    bool allDestroyed = true;
    for (int i = 0; i < asteroidCount; i++)
      if (asteroids[i].active)
      {
        allDestroyed = false;
        break;
      }
    if (allDestroyed)
      spawnAsteroids(3 + score / 100);

    drawAsteroidsFrame();

    // HUD
    tft.fillRect(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, 18, ST77XX_BLACK);
    tft.drawLine(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, SCREEN_HEIGHT - 18, ST77XX_WHITE);
    tft.setCursor(5, SCREEN_HEIGHT - 15);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.print("Score:");
    tft.print(score);
    tft.setCursor(SCREEN_WIDTH - 45, SCREEN_HEIGHT - 15);
    tft.print("Lives:");
    tft.print(lives);

    return true;
  }

  const char *getName() override { return "Asteroids"; }

private:
  // controllers
  ControllerPtr ctrls[2] = {nullptr, nullptr};

  // constants
  static const int TILE_HEIGHT = 32;
  static const int MAX_ASTEROIDS = 20;
  static const int MAX_BULLETS = 10;
  static constexpr float MAX_SHIP_SPEED = 3.0f;
  static constexpr float SHIP_SIZE = 8.0f;

  // layout (computed in init)
  int centerX;
  int centerY;
  int playfieldHeight;

  // ship
  float shipX, shipY;
  float shipVelX, shipVelY;
  float shipAngle;
  float shipSpeed;

  struct Bullet
  {
    float x, y, velX, velY;
    bool active;
  } bullets[MAX_BULLETS];

  struct Asteroid
  {
    float x, y, velX, velY;
    int size;
    bool active;
  } asteroids[MAX_ASTEROIDS];
  int asteroidCount = 0;

  int score = 0;
  int lives = 3;
  float speedMultiplier = 1.0f;
  uint32_t lastSpeedChangeTime = 0;
  uint32_t lastShotTime = 0;
  uint16_t frameBuffer[TILE_HEIGHT][240];
  int renderTop = 0;

  void drawPixel(int x, int y, uint16_t color)
  {
    if (x >= 0 && x < SCREEN_WIDTH && y >= renderTop && y < renderTop + TILE_HEIGHT)
      frameBuffer[y - renderTop][x] = color;
  }

  void drawLine(int x0, int y0, int x1, int y1, uint16_t color)
  {
    int deltaX = abs(x1 - x0);
    int stepX = x0 < x1 ? 1 : -1;
    int deltaY = -abs(y1 - y0);
    int stepY = y0 < y1 ? 1 : -1;
    int error = deltaX + deltaY;
    while (true)
    {
      drawPixel(x0, y0, color);
      if (x0 == x1 && y0 == y1)
        break;
      int doubleError = error * 2;
      if (doubleError >= deltaY)
      {
        error += deltaY;
        x0 += stepX;
      }
      if (doubleError <= deltaX)
      {
        error += deltaX;
        y0 += stepY;
      }
    }
  }

  void drawBorder()
  {
    for (int offset = 0; offset < 3; offset++)
    {
      int right = SCREEN_WIDTH - 1 - offset;
      int bottom = playfieldHeight - 1 - offset;
      drawLine(offset, offset, right, offset, ST77XX_WHITE);
      drawLine(right, offset, right, bottom, ST77XX_WHITE);
      drawLine(right, bottom, offset, bottom, ST77XX_WHITE);
      drawLine(offset, bottom, offset, offset, ST77XX_WHITE);
    }
  }

  void drawShip()
  {
    float rad = shipAngle * M_PI / 180.0;
    float cos_a = cos(rad);
    float sin_a = sin(rad);
    int x1 = shipX + cos_a * SHIP_SIZE;
    int y1 = shipY - sin_a * SHIP_SIZE;
    int x2 = shipX - cos_a * (SHIP_SIZE * 0.7) - sin_a * (SHIP_SIZE * 0.7);
    int y2 = shipY + sin_a * (SHIP_SIZE * 0.7) - cos_a * (SHIP_SIZE * 0.7);
    int x3 = shipX - cos_a * (SHIP_SIZE * 0.7) + sin_a * (SHIP_SIZE * 0.7);
    int y3 = shipY + sin_a * (SHIP_SIZE * 0.7) + cos_a * (SHIP_SIZE * 0.7);
    drawLine(x1, y1, x2, y2, getPaddleColor(0));
    drawLine(x2, y2, x3, y3, getPaddleColor(0));
    drawLine(x3, y3, x1, y1, getPaddleColor(0));
  }

  void drawAsteroidShape(float x, float y, int size)
  {
    int radius = 10 - size * 3;
    for (int i = 0; i < 8; i++)
    {
      float a1 = (float)i * M_PI / 4.0;
      float a2 = (float)(i + 1) * M_PI / 4.0;
      int x1 = x + cos(a1) * radius;
      int y1 = y + sin(a1) * radius;
      int x2 = x + cos(a2) * radius;
      int y2 = y + sin(a2) * radius;
      drawLine(x1, y1, x2, y2, ST77XX_WHITE);
    }
  }

  void spawnAsteroids(int count)
  {
    asteroidCount = 0;
    for (int i = 0; i < count && asteroidCount < MAX_ASTEROIDS; i++)
    {
      asteroids[asteroidCount].x = random(20, SCREEN_WIDTH - 20);
      asteroids[asteroidCount].y = random(20, SCREEN_HEIGHT - 40);
      asteroids[asteroidCount].velX = (random(-100, 100) / 100.0f) * speedMultiplier;
      asteroids[asteroidCount].velY = (random(-100, 100) / 100.0f) * speedMultiplier;
      asteroids[asteroidCount].size = 0;
      asteroids[asteroidCount].active = true;
      asteroidCount++;
    }
  }

  void drawAsteroidsFrame()
  {
    for (int rt = 0; rt < playfieldHeight; rt += TILE_HEIGHT)
    {
      int tileHeight = min(TILE_HEIGHT, playfieldHeight - rt);
      renderTop = rt;
      memset(frameBuffer, 0, sizeof(frameBuffer));
      drawBorder();
      drawShip();
      for (int i = 0; i < MAX_BULLETS; i++)
      {
        if (bullets[i].active)
        {
          drawPixel(bullets[i].x, bullets[i].y, ST77XX_YELLOW);
          drawPixel(bullets[i].x + 1, bullets[i].y, ST77XX_YELLOW);
        }
      }
      for (int i = 0; i < asteroidCount; i++)
        if (asteroids[i].active)
          drawAsteroidShape(asteroids[i].x, asteroids[i].y, asteroids[i].size);
      tft.drawRGBBitmap(0, rt, &frameBuffer[0][0], SCREEN_WIDTH, tileHeight);
    }
  }
};
