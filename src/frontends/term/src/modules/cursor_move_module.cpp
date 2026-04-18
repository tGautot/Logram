
#include "cached_filtered_file_navigator.hpp"
#include "terminal_modules.hpp"

bool isCursorOnLastLine(term_state_t &term_state, CachedFilteredFileNavigator *cfn){
  return (cfn->localToGlobalLineId(term_state.cy + term_state.line_offset) == cfn->known_last_line);
}

void CursorMoveModule::registerUserInputMapping(LogramTerminal& term){
  // Dont register any input, this module only handles move actions
};
void CursorMoveModule::registerUserActionCallback(LogramTerminal& term){
  term.registerActionCallback([](user_action_t act, term_state_t& term_state, CachedFilteredFileNavigator* cfn)-> int{
    bool last_line = isCursorOnLastLine(term_state, cfn);
    if(act == ACTION_MOVE_DOWN){
      if(term_state.cy < term_state.nrows-1 - term_state.num_status_line - 4*(1-last_line)){
        if(term_state.displayed_pls[term_state.cy+1] == nullptr) return 0;
        term_state.cy++;
      } else {
        if(!last_line) term_state.line_offset++;
      }
    }
    else if(act == ACTION_MOVE_UP){
      if(term_state.cy > 4 || (term_state.line_offset == 0 && term_state.cy > 0) ){
        term_state.cy--;
      } else {
        if(term_state.line_offset != 0) {
          term_state.line_offset--;
        }
      }
    }
    else if(act == ACTION_MOVE_LEFT){
      if(term_state.cx > term_state.info_col_size){
        term_state.cx--;
      } else {
        if(term_state.hrztl_line_offset>0) term_state.hrztl_line_offset--;
      }
    }
    else if(act == ACTION_MOVE_RIGHT){
      if(term_state.cx < term_state.ncols - 1){
        term_state.cx++;
      } else {
        term_state.hrztl_line_offset++;
      }
    }

    return 0;
  });
};
void CursorMoveModule::registerCommandCallback(LogramTerminal&){
  
}