// Game interface used by all games
#ifndef GAME_H
#define GAME_H

#include <Bluepad32.h>

#define BUZZER_PIN 26

extern Adafruit_ST7789 tft;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

class Game
{
public:
  virtual void setControllers(ControllerPtr p1, ControllerPtr p2) = 0;
  virtual void init() = 0;
  virtual bool update() = 0; // return false to indicate the game ended
  virtual const char *getName() = 0;
  virtual ~Game() {}

  uint16_t getPaddleColor(int playerIndex)
  {
    if (playerIndex == 0)
      return ST77XX_GREEN;

    if (playerIndex == 1)
      return ST77XX_BLUE;

    return ST77XX_WHITE;
  }

  void drawGameBorder()
  {
    const int PLAYFIELD_HEIGHT = SCREEN_HEIGHT - 18;
    const int PLAYFIELD_BORDER_WIDTH = 5;

    for (int offset = 0; offset < PLAYFIELD_BORDER_WIDTH; offset++)
    {
      tft.drawRect(
          offset,
          offset,
          SCREEN_WIDTH - offset * 2,
          PLAYFIELD_HEIGHT - offset * 2,
          ST77XX_WHITE);
    }
  }

  // Simple sound helpers used by games
  void playPaddleHit()
  {
    tone(BUZZER_PIN, 800, 50);
  }
  void playWallHit()
  {
    tone(BUZZER_PIN, 500, 50);
  }
  void playScoreSound()
  {
    tone(BUZZER_PIN, 200, 250);
  }
};

#endif // GAME_H
