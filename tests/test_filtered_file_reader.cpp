#include <catch2/catch_test_macros.hpp>

#include "ConfigHandler.hpp"
#include "processed_line.hpp"
#include "test_helpers.hpp"

TEST_CASE("Basic Filtered File Reader") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  // Shared across sections so that state carries over — stronger testing,
  // harder to debug in isolation.
  FilteredFileReader ffr(filename, std::move(lf), nullptr);
  ProcessedLine pl;

  SECTION("Filter on string, forward") {
    ffr.m_config->accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(ffr.m_config->parser->format.get(), "Level", FilterComparison::EQUAL, base_val);
    ffr.setFilter(filter);

    int count = 0;
    while (ffr.getNextValidLine(pl) != 0) {
      std::cout << "Got next valid line: " << pl.raw_line << std::endl;
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
      count++;
    }
    REQUIRE(count == 10);
  }

  SECTION("Filter on string, backwards") {
    ffr.m_config->accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(ffr.m_config->parser->format.get(), "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);

    while(ffr.getNextValidLine(pl)); // go to EOF

    int count = 9;
    while (ffr.getPreviousValidLine(pl) != 0) {
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
      count--;
    }
    REQUIRE(count == -1);
  }

  SECTION("Filter on string, forward then backwards") {
    ffr.m_config->accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(ffr.m_config->parser->format.get(), "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);

    ffr.seekRawLine(0);
    int count = 0;
    while (ffr.getNextValidLine( pl) != 0) {
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
      count++;
    }
    REQUIRE(count == 10);
    while (ffr.getPreviousValidLine( pl) != 0) {
      count--;
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
    }
    REQUIRE(count == 0);
  }

  SECTION("Filter on string, backwards then forward") {
    ffr.m_config->accept_bad_format = false;
    std::string base_val = "INFO";
    std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(ffr.m_config->parser->format.get(), "Level", FilterComparison::EQUAL, &base_val);
    ffr.setFilter(filter);

    while(ffr.getNextValidLine(pl)); // go to EOF

    int count = 9;
    while (ffr.getPreviousValidLine(pl)) {
      LOG(1, "Got previous line %llu %.*s\n", pl.line_num, STRING_VIEW_PRINT(pl.raw_line));
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
      count--;
    }
    count++;
    REQUIRE(count == 0);
    while (ffr.getNextValidLine( pl)) {
      LOG(1, "Got next line %llu %.*s\n", pl.line_num, STRING_VIEW_PRINT(pl.raw_line));
      REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
      REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
      count++;
    }
    REQUIRE(count == 10);
  }

  teardown();
}

// ──────────────────────────────────────────────
// getPreviousValidLine – boundary behaviour
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::getPreviousValidLine - returns 0 at beginning of file") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, std::move(lf), filter);
  ffr.m_config->accept_bad_format = false;
  ProcessedLine pl;

  // At the very start there is no previous line.
  REQUIRE(ffr.getPreviousValidLine(pl) == 0);

  teardown();
}

// ──────────────────────────────────────────────
// seekRawLine – repositions the reader
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::seekRawLine - repositions the reader") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, std::move(lf), filter);
  ffr.m_config->accept_bad_format = false;
  ProcessedLine pl;

  SECTION("seekRawLine(0) after reading restarts iteration from the first line") {
    ffr.getNextValidLine(pl);
    ffr.getNextValidLine(pl);
    ffr.seekRawLine(0);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  while(ffr.getNextValidLine(pl)); // Index whole file

  SECTION("seekRawLine to a matching line returns that line first") {
    ffr.seekRawLine(count_to_info_line(3)); // raw line 20, which is an INFO line
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(3));
  }

  SECTION("seekRawLine to a non-matching line advances to the next valid line") {
    // Seek one past the first INFO line (raw line 4 → seek to 5).
    // The next INFO line after that is count_to_info_line(1).
    ffr.seekRawLine((line_t)count_to_info_line(0) + 1);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(1));
  }

  SECTION("seekRawLine then getPreviousValidLine returns 0 when nothing precedes seek point") {
    ffr.seekRawLine(count_to_info_line(0)); // seek to exactly the first INFO line
    // Nothing valid before this point.
    REQUIRE(ffr.getPreviousValidLine(pl) == 0);
  }

  teardown();
}

