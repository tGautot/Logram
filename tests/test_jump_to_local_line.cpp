#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"


// Verify local_to_global_id is strictly monotonically increasing (no duplicates)
static void check_mapping_monotonic(CachedFilteredFileNavigator* cfn) {
  for (size_t i = 1; i < cfn->local_to_global_id.size(); i++) {
    REQUIRE(cfn->local_to_global_id[i] > cfn->local_to_global_id[i - 1]);
  }
}

// ============================================================
// 1. Jump forward within known range (small hop, step-by-step path)
// ============================================================

TEST_CASE("jumpToLocalLine - forward small hop") {
  setup();
  auto* cfn = make_cfn(15);
  // Initial block holds lines 0-14

  SECTION("jump just past block end") {
    cfn->jumpToLocalLine(20);
    line_info_t info = cfn->getLine(20);
    REQUIRE(info.line != nullptr);
    REQUIRE(info.line->line_num == 20);
    // Block should now contain line 20
    REQUIRE(cfn->block.first_line_local_id <= 20);
    REQUIRE(cfn->block.first_line_local_id + cfn->block.lines.size() > 20);
  }
  SECTION("jump to last line of initial block is a no-op") {
    line_t first_before = cfn->block.first_line_local_id;
    size_t size_before = cfn->block.lines.size();
    cfn->jumpToLocalLine(14);
    REQUIRE(cfn->block.first_line_local_id == first_before);
    REQUIRE(cfn->block.lines.size() == size_before);
  }

  check_mapping_monotonic(cfn);
  teardown();
}

// ============================================================
// 2. Jump forward into uncharted territory
// ============================================================

TEST_CASE("jumpToLocalLine - forward into uncharted territory") {
  setup();
  auto* cfn = make_cfn(15);
  // Initial block 0-14, jump to line 50 — well beyond block_size distance

  cfn->jumpToLocalLine(50);

  line_info_t info = cfn->getLine(50);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.line->line_num == 50);
  REQUIRE(cfn->block.first_line_local_id <= 50);
  REQUIRE(cfn->block.first_line_local_id + cfn->block.lines.size() > 50);
  check_mapping_monotonic(cfn);
  teardown();
}

// ============================================================
// 3. Jump to LINE_T_MAX (EOF)
// ============================================================

TEST_CASE("jumpToLocalLine - jump to LINE_T_MAX reaches EOF") {
  setup();
  auto* cfn = make_cfn(15);

  cfn->jumpToLocalLine(LINE_T_MAX);

  REQUIRE(cfn->block.contains_last_line);
  // Last line of file should be in the block
  line_t last_local = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
  line_info_t info = cfn->getLine(last_local);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.flags & INFO_EOF);
  check_mapping_monotonic(cfn);
  teardown();
}

TEST_CASE("jumpToLocalLine - LINE_T_MAX block ends at correct last line") {
  setup();

  SECTION("block_size=15") {
    auto* cfn = make_cfn(15);
    cfn->jumpToLocalLine(LINE_T_MAX);
    line_t last_local = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
    REQUIRE(last_local == TOTAL_LINES - 1);
    check_mapping_monotonic(cfn);
  }
  SECTION("block_size=10") {
    auto* cfn = make_cfn(10);
    cfn->jumpToLocalLine(LINE_T_MAX);
    line_t last_local = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
    REQUIRE(last_local == TOTAL_LINES - 1);
    check_mapping_monotonic(cfn);
  }
  SECTION("block_size=7") {
    auto* cfn = make_cfn(7);
    cfn->jumpToLocalLine(LINE_T_MAX);
    line_t last_local = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
    REQUIRE(last_local == TOTAL_LINES - 1);
    check_mapping_monotonic(cfn);
  }

  teardown();
}

// ============================================================
// 4. Jump backward to a previously visited line
// ============================================================

