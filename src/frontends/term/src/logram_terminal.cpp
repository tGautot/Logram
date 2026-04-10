#include "logram_terminal.hpp"
#include "ConfigHandler.hpp"
#include "filter_parsing.hpp"
#include "line_format.hpp"
#include "processed_line.hpp"

#include <exception>
#include <filesystem>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>

extern "C"{
#include "logging.h"
}
#include "logging.hpp"
#include "terminal_state.hpp"

#define CTRL_CHR(c) (c & 0x1f)

#define ESC_CHR '\x1b'
#define ESC_CMD "\x1b["

#define CLEAR_SCREEN() write(STDOUT_FILENO, ESC_CMD "2J", 4);
#define CURSOR_TL() write(STDOUT_FILENO, ESC_CMD "H", 3);

#define ANSI_RESET "\033[0m"

std::string ansi(const std::string& color, bool bold) {
  static const std::map<std::string, int> color_codes = {
    // To reset only either foreground or background color, instead of both
    {"fg_reset",         39}, {"bg_reset",          49},
    // Actual Colors
    {"fg_black",         30}, {"fg_red",            31},
    {"fg_green",         32}, {"fg_yellow",         33},
    {"fg_blue",          34}, {"fg_purple",         35},
    {"fg_cyan",          36}, {"fg_white",          37},
    {"fg_default",       39},
    {"fg_bright_black",  90}, {"fg_bright_red",     91},
    {"fg_bright_green",  92}, {"fg_bright_yellow",  93},
    {"fg_bright_blue",   94}, {"fg_bright_purple",  95},
    {"fg_bright_cyan",   96}, {"fg_bright_white",   97},
    {"bg_black",         40}, {"bg_red",            41},
    {"bg_green",         42}, {"bg_yellow",         43},
    {"bg_blue",          44}, {"bg_purple",         45},
    {"bg_cyan",          46}, {"bg_white",          47},
    {"bg_default",       49},
    {"bg_bright_black", 100}, {"bg_bright_red",    101},
    {"bg_bright_green", 102}, {"bg_bright_yellow", 103},
    {"bg_bright_blue",  104}, {"bg_bright_purple", 105},
    {"bg_bright_cyan",  106}, {"bg_bright_white",  107},
  };
  auto it = color_codes.find(color);
  if (it == color_codes.end()) return "";
  std::string seq = "\033[";
  if (bold) seq += "1;";
  seq += std::to_string(it->second) + "m";
  return seq;
}



int getCursorPosition(uint16_t *rows, uint16_t *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, ESC_CMD "6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);
  
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%hd;%hd", rows, cols) != 2) return -1;
  return 0;
}

int getWindowSize(uint16_t *rows, uint16_t *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, ESC_CMD "999C" ESC_CMD "999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

static struct termios orig_term;
static void die(const char* s);

static void rollbackTerm(){
  if(write(STDOUT_FILENO, ESC_CMD "?1049l", 8) != 8) die("write rollback alternate buf");
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term) == -1) die("tcsetattr");
}

static void die(const char* s){
  CLEAR_SCREEN();
  CURSOR_TL();
  rollbackTerm();
  perror(s);
  exit(1);
}

static void setupTerm(term_state_t stt){
  if(tcgetattr(STDIN_FILENO, &orig_term) == -1) die("tcgetattr");
  atexit(rollbackTerm);

  struct termios upd_term = orig_term;

  // No echo: don't print what user is typing
  // No canonical mode: read char by char (don't wait for enter key)
  // No sig: don't allow ctrl+c/z to stop the program
  // No exten: disable ctrl+v escaping next input
  upd_term.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

  // No crnl: Don't map ctrl+m ('\r') to '\n'
  // No ixon: Hopefully you won't run this program on a printer
  upd_term.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);

  // Disable any kind of ouput processing
  upd_term.c_oflag &= ~(OPOST);

  upd_term.c_cflag |= (CS8);


  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &upd_term) == -1) die("tcsetattr");

  if(write(STDOUT_FILENO, ESC_CMD "?1049h", 8) != 8) die("write alternate buf");
}



