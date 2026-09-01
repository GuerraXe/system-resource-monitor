add_library(srm_platform STATIC)
target_compile_features(srm_platform PUBLIC cxx_std_20)
target_include_directories(srm_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(srm_platform PUBLIC srm_core)

if (WIN32)
    target_sources(srm_platform PRIVATE
        windows/win_string.cpp
        windows/memory_monitor.cpp
        windows/disk_monitor.cpp
        windows/cpu_monitor.cpp
    )
else()
    # No sources added: fail the configure step loudly instead of silently
    # producing a binary with no working monitor backend. Adding support
    # means implementing platform/interfaces.hpp under a new platform/<os>/
    # directory -- see ARCHITECTURE.md for what that involves per metric.
    message(FATAL_ERROR
        "system-resource-monitor: no platform backend implemented for this OS. "
        "See ARCHITECTURE.md for the platform/<os>/ extension point.")
endif()
