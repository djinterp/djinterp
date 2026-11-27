# =============================================================================
# djinterp_module.cmake
#
#   Shared CMake functionality for djinterp test/exe modules.
#
#   PUBLIC ENTRY POINTS
#   -------------------
#   djinterp_add_test_executable(<target>
#       SOURCES   <files>...
#       [DEFINES  <macros>...]
#       [INCLUDES <dirs>...]
#       [LIBRARIES <libs>...])
#       Creates a test exe.  Auto-links the C runtime, defines D_TESTING=1,
#       sets the per-config output directory to
#         <bin>/<arch>-<Config>/<rel>/
#       and registers itself with CTest under its target name.
#
#   djinterp_add_executable(<target> ...)
#       Same shape as the test variant, without D_TESTING and without CTest
#       registration.  For non-test exes.
#
#   djinterp_add_test_subtree(<aggregate_name>)
#       Creates an aggregate target whose dependencies are every test exe
#       defined under THIS directory and any descendant directory.  Works at
#       any depth; nest aggregates freely.  Auto-recurses into immediate
#       subdirectories that contain a CMakeLists.txt - so a single call in a
#       parent directory wires up the entire subtree below it.
#
#   djinterp_compute_output_dir(<out_var> <config>)
#       Lower-level helper: returns the bin path for the current source dir
#       under the given config.
#
#
#   DIRECTORY ROLES
#   ---------------
#   Two roles, one helper call each.  No directory needs both, no directory
#   needs neither (if it contains nothing actionable, just don't give it a
#   CMakeLists).
#
#       LEAF:      contains a main.cpp + CMakeLists.txt that calls
#                  djinterp_add_test_executable().  Builds an exe.
#
#       AGGREGATE: contains a CMakeLists.txt that calls
#                  djinterp_add_test_subtree().  Recurses into children
#                  and produces a meta-target that builds them all.
#
# =============================================================================

include_guard(GLOBAL)


# -----------------------------------------------------------------------------
# djinterp_compute_output_dir
#   Per-config output directory for the target currently being defined.
# Format: <bin>/<arch>-<Config>/<rel>, where <rel> is the path from
# DJINTERP_CONFIG_ROOT to the calling source directory.  This mirrors the
# inc/src/tests layout exactly.
# -----------------------------------------------------------------------------
function(djinterp_compute_output_dir _out_var _config)

    if (DEFINED CMAKE_VS_PLATFORM_NAME)
        set(_arch "${CMAKE_VS_PLATFORM_NAME}")
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_arch "x64")
    else()
        set(_arch "Win32")
    endif()

    file(RELATIVE_PATH _rel
         "${DJINTERP_CONFIG_ROOT}"
         "${CMAKE_CURRENT_SOURCE_DIR}")

    set(${_out_var}
        "${DJINTERP_BIN_ROOT}/${_arch}-${_config}/${_rel}"
        PARENT_SCOPE)

endfunction()


