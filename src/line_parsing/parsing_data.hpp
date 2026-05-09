#ifndef PARSING_DATA_HPP
#define PARSING_DATA_HPP

#include "line_format.hpp"

#include <stdint.h>
#include <string>
#include <iostream>



class ParsedLine {
  public:

  int64_t* int_fields;
  double* dbl_fields;
  char* chr_fields;
  std::string_view* str_fields;
  int64_t* date_fields;

  ParsedLine(LineFormat* fmt);
  ParsedLine(ParsedLine&& old);
  ~ParsedLine();

  int64_t* getIntField(int id) const { return int_fields + id; }
  double* getDblField(int id) const { return dbl_fields + id; }
  char* getChrField(int id) const { return chr_fields + id; }
  std::string_view* getStrField(int id) const { return str_fields + id; }
  int64_t* getDateField(int id) const { return date_fields + id; }

  void asStringToStream(std::ostream& os, LineFormat& fmt);
};


#endif
