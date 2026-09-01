# Depends on core/, cli/ (for Config), and monitor/ (for SystemSnapshot) --
# never on a concrete platform/<os> backend.
add_library(srm_presentation STATIC
    renderer.cpp
)
target_compile_features(srm_presentation PUBLIC cxx_std_20)
target_include_directories(srm_presentation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(srm_presentation PUBLIC srm_core srm_cli srm_monitor)
