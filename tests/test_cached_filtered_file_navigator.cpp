#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "test_helpers.hpp"

#include <cstdlib>
#include <ctime>

TEST_CASE("CachedFilteredFileNavigator - Simple test") {
  setup();
  std::unique_ptr<LineFormat> lf = getDefaultLineFormat();
  std::string filename = TEST_FOLDER "data/sample.log";
  std::string base_val = "INFO";
  std::shared_ptr<LineFilter> filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &base_val);
  CachedFilteredFileNavigator cfn(filename, std::move(lf), filter, 4);

  std::srand(std::time({}));
  for (int i = 0; i < 100; i++) {
    int lineid = std::rand() % 10;
    std::string_view line = cfn.getLine(lineid).line->raw_line;
    LOG(1, "lineid=%d\n", lineid);
    REQUIRE_THAT(SV_TO_STR(line), Catch::Matchers::Equals(SV_TO_STR(info_and_bf_lines[lineid])));
  }

  teardown();
}

// ============================================================
// reset_and_refill_block coverage (exercised via setFilter,
// setBadFormatAccepted, setLineFormat)
//
// The bug fixed in bda0a4938c left block.first_line_local_id at 0 even
// when more than block_size valid lines were read before the anchor, and
// also failed to reset block.contains_last_line. Tests below probe both.
// ============================================================

TEST_CASE("CachedFilteredFileNavigator::setFilter - block positions around anchor far in file") {
  setup();
  // Unfiltered CFN, small block_size so we will read past block_size lines
  // before reaching the anchor after the filter is applied.
  auto* cfn = make_cfn(5);

  auto lf = getDefaultLineFormat();
  std::string val = "INFO";
  auto filter = std::make_shared<FieldFilter>(lf.get(), "Level", FilterComparison::EQUAL, &val);

  // Anchor global=50 is past INFO #8 (global 49) and before INFO #9 (global 57).
  // With accept_bad_format=true (default), there are 14 filtered lines total;
  // the block of size 5 must slide forward as we read them.
  cfn->setFilter(filter, 50);

  // Pre-fix: first_line_local_id stuck at 0. Post-fix: it advances as the
  // cyclic deque slides.
  REQUIRE(cfn->block.first_line_local_id > 0);
  REQUIRE(cfn->block.lines.size() == 5);
  // We read until EOF while trying to reach `around_global_line` past the
  // last valid line, so EOF should be marked.
  REQUIRE(cfn->block.contains_last_line);

  // getLine(9) is the last filtered line; global line 57.
  line_info_t last = cfn->getLine(FILTERED_LINES - 1);
  REQUIRE(last.line != nullptr);
  REQUIRE(last.line->line_num == 57);
  REQUIRE((last.flags & INFO_EOF) != 0);

  // getLine at first_line_local_id must return the front of the block.
  line_t flli = cfn->block.first_line_local_id;
  line_info_t first_in_block = cfn->getLine(flli);
  REQUIRE(first_in_block.line != nullptr);
  REQUIRE(first_in_block.line->line_num == cfn->block.lines.front().line_num);

  teardown();
}

TEST_CASE("CachedFilteredFileNavigator::setFilter - clears stale contains_last_line") {
  setup();
  // Construct with INFO filter and block_size 20: only 14 filtered lines so
  // the constructor reaches EOF and sets contains_last_line = true.
  auto* cfn = make_info_filtered_cfn(20);
  REQUIRE(cfn->block.contains_last_line);

  // Swap to a pass-all filter with an early anchor. The new block cannot
  // reach EOF (62 raw lines > block_size=20), so contains_last_line must
  // be cleared. Pre-fix it was not reset and stayed true.
  cfn->setFilter(nullptr, 10);

  REQUIRE_FALSE(cfn->block.contains_last_line);
  REQUIRE(cfn->block.lines.size() == 20);

  teardown();
}

TEST_CASE("CachedFilteredFileNavigator::setBadFormatAccepted - resets block around anchor") {
  setup();
  // INFO filter, block_size=3, accept_bad_format default (true) → 14 filtered.
  auto* cfn = make_info_filtered_cfn(3);

  // Turn accept_bad_format off → only the 10 INFO lines remain. Anchor
  // global=50 is just past INFO #8 (global 49); block_size=3 must slide.
  cfn->setBadFormatAccepted(false, 50);

  REQUIRE(cfn->block.lines.size() == 3);
  // We read through 10 filtered lines; first_line_local_id must advance.
  REQUIRE(cfn->block.first_line_local_id > 0);
  // Reached EOF while chasing the anchor past the last line.
  REQUIRE(cfn->block.contains_last_line);

  // Last INFO line is global 57; local id 9 of the new (filtered) view.
  line_info_t last = cfn->getLine(9);
  REQUIRE(last.line != nullptr);
  REQUIRE(last.line->line_num == 57);

  teardown();
}

TEST_CASE("CachedFilteredFileNavigator::setLineFormat - resets block around anchor") {
  setup();
  auto* cfn = make_info_filtered_cfn(3);

  // Apply a new LineFormat (effectively the same parse). Anchor past the last
  // filtered line forces a full read through EOF.
  cfn->setLineFormat(getDefaultLineFormat(), 100);

  REQUIRE(cfn->block.lines.size() == 3);
  REQUIRE(cfn->block.first_line_local_id > 0);
  REQUIRE(cfn->block.contains_last_line);

  teardown();
}
