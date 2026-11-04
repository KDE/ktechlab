set(SRC_DIR ${PROJECT_SOURCE_DIR}/src/)

include_directories(
    ${SRC_DIR}  # needed for subdirs
    ${SRC_DIR}/core
    ${SRC_DIR}/electronics
    ${SRC_DIR}/electronics/simulation
    ${CMAKE_BINARY_DIR}/src/core  # for the kcfg file
)
enable_testing(true)

add_compile_options("--coverage")
add_link_options("--coverage")

add_executable(test_oscilloscope test_oscilloscope.cpp)

target_link_libraries(test_oscilloscope
    PRIVATE
    test_ktechlab
    Qt6::Test
)

# Needed for CI to find and run the test
add_test(NAME test_oscilloscope COMMAND test_oscilloscope)

