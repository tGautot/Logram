#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <ctime>

#include "test_helpers.hpp"

// Tests for the DATE field type.
//
// Two groups:
//   1. Format-string parsing -- exercises fromFormatString, getFieldFromName,
//      and the field-count accessors. Independent of parse_date; passes now.
//   2. End-to-end parsing -- feeds real lines through Parser::parseLine.
//      Depends on parse_date being implemented in parsing_basics.cpp.
//      Expected values are produced by `expected_local_epoch_ms` below, which
//      mirrors parse_date's representation: local-time mktime() seconds * 1000
//      + ms, routed through std::chrono::system_clock for clarity. Both sides
//      use mktime() so DST/timezone effects cancel out on a single host.

// Mirrors parse_date's representation:
//   epoch milliseconds derived from a local-time mktime(), plus the ms field.
// Wrapped through std::chrono::system_clock::from_time_t for clarity.
static int64_t expected_local_epoch_ms(int year, int mo, int day,
                                       int hr = 0, int mn = 0, int sec = 0,
                                       int ms = 0) {
  std::tm t{};
  t.tm_year  = year - 1900;
  t.tm_mon   = mo - 1;
  t.tm_mday  = day;
  t.tm_hour  = hr;
  t.tm_min   = mn;
  t.tm_sec   = sec;
  t.tm_isdst = -1;  // let the C library figure DST out
  std::time_t s = std::mktime(&t);
  auto tp = std::chrono::system_clock::from_time_t(s);
  return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count() + ms;
}

// ---------------------------------------------------------------------------
// 1. Format-string parsing
// ---------------------------------------------------------------------------

TEST_CASE("DATE - fromFormatString builds a LineDateField with name and format") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD}");
  REQUIRE(lf->fields.size() == 1);
  auto* df = dynamic_cast<LineDateField*>(lf->fields[0]);
  REQUIRE(df != nullptr);
  REQUIRE(df->name == "ts");
  REQUIRE(df->opt != nullptr);
  REQUIRE(df->opt->format == "YYYY-MM-DD");
  teardown();
}

TEST_CASE("DATE - format string preserves spaces, colons, dots and dashes") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:created,YYYY-MM-DD hh:mm:ss.fff}");
  auto* df = dynamic_cast<LineDateField*>(lf->fields[0]);
  REQUIRE(df != nullptr);
  REQUIRE(df->opt->format == "YYYY-MM-DD hh:mm:ss.fff");
  teardown();
}

TEST_CASE("DATE - empty name is allowed") {
  setup();
  // {CHR:,...} allows an empty name; same convention should hold for DATE.
  auto lf = LineFormat::fromFormatString("{DATE:,YYYY}");
  auto* df = dynamic_cast<LineDateField*>(lf->fields[0]);
  REQUIRE(df != nullptr);
  REQUIRE(df->name.empty());
  REQUIRE(df->opt->format == "YYYY");
  teardown();
}

TEST_CASE("DATE - getFieldFromName returns the LineDateField") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:start,YYYY-MM-DD}");
  LineField* found = lf->getFieldFromName("start");
  REQUIRE(found != nullptr);
  REQUIRE(found->ft == FieldType::DATE);
  REQUIRE(dynamic_cast<LineDateField*>(found) != nullptr);
  teardown();
}

TEST_CASE("DATE - getNDateFields counts only DATE fields") {
  setup();
  auto lf = LineFormat::fromFormatString(
      "{DATE:a,YYYY-MM-DD} {INT:n} {DATE:b,hh:mm:ss}");
  REQUIRE(lf->getNDateFields()       == 2);
  REQUIRE(lf->getNIntFields()        == 1);
  REQUIRE(lf->getNStringFields()     == 0);
  REQUIRE(lf->getNCharFields()       == 0);
  REQUIRE(lf->getNDoubleFields()     == 0);
  REQUIRE(lf->getNWhiteSpaceFields() == 2);
  teardown();
}

TEST_CASE("DATE - missing format throws") {
  setup();
  // No-param form: DATE requires a format spec.
  REQUIRE_THROWS(LineFormat::fromFormatString("{DATE:ts}"));
  teardown();
}

TEST_CASE("DATE - empty format throws") {
  setup();
  REQUIRE_THROWS(LineFormat::fromFormatString("{DATE:ts,}"));
  teardown();
}

TEST_CASE("DATE - unterminated format throws") {
  setup();
  REQUIRE_THROWS(LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD"));
  teardown();
}

TEST_CASE("DATE - field appears at the right position when mixed with literals") {
  setup();
  auto lf = LineFormat::fromFormatString("[{DATE:ts,YYYY-MM-DD}] {STR:msg}");
  // Expected layout: '[', DATE, ']', WS, STR -> 5 fields.
  REQUIRE(lf->fields.size() == 5);
  REQUIRE(lf->fields[0]->ft == FieldType::CHR);   // '['
  REQUIRE(lf->fields[1]->ft == FieldType::DATE);
  REQUIRE(lf->fields[2]->ft == FieldType::CHR);   // ']'
  REQUIRE(lf->fields[3]->ft == FieldType::WS);
  REQUIRE(lf->fields[4]->ft == FieldType::STR);
  teardown();
}

// ---------------------------------------------------------------------------
// 2. End-to-end parsing (require a working parse_date)
// ---------------------------------------------------------------------------

TEST_CASE("DATE - single date-only field matches std::chrono epoch ms") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("2026-05-09", &pl));
  uint64_t val = *(pl.getDateField(0));
  LOG(3, "date field val:%lld, expected:%lld\n", val, expected_local_epoch_ms(2026, 5, 9));
  REQUIRE(*(pl.getDateField(0)) == expected_local_epoch_ms(2026, 5, 9));
  teardown();
}

