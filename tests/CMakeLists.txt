# All test cases across the suite link into a single srm_tests binary that
# self-registers TEST_CASE blocks and runs them from one main(). CTest just
# invokes that one binary; new test files only need to be added to SOURCES
# below.
add_executable(srm_tests
    support/runner_main.cpp
    placeholder_test.cpp
    core/math_tests.cpp
    core/format_tests.cpp
    core/result_tests.cpp
    platform/windows/memory_monitor_translate_tests.cpp
    platform/windows/memory_monitor_integration_tests.cpp
    platform/windows/disk_monitor_translate_tests.cpp
    platform/windows/disk_monitor_integration_tests.cpp
    platform/windows/cpu_monitor_translate_tests.cpp
    platform/windows/cpu_monitor_integration_tests.cpp
    platform/windows/process_monitor_translate_tests.cpp
    platform/windows/process_monitor_integration_tests.cpp
    platform/windows/network_monitor_translate_tests.cpp
    platform/windows/network_monitor_integration_tests.cpp
)
target_compile_features(srm_tests PRIVATE cxx_std_20)
target_include_directories(srm_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(srm_tests PRIVATE srm_core srm_platform)

add_test(NAME srm_tests COMMAND srm_tests)
