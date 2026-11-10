#*******************************************************************************
# djinterp [build]                                                CMakeLists.txt
#
# The benchmarks, built when DJINTERP_BUILD_BENCHMARKS is ON (the root
# CMakeLists.txt, section 5). They are timing programs: they print figures
# and never fail, so they are not tests and CTest does not run them.
#     tests/bench/util/ostream     lane D's output stream benchmarks, linked
#                                  as their README builds them:
#         bench_ostream    the stream's C kernel (bench.c)
#         bench_stdio      stdio, for scale (bench_stdio.c)
#         bench_template   the template engine and the environment printer
#                          (bench_template.c)
#         bench_cpp        the C++ face (bench_cpp.cpp), at C++20
# Their figures are meant at -O2: configure with -DCMAKE_BUILD_TYPE=Release.
#
# path:      /build/cmake/config/bench/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
#                                                            revised: 2026.11.09
#*******************************************************************************

set(_bench "${DJINTERP_TESTS_ROOT}/bench/util/ostream")
set(_c     "${DJINTERP_SRC_ROOT}/djinterp/c")

# the stream's units, and the environment's over them
add_library(djinterp_bench_stream STATIC
    "${_c}/util/sink_common.c"
    "${_c}/util/ostream/ostream.c"
    "${_c}/util/ostream/ostream_string.c"
    "${_c}/util/ostream/ostream_file.c"
    "${_c}/util/ostream/ostream_console.c"
    "${_c}/text/ostream_format.c"
    "${_c}/dstring.c"
    "${_c}/string_fn.c"
    "${_c}/djinterp.c"
    "${_c}/memory/dmemory.c"
    "${_c}/filesys/file_open.c"
    "${_c}/filesys/file_common.c"
)
target_include_directories(djinterp_bench_stream PUBLIC "${DJINTERP_INC_ROOT}")
target_compile_definitions(djinterp_bench_stream PUBLIC _XOPEN_SOURCE=700)
set_target_properties(djinterp_bench_stream PROPERTIES
    C_STANDARD 11  FOLDER "djinterp/bench")

add_library(djinterp_bench_env STATIC
    "${_c}/option/option_env.c"
    "${_c}/option/option_env_registry.c"
    "${_c}/option/option_set_common.c"
    "${_c}/option/option_common.c"
    "${DJINTERP_SRC_ROOT}/djinterp/env/env_printer.c"
    "${_c}/text/ostream_template.c"
)
target_link_libraries(djinterp_bench_env PUBLIC djinterp_bench_stream)
if(NOT WIN32)
    target_link_libraries(djinterp_bench_env PUBLIC m)
endif()
set_target_properties(djinterp_bench_env PROPERTIES
    C_STANDARD 11  FOLDER "djinterp/bench")

add_executable(bench_ostream "${_bench}/bench.c")
target_link_libraries(bench_ostream PRIVATE djinterp_bench_stream)

add_executable(bench_stdio "${_bench}/bench_stdio.c")
target_compile_definitions(bench_stdio PRIVATE _XOPEN_SOURCE=700)

add_executable(bench_template "${_bench}/bench_template.c")
target_link_libraries(bench_template PRIVATE djinterp_bench_env)

add_executable(bench_cpp "${_bench}/bench_cpp.cpp")
target_link_libraries(bench_cpp PRIVATE djinterp_bench_stream)
set_target_properties(bench_cpp PROPERTIES CXX_STANDARD 20)

set_target_properties(bench_ostream bench_stdio bench_template PROPERTIES
    C_STANDARD 11)
set_target_properties(bench_ostream bench_stdio bench_template bench_cpp
    PROPERTIES FOLDER "djinterp/bench")
