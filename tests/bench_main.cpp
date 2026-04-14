
#include "filter_parsing.hpp"
#include "filtered_file_reader.hpp"
#include "line_filter.hpp"
#include "line_format.hpp"
#include "perf_analyzer.hpp"
#include "processed_line.hpp"

#include <memory>
#include <utility>


int main(int argc, char** argv){

  if(argc < 2){
    std::cout << "Please provide a file path as argument" << std::endl;
    exit(1);
  }

  std::string file_path = argv[1];
  std::string line_format_str = "{INT:Date} {INT:Time} {INT:Tid} {STR:Level} {STR:Source}: {STR:Mesg}";
  //std::string active_filter_str = "";

  std::unique_ptr<LineFormat> line_format;
  std::shared_ptr<LineFilter> line_filter;

  uint64_t lines_processed = 0;
  {
    FilteredFileReader* ffr;
    std::cout << "Running bench test on file located at " << file_path << std::endl;
    SECTION_PERF("GLOBAL_SECTION_PERF");
    {
      SECTION_PERF("GLOBAL_SETUP");
      line_format = LineFormat::fromFormatString(line_format_str);
      //line_filter = parse_filter_decl(active_filter_str, line_format.get());
      ffr = new FilteredFileReader(file_path, std::move(line_format), nullptr);
    }
    
    {
      SECTION_PERF("GLOBAL_RUNTIME");
      ProcessedLine pl; 
      while(ffr->getNextValidLine(pl) != 0){ 
        lines_processed++; } 
    }

  }
  std::cout << "Processed " << lines_processed << " lines considering validity, perff results:\n";
  DataAnalyzer::print_stats();
}