#*******************************************************************************
# bitwisedom [cmake]                                             Freshness.cmake
#
# Sources that changed since this build last configured, marked to compile
# again.
#   An overlay unzipped over the trees keeps the times its archive holds,
# and those can be older than the last build's objects: a build would then
# take the objects of the files the overlay replaced for the files it
# brought, and the tests would run the code of the overlay before -- and
# fail on the section counts it changed. So each configure keeps a hash of
# every source it watches -- bitwisedom's own and the kit's here, and in
# build/ui the window's and uxoxo's with the Dear ImGui in it -- in the build
# tree, and touches each whose content differs
# from the hash the last configure kept: the build then sees it as new, and
# compiles it, and what includes it, again. A build tree configured before
# it kept any hashes, but configured all the same, has every source touched
# once; a new build tree compiles everything anyway, and has none touched.
#   Only the time of a changed file is changed, never its content. A file
# that cannot be touched -- one read-only -- is named, and the configure
# goes on.
#
#     bitwisedom_freshen(<stamp> <root>...)
#                               watches the sources under each root
#
# path:      /build/cmake/djinterp/tools/bitwisedom/cmake/Freshness.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
#                                                            revised: 2026.10.22
#*******************************************************************************
include_guard(GLOBAL)

# bitwisedom_freshen(<stamp> <root>...)
#   Touches the sources under the roots whose content changed since the
# hashes in <stamp> were kept -- every one, in a build tree configured
# before any were kept -- and keeps their hashes there for the next
# configure. A root that does not exist is passed over.
function(bitwisedom_freshen _stamp)
    set(_files "")

    foreach(_root IN LISTS ARGN)
        if(NOT IS_DIRECTORY "${_root}")
            continue()
        endif()

        file(GLOB_RECURSE _found LIST_DIRECTORIES false
             "${_root}/*.c" "${_root}/*.cpp" "${_root}/*.h"
             "${_root}/*.hpp" "${_root}/*.inc" "${_root}/*.inl")

        # what no build compiles: ImGui's examples and documents
        list(FILTER _found EXCLUDE REGEX "/(examples|docs|\\.git)/")
        list(APPEND _files ${_found})
    endforeach()

    list(REMOVE_DUPLICATES _files)
    list(SORT _files)

    # the hashes the last configure kept, by path; none at all, in a tree
    # configured before any were kept, is every file changed
    set(_kept FALSE)
    set(_every FALSE)

    if(EXISTS "${_stamp}")
        set(_kept TRUE)
        file(STRINGS "${_stamp}" _lines)

        foreach(_line IN LISTS _lines)
            string(FIND "${_line}" " " _space)

            if(_space LESS 0)
                continue()
            endif()

            string(SUBSTRING "${_line}" 0 ${_space} _hash)
            math(EXPR _from "${_space} + 1")
            string(SUBSTRING "${_line}" ${_from} -1 _path)
            string(SHA1 _key "${_path}")
            set("_was_${_key}" "${_hash}")
        endforeach()
    elseif(EXISTS "${CMAKE_BINARY_DIR}/CMakeCache.txt")
        set(_every TRUE)
    endif()

    set(_text "")
    set(_changed "")

    foreach(_file IN LISTS _files)
        file(SHA1 "${_file}" _hash)
        string(SHA1 _key "${_file}")
        string(APPEND _text "${_hash} ${_file}\n")

        if( (_every) OR
            ( (_kept) AND
              (DEFINED "_was_${_key}") AND
              (NOT "${_was_${_key}}" STREQUAL "${_hash}") ) )
            list(APPEND _changed "${_file}")
        endif()
    endforeach()

    # touched forty at a time, as a command line holds them
    list(LENGTH _changed _count)
    set(_rest "${_changed}")
    set(_refused "")

    while(_rest)
        list(LENGTH _rest _left)
        set(_take 40)

        if(_left LESS _take)
            set(_take ${_left})
        endif()

        list(SUBLIST _rest 0 ${_take} _batch)

        if(_left GREATER _take)
            list(SUBLIST _rest ${_take} -1 _rest)
        else()
            set(_rest "")
        endif()

        execute_process(COMMAND "${CMAKE_COMMAND}" -E touch_nocreate ${_batch}
                        RESULT_VARIABLE _result
                        OUTPUT_QUIET
                        ERROR_QUIET)

        if(NOT _result EQUAL 0)
            list(APPEND _refused ${_batch})
        endif()
    endwhile()

    if(_count GREATER 0)
        if(_every)
            message(STATUS "bitwisedom: this build tree kept no hashes of its "
                           "sources; all ${_count} marked to compile again")
        else()
            message(STATUS "bitwisedom: ${_count} source(s) changed since "
                           "this build tree was last configured; marked to "
                           "compile again")
        endif()
    endif()

    if(_refused)
        list(JOIN _refused "\n    " _named)
        message(WARNING "bitwisedom: could not mark these to compile again "
                        "(read-only?); if the build takes old objects for "
                        "them, build with --clean-first:\n    ${_named}")
    endif()

    file(WRITE "${_stamp}.new" "${_text}")
    file(COPY_FILE "${_stamp}.new" "${_stamp}" ONLY_IF_DIFFERENT)
    file(REMOVE "${_stamp}.new")
endfunction()
