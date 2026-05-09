#include "parsing_data.hpp"

#include <cstdlib>

#include "logging.hpp"

ParsedLine::ParsedLine(LineFormat* fmt){
  //LOG(1, "nint=%d,ndbl=%d,nchr=%d,nstr=%d,ndate=%d\n", fmt->nint, fmt->ndbl, fmt->nchr, fmt->nstr, fmt->ndate);
  int_fields  = new int64_t[fmt->nint];
  dbl_fields  = new double[fmt->ndbl];
  chr_fields  = new char[fmt->nchr];
  str_fields  = new std::string_view[fmt->nstr];
  date_fields = new int64_t[fmt->ndate];
}

ParsedLine::ParsedLine(ParsedLine&& old) : int_fields(old.int_fields), dbl_fields(old.dbl_fields), chr_fields(old.chr_fields), str_fields(old.str_fields), date_fields(old.date_fields){

  old.int_fields  = nullptr;
  old.dbl_fields  = nullptr;
  old.chr_fields  = nullptr;
  old.str_fields  = nullptr;
  old.date_fields = nullptr;

}

ParsedLine::~ParsedLine(){

  delete[] int_fields;
  delete[] dbl_fields;
  delete[] chr_fields;
  delete[] str_fields;
  delete[] date_fields;
}

void ParsedLine::asStringToStream(std::ostream& os, LineFormat& format){
  int i;
  os << "ParsedLine: ints(";
  for(i = 0; i < format.nint-1; i++) os << *getIntField(i) << ", ";
  if(format.nint >= 1) os << *getIntField(format.nint-1);

  os << "); dbls(";

  for(i = 0; i < format.ndbl-1; i++) os << *getDblField(i) << ", ";
  if(format.ndbl >= 1) os << *getDblField(format.ndbl-1);

  os  << "); chrs(";

  for(i = 0; i < format.nchr-1; i++) os << *getChrField(i) << ", ";
  if(format.nchr >= 1) os << *getChrField(format.nchr-1);

  os  << "); strs(\"";

  for(i = 0; i < format.nstr-1; i++) os << *getStrField(i) << "\", \"";
  if(format.nstr >= 1) os << *getStrField(format.nstr-1);

  os << "\"); dates(";

  for(i = 0; i < format.ndate-1; i++) os << *getDateField(i) << ", ";
  if(format.ndate >= 1) os << *getDateField(format.ndate-1);

  os << ")";
}