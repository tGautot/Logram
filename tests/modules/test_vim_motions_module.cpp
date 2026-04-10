#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include "logram_terminal.hpp"
#include "terminal_modules.hpp"

TEST_CASE("VimMotionsModule - move input mappings") {
  setup();
  auto* cfn = make_cfn();
  LogramTerminal term(cfn);

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);

  bool partial = false;

  SECTION("h maps to MOVE_LEFT") {
    REQUIRE(term.matchInputSequence("h", partial) == ACTION_MOVE_LEFT);
    REQUIRE_FALSE(partial);
  }
  SECTION("j maps to MOVE_DOWN") {
    REQUIRE(term.matchInputSequence("j", partial) == ACTION_MOVE_DOWN);
    REQUIRE_FALSE(partial);
  }
  SECTION("k maps to MOVE_UP") {
    REQUIRE(term.matchInputSequence("k", partial) == ACTION_MOVE_UP);
    REQUIRE_FALSE(partial);
  }
  SECTION("l maps to MOVE_RIGHT") {
    REQUIRE(term.matchInputSequence("l", partial) == ACTION_MOVE_RIGHT);
    REQUIRE_FALSE(partial);
  }
  SECTION("g is partial match for gg") {
    user_action_t action = term.matchInputSequence("g", partial);
    REQUIRE(action == ACTION_NONE);
    REQUIRE(partial);
  }

  teardown();
}

// ============================================================
// ACTION_GO_TO_FILE_END (G)
// ============================================================

TEST_CASE("VimMotionsModule - G jumps to end of file") {
  setup();
  // block_size=15 forces multiple block traversals to reach line 61
  auto* cfn = make_cfn(15);
  LogramTerminal term(cfn);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;
  term.term_state.cy = 0;
  term.term_state.line_offset = 0;

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);

  bool partial = false;
  line_t last_local = TOTAL_LINES - 1; // 61

  SECTION("cursor and offset land on last line") {
    term.handleUserAction(term.matchInputSequence("G", partial));
    REQUIRE(term.term_state.cy + term.term_state.line_offset == last_local);
  }
  SECTION("cy is at bottom of screen when file is longer than screen") {
    term.handleUserAction(term.matchInputSequence("G", partial));
    int max_cy = term.term_state.nrows - term.term_state.num_status_line - 1;
    REQUIRE(term.term_state.cy == max_cy);
  }
  SECTION("block contains last line after jump") {
    term.handleUserAction(term.matchInputSequence("G", partial));
    REQUIRE(cfn->block.contains_last_line);
  }

  teardown();
}

TEST_CASE("VimMotionsModule - G when file fits on screen") {
  setup();
  auto* cfn = make_cfn(15);
  LogramTerminal term(cfn);
  // nrows large enough that all 62 lines fit (62 content rows + 1 status = 63)
  term.term_state.nrows = 70;
  term.term_state.ncols = 80;
  term.term_state.cy = 0;
  term.term_state.line_offset = 0;

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);

  bool partial = false;
  term.handleUserAction(term.matchInputSequence("G", partial));

  line_t last_local = TOTAL_LINES - 1; // 61
  REQUIRE(term.term_state.cy + term.term_state.line_offset == last_local);

  teardown();
}

// ============================================================
// ACTION_GO_TO_FILE_BEGINNING (gg)
// ============================================================

TEST_CASE("VimMotionsModule - gg jumps to beginning of file") {
  setup();
  auto* cfn = make_cfn(15);
  LogramTerminal term(cfn);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);

  // First go to end so we're far from the beginning
  bool partial = false;
  term.handleUserAction(term.matchInputSequence("G", partial));
  REQUIRE(term.term_state.cy > 0);

  SECTION("cy and offset reset to 0") {
    term.handleUserAction(term.matchInputSequence("gg", partial));
    REQUIRE(term.term_state.cy == 0);
    REQUIRE(term.term_state.line_offset == 0);
  }
  SECTION("block contains first line after jump") {
    term.handleUserAction(term.matchInputSequence("gg", partial));
    // Line 0 should now be reachable from the block
    line_info_t info = cfn->getLine(0);
    REQUIRE(info.line != nullptr);
    REQUIRE(info.flags & INFO_IS_FIRST_LINE);
  }

  teardown();
}

TEST_CASE("VimMotionsModule - gg from middle of file") {
  setup();
  auto* cfn = make_cfn(15);
  LogramTerminal term(cfn);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;
  term.term_state.cy = 10;
  term.term_state.line_offset = 20;

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);

  bool partial = false;
  term.handleUserAction(term.matchInputSequence("gg", partial));

  REQUIRE(term.term_state.cy == 0);
  REQUIRE(term.term_state.line_offset == 0);

  teardown();
}

TEST_CASE("VimMotionsModule - gg from last line") {
  setup();
  auto* cfn = make_cfn(15);
  LogramTerminal term(cfn);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;
  term.term_state.cy = 0;
  term.term_state.line_offset = 0;

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);

  bool partial = false;
  // Go to end first
  term.handleUserAction(term.matchInputSequence("G", partial));
  line_t last_local = TOTAL_LINES - 1;
  REQUIRE(term.term_state.cy + term.term_state.line_offset == last_local);

  // Now gg back to beginning
  term.handleUserAction(term.matchInputSequence("gg", partial));
  REQUIRE(term.term_state.cy == 0);
  REQUIRE(term.term_state.line_offset == 0);

  // First line should be accessible
  line_info_t info = cfn->getLine(0);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.flags & INFO_IS_FIRST_LINE);

  teardown();
}

TEST_CASE("VimMotionsModule - gg from first line is a no-op") {
  setup();
  auto* cfn = make_cfn(15);
  LogramTerminal term(cfn);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;
  term.term_state.cy = 0;
  term.term_state.line_offset = 0;

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);

  bool partial = false;
  term.handleUserAction(term.matchInputSequence("gg", partial));

  REQUIRE(term.term_state.cy == 0);
  REQUIRE(term.term_state.line_offset == 0);

  teardown();
}

TEST_CASE("VimMotionsModule - G then gg then G then gg round-trip") {
  setup();
  auto* cfn = make_cfn(15);
  LogramTerminal term(cfn);
  term.term_state.nrows = 25;
  term.term_state.ncols = 80;
  term.term_state.cy = 0;
  term.term_state.line_offset = 0;

  VimMotionsModule mod;
  mod.registerUserInputMapping(term);
  mod.registerUserActionCallback(term);

  bool partial = false;
  line_t last_local = TOTAL_LINES - 1;

  // First G
  term.handleUserAction(term.matchInputSequence("G", partial));
  REQUIRE(term.term_state.cy + term.term_state.line_offset == last_local);
  REQUIRE(cfn->block.contains_last_line);

  // First gg
  term.handleUserAction(term.matchInputSequence("gg", partial));
  REQUIRE(term.term_state.cy == 0);
  REQUIRE(term.term_state.line_offset == 0);
  line_info_t info = cfn->getLine(0);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.flags & INFO_IS_FIRST_LINE);

  // Second G — should reach the same state as the first
  term.handleUserAction(term.matchInputSequence("G", partial));
  REQUIRE(term.term_state.cy + term.term_state.line_offset == last_local);
  REQUIRE(cfn->block.contains_last_line);

  // Second gg
  term.handleUserAction(term.matchInputSequence("gg", partial));
  REQUIRE(term.term_state.cy == 0);
  REQUIRE(term.term_state.line_offset == 0);
  info = cfn->getLine(0);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.flags & INFO_IS_FIRST_LINE);

  teardown();
}
