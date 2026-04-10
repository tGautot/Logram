#include "common.hpp"
#include "cached_filtered_file_navigator.hpp"
#include "logram_terminal.hpp"
#include "processed_line.hpp"
#include "terminal_modules.hpp"
#include "terminal_state.hpp"
#include <algorithm>
#include <cstddef>
#include <unistd.h>
#include <utility>

#define ACTION_SEARCH_NEXT 100
#define ACTION_SEARCH_PREV 101
#define ACTION_START_SEARCH 110

extern "C" {
  #include "logging.h"
}
#include "logging.hpp"

static bool searching = false;
static std::string match_str = "";

static line_t search(term_state_t& state, CachedFilteredFileNavigator* cfn, bool forward){
  
  line_t cursor_start_line = state.line_offset + state.cy+(forward && searching);
  LOG(3, "Commanding next search to start at global line %lu\n", cursor_start_line);
  auto [ line_num, char_pos] = cfn->findNextOccurence(match_str, cursor_start_line, forward);
  LOG(3, "Found match at local line %llu\n", line_num);
  if(line_num == LINE_T_MAX){
    throw std::runtime_error("No more matches");
  } else {
    size_t vert_offset = std::max(state.nrows / 2, 1) - 1;
    if(forward){
      if((size_t) state.cy >= vert_offset){
        state.line_offset = line_num - state.cy;
      } else {
        if(line_num <= state.line_offset + vert_offset){
          state.cy = line_num - state.line_offset;
        } else {
          state.cy = vert_offset;
          state.line_offset = line_num - state.cy;
        }
      }
    } else {
      if((size_t) state.cy <= vert_offset){
        if((size_t) state.cy > line_num){
          state.line_offset = 0;
          state.cy = line_num;
        } else {
          state.line_offset = line_num - state.cy;
        }
      } else {
        if(line_num >= state.line_offset + vert_offset){
          state.cy = line_num - state.line_offset;
        } else {
          state.cy = vert_offset;
          state.line_offset = line_num - state.cy;
        }
      }
    }
    state.cx = char_pos + state.info_col_size;
  }
  return line_num;
}

void TextSearchModule::registerUserInputMapping(LogramTerminal& term) {
  term.registerUserInputMapping("n", ACTION_SEARCH_NEXT);
  term.registerUserInputMapping("N", ACTION_SEARCH_PREV);
  term.registerUserInputMapping("/", ACTION_START_SEARCH);
};
void TextSearchModule::registerUserActionCallback(LogramTerminal& term) {
  term.registerActionCallback([](user_action_t action, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int{
    if(!searching) return 0;
    if(action != ACTION_SEARCH_NEXT && action != ACTION_SEARCH_PREV) return 0;
    search( state, cfn, action == ACTION_SEARCH_NEXT);
    return 0;
  });
  term.registerActionCallback([](user_action_t action, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int{
    if(action == ACTION_START_SEARCH){
      // Setup search mode manually
      state.input_mode = RAW;
      state.raw_input = ":?";  
      state.raw_input_cursor = 2;
    }
    return 0;
  });
};
void TextSearchModule::registerCommandCallback(LogramTerminal& term){
  term.registerCommandCallback([](std::string& cmd, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int {
    LOG_ENTRY("LAMBDA search module command callback search");
    size_t substr_pos = cmd.find(":?");
    LOG_FCT(3, "Full command is %s, found search query at pos %lu\n", cmd.data(), substr_pos);
    if(substr_pos == 0){
      match_str = cmd.substr(2);
      state.highlight_word = match_str;
      line_t l = search(state, cfn, true);
      // Set searching=true after first search, to let the search know if it is a first call or not
      searching = true;
      LOG_FCT(3, "Command is indeed for search, looking for string %s", match_str.data());
      LOG_FCT(3, "Found it at %lu\n", l);
      return 1;
    } else {
      // TODO maybe searching = false;? See how it feels without any reset
    }
    return 0;
  });
}