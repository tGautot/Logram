#pragma once

#include "line_format.hpp"
#include "line_parser.hpp"
#include "line_filter.hpp"
#include "processed_line.hpp"
#include "filtered_file_reader.hpp"
#include "cached_filtered_file_navigator.hpp"

#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

extern "C" {
  #include "logging.h"
}
#include "logging.hpp"

#define SV_TO_STR(sv) std::string(sv.data(), sv.length())
#define STRING_VIEW_PRINT(sv) static_cast<int>(sv.length()), sv.data()

#define TEST_FOLDER "../../tests/"
#define SAMPLE_LOG TEST_FOLDER "data/sample.log"

// sample.log has 62 raw lines (local IDs 0-61 unfiltered)
static constexpr int TOTAL_LINES = 62;
// With INFO filter + accept_bad_format: 10 INFO + 4 binary = 14
static constexpr int FILTERED_LINES = 14;

extern int g_log_level;

inline void setup() {
  logger_setup();
  logger_set_minlvl(g_log_level);
}

inline void teardown() {
  logger_teardown();
}

// The default log format used across most integration tests (for data/sample.log):
//   {INT:Date} {INT:Time} {STR:Level} :{CHR:,.,1}{STR:Source}: {STR:Mesg}
//   2 INT, 0 DBL, 3 CHR (':', '.', ':'), 3 STR, 4 WS
inline std::unique_ptr<LineFormat> getDefaultLineFormat() {
  std::unique_ptr<LineFormat> lf = std::make_unique<LineFormat>();
  lf->addField(new LineIntField("Date"));
  lf->addField(new WhitespaceField());
  lf->addField(new LineIntField("Time"));
  lf->addField(new WhitespaceField());
  lf->addField(new LineStrField("Level", StrFieldStopType::DELIM, ' ', 0));
  lf->addField(new WhitespaceField());
  lf->addField(new LineChrField("", ':', false));
  lf->addField(new LineChrField("", '.', true));
  lf->addField(new LineStrField("Source", StrFieldStopType::DELIM, ':', 0));
  lf->addField(new LineChrField("", ':', false));
  lf->addField(new WhitespaceField());
  lf->addField(new LineStrField("Mesg", StrFieldStopType::DELIM, 0, 0));
  return lf;
}

// The 10 INFO lines from sample.log (used by FilteredFileReader and interface tests)
static const std::string_view info_lines[10] = {
  "0322 085338 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085352 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085352 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085353 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085353 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085409 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085409 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085422 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085422 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085424 INFO   :......router_forward_getOI: Ioctl to query route entry successful"
};

// Same lines interleaved with binary-format (non-parseable) lines,
// as returned by CachedFilteredFileNavigator (which includes surrounding context)
static const std::string_view info_and_bf_lines[14] = {
  "0322 085338 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085352 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085352 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085353 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0x00 0x01 0x02 0x03 ..Da..Ba",
  "0x04 0x05 0x06 0x07 ..Da..Ba",
  "0x08 0x09 0x0A 0x0B ..Da..Ba",
  "0x0C 0x0D 0x0E 0x0F ..Da..Ba",
  "0322 085353 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085409 INFO   :......router_forward_getOI: Ioctl to query route entry successful",
  "0322 085409 INFO   :......rsvp_flow_stateMachine: state RESVED, event T1OUT",
  "0322 085422 INFO   :.......rsvp_parse_objects: obj RSVP_HOP hop=9.67.116.99, lih=0",
  "0322 085422 INFO   :.......rsvp_flow_stateMachine: state RESVED, event RESV",
  "0322 085424 INFO   :......router_forward_getOI: Ioctl to query route entry successful"
};

// Maps the nth INFO line (local id) to its global line number in sample.log
inline CachedFilteredFileNavigator* make_cfn(int bsize = 1000) {
  std::string filename = SAMPLE_LOG;
  return new CachedFilteredFileNavigator(filename, getDefaultLineFormat(), nullptr, bsize);
}

inline CachedFilteredFileNavigator* make_info_filtered_cfn(int bsize = 1000) {
  std::string filename = SAMPLE_LOG;
  auto lf = getDefaultLineFormat();
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(
      lf.get(), "Level", FilterComparison::EQUAL, &val);
  return new CachedFilteredFileNavigator(filename, std::move(lf), filter, bsize);
}

inline int count_to_info_line(int id) {
  switch (id) {
    case 0: return 4;
    case 1: return 12;
    case 2: return 14;
    case 3: return 20;
    case 4: return 29;
    case 5: return 36;
    case 6: return 41;
    case 7: return 49;
    case 8: return 51;
    case 9: return 57;
  }
  return -1;
}
