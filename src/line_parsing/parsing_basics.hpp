#ifndef PARSING_BASICS_HPP
#define PARSING_BASICS_HPP

#include <stdint.h>
#include "line_format.hpp"


typedef struct parse_instruction {
  void* format_args;
  FieldType ft;
} parse_instruction_t;


int parse_int(const char** s, int64_t* res);
int parse_dbl(const char** s, double* res);
int parse_chr(const char** s, _ChrFieldOption* fmtagrs, void* res);
int parse_str(const char** s, _StrFieldOption* fmtagrs, void* res);
int parse_ws (const char** s);
int parse_date(const char** s, _DateFieldOption* fmtargs, int64_t* res);


#define STR_PARSE_STOP_DELIM 0
#define STR_PARSE_STOP_NCHAR 1


/*
typedef struct parse_int_params {
  int64_t* res;
} parse_int_params_t;

typedef struct parse_dbl_params {
  double* res;
} parse_dbl_params_t;


typedef struct parse_str_params{
  int stop_type;

  // For STOP_DELIM
  char delim;

  // for STOP_NCHAR
  int nchar;
  bool skip_eol;

  // To populate result
  //char** res;
} parse_str_params_t;

typedef struct parse_chr_params{
  char to_parse;
  bool do_reapeat;

  //char* res;
} parse_chr_params_t;
*/

#endif

