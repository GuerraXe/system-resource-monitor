# Depends only on core/ and platform/interfaces.hpp (a header) -- never on
# a concrete platform/<os>/ backend. Concrete monitors are constructed and
# injected by main.cpp, so this library doesn't need to link srm_platform.
add_library(srm_monitor STATIC
    engine.cpp
)
target_compile_features(srm_monitor PUBLIC cxx_std_20)
target_include_directories(srm_monitor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(srm_monitor PUBLIC srm_core)
