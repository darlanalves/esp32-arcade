// Snake game module (refactored to support two players)
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Bluepad32.h>
#include "game.h"

extern Adafruit_ST7789 tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

namespace Snake
{
  const int GRID_SIZE = 12;  // pixels per grid cell
  const int BORDER = 7;      // pixels border around the visible screen (rounded corners)
  const int HUD_HEIGHT = 18; // height reserved for HUD at bottom

  const int GAME_AREA_LEFT = BORDER;
  const int GAME_AREA_TOP = BORDER;
  const int GAME_AREA_WIDTH = SCREEN_WIDTH - BORDER * 2;
  const int GAME_AREA_HEIGHT = SCREEN_HEIGHT - HUD_HEIGHT - BORDER * 2;

  const int GRID_WIDTH = GAME_AREA_WIDTH / GRID_SIZE;
  const int GRID_HEIGHT = GAME_AREA_HEIGHT / GRID_SIZE; // leaving room for HUD

  const int MAX_LENGTH = 100;
}

class SnakeGame : public Game
{
public:
  struct Player
  {
    ControllerPtr ctrl = nullptr;
    uint16_t color = ST77XX_GREEN;
    int headX = 0, headY = 0;
    int dirX = 1, dirY = 0;
    int nextDirX = 1, nextDirY = 0;
    int bodyX[Snake::MAX_LENGTH];
    int bodyY[Snake::MAX_LENGTH];
    int length = 3;
    int lives = 3;
    bool alive = true;
    int score = 0;
  };

  Player p[2];
  int foodX = 0, foodY = 0;

  float speedMultiplier = 1.0f;
  uint32_t lastSpeedChangeTime = 0;
  uint32_t lastMoveTime = 0;
  int moveDelay = 100; // ms

  void drawCell(int cellX, int cellY, int inset, uint16_t color)
  {
    int px = Snake::GAME_AREA_LEFT + cellX * Snake::GRID_SIZE + inset;
    int py = Snake::GAME_AREA_TOP + cellY * Snake::GRID_SIZE + inset;
    tft.fillRect(px, py, Snake::GRID_SIZE - inset * 2, Snake::GRID_SIZE - inset * 2, color);
  }

