#include <catch2/catch_test_macros.hpp>

#include "line_filter.hpp"
#include "line_format.hpp"
#include "line_parser.hpp"
#include "filter_parsing.hpp"
#include "test_helpers.hpp"
#include "log_parser_terminal.hpp"
#include "terminal_modules.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <utility>

// Format: {INT:Val} {STR:Name}
static std::unique_ptr<LineFormat> makeSimpleFormat() {
  return LineFormat::fromFormatString("{INT:Val} {STR:Name}");
}

// Build a LogParserTerminal with FilterManagementModule registered,
// backed by the sample.log LPI used across the test suite.
static LogParserTerminal make_filter_term(CachedFilteredFileNavigator* cfn) {
  LogParserTerminal term(cfn);
  term.term_state.nrows = 20;
  term.term_state.ncols = 120;
  term.term_state.cy = 0;
  term.term_state.cx = 0;
  term.term_state.line_offset = 0;

  FilterManagementModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);
  mod.registerCommandCallback(term);

  return term;
}

// Send a command string through the terminal's command callback path.
static void send_cmd(LogParserTerminal& term, const std::string& cmd) {
  term.term_state.raw_input = cmd;
  term.submitRawInput();
}

// ──────────────────────────────────────────────
// find_next_bitwise_op
// ──────────────────────────────────────────────

TEST_CASE("find_next_bitwise_op - finds first operator") {
  setup();
  SECTION("AND") {
    std::string s = "foo AND bar";
    auto [pos, op] = find_next_bitwise_op(s);
    REQUIRE(pos == 4);
    REQUIRE(op == BitwiseOp::AND);
  }

  SECTION("OR") {
    std::string s = "foo OR bar";
    auto [pos, op] = find_next_bitwise_op(s);
    REQUIRE(pos == 4);
    REQUIRE(op == BitwiseOp::OR);
  }

  SECTION("XOR") {
    std::string s = "foo XOR bar";
    auto [pos, op] = find_next_bitwise_op(s);
    REQUIRE(pos == 4);
    REQUIRE(op == BitwiseOp::XOR);
  }

  SECTION("NOR") {
    std::string s = "foo NOR bar";
    auto [pos, op] = find_next_bitwise_op(s);
    REQUIRE(pos == 4);
    REQUIRE(op == BitwiseOp::NOR);
  }
  teardown();
}

TEST_CASE("find_next_bitwise_op - returns earliest match") {
  setup();
  std::string s = "a OR b AND c";
  auto [pos, op] = find_next_bitwise_op(s);
  REQUIRE(pos == 2);
  REQUIRE(op == BitwiseOp::OR);
  teardown();
}

TEST_CASE("find_next_bitwise_op - no match returns npos") {
  setup();
  std::string s = "Name EQ hello";
  auto [pos, op] = find_next_bitwise_op(s);
  REQUIRE(pos == std::string::npos);
  teardown();
}

TEST_CASE("find_next_bitwise_op - start_pos skips earlier matches") {
  setup();
  std::string s = "a OR b AND c";
  auto [pos, op] = find_next_bitwise_op(s, 5);
  REQUIRE(pos == 7);
  REQUIRE(op == BitwiseOp::AND);
  teardown();
}

// ──────────────────────────────────────────────
// find_next_comparator – short tags
// ──────────────────────────────────────────────

TEST_CASE("find_next_comparator - short tags") {
  setup();
  struct TestCase {
    const char* tag;
    FilterComparison expected;
  };

  TestCase cases[] = {
    {"EQ", FilterComparison::EQUAL},
    {"ST", FilterComparison::SMALLER},
    {"SE", FilterComparison::SMALLER_EQ},
    {"GT", FilterComparison::GREATER},
    {"GE", FilterComparison::GREATER_EQ},
    {"CT", FilterComparison::CONTAINS},
    {"BW", FilterComparison::BEGINS_WITH},
    {"EW", FilterComparison::ENDS_WITH},
  };

  for (auto& tc : cases) {
    SECTION(tc.tag) {
      std::string s = std::string("Name ") + tc.tag + " value";
      auto [pos, size, comp, ci] = find_next_comparator(s);
      REQUIRE(pos == 5);
      REQUIRE(size == strlen(tc.tag));
      REQUIRE(comp == tc.expected);
      REQUIRE_FALSE(ci);
    }
  }
  teardown();
}

// ──────────────────────────────────────────────
// find_next_comparator – long aliases
// ──────────────────────────────────────────────

