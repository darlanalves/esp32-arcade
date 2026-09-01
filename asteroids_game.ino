// Asteroids game module
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bluepad32.h>
#include <math.h>

extern Adafruit_ST7789 tft;
extern void playPaddleHit();
extern void playWallHit();
extern void playScoreSound();
extern uint16_t getPaddleColor(int playerIndex);
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const uint8_t GAME_DPAD_UP;
extern const uint8_t GAME_DPAD_DOWN;

namespace Asteroids {
  // Game constants
  const int CENTER_X = SCREEN_WIDTH / 2;
  const int CENTER_Y = (SCREEN_HEIGHT - 20) / 2;
  const int MAX_ASTEROIDS = 20;
  const int MAX_BULLETS = 10;
  
  // Player ship
  float shipX, shipY;
  float shipVelX, shipVelY;
  float shipAngle;  // In degrees
  float shipSpeed = 0;
  const float MAX_SHIP_SPEED = 3.0f;
  const float SHIP_SIZE = 8;
  
  // Bullets
  struct Bullet {
    float x, y;
    float velX, velY;
    bool active;
  };
  Bullet bullets[MAX_BULLETS];
  
  // Asteroids
  struct Asteroid {
    float x, y;
    float velX, velY;
    int size;  // 0 = large, 1 = medium, 2 = small
    bool active;
  };
  Asteroid asteroids[MAX_ASTEROIDS];
  int asteroidCount = 0;
  
  int score = 0;
  int lives = 3;
  float speedMultiplier = 1.0f;
  uint32_t lastSpeedChangeTime = 0;
  uint32_t lastShotTime = 0;
  
  void drawShip() {
    float rad = shipAngle * M_PI / 180.0;
    float cos_a = cos(rad);
    float sin_a = sin(rad);
    
    // Ship points (triangle)
    int x1 = shipX + cos_a * SHIP_SIZE;
    int y1 = shipY - sin_a * SHIP_SIZE;
    int x2 = shipX - cos_a * (SHIP_SIZE * 0.7) - sin_a * (SHIP_SIZE * 0.7);
    int y2 = shipY + sin_a * (SHIP_SIZE * 0.7) - cos_a * (SHIP_SIZE * 0.7);
    int x3 = shipX - cos_a * (SHIP_SIZE * 0.7) + sin_a * (SHIP_SIZE * 0.7);
    int y3 = shipY + sin_a * (SHIP_SIZE * 0.7) + cos_a * (SHIP_SIZE * 0.7);
    
    tft.drawLine(x1, y1, x2, y2, getPaddleColor(0));
    tft.drawLine(x2, y2, x3, y3, getPaddleColor(0));
    tft.drawLine(x3, y3, x1, y1, getPaddleColor(0));
  }
  
  void drawAsteroid(float x, float y, int size) {
    int radius = 4 + size * 3;
    for (int i = 0; i < 8; i++) {
      float a1 = (float)i * M_PI / 4.0;
      float a2 = (float)(i + 1) * M_PI / 4.0;
      int x1 = x + cos(a1) * radius;
      int y1 = y + sin(a1) * radius;
      int x2 = x + cos(a2) * radius;
      int y2 = y + sin(a2) * radius;
      tft.drawLine(x1, y1, x2, y2, ST77XX_WHITE);
    }
  }
  
  void spawnAsteroids(int count) {
    asteroidCount = 0;
    for (int i = 0; i < count && asteroidCount < MAX_ASTEROIDS; i++) {
      asteroids[asteroidCount].x = random(20, SCREEN_WIDTH - 20);
      asteroids[asteroidCount].y = random(20, SCREEN_HEIGHT - 40);
      asteroids[asteroidCount].velX = (random(-100, 100) / 100.0f) * speedMultiplier;
      asteroids[asteroidCount].velY = (random(-100, 100) / 100.0f) * speedMultiplier;
      asteroids[asteroidCount].size = 0;  // Large
      asteroids[asteroidCount].active = true;
      asteroidCount++;
    }
  }
}

