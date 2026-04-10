
#include "filter_parsing.hpp"
#include "line_filter.hpp"
#include "line_format.hpp"
#include "terminal_modules.hpp"
#include "ConfigHandler.hpp"
#include "string_utils.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>

#include "logging.hpp"

void ConfigManagerModule::registerUserInputMapping(LogramTerminal&){}
void ConfigManagerModule::registerUserActionCallback(LogramTerminal&) {}
void ConfigManagerModule::registerCommandCallback(LogramTerminal& lpt) {
  lpt.registerCommandCallback([&lpt](std::string& cmd, term_state_t& state, CachedFilteredFileNavigator* cfn) -> int{
    if(cmd.find(":cfg ") != 0) return 0;

    std::string subcmd = cmd.substr(5); // skip ":cfg "
    trim(subcmd);

    if(subcmd.find("set ") == 0) {
      std::string kv_str = subcmd.substr(4); // skip "set "
      size_t eq = kv_str.find('=');
      if(eq == std::string::npos){
        trim(kv_str);
        if(kv_str == "filter"){
          std::shared_ptr<LineFilter> filter = cfn->getFilter();
          if(filter == nullptr) throw std::runtime_error("Cannot save filter, there is no active filter");
          ConfigHandler cfg;
          std::string profile = cfg.getProfileForFile(std::filesystem::canonical(cfn->filename).string());
          cfg.set(profile, CFG_FILTER, filter->to_string());
          cfg.save(profile);
          return 1;
        }
        throw std::invalid_argument("Unexpected input for ':cfg set' command");
      }
      

      std::string key = kv_str.substr(0, eq);
      std::string val = kv_str.substr(eq + 1);

      // Special case here, might want a better way to handle "big" config changes that pushing it from here
      // PS: we do it before setting config, that way if format is badly formated we don't save it
      line_t global_focus_line = cfn->localToGlobalLineId(state.line_offset + state.cy);
      bool need_cursor_move = false;
      if(key == CFG_LINE_FORMAT){
        std::unique_ptr<LineFormat> format = LineFormat::fromFormatString(val);
        cfn->setLineFormat(std::move(format), global_focus_line);
        need_cursor_move = true;
      }
      if(key == CFG_HIDE_BAD_FMT){
        bool hidden = val == "true";
        cfn->setBadFormatAccepted(!hidden, global_focus_line);
        need_cursor_move = true;
      }
      if(need_cursor_move){
        line_t new_local_id = cfn->globalToLocalLineId(global_focus_line);
        if(new_local_id <= (line_t) state.cy){
          state.cy = new_local_id;
          state.line_offset = 0;
        } else {
          state.line_offset = new_local_id - state.cy;
        }
      }

      ConfigHandler cfg;
      LOG(1, "stored in cfn %s, absolute is %s\n", cfn->filename.data(), std::filesystem::canonical(cfn->filename).string().data());
      std::string profile = cfg.getProfileForFile(std::filesystem::canonical(cfn->filename).string());
      cfg.set(profile, key, val);
      cfg.save(profile);

      return 1;
    }

    if(subcmd.find("saveas ") == 0){
      std::string profile_name = subcmd.substr(7);
      trim(profile_name);
      ConfigHandler cfg;
      std::string c9l_fn = std::filesystem::canonical(cfn->filename).string();
      if(!cfg.copyProfileToNew(cfg.getProfileForFile(c9l_fn), profile_name)){
        throw std::runtime_error("Could NOT create new profile. Does'" + profile_name + "' already exist?");
      }
      cfg.setProfileForFile(c9l_fn, profile_name);
      cfg.save(profile_name);
      lpt.m_profile = profile_name;
      return 1;
    }
    if(subcmd.find("load ") == 0){
      std::string profile_name = subcmd.substr(5);
      trim(profile_name);
      ConfigHandler cfg;
      cfg.setProfileForFile(std::filesystem::canonical(cfn->filename).string(), profile_name);
      
      state.line_offset = 0;
      state.cy = 0;
      
      std::string format_str = cfg.get(profile_name, CFG_LINE_FORMAT);
      std::unique_ptr<LineFormat> line_format = LineFormat::fromFormatString(format_str);
      std::string filter_str = cfg.get(profile_name, CFG_FILTER);
      std::shared_ptr<LineFilter> line_filter = parse_filter_decl(filter_str, line_format.get());
      cfn->setLineFormat(std::move(line_format), 0);
      cfn->setFilter(line_filter);
      
      cfg.save(profile_name);
      lpt.m_profile = profile_name;
      return 1;
    }
    throw std::invalid_argument("Could not understand config command (expected set, saveas or load)");
  });
}