TEST_CASE("find_next_comparator - long aliases") {
  setup();
  struct TestCase {
    const char* tag;
    FilterComparison expected;
  };

  TestCase cases[] = {
    {"EQUAL", FilterComparison::EQUAL},
    {"SMALLER", FilterComparison::SMALLER},
    {"SMALLER_THAN", FilterComparison::SMALLER},
    {"SMALLER_EQ", FilterComparison::SMALLER_EQ},
    {"SMALLER_EQUAL", FilterComparison::SMALLER_EQ},
    {"SMALLER_OR_EQUAL", FilterComparison::SMALLER_EQ},
    {"GREATER", FilterComparison::GREATER},
    {"GREATER_THAN", FilterComparison::GREATER},
    {"GREATER_EQ", FilterComparison::GREATER_EQ},
    {"GREATER_EQUAL", FilterComparison::GREATER_EQ},
    {"GREATER_OR_EQUAl", FilterComparison::GREATER_EQ},
    {"CONTAINS", FilterComparison::CONTAINS},
    {"BEGINS_WITH", FilterComparison::BEGINS_WITH},
    {"ENDS_WITH", FilterComparison::ENDS_WITH},
  };

  for (auto& tc : cases) {
    SECTION(tc.tag) {
      std::string s = std::string("Name ") + tc.tag + " value";
      auto [pos, size, comp, ci] = find_next_comparator(s);
      REQUIRE(pos == 5);
      REQUIRE(size == strlen(tc.tag));
      REQUIRE(comp == tc.expected);
      REQUIRE_FALSE(ci);
    }
  }
  teardown();
}

// ──────────────────────────────────────────────
// find_next_comparator – _CI suffix
// ──────────────────────────────────────────────

TEST_CASE("find_next_comparator - _CI suffix sets case-insensitive flag") {
  setup();
  struct TestCase {
    const char* tag;
    FilterComparison expected;
  };

  TestCase cases[] = {
    {"EQ_CI", FilterComparison::EQUAL},
    {"EQUAL_CI", FilterComparison::EQUAL},
    {"ST_CI", FilterComparison::SMALLER},
    {"GT_CI", FilterComparison::GREATER},
    {"CT_CI", FilterComparison::CONTAINS},
    {"CONTAINS_CI", FilterComparison::CONTAINS},
    {"BW_CI", FilterComparison::BEGINS_WITH},
    {"BEGINS_WITH_CI", FilterComparison::BEGINS_WITH},
    {"EW_CI", FilterComparison::ENDS_WITH},
    {"ENDS_WITH_CI", FilterComparison::ENDS_WITH},
    {"SE_CI", FilterComparison::SMALLER_EQ},
    {"GE_CI", FilterComparison::GREATER_EQ},
  };

  for (auto& tc : cases) {
    SECTION(tc.tag) {
      std::string s = std::string("Name ") + tc.tag + " value";
      auto [pos, size, comp, ci] = find_next_comparator(s);
      REQUIRE(pos == 5);
      REQUIRE(size == strlen(tc.tag));
      REQUIRE(comp == tc.expected);
      REQUIRE(ci);
    }
  }
  teardown();
}

TEST_CASE("find_next_comparator - start_pos") {
  setup();
  // The comparator after start_pos=8 should be found
  std::string s = "Name EQ x EQ y";
  auto [pos, size, comp, ci] = find_next_comparator(s, 8);
  REQUIRE(pos == 10);
  teardown();
}

// ──────────────────────────────────────────────
// parse_filter_decl – simple field filters
// ──────────────────────────────────────────────

