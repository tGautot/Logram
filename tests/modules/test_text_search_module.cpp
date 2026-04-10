#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include "logram_terminal.hpp"
#include "terminal_modules.hpp"

// ACTION codes from text_search_module.cpp (file-scope #defines, not exported)
#define ACTION_SEARCH_NEXT 100
#define ACTION_SEARCH_PREV 101
#define ACTION_START_SEARCH 110

// Helper: build a terminal with TextSearchModule fully registered and display populated
static LogramTerminal make_search_term(CachedFilteredFileNavigator* cfn, int nrows, int ncols,
                                           int cy, uint64_t line_offset) {
  LogramTerminal term(cfn);
  term.term_state.nrows = nrows;
  term.term_state.ncols = ncols;
  term.term_state.cy = cy;
  term.term_state.cx = 0;
  term.term_state.line_offset = line_offset;

  TextSearchModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);
  mod.registerCommandCallback(term);

  term.updateDisplayState();
  return term;
}

// Simulate the user typing /<pattern><Enter>
static void trigger_search(LogramTerminal& term, const std::string& pattern) {
  term.term_state.raw_input = ":?" + pattern;
  term.submitRawInput();
}

// ============================================================
// Input mappings
// ============================================================

TEST_CASE("TextSearchModule - input mappings") {
  setup();
  auto* cfn = make_cfn();
  LogramTerminal term(cfn);

  TextSearchModule mod;
  mod.registerUserInputMapping(term);

  bool partial = false;

  SECTION("n maps to SEARCH_NEXT") {
    REQUIRE(term.matchInputSequence("n", partial) == ACTION_SEARCH_NEXT);
    REQUIRE_FALSE(partial);
  }
  SECTION("N maps to SEARCH_PREV") {
    REQUIRE(term.matchInputSequence("N", partial) == ACTION_SEARCH_PREV);
    REQUIRE_FALSE(partial);
  }
  SECTION("/ maps to START_SEARCH") {
    REQUIRE(term.matchInputSequence("/", partial) == ACTION_START_SEARCH);
    REQUIRE_FALSE(partial);
  }

  teardown();
}

// ============================================================
// ACTION_START_SEARCH (/)
// ============================================================

TEST_CASE("TextSearchModule - / sets RAW input mode with :? prefix") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  bool partial = false;
  term.handleUserAction(term.matchInputSequence("/", partial));

  REQUIRE(term.term_state.input_mode == RAW);
  REQUIRE(term.term_state.raw_input == ":?");

  teardown();
}

// ============================================================
// Forward search via :? command
// ============================================================

TEST_CASE("TextSearchModule - forward search jumps to match") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  // "LASTLINE" is on global/local line 61
  trigger_search(term, "LASTLINE");

  REQUIRE(term.term_state.cy + term.term_state.line_offset == 61);
  REQUIRE(term.term_state.highlight_word == "LASTLINE");

  teardown();
}

TEST_CASE("TextSearchModule - forward search skips current line") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  // "source address" appears on global lines 0, 21, 37, 58.
  // Cursor is on line 0 → forward starts from line 1.
  // Next "source address" is on global line 21.
  trigger_search(term, "source address");

  REQUIRE(term.term_state.cy + term.term_state.line_offset == 21);

  teardown();
}

TEST_CASE("TextSearchModule - no match leaves cursor unchanged") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_search_term(cfn, 25, 80, 5, 10);

  trigger_search(term, "ZZZZNOTFOUND");

  REQUIRE(term.term_state.cy + term.term_state.line_offset == 15);

  teardown();
}

TEST_CASE("TextSearchModule - search sets cx to match position") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  trigger_search(term, "LASTLINE");

  // Raw line: "0322 085424 TRACE  :......router_forward_getOI:         route handle:   LASTLINE"
  std::string raw = "0322 085424 TRACE  :......router_forward_getOI:         route handle:   LASTLINE";
  size_t expected_pos = raw.find("LASTLINE");
  REQUIRE(term.term_state.cx == (int)(expected_pos + term.term_state.info_col_size));

  teardown();
}

// ============================================================
// Search next (n) / prev (N)
// ============================================================

TEST_CASE("TextSearchModule - search next finds subsequent match") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  // "Ioctl" appears on global lines 20, 36, 57
  trigger_search(term, "Ioctl");

  // First match: global line 20
  REQUIRE(term.term_state.cy + term.term_state.line_offset == 20);

  // Refresh display for the new position, then search next
  term.updateDisplayState();
  term.handleUserAction(ACTION_SEARCH_NEXT);

  // Second match: global line 36
  REQUIRE(term.term_state.cy + term.term_state.line_offset == 36);

  teardown();
}

TEST_CASE("TextSearchModule - search prev goes backward") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  // Search forward to "Ioctl" → lands on global 20
  trigger_search(term, "Ioctl");
  REQUIRE(term.term_state.cy + term.term_state.line_offset == 20);

  // Search next → lands on global 36
  term.updateDisplayState();
  term.handleUserAction(ACTION_SEARCH_NEXT);
  REQUIRE(term.term_state.cy + term.term_state.line_offset == 36);

  // Search prev → should go back to global 20
  term.updateDisplayState();
  term.handleUserAction(ACTION_SEARCH_PREV);
  REQUIRE(term.term_state.cy + term.term_state.line_offset == 20);

  teardown();
}

// ============================================================
// Filtered search — exposes global-vs-local line offset bug
// ============================================================

TEST_CASE("TextSearchModule - filtered: search uses local line offset") {
  setup();
  // INFO filter: 14 valid lines (local 0-13)
  // "Ioctl" appears on INFO lines at global 20, 36, 57
  // which are local 3, 9, 13
  auto* cfn = make_info_filtered_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  // First INFO "Ioctl" match is local 3 (global 20)
  trigger_search(term, "Ioctl");

  REQUIRE(term.term_state.cy + term.term_state.line_offset == 3);

  teardown();
}

TEST_CASE("TextSearchModule - filtered: search next uses local offset") {
  setup();
  auto* cfn = make_info_filtered_cfn();
  auto term = make_search_term(cfn, 25, 80, 0, 0);

  // First match: local 3 (global 20)
  trigger_search(term, "Ioctl");

  // Search next: local 9 (global 36)
  term.updateDisplayState();
  term.handleUserAction(ACTION_SEARCH_NEXT);

  REQUIRE(term.term_state.cy + term.term_state.line_offset == 9);

  teardown();
}
