// Game interface used by all games
#ifndef GAME_H
#define GAME_H

#include <Bluepad32.h>

// Game-level DPAD masks (use Bluepad32 DPAD_* macros)
static const uint8_t GAME_DPAD_UP = DPAD_UP;
static const uint8_t GAME_DPAD_DOWN = DPAD_DOWN;

class Game
{
public:
  virtual void setControllers(ControllerPtr p1, ControllerPtr p2) = 0;
  virtual void init() = 0;
  virtual bool update() = 0; // return false to indicate the game ended
  virtual const char *getName() = 0;
  virtual ~Game() {}
};
#endif // GAME_H
#endif // GAME_H
