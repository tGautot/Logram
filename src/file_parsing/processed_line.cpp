#include "processed_line.hpp"
#include <cstring>


ProcessedLine::ProcessedLine(line_t line, const char* s, size_t n_char, Parser* p, file_pos_t strm_pos) {
  set_data(line, s, n_char, p, strm_pos);
}

void  ProcessedLine::set_data(line_t line, const char* s, size_t n_char, Parser* p,file_pos_t strm_pos) {
  line_num = line;
  raw_line = std::string_view(s, n_char);
  if(p->format == nullptr){
    pl = nullptr;
    well_formated = false;
  } else {
    // Assume that, if parsed line is already allocated, it is for the current format and thus right size.
    if(pl == nullptr){
      pl = std::make_unique<ParsedLine>(p->format.get());
    }
    well_formated = p->parseLine(raw_line, pl.get());
  }
  stt_pos = strm_pos;
}