// ──────────────────────────────────────────────
// setFilter – changes active filter and resets position
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::setFilter - changes the active filter and resets position") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";

  FilteredFileReader ffr(filename, std::move(lf), nullptr);
  ffr.m_config->accept_bad_format = false;
  ProcessedLine pl;

  SECTION("setFilter resets iteration to the beginning of the file") {
    std::string v = "INFO";
    ffr.setFilter(std::make_shared<FieldFilter>(ffr.m_config->parser->format.get(), "Level", FilterComparison::EQUAL, &v));
    ffr.getNextValidLine(pl); // consume first INFO line
    ffr.getNextValidLine(pl); // consume second INFO line
    REQUIRE(pl.line_num > (line_t)count_to_info_line(0));

    std::string v2 = "INFO";
    ffr.setFilter(std::make_shared<FieldFilter>(ffr.m_config->parser->format.get(), "Level", FilterComparison::EQUAL, &v2));
    ffr.getNextValidLine(pl); // should be back at the first INFO line
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("setFilter(nullptr) removes the filter; all lines are returned") {
    std::string v = "INFO";
    ffr.setFilter(std::make_shared<FieldFilter>(ffr.m_config->parser->format.get(), "Level", FilterComparison::EQUAL, &v));
    int with_filter = 0;
    while (ffr.getNextValidLine(pl)) with_filter++;
    REQUIRE(with_filter == 10);

    ffr.setFilter(nullptr);
    int without_filter = 0;
    LOG(1, "ffr [accept_bad_fmt]=%d\n", ffr.m_config->accept_bad_format);
    while (ffr.getNextValidLine(pl)) without_filter++;
    // accept_bad_format=false: 4 binary lines rejected → 62-4=58
    REQUIRE(without_filter == 58);
  }

  teardown();
}

// ──────────────────────────────────────────────
// setFormat – switches the active parsing format
//
// NOTE: these tests document INTENDED post-refactor behaviour.
// The current implementation updates m_lf but does not rebuild
// the internal Parser, so they are expected to fail until the
// refactor is complete.
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::setFormat - switches the active parsing format") {
  setup();
  std::string filename = TEST_FOLDER "data/sample.log";

  // Correct format: matches the log lines, all well-formatted.
  std::unique_ptr<LineFormat> lf_correct = getDefaultLineFormat();

  // Wrong format: five consecutive INT fields — will fail to parse log lines.
  std::unique_ptr<LineFormat> lf_wrong = LineFormat::fromFormatString("{INT:A} {INT:B} {INT:C} {INT:D} {INT:E}");

  FilteredFileReader ffr(filename, std::move(lf_correct), nullptr);
  ProcessedLine pl;

  SECTION("with correct format lines parse successfully") {
    ffr.getNextValidLine(pl);
    REQUIRE(pl.well_formated == true);
  }

  SECTION("after setFormat(wrong), lines fail to parse") {
    ffr.setFormat(std::move(lf_wrong));
    // NEED to reset Processed Line's Parsed Line after changing format to trigger re-allocation
    pl.pl = nullptr;
    ffr.seekRawLine(0);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.well_formated == false);
  }

  SECTION("after setFormat(wrong) then setFormat(correct), lines parse again") {
    ffr.setFormat(std::move(lf_wrong));
    pl.pl = nullptr;
    ffr.seekRawLine(0);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.well_formated == false);

    std::unique_ptr<LineFormat> lf_correct2 = getDefaultLineFormat();
    ffr.setFormat(std::move(lf_correct2));
    pl.pl = nullptr;
    ffr.seekRawLine(0);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.well_formated == true);
  }

  teardown();
}

