#include "parsing_basics.hpp"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

int parse_int(const char** s, int64_t* res){  
  *((int64_t*)res) = atol(*s);
  
  if(*((int64_t*)res) == 0 && **s != '0') return -1;
  while(**s >= '0' && **s <= '9') (*s)++; // Move pointer fwd until char is not digit
  return 0;
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
    return  (*s)[nchar] != 0 && 
            (*s)[nchar] != '\n' && 
            !((*s)[nchar] == '\r' && (*s)[nchar+1] == '\n');};
  if(p->stop_type == StrFieldStopType::NCHAR){
    nchar = p->nchar;
  }
  else if(p->stop_type == StrFieldStopType::DELIM){    
    while((*s)[nchar] != p->delim && not_eol()){
      nchar++;
    }
  }
  else if (p->stop_type == StrFieldStopType::ANY_WS){
    while((*s)[nchar] != ' ' && (*s)[nchar] != '\t' && not_eol()){
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
  while(**s != 0 && **s != '\n' && (**s == ' ' || **s == '\t')){(*s)++;}
  return 0;
}