  void drawHud()
  {
    int hudY = SCREEN_HEIGHT - Snake::HUD_HEIGHT - Snake::BORDER;
    tft.fillRect(0, hudY, SCREEN_WIDTH, Snake::HUD_HEIGHT, ST77XX_BLACK);
    tft.drawLine(0, hudY, SCREEN_WIDTH, hudY, ST77XX_WHITE);
    tft.setTextSize(1);

    // Left: P1
    tft.setCursor(4, hudY + 3);
    tft.setTextColor(p[0].color);
    tft.print("P1 ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print("L:");
    tft.print(p[0].lives);
    tft.print(" S:");
    tft.print(p[0].score);

    // Right: P2
    String buf;
    buf += "P2 ";
    buf += "L:";
    buf += String(p[1].lives);
    buf += " S:";
    buf += String(p[1].score);
    int txtW = buf.length() * 6; // approx width (small font)
    tft.setCursor(SCREEN_WIDTH - txtW - 4, hudY + 3);
    tft.setTextColor(p[1].color);
    tft.print(buf);
  }

  void spawnFood()
  {
    bool ok = false;
    int attempts = 0;
    while (!ok && attempts < 1000)
    {
      attempts++;
      foodX = random(Snake::GRID_WIDTH);
      foodY = random(Snake::GRID_HEIGHT);
      ok = true;
      // don't spawn on any player's body
      for (int pi = 0; pi < 2 && ok; ++pi)
      {
        for (int i = 0; i < p[pi].length; ++i)
        {
          if (p[pi].bodyX[i] == foodX && p[pi].bodyY[i] == foodY)
          {
            ok = false;
            break;
          }
        }
      }
    }
  }

  bool isOnAnyBody(int x, int y)
  {
    for (int pi = 0; pi < 2; ++pi)
    {
      for (int i = 0; i < p[pi].length; ++i)
      {
        if (p[pi].bodyX[i] == x && p[pi].bodyY[i] == y)
          return true;
      }
    }
    return false;
  }

  bool checkSelfCollision(int pi)
  {
    for (int i = 1; i < p[pi].length; ++i)
    {
      if (p[pi].bodyX[i] == p[pi].headX && p[pi].bodyY[i] == p[pi].headY)
        return true;
    }
    return false;
  }

  bool checkCollisionWithOther(int me)
  {
    int other = 1 - me;
    for (int i = 0; i < p[other].length; ++i)
    {
      if (p[other].bodyX[i] == p[me].headX && p[other].bodyY[i] == p[me].headY)
        return true;
    }
    return false;
  }

  void respawnPlayer(int pi)
  {
    // Reset basic state and place on opposite halves
    p[pi].alive = true;
    p[pi].length = 3;
    p[pi].dirX = (pi == 0) ? 1 : -1;
    p[pi].dirY = 0;
    if (pi == 0)
    {
      p[pi].headX = Snake::GRID_WIDTH / 4;
      p[pi].headY = Snake::GRID_HEIGHT / 2;
    }
    else
    {
      p[pi].headX = (Snake::GRID_WIDTH * 3) / 4;
      p[pi].headY = Snake::GRID_HEIGHT / 2;
    }
    for (int i = 0; i < p[pi].length; ++i)
    {
      p[pi].bodyX[i] = p[pi].headX - i * p[pi].dirX;
      p[pi].bodyY[i] = p[pi].headY - i * p[pi].dirY;
    }
    // If spawned on food, move food
    if (p[pi].headX == foodX && p[pi].headY == foodY)
      spawnFood();
  }

  void init()
  {
    // Controllers are provided via setControllers(); don't overwrite them here
    p[0].color = getPaddleColor(0);
    p[1].color = getPaddleColor(1);
    p[0].lives = 3;
    p[1].lives = 3;
    p[0].score = 0;
    p[1].score = 0;
    respawnPlayer(0);
    respawnPlayer(1);

    speedMultiplier = 1.0f;
    lastMoveTime = millis();

    spawnFood();

    tft.fillScreen(ST77XX_BLACK);
    drawGameBorder();
    // initial draw
    drawEverything();
    drawHud();
  }

  void drawEverything()
  {
    tft.fillScreen(ST77XX_BLACK);
    drawGameBorder();
    // food
    drawCell(foodX, foodY, 2, ST77XX_RED);
    // players
    for (int pi = 0; pi < 2; ++pi)
    {
      for (int i = 0; i < p[pi].length; ++i)
      {
        int inset = (i == 0) ? 1 : 1;
        uint16_t c = (i == 0) ? p[pi].color : ST77XX_GREEN;
        if (!p[pi].alive)
          c = tft.color565(128, 128, 128); // gray for dead
        drawCell(p[pi].bodyX[i], p[pi].bodyY[i], inset, c);
      }
    }
  }

  bool update()
  {
    // Check for return to menu via P1 home button (preserve previous behavior)
    if (p[0].ctrl && p[0].ctrl->isConnected() && p[0].ctrl->miscBack())
      return false;

    // Input handling for both players
    for (int pi = 0; pi < 2; ++pi)
    {
      if (!p[pi].ctrl)
        continue;
      ControllerPtr C = p[pi].ctrl;
      if (!C->isConnected())
        continue;

      int axisX = C->axisX();
      int axisY = C->axisY();
      uint8_t dpad = C->dpad();

      if ((dpad & DPAD_UP) || axisY < -200)
      {
        p[pi].nextDirX = 0;
        p[pi].nextDirY = -1;
      }
      if ((dpad & DPAD_DOWN) || axisY > 200)
      {
        p[pi].nextDirX = 0;
        p[pi].nextDirY = 1;
      }
      if (axisX < -200)
      {
        p[pi].nextDirX = -1;
        p[pi].nextDirY = 0;
      }
      if (axisX > 200)
      {
        p[pi].nextDirX = 1;
        p[pi].nextDirY = 0;
      }

      // Speed control only via P1 for now
      if (pi == 0)
      {
        uint32_t now = millis();
        if (now - lastSpeedChangeTime > 150)
        {
          uint16_t btns = C->buttons();
          if (btns & 0x04)
          {
            speedMultiplier = max(0.3f, speedMultiplier - 0.1f);
            lastSpeedChangeTime = now;
            drawHud();
          }
          if (btns & 0x08)
          {
            speedMultiplier = min(2.0f, speedMultiplier + 0.1f);
            lastSpeedChangeTime = now;
            drawHud();
          }
        }
      }
    }

    uint32_t now = millis();
    int currentDelay = (int)(moveDelay / speedMultiplier);

    if (now - lastMoveTime > currentDelay)
    {
      lastMoveTime = now;

      // Move each alive player
      for (int pi = 0; pi < 2; ++pi)
      {
        if (!p[pi].alive)
          continue;

        // Prevent 180-degree turns
        if (!(p[pi].dirX != 0 && p[pi].nextDirX != 0 && (p[pi].dirX + p[pi].nextDirX == 0)))
        {
          if (!(p[pi].dirY != 0 && p[pi].nextDirY != 0 && (p[pi].dirY + p[pi].nextDirY == 0)))
          {
            p[pi].dirX = p[pi].nextDirX;
            p[pi].dirY = p[pi].nextDirY;
          }
        }

        int tailX = p[pi].bodyX[p[pi].length - 1];
        int tailY = p[pi].bodyY[p[pi].length - 1];

        p[pi].headX += p[pi].dirX;
        p[pi].headY += p[pi].dirY;

        // Wall collision
        if (p[pi].headX < 0 || p[pi].headX >= Snake::GRID_WIDTH || p[pi].headY < 0 || p[pi].headY >= Snake::GRID_HEIGHT)
        {
          playWallHit();
          p[pi].lives--;
          p[pi].alive = false;
          if (p[pi].lives > 0)
            respawnPlayer(pi);
          continue;
        }

        // Self collision
        if (checkSelfCollision(pi))
        {
          playPaddleHit();
          p[pi].lives--;
          p[pi].alive = false;
          if (p[pi].lives > 0)
            respawnPlayer(pi);
          continue;
        }

        // Collision with other snake
        if (checkCollisionWithOther(pi))
        {
          playPaddleHit();
          p[pi].lives--;
          p[pi].alive = false;
          if (p[pi].lives > 0)
            respawnPlayer(pi);
          continue;
        }

        bool ateFood = (p[pi].headX == foodX && p[pi].headY == foodY);

        // Shift body
        for (int i = p[pi].length - 1; i > 0; --i)
        {
          p[pi].bodyX[i] = p[pi].bodyX[i - 1];
          p[pi].bodyY[i] = p[pi].bodyY[i - 1];
        }
        p[pi].bodyX[0] = p[pi].headX;
        p[pi].bodyY[0] = p[pi].headY;

        if (ateFood)
        {
          if (p[pi].length < Snake::MAX_LENGTH)
          {
            p[pi].bodyX[p[pi].length] = tailX;
            p[pi].bodyY[p[pi].length] = tailY;
            p[pi].length++;
          }
          p[pi].score += 10;
          playScoreSound();
          spawnFood();
        }
      }

      // After moving both players, redraw full state for simplicity
      drawEverything();
      drawHud();

      // Check win condition: last player standing (other player's lives==0)
      if (p[0].lives <= 0 && p[1].lives > 0)
        return false;
      if (p[1].lives <= 0 && p[0].lives > 0)
        return false;
      if (p[0].lives <= 0 && p[1].lives <= 0)
        return false; // both dead -> end
    }

    return true;
  }

  void setControllers(ControllerPtr p1, ControllerPtr p2)
  {
    p[0].ctrl = p1;
    p[1].ctrl = p2;
  }

  const char *getName()
  {
    return "Snake";
  }
};
