#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include "logram_terminal.hpp"
#include "terminal_modules.hpp"

// Helper: create a terminal with the cursor module registered, display state populated
static LogramTerminal make_term(CachedFilteredFileNavigator* cfn, int nrows, int ncols,
                                   int cy, int cx, uint64_t line_offset) {
  LogramTerminal term(cfn);
  term.term_state.nrows = nrows;
  term.term_state.ncols = ncols;
  term.term_state.cy = cy;
  term.term_state.cx = cx;
  term.term_state.line_offset = line_offset;

  CursorMoveModule mod;
  mod.registerUserActionCallback(term);

  term.updateDisplayState();
  return term;
}

// ============================================================
// ACTION_MOVE_DOWN
// ============================================================

TEST_CASE("CursorMove - move down basic") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 10, 0);

  term.handleUserAction(ACTION_MOVE_DOWN);
  REQUIRE(term.term_state.cy == 1);
  REQUIRE(term.term_state.line_offset == 0);
  teardown();
}

TEST_CASE("CursorMove - move down scrolls at boundary") {
  setup();
  auto* cfn = make_cfn();
  // nrows=25, num_status_line=1, not on last line => boundary = 25-1-1-4 = 19
  auto term = make_term(cfn, 25, 80, 19, 10, 0);

  term.handleUserAction(ACTION_MOVE_DOWN);
  REQUIRE(term.term_state.cy == 19);
  REQUIRE(term.term_state.line_offset == 1);
  teardown();
}

TEST_CASE("CursorMove - move down stops at EOF") {
  setup();
  auto* cfn = make_cfn();
  // 62 lines, 0-indexed => known_last_line = 61
  // Put cursor on last line: cy + line_offset == 61
  auto term = make_term(cfn, 25, 80, 10, 10, 51);

  int old_cy = term.term_state.cy;
  uint64_t old_offset = term.term_state.line_offset;
  term.handleUserAction(ACTION_MOVE_DOWN);
  // Should not move or scroll
  REQUIRE(term.term_state.cy == old_cy);
  REQUIRE(term.term_state.line_offset == old_offset);
  teardown();
}

TEST_CASE("CursorMove - move down stops at nullptr line") {
  setup();
  auto* cfn = make_cfn();
  // Offset 58 => lines 58-61 exist (4 lines), rest are nullptr
  // With nrows=25, displayed_pls has 24 slots; [0]-[3] have data, [4]+ are nullptr
  // cy=3 is on last data line, cy+1=4 is nullptr
  // cy + line_offset = 3 + 58 = 61 = known_last_line => on last line
  // boundary = nrows-1-num_status_line-0 = 23, cy(3) < 23 => tries to move
  // but displayed_pls[4] == nullptr => should not move
  auto term = make_term(cfn, 25, 80, 3, 10, 58);

  term.handleUserAction(ACTION_MOVE_DOWN);
  REQUIRE(term.term_state.cy == 3);
  teardown();
}

// ============================================================
// ACTION_MOVE_UP
// ============================================================

TEST_CASE("CursorMove - move up basic") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 10, 10, 0);

  term.handleUserAction(ACTION_MOVE_UP);
  REQUIRE(term.term_state.cy == 9);
  REQUIRE(term.term_state.line_offset == 0);
  teardown();
}

TEST_CASE("CursorMove - move up scrolls at boundary") {
  setup();
  auto* cfn = make_cfn();
  // cy=4, line_offset=10 => cy not > 4, line_offset != 0 => scroll
  auto term = make_term(cfn, 25, 80, 4, 10, 10);

  term.handleUserAction(ACTION_MOVE_UP);
  REQUIRE(term.term_state.cy == 4);
  REQUIRE(term.term_state.line_offset == 9);
  teardown();
}

TEST_CASE("CursorMove - move up at top of file with cy > 0") {
  setup();
  auto* cfn = make_cfn();
  // line_offset=0, cy=3 => cy > 0 and line_offset==0 => move up
  auto term = make_term(cfn, 25, 80, 3, 10, 0);

  term.handleUserAction(ACTION_MOVE_UP);
  REQUIRE(term.term_state.cy == 2);
  REQUIRE(term.term_state.line_offset == 0);
  teardown();
}

