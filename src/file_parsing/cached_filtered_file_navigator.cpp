#include "cached_filtered_file_navigator.hpp"
#include "common.hpp"
#include "line_filter.hpp"
#include "line_format.hpp"
#include "logging.hpp"
#include "processed_line.hpp"

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <string_view>

#define STRING_VIEW_PRINT(sv) static_cast<int>(sv.length()), sv.data()

CachedFilteredFileNavigator::CachedFilteredFileNavigator(std::string fname, std::unique_ptr<LineFormat> fmt, std::shared_ptr<LineFilter> fltr,  int bsize)
  : block_size(bsize), block(bsize), filename(fname){
  LOG_FUNCENTRY(5, "CachedFilteredFileNavigator::CachedFilteredFileNavigator");
  ffr = new FilteredFileReader(fname, std::move(fmt), fltr);
  known_last_line = LINE_T_MAX;
  known_first_line = 0;

  ProcessedLine pl;
  uint32_t line_local_id = 0;
  bool found_line = false;
  while( line_local_id < block_size && 
      (found_line = ffr->getNextValidLine(pl)) == true){
    block.lines.push_back(std::move(pl));
    LOG_FCT(5, "Read %d bytes, line is %.*s\n", pl.raw_line.length(), STRING_VIEW_PRINT(block.lines.back().raw_line));
    local_to_global_id.push_back(pl.line_num);
    line_local_id++;
  }
  known_first_line = block.lines[0].line_num;
  block.first_line_local_id = 0;
  // Special case where first block is first and also last
  if(!found_line) {
    block.contains_last_line = true;
    known_last_line = block.lines.back().line_num;
  }
  
}

CachedFilteredFileNavigator::~CachedFilteredFileNavigator(){
  delete ffr;
}

void CachedFilteredFileNavigator::reset_and_refill_block(line_t around_global_line){
  LOG_FUNCENTRY(5, "CachedFilteredFileNavigator::reset_and_refill_block");
  
  local_to_global_id.clear();
  block.lines.clear();
  known_last_line = LINE_T_MAX;
  known_first_line = 0;

  bool found_anchor =  false;
  ProcessedLine pl;
  uint32_t nread, line_local_id = 0, lines_left = block_size/2;
  ffr->jumpToGlobalLine(0);
  block.first_line_local_id = 0;
  while( (block.lines.size() < block_size || lines_left > 0)  && 
      (nread = ffr->getNextValidLine(pl)) != 0){
    if(block.lines.full()){
      block.first_line_local_id++;
    }
    block.lines.push_back(std::move(pl));
    LOG_FCT(9, "Read %d bytes, line is %.*s\n", nread, STRING_VIEW_PRINT(block.lines.back().raw_line));
    if(line_local_id == 0) known_first_line = block.lines[0].line_num;
    local_to_global_id.push_back(pl.line_num);
    line_local_id++;
    found_anchor = pl.line_num > around_global_line;
    if(found_anchor){ lines_left--; }
  }

  if(nread == 0){
    block.contains_last_line = true;
    known_last_line = block.lines.back().line_num;
  }

}

void CachedFilteredFileNavigator::setLineFormat(std::unique_ptr<LineFormat> lf, line_t global_anchor_line){
  ffr->setFormat(std::move(lf));
  reset_and_refill_block(global_anchor_line);
}

LineFormat* CachedFilteredFileNavigator::getLineFormat(){
  // We'll see about this raw pointer cast...
  return ffr->m_config->parser->format.get();
}


void CachedFilteredFileNavigator::setFilter(std::shared_ptr<LineFilter> lf, line_t global_anchor_line){
  ffr->setFilter(lf);
  reset_and_refill_block(global_anchor_line);
}

std::shared_ptr<LineFilter> CachedFilteredFileNavigator::getFilter(){
  return ffr->m_config->filter;
}

void CachedFilteredFileNavigator::setBadFormatAccepted(bool accept, line_t global_anchor_line){
  ffr->acceptBadFormat(accept);
  reset_and_refill_block(global_anchor_line);

}

void CachedFilteredFileNavigator::print_lines_in_block(){
  printf("current block: flli: %lu - llli: %lu\n", block.first_line_local_id, block.first_line_local_id + block_size);
  for(size_t i = 0; i < block_size; i++){
    printf("\t%s\n", block.lines[i].raw_line.data());
  }
}