TEST_CASE("DATE - parsing the same date twice gives the same int64") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("2026-05-08", &pl));
  int64_t a1 = *(pl.getDateField(0));
  REQUIRE(p->parseLine("2026-05-08", &pl));
  int64_t a2 = *(pl.getDateField(0));

  REQUIRE(a1 == a2);
  REQUIRE(a1 == expected_local_epoch_ms(2026, 5, 8));
  teardown();
}

TEST_CASE("DATE - consecutive days differ by 86_400_000 ms") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("2026-05-08", &pl));
  int64_t d0 = *(pl.getDateField(0));
  REQUIRE(p->parseLine("2026-05-09", &pl));
  int64_t d1 = *(pl.getDateField(0));

  REQUIRE(d0 == expected_local_epoch_ms(2026, 5, 8));
  REQUIRE(d1 == expected_local_epoch_ms(2026, 5, 9));
  // 24h * 60min * 60s * 1000ms (assuming no DST transition between the two)
  REQUIRE(d1 - d0 == 86'400'000);
  teardown();
}

TEST_CASE("DATE - mixed with STR field, cursor advances correctly") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD} {STR:msg}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("2026-05-08 hello world", &pl));
  REQUIRE(*(pl.getDateField(0)) == expected_local_epoch_ms(2026, 5, 8));
  REQUIRE(*(pl.getStrField(0))  == std::string_view("hello world"));
  teardown();
}

TEST_CASE("DATE - mixed with INT and STR fields") {
  setup();
  auto lf = LineFormat::fromFormatString(
      "{INT:lvl} {DATE:ts,YYYY-MM-DD} {STR:msg}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("3 2026-05-08 boot complete", &pl));
  REQUIRE(*(pl.getIntField(0))  == 3);
  REQUIRE(*(pl.getDateField(0)) == expected_local_epoch_ms(2026, 5, 8));
  REQUIRE(*(pl.getStrField(0))  == std::string_view("boot complete"));
  teardown();
}

TEST_CASE("DATE - multiple DATE fields populate distinct slots") {
  setup();
  auto lf = LineFormat::fromFormatString(
      "{DATE:start,YYYY-MM-DD}-{DATE:end,YYYY-MM-DD}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("2026-05-08-2026-05-09", &pl));
  REQUIRE(*(pl.getDateField(0)) == expected_local_epoch_ms(2026, 5, 8));
  REQUIRE(*(pl.getDateField(1)) == expected_local_epoch_ms(2026, 5, 9));
  teardown();
}

TEST_CASE("DATE - format with time component matches std::chrono") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD hh:mm:ss}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("2026-05-08 12:30:00", &pl));
  REQUIRE(*(pl.getDateField(0)) ==
          expected_local_epoch_ms(2026, 5, 8, 12, 30, 0));

  REQUIRE(p->parseLine("2026-05-08 12:30:01", &pl));
  REQUIRE(*(pl.getDateField(0)) ==
          expected_local_epoch_ms(2026, 5, 8, 12, 30, 1));
  teardown();
}

TEST_CASE("DATE - format with millisecond component matches std::chrono") {
  setup();
  // 'fff' is the millisecond placeholder convention used here. If you pick a
  // different one (e.g. 'SSS'), update the format spec in this test.
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD hh:mm:ss.fff}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  REQUIRE(p->parseLine("2026-05-08 12:30:00.000", &pl));
  REQUIRE(*(pl.getDateField(0)) ==
          expected_local_epoch_ms(2026, 5, 8, 12, 30, 0, 0));

  REQUIRE(p->parseLine("2026-05-08 12:30:00.001", &pl));
  REQUIRE(*(pl.getDateField(0)) ==
          expected_local_epoch_ms(2026, 5, 8, 12, 30, 0, 1));

  REQUIRE(p->parseLine("2026-05-08 12:30:00.999", &pl));
  REQUIRE(*(pl.getDateField(0)) ==
          expected_local_epoch_ms(2026, 5, 8, 12, 30, 0, 999));
  teardown();
}

TEST_CASE("DATE - parse_date failure causes parseLine to return false") {
  setup();
  auto lf = LineFormat::fromFormatString("{DATE:ts,YYYY-MM-DD}");
  auto p  = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(p->format.get());

  // Garbage that does not match YYYY-MM-DD must be rejected.
  REQUIRE_FALSE(p->parseLine("notadate!!", &pl));
  teardown();
}
