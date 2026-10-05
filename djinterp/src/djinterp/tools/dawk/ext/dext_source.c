/*******************************************************************************
* djinterp [djinterp]                                              dext_source.c
*
* Definitions for dext_source.h.
*   The file is read in chunks to its end rather than sized with a seek, so a
* pipe or a file that grows while it is read is handled the same way as any
* other; the text is released before returning, since the tree copies what it
* keeps.
*
*
* path:      /src/djinterp/tools/dawk/ext/dext_source.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/dawk/ext/dext_source.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdint.h>   // uint32_t
#include <stdio.h>    // FILE, fopen, fread, fclose
#include <stdlib.h>   // realloc, free
#include <string.h>   // memcpy, memcmp, memset, strrchr, strcmp
// djinterp
#include "../../../../../inc/djinterp/parse/source/source_reader.h"  // d_source
#include "../../../../../inc/djinterp/tools/dawk/dss.h"          // D_DSS_NO_INDEX
#include "../../../../../inc/djinterp/tools/dawk/ext/dext_c.h"   // d_ext_c_build
#include "../../../../../inc/djinterp/tools/dawk/ext/dext_cpp.h" // d_ext_cpp_build


/**
 * @brief Reads a whole file into a heap buffer.
 *
 * @param[in]  _path        the file.
 * @param[out] _out_length  receives its length in bytes.
 * @return the text, which the caller frees, or `NULL` if the file could not
 *         be read; an empty file is a one-byte buffer of length zero.
 */
static char*
d_internal_read(
    const char* _path,
    size_t*     _out_length
)
{
    FILE* const handle = fopen(_path, "rb");

    // unreadable: no text
    if (!handle)
    {
        return NULL;
    }

    char*  text     = malloc(1u);
    size_t length   = 0u;
    size_t capacity = 1u;
    char   chunk[8192];
    size_t got      = 0u;

    // read to the end, doubling as needed
    while ( text &&
            ((got = fread(chunk, 1u, sizeof(chunk), handle)) > 0u) )
    {
        if ((length + got) > capacity)
        {
            size_t grown = capacity * 2u;

            while (grown < (length + got))
            {
                grown *= 2u;
            }

            char* const moved = realloc(text, grown);

            // check if memory allocation was successful
            if (!moved)
            {
                free(text);
                text = NULL;
                break;
            }

            text     = moved;
            capacity = grown;
        }

        memcpy(text + length, chunk, got);
        length += got;
    }

    fclose(handle);
    *_out_length = length;

    return text;
}


/*
d_ext_source_generated
  A plain scan of the first 4 KiB: the marker sits in a generator's banner,
so a file that carries it anywhere later is not one the generator wrote.
*/
bool
d_ext_source_generated(
    const char* _text,
    size_t      _length
)
{
    static const char marker[] = "Auto-generated. Do not edit by hand.";
    const size_t      width    = sizeof(marker) - 1u;
    const size_t      window   = (_length < 4096u) ? _length : 4096u;

    for (size_t at = 0u; (at + width) <= window; ++at)
    {
        if (memcmp(_text + at, marker, width) == 0)
        {
            return true;
        }
    }

    return false;
}


/*
d_ext_source_accepts
  C++ by its own list, C by `.c` and `.h`; anything else is not source.
*/
bool
d_ext_source_accepts(
    const char* _path
)
{
    const char* const dot = _path ? strrchr(_path, '.') : NULL;

    // parameter validation first
    if (!dot)
    {
        return false;
    }

    return ( d_ext_cpp_accepts(_path)   ||
             (strcmp(dot, ".c") == 0)   ||
             (strcmp(dot, ".h") == 0) );
}


/*
d_ext_source_build
  The file node is marked before anything is read, so a sheet can key on the
language even for a file that turns out to be generated or unreadable.
*/
uint32_t
d_ext_source_build(
    struct d_node_tree*        _tree,
    uint32_t                   _file,
    const char*                _path,
    struct d_token_node_stats* _out_stats
)
{
    struct d_token_node_stats scratch;
    struct d_token_node_stats* const stats = _out_stats ? _out_stats
                                                        : &scratch;

    memset(stats, 0, sizeof(*stats));

    // parameter validation first
    if ( (!_tree)                        ||
         (_file == D_DSS_NO_INDEX)       ||
         (!d_ext_source_accepts(_path)) )
    {
        return D_DSS_NO_INDEX;
    }

    const bool cpp = d_ext_cpp_accepts(_path);

    (void)d_node_set_attribute(_tree,
                               _file,
                               "lang",
                               cpp ? "c++" : "c");

    size_t      length = 0u;
    char* const text   = d_internal_read(_path,
                                         &length);

    // unreadable, or generated: the file node alone
    if ( (!text) ||
         d_ext_source_generated(text, length) )
    {
        free(text);

        return D_DSS_NO_INDEX;
    }

    struct d_source source;

    d_source_init(&source,
                  text,
                  length,
                  _path);
    (void)d_source_skip_bom(&source);

    const struct d_token_node_input input = { &source, NULL };
    const uint32_t                  root  = cpp
        ? d_ext_cpp_build(_tree, _file, 0u, &input, stats)
        : d_ext_c_build(_tree, _file, 0u, &input, stats);

    free(text);

    return root;
}
