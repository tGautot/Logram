
#include "filter_parsing.hpp"
#include "line_filter.hpp"
#include "terminal_modules.hpp"
#include <cctype>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

#include "logging.hpp"

void FilterManagementModule::registerUserInputMapping(LogramTerminal&){}
void FilterManagementModule::registerUserActionCallback(LogramTerminal&) {}
void FilterManagementModule::registerCommandCallback(LogramTerminal& lpt) {
  lpt.registerCommandCallback([](std::string& cmd, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int{
    
    auto update_term_state_with_filter = [&state, cfn](std::shared_ptr<LineFilter> filter){
      // When setting a new filter the whole global->local mapping changes
      // That means whichever line the cursor is on right now might not make any sense after applying the filter
      // (e.g. cursor is on last line without filter -> add new filter -> cursor will be on nothing)
      // So we must apply the filter, parse the file until the global line on which the filter lives
      // Then check which new local line that global line maps to and update the terminal state to now have the cursor there
      LOG(1, "Setting new filter to %p\n", filter.get());
      line_t global_focus_line = cfn->localToGlobalLineId(state.line_offset + state.cy);
      cfn->setFilter(filter, global_focus_line);
      line_t new_local_id = cfn->globalToLocalLineId(global_focus_line);
      if(new_local_id <= (line_t) state.cy){
        state.cy = new_local_id;
        state.line_offset = 0;
      } else {
        state.line_offset = new_local_id - state.cy;
      }
    };
    
    
    if(cmd.find(":fclear") == 0){
      update_term_state_with_filter(nullptr);
      return 1;
    }

    LineFormat* lf = cfn->getLineFormat() ;
    if(lf == nullptr){
      throw std::runtime_error("Cannot set filter without having specified a line format");
    }
    size_t space_pos = cmd.find(" ");
    std::string short_cmd = cmd.substr(0, space_pos);
    LOG(3, "short cmd is %s\n", short_cmd.data());
    if(short_cmd == ":fset" || short_cmd == ":f" || short_cmd == ":fadd" || short_cmd == ":fand" ||
        short_cmd == ":for" || short_cmd == ":fxor" || short_cmd == ":fnor" || short_cmd == ":fout"){
      
      std::string filter_str = "";

      size_t arg_start = space_pos; 
      while(arg_start < cmd.size() && std::isspace(cmd[arg_start])){arg_start++;};
      
      if(arg_start < cmd.size()){
        std::ifstream file(cmd.data() + arg_start);
        if(file.is_open() && file.good()){
          getline(file, filter_str);
        } else {
          filter_str = cmd.data() + arg_start;
        }
      } else {
        throw std::runtime_error("Expecting a second argument");
      }

      std::shared_ptr<LineFilter> filter = parse_filter_decl(filter_str, lf);

      if(cmd.find(":fset ") == 0){
        update_term_state_with_filter(filter);
        return 1;
      }
      
      
      
      std::shared_ptr<LineFilter> old_filter = cfn->getFilter();
      if(old_filter == nullptr) {
        LOG(1, "No filter yet, adding it\n");
        if(short_cmd == ":fout"){
          filter->invert();
        }
        update_term_state_with_filter(filter);
        
        return 1;
      }
      LOG(1, "Already filter active, combining them...\n");
      std::shared_ptr<LineFilter> new_filter;
      if(cmd.find(":f ") == 0 || cmd.find(":fadd ") == 0 || cmd.find(":fand ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, AND);
      }else if(cmd.find(":for ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, OR);
      } else if(cmd.find(":fxor ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, XOR);
      } else if(cmd.find(":fnor ") == 0){
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, NOR);
      } else if(cmd.find(":fout ") == 0){
        // Filter OUT, we don't want to see the lines passing that filter
        filter->invert();
        new_filter = std::make_shared<CombinedFilter>(old_filter, filter, AND);
      }
      update_term_state_with_filter(new_filter);
      return 1;
    }    
    return 0;
  });
}
