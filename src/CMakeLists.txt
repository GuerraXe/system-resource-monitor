# Milestone 0: minimal executable target to prove the build system works end
# to end. Real sources (core/, platform/, monitor/, cli/, presentation/) are
# added incrementally in later milestones.
add_executable(srm main.cpp)
target_compile_features(srm PRIVATE cxx_std_20)
