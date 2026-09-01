# Pure, OS-independent data types and logic: Result<T>, snapshot structs,
# and the math/formatting free functions. No platform/ dependency in either
# direction -- platform/ depends on core/, never the reverse.
add_library(srm_core STATIC
    math.cpp
    format.cpp
)
target_compile_features(srm_core PUBLIC cxx_std_20)
target_include_directories(srm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
