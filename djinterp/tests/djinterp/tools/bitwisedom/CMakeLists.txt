#*******************************************************************************
# bitwisedom [tests]                                              CMakeLists.txt
#
# The tests: the model at every level, the headers it writes compiled at
# every level, and the window driven headless.
#
#     model_c++NN        the model's suite, built at C++17, 20 and 23 -- each
#                        level this compiler can select; 32 sections
#     header_*           each example's header and self-test, written by
#                        bitwisedom-gen and compiled at every C and C++ level
#                        the compiler can select: the self-test does not
#                        compile unless every macro has the sheet's value;
#                        21 examples, 16 tests each under GCC and Clang, 10
#                        under cl. A sheet wider than an integer -- the PCI
#                        and CPUID examples, and tests/sheets/wide_mix -- has
#                        its self-test run as well, which checks its arrays
#                        and the macros that read and write them; and again
#                        with arrays of 8-, 16- and 64-bit words
#     bitfield_*         each large example's file (in res/), and
#                        each test sheet, read and written back as a
#                        bitfield: the same bytes
#     platform_*         the platform examples that describe the platform
#                        the build targets -- Windows, macOS, Linux or
#                        FreeBSD -- each compiled beside the platform's own
#                        headers, every value compared; and, where GCC or
#                        Clang targets x86, the CPUID example beside
#                        <cpuid.h>
#     ui_smoke           the window through ImGui's null backend: every tab,
#                        every row, every setting, operations, files and a
#                        wandering mouse; then menus, renames, resizes and
#                        code in and out, as a user would; a sheet of a
#                        thousand rows, its groups opened and closed one by
#                        one and all at once; a new sheet, children added
#                        in the tree, square cells, the menu bar and Help,
#                        and readable text; with every ID clash and every
#                        ImGui error recorded; 28 sections
#
# path:      /tests/djinterp/tools/bitwisedom/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.18
#*******************************************************************************