// ──────────────────────────────────────────────
// goToPosition – repositions using a stored stream position
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::goToPosition - repositions using a stored stream position") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, std::move(lf), filter);
  ffr.m_config->accept_bad_format = false;
  ProcessedLine pl;

  SECTION("getNextValidLine after jumpToLine returns the same line") {
    ProcessedLine info0;
    ffr.getNextValidLine(info0);  // INFO line 0 at raw line 4
    ffr.getNextValidLine(pl);      // advance past it

    ffr.jumpToGlobalLine(info0.line_num);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("getPreviousValidLine after jumpToLine returns lines before the position") {
    ProcessedLine info1;
    ffr.getNextValidLine(pl);      // INFO line 0 (raw line 4)
    ffr.getNextValidLine(info1);  // INFO line 1 (raw line 12)

    // Seek to start of INFO line 1; previous valid line should be INFO line 0.
    ffr.jumpToGlobalLine(info1.line_num);
    ffr.getPreviousValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  teardown();
}

// ──────────────────────────────────────────────
// skipNextRawLines – advances exactly N raw lines
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader::skipNextRawLines - advances exactly N raw lines") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, std::move(lf), filter);
  ffr.m_config->accept_bad_format = false;
  ProcessedLine pl;

  // INFO lines are at raw lines: 4, 12, 14, 20, ...  (0-indexed)

  SECTION("skipNextRawLines(0) does not move the position") {
    ffr.seekRawLine(0);
    ffr.skipNextRawLines(0);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  while(ffr.getNextValidLine(pl)); // Index whole file

  SECTION("skipNextRawLines lands exactly on an INFO line") {
    // raw line 4 is an INFO line; skip to it and it is the next result.
    ffr.seekRawLine(0);
    ffr.skipNextRawLines(count_to_info_line(0)); // skip 4 lines → now at raw line 4
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(0));
  }

  SECTION("skipNextRawLines lands past an INFO line, next valid is the following one") {
    // raw line 4 is INFO; skip 5 lines → now at raw line 5 (not INFO).
    // Next INFO line is at raw line 12.
    ffr.seekRawLine(0);
    ffr.skipNextRawLines(count_to_info_line(0) + 1);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(1));
  }

  SECTION("skipNextRawLines(1) after goToPosition steps past the seeked line") {
    ProcessedLine info1;
    ffr.seekRawLine(0);
    ffr.getNextValidLine(pl);       // INFO line 0
    ffr.getNextValidLine(info1);   // INFO line 1 at raw line 12

    // Seek to the start of INFO line 1, then step past it.
    ffr.jumpToGlobalLine(info1.line_num);
    ffr.skipNextRawLines(1);
    ffr.getNextValidLine(pl);
    REQUIRE(pl.line_num == (line_t)count_to_info_line(2)); // INFO line 2 at raw line 14
  }

  teardown();
}

// ──────────────────────────────────────────────
// CRLF file handling
// ──────────────────────────────────────────────

TEST_CASE("FilteredFileReader - CRLF line endings, forward") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample_crlf.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, std::move(lf), filter);
  ffr.m_config->accept_bad_format = false;
  ProcessedLine pl;

  int count = 0;
  while (ffr.getNextValidLine(pl)) {
    REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
    REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
    count++;
  }
  REQUIRE(count == 10);

  teardown();
}

TEST_CASE("FilteredFileReader - CRLF line endings, backward") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample_crlf.log";
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &val);

  FilteredFileReader ffr(filename, std::move(lf), filter);
  ffr.m_config->accept_bad_format = false;
  ProcessedLine pl;

  while (ffr.getNextValidLine(pl)); // go to EOF

  int count = 9;
  while (ffr.getPreviousValidLine(pl)) {
    REQUIRE(pl.line_num == (line_t)count_to_info_line(count));
    REQUIRE(SV_TO_STR(pl.raw_line) == info_lines[count]);
    count--;
  }
  REQUIRE(count == -1);

  teardown();
}
