#include "parsing_basics.hpp"
#include "line_format.hpp"
#include "perf_analyzer.hpp"
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "logging.hpp"

#define IS_NUM 1
#define IS_ALPHA 2
#define IS_SPACE 4
#define IS_EOL 8


const char glt_c[256] = {
    /* 0x00-0x0F: NUL      SOH STX ETX EOT ENQ ACK BEL BS  TAB        LF       VT  FF  CR       SO  SI  */
                  IS_EOL,  0,  0,  0,  0,  0,  0,  0,  0,  IS_SPACE,  IS_EOL,  0,  0,  IS_EOL,  0,  0,
    /* 0x10-0x1F: DLE DC1 DC2 DC3 DC4 NAK SYN ETB CAN EM  SUB ESC FS  GS  RS  US  */
                  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    IS_SPACE, /* 0x20: SP  */
    0, /* 0x21: !   */
    0, /* 0x22: "   */
    0, /* 0x23: #   */
    0, /* 0x24: $   */
    0, /* 0x25: %   */
    0, /* 0x26: &   */
    0, /* 0x27: '   */
    0, /* 0x28: (   */
    0, /* 0x29: )   */
    0, /* 0x2A: *   */
    0, /* 0x2B: +   */
    0, /* 0x2C: ,   */
    0, /* 0x2D: -   */
    0, /* 0x2E: .   */
    0, /* 0x2F: /   */
    IS_NUM, /* 0x30: 0   */
    IS_NUM, /* 0x31: 1   */
    IS_NUM, /* 0x32: 2   */
    IS_NUM, /* 0x33: 3   */
    IS_NUM, /* 0x34: 4   */
    IS_NUM, /* 0x35: 5   */
    IS_NUM, /* 0x36: 6   */
    IS_NUM, /* 0x37: 7   */
    IS_NUM, /* 0x38: 8   */
    IS_NUM, /* 0x39: 9   */
    0, /* 0x3A: :   */
    0, /* 0x3B: ;   */
    0, /* 0x3C: <   */
    0, /* 0x3D: =   */
    0, /* 0x3E: >   */
    0, /* 0x3F: ?   */
    0, /* 0x40: @   */
    IS_ALPHA, /* 0x41: A   */
    IS_ALPHA, /* 0x42: B   */
    IS_ALPHA, /* 0x43: C   */
    IS_ALPHA, /* 0x44: D   */
    IS_ALPHA, /* 0x45: E   */
    IS_ALPHA, /* 0x46: F   */
    IS_ALPHA, /* 0x47: G   */
    IS_ALPHA, /* 0x48: H   */
    IS_ALPHA, /* 0x49: I   */
    IS_ALPHA, /* 0x4A: J   */
    IS_ALPHA, /* 0x4B: K   */
    IS_ALPHA, /* 0x4C: L   */
    IS_ALPHA, /* 0x4D: M   */
    IS_ALPHA, /* 0x4E: N   */
    IS_ALPHA, /* 0x4F: O   */
    IS_ALPHA, /* 0x50: P   */
    IS_ALPHA, /* 0x51: Q   */
    IS_ALPHA, /* 0x52: R   */
    IS_ALPHA, /* 0x53: S   */
    IS_ALPHA, /* 0x54: T   */
    IS_ALPHA, /* 0x55: U   */
    IS_ALPHA, /* 0x56: V   */
    IS_ALPHA, /* 0x57: W   */
    IS_ALPHA, /* 0x58: X   */
    IS_ALPHA, /* 0x59: Y   */
    IS_ALPHA, /* 0x5A: Z   */
    0, /* 0x5B: [   */
    0, /* 0x5C: \   */
    0, /* 0x5D: ]   */
    0, /* 0x5E: ^   */
    0, /* 0x5F: _   */
    0, /* 0x60: `   */
    IS_ALPHA, /* 0x61: a   */
    IS_ALPHA, /* 0x62: b   */
    IS_ALPHA, /* 0x63: c   */
    IS_ALPHA, /* 0x64: d   */
    IS_ALPHA, /* 0x65: e   */
    IS_ALPHA, /* 0x66: f   */
    IS_ALPHA, /* 0x67: g   */
    IS_ALPHA, /* 0x68: h   */
    IS_ALPHA, /* 0x69: i   */
    IS_ALPHA, /* 0x6A: j   */
    IS_ALPHA, /* 0x6B: k   */
    IS_ALPHA, /* 0x6C: l   */
    IS_ALPHA, /* 0x6D: m   */
    IS_ALPHA, /* 0x6E: n   */
    IS_ALPHA, /* 0x6F: o   */
    IS_ALPHA, /* 0x70: p   */
    IS_ALPHA, /* 0x71: q   */
    IS_ALPHA, /* 0x72: r   */
    IS_ALPHA, /* 0x73: s   */
    IS_ALPHA, /* 0x74: t   */
    IS_ALPHA, /* 0x75: u   */
    IS_ALPHA, /* 0x76: v   */
    IS_ALPHA, /* 0x77: w   */
    IS_ALPHA, /* 0x78: x   */
    IS_ALPHA, /* 0x79: y   */
    IS_ALPHA, /* 0x7A: z   */
    0, /* 0x7B: {   */
    0, /* 0x7C: |   */
    0, /* 0x7D: }   */
    0, /* 0x7E: ~   */
    0, /* 0x7F: DEL */
    /* 0x80-0x8F: (extended - non-standard) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x90-0x9F: */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xA0-0xAF: */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xB0-0xBF: */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xC0-0xCF: */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xD0-0xDF: */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xE0-0xEF: */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xF0-0xFF: */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


int parse_int(const char** s, int64_t* res){  
  *res = 0;
  bool has_digit = glt_c[(unsigned char)**s] & IS_NUM; 
  LOG(3, "Parsing int fuirst char is %c : %d\n", **s, has_digit);
  while(glt_c[(unsigned char)**s] & IS_NUM){
    *res = *res * 10 + (**s - '0');
    (*s)++;
  }  // Move pointer fwd until char is not digit
  return !has_digit; // return 0 means OK, so invert
}

int parse_dbl(const char** s, double* res){  
  *((double*)res) = atof(*s);

  if(*((double*)res) == 0.0 && **s != '0') return -1;
  int dot_ok = 1;
  while( (**s >= '0' && **s <= '9') || (**s == '.' && dot_ok--)) (*s)++; // Move pointer fwd until char is not digit
  return 0;
}

int parse_chr(const char** s, _ChrFieldOption* p, void* res){

  if(**s == p->target){
    //*(p->res) = p->to_parse;
    *((char*)res) = p->target;
    (*s)++;
  }
  else {
    *((char*)res) = 0;
    return -1;
  }
  while(**s == p->target && p->repeat){
    (*s)++;
  }
  return 0;
}

int parse_str(const char** s, _StrFieldOption* p, void* res){
  int nchar = 0;
  auto not_eol = [&s, &nchar]()->bool{
    return  !(glt_c[(unsigned char)(*s)[nchar]] & IS_EOL);
  };
  if(p->stop_type == StrFieldStopType::NCHAR){
    nchar = p->nchar;
  }
  else if(p->stop_type == StrFieldStopType::DELIM){    
    while((*s)[nchar] != p->delim && not_eol()){
      nchar++;
    }
  }
  else if (p->stop_type == StrFieldStopType::ANY_WS){
    while(!(glt_c[(unsigned char)(*s)[nchar]] & (IS_SPACE | IS_EOL))){
      nchar++;
    }
  }
  else {
    throw std::runtime_error("Unknown StrFieldStopType");
  }
  
  std::string_view* res_sv = (std::string_view*) res;
  *res_sv = std::string_view(*s, nchar);
  *s += nchar;
  return 0;
}

int parse_ws(const char** s){
  while(glt_c[(unsigned char)**s] & IS_SPACE){(*s)++;}
  return 0;
}

#include <ctime>
#include <cstddef>
#include "logging.hpp"

bool parse_date_delimiter_char(char c, datetime_t* _unused, char expected){
  return c == expected;
}

#define PARSE_DATETIME_PART_FUNC(FIELD) \
bool parse_date_##FIELD##_char(char c, datetime_t* v, char _unsued){ \
  if(glt_c[(unsigned char)c] & IS_NUM){ \
    v-> FIELD = v-> FIELD * 10 + (c - '0'); \
    return true; \
  } \
  return false; \
}

