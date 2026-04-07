#ifndef LOG_PARSER_TERMINAL_HPP
#define LOG_PARSER_TERMINAL_HPP

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <string>
#include <functional>

#include "line_format.hpp"
#include "terminal_state.hpp"
#include "log_parser_interface.hpp"


typedef uint64_t user_action_t;
#define ACTION_NONE 0
using ActionCallbackPtr =  std::function<int(user_action_t, term_state_t&, LogParserInterface*)>;
using CommandCallbackPtr =  std::function<int(std::string&, term_state_t&, LogParserInterface*)>;

std::string ansi(const std::string& color, bool bold = false);

class LogParserTerminal {

public:
  term_state_t term_state;

  std::string frame_str;
  LogParserInterface* lpi;
  std::string m_profile;

  LogParserTerminal(const std::string& filename);
  LogParserTerminal(const std::string& filename, std::unique_ptr<LineFormat> line_format);
  LogParserTerminal(LogParserInterface* lpi_ptr); // For unit testing (no terminal setup)

  std::vector<std::pair<std::string, user_action_t>> user_input_mappings;
  std::vector<ActionCallbackPtr> action_cbs;
  std::vector<CommandCallbackPtr> command_cbs;


  void registerUserInputMapping(std::string input_seq, user_action_t action_code);
  void registerActionCallback(ActionCallbackPtr action_cb);
  void registerCommandCallback(CommandCallbackPtr command_cb);

  bool isCursorOnLastLine();

  void computeFrameStr(); 

  void insertAtRawCursor(const std::string& s);
  void submitRawInput();
  void backspaceRawInput();
  void processRawCsiSequence(const std::string& params, char final_byte);
  void processRawNonCsiEsc(char c2);
  user_action_t matchInputSequence(const std::string& seq, bool& partial_match);

  user_action_t getUserAction();
  void handleUserAction(user_action_t action);

  void updateDisplayState();
  void drawRows();
  void loop();


};

#endif
