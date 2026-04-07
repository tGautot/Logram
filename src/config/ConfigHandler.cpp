#include "ConfigHandler.hpp"
#include "logging.hpp"

#include <fstream>
#include <sstream>
#include <vector>

// Static storage — shared across all ConfigHandler instances
std::map<std::string, std::map<std::string, std::string>> ConfigHandler::s_sections;
bool ConfigHandler::s_loaded = false;
bool ConfigHandler::s_dirty  = false;

static std::string cfgFilePath() {
  const char* home = getenv("HOME");
  return home ? std::string(home) + "/.logram" : ".logram";
}

static const std::string DEFAULT_CFG =
  "Here is a short explanation of the .logram config file\n"
  "This is kind of a comment and there won't be another\n"
  "\n"
  "[[" CFG_COMMON_PROFILE "]]\n"
  CFG_BG_COLOR      "=default\n"
  CFG_TEXT_COLOR    "=default\n"
  CFG_SL_BG_COLOR   "=default\n"
  CFG_SL_TXT_COLOR  "=default\n"
  CFG_LINE_FORMAT   "={STR:,0}\n"
  CFG_HIDE_BAD_FMT  "=false\n"
  CFG_LINE_NUM_MODE "=global\n";

static void parse_stream(std::istream& stream,
                         std::map<std::string, std::map<std::string, std::string>>& sections) {
  std::string line;
  std::string current_section;
  while (std::getline(stream, line)) {
    if (line.size() >= 4 && line[0] == '[' && line[1] == '[') {
      size_t close = line.find("]]", 2);
      if (close != std::string::npos) {
        current_section = line.substr(2, close - 2);
        sections.emplace(current_section, std::map<std::string, std::string>{});
        continue;
      }
    }
    if (current_section.empty()) continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    sections[current_section][line.substr(0, eq)] = line.substr(eq + 1);
  }
}

void ConfigHandler::loadFromDisk() {
  std::string path = cfgFilePath();
  std::ifstream in(path);
  if (!in.is_open()) {
    LOG(3, "Config file not found, creating it at %s\n", path.data());
    std::ofstream out(path);
    if (out.is_open()) {
      out << DEFAULT_CFG;
      out.close();
    }
    std::istringstream defaults(DEFAULT_CFG);
    parse_stream(defaults, s_sections);
  } else {
    parse_stream(in, s_sections);
  }
  s_loaded = true;
}

ConfigHandler::ConfigHandler() {
  if (!s_loaded)
    loadFromDisk();
}

std::string ConfigHandler::get(const std::string& profile, const std::string& key,
                               const std::string& default_val) const {
  if (!profile.empty()) {
    auto pit = s_sections.find(profile);
    if (pit != s_sections.end()) {
      auto kit = pit->second.find(key);
      if (kit != pit->second.end()) return kit->second;
    }
  }
  // Fall back to common
  if (profile != CFG_COMMON_PROFILE) {
    auto cit = s_sections.find(CFG_COMMON_PROFILE);
    if (cit != s_sections.end()) {
      auto kit = cit->second.find(key);
      if (kit != cit->second.end()) return kit->second;
    }
  }
  return default_val;
}

void ConfigHandler::set(const std::string& profile, const std::string& key, const std::string& val) {
  s_sections[profile][key] = val;
  s_dirty = true;
}

ConfigHandler::~ConfigHandler() {
  if (s_dirty)
    saveAll();
}

void ConfigHandler::saveAll() {
  std::string path = cfgFilePath();

  std::string preambule;
  {
    std::ifstream is(path);
    if (!is.is_open() || !is.good()) {
      LOG(3, "Couldn't open config file for reqd preambule in saveAll\n");
      return;
    }  
    std::string s;
    while(getline(is, s)){
      if(s.size() >= 4 && s[0] == '[' && s[1] == '[' && s.find("]]") != std::string::npos){
        break;
      }
      preambule += s + '\n';
    }
  }

  std::ofstream out(path);
  if (!out.is_open()) {
    LOG(3, "Couldn't open config file for writing in saveAll\n");
    return;
  }
  auto write_section = [&out](const std::string&  section, const std::map<std::string, std::string>& kv){
    out << "[[" << section << "]]\n";
    for (const auto& [k, v] : kv)
      out << k << "=" << v << "\n";
  };
  
  out << preambule;
  
  write_section(CFG_COMMON_PROFILE, s_sections[CFG_COMMON_PROFILE]);
  for (const auto& [section, kv] : s_sections) {
    out << "\n";
    if(section != CFG_COMMON_PROFILE && section != CFG_PROFILE_MAPPING) write_section(section, kv);
  }
  out << "\n";
  write_section(CFG_PROFILE_MAPPING, s_sections[CFG_PROFILE_MAPPING]);
  out.close();
  s_dirty = false;
}

void ConfigHandler::save(const std::string& profile) {
  LOG_FUNCENTRY(3, "ConfigHandler::save");
  std::string path = cfgFilePath();

  std::vector<std::string> lines;
  {
    std::ifstream in(path);
    if (in.is_open()) {
      std::string line;
      while (std::getline(in, line))
        lines.push_back(line);
    }
  }

  std::string banner = "[[" + profile + "]]";
  std::vector<std::string> new_section;
  new_section.push_back(banner);
  auto sit = s_sections.find(profile);
  if (sit != s_sections.end()) {
    for (const auto& [k, v] : sit->second)
      new_section.push_back(k + "=" + v);
  }

  int section_start = -1;
  int section_end   = (int)lines.size();
  for (int i = 0; i < (int)lines.size(); i++) {
    if (lines[i].find(banner) == 0) {
      section_start = i;
      for (int j = i + 1; j < (int)lines.size(); j++) {
        if (lines[j].size() >= 2 && lines[j][0] == '[' && lines[j][1] == '[') {
          section_end = j;
          break;
        }
      }
      break;
    }
  }

  if (section_start == -1) {
    if (!lines.empty() && !lines.back().empty())
      lines.push_back("");
    lines.insert(lines.end(), new_section.begin(), new_section.end());
  } else {
    lines.erase(lines.begin() + section_start, lines.begin() + section_end);
    lines.insert(lines.begin() + section_start, new_section.begin(), new_section.end());
  }

  std::ofstream out(path);
  if (!out.is_open()) {
    LOG(3, "Couldn't open config file for writing\n");
    LOG_EXIT();
    return;
  }
  for (const auto& l : lines)
    out << l << "\n";
  out.close();
  LOG_EXIT();
}


bool ConfigHandler::copyProfileToNew(const std::string& base_prof, const std::string& new_prof){
  if(s_sections.find(new_prof) != s_sections.end()) return false;
  if(s_sections.find(base_prof) == s_sections.end()) return false;
  s_sections[new_prof] = s_sections[base_prof];
  return true;
}

std::string ConfigHandler::getProfileForFile(const std::string& file_path) const {
  auto sit = s_sections.find(CFG_PROFILE_MAPPING);
  if (sit == s_sections.end()) return "";
  auto it = sit->second.find(file_path);
  if (it == sit->second.end()) return "";
  return it->second;
}

void ConfigHandler::setProfileForFile(const std::string& file_path, const std::string& profile_name) {
  s_sections[CFG_PROFILE_MAPPING][file_path] = profile_name;
  s_dirty = true;
  save(CFG_PROFILE_MAPPING);
}
