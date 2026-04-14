#ifndef PERF_ANALYZER_HPP
#define PERF_ANALYZER_HPP

#include <vector>
#include <map>
#include <string>
#include <cstdint>



class DataAnalyzer {
  private:
  // Global run storage
  static std::map<std::string,
                  std::vector<int64_t> > profile_samples;

  public:
  static void add_sample(const std::string& profile, int64_t sample);
  
  static void print_stats();
  
  
  
  
};

class SectionPerfAnalyzer{
private:
  int64_t start;
  const char* name;
public:

  SectionPerfAnalyzer(const char* section_name);
  ~SectionPerfAnalyzer();


};

#define SECTION_PERF(x) SectionPerfAnalyzer section_perf_analyzer_sentinel = SectionPerfAnalyzer(x)


#endif