LogramTerminal::LogramTerminal(const std::string& filename) : LogramTerminal(filename, nullptr){}

LogramTerminal::LogramTerminal(CachedFilteredFileNavigator* cfn_ptr)
  : cfn(cfn_ptr), m_profile(CFG_COMMON_PROFILE) {
  term_state.cx = 4;
  term_state.cy = 0;
  term_state.input_mode = InputMode::ACTION;
  term_state.raw_input = "";
}

LogramTerminal::LogramTerminal(const std::string& filename, std::unique_ptr<LineFormat> line_format){
  ConfigHandler cfg;
  m_profile = cfg.getProfileForFile(std::filesystem::canonical(filename).string());
  
  std::shared_ptr<LineFilter> filter = nullptr;
  std::string filter_str;
  if( (filter_str = cfg.get(m_profile, CFG_FILTER, "")) != ""){
    filter = parse_filter_decl(filter_str, line_format.get());
  }
  cfn = new CachedFilteredFileNavigator(filename, std::move(line_format), filter);
  setupTerm(term_state);
  atexit(rollbackTerm);
  term_state.cx = 4;
  term_state.cy = 0;
  term_state.input_mode = InputMode::ACTION;
  term_state.raw_input = "";
}

void LogramTerminal::registerUserInputMapping(std::string input_seq, user_action_t action_code){
  user_input_mappings.push_back({input_seq, action_code});
}

void LogramTerminal::registerActionCallback(ActionCallbackPtr action_cb){
  action_cbs.push_back(action_cb);
}

void LogramTerminal::registerCommandCallback(CommandCallbackPtr cmd_cb){
  command_cbs.push_back(cmd_cb);
}

bool LogramTerminal::isCursorOnLastLine(){
  return (term_state.cy + term_state.line_offset == cfn->known_last_line);
}

void LogramTerminal::updateDisplayState(){
  term_state.displayed_pls.clear();
  ConfigHandler cfg;
  std::string line_numbering = cfg.get(m_profile, CFG_LINE_NUM_MODE) ;
  std::vector<std::string> line_info_str; 
  for(int i = 0; i < term_state.nrows-term_state.num_status_line; i++){
    line_info_t lineinfo = cfn->getLine(i+term_state.line_offset);
    term_state.displayed_pls.push_back(lineinfo.line);
    if(lineinfo.line != nullptr){
      term_state.info_col_size = 2/*size of "~ "*/  + std::to_string(lineinfo.line->line_num).size();
      if(line_numbering == "local"){
        term_state.info_col_size = 2/*size of "~ "*/ + std::to_string(i+term_state.line_offset).size();
      } else if(line_numbering == "both"){
        term_state.info_col_size += 3/*size of " ()"*/ + std::to_string(i+term_state.line_offset).size();
      }
    }
  }
}

