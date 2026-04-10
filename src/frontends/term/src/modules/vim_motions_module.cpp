
#include "logram_terminal_module.hpp"
#include "terminal_modules.hpp"
#include <cctype>
#include <cstdlib>

#define ACTION_GO_TO_FILE_BEGINNING 200
#define ACTION_GO_TO_FILE_END 201

#define is_digit(c) (c >= '0' && c <= '9')

void VimMotionsModule::registerUserInputMapping(LogramTerminal& lpt){
  lpt.registerUserInputMapping("gg", ACTION_GO_TO_FILE_BEGINNING);
  lpt.registerUserInputMapping("G", ACTION_GO_TO_FILE_END);
  lpt.registerUserInputMapping("h", ACTION_MOVE_LEFT);
  lpt.registerUserInputMapping("j", ACTION_MOVE_DOWN);
  lpt.registerUserInputMapping("k", ACTION_MOVE_UP);
  lpt.registerUserInputMapping("l", ACTION_MOVE_RIGHT);
}
void VimMotionsModule::registerUserActionCallback(LogramTerminal& lpt) {
  lpt.registerActionCallback([](user_action_t act, term_state_t& state, CachedFilteredFileNavigator* cfn)-> int{
    if(act == ACTION_GO_TO_FILE_BEGINNING){
      state.cy = 0;
      state.line_offset = 0;
      cfn->jumpToLocalLine(0);
    }
    if(act == ACTION_GO_TO_FILE_END){
      cfn->jumpToLocalLine(LINE_T_MAX);
      // We ideally want to put the cursor as low as possible (cy = num_rows - status_lines)
      line_t line_num = cfn->block.first_line_local_id + cfn->block.lines.size()-1;
      if(line_num > (size_t) state.nrows - state.num_status_line - 1) state.cy = state.nrows - state.num_status_line -1;
      else state.cy = line_num;
      state.line_offset = line_num - state.cy;
    }
    return 0;
  });
}
void VimMotionsModule::registerCommandCallback(LogramTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int{
    if(cmd.size() == 1 || !is_digit(cmd[1])) {
      return 0;
    }

    size_t line_num = atoll(cmd.data() + 1);
    cfn->jumpToLocalLine(line_num);

    if(cfn->block.contains_last_line && line_num >= cfn->block.first_line_local_id + cfn->block.size()){
      line_num = cfn->block.first_line_local_id + cfn->block.size()-1;
    }
    
    // Must cy + line_offset = line_num
    // Ideally don't change cy

    if(line_num >= (size_t) state.cy){
      state.line_offset = line_num - state.cy;
    } else {
      state.line_offset = 0;
      state.cy = line_num;
    }
    
    return 1;
  });
}
