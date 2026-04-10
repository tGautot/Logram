Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(run_tests
  test_main.cpp
  test_line_format.cpp
  test_line_parser.cpp
  test_line_filter.cpp
  test_filtered_file_reader.cpp
  test_cached_filtered_file_navigator.cpp
  test_cyclic_deque.cpp
  test_filters.cpp
  test_filter_roundtrip.cpp
  test_logram_terminal.cpp
  test_jump_to_local_line.cpp
  test_find_next_occurence.cpp
  modules/test_cursor_move_module.cpp
  modules/test_wasd_module.cpp
  modules/test_arrows_module.cpp
  modules/test_vim_motions_module.cpp
  modules/test_text_search_module.cpp
  modules/test_filter_management_module.cpp)

target_include_directories(run_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(run_tests
  PRIVATE
    logging
    Catch2::Catch2
    line_parsing
    file_parsing
    logramtui)
