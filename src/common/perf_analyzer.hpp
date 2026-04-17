#ifndef PERF_ANALYZER_HPP
#define PERF_ANALYZER_HPP

#include <vector>
#include <map>
#include <string>
#include <cstdint>

typedef std::map<std::string, std::vector<int64_t> >::iterator _sample_entry;
typedef std::map<std::string, int64_t>::iterator _counter_entry;

class DataAnalyzer {
  private:
  // Global run storage
  static std::map<std::string,
                  std::vector<int64_t> > profile_samples;
  static std::map<std::string, int64_t> profile_counter;

  public:
  static void add_sample(const _sample_entry& entry, int64_t sample);
  
  static void print_stats();
  
  static _sample_entry get_sample_entry(const std::string& name);
  static _counter_entry get_counter_entry(const std::string& name);
  
};

enum perf_type_e {
  CYCLES, TIME
};

class SectionPerfAnalyzer{
private:
  int64_t start;
  _sample_entry m_entry;
  perf_type_e m_pt;
public:

  SectionPerfAnalyzer(const _sample_entry& entry, perf_type_e pt = CYCLES);
  ~SectionPerfAnalyzer();
};


#define SECTION_PERF(x) \
  static _sample_entry section_perf_profile_entry = DataAnalyzer::get_sample_entry(x); \
  SectionPerfAnalyzer section_perf_analyzer_sentinel = SectionPerfAnalyzer(section_perf_profile_entry)


#define SECTION_TIME(x) \
  static _sample_entry section_time_profile_entry = DataAnalyzer::get_sample_entry(x); \
  SectionPerfAnalyzer section_perf_analyzer_sentinel = SectionPerfAnalyzer(section_time_profile_entry, TIME)

#define SECTION_USAGE_COUNT(x) \
  static _counter_entry section_counter_profile_entry = DataAnalyzer::get_counter_entry(x); \
  section_counter_profile_entry->second++;


#endif