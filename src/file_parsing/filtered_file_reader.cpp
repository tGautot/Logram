#include "filtered_file_reader.hpp"
#include "common.hpp"
#include "line_format.hpp"
#include "processed_line.hpp"


#include <cassert>
#include <cstdio>
#include <memory>
#include <stdexcept>

#ifdef WIN32
// TODO
#else
extern "C"{
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
}
#endif

#include "logging.hpp"

#include "perf_analyzer.hpp"

char readCharAtPos(unsigned long long cp){return *(char*)cp;}

static FileData g_opened_file = {0, {}, "", nullptr};

FilteredFileReader::FilteredFileReader(std::string& fname) 
 : m_file_data(g_opened_file){

  if(m_file_data.data == nullptr){
    m_file_data.fname = fname;
#ifdef WIN32
    // TODO
#else 
    int fd = open(fname.data(), O_RDONLY);
    if (fd < 0) throw std::system_error(errno, std::system_category());

    struct stat st;
    fstat(fd, &st); // This can take a long time, but I don't know about any alternatives
    m_file_data.size = st.st_size;

    if (m_file_data.size > 0) {
        void* ptr = mmap(nullptr, m_file_data.size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (ptr == MAP_FAILED) {
            close(fd);
            throw std::system_error(errno, std::system_category());
        }
        m_file_data.data = static_cast<const char*>(ptr);
        LOG(1, "mmaped filed at address %p\n", m_file_data.data);

        //madvise(const_cast<char*>(m_file_data.data), m_file_data.size, MADV_RANDOM);
    } else { 
      // Create file is empty, some code doesn't handle it, instead of reworking everything
      // just create a "fake file" with one empty line
      m_file_data.size = 2;
      char* fake_data = (char*)malloc(2);
      fake_data[0]= ' '; fake_data[1] = 0;
      m_file_data.data = (const char*)fake_data;
      
    }
    close(fd);
    m_file_data.line_index = {0};
#endif
  }

  m_filtered_file_data = std::make_shared<FilteredFileData>();
  m_config = std::make_shared<FilterFileReaderConfig>();
  m_config->parser = nullptr;
  m_config->filter = nullptr;
  m_config->accept_bad_format = true;
  
  m_cursor = 0;
  m_curr_line = 0;
}

FilteredFileReader::FilteredFileReader(std::string& fname, std::unique_ptr<LineFormat> line_format)
  : FilteredFileReader(fname){
  m_config->parser = Parser::fromLineFormat(std::move(line_format));
}

FilteredFileReader::FilteredFileReader(std::string& fname, std::unique_ptr<LineFormat> line_format, std::shared_ptr<LineFilter> filter)
  : FilteredFileReader(fname){
  m_config->parser = Parser::fromLineFormat(std::move(line_format));
  m_config->filter = filter;
}

FilteredFileReader::~FilteredFileReader(){
  munmap((void*)m_file_data.data, m_file_data.size);
  g_opened_file = {0, {}, "", nullptr};
}

void FilteredFileReader::reset(){
  m_curr_line = 0;
  m_cursor = 0;
  m_filtered_file_data->line_passes.clear();
  m_filtered_file_data->valid_line_index.clear();
}

void FilteredFileReader::setFormat(std::unique_ptr<LineFormat> line_format){
  reset();
  m_config->parser = Parser::fromLineFormat(std::move(line_format));
}

void FilteredFileReader::setFilter(std::shared_ptr<LineFilter> filter){
  reset();
  m_config->filter = filter;
}

void FilteredFileReader::acceptBadFormat(bool accept){
  reset();
  m_config->accept_bad_format = accept;
}

void FilteredFileReader::jumpToGlobalLine(line_t line_num){
  LOG(9, "Jumping to global line %lu\n", line_num);
  if(line_num >= m_file_data.line_index.size()){
    LOG(1, "Invalid index in FilteredFileReader::jumpToGlobalLine=%llu\n", line_num);
    throw std::runtime_error("Cannot use jumpToGlobalLine with an index that hasn't yet parsed");
  }
  m_curr_line = line_num;
  m_cursor = m_file_data.line_index[m_curr_line];
}

void FilteredFileReader::jumpToLocalLine(line_t line_num){
  LOG(9, "Jumping to local line %lu\n", line_num);
  if(line_num >= m_filtered_file_data->valid_line_index.size()){
    LOG(1, "Invalid index in FilteredFileReader::jumpToLocqlLine=%llu (max is %llu)\n", line_num, m_filtered_file_data->valid_line_index.size()-1);
    throw std::runtime_error("Cannot use jumpToLocqlLine with an index that hasn't yet parsed");
  }
  jumpToGlobalLine(m_filtered_file_data->valid_line_index[line_num]);
}

size_t FilteredFileReader::getNextRawLine(const char** s){
  LOG_FUNCENTRY(9, "FilteredFileReader::getNextRawLine");
  LOG_FCT(9, "m_curr_line=%lu,already indexed %lu lines\n", m_curr_line, m_file_data.line_index.size()); 
  SECTION_PERF("FilteredFileReader::getNextRawLine");
  size_t sz = 0;
  *s = m_file_data.data + m_cursor;
  if(m_curr_line+1 == m_file_data.line_index.size()){ // On last indexed line need to find next LF
    if(m_cursor >= m_file_data.size){ // Rerached eof
      *s = nullptr;
      LOG_EXIT();
      return 0;
    }
    LOG_FCT(9, "Exploring next line...\n");
    while(m_cursor + sz < m_file_data.size && m_file_data.data[m_cursor+sz] != '\n'){sz++;}
    m_cursor = std::min(m_cursor+sz+1, m_file_data.size); // +1 to go beyond \n
  } else if(m_curr_line+1 < m_file_data.line_index.size()){
    sz = m_file_data.line_index[m_curr_line+1] - m_file_data.line_index[m_curr_line];
    // Strip the trailing \n if present (last line of file may lack one)
    if(sz > 0 && m_file_data.data[m_file_data.line_index[m_curr_line] + sz - 1] == '\n') sz--;
    m_cursor = m_file_data.line_index[m_curr_line+1];
  } else {
    throw std::runtime_error("Missing lines in index (getNextRawLine)");
  }
  
  m_curr_line++;
  if(sz > 0 && (*s)[sz-1] == '\r'){
    // To handle files with CRLF
    sz--;
  }
  LOG_EXIT();
  return sz;
}

size_t FilteredFileReader::getPrevRawLine(const char** s){
  LOG_FUNCENTRY(9, "FilteredFileReader::getPrevRawLine");
  LOG_FCT(9, "m_curr_line=%lu,already indexed %lu lines\n", m_curr_line, m_file_data.line_index.size()); 
  SECTION_PERF("FilteredFileReader::getPrevRawLine");
  
  if(m_curr_line > m_file_data.line_index.size()){
    throw std::runtime_error("Missing lines in index (getPrevRawLine)");
  }
  if(m_curr_line == 0){
    *s = nullptr;
    return 0;
  }
  file_pos_t prev_line_begin = m_file_data.line_index[m_curr_line-1];
  size_t sz = m_cursor - prev_line_begin;
  // Strip the trailing \n if present (last line of file may lack one)
  if(sz > 0 && m_file_data.data[prev_line_begin + sz - 1] == '\n') sz--;
  *s = m_file_data.data + prev_line_begin;
  m_cursor = prev_line_begin;
  m_curr_line--;
  if(sz > 0 && (*s)[sz-1] == '\r'){
    // To handle files with CRLF
    sz--;
  }
  LOG_EXIT();
  return sz;
}

void FilteredFileReader::skipNextRawLines(line_t n){
  const char* s;
  while(n){
    getNextRawLine(&s);    
    n--;
  }
}

void FilteredFileReader::seekRawLine(line_t num){
  const char* s;
  jumpToGlobalLine(std::min(num, m_file_data.line_index.size()-1));
  while(num >= m_file_data.line_index.size()){
    getNextRawLine(&s);
  } 
  m_curr_line = num;
  m_cursor = m_file_data.line_index[num];
}

bool FilteredFileReader::getNextValidLine(ProcessedLine& pl){
  LOG_FUNCENTRY(9, "FilteredFileReader::getNextValidLine");
  size_t numof_tested_lines = m_filtered_file_data->line_passes.size();
  LOG_FCT(9, "Searching for next valid line from %lu (already indexed %llu , and tested %llu)\n",  m_curr_line, m_file_data.line_index.size(), numof_tested_lines);
  
  if(m_cursor >= m_file_data.size) {LOG_EXIT(); return false;}
  SECTION_PERF("FilteredFileReader::getNextValidLine");
  

  assert(m_curr_line <= numof_tested_lines);

  const char* s;
  while(m_curr_line < numof_tested_lines){
    LOG_FCT(9, "Curr line (%llu) already explored moving forward...\n", m_curr_line);
    if(m_filtered_file_data->line_passes[m_curr_line]){
      LOG_FCT(9, "line %d matches, stopping...\n", m_curr_line);
      size_t line_size = getNextRawLine(&s);
      pl.set_data(m_curr_line-1, s, line_size, m_config->parser.get(), m_cursor);
      LOG_EXIT();
      return true;
    }
    getNextRawLine(&s);
  }

  if(m_curr_line == numof_tested_lines){
    LOG_FCT(9, "Need to explore new lines\n");
    size_t line_size;
    do {
      line_size = getNextRawLine(&s);

      if(s == nullptr){
        LOG_FCT(1, "reached eof, stoping\n");
        return false;
      }
      LOG_FCT(9, "Not at line %llu (out of %llu indexed)\n", m_curr_line, m_file_data.line_index.size());
      if(m_file_data.line_index.size() == m_curr_line){ // Curr_line is at beginning of "next line"
        LOG_FCT(9, "New line indexable!!!\n");
        m_file_data.line_index.push_back(m_cursor);
      }
      else if(m_curr_line > m_file_data.line_index.size() )
        throw std::runtime_error("SKipped lines index");
      
      {
       // SECTION_PERF("PROCESSED_LINE_SETUP");
        pl.set_data(m_curr_line-1, s, line_size, m_config->parser.get(), m_cursor);
      }      
      LOG(5, "Checking if line \"%s\" is accepted..\n", SV_TO_STR(pl.raw_line).data());
      // Badly formatted: accept/reject based solely on accept_bad_format                                                                                                                         
      // Well formatted: apply filter normally
     
      bool line_passes;
      {
        SECTION_PERF("FILTER_EVALUATION");
        line_passes = (!pl.well_formated && m_config->accept_bad_format) || 
          (pl.well_formated && (m_config->filter == nullptr || m_config->filter->passes(&pl)));
      }

      m_filtered_file_data->line_passes.push_back(line_passes);
      if(m_filtered_file_data->line_passes.back())
        m_filtered_file_data->valid_line_index.push_back(m_curr_line-1);

      LOG_FCT(9, "Explored a new line, curr now at %llu, indexed %llu lines\n", m_curr_line, m_file_data.line_index.size());
    
    } while(m_filtered_file_data->line_passes.back() == false);
    LOG_EXIT();
    return m_filtered_file_data->line_passes.back();
  } else {
    throw std::runtime_error("Cannot call getNextValidLine when m_curr_line is in uncharted territory");
  }
}




bool FilteredFileReader::getPreviousValidLine(ProcessedLine& pl){
  SECTION_PERF("FilteredFileReader::getPreviousValidLine");
  while(m_curr_line > 0){
    m_curr_line--;
    m_cursor = m_file_data.line_index[m_curr_line];
    if(m_filtered_file_data->line_passes[m_curr_line]){
      const char* s;
      size_t line_size = getNextRawLine(&s);
      pl.set_data(m_curr_line-1, s, line_size, m_config->parser.get(), m_cursor);

      // Move cursor to beginning of line we just parsed, undoing getNextRawLine
      m_curr_line--;
      m_cursor = m_file_data.line_index[m_curr_line];
      return true;
    }
  }
  return false;  
}
