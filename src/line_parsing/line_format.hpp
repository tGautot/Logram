#ifndef LINE_FORMAT_HPP
#define LINE_FORMAT_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

enum FieldType {
  INT, DBL, CHR, STR, WS
};

class LineField{
public: 
  std::string name;
  FieldType ft;

  LineField(std::string field_name, FieldType field_type) : name(field_name), ft(field_type) {} 
  virtual ~LineField() {}
};



class LineIntField : public LineField {
public:
  LineIntField(std::string field_name) : LineField(field_name, FieldType::INT) {}
  ~LineIntField() {}
};

class LineDblField : public LineField {
public:
  LineDblField(std::string field_name) : LineField(field_name, FieldType::DBL) {}
  ~LineDblField() {}
};

class _ChrFieldOption {
public:
  char target;
  bool repeat;
  _ChrFieldOption(char to_parse, bool do_repeat) : target(to_parse), repeat(do_repeat){}
};

class LineChrField : public LineField {
public:
  _ChrFieldOption* opt;

  LineChrField(std::string field_name, char target, bool repeat) : LineField(field_name, FieldType::CHR) {
    opt = new _ChrFieldOption(target, repeat);
  }
  ~LineChrField() {
    delete opt;
  }
};

enum StrFieldStopType {
  NCHAR, DELIM, ANY_WS
};

class _StrFieldOption {
public:
  StrFieldStopType stop_type;
  char delim;
  int nchar;

  _StrFieldOption(StrFieldStopType st, char lim, int n) : stop_type(st), delim(lim), nchar(n){};
};

class LineStrField : public LineField {
public:
  _StrFieldOption* opt;

  LineStrField(std::string field_name, StrFieldStopType st, char lim, int n) : LineField(field_name, FieldType::STR) {
    opt = new _StrFieldOption(st, lim, n);
  }
  ~LineStrField() {
    delete opt;
  }
};

class WhitespaceField : public LineField {
public: 
  WhitespaceField() : LineField("", FieldType::WS) {}
};



class LineFormat {
  std::map<std::string, LineField*> name_to_field;
public:
  std::vector<LineField*> fields;
  int nint, ndbl, nchr, nstr, nws;

  LineFormat() : nint(0), ndbl(0), nchr(0), nstr(0), nws(0){}
  ~LineFormat(){
    for(auto field : fields){
      delete field;
    }
  }

  int getNIntFields() const { return nint; };
  int getNDoubleFields() const { return ndbl; };
  int getNCharFields() const { return nchr; };
  int getNStringFields() const { return nstr; };
  int getNWhiteSpaceFields() const { return nws; };

  LineField* getFieldFromName(std::string field_name) {
    if(name_to_field.count(field_name) > 0){
        return name_to_field[field_name];
      }
      return nullptr;
  }

  // Takes ownership of linefield
  void addField(LineField* lf) {
    fields.push_back(lf);
    if(lf->name != ""){
      if(name_to_field.count(lf->name) > 0){
        // TODO throw error, cant have same name twice
      }
      name_to_field[lf->name] = lf;
    }
    switch (lf->ft)
    {
    case FieldType::INT:
      nint++;
      break;
    case FieldType::DBL:
      ndbl++;
      break;
    case FieldType::CHR:
      nchr++;
      break;
    case FieldType::STR:
      nstr++;
      break;
    case FieldType::WS:
      nws++;
      break;
    default:
      // TODO throw error
      break;
    }
  }

  void toString(){
    for(LineField* lf : fields){
      std::string name = lf->name;
      LineChrField* lcf;
      LineStrField* lsf;
      switch (lf->ft)
      {
      case FieldType::INT:
        printf("{INT:%s}", name.data());
        break;
      case FieldType::DBL:
        printf("{DBL:%s}", name.data());
        break;
      case FieldType::CHR:
        lcf = dynamic_cast<LineChrField*>(lf);
        printf("{CHR:%s,%c,%d}", name.data(), lcf->opt->target, lcf->opt->repeat);
        break;
      case FieldType::STR:
        lsf = dynamic_cast<LineStrField*>(lf);
        printf("{STR:%s,%d,%c,%d}", name.data(), lsf->opt->stop_type, lsf->opt->delim, lsf->opt->nchar);
        break;
      case FieldType::WS:
        printf(" ");
        break;
      default:
        // TODO throw error
        break;
      }
    }
  }

