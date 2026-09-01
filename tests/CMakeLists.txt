# All test cases across the suite link into a single srm_tests binary that
# self-registers TEST_CASE blocks and runs them from one main(). CTest just
# invokes that one binary; new test files only need to be added to SOURCES
# below.
add_executable(srm_tests
    support/runner_main.cpp
    placeholder_test.cpp
)
target_compile_features(srm_tests PRIVATE cxx_std_20)
target_include_directories(srm_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME srm_tests COMMAND srm_tests)