# -----------------------------------------------------------------------------
# _djinterp_apply_common
#   INTERNAL.  Project-wide compile/link configuration for any executable
# created via the helpers.  The contents of the former djinterp.props live
# here.
# -----------------------------------------------------------------------------
function(_djinterp_apply_common _target)

    # ---- includes ---------------------------------------------------------
    target_include_directories(${_target} PRIVATE
        "${DJINTERP_INC_ROOT}"
    )

    # imgui include is conditional - matches djinterp.props but doesn't
    # hard-fail on toolchains where imgui is not vendored
    if (IS_DIRECTORY "${DJINTERP_INC_ROOT}/imgui")
        target_include_directories(${_target} PRIVATE
            "${DJINTERP_INC_ROOT}/imgui"
        )
        target_compile_definitions(${_target} PRIVATE
            IMGUI_DEFINE_MATH_OPERATORS
        )
    endif()

    # ---- standards --------------------------------------------------------
    target_compile_features(${_target} PRIVATE
        cxx_std_20
        c_std_17
    )

    set_target_properties(${_target} PROPERTIES
        CXX_EXTENSIONS OFF
        C_EXTENSIONS   OFF
    )

    # ---- shared C runtime -------------------------------------------------
    target_link_libraries(${_target} PRIVATE
        djinterp_c_runtime
    )

    # ---- platform / compiler flags ---------------------------------------
    if (MSVC)
        target_compile_definitions(${_target} PRIVATE
            _UNICODE
            UNICODE
        )

        # /Zc:__cplusplus: without it cl gives __cplusplus as 199711 at
        # every level, misleading any code that reads it
        target_compile_options(${_target} PRIVATE
            /W3
            /sdl
            /permissive-
            /Zc:preprocessor
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
        )

        # Release-only optimizations (mirror WholeProgramOptimization +
        # FunctionLevelLinking + IntrinsicFunctions in the original vcxproj).
        target_compile_options(${_target} PRIVATE
            $<$<CONFIG:Release>:/Gy>
            $<$<CONFIG:Release>:/Oi>
            $<$<CONFIG:Release>:/GL>
        )
        target_link_options(${_target} PRIVATE
            $<$<CONFIG:Release>:/OPT:REF>
            $<$<CONFIG:Release>:/OPT:ICF>
            $<$<CONFIG:Release>:/LTCG>
        )

        set_target_properties(${_target} PROPERTIES
            VS_GLOBAL_GenerateDebugInformation "true"
            LINK_FLAGS                         "/SUBSYSTEM:CONSOLE /DEBUG"
        )
    endif()

    # ---- per-config output dirs -------------------------------------------
    if (CMAKE_CONFIGURATION_TYPES)
        set(_configs ${CMAKE_CONFIGURATION_TYPES})
    elseif (CMAKE_BUILD_TYPE)
        set(_configs "${CMAKE_BUILD_TYPE}")
    else()
        set(_configs Debug Release)
    endif()

    foreach(_cfg ${_configs})
        string(TOUPPER "${_cfg}" _CFG)
        djinterp_compute_output_dir(_out "${_cfg}")
        set_target_properties(${_target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${_out}"
            LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${_out}"
            ARCHIVE_OUTPUT_DIRECTORY_${_CFG} "${_out}"
            PDB_OUTPUT_DIRECTORY_${_CFG}     "${_out}"
        )
    endforeach()

    # ---- VS solution explorer folder --------------------------------------
    file(RELATIVE_PATH _rel
         "${DJINTERP_CONFIG_ROOT}"
         "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(_folder "${_rel}" DIRECTORY)
    if (_folder)
        set_target_properties(${_target} PROPERTIES FOLDER "${_folder}")
    endif()

endfunction()


# -----------------------------------------------------------------------------
# djinterp_add_executable
#   Generic executable wrapper.  Use this for non-test exes.
# -----------------------------------------------------------------------------
function(djinterp_add_executable _target)

    cmake_parse_arguments(_arg
        ""
        ""
        "SOURCES;DEFINES;LIBRARIES;INCLUDES"
        ${ARGN})

    if (NOT _arg_SOURCES)
        message(FATAL_ERROR
                "djinterp_add_executable(${_target}): SOURCES is required")
    endif()

    add_executable(${_target} ${_arg_SOURCES})

    _djinterp_apply_common(${_target})

    if (_arg_DEFINES)
        target_compile_definitions(${_target} PRIVATE ${_arg_DEFINES})
    endif()

    if (_arg_INCLUDES)
        target_include_directories(${_target} PRIVATE ${_arg_INCLUDES})
    endif()

    if (_arg_LIBRARIES)
        target_link_libraries(${_target} PRIVATE ${_arg_LIBRARIES})
    endif()

endfunction()


# -----------------------------------------------------------------------------
# djinterp_add_test_executable
#   Test-flavored wrapper.  Same shape as djinterp_add_executable plus:
#       - automatically defines D_TESTING=1
#       - registers the target as a CTest test
#       - records the target in a global registry so aggregates can find it
# -----------------------------------------------------------------------------
function(djinterp_add_test_executable _target)

    cmake_parse_arguments(_arg
        ""
        ""
        "SOURCES;DEFINES;LIBRARIES;INCLUDES"
        ${ARGN})

    # a name two leaves give: the configure step names both, rather than
    # CMake's "target already exists", which names neither
    get_property(_first GLOBAL PROPERTY "DJINTERP_TEST_TARGET_DIR_${_target}")
    if (_first)
        message(FATAL_ERROR
            "djinterp: the test target '${_target}' is made by two leaves:\n"
            "  ${_first}\n  ${CMAKE_CURRENT_SOURCE_DIR}\n"
            "Give one of them a name of its own.")
    endif()
    set_property(GLOBAL PROPERTY "DJINTERP_TEST_TARGET_DIR_${_target}"
                 "${CMAKE_CURRENT_SOURCE_DIR}")

    #   D_TESTING marks the test build.  D_TEST_OUTPUT_DIR hands the target's
    # own output directory to the sources as a string literal, so a runner that
    # wants an ABSOLUTE report path can set
    #       opts.set<test_option::output_file>(D_TEST_OUTPUT_DIR "/suite.pdf");
    # and be independent of the working directory entirely.  Runners that leave
    # output_file empty need nothing: the working-directory property below
    # already lands their reports in the same place.
    djinterp_add_executable(${_target}
        SOURCES   ${_arg_SOURCES}
        DEFINES   D_TESTING=1
                  "D_TEST_OUTPUT_DIR=\"$<TARGET_FILE_DIR:${_target}>\""
                  ${_arg_DEFINES}
        INCLUDES  ${_arg_INCLUDES}
        LIBRARIES ${_arg_LIBRARIES})

    # ---- runtime working directory -----------------------------------------
    #   The suites emit their reports through test_option::output_file, which
    # defaults to empty - so an unqualified report name is written relative to
    # the PROCESS's working directory.  Left unset, that is wherever the runner
    # happened to be launched from (the build dir under ctest, the project dir
    # under F5), and reports scatter accordingly.
    #   Pinning it to the target's own output directory puts every report beside
    # the exe that produced it, under bin/<arch>-<Config>/<rel>/.  Set for both
    # launch paths: WORKING_DIRECTORY for ctest, VS_DEBUGGER_WORKING_DIRECTORY
    # for F5 / Debug in the IDE.  $<TARGET_FILE_DIR:...> resolves per-config, so
    # Debug and Release reports never land in the same folder.
    set_target_properties(${_target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${_target}>")

    # ---- no dialogs under MSVC ---------------------------------------------
    #   A test that fails in the C runtime (an assertion, an invalid
    # parameter, a heap check, abort, a crash) writes to stderr and ends,
    # rather than showing a dialog that holds the run until CTest stops it.
    if (MSVC)
        target_sources(${_target} PRIVATE
            "${DJINTERP_TESTS_ROOT}/djinterp/msvc_no_dialog.cpp")
    endif()

    # CTest registration: enables `ctest -R <target>` from the build dir
    if (BUILD_TESTING)
        add_test(NAME              ${_target}
                 COMMAND           ${_target}
                 WORKING_DIRECTORY "$<TARGET_FILE_DIR:${_target}>")
    endif()

    # The module's CTest label and its target djinterp_tests_<module>
    # (ruled 2026.11.17): `ctest -L '^container$'` runs a module's suites,
    # and djinterp_add_module_test_targets makes the target that builds them.
    djinterp_test_module(_module "${CMAKE_CURRENT_SOURCE_DIR}")
    if (_module)
        if (BUILD_TESTING)
            set_tests_properties(${_target} PROPERTIES LABELS "${_module}")
        endif()
        set_property(GLOBAL APPEND PROPERTY DJINTERP_TEST_MODULES ${_module})
        set_property(GLOBAL APPEND PROPERTY
                     "DJINTERP_TEST_MODULE_${_module}" ${_target})
    endif()

    # Register in the global aggregate registry.  Stored as parallel lists
    # of (target_name, source_dir) so djinterp_add_test_subtree can pick out
    # everything below a given directory.
    set_property(GLOBAL APPEND PROPERTY
                 DJINTERP_TEST_TARGETS      ${_target})
    set_property(GLOBAL APPEND PROPERTY
                 DJINTERP_TEST_TARGET_DIRS  "${CMAKE_CURRENT_SOURCE_DIR}")

endfunction()


# -----------------------------------------------------------------------------
# djinterp_add_test_subtree
#   Aggregate target.  Auto-recurses into every immediate subdirectory that
# contains a CMakeLists.txt; after recursion completes, creates a custom
# target named <aggregate_name> that depends on every test exe registered
# from this directory or any descendant.
#
#   add_subdirectory() is synchronous, so by the time this function returns
# from its recursion, every leaf has already registered itself in the global
# registry.  We then filter by source-dir prefix.
#
#   This generalizes seamlessly:
#     testing/djinterp/CMakeLists.txt              -> djinterp_add_test_subtree(all_djinterp)
#     testing/djinterp/core/CMakeLists.txt         -> djinterp_add_test_subtree(all_djinterp_core)
#     testing/djinterp/core/container/CMakeLists.txt
#                                                  -> djinterp_add_test_subtree(all_containers)
#     testing/djinterp/core/container/tree/CMakeLists.txt
#                                                  -> djinterp_add_test_subtree(all_trees)
#
#   Each aggregate sees ALL the leaves under it (transitively), not just
# its immediate children.  Aggregates are independent of one another; the
# same leaf can be a member of multiple aggregates at different depths.
# -----------------------------------------------------------------------------
function(djinterp_add_test_subtree _aggregate_name)

    # ---- recurse into every immediate child dir with a CMakeLists --------
    file(GLOB _entries
         CONFIGURE_DEPENDS
         RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}/*")

    foreach(_entry ${_entries})
        set(_full "${CMAKE_CURRENT_SOURCE_DIR}/${_entry}")
        if (IS_DIRECTORY "${_full}" AND
            EXISTS "${_full}/CMakeLists.txt")
            add_subdirectory("${_entry}")
        endif()
    endforeach()

    # ---- collect every leaf target whose source dir is under us ----------
    get_property(_all_targets GLOBAL PROPERTY DJINTERP_TEST_TARGETS)
    get_property(_all_dirs    GLOBAL PROPERTY DJINTERP_TEST_TARGET_DIRS)

    set(_my_deps "")
    list(LENGTH _all_targets _n)
    if (_n GREATER 0)
        math(EXPR _last "${_n} - 1")
        foreach(_i RANGE 0 ${_last})
            list(GET _all_targets ${_i} _t)
            list(GET _all_dirs    ${_i} _d)

            # prefix match: source dir of the leaf starts with our source dir
            string(LENGTH "${CMAKE_CURRENT_SOURCE_DIR}" _our_len)
            string(SUBSTRING "${_d}" 0 ${_our_len} _d_prefix)
            if (_d_prefix STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
                list(APPEND _my_deps ${_t})
            endif()
        endforeach()
    endif()

    # ---- create the aggregate -------------------------------------------
    if (TARGET ${_aggregate_name})
        message(FATAL_ERROR
                "djinterp_add_test_subtree(${_aggregate_name}): "
                "target name already in use")
    endif()

    add_custom_target(${_aggregate_name})

    if (_my_deps)
        add_dependencies(${_aggregate_name} ${_my_deps})
    else()
        message(WARNING
                "djinterp_add_test_subtree(${_aggregate_name}): "
                "no test leaves found under ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # ---- VS solution explorer folder for the aggregate ------------------
    file(RELATIVE_PATH _rel
         "${DJINTERP_CONFIG_ROOT}"
         "${CMAKE_CURRENT_SOURCE_DIR}")
    if (_rel)
        set_target_properties(${_aggregate_name} PROPERTIES
                              FOLDER "${_rel}")
    endif()

endfunction()


# -----------------------------------------------------------------------------
# djinterp_test_module
#   The module a leaf's directory tests, as the banner tag names it: under
# testing/djinterp/, the first folder below c/ or cpp/ without modules/ or
# core/, util/ as utility; env and tools/ their own; a leaf for a file of the
# language folder itself (dstring, dtime, ...) "root". Empty outside
# testing/djinterp/.
# -----------------------------------------------------------------------------
function(djinterp_test_module _out_var _dir)
    set(_module "")
    file(RELATIVE_PATH _leaf "${DJINTERP_CONFIG_ROOT}/testing/djinterp" "${_dir}")
    if (_leaf AND NOT _leaf MATCHES "^\\.\\.")
        string(REPLACE "/" ";" _parts "${_leaf}")
        list(GET _parts 0 _first)
        if (_first STREQUAL "tools")
            set(_module "tools")
        elseif (_first STREQUAL "c" OR _first STREQUAL "cpp")
            list(REMOVE_AT _parts 0)
            foreach(_part IN LISTS _parts)
                if (NOT _part STREQUAL "modules" AND NOT _part STREQUAL "core")
                    set(_module "${_part}")
                    break()
                endif()
            endforeach()
            if (_module STREQUAL "util")
                set(_module "utility")
            elseif (_module AND NOT _module STREQUAL "env" AND
                    NOT IS_DIRECTORY "${DJINTERP_INC_ROOT}/djinterp/${_first}/${_module}" AND
                    NOT IS_DIRECTORY "${DJINTERP_INC_ROOT}/djinterp/${_first}/modules/${_module}")
                set(_module "root")
            endif()
        endif()
    endif()
    set(${_out_var} "${_module}" PARENT_SCOPE)
endfunction()


# -----------------------------------------------------------------------------
# djinterp_add_module_test_targets
#   One target a module, djinterp_tests_<module>, that builds every suite of
# the module, C and C++ alike. Called once, after the testing tree.
# -----------------------------------------------------------------------------
function(djinterp_add_module_test_targets)
    get_property(_modules GLOBAL PROPERTY DJINTERP_TEST_MODULES)
    if (NOT _modules)
        return()
    endif()
    list(REMOVE_DUPLICATES _modules)
    foreach(_module IN LISTS _modules)
        get_property(_targets GLOBAL PROPERTY "DJINTERP_TEST_MODULE_${_module}")
        add_custom_target(djinterp_tests_${_module})
        add_dependencies(djinterp_tests_${_module} ${_targets})
        set_target_properties(djinterp_tests_${_module} PROPERTIES
                              FOLDER "djinterp/modules")
    endforeach()
endfunction()