void LogramTerminal::drawRows(){
  LOG_FUNCENTRY(5, "LogramTerminal::drawRows");
  frame_str += ESC_CMD "J";

  // Render file lines
  std::string_view fetched_line;
  ConfigHandler cfg;
  std::string line_numbering = cfg.get(m_profile, CFG_LINE_NUM_MODE) ;
  frame_str += ansi("bg_" + cfg.get(m_profile, CFG_BG_COLOR), false);
  frame_str += ansi("fg_" + cfg.get(m_profile, CFG_TEXT_COLOR), false);
  for(int i = 0; i < term_state.nrows-term_state.num_status_line; i++){
    const ProcessedLine* pl = term_state.displayed_pls[i];
    if(pl != nullptr){
      fetched_line = pl->raw_line;
      LOG_FCT(9, "Adding to display line(%d): '%s'\n", pl->well_formated, std::string(fetched_line).data());

      std::string line_num_str = std::to_string(pl->line_num); // By default, assume global
      if(line_numbering == "local"){
        line_num_str = std::to_string(term_state.line_offset+i);
      } else if (line_numbering == "both"){
        line_num_str += " (" + std::to_string(term_state.line_offset+i) + ")";
      } 

      // Spaces before linenum, if needed
      if(!pl->well_formated){
        frame_str += ansi("bg_red");
      }

      if(line_num_str.size()+2 < term_state.info_col_size) {
        frame_str += std::string(term_state.info_col_size - line_num_str.size() - 2, ' ');
      }

      frame_str += ansi("fg_white") + line_num_str + "~ " + ansi("fg_" + cfg.get(m_profile, CFG_TEXT_COLOR));

      if(!pl->well_formated){
        frame_str += ansi("bg_" + cfg.get(m_profile, CFG_BG_COLOR));
      }

      if(fetched_line.size() <= term_state.vert_line_offset){
        frame_str += std::string(term_state.ncols - term_state.info_col_size, ' ') + "\r\n";
        continue;
      }

      std::string formatted_line = std::string(fetched_line);
      formatted_line.erase(std::remove(formatted_line.begin(), formatted_line.end(), '\r'), formatted_line.end());
      size_t match_pos = 0;
      size_t first_visible_char = term_state.vert_line_offset;
      size_t last_visible_char = first_visible_char + term_state.ncols - term_state.info_col_size;
      size_t added_ansi_chars = 0;
      // Setup ansi seq in formatted line to highlight the word
      if(term_state.highlight_word != "") while((match_pos = formatted_line.find(term_state.highlight_word, match_pos)) != std::string::npos){
        static std::string end_invert_tag = "\e[27m"; 
        static std::string start_invert_tag = "\e[7m"; 
        LOG_FCT(5, "Found one match in line %lu highlighting it...\n", pl->line_num);
        if(match_pos + term_state.highlight_word.size() < first_visible_char){
          LOG_FCT(5, "Match is before the screen, looking ofr next\n");
          match_pos += term_state.highlight_word.size();
          continue;
        }     
        if(match_pos > last_visible_char+added_ansi_chars){
          LOG_FCT(5, "Match is beyond the screen, stopping there...\n");
          break;
        }
        formatted_line.insert(formatted_line.begin() + 
                  std::min(match_pos + term_state.highlight_word.length(), last_visible_char+added_ansi_chars), 
                  end_invert_tag.begin(), end_invert_tag.end());
        formatted_line.insert(formatted_line.begin() + 
                  std::max(match_pos, first_visible_char) , start_invert_tag.begin(), start_invert_tag.end());
        
        added_ansi_chars += start_invert_tag.length() + end_invert_tag.length();
        match_pos += term_state.highlight_word.length() + start_invert_tag.length() + end_invert_tag.length();
      } 

      formatted_line = formatted_line.substr(term_state.vert_line_offset, term_state.ncols - term_state.info_col_size + added_ansi_chars);
      
      frame_str += formatted_line;
      if(fetched_line.size() + term_state.info_col_size - term_state.vert_line_offset < term_state.ncols){
        frame_str += std::string(
            std::min(static_cast<int>(term_state.ncols - term_state.info_col_size - fetched_line.size() + term_state.vert_line_offset),
                     static_cast<int>(term_state.ncols - term_state.info_col_size))
            , ' ');
      }
    } else {
      frame_str += std::string(term_state.ncols, ' ');
    }
    
    frame_str += "\r\n";
  }
  
  char buf[80];

  frame_str += ansi("bg_" + cfg.get(m_profile, CFG_SL_BG_COLOR), false);
  frame_str += ansi("fg_" + cfg.get(m_profile, CFG_SL_TXT_COLOR), false);
  if(term_state.input_mode == ACTION){
    if(term_state.latest_error.empty()){
    // Status Line
      snprintf(buf, 80, "Status: cy%d:cx%d:lo%lu:vlo%lu (%d:%d) frame: %lu", term_state.cy, term_state.cx, term_state.line_offset, term_state.vert_line_offset, term_state.nrows, term_state.ncols, term_state.frame_num);
      char buf2[81];
      snprintf(buf2, 80, "BLK flli=%lu,frm=%lu,to=%lu,cll=%s  ", cfn->block.first_line_local_id, cfn->block.lines.front().line_num, cfn->block.lines.back().line_num, cfn->block.contains_last_line ? "true" : "false");
      frame_str += buf;
      if(strlen(buf) + strlen(buf2) < term_state.ncols){
        frame_str += std::string(term_state.ncols-strlen(buf)-strlen(buf2), ' ');
      }
      frame_str += buf2;
    } else {
      frame_str += ansi("bg_red", false);
      frame_str += ansi("fg_white" , true);
      frame_str += term_state.latest_error.substr(0, term_state.ncols);
      if(term_state.latest_error.size() < term_state.ncols){
        frame_str += std::string(term_state.ncols - term_state.latest_error.size(), ' '); 
      }
    }
  } else {
    if(term_state.input_mode == RAW) {
      std::string command = term_state.raw_input.substr(0,term_state.ncols);
      frame_str += command + std::string(term_state.ncols - command.length(), ' ');
    } else {
      snprintf(buf, 80, "Unknown input mode %d", term_state.input_mode);
      frame_str += buf;
      if(strlen(buf) < term_state.ncols){
        frame_str += std::string(term_state.ncols-strlen(buf), ' ');
      }
    }
  } 
  frame_str += ANSI_RESET;
}

