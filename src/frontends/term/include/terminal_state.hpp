#ifndef TERMINAL_STATE_HPP
#define TERMINAL_STATE_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "processed_line.hpp"

enum InputMode {
  ACTION, RAW
};

typedef struct term_state_t {
  int cy, cx; // Cursor position on the screen (0-indexed)
  uint16_t info_col_size = 0; // num of cols to display linenum + decorator
  uint16_t num_status_line = 1; // Number of non-text lines at the bottom
  uint16_t nrows, ncols; // Number of displayed rows/cols (size of display - offsets)
  uint64_t line_offset = 0, frame_num = 0;
  uint64_t hrztl_line_offset = 0; // n means first displayed char of each line is the n-th
  uint64_t current_action_multiplier = 1;
  InputMode input_mode;
  std::string raw_input;
  size_t raw_input_cursor = 0;
  std::vector<const ProcessedLine*> displayed_pls;
  std::string highlight_word;
  std::string latest_error;
} term_state_t;

  #endif