  static std::unique_ptr<LineFormat> fromFormatString(const std::string& fmt_str){
    // Grammar:
    //   Outside {...}: each char becomes a literal LineChrField; ' ' becomes WhitespaceField.
    //   {TAG:name}            no-param form
    //   {TAG:name,params...}  parameterized form (params depend on the tag)
    // Tag is whatever appears between '{' and ':' -- length is not fixed.
    // Currently supported:
    //   {INT:name}
    //   {DBL:name}
    //   {CHR:name,c,r}    literal char `c`; r != '0' means it may repeat
    //   {STR:name}        stops at the char following '}', or any whitespace if that char is ' '
    //   {STR:name,N}      fixed-length string of N chars
    //
    // To add a new tag: declare its LineField subclass, then add a branch in the
    // dispatch below (and in the param section if it takes parameters).

    auto lf = std::make_unique<LineFormat>();
    const size_t n = fmt_str.size();
    size_t idx = 0;

    auto fail = [&](const char* msg){
      throw std::runtime_error(
          std::string("LineFormat::fromFormatString: ") + msg +
          " (offset " + std::to_string(idx) + " in \"" + fmt_str + "\")");
    };

    while(idx < n){
      const char c0 = fmt_str[idx];

      // Literal char outside a tag.
      if(c0 != '{'){
        if(c0 == ' ') lf->addField(new WhitespaceField());
        else          lf->addField(new LineChrField("", c0, false));
        ++idx;
        continue;
      }

      // Tag layout: '{' TAG ':' NAME (',' PARAMS)? '}'

      // 1. Find the ':' that ends the tag.
      const size_t tag_begin = idx + 1;
      size_t colon = tag_begin;
      while(colon < n && fmt_str[colon] != ':' && fmt_str[colon] != '}'){
        ++colon;
      }
      if(colon >= n || fmt_str[colon] != ':') fail("expected ':' after tag");
      const std::string tag = fmt_str.substr(tag_begin, colon - tag_begin);
      if(tag.empty()) fail("empty tag");

      // 2. Read name up to ',' or '}'.
      const size_t name_begin = colon + 1;
      size_t name_end = name_begin;
      while(name_end < n && fmt_str[name_end] != ',' && fmt_str[name_end] != '}'){
        ++name_end;
      }
      if(name_end >= n) fail("unterminated tag, expected ',' or '}'");
      const std::string name = fmt_str.substr(name_begin, name_end - name_begin);

      // 3. Parse params (if any) and locate the position after '}'.
      StrFieldStopType str_stop = DELIM;
      int  str_nchar  = 0;
      char str_delim  = 0;
      char chr_target = 0;
      bool chr_repeat = false;
      size_t next_idx = 0;

      if(fmt_str[name_end] == ','){
        size_t p = name_end + 1;

        if(tag == "STR"){
          // {STR:name,N}
          if(p >= n || fmt_str[p] < '0' || fmt_str[p] > '9'){
            fail("STR ',' form requires a digit count");
          }
          str_nchar = std::atoi(fmt_str.data() + p);
          while(p < n && fmt_str[p] >= '0' && fmt_str[p] <= '9') ++p;
          if(p >= n || fmt_str[p] != '}') fail("STR count must be followed by '}'");
          str_stop = NCHAR;
          next_idx = p + 1;

        } else if(tag == "CHR"){
          // {CHR:name,c,r}
          if(p + 3 >= n)             fail("truncated CHR params, expected 'c,r}'");
          chr_target = fmt_str[p];
          if(fmt_str[p + 1] != ',')  fail("CHR target must be followed by ','");
          chr_repeat = fmt_str[p + 2] != '0';
          if(fmt_str[p + 3] != '}')  fail("CHR repeat flag must be followed by '}'");
          next_idx = p + 4;

        } else {
          fail("tag does not accept parameters");
        }

      } else {
        // No-param form: '}' closes the tag.
        next_idx = name_end + 1;

        if(tag == "STR"){
          // STR with no count: stop at the char following '}', or ANY_WS if it's a space.
          const char follow = (next_idx < n) ? fmt_str[next_idx] : '\0';
          if(follow == ' '){
            str_stop = ANY_WS;
          } else {
            str_stop = DELIM;
            str_delim = follow;
          }
        }
      }

      // 4. Construct the field.
      if      (tag == "INT") lf->addField(new LineIntField(name));
      else if (tag == "DBL") lf->addField(new LineDblField(name));
      else if (tag == "STR") lf->addField(new LineStrField(name, str_stop, str_delim, str_nchar));
      else if (tag == "CHR") lf->addField(new LineChrField(name, chr_target, chr_repeat));
      else                   fail("unknown tag");

      idx = next_idx;
    }

    return lf;
  }
};

#endif