PARSE_DATETIME_PART_FUNC(yr)
PARSE_DATETIME_PART_FUNC(month)
PARSE_DATETIME_PART_FUNC(day)
PARSE_DATETIME_PART_FUNC(hr)
PARSE_DATETIME_PART_FUNC(min)
PARSE_DATETIME_PART_FUNC(sec)
PARSE_DATETIME_PART_FUNC(ms)

// Returns days since 1970-01-01 handling leap years.
// from: http://howardhinnant.github.io/date_algorithms.html#days_from_civil                                                                                                              
constexpr int64_t days_from_civil(int y, unsigned m, unsigned d) noexcept {                                                                                                                 
  y -= m <= 2;                                                                                                                                                                              
  const int era = (y >= 0 ? y : y - 399) / 400;                                                                                                                                             
  const unsigned yoe = static_cast<unsigned>(y - era * 400);                                                                                                                                
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;                                                                                                                      
  const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;                                                                                                                                   
  return era * 146097LL + static_cast<int64_t>(doe) - 719468;                                                                                                                               
}                                                                                                                                                                                           
                                                                                                                                                                                            
static inline int64_t utc_epoch_seconds(int y, unsigned mo, unsigned d,                                                                                                                     
                                        unsigned h, unsigned mi, unsigned s) {
  return days_from_civil(y, mo, d) * 86400LL                                                                                                                                                
        + int64_t(h) * 3600 + int64_t(mi) * 60 + int64_t(s);
}


