# =============================================================================
# testing/djinterp/cpp/container/traits/  ::  node_probes (LEAF, AGENTS-cmake R1)
#   Builds node_probes_tests once per C++ level, C++11 to C++23, from the runner
# in THIS directory plus the section TUs globbed from the shared tests dir below.
# The suite is compile time: each answer of node_probes.hpp's eight probes is a
# static_assert, so a level that builds has passed (the probes answer as they
# did before they moved, at every standard). Running a target prints the
# number of answers and the level.
#   Runnable alone: ctest -R node_probes_tests
#
#   cache:  DJINTERP_NODE_PROBES_CXX_LEVELS    C++ levels to build the suite at
# =============================================================================

set(DJINTERP_NODE_PROBES_CXX_LEVELS "11;14;17;20;23" CACHE STRING
    "C++ levels to build node_probes_tests at")

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/traits")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/node_probes_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

foreach(_level IN LISTS DJINTERP_NODE_PROBES_CXX_LEVELS)
    if (NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        message(STATUS "djinterp node_probes_tests: C++${_level} left out -- "
                       "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} "
                       "cannot select it")
        continue()
    endif()

    # cl's lowest level is C++14, so its C++11 build would be the C++14 one
    # under another name
    if ( (MSVC) AND (_level EQUAL 11) )
        message(STATUS "djinterp node_probes_tests: C++11 left out -- the "
                       "lowest level a cl-style driver selects is C++14")
        continue()
    endif()

    set(_target node_probes_tests_cxx${_level})

    djinterp_add_test_executable(${_target}
        SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/node_probes_tests_runner.cpp"
            ${_sources}
        INCLUDES
            "${DJINTERP_INC_ROOT}/djinterp/cpp/container/traits"
            "${_tests_dir}")

    # the common setup asks for C++20 as a compile feature, which would raise
    # every level to it; this target asks for its own level instead
    set_property(TARGET ${_target} PROPERTY COMPILE_FEATURES "")
    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD          ${_level}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF)
endforeach()