TEST_CASE("jumpToLocalLine - backward to previously visited line") {
  setup();
  auto* cfn = make_cfn(15);

  // First explore forward to line 40
  cfn->jumpToLocalLine(40);
  REQUIRE(cfn->getLine(40).line != nullptr);

  SECTION("jump back within one block_size distance") {
    cfn->jumpToLocalLine(30);
    line_info_t info = cfn->getLine(30);
    REQUIRE(info.line != nullptr);
    REQUIRE(info.line->line_num == 30);
    REQUIRE(cfn->block.first_line_local_id <= 30);
  }
  SECTION("jump back far (more than block_size away)") {
    cfn->jumpToLocalLine(5);
    line_info_t info = cfn->getLine(5);
    REQUIRE(info.line != nullptr);
    REQUIRE(info.line->line_num == 5);
    REQUIRE(cfn->block.first_line_local_id <= 5);
    REQUIRE(cfn->block.first_line_local_id + cfn->block.lines.size() > 5);
  }

  check_mapping_monotonic(cfn);
  teardown();
}

// ============================================================
// 5. Jump to 0 from the end
// ============================================================

TEST_CASE("jumpToLocalLine - jump to 0 from end of file") {
  setup();
  auto* cfn = make_cfn(15);

  cfn->jumpToLocalLine(LINE_T_MAX);
  REQUIRE(cfn->block.contains_last_line);

  cfn->jumpToLocalLine(0);
  line_info_t info = cfn->getLine(0);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.line->line_num == 0);
  REQUIRE(info.flags & INFO_IS_FIRST_LINE);
  REQUIRE(cfn->block.first_line_local_id == 0);

  check_mapping_monotonic(cfn);
  teardown();
}

// ============================================================
// 6. Round-trips (end → beginning → end → beginning)
// ============================================================

TEST_CASE("jumpToLocalLine - round-trip end-beginning-end-beginning") {
  setup();
  auto* cfn = make_cfn(15);

  // First trip to end
  cfn->jumpToLocalLine(LINE_T_MAX);
  REQUIRE(cfn->block.contains_last_line);
  line_t last_local_1 = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
  REQUIRE(last_local_1 == TOTAL_LINES - 1);
  size_t mapping_size_after_first_end = cfn->local_to_global_id.size();

  // Back to beginning
  cfn->jumpToLocalLine(0);
  line_info_t info = cfn->getLine(0);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.flags & INFO_IS_FIRST_LINE);

  // Mapping should not have grown (all lines already explored)
  REQUIRE(cfn->local_to_global_id.size() == mapping_size_after_first_end);
  check_mapping_monotonic(cfn);

  // Second trip to end — should produce same block state as first
  cfn->jumpToLocalLine(LINE_T_MAX);
  REQUIRE(cfn->block.contains_last_line);
  line_t last_local_2 = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
  REQUIRE(last_local_2 == TOTAL_LINES - 1);

  // Mapping must not have gained spurious entries
  REQUIRE(cfn->local_to_global_id.size() == mapping_size_after_first_end);
  check_mapping_monotonic(cfn);

  // Second trip back to beginning
  cfn->jumpToLocalLine(0);
  info = cfn->getLine(0);
  REQUIRE(info.line != nullptr);
  REQUIRE(info.flags & INFO_IS_FIRST_LINE);
  REQUIRE(cfn->local_to_global_id.size() == mapping_size_after_first_end);

  teardown();
}

TEST_CASE("jumpToLocalLine - round-trip line content is stable") {
  setup();
  auto* cfn = make_cfn(10);

  // Read line 30's content before any jumps (explore forward to it)
  cfn->jumpToLocalLine(30);
  std::string line30_first(cfn->getLine(30).line->raw_line);

  // Go to end
  cfn->jumpToLocalLine(LINE_T_MAX);

  // Back to 30
  cfn->jumpToLocalLine(30);
  std::string line30_second(cfn->getLine(30).line->raw_line);

  REQUIRE(line30_first == line30_second);

  // To beginning
  cfn->jumpToLocalLine(0);

  // Back to 30 again
  cfn->jumpToLocalLine(30);
  std::string line30_third(cfn->getLine(30).line->raw_line);

  REQUIRE(line30_first == line30_third);

  teardown();
}

