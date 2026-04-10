#ifndef CACHED_FILTERED_FILE_NAVIGATOR_HPP
#define CACHED_FILTERED_FILE_NAVIGATOR_HPP


#include "common.hpp"
#include "processed_line.hpp"
#include "line_filter.hpp"
#include "filtered_file_reader.hpp"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <iterator>
#include <memory>
#include "cyclic_deque.hpp"


struct LineBlock {
  cyclic_deque<ProcessedLine> lines;
  
  // Local line number, i.e. nth filtered line
  line_t first_line_local_id;
  bool contains_last_line = false;

  LineBlock(size_t max_size):lines(max_size){};
  LineBlock(LineBlock&& tomove): lines(std::move(tomove.lines)){
    contains_last_line = tomove.contains_last_line;
    first_line_local_id = tomove.first_line_local_id;
  }

  size_t size(){
    return lines.size();
  }
};

#define INFO_NONE 0
#define INFO_EOF 1
#define INFO_IS_FIRST_LINE 2
#define INFO_IS_MALFORMED 4
#define INFO_ERROR 128
typedef struct {
  const ProcessedLine* line;
  uint8_t flags;
} line_info_t;


class CachedFilteredFileNavigator {
private:
  line_t  known_first_line=0;
  FilteredFileReader* ffr;

  void print_lines_in_block();

  void reset_and_refill_block(line_t around_global_line);

  
  public:
  uint32_t block_size;
  LineBlock block;
  std::string filename;
  line_t known_last_line=LINE_T_MAX;
  std::vector<line_t> local_to_global_id;

  CachedFilteredFileNavigator(std::string fname, std::unique_ptr<LineFormat> fmt, std::shared_ptr<LineFilter> fltr, int bsize = 1000);
  ~CachedFilteredFileNavigator();

  void setLineFormat(std::unique_ptr<LineFormat> lf, line_t global_anchor_line);
  LineFormat* getLineFormat();
  void setFilter(std::shared_ptr<LineFilter> lf, line_t global_anchor_line = 1);
  std::shared_ptr<LineFilter> getFilter();
  void setBadFormatAccepted(bool accept, line_t global_anchor_line);

  line_t highestIndexedGlobalLine(){return ffr->m_filtered_file_data->line_passes.size();}
  bool isGlobalLineAccepted(line_t global_id){return ffr->m_filtered_file_data->line_passes[global_id];}
  line_t globalToLocalLineId(line_t global_id){
    auto& valid_lines_idx = ffr->m_filtered_file_data->valid_line_index;
    auto itt = std::find(valid_lines_idx.begin(), valid_lines_idx.end(), global_id);
    if(itt == valid_lines_idx.end()) return globalToPriorLocalLine(global_id);
    return std::distance(valid_lines_idx.begin(), itt);
  }
  line_t globalToPriorLocalLine(line_t global_id){
    auto& valid_lines_idx = ffr->m_filtered_file_data->valid_line_index;
    auto itt = std::lower_bound(valid_lines_idx.begin(), valid_lines_idx.end(), global_id);
    if(itt == valid_lines_idx.begin()) return 0; // no prior line, fall to first
    --itt;
    return std::distance(valid_lines_idx.begin(), itt);
  }
  line_t globalToLaterLocalLine(line_t global_id){
    return globalToPriorLocalLine(global_id)+1;
  }
  line_t localToGlobalLineId(line_t local_id){return ffr->m_filtered_file_data->valid_line_index[local_id];};

  //std::vector<std::string_view> getFromFirstLine(size_t count)
  

  /**
    * This is the main, simplest function to get a line in LOCAL ID
    * from the file. This function call expects some locality between
    * successive calls, don't use to to jump between very far away places
    * in the file, as this function will read everything that is in between
    * For this use jumpTo(Local/Global)Line instead
    */
  line_info_t getLine(line_t local_line_id);

  void jumpToLocalLine(line_t local_line_id);

  std::pair<line_t, size_t> findNextOccurence(std::string match, line_t from_local, bool forward = true);
  
};



#endif