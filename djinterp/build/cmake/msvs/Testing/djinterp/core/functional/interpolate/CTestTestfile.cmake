# CMake generated Testfile for 
# Source directory: D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate
# Build directory: D:/sam/code/projects/frameworks/djinterp/release/build/cmake/msvs/testing/djinterp/core/functional/interpolate
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[interpolate_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Debug/testing/djinterp/core/functional/interpolate/interpolate_tests.exe")
  set_tests_properties([=[interpolate_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Debug/testing/djinterp/core/functional/interpolate" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;15;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[interpolate_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Release/testing/djinterp/core/functional/interpolate/interpolate_tests.exe")
  set_tests_properties([=[interpolate_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-Release/testing/djinterp/core/functional/interpolate" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;15;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[interpolate_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-MinSizeRel/testing/djinterp/core/functional/interpolate/interpolate_tests.exe")
  set_tests_properties([=[interpolate_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-MinSizeRel/testing/djinterp/core/functional/interpolate" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;15;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[interpolate_tests]=] "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-RelWithDebInfo/testing/djinterp/core/functional/interpolate/interpolate_tests.exe")
  set_tests_properties([=[interpolate_tests]=] PROPERTIES  WORKING_DIRECTORY "D:/sam/code/projects/frameworks/djinterp/release/bin/x64-RelWithDebInfo/testing/djinterp/core/functional/interpolate" _BACKTRACE_TRIPLES "D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/djinterp_module.cmake;267;add_test;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;15;djinterp_add_test_executable;D:/sam/code/projects/frameworks/djinterp/release/build/cmake/config/testing/djinterp/core/functional/interpolate/CMakeLists.txt;0;")
else()
  add_test([=[interpolate_tests]=] NOT_AVAILABLE)
endif()
