add_library(srm_cli STATIC
    parser.cpp
)
target_compile_features(srm_cli PUBLIC cxx_std_20)
target_include_directories(srm_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(srm_cli PUBLIC srm_core)
