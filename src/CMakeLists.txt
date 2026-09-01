add_subdirectory(core)

# Real sources (platform/, monitor/, cli/, presentation/) are added
# incrementally in later milestones.
add_executable(srm main.cpp)
target_compile_features(srm PRIVATE cxx_std_20)
target_link_libraries(srm PRIVATE srm_core)
