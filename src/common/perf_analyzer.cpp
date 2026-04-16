#include "perf_analyzer.hpp"

#include <iostream>


std::map<std::string, std::vector<int64_t> > DataAnalyzer::profile_samples = {};

void DataAnalyzer::add_sample(const _profile_entry& entry, int64_t sample){
  // Clearly not thread safe, update when Logram goes multithreaded
  entry->second.push_back(sample);
}

void DataAnalyzer::print_stats(){
  std::cout << "Collected data for " << profile_samples.size() << " profiles";

  for(auto itt : profile_samples) {
    std::cout << "\n\n[" + itt.first + "]: " << itt.second.size() << " samples\n";
    if(itt.second.size() == 0) continue;
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


_profile_entry DataAnalyzer::get_profile_entry(const std::string& profile){
  // Since we only ever do  insert, no iterator is ever invalidated
  auto itt = profile_samples.find(profile);
  if(itt == profile_samples.end()){
    profile_samples[profile] = {};
    // Allocate a lot of memory on creation
    profile_samples[profile].reserve(100000);
    itt = profile_samples.find(profile);
  }
  return itt;
}


SectionPerfAnalyzer::SectionPerfAnalyzer(const _profile_entry& entry) : m_entry(entry){
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
  DataAnalyzer::add_sample(m_entry, end-start);
#endif
}