# CMake generated Testfile for 
# Source directory: D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern
# Build directory: D:/sam/code/projects/frameworks/djinterp/release/build/cmake/msvs/testing/djinterp/core/paradigm/pattern/pattern
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[pattern_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Debug/testing/djinterp/core/paradigm/pattern/pattern/pattern_tests.exe")
  set_tests_properties([=[pattern_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Debug/testing/djinterp/core/paradigm/pattern/pattern" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;27;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[pattern_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Release/testing/djinterp/core/paradigm/pattern/pattern/pattern_tests.exe")
  set_tests_properties([=[pattern_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Release/testing/djinterp/core/paradigm/pattern/pattern" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;27;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[pattern_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-MinSizeRel/testing/djinterp/core/paradigm/pattern/pattern/pattern_tests.exe")
  set_tests_properties([=[pattern_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-MinSizeRel/testing/djinterp/core/paradigm/pattern/pattern" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;27;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[pattern_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-RelWithDebInfo/testing/djinterp/core/paradigm/pattern/pattern/pattern_tests.exe")
  set_tests_properties([=[pattern_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-RelWithDebInfo/testing/djinterp/core/paradigm/pattern/pattern" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;27;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern/CMakeLists.txt;0;")
else()
  add_test([=[pattern_tests]=] NOT_AVAILABLE)
endif()
