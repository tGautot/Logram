#ifndef PROCESSED_LINE_HPP
#define PROCESSED_LINE_HPP


#include "common.hpp"
#include "line_parser.hpp"
#include <memory>

typedef unsigned long long file_pos_t;

// Quick to copy
class ProcessedLine {
public:
  file_pos_t stt_pos;
  line_t line_num = 0;
  std::string_view raw_line = "";
  std::unique_ptr<ParsedLine> pl = nullptr;
  bool well_formated = false;

  ProcessedLine() = default;
  ProcessedLine(const ProcessedLine&) = delete;
  ProcessedLine(line_t line, const char* s, size_t n_char, Parser* p, file_pos_t strm_pos);
  ProcessedLine(ProcessedLine&&) = default;
  ~ProcessedLine() = default;

  void swap(ProcessedLine& other);

  void set_data(line_t line, const char* s, size_t n_char, Parser* p,file_pos_t strm_pos);

};


#endif