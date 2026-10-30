#*******************************************************************************
# vparse [cl_standin]                                             override.cmake
#
# The link and archive rules for the stand-in for cl. CMake reads this after
# its own files for MSVC, through CMAKE_USER_MAKE_RULES_OVERRIDE, which
# toolchain.cmake sets.
#   Those files call link.exe and lib.exe with their command lines, by way of
# the manifest step. Here an executable is linked by the script link, a
# module -- a DLL, such as the window `vparse gui` loads -- by the same
# script, and an archive made by ar, each in the usual form. The Windows
# libraries cl links by default are not named: the MinGW-w64 driver names its
# own.
#
# path:      /build/cmake/config/djinterp/tools/vparse/cl_standin/override.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
#                                                            revised: 2026.10.26
#*******************************************************************************
foreach(_language C CXX)
    set(CMAKE_${_language}_LINK_EXECUTABLE
        "<CMAKE_LINKER> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

    # a module has no import library: it is found and loaded by name, never
    # linked against
    set(CMAKE_${_language}_CREATE_SHARED_MODULE
        "<CMAKE_LINKER> -shared <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

    # an archive is made afresh: ar adds to one that is there
    set(CMAKE_${_language}_CREATE_STATIC_LIBRARY
        "<CMAKE_COMMAND> -E rm -f <TARGET>"
        "<CMAKE_AR> rcs <TARGET> <OBJECTS>")

    # the cache entry CMake makes from the first, after this file is read,
    # and the variable itself: which of the two a project sees depends on
    # the CMake version it asks for
    set(CMAKE_${_language}_STANDARD_LIBRARIES_INIT "")
    set(CMAKE_${_language}_STANDARD_LIBRARIES      "")
endforeach()
