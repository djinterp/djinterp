# =============================================================================
# djinterp - build/cmake/djinterp/standards.cmake
#
#   Whether the compiler builds a C or C++ level as that level, for a leaf
# that builds its suite once per level. Each leaf asks this one function, so
# a level the compiler cannot select is left out everywhere alike, with a
# status line saying why, rather than built at another level under its name.
#
#   djinterp_standard_selectable(<C|CXX> <level> <result> [<reason>])
#     <result> is TRUE when the compiler builds <level> as itself; else FALSE,
#     and <reason>, if named, says why, for the leaf's status line.
#
#   A level is selected when all three hold:
#     1. it is among the compiler's features (c_std_<level>, cxx_std_<level>);
#     2. the compiler's switch for it is not a draft's ("latest", "preview"),
#        which names whatever the compiler's newest draft is, not the level;
#     3. a program built at it with the leaves' properties (the level
#        required, extensions off) sees the level's own value: in C,
#        __STDC_VERSION__; in C++, __cplusplus, or _MSVC_LANG where cl
#        defines it. A compiler with no switch for a level builds its default
#        instead, and CMake lists the level all the same: MSVC builds C++14
#        for C++98 and C++11, and for C99 a C with no __STDC_VERSION__.
#   The values the probe accepts, as each level's drafts give them too:
#     C:   99 [199901, 201112)  11 [201112, 201710)  17 [201710, 201711)
#          23 [201711, ...)
#     C++: 98 [199711, 201103)  11 [201103, 201402)  14 [201402, 201703)
#          17 [201703, 202002)  20 [202002, 202100)  23 [202100, 202400)
#          26 [202400, ...)
#   The probe runs once per language and level, and its answer is cached
# (DJINTERP_STANDARD_<C|CXX><level>), so a leaf may ask as often as it likes.
#
#   A leaf that also stands alone includes this file itself, by the repository
# root it computes; the project includes it before any leaf.
# =============================================================================

include_guard(GLOBAL)

set(_djinterp_standards_dir "${CMAKE_CURRENT_LIST_DIR}")

# djinterp_standard_selectable
#   function: see the file's banner.
function(djinterp_standard_selectable _lang _level _result)

    set(_reason_var "${ARGV3}")

    if (_lang STREQUAL "C")
        set(_name     "C${_level}")
        set(_feature  "c_std_${_level}")
        set(_macro    "__STDC_VERSION__")
        set(_ranges   "99:199901:201112" "11:201112:201710"
                      "17:201710:201711" "23:201711:0")
        set(_ext      "c")
    elseif (_lang STREQUAL "CXX")
        set(_name     "C++${_level}")
        set(_feature  "cxx_std_${_level}")
        set(_macro    "D_INTERNAL_STANDARD_CPLUSPLUS")
        set(_ranges   "98:199711:201103" "11:201103:201402"
                      "14:201402:201703" "17:201703:202002"
                      "20:202002:202100" "23:202100:202400"
                      "26:202400:0")
        set(_ext      "cpp")
    else()
        message(FATAL_ERROR
                "djinterp_standard_selectable: no language '${_lang}'; "
                "C or CXX")
    endif()

    set(_selected FALSE)
    set(_why      "")

    get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    set(_option "${CMAKE_${_lang}${_level}_STANDARD_COMPILE_OPTION}")

    if (NOT "${_lang}" IN_LIST _languages)
        set(_why "the project does not enable ${_lang}")
    elseif (NOT "${_feature}" IN_LIST CMAKE_${_lang}_COMPILE_FEATURES)
        set(_why "${CMAKE_${_lang}_COMPILER_ID} "
                 "${CMAKE_${_lang}_COMPILER_VERSION} cannot select it")
        string(CONCAT _why ${_why})
    elseif ("${_option}" MATCHES "latest|preview")
        set(_why "the compiler's switch for it is ${_option}, its latest "
                 "draft and not the level")
        string(CONCAT _why ${_why})
    else()
        # the level's range of values
        set(_low  "")
        set(_high "")
        foreach (_range IN LISTS _ranges)
            string(REPLACE ":" ";" _parts "${_range}")
            list(GET _parts 0 _key)
            if (_key STREQUAL _level)
                list(GET _parts 1 _low)
                list(GET _parts 2 _high)
            endif()
        endforeach()

        if (_low STREQUAL "")
            set(_why "djinterp knows no value of ${_macro} for it")
        else()
            set(_cache "DJINTERP_STANDARD_${_lang}${_level}")
            if (NOT DEFINED ${_cache})
                _djinterp_standard_probe("${_lang}" "${_level}" "${_ext}"
                                         "${_macro}" "${_low}" "${_high}"
                                         _probed)
                set(${_cache} "${_probed}" CACHE INTERNAL
                    "whether the ${_lang} compiler builds ${_name} as itself")
            endif()

            if (${_cache})
                set(_selected TRUE)
            elseif ("${_option}" STREQUAL "")
                set(_why "the compiler has no switch for it, and what it "
                         "builds in its place is another level")
                string(CONCAT _why ${_why})
            else()
                set(_why "a program built with ${_option} does not see "
                         "${_name}'s value of ${_macro}")
                string(CONCAT _why ${_why})
            endif()
        endif()
    endif()

    set(${_result} ${_selected} PARENT_SCOPE)
    if (_reason_var)
        set(${_reason_var} "${_why}" PARENT_SCOPE)
    endif()

endfunction()

# _djinterp_standard_probe
#   function: build, at one level with the leaves' properties, a program that
# compiles only where the level's macro has the level's value.
function(_djinterp_standard_probe _lang _level _ext _macro _low _high _out)

    set(_dir "${CMAKE_BINARY_DIR}/djinterp_standards/${_lang}${_level}")
    file(MAKE_DIRECTORY "${_dir}")
    set(_source "${_dir}/probe.${_ext}")

    set(_text "")
    if (_lang STREQUAL "CXX")
        string(APPEND _text
               "#if defined(_MSVC_LANG)\n"
               "#   define D_INTERNAL_STANDARD_CPLUSPLUS _MSVC_LANG\n"
               "#else\n"
               "#   define D_INTERNAL_STANDARD_CPLUSPLUS __cplusplus\n"
               "#endif\n")
    else()
        string(APPEND _text
               "#if !defined(__STDC_VERSION__)\n"
               "#   error \"no __STDC_VERSION__: not a level of C\"\n"
               "#endif\n")
    endif()
    string(APPEND _text
           "#if (${_macro} < ${_low}L)\n"
           "#   error \"below the level\"\n"
           "#endif\n")
    if (NOT _high STREQUAL "0")
        string(APPEND _text
               "#if (${_macro} >= ${_high}L)\n"
               "#   error \"above the level\"\n"
               "#endif\n")
    endif()
    string(APPEND _text "int main(void) { return 0; }\n")
    file(WRITE "${_source}" "${_text}")

    try_compile(_built "${_dir}/build"
                SOURCES "${_source}"
                ${_lang}_STANDARD          ${_level}
                ${_lang}_STANDARD_REQUIRED ON
                ${_lang}_EXTENSIONS        OFF
                OUTPUT_VARIABLE            _log)

    set(${_out} ${_built} PARENT_SCOPE)

endfunction()
