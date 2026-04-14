#include "perf_analyzer.hpp"

#include <iostream>


std::map<std::string, std::vector<int64_t> > DataAnalyzer::profile_samples = {};

void DataAnalyzer::add_sample(const std::string& profile, int64_t sample){
  // Clearly not thread safe, update when Logram goes multithreaded
  auto itt = profile_samples.find(profile);
  if(itt == profile_samples.end()){
    profile_samples[profile] = {sample};
    // Allocate a lot of memory on creation, that way we lose time (hopefully) only once
    profile_samples[profile].reserve(100000);
  } else {
    itt->second.push_back(sample);
  }
}

void DataAnalyzer::print_stats(){
  std::cout << "Collected data for " << profile_samples.size() << " profiles";

  for(auto itt : profile_samples) {
    std::cout << "\n\n[" + itt.first + "]: " << itt.second.size() << " samples\n";
    int64_t min = INT64_MAX, max = INT64_MIN, avg = 0;
    for(auto sample : itt.second){
      min = std::min(min, sample);
      max = std::max(max, sample);
      // If overflow, not my problem
      avg += sample;
    }
    avg /= itt.second.size();
    std::cout << "MIN: " << min << "\tMAX:\t" << max << "\tAVG:" << avg << "\n"; 
  }

  std::cout << std::endl;

}


SectionPerfAnalyzer::SectionPerfAnalyzer(const char*  section_name) : name(section_name){
#ifdef PERF_ANALYZE_ON
  uint32_t lo, hi;
  // Watching cpp talks has its perks
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  start = ((uint64_t)hi << 32) | lo;
#endif 
}

SectionPerfAnalyzer::~SectionPerfAnalyzer() {
#ifdef PERF_ANALYZE_ON
  uint32_t lo, hi;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  int64_t end = ((uint64_t)hi << 32) | lo;
  DataAnalyzer::add_sample(name, end-start);
#endif
}