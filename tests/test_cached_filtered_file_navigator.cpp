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