void LogramTerminal::computeFrameStr(){
  char buf[32];

  //frame_str += ESC_CMD "2J";
  getWindowSize(&term_state.nrows, &term_state.ncols);
  frame_str = "";
  frame_str += ESC_CMD "?25l"; // Disable cursor display
  frame_str += ESC_CMD "H"; // Set cursor at top left position
  updateDisplayState();
  drawRows();
  
  // Position cursor
  if(term_state.input_mode == RAW){
    snprintf(buf, 32, ESC_CMD "%d;%zuH", term_state.nrows, term_state.raw_input_cursor + 1);
  } else {
    snprintf(buf, 32, ESC_CMD "%d;%dH", term_state.cy+1, term_state.cx+1);
  }
  
  frame_str += buf;
  frame_str += ESC_CMD "?25h"; // enable cursor display
}


void LogramTerminal::loop(){
  while (1) {
    computeFrameStr();
    write(STDOUT_FILENO, frame_str.data(), frame_str.size());
    handleUserAction(getUserAction());
    term_state.frame_num++;
  }
}

inline char readByte(){
  char c;
  if(read(STDIN_FILENO, &c, 1) == -1) return 0; // TODO die("read");
  return c;
}

void LogramTerminal::insertAtRawCursor(const std::string& s){
  term_state.raw_input.insert(term_state.raw_input_cursor, s);
  term_state.raw_input_cursor += s.size();
}

void LogramTerminal::submitRawInput(){
  int cmd_used = 0;
  try{
    for(auto cmd_cb : command_cbs){
      cmd_used |= cmd_cb(term_state.raw_input, term_state, cfn);
    }
    if(cmd_used == 0){
      term_state.latest_error = "The command '" + term_state.raw_input + "' was not recognized";
    }
  } catch(std::exception& e){
    term_state.latest_error = e.what();
  }
  term_state.input_mode = ACTION;
  term_state.raw_input = "";
  term_state.raw_input_cursor = 0;
}

void LogramTerminal::backspaceRawInput(){
  if(term_state.raw_input_cursor > 1) {
    term_state.raw_input.erase(term_state.raw_input_cursor - 1, 1);
    term_state.raw_input_cursor--;
  }
}

