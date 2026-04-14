#ifndef PERF_ANALYZER_HPP
#define PERF_ANALYZER_HPP

#include <vector>
#include <map>
#include <string>
#include <cstdint>

typedef std::map<std::string, std::vector<int64_t> >::iterator _profile_entry;

class DataAnalyzer {
  private:
  // Global run storage
  static std::map<std::string,
                  std::vector<int64_t> > profile_samples;

  public:
  static void add_sample(const _profile_entry& entry, int64_t sample);
  
  static void print_stats();
  
  static _profile_entry get_profile_entry(const std::string& name);
  
};

class SectionPerfAnalyzer{
private:
  int64_t start;
  _profile_entry m_entry;
public:

  SectionPerfAnalyzer(const _profile_entry& entry);
  ~SectionPerfAnalyzer();


};

#define SECTION_PERF(x) \
  static _profile_entry section_perf_profile_entry = DataAnalyzer::get_profile_entry(x); \
  SectionPerfAnalyzer section_perf_analyzer_sentinel = SectionPerfAnalyzer(section_perf_profile_entry)


#endif