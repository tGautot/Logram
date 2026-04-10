#ifndef LP_TERMINAL_MODULE_HPP
#define LP_TERMINAL_MODULE_HPP

#include "terminal_state.hpp"
#include "log_parser_terminal.hpp"
#include "cached_filtered_file_navigator.hpp"


#define ACTION_MOVE_UP    10
#define ACTION_MOVE_DOWN  11
#define ACTION_MOVE_LEFT  12
#define ACTION_MOVE_RIGHT 13

class LogParserTerminalModule {
  virtual void registerUserInputMapping(LogParserTerminal&) = 0;
  virtual void registerUserActionCallback(LogParserTerminal&) = 0;
  virtual void registerCommandCallback(LogParserTerminal&) = 0;

};

#endif