#-------------------------------------------------------------------------------
# the model, at every level
#-------------------------------------------------------------------------------
foreach(_level 17 20 23)
    if(NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        continue()
    endif()

    if(_level STREQUAL BITWISEDOM_CXX_STANDARD)
        set(_core bitwisedom_core)
    else()
        set(_core bitwisedom_core_${_level})
        bitwisedom_add_core(${_core} ${_level})
    endif()

    add_executable(bitwisedom_model_tests_${_level} model_tests.cpp)
    bitwisedom_output(bitwisedom_model_tests_${_level})
    target_link_libraries(bitwisedom_model_tests_${_level} PRIVATE ${_core})
    bitwisedom_apply_compiler_defaults(bitwisedom_model_tests_${_level}
                                      OWN CXX_STANDARD ${_level})

    add_test(NAME model_c++${_level}
             COMMAND bitwisedom_model_tests_${_level})
    set_tests_properties(model_c++${_level} PROPERTIES
        PASS_REGULAR_EXPRESSION "passed: 36   failed: 0")
endforeach()


#-------------------------------------------------------------------------------
# the generated headers, compiled
#-------------------------------------------------------------------------------
# each level: its generator name, its language, and its flag per compiler
# family ("" where that compiler cannot select it)
set(_levels
    "c99|c|-std=c99|"
    "c11|c|-std=c11|/std:c11"
    "c17|c|-std=c17|/std:c17"
    "c23|c|-std=c2x|"
    "cpp98|cxx|-std=c++98|"
    "cpp11|cxx|-std=c++11|"
    "cpp14|cxx|-std=c++14|/std:c++14"
    "cpp17|cxx|-std=c++17|/std:c++17"
    "cpp20|cxx|-std=c++20|/std:c++20"
    "cpp23|cxx|-std=c++2b|/std:c++latest")

# every example, as bitwisedom-gen --list-examples lists them
set(_examples
    uart_cr rv32i_itype posix_mode
    usb_request_type lm75_temperature x86_eflags x86_64_pte
    ieee754_single rgb565
    win32_hresult win32_window_style
    macos_macho_flags macos_cpu_type
    linux_ioctl linux_epoll_events linux_capabilities
    freebsd_file_flags freebsd_kevent
    pci_config_header
    win32_hresults x86_cpuid)

# the sheets wider than an integer, whose self-tests run: an example, and a
# sheet made to try the generator where it is hardest -- fields across every
# word size's boundaries, values in every encoding, a range too wide for an
# integer, literals an integer holds and one it does not
set(_array_examples pci_config_header x86_cpuid)
set(_test_sheets "${CMAKE_CURRENT_LIST_DIR}/sheets/wide_mix.bitfield.json")

# the sources with 64-bit values: C++98 has no long long, so their constants
# are longs, and a long has 32 bits on Windows and on every 32-bit target
set(_wide_examples x86_64_pte linux_capabilities wide_mix)

if(WIN32 OR CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(_long_bits 32)
else()
    set(_long_bits 64)
endif()

if(MSVC)
    set(_style msvc)
else()
    set(_style gnu)
endif()

#[[
bitwisedom_header_tests
  The header tests of one source -- an example by NAME, or a SHEET file
named NAME -- at each of LEVELS (every level when none are given), in every
form the level has. RUN runs the self-tests; WORD gives a wide sheet's
array words a width of their own, and its tests a suffix.
]]
function(bitwisedom_header_tests _name)
    cmake_parse_arguments(_arg "RUN" "SHEET;WORD" "LEVELS" ${ARGN})

    set(_source "-DEXAMPLE=${_name}")
    set(_suffix "")
    set(_options "-DRUN=${_arg_RUN}")
    set(_passes "self-test compiled")

    if(_arg_SHEET)
        set(_source "-DSHEET=${_arg_SHEET}")
    endif()

    if(_arg_WORD)
        set(_suffix "_w${_arg_WORD}")
        list(APPEND _options "-DWORD=${_arg_WORD}")
    endif()

    if(_arg_RUN)
        set(_passes "self-test compiled and ran")
    endif()

    foreach(_entry ${_levels})
        string(REPLACE "|" ";" _parts "${_entry}")
        list(GET _parts 0 _level)
        list(GET _parts 1 _language)
        list(GET _parts 2 _gnu)
        list(GET _parts 3 _msvc)

        if(_arg_LEVELS AND NOT _level IN_LIST _arg_LEVELS)
            continue()
        endif()

        if((_level STREQUAL "cpp98") AND (_long_bits EQUAL 32) AND
           ( (_name IN_LIST _wide_examples) OR (_arg_WORD EQUAL 64) ))
            continue()
        endif()

        if(_language STREQUAL "c")
            set(_compiler "${CMAKE_C_COMPILER}")
            set(_forms macros)
        else()
            set(_compiler "${CMAKE_CXX_COMPILER}")
            set(_forms macros constants)
        endif()

        if(MSVC)
            set(_flag "${_msvc}")
        else()
            set(_flag "${_gnu}")
        endif()

        if(_flag STREQUAL "")
            continue()
        endif()

        foreach(_form ${_forms})
            set(_test header_${_name}${_suffix}_${_level}_${_form})

            add_test(NAME ${_test}
                     COMMAND "${CMAKE_COMMAND}"
                             "-DGEN=$<TARGET_FILE:bitwisedom-gen>"
                             "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
                             "${_source}"
                             ${_options}
                             "-DLEVEL=${_level}"
                             "-DFORM=${_form}"
                             "-DLANGUAGE=${_language}"
                             "-DCOMPILER=${_compiler}"
                             "-DFLAG=${_flag}"
                             "-DSTYLE=${_style}"
                             "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/headers"
                             -P "${CMAKE_CURRENT_LIST_DIR}/check_header.cmake")
            set_tests_properties(${_test} PROPERTIES
                PASS_REGULAR_EXPRESSION "${_passes}")
        endforeach()
    endforeach()
endfunction()

foreach(_example ${_examples})
    if(_example IN_LIST _array_examples)
        bitwisedom_header_tests(${_example} RUN)
    else()
        bitwisedom_header_tests(${_example})
    endif()
endforeach()

foreach(_sheet ${_test_sheets})
    get_filename_component(_name "${_sheet}" NAME)
    string(REGEX REPLACE "[.].*$" "" _name "${_name}")
    bitwisedom_header_tests(${_name} SHEET "${_sheet}" RUN)
endforeach()

# a wide sheet's arrays of bytes, of 16-bit words and of 64-bit words, as C
# and as C++'s macros and constants
foreach(_word 8 16 64)
    foreach(_example ${_array_examples})
        bitwisedom_header_tests(${_example} RUN WORD ${_word}
                                LEVELS c11 cpp17)
    endforeach()

    foreach(_sheet ${_test_sheets})
        get_filename_component(_name "${_sheet}" NAME)
        string(REGEX REPLACE "[.].*$" "" _name "${_name}")
        bitwisedom_header_tests(${_name} SHEET "${_sheet}" RUN WORD ${_word}
                                LEVELS c11 cpp17)
    endforeach()
endforeach()


#-------------------------------------------------------------------------------
# the large examples, there and back
#-------------------------------------------------------------------------------
# Each file in the resource examples/large folder, and each test sheet, is a bitfield as
# bitwis(e)dom writes one, so read and written back it is the same bytes:
# nothing of it is lost on the way in, and the script that made it lays it
# out as the writer does.
file(GLOB _large_files "${BITWISEDOM_EXAMPLES_DIR}/large/*.bitfield.json")