line_info_t CachedFilteredFileNavigator::getLine(line_t local_line_id){
  LOG_FUNCENTRY(9, "CachedFilteredFileNavigator::getLine");
  LOG_FCT(9, "Looking for line %llu, block rqnge is [%llu, %llu[\n", local_line_id, block.first_line_local_id, block.first_line_local_id + block.size());
  if(local_line_id >= block.first_line_local_id && (local_line_id < block.first_line_local_id + block.size() || block.contains_last_line) ){
    if( (block.contains_last_line) && local_line_id >= block.first_line_local_id + block.size() ){
      return {nullptr, INFO_EOF};
    }
    LOG_FCT(9, "Is in block!\n");
    const ProcessedLine& pl = block.lines[local_line_id - block.first_line_local_id]; 
    line_info_t ret{&pl, 0};
    if(pl.line_num == known_first_line) ret.flags |= INFO_IS_FIRST_LINE;
    if(pl.line_num == known_last_line) ret.flags |= INFO_EOF;
    if(!pl.well_formated) ret.flags |= INFO_IS_MALFORMED;
    LOG_EXIT();
    return ret;
  }

  // The only way "lines" is not full, is if there are not enough lines in the file to fill it
  // Meaning that the whole file is already in the block, hence we should have found it
  // TODO replace assert with return empty "last line"
  assert(block.lines.full());

  if(local_line_id < block.first_line_local_id){
    LOG_FCT(9, "Is before block!\n");

    // Should not happen since local_line_id is line_t
    if(block.first_line_local_id == 0) {
      LOG_EXIT();
      return {nullptr, INFO_ERROR};
    }

    block.contains_last_line = false;

    // Requested line is before
    // Fill block by going upwards in the file
    // Since we are using a cyclic deque and it MUST be full, push back will put at front and inversly
    // So lets do a small trick where we put our data at the CURRENT back, and then push front... the back :)
    size_t nread;
    bool got_req_line = false;
    int block_offset = block_size/10;
    int i = block_offset;
    ffr->jumpToGlobalLine(block.lines.front().line_num);
    while( (nread = ffr->getPreviousValidLine(block.lines.back())) != 0){
      block.lines.push_front();
      block.first_line_local_id--;
      got_req_line = got_req_line || (block.first_line_local_id == local_line_id);
      if(got_req_line){
        if(i <= 0) break;
        i--;
      }
    }
    LOG_EXIT();
    return getLine(local_line_id);
  }

  // Current case is
  // local_line_id >= block.first_line_local_id + block.size()
  // Requested line is after what block holds
  // Move forward
  if(block.contains_last_line) {
    LOG_EXIT();
    return {nullptr, INFO_EOF};
  }
  LOG_FCT(9, "Is after block!\n");
  
  uint32_t nread;
  int block_offset = block_size/10;
  int i = block_offset;
  line_t block_last_line_local_id = block.first_line_local_id + block.lines.size() - 1;
  bool got_req_line =false;
  ffr->jumpToGlobalLine(block.lines.back().line_num+1);
  while( (nread = ffr->getNextValidLine(block.lines.front())) != 0){
    block.lines.push_back();
    block.first_line_local_id++;
    
    block_last_line_local_id++;
    if(block_last_line_local_id > local_to_global_id.size()){
      LOG_EXIT();
      throw std::runtime_error("Missed a local<->global mapping");
    }
    local_to_global_id.push_back(block.lines.back().line_num);

    got_req_line = got_req_line || (block.first_line_local_id + block_size - 1 == local_line_id);
    if(got_req_line){
      if(i <= 0) break;
      i--;
    }
  }
  if(nread == 0) {
    known_last_line = block.lines.back().line_num;
    block.contains_last_line = true;
  }
  LOG_EXIT();
  return getLine(local_line_id);
}


