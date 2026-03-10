# CMake generated Testfile for 
# Source directory: /workspaces/backend
# Build directory: /workspaces/backend/build_ninja
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(unit_tests "/workspaces/backend/build_ninja/test_target")
set_tests_properties(unit_tests PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/backend/CMakeLists.txt;187;add_test;/workspaces/backend/CMakeLists.txt;0;")
subdirs("extern/Catch2")