TEST_CASE("CursorMove - move up stuck at absolute top") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 10, 0);

  term.handleUserAction(ACTION_MOVE_UP);
  REQUIRE(term.term_state.cy == 0);
  REQUIRE(term.term_state.line_offset == 0);
  teardown();
}

// ============================================================
// ACTION_MOVE_LEFT
// ============================================================

TEST_CASE("CursorMove - move left basic") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 20, 0);

  term.handleUserAction(ACTION_MOVE_LEFT);
  REQUIRE(term.term_state.cx == 19);
  teardown();
}

TEST_CASE("CursorMove - move left stops at info column boundary") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 0, 0);
  int info_col = term.term_state.info_col_size;
  term.term_state.cx = info_col;

  term.handleUserAction(ACTION_MOVE_LEFT);
  REQUIRE(term.term_state.cx == info_col);
  teardown();
}

// ============================================================
// ACTION_MOVE_RIGHT
// ============================================================

TEST_CASE("CursorMove - move right basic") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 10, 0);

  term.handleUserAction(ACTION_MOVE_RIGHT);
  REQUIRE(term.term_state.cx == 11);
  teardown();
}

TEST_CASE("CursorMove - move right stops at ncols boundary") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 79, 0);

  term.handleUserAction(ACTION_MOVE_RIGHT);
  REQUIRE(term.term_state.cx == 79);
  teardown();
}

// ============================================================
// Horizontal scroll via hrztl_line_offset
// ============================================================

TEST_CASE("CursorMove - move right at edge increments hrztl_line_offset") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 79, 0);

  REQUIRE(term.term_state.hrztl_line_offset == 0);
  term.handleUserAction(ACTION_MOVE_RIGHT);
  REQUIRE(term.term_state.hrztl_line_offset == 1);
  REQUIRE(term.term_state.cx == 79); // cursor stays at right edge
  teardown();
}

TEST_CASE("CursorMove - move right in middle does not change hrztl_line_offset") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 40, 0);

  term.handleUserAction(ACTION_MOVE_RIGHT);
  REQUIRE(term.term_state.cx == 41);
  REQUIRE(term.term_state.hrztl_line_offset == 0);
  teardown();
}

TEST_CASE("CursorMove - repeated move right at edge increments hrztl_line_offset each time") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 79, 0);

  for (int i = 0; i < 5; i++) {
    term.handleUserAction(ACTION_MOVE_RIGHT);
  }
  REQUIRE(term.term_state.hrztl_line_offset == 5);
  REQUIRE(term.term_state.cx == 79);
  teardown();
}

TEST_CASE("CursorMove - move left at info_col with hrztl_line_offset > 0 decrements it") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 0, 0);
  int info_col = term.term_state.info_col_size;
  term.term_state.cx = info_col;
  term.term_state.hrztl_line_offset = 3;

  term.handleUserAction(ACTION_MOVE_LEFT);
  REQUIRE(term.term_state.hrztl_line_offset == 2);
  REQUIRE(term.term_state.cx == info_col); // cursor stays at left edge
  teardown();
}

TEST_CASE("CursorMove - move left at info_col with hrztl_line_offset == 0 stays at 0") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 0, 0);
  int info_col = term.term_state.info_col_size;
  term.term_state.cx = info_col;
  term.term_state.hrztl_line_offset = 0;

  term.handleUserAction(ACTION_MOVE_LEFT);
  REQUIRE(term.term_state.hrztl_line_offset == 0);
  REQUIRE(term.term_state.cx == info_col);
  teardown();
}

TEST_CASE("CursorMove - move left in middle does not change hrztl_line_offset") {
  setup();
  auto* cfn = make_cfn();
  auto term = make_term(cfn, 25, 80, 0, 40, 0);
  term.term_state.hrztl_line_offset = 5;

  term.handleUserAction(ACTION_MOVE_LEFT);
  REQUIRE(term.term_state.cx == 39);
  REQUIRE(term.term_state.hrztl_line_offset == 5); // unchanged
  teardown();
}
