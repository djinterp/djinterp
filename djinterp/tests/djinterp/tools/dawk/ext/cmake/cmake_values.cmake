#*******************************************************************************
# djinterp [tools]                                            cmake_values.cmake
#
# Arguments in every form, for comparing dcmake's values with CMake's.
#   Run under CMake, each show(...) below prints its arguments as CMake read
# them: `argc=N`, then `|[value]` for each. test_dcmake --values prints the
# same from dcmake's reading of this file -- an unquoted argument divided at
# its `;` as CMake divides it -- and cmake.sh compares the two byte for byte.
# A case is one line here; nothing is expanded, so no case holds a variable
# reference.
#
#   cmake -P tests/djinterp/tools/dawk/ext/cmake/cmake_values.cmake
#
# path:      /tests/djinterp/tools/dawk/ext/cmake/cmake_values.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
#                                                            revised: 2026.10.20
#*******************************************************************************

# show
#   function: prints its arguments, one bracketed value after another
function(show)
    set(_line "argc=${ARGC}")

    # every argument, by number: ARGV<n> holds it as CMake read it
    if(ARGC GREATER 0)
        math(EXPR _last "${ARGC} - 1")

        foreach(_at RANGE 0 ${_last})
            string(APPEND _line "|[${ARGV${_at}}]")
        endforeach()
    endif()

    message("${_line}")
endfunction()

# the three kinds
show(a "b c" [[d]])
show("" x "" y)
show(  spaced   out  )

# escapes, quoted and not
show("\t|\n|\r|\$|\"|\\|\#|\(|\)|\ |\@|\^")
show(\t\n\r \$ \" \\ \# \( \) \@ \^ a\ b)
show("a\;b")
show(a\;b)

# a continuation, and line endings inside a quoted argument
show("a\
b" "c
d")

# brackets: their own `=` count closes them, and a first newline is dropped
show([==[
x]]y]=]z]==] [=[]]]=] [[
]])

# the legacy forms
show(-Da="b c" a"b c"d $(MAKE)x x$(Y)z)

# brackets that open nothing, and `[` and `]` inside an argument
show([x] a[[b]] [=x] ]])

# parentheses nested in the arguments are arguments of their own
show((a (b)) c)

# lists: an unquoted argument divided at each `;` not escaped and not in
# square brackets, its empty items dropped; a quoted one passed whole
show(a;b ;c; ;; "x;y" a\;b a\\;b [x;y]z ]a;b[ a[[;]]b)
show(d;;e "" ";" [[f;g]])

# comments among the arguments are nothing
show(a # a comment
     b #[[ a bracket
     comment ]] c)
