/*******************************************************************************
* djinterp [djinterp]                                                 dsection.c
*
* Section-delimiter extension:
*   A delimiter is `//`, then optionally one space, then at least twenty of a
* single fill character, `=` or `-`, and nothing else.  Twenty is the floor
* because a short run is a comment that happens to contain dashes, and the
* cost of mistaking one for a delimiter is rewriting prose.
*
*
* path:      /src/djinterp/tools/dawk/dsection.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dsection.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fgets, fclose
#include <string.h>  // strlen, strncmp
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"  // D_BANNER_LINE_MAX
#include "../../../../inc/djinterp/tools/dawk/dedit.h"    // d_edit_trim_end


// D_SECTION_FILL_MIN
//   constant: the shortest run of fill that counts as a delimiter.
#define D_SECTION_FILL_MIN 20u


uint32_t
d_section_build(
    struct d_node_tree* _tree,
    uint32_t            _file,
    const char*         _path
)
{
    // parameter validation first
    if ((!_tree) || (!_path) || (_file == D_DSS_NO_INDEX))
    {
        return 0;
    }

    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return 0;
    }

    char     buffer[D_BANNER_LINE_MAX];
    uint32_t line  = 0;
    uint32_t count = 0;

    while (fgets(buffer, (int)sizeof(buffer), handle))
    {
        ++line;

        const size_t length = d_edit_trim_end(buffer, strlen(buffer));

        if ((length < 2u) || (strncmp(buffer, "//", 2u) != 0))
        {
            continue;
        }

        const bool   spaced = ((length > 2u) && (buffer[2] == ' '));
        const size_t from   = spaced ? 3u : 2u;

        if (from >= length)
        {
            continue;
        }

        const char fill = buffer[from];

        if ((fill != '=') && (fill != '-'))
        {
            continue;
        }

        size_t at = from;

        while ((at < length) && (buffer[at] == fill))
        {
            ++at;
        }

        // the whole rest of the line must be fill, and enough of it
        if ((at != length) || ((length - from) < D_SECTION_FILL_MIN))
        {
            continue;
        }

        const uint32_t delimiter = d_node_add(_tree, _file, "delimiter");

        if (delimiter == D_DSS_NO_INDEX)
        {
            break;
        }

        struct d_node* const node = d_node_at(_tree, delimiter);

        node->line         = line;
        node->start_column = 1u;
        node->width        = (uint32_t)length;
        node->end_column   = (uint32_t)length;

        (void)d_node_set_attribute(_tree, delimiter, "form",
                                   spaced ? "spaced" : "guide");
        (void)d_node_set_attribute(_tree, delimiter, "level",
                                   (fill == '=') ? "section" : "subsection");
        (void)d_node_set_text(_tree, delimiter, buffer, length);

        ++count;
    }

    (void)fclose(handle);

    return count;
}
