#include "perf_analyzer.hpp"

#include <iostream>
#include <chrono>

#define GET_MS(x) \
    auto GET_MS_now = std::chrono::system_clock::now(); \
    auto GET_MS_duration = GET_MS_now.time_since_epoch(); \
    x = std::chrono::duration_cast<std::chrono::milliseconds>(GET_MS_duration).count();



std::map<std::string, std::vector<int64_t> > DataAnalyzer::profile_samples = {};
std::map<std::string, int64_t > DataAnalyzer::profile_counter = {};

void DataAnalyzer::add_sample(const _sample_entry& entry, int64_t sample){
  // Clearly not thread safe, update when Logram goes multithreaded
  entry->second.push_back(sample);
}

void DataAnalyzer::print_stats(){
  std::cout << "Collected data for " << profile_samples.size() << " profiles";


  std::cout << "Samples ========================================\n";
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

  std::cout << "\n\nCounters ========================================\n";
  for(auto itt : profile_counter) {
    std::cout << itt.first << " = " << itt.second << "\n";
  }

  std::cout << std::endl;

}


_sample_entry DataAnalyzer::get_sample_entry(const std::string& profile){
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

_counter_entry DataAnalyzer::get_counter_entry(const std::string& profile){
  // Since we only ever do  insert, no iterator is ever invalidated
  auto itt = profile_counter.find(profile);
  if(itt == profile_counter.end()){
    profile_counter[profile] = 0;
    // Allocate a lot of memory on creation
    itt = profile_counter.find(profile);
  }
  return itt;
}


SectionPerfAnalyzer::SectionPerfAnalyzer(const _sample_entry& entry, perf_type_e pt) : m_entry(entry), m_pt(pt){
#ifdef PERF_ANALYZE_ON
  if(pt == CYCLES){
  uint32_t lo, hi;
  // Watching cpp talks has its perks
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  start = ((uint64_t)hi << 32) | lo;
} else {
  GET_MS(start); 
}
#endif 
}

SectionPerfAnalyzer::~SectionPerfAnalyzer() {
#ifdef PERF_ANALYZE_ON
  int64_t end;
  if(m_pt == CYCLES){
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    end = ((uint64_t)hi << 32) | lo;
  } else {
    GET_MS(end);
  }
  DataAnalyzer::add_sample(m_entry, end-start);
#endif
}