int parse_date(const char** s, _DateFieldOption* p, int64_t* res){
  // TODO: parse `*s` according to `p->format` (e.g. "YYYY-MM-DD hh:mm:ss.fff"),
  // write the parsed date into `*res` (representation up to caller -- e.g. epoch ms),
  // advance `*s` past the consumed characters, and return 0 on success / -1 on failure.
  
  if(p->parsing_routine.size() == 0){
    // A datetime parsing routine is a sequence n steps from two different categories:
    //  1. Parsing a "value char"
    //  2. Parsing a "delimiter char"
    // Each character of the format string can be mapped to one of those steps
    //
    // In the format string might have YYYY, which in a line could be 2026
    // When parsing any of those char (e.g. the 0), we want the parsing function to known where to put/update the datetime value
    // So we can simply give it the address of the datetime function  
    
    LOG(3, "Rebuilding date format parsing routine\n");
    for(auto c : p->format){
      switch(c){
        case 'Y':
          p->parsing_routine.push_back({parse_date_yr_char, 0});
          break;
        case 'M':
          p->parsing_routine.push_back({parse_date_month_char, 0});
          break;
        case 'D':
          p->parsing_routine.push_back({parse_date_day_char, 0});
          break;
        case 'h':
          p->parsing_routine.push_back({parse_date_hr_char, 0});
          break;
        case 'm':
          p->parsing_routine.push_back({parse_date_min_char, 0});
          break;
        case 's':
          p->parsing_routine.push_back({parse_date_sec_char, 0});
          break;
        case 'f':
          p->parsing_routine.push_back({parse_date_ms_char, 0});
          break;
        default:
          p->parsing_routine.push_back({parse_date_delimiter_char, c});
          break;
      }
    }
  }
  
  datetime_t t{};
  {
    SECTION_PERF("PARSE_DATE");
    

    for(size_t i = 0; i < p->parsing_routine.size(); i++){
      auto step = p->parsing_routine[i];
      bool res = step.func(**s, &t, step.param_c);
      if(!res) return -1;
      (*s)++;
    }

  }

  {
    SECTION_PERF("COMPUTE DATE");
    int64_t c_timestamp = utc_epoch_seconds(t.yr, t.month, t.day, t.hr, t.min, t.sec);
    *res = ((int64_t)c_timestamp) * 1000 + t.ms;
    LOG(9, "Sec timestamp: %lld, Final timestamp:%lld\n", c_timestamp, c_timestamp * 1000 + t.ms);
  }
  return 0;
}