void Asteroids_init(ControllerPtr myControllers[]) {
  Asteroids::shipX = Asteroids::CENTER_X;
  Asteroids::shipY = Asteroids::CENTER_Y;
  Asteroids::shipVelX = 0;
  Asteroids::shipVelY = 0;
  Asteroids::shipAngle = 0;
  Asteroids::shipSpeed = 0;
  
  Asteroids::score = 0;
  Asteroids::lives = 3;
  Asteroids::speedMultiplier = 1.0f;
  
  for (int i = 0; i < Asteroids::MAX_BULLETS; i++) {
    Asteroids::bullets[i].active = false;
  }
  
  Asteroids::spawnAsteroids(3);
  
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - 18, ST77XX_WHITE);
}

bool Asteroids_update(ControllerPtr myControllers[]) {
  // Check for return to menu (home button)
  if (myControllers[0] && myControllers[0]->isConnected() && myControllers[0]->miscButtons()) {
    return false;
  }
  
  // Input handling
  if (myControllers[0] && myControllers[0]->isConnected()) {
    int axisX = myControllers[0]->axisX();
    int axisY = myControllers[0]->axisY();
    uint16_t btns = myControllers[0]->buttons();
    
    // Rotation with analog stick
    if (axisX != 0 || axisY != 0) {
      Asteroids::shipAngle = atan2(-axisY, axisX) * 180.0 / M_PI + 90;
    }
    
    // Accelerate with button A or X
    if (btns & 0x01 || btns & 0x02) {  // A (0x01) or X (0x02)
      Asteroids::shipSpeed = min(Asteroids::MAX_SHIP_SPEED, Asteroids::shipSpeed + 0.15f);
    } else {
      Asteroids::shipSpeed *= 0.95f;  // Friction
    }
    
    // Shoot with button B or Y
    if ((btns & 0x04) || (btns & 0x08)) {  // B (0x04) or Y (0x08)
      uint32_t now = millis();
      if (now - Asteroids::lastShotTime > 200) {
        // Find inactive bullet
        for (int i = 0; i < Asteroids::MAX_BULLETS; i++) {
          if (!Asteroids::bullets[i].active) {
            float rad = Asteroids::shipAngle * M_PI / 180.0;
            Asteroids::bullets[i].x = Asteroids::shipX + cos(rad) * Asteroids::SHIP_SIZE;
            Asteroids::bullets[i].y = Asteroids::shipY - sin(rad) * Asteroids::SHIP_SIZE;
            Asteroids::bullets[i].velX = cos(rad) * 4.0f;
            Asteroids::bullets[i].velY = -sin(rad) * 4.0f;
            Asteroids::bullets[i].active = true;
            playPaddleHit();
            Asteroids::lastShotTime = now;
            break;
          }
        }
      }
    }
    
    // Speed control with L1/R1
    uint32_t now = millis();
    if (now - Asteroids::lastSpeedChangeTime > 150) {
      if (btns & 0x10) {  // L2 button
        Asteroids::speedMultiplier = max(0.3f, Asteroids::speedMultiplier - 0.1f);
        Asteroids::lastSpeedChangeTime = now;
      }
      if (btns & 0x20) {  // R2 button
        Asteroids::speedMultiplier = min(2.0f, Asteroids::speedMultiplier + 0.1f);
        Asteroids::lastSpeedChangeTime = now;
      }
    }
  }
  
  // Update ship position
  float rad = Asteroids::shipAngle * M_PI / 180.0;
  Asteroids::shipVelX = cos(rad) * Asteroids::shipSpeed;
  Asteroids::shipVelY = -sin(rad) * Asteroids::shipSpeed;
  Asteroids::shipX += Asteroids::shipVelX;
  Asteroids::shipY += Asteroids::shipVelY;
  
  // Wrap around screen
  if (Asteroids::shipX < 0) Asteroids::shipX = SCREEN_WIDTH;
  if (Asteroids::shipX > SCREEN_WIDTH) Asteroids::shipX = 0;
  if (Asteroids::shipY < 0) Asteroids::shipY = SCREEN_HEIGHT - 20;
  if (Asteroids::shipY > SCREEN_HEIGHT - 20) Asteroids::shipY = 0;
  
  // Update bullets
  for (int i = 0; i < Asteroids::MAX_BULLETS; i++) {
    if (Asteroids::bullets[i].active) {
      Asteroids::bullets[i].x += Asteroids::bullets[i].velX;
      Asteroids::bullets[i].y += Asteroids::bullets[i].velY;
      
      if (Asteroids::bullets[i].x < 0 || Asteroids::bullets[i].x > SCREEN_WIDTH ||
          Asteroids::bullets[i].y < 0 || Asteroids::bullets[i].y > SCREEN_HEIGHT - 20) {
        Asteroids::bullets[i].active = false;
      }
    }
  }
  
  // Update asteroids
  for (int i = 0; i < Asteroids::asteroidCount; i++) {
    if (Asteroids::asteroids[i].active) {
      Asteroids::asteroids[i].x += Asteroids::asteroids[i].velX;
      Asteroids::asteroids[i].y += Asteroids::asteroids[i].velY;
      
      // Wrap around
      if (Asteroids::asteroids[i].x < 0) Asteroids::asteroids[i].x = SCREEN_WIDTH;
      if (Asteroids::asteroids[i].x > SCREEN_WIDTH) Asteroids::asteroids[i].x = 0;
      if (Asteroids::asteroids[i].y < 0) Asteroids::asteroids[i].y = SCREEN_HEIGHT - 20;
      if (Asteroids::asteroids[i].y > SCREEN_HEIGHT - 20) Asteroids::asteroids[i].y = 0;
      
      // Check bullet collisions
      for (int j = 0; j < Asteroids::MAX_BULLETS; j++) {
        if (Asteroids::bullets[j].active) {
          int radius = 4 + Asteroids::asteroids[i].size * 3;
          float dx = Asteroids::bullets[j].x - Asteroids::asteroids[i].x;
          float dy = Asteroids::bullets[j].y - Asteroids::asteroids[i].y;
          if (dx*dx + dy*dy < radius*radius) {
            Asteroids::bullets[j].active = false;
            Asteroids::asteroids[i].active = false;
            Asteroids::score += (3 - Asteroids::asteroids[i].size) * 10;
            playScoreSound();
            
            // Spawn smaller asteroids
            if (Asteroids::asteroids[i].size < 2 && Asteroids::asteroidCount < Asteroids::MAX_ASTEROIDS - 1) {
              for (int k = 0; k < 2; k++) {
                Asteroids::asteroids[Asteroids::asteroidCount].x = Asteroids::asteroids[i].x;
                Asteroids::asteroids[Asteroids::asteroidCount].y = Asteroids::asteroids[i].y;
                Asteroids::asteroids[Asteroids::asteroidCount].velX = (random(-100, 100) / 100.0f);
                Asteroids::asteroids[Asteroids::asteroidCount].velY = (random(-100, 100) / 100.0f);
                Asteroids::asteroids[Asteroids::asteroidCount].size = Asteroids::asteroids[i].size + 1;
                Asteroids::asteroids[Asteroids::asteroidCount].active = true;
                Asteroids::asteroidCount++;
              }
            }
          }
        }
      }
    }
  }
  
  // Check if all asteroids destroyed
  bool allDestroyed = true;
  for (int i = 0; i < Asteroids::asteroidCount; i++) {
    if (Asteroids::asteroids[i].active) {
      allDestroyed = false;
      break;
    }
  }
  if (allDestroyed) {
    Asteroids::spawnAsteroids(3 + Asteroids::score / 100);
  }
  
  // Render
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - 18, ST77XX_WHITE);
  
  // Draw ship
  Asteroids::drawShip();
  
  // Draw bullets
  for (int i = 0; i < Asteroids::MAX_BULLETS; i++) {
    if (Asteroids::bullets[i].active) {
      tft.drawPixel(Asteroids::bullets[i].x, Asteroids::bullets[i].y, ST77XX_YELLOW);
      tft.drawPixel(Asteroids::bullets[i].x+1, Asteroids::bullets[i].y, ST77XX_YELLOW);
    }
  }
  
  // Draw asteroids
  for (int i = 0; i < Asteroids::asteroidCount; i++) {
    if (Asteroids::asteroids[i].active) {
      Asteroids::drawAsteroid(Asteroids::asteroids[i].x, Asteroids::asteroids[i].y, Asteroids::asteroids[i].size);
    }
  }
  
  // Display score, lives, and speed
  tft.fillRect(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, 18, ST77XX_BLACK);
  tft.drawLine(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, SCREEN_HEIGHT - 18, ST77XX_WHITE);
  
  tft.setCursor(5, SCREEN_HEIGHT - 15);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("Score:");
  tft.print(Asteroids::score);
  
  tft.setCursor(SCREEN_WIDTH - 45, SCREEN_HEIGHT - 15);
  tft.print("Lives:");
  tft.print(Asteroids::lives);
  
  return true;
}
