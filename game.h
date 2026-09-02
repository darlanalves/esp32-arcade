// Game interface used by all games
#ifndef GAME_H
#define GAME_H

#include <Bluepad32.h>

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
};
#endif // GAME_H
#endif // GAME_H
