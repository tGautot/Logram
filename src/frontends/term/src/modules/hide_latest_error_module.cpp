
#include "terminal_modules.hpp"

#define HIDE_ERROR_ACTION 300

void HideLatestErrorModule::registerUserInputMapping(LogramTerminal& lpt){
  lpt.registerUserInputMapping(std::string({(char)13}) /*Enter*/, HIDE_ERROR_ACTION);
}
void HideLatestErrorModule::registerUserActionCallback(LogramTerminal& lpt) {
  lpt.registerActionCallback([](user_action_t act, term_state_t& term_state, CachedFilteredFileNavigator* cfn)-> int{
    if(act == HIDE_ERROR_ACTION){
      term_state.latest_error = "";
      return 1;
    }
    return 0;
  });
}
void HideLatestErrorModule::registerCommandCallback(LogramTerminal& lpt) {}
