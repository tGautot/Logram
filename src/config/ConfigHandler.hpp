#ifndef CONFIG_HANDLER_HPP
#define CONFIG_HANDLER_HPP

#include <map>
#include <string>

#define CFG_BG_COLOR        "bg_col"
#define CFG_TEXT_COLOR      "txt_col"
#define CFG_SL_BG_COLOR     "sl_bg_col"
#define CFG_SL_TXT_COLOR    "sl_txt_col"
#define CFG_LINE_FORMAT     "line_format"
#define CFG_HIDE_BAD_FMT    "hide_bad_fmt"   // "true" / "false"
#define CFG_LINE_NUM_MODE   "line_num_mode"  // "global" / "local" / "both"
#define CFG_FILTER          "filter"

#define CFG_COMMON_PROFILE  "common"
#define CFG_PROFILE_MAPPING "profile_mapping"

class ConfigHandler {
public:
  // Lightweight: ensures ~/.logram has been loaded into static storage (no-op after first call)
  ConfigHandler();
  // Saves all sections to disk if any value was set since last saveAll
  ~ConfigHandler();

  // Per-profile kv access: looks in profile first, falls back to common, then default_val
  std::string get(const std::string& profile, const std::string& key,
                  const std::string& default_val = "") const;
  void set(const std::string& profile, const std::string& key, const std::string& val);

  // Rewrites only the given profile's section in ~/.logram
  void save(const std::string& profile);

  bool copyProfileToNew(const std::string& base_prof, const std::string& new_prof);
  // profile_mapping section helpers (<file_path>=<profile_name>)
  std::string getProfileForFile(const std::string& file_path) const;
  void setProfileForFile(const std::string& file_path, const std::string& profile_name);

private:
  static std::map<std::string, std::map<std::string, std::string>> s_sections;
  static bool s_loaded;
  static bool s_dirty;
  static void saveAll();
  static void loadFromDisk();
};

#endif
