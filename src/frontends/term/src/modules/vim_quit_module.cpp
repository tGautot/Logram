#include "terminal_modules.hpp"

extern "C" {
  #include "logging.h"
}
#include "logging.hpp"

#include <string>

void VimQuitModule::registerUserInputMapping(LogramTerminal&){}
void VimQuitModule::registerUserActionCallback(LogramTerminal&) {}
void VimQuitModule::registerCommandCallback(LogramTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int{
    LOG_ENTRY("LAMBDA VimQuitModule");
    LOG_FCT(5, "Cmd is %s, match=%ld\n", cmd.data(), cmd.find(":q")); 
    if(cmd.find(":q") == 0) {
      exit(0);
    }
    return 0;
  });
}