// ============================================================
// 7. All above with INFO filter (14 filtered lines)
// ============================================================

TEST_CASE("jumpToLocalLine - filtered: forward small hop") {
  setup();
  auto* cfn = make_info_filtered_cfn(5);
  // Initial block holds filtered lines 0-4

  cfn->jumpToLocalLine(8);
  line_info_t info = cfn->getLine(8);
  REQUIRE(info.line != nullptr);
  REQUIRE(SV_TO_STR(info.line->raw_line) == SV_TO_STR(info_and_bf_lines[8]));
  check_mapping_monotonic(cfn);
  teardown();
}

TEST_CASE("jumpToLocalLine - filtered: jump to LINE_T_MAX") {
  setup();
  auto* cfn = make_info_filtered_cfn(5);

  cfn->jumpToLocalLine(LINE_T_MAX);
  REQUIRE(cfn->block.contains_last_line);
  line_t last_local = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
  REQUIRE(last_local == FILTERED_LINES - 1);

  line_info_t info = cfn->getLine(last_local);
  REQUIRE(info.line != nullptr);
  REQUIRE(SV_TO_STR(info.line->raw_line) == SV_TO_STR(info_and_bf_lines[FILTERED_LINES - 1]));

  check_mapping_monotonic(cfn);
  teardown();
}

TEST_CASE("jumpToLocalLine - filtered: backward after reaching end") {
  setup();
  auto* cfn = make_info_filtered_cfn(5);

  cfn->jumpToLocalLine(LINE_T_MAX);
  REQUIRE(cfn->block.contains_last_line);

  cfn->jumpToLocalLine(0);
  line_info_t info = cfn->getLine(0);
  REQUIRE(info.line != nullptr);
  REQUIRE(SV_TO_STR(info.line->raw_line) == SV_TO_STR(info_and_bf_lines[0]));
  REQUIRE(info.flags & INFO_IS_FIRST_LINE);

  check_mapping_monotonic(cfn);
  teardown();
}

TEST_CASE("jumpToLocalLine - filtered: round-trip") {
  setup();
  auto* cfn = make_info_filtered_cfn(5);

  // To end
  cfn->jumpToLocalLine(LINE_T_MAX);
  line_t last_local_1 = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
  REQUIRE(last_local_1 == FILTERED_LINES - 1);
  size_t mapping_size = cfn->local_to_global_id.size();

  // To beginning
  cfn->jumpToLocalLine(0);
  REQUIRE(cfn->getLine(0).line != nullptr);

  // To end again
  cfn->jumpToLocalLine(LINE_T_MAX);
  REQUIRE(cfn->block.contains_last_line);
  line_t last_local_2 = cfn->block.first_line_local_id + cfn->block.lines.size() - 1;
  REQUIRE(last_local_2 == FILTERED_LINES - 1);

  // Mapping must not grow
  REQUIRE(cfn->local_to_global_id.size() == mapping_size);
  check_mapping_monotonic(cfn);

  // To beginning again
  cfn->jumpToLocalLine(0);
  REQUIRE(cfn->getLine(0).flags & INFO_IS_FIRST_LINE);

  teardown();
}

TEST_CASE("jumpToLocalLine - filtered: all lines accessible after full traversal") {
  setup();
  auto* cfn = make_info_filtered_cfn(5);

  // Force full exploration
  cfn->jumpToLocalLine(LINE_T_MAX);
  cfn->jumpToLocalLine(0);

  // Every filtered line should be reachable and correct
  for (int i = 0; i < FILTERED_LINES; i++) {
    line_info_t info = cfn->getLine(i);
    REQUIRE(info.line != nullptr);
    REQUIRE(SV_TO_STR(info.line->raw_line) == SV_TO_STR(info_and_bf_lines[i]));
  }

  teardown();
}
