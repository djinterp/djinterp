/*******************************************************************************
* djinterp [djinterp]                                                 dext_cpp.c
*
* Definitions for the non-inline declarations in dext_cpp.h.
*
*
* path:      /src/djinterp/tools/dawk/ext/dext_cpp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/dawk/ext/dext_cpp.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t
#include <string.h>   // strlen, strrchr, strcmp

// djinterp
#include "../../../../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../../../../inc/djinterp/parse/source/source_reader.h"


/*
d_ext_cpp_accepts
  Reports whether a path names C++ source by its extension.
*/
bool
d_ext_cpp_accepts(
    const char* _path
)
{
    static const char* const claimed[] =
    {
        ".cpp",
        ".hpp",
        ".cc",
        ".hh",
        ".cxx",
        ".hxx",
        ".ipp",
        ".tpp"
    };

    // parameter validation first
    if (!_path)
    {
        return false;
    }

    const char* const dot = strrchr(_path, '.');

    // a path with no extension is claimed by nothing
    if (!dot)
    {
        return false;
    }

    for (size_t at = 0u; at < (sizeof(claimed) / sizeof(claimed[0])); ++at)
    {
        if (strcmp(dot, claimed[at]) == 0)
        {
            return true;
        }
    }

    return false;
}


/*
d_ext_cpp_build
  Builds the node tree for C++ source.
*/
uint32_t
d_ext_cpp_build(
    struct d_node_tree*              _tree,
    uint32_t                         _parent,
    unsigned                         _level,
    const struct d_token_node_input* _input,
    struct d_token_node_stats*       _out_stats
)
{
    // parameter validation first
    if ( (!_tree)  ||
         (!_input) )
    {
        return D_DSS_NO_INDEX;
    }

    const unsigned level = (_level == 0u) ? d_lex_cpp_default_level() : _level;

    return d_token_node_build(_tree,
                              _parent,
                              d_lex_cpp_dialect(level),
                              _input,
                              _out_stats);
}
