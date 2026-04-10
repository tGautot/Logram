add_library(logram)

target_compile_options(logram PRIVATE "${CMAKE_CXX_FLAGS_WARNINGS}")

add_subdirectory(common)
add_subdirectory(line_parsing)
add_subdirectory(file_parsing)
add_subdirectory(config)

target_link_libraries(logram
    PUBLIC
        line_parsing
        file_parsing
        config_handler
)


add_subdirectory(frontends)