void LogramTerminal::processRawCsiSequence(const std::string& params, char final_byte){
  if(final_byte == 'C' && params.empty()) { // right arrow
    if(term_state.raw_input_cursor < term_state.raw_input.size())
      term_state.raw_input_cursor++;
  } else if(final_byte == 'D' && params.empty()) { // left arrow
    if(term_state.raw_input_cursor > 1)
      term_state.raw_input_cursor--;
  } else if(final_byte == '~' && params == "3") { // delete key
    if(term_state.raw_input_cursor < term_state.raw_input.size())
      term_state.raw_input.erase(term_state.raw_input_cursor, 1);
  } else { // unknown CSI: display verbatim
    std::string r = "\\e[" + params;
    if((unsigned char)final_byte < 32) { r += '^'; r += (char)('@' + final_byte); }
    else r += final_byte;
    insertAtRawCursor(r);
  }
}

void LogramTerminal::processRawNonCsiEsc(char c2){
  insertAtRawCursor("\\e");
  if((unsigned char)c2 < 32) insertAtRawCursor({'^', (char)('@' + c2)});
  else insertAtRawCursor(std::string(1, c2));
}

user_action_t LogramTerminal::matchInputSequence(const std::string& seq, bool& partial_match){
  LOG_FUNCENTRY(3, "LogramTerminal::matchInputSequence");
  partial_match = false;
  LOG_FCT(3, "Trying to match input sequence %s against %d mappings\n", seq.data(), user_input_mappings.size());
  for(auto mapping : user_input_mappings){
    std::string match = mapping.first;
    if(match.size() < seq.size()) continue;
    if(match.rfind(seq, 0) == 0){
      if(match.size() == seq.size()){
        LOG_FCT(3, "Found match, action id is %d\n", mapping.second);
        return mapping.second;
      }
      partial_match = true;
    }
  }
  return ACTION_NONE;
}

user_action_t LogramTerminal::getUserAction(){
  LOG_FUNCENTRY(3, "LogramTerminal::getUserAction");
  std::string seq = "";
  bool need_next_byte = true;

  while(1){
    if(term_state.input_mode == RAW){
      char c = readByte();
      LOG(5, "In raw mode and read char %d\n", c);

      if(c == 13) { // <Enter>
        LOG_FCT(3, "Sending command '%s' to modules.\n", term_state.raw_input.data());
        submitRawInput();
        break;
      }
      if(c == 127) { // <Backspace>
        backspaceRawInput();
        break;
      }
      if(c == ESC_CHR) {
        char c2 = readByte();
        if(c2 == '[') {
          // Collect parameter bytes (0x30-0x3F), then read the final byte
          std::string params;
          char final_byte = readByte();
          while((unsigned char)final_byte >= 0x30 && (unsigned char)final_byte <= 0x3F) {
            params += final_byte;
            final_byte = readByte();
          }
          processRawCsiSequence(params, final_byte);
        } else {
          processRawNonCsiEsc(c2);
        }
        break;
      }
      if((unsigned char)c < 32) {
        insertAtRawCursor({'^', (char)('@' + c)});
      } else {
        insertAtRawCursor(std::string(1, c));
      }
      break;
    }

    if(need_next_byte) seq += readByte();
    if(seq == ":"){
      term_state.input_mode = RAW;
      term_state.raw_input = ":";
      term_state.raw_input_cursor = 1;
      break;
    }
    if(seq == "q") exit(0);
    bool partial_match = false;
    user_action_t matched = matchInputSequence(seq, partial_match);
    if(matched != ACTION_NONE){
      LOG_EXIT();
      return matched;
    }

    LOG_FCT(3, "found no match, patial match: %d\n", partial_match);

    need_next_byte = true;
    // We might find a match if we add characters
    if(partial_match) continue;
    // Single character sequence and no match, exit
    if(seq.size() == 1) break;

    // Multi char seq without match, we might have consumed chars
    // needed to match a specific seq, remove first char and retry
    need_next_byte = false;
    seq = seq.substr(1);
  }

  return ACTION_NONE;
}

void LogramTerminal::handleUserAction(user_action_t action){
  try {
    for(ActionCallbackPtr cb : action_cbs){
      cb(action, term_state, cfn);
    }
  } catch (std::exception& e){
    term_state.latest_error = e.what();
  }
  term_state.current_action_multiplier = 1;
}