TEST_CASE("parse_filter_decl - simple field filter") {
  setup();
  auto lf = makeSimpleFormat();
  auto parser = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "42 hello";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("EQ match") {
    auto f = parse_filter_decl("Name EQ hello", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("EQ miss") {
    auto f = parse_filter_decl("Name EQ world", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }

  SECTION("Long alias EQUAL") {
    auto f = parse_filter_decl("Name EQUAL hello", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("CONTAINS") {
    auto f = parse_filter_decl("Name CT ell", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("BEGINS_WITH") {
    auto f = parse_filter_decl("Name BW hel", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("ENDS_WITH") {
    auto f = parse_filter_decl("Name EW llo", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("CONTAINS miss") {
    auto f = parse_filter_decl("Name CT xyz", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }

  SECTION("Integer field - EQUAL") {
    auto f = parse_filter_decl("Val EQ 42", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("Integer field - GREATER") {
    auto f = parse_filter_decl("Val GT 10", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("Integer field - SMALLER") {
    auto f = parse_filter_decl("Val ST 100", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("Integer field - GREATER miss") {
    auto f = parse_filter_decl("Val GT 100", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }
  teardown();
}

// ──────────────────────────────────────────────
// parse_filter_decl – case-insensitive filters
// ──────────────────────────────────────────────

TEST_CASE("parse_filter_decl - case-insensitive filter") {
  setup();
  auto lf = makeSimpleFormat();
  auto parser = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "42 hello";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("EQ_CI matches mixed case") {
    auto f = parse_filter_decl("Name EQ_CI HELLO", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("CT_CI contains upper case") {
    auto f = parse_filter_decl("Name CT_CI ELL", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("CONTAINS_CI long alias") {
    auto f = parse_filter_decl("Name CONTAINS_CI ELL", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("BW_CI begins with upper case") {
    auto f = parse_filter_decl("Name BW_CI HEL", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("EW_CI ends with upper case") {
    auto f = parse_filter_decl("Name EW_CI LLO", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("EQ_CI miss") {
    auto f = parse_filter_decl("Name EQ_CI WORLD", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }
  teardown();
}

// ──────────────────────────────────────────────
// parse_filter_decl – combined filters (bool ops)
// ──────────────────────────────────────────────

TEST_CASE("parse_filter_decl - combined filters") {
  setup();
  auto lf = makeSimpleFormat();
  auto parser = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "42 hello";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("AND - both true") {
    auto f = parse_filter_decl("Name EQ hello AND Val EQ 42", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("AND - one false") {
    auto f = parse_filter_decl("Name EQ hello AND Val EQ 99", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }

  SECTION("OR - one true") {
    auto f = parse_filter_decl("Name EQ hello OR Val EQ 99", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("OR - both false") {
    auto f = parse_filter_decl("Name EQ world OR Val EQ 99", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }

  SECTION("XOR - one true") {
    auto f = parse_filter_decl("Name EQ hello XOR Val EQ 99", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("XOR - both true") {
    auto f = parse_filter_decl("Name EQ hello XOR Val EQ 42", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }

  SECTION("NOR - both false") {
    auto f = parse_filter_decl("Name EQ world NOR Val EQ 99", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("NOR - one true") {
    auto f = parse_filter_decl("Name EQ hello NOR Val EQ 99", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }
  teardown();
}

// ──────────────────────────────────────────────
// parse_filter_decl – parenthesized expressions
// ──────────────────────────────────────────────

TEST_CASE("parse_filter_decl - parenthesized expressions") {
  setup();
  auto lf = makeSimpleFormat();
  auto parser = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "42 hello";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("Redundant outer parens are stripped") {
    auto f = parse_filter_decl("(Name EQ hello)", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("Double-wrapped parens") {
    auto f = parse_filter_decl("((Name EQ hello))", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("Left-hand group: (A AND B) OR C") {
    // (true AND false) OR true  → false OR true → true
    auto f = parse_filter_decl("(Name EQ hello AND Val EQ 99) OR Val EQ 42", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("Left-hand group: (A OR B) AND C") {
    // (true OR false) AND false  → true AND false → false
    auto f = parse_filter_decl("(Name EQ hello OR Val EQ 99) AND Val EQ 99", parser->format.get());
    REQUIRE_FALSE(f->passes(&pl));
  }

  SECTION("Nested parens") {
    // ((true AND true)) OR false → true
    auto f = parse_filter_decl("((Name EQ hello AND Val EQ 42)) OR Name EQ world", parser->format.get());
    REQUIRE(f->passes(&pl));
  }
  teardown();
}

// ──────────────────────────────────────────────
// parse_filter_decl – whitespace handling
// ──────────────────────────────────────────────

TEST_CASE("parse_filter_decl - whitespace handling") {
  setup();
  auto lf = makeSimpleFormat();
  auto parser = Parser::fromLineFormat(std::move(lf));
  ParsedLine pl(parser->format.get());
  std::string line = "42 hello";
  REQUIRE(parser->parseLine(line, &pl));

  SECTION("Leading/trailing whitespace") {
    auto f = parse_filter_decl("  Name EQ hello  ", parser->format.get());
    REQUIRE(f->passes(&pl));
  }

  SECTION("Extra spaces around comparator") {
    auto f = parse_filter_decl("Name  EQ  hello", parser->format.get());
    REQUIRE(f->passes(&pl));
  }
  teardown();
}

// ──────────────────────────────────────────────
// parse_filter_decl – error cases
// ──────────────────────────────────────────────

TEST_CASE("parse_filter_decl - unknown field throws") {
  setup();
  auto lf = makeSimpleFormat();
  REQUIRE_THROWS_AS(
    parse_filter_decl("Unknown EQ hello", lf.get()),
    std::invalid_argument
  );
  teardown();
}

TEST_CASE("parse_filter_decl - unmatched paren throws") {
  setup();
  auto lf = makeSimpleFormat();
  REQUIRE_THROWS_AS(
    parse_filter_decl("(Name EQ hello", lf.get()),
    std::runtime_error
  );
  teardown();
}

// ──────────────────────────────────────────────
// parse_filter_decl – line_num special filter
// ──────────────────────────────────────────────

TEST_CASE("parse_filter_decl - line_num CT returns LineNumberFilter") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("Basic range") {
    auto f = parse_filter_decl("line_num CT 5,10", lf.get());
    auto* lnf = dynamic_cast<LineNumberFilter*>(f.get());
    REQUIRE(lnf != nullptr);
    REQUIRE(lnf->line_from == 5);
    REQUIRE(lnf->line_to == 10);
  }

  SECTION("CONTAINS long alias") {
    auto f = parse_filter_decl("line_num CONTAINS 5,10", lf.get());
    auto* lnf = dynamic_cast<LineNumberFilter*>(f.get());
    REQUIRE(lnf != nullptr);
    REQUIRE(lnf->line_from == 5);
    REQUIRE(lnf->line_to == 10);
  }

  SECTION("Whitespace around comma") {
    auto f = parse_filter_decl("line_num CT 5 , 10", lf.get());
    auto* lnf = dynamic_cast<LineNumberFilter*>(f.get());
    REQUIRE(lnf != nullptr);
    REQUIRE(lnf->line_from == 5);
    REQUIRE(lnf->line_to == 10);
  }

  SECTION("Single-line range") {
    auto f = parse_filter_decl("line_num CT 42,42", lf.get());
    auto* lnf = dynamic_cast<LineNumberFilter*>(f.get());
    REQUIRE(lnf != nullptr);
    REQUIRE(lnf->line_from == 42);
    REQUIRE(lnf->line_to == 42);
  }

  SECTION("Large range") {
    auto f = parse_filter_decl("line_num CT 0,999999", lf.get());
    auto* lnf = dynamic_cast<LineNumberFilter*>(f.get());
    REQUIRE(lnf != nullptr);
    REQUIRE(lnf->line_from == 0);
    REQUIRE(lnf->line_to == 999999);
  }
  teardown();
}

TEST_CASE("parse_filter_decl - line_num with wrong comparator throws") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("EQ") {
    REQUIRE_THROWS_AS(
      parse_filter_decl("line_num EQ 5,10", lf.get()),
      std::invalid_argument
    );
  }

  SECTION("GT") {
    REQUIRE_THROWS_AS(
      parse_filter_decl("line_num GT 5,10", lf.get()),
      std::invalid_argument
    );
  }

  SECTION("BW") {
    REQUIRE_THROWS_AS(
      parse_filter_decl("line_num BW 5,10", lf.get()),
      std::invalid_argument
    );
  }
  teardown();
}

TEST_CASE("parse_filter_decl - line_num with non-numeric value throws") {
  setup();
  auto lf = makeSimpleFormat();

  SECTION("Both non-numeric") {
    REQUIRE_THROWS_AS(
      parse_filter_decl("line_num CT abc,def", lf.get()),
      std::invalid_argument
    );
  }

  SECTION("First non-numeric") {
    REQUIRE_THROWS_AS(
      parse_filter_decl("line_num CT abc,10", lf.get()),
      std::invalid_argument
    );
  }

  SECTION("Second non-numeric") {
    REQUIRE_THROWS_AS(
      parse_filter_decl("line_num CT 5,def", lf.get()),
      std::invalid_argument
    );
  }
  teardown();
}

// ──────────────────────────────────────────────
// FilterManagementModule commands
// ──────────────────────────────────────────────

// sample.log uses: {INT:Date} {INT:Time} {STR:Level} ...
// The 10 INFO lines are a known subset (see test_helpers.hpp).

TEST_CASE("FilterManagementModule - :fclear removes filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  // Pre-condition: there is an active filter
  REQUIRE(cfn->getFilter() != nullptr);

  send_cmd(term, ":fclear");
  REQUIRE(cfn->getFilter() == nullptr);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fset sets a new filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  REQUIRE(cfn->getFilter() == nullptr);

  send_cmd(term, ":fset Level EQ INFO");
  REQUIRE(cfn->getFilter() != nullptr);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fset replaces existing filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  auto old_filter = cfn->getFilter();
  REQUIRE(old_filter != nullptr);

  send_cmd(term, ":fset Level EQ TRACE");
  auto new_filter = cfn->getFilter();
  REQUIRE(new_filter != nullptr);
  REQUIRE(new_filter != old_filter);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fset line_num CT sets LineNumberFilter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  send_cmd(term, ":fset line_num CT 5,10");
  auto filter = cfn->getFilter();
  REQUIRE(filter != nullptr);
  auto* lnf = dynamic_cast<LineNumberFilter*>(filter.get());
  REQUIRE(lnf != nullptr);
  REQUIRE(lnf->line_from == 5);
  REQUIRE(lnf->line_to == 10);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :f ANDs with existing filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  // Start with Level==INFO filter, add Mesg CONTAINS "Ioctl"
  send_cmd(term, ":f Mesg CT Ioctl");
  auto filter = cfn->getFilter();
  REQUIRE(filter != nullptr);

  // The combined filter should be narrower — verify it's a CombinedFilter
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined != nullptr);
  REQUIRE(combined->op == BitwiseOp::AND);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fadd ANDs with existing filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  send_cmd(term, ":fadd Mesg CT Ioctl");
  auto filter = cfn->getFilter();
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined != nullptr);
  REQUIRE(combined->op == BitwiseOp::AND);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fand ANDs with existing filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  send_cmd(term, ":fand Mesg CT Ioctl");
  auto filter = cfn->getFilter();
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined != nullptr);
  REQUIRE(combined->op == BitwiseOp::AND);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :for ORs with existing filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  send_cmd(term, ":for Level EQ TRACE");
  auto filter = cfn->getFilter();
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined != nullptr);
  REQUIRE(combined->op == BitwiseOp::OR);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fxor XORs with existing filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  send_cmd(term, ":fxor Level EQ TRACE");
  auto filter = cfn->getFilter();
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined != nullptr);
  REQUIRE(combined->op == BitwiseOp::XOR);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fnor NORs with existing filter") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  send_cmd(term, ":fnor Level EQ TRACE");
  auto filter = cfn->getFilter();
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined != nullptr);
  REQUIRE(combined->op == BitwiseOp::NOR);

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :fout inverts then ANDs") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  send_cmd(term, ":fout Level EQ TRACE");
  auto filter = cfn->getFilter();
  LOG(1, "Got filter %p\n", filter.get());
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined != nullptr);
  REQUIRE(combined->op == BitwiseOp::AND);
  // The right-hand filter should be inverted
  REQUIRE(combined->right_filter->is_inverted());

  delete cfn;
  teardown();
}

TEST_CASE("FilterManagementModule - :f on empty filter just sets it") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  REQUIRE(cfn->getFilter() == nullptr);

  // :f with no existing filter should just set, not combine
  send_cmd(term, ":f Level EQ INFO");
  auto filter = cfn->getFilter();
  REQUIRE(filter != nullptr);
  // Should NOT be a CombinedFilter — there was nothing to combine with
  auto* combined = dynamic_cast<CombinedFilter*>(filter.get());
  REQUIRE(combined == nullptr);

  delete cfn;
  teardown();
}

// ──────────────────────────────────────────────
// Cursor repositioning when filter changes
// ──────────────────────────────────────────────

// sample.log (62 lines, 0-indexed):
//   INFO at globals:  4, 12, 14, 20, 29, 36, 41, 49, 51, 57
//   Binary (bad fmt): 25, 26, 27, 28
// With INFO filter + accept_bad_format:
//   valid_line_index = [4,12,14,20, 25,26,27,28, 29,36,41,49,51,57]
//   local: 0  1  2  3   4  5  6  7   8  9 10 11 12 13

TEST_CASE("Cursor reposition - fset: cursor on passing line stays on same global line") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  // Cursor on global 4 (first INFO line)
  term.term_state.cy = 4;
  term.term_state.line_offset = 0;

  send_cmd(term, ":fset Level EQ INFO");

  // Global 4 passes → local 0
  REQUIRE(term.term_state.line_offset + term.term_state.cy == 0);
  REQUIRE(cfn->localToGlobalLineId(term.term_state.line_offset + term.term_state.cy) == 4);

  delete cfn;
  teardown();
}

TEST_CASE("Cursor reposition - fset: cursor on passing line with line_offset") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  // Cursor on global 12 (second INFO) via cy=2, offset=10
  term.term_state.cy = 2;
  term.term_state.line_offset = 10;

  send_cmd(term, ":fset Level EQ INFO");

  // Global 12 passes → local 1
  REQUIRE(term.term_state.line_offset + term.term_state.cy == 1);
  REQUIRE(cfn->localToGlobalLineId(term.term_state.line_offset + term.term_state.cy) == 12);

  delete cfn;
  teardown();
}

TEST_CASE("Cursor reposition - fset: cursor on non-passing line, nearby prior passing line") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  // Cursor on global 5 (TRACE, right after INFO at global 4)
  term.term_state.cy = 5;
  term.term_state.line_offset = 0;

  send_cmd(term, ":fset Level EQ INFO");

  // Global 5 doesn't pass → prior passing = global 4 → local 0
  REQUIRE(term.term_state.line_offset + term.term_state.cy == 0);
  REQUIRE(cfn->localToGlobalLineId(term.term_state.line_offset + term.term_state.cy) == 4);

  delete cfn;
  teardown();
}

TEST_CASE("Cursor reposition - fset: cursor on non-passing line, distant prior passing line") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  // Cursor on global 30 (TRACE, just after INFO at global 29)
  // Prior passing line: global 29 → local 8
  term.term_state.cy = 0;
  term.term_state.line_offset = 30;

  send_cmd(term, ":fset Level EQ INFO");

  line_t cursor_local = term.term_state.line_offset + term.term_state.cy;
  REQUIRE(cursor_local == 8);
  REQUIRE(cfn->localToGlobalLineId(cursor_local) == 29);

  delete cfn;
  teardown();
}

TEST_CASE("Cursor reposition - fset: cursor before any passing line goes to first passing") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  // Cursor on global 0 (TRACE, before any INFO line)
  term.term_state.cy = 0;
  term.term_state.line_offset = 0;

  send_cmd(term, ":fset Level EQ INFO");

  // No prior passing line → first passing = global 4 → local 0
  REQUIRE(term.term_state.line_offset + term.term_state.cy == 0);
  REQUIRE(cfn->localToGlobalLineId(term.term_state.line_offset + term.term_state.cy) == 4);

  delete cfn;
  teardown();
}

TEST_CASE("Cursor reposition - fset: cursor near end of file goes to last prior passing line") {
  setup();
  CachedFilteredFileNavigator* cfn = make_cfn();
  auto term = make_filter_term(cfn);

  // Cursor on global 60 (TRACE, near end of file)
  // Prior passing line: global 57 → local 13
  term.term_state.cy = 0;
  term.term_state.line_offset = 60;

  send_cmd(term, ":fset Level EQ INFO");

  line_t cursor_local = term.term_state.line_offset + term.term_state.cy;
  REQUIRE(cursor_local == 13);
  REQUIRE(cfn->localToGlobalLineId(cursor_local) == 57);

  delete cfn;
  teardown();
}

TEST_CASE("Cursor reposition - fclear: cursor stays on same global line") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  // With INFO filter, cursor at local 0 → global 4
  term.term_state.cy = 0;
  term.term_state.line_offset = 0;

  send_cmd(term, ":fclear");

  // After clearing, global 4 → local 4 (all lines visible)
  REQUIRE(term.term_state.line_offset + term.term_state.cy == 4);
  REQUIRE(cfn->localToGlobalLineId(term.term_state.line_offset + term.term_state.cy) == 4);

  delete cfn;
  teardown();
}

TEST_CASE("Cursor reposition - fclear: preserves screen position with line_offset") {
  setup();
  CachedFilteredFileNavigator* cfn = make_info_filtered_cfn();
  auto term = make_filter_term(cfn);

  // With INFO filter, cursor at local 9 → global 36, displayed at cy=9
  term.term_state.cy = 9;
  term.term_state.line_offset = 0;

  send_cmd(term, ":fclear");

  // After clearing, global 36 → local 36 (all lines visible)
  // new_local(36) > cy(9) → line_offset = 36 - 9 = 27, cy stays 9
  REQUIRE(term.term_state.cy == 9);
  REQUIRE(term.term_state.line_offset == 27);
  REQUIRE(cfn->localToGlobalLineId(term.term_state.line_offset + term.term_state.cy) == 36);

  delete cfn;
  teardown();
}