foreach(_file ${_large_files} ${_test_sheets})
    get_filename_component(_name "${_file}" NAME)
    string(REGEX REPLACE "[.]bitfield[.]json$" "" _name "${_name}")

    add_test(NAME bitfield_${_name}
             COMMAND "${CMAKE_COMMAND}"
                     "-DGEN=$<TARGET_FILE:bitwisedom-gen>"
                     "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
                     "-DFILE=${_file}"
                     "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/bitfields"
                     -P "${CMAKE_CURRENT_LIST_DIR}/check_bitfield.cmake")
    set_tests_properties(bitfield_${_name} PROPERTIES
        PASS_REGULAR_EXPRESSION "is the bitfield it writes back")
endforeach()


#-------------------------------------------------------------------------------
# the platform examples, against the platform
#-------------------------------------------------------------------------------
# Each is checked where the build targets the platform it describes: its
# header, from bitwisedom-gen, is compiled beside the platform's own, and
# tests/platform/<example>.c compares every value at compile time.
if(WIN32)
    set(_platform_examples win32_hresult win32_window_style win32_hresults)
elseif(APPLE)
    set(_platform_examples macos_macho_flags macos_cpu_type)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_platform_examples linux_ioctl linux_epoll_events linux_capabilities
                           pci_config_header)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(_platform_examples freebsd_file_flags freebsd_kevent)
else()
    set(_platform_examples)
endif()

# the CPUID example describes the processor: <cpuid.h> is GCC's and Clang's
if( (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|x86|i[3-6]86)$") AND
    (NOT MSVC) )
    list(APPEND _platform_examples x86_cpuid)
endif()

foreach(_example ${_platform_examples})
    add_test(NAME platform_${_example}
             COMMAND "${CMAKE_COMMAND}"
                     "-DGEN=$<TARGET_FILE:bitwisedom-gen>"
                     "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
                     "-DEXAMPLE=${_example}"
                     "-DCHECK=${CMAKE_CURRENT_LIST_DIR}/platform/${_example}.c"
                     "-DCOMPILER=${CMAKE_C_COMPILER}"
                     "-DSTYLE=${_style}"
                     "-DWORK=${CMAKE_CURRENT_BINARY_DIR}/platform"
                     -P "${CMAKE_CURRENT_LIST_DIR}/check_platform.cmake")
    set_tests_properties(platform_${_example} PROPERTIES
        PASS_REGULAR_EXPRESSION "agrees with the platform's headers")
endforeach()


#-------------------------------------------------------------------------------
# the window, headless
#-------------------------------------------------------------------------------
# It compiles its own ImGui and its own copy of uxoxo's platform, with every
# ID clash recorded (IMGUI_DEBUG_HIGHLIGHT_ALL_ID_CONFLICTS): without that,
# ImGui notices a clash only under the mouse. The define never reaches the
# window itself.
if(TARGET bitwisedom)
    set(_imgui "${UXOXO_ROOT}/inc/imgui")

    add_executable(bitwisedom_ui_smoke
        ui_smoke.cpp
        ${BITWISEDOM_APP_SOURCES}
        "${UXOXO_ROOT}/src/uxoxo/platform/imgui_platform.cpp"
        "${_imgui}/imgui.cpp"
        "${_imgui}/imgui_draw.cpp"
        "${_imgui}/imgui_tables.cpp"
        "${_imgui}/imgui_widgets.cpp"
        "${_imgui}/imgui_demo.cpp"
        "${_imgui}/misc/cpp/imgui_stdlib.cpp"
        "${_imgui}/backends/imgui_impl_null.cpp")
    bitwisedom_output(bitwisedom_ui_smoke)
    target_include_directories(bitwisedom_ui_smoke PRIVATE
        "${BITWISEDOM_SOURCE_DIR}")
    target_include_directories(bitwisedom_ui_smoke SYSTEM PRIVATE
        ${BITWISEDOM_UXOXO_INCLUDES})
    target_compile_definitions(bitwisedom_ui_smoke PRIVATE
        IMGUI_DEBUG_HIGHLIGHT_ALL_ID_CONFLICTS)
    target_link_libraries(bitwisedom_ui_smoke PRIVATE bitwisedom::core)
    if(WIN32)
        target_link_libraries(bitwisedom_ui_smoke PRIVATE comdlg32)
    endif()
    bitwisedom_apply_dependency_defaults(bitwisedom_ui_smoke)

    add_test(NAME ui_smoke
             COMMAND bitwisedom_ui_smoke
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    set_tests_properties(ui_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "passed: 44   failed: 0")
endif()