void CachedFilteredFileNavigator::jumpToLocalLine(line_t local_line_id){
  LOG_FUNCENTRY(5, "CachedFilteredFileNavigator::jumpToLocalLine");
  LOG_FCT(5,"Jumping to local line %ld\n", local_line_id);
  line_t block_last_line = block.first_line_local_id + block_size;
  if(local_line_id >= block.first_line_local_id && (local_line_id <= block_last_line || block.contains_last_line)){
    // Already in block
    LOG_EXIT();
    return;
  }

  line_t around_global_line; // Global id of the line around which the block might need to be formed
  line_t around_local_line; // local id of "around_line"

  if(local_line_id < block.first_line_local_id){
    LOG_FCT(5, "Local line is above what current block holds\n");
    block.contains_last_line = false;
    if(block.first_line_local_id - local_line_id < block_size){
      // Go there step by step, that way the block will be already correctly populated
      getLine(local_line_id);
      LOG_EXIT();
      return;
    }
    // Line is too far before in the file
    // But since it is _before_, we must have gone there already and thus know the lower bound
    around_global_line = local_to_global_id[local_line_id];
    around_local_line = local_line_id;
    
  } else { // local > first line of block
    if(local_line_id - block_last_line < block_size){
      getLine(local_line_id);
      LOG_EXIT();
      return;
    }
    LOG_FCT(5, "Local line is later what current block holds\n");

    // Line is later in the file
    // Maybe we already saw it
    
    // Never indexed local id, it means we never saw it, and we need to go there sequentially
    if(local_line_id >= local_to_global_id.size()){
      around_global_line = LINE_T_MAX;
      around_local_line = local_to_global_id.size()-1;
    } else {
      around_global_line = local_to_global_id[local_line_id];
      around_local_line = local_line_id;
    }

    if(around_global_line == LINE_T_MAX){  // Unkown best we can do is go as far as we explored, and walk from there    
      line_t lb;

      assert(local_to_global_id.size() > 0);

      size_t idx = around_local_line;
      lb = local_to_global_id[idx];
      line_t start_local_line = idx;
      
      block.lines.clear();
      
      bool found_line =  false;
      ProcessedLine pl;
      uint32_t nread, last_read_local_id = start_local_line, lines_left = block_size/2;
      ffr->jumpToGlobalLine(lb);
      block.first_line_local_id = start_local_line;
      LOG_FCT(5, "Requested line is in uncharted teritory, going on an adventure starting at (g:%lu,l:%lu)\n", lb, start_local_line);
      
      while( lines_left > 0  && 
          (nread = ffr->getNextValidLine( pl)) != 0){
        if(block.lines.full()){
          block.first_line_local_id++;
        }
        block.lines.push_back(std::move(pl));
        if(last_read_local_id == 0) known_first_line = block.lines[0].line_num;
        if(last_read_local_id >= local_to_global_id.size()){
          local_to_global_id.push_back(pl.line_num);
        }
        last_read_local_id++;
        found_line = pl.line_num > local_line_id;
        if(found_line){ lines_left--; }
      }

      if(nread == 0){
        known_last_line = block.lines.back().line_num;
        block.contains_last_line = true;
      }

      if(!block.lines.full()){ // Wasn't too far from lower bound, and found it without filling whole block
        ffr->jumpToGlobalLine(lb);
        lines_left = block_size-block.lines.size();
        last_read_local_id = start_local_line-1;
        while( lines_left > 0  && 
            (nread = ffr->getPreviousValidLine( pl)) != 0){
          block.lines.push_front(std::move(pl));
          last_read_local_id--;
          block.first_line_local_id--;
          lines_left--;
        }
      }

      LOG_EXIT();
      return;
    } 
  }

  LOG_FCT(5, "Jumping to a known place of the file, arougn global line %lu\n", around_global_line);
  size_t segment_lines_left = block_size/2, nread = 0;
  ProcessedLine pl;
  size_t line_storage_id = 0;
  bool no_more_above = false, no_more_below = false, finished = false;
  block.lines.clear();
  ffr->jumpToGlobalLine(around_global_line);
  block.first_line_local_id = around_local_line;
read_backwards:
  LOG_FCT(5, "Reading backwards (%ld lines max)\n", segment_lines_left);
  while( segment_lines_left > 0  && 
      (nread = ffr->getPreviousValidLine(pl)) != 0){
    block.lines.push_front(std::move(pl));
    line_storage_id++;
    block.first_line_local_id--;
    segment_lines_left--;
  }
  no_more_above = nread == 0;
  segment_lines_left = block_size - line_storage_id;
  finished = (line_storage_id == block_size) || no_more_below;
  if(finished) goto done;
  ffr->jumpToGlobalLine(around_global_line);
read_forward:
  LOG_FCT(5, "Reading forward (%ld lines max)\n", segment_lines_left);
  while( segment_lines_left > 0  && 
      (nread = ffr->getNextValidLine(pl)) != 0){
    block.lines.push_back(std::move(pl));
    line_storage_id++;
    segment_lines_left--;
  }
  no_more_below = nread == 0;
  segment_lines_left = block_size - line_storage_id;
  finished = (line_storage_id == block_size) || (no_more_above && no_more_below);
  if(finished) goto done;

  // Not finished, either
  // Could read half block above, but not below (try again, above)
  // OR
  // Could read half block below, but not above (try again, below)
  // OR
  // Could not read half block in either direction (doomed, should never happen as file fits into 1 block)

  if(no_more_below && !no_more_above) goto read_backwards;
  if(no_more_above && !no_more_below) goto read_forward;
  

done:
  LOG_EXIT();
  return;

}

std::pair<line_t, size_t> CachedFilteredFileNavigator::findNextOccurence(std::string match, line_t from_local, bool forward){
  LOG(3, "Starting at line num %lu\n", from_local);
  if(from_local >= ffr->m_filtered_file_data->valid_line_index.size()){
    return {LINE_T_MAX, SIZE_MAX};  
  }
  ffr->jumpToLocalLine(from_local);
  ProcessedLine pl;
  line_t offset = 0;
  while((forward) ? ffr->getNextValidLine(pl) != 0 :
                    ffr->getPreviousValidLine(pl) != 0){
    LOG(3, "Checking str\"%s\" for match agains \"%s\"\n", SV_TO_STR(pl.raw_line).data(), match.data());
    offset++;
    size_t sttpos = pl.raw_line.find(match);
    if(sttpos != std::string::npos){
      // Found match
      return {from_local + (forward ? offset-1 : -offset), sttpos};
    }
  }
  return {LINE_T_MAX, SIZE_MAX};
}