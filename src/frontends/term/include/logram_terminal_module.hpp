#ifndef LOGRAM_TERMINAL_MODULE_HPP
#define LOGRAM_TERMINAL_MODULE_HPP

#include "terminal_state.hpp"
#include "logram_terminal.hpp"
#include "cached_filtered_file_navigator.hpp"


#define ACTION_MOVE_UP    10
#define ACTION_MOVE_DOWN  11
#define ACTION_MOVE_LEFT  12
#define ACTION_MOVE_RIGHT 13

class LogramTerminalModule {
  virtual void registerUserInputMapping(LogramTerminal&) = 0;
  virtual void registerUserActionCallback(LogramTerminal&) = 0;
  virtual void registerCommandCallback(LogramTerminal&) = 0;

};

#endif
