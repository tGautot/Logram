#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include "log_parser_terminal.hpp"
#include "terminal_modules.hpp"

TEST_CASE("WasdModule - input mappings") {
  setup();
  auto* cfn = make_cfn();
  LogParserTerminal term(cfn);

  WasdModule mod;
  mod.registerUserInputMapping(term);

  bool partial = false;

  SECTION("w maps to MOVE_UP") {
    REQUIRE(term.matchInputSequence("w", partial) == ACTION_MOVE_UP);
    REQUIRE_FALSE(partial);
  }
  SECTION("s maps to MOVE_DOWN") {
    REQUIRE(term.matchInputSequence("s", partial) == ACTION_MOVE_DOWN);
    REQUIRE_FALSE(partial);
  }
  SECTION("d maps to MOVE_RIGHT") {
    REQUIRE(term.matchInputSequence("d", partial) == ACTION_MOVE_RIGHT);
    REQUIRE_FALSE(partial);
  }
  SECTION("a maps to MOVE_LEFT") {
    REQUIRE(term.matchInputSequence("a", partial) == ACTION_MOVE_LEFT);
    REQUIRE_FALSE(partial);
  }
  SECTION("unregistered key returns ACTION_NONE") {
    REQUIRE(term.matchInputSequence("x", partial) == ACTION_NONE);
  }

  teardown();
}
