#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include "logram_terminal.hpp"
#include "terminal_modules.hpp"

TEST_CASE("ArrowsModule - input mappings") {
  setup();
  auto* cfn = make_cfn();
  LogramTerminal term(cfn);

  ArrowsModule mod;
  mod.registerUserInputMapping(term);

  bool partial = false;

  SECTION("up arrow maps to MOVE_UP") {
    REQUIRE(term.matchInputSequence("\x1b[A", partial) == ACTION_MOVE_UP);
    REQUIRE_FALSE(partial);
  }
  SECTION("down arrow maps to MOVE_DOWN") {
    REQUIRE(term.matchInputSequence("\x1b[B", partial) == ACTION_MOVE_DOWN);
    REQUIRE_FALSE(partial);
  }
  SECTION("right arrow maps to MOVE_RIGHT") {
    REQUIRE(term.matchInputSequence("\x1b[C", partial) == ACTION_MOVE_RIGHT);
    REQUIRE_FALSE(partial);
  }
  SECTION("left arrow maps to MOVE_LEFT") {
    REQUIRE(term.matchInputSequence("\x1b[D", partial) == ACTION_MOVE_LEFT);
    REQUIRE_FALSE(partial);
  }
  SECTION("partial escape sequence is flagged") {
    user_action_t action = term.matchInputSequence("\x1b[", partial);
    REQUIRE(action == ACTION_NONE);
    REQUIRE(partial);
  }
  SECTION("unrelated key returns ACTION_NONE") {
    REQUIRE(term.matchInputSequence("w", partial) == ACTION_NONE);
    REQUIRE_FALSE(partial);
  }

  teardown();
}
