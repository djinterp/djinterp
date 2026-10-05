/*******************************************************************************
* djinterp [djinterp]                                                   dguard.c
*
* Include-guard extension:
*   Scans a header for its guard and emits three nodes carrying the symbol
* each one names.  A guard that is absent emits nodes with `present` false,
* for the same reason the banner extension does: a rule cannot match what
* does not exist.
*   Only the first `#ifndef` is taken.  A header may contain many, but the
* guard is the one that opens the file, and a later conditional is not a
* second guard.
*
*
* path:      /src/djinterp/tools/dawk/dguard.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dguard.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fgets
#include <string.h>  // strncmp, strlen, strstr
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"  // D_BANNER_LINE_MAX
#include "../../../../inc/djinterp/tools/dawk/dedit.h"    // d_edit_trim_end


/*
d_internal_symbol
  Copies the identifier that follows a directive, stopping at whitespace or a
comment.  Returns its length.
*/
static size_t
d_internal_symbol(
    const char* _text,
    char*       _out,
    size_t      _out_size
)
{
    size_t at = 0;

    while ((_text[at] == ' ') || (_text[at] == '\t'))
    {
        ++at;
    }

    size_t length = 0;

    while ( (_text[at + length] != '\0') &&
            (_text[at + length] != ' ')  &&
            (_text[at + length] != '\t') &&
            (_text[at + length] != '\r') &&
            (_text[at + length] != '\n') &&
            (length + 1u < _out_size) )
    {
        _out[length] = _text[at + length];
        ++length;
    }

    _out[length] = '\0';

    return length;
}


/*
d_internal_part
  Adds one guard part -- ifndef, define or endif -- with the symbol it names
as its text and the geometry a layout rule would need.
*/
static void
d_internal_part(
    struct d_node_tree* _tree,
    uint32_t            _guard,
    const char*         _type,
    const char*         _symbol,
    uint32_t            _line,
    uint32_t            _column
)
{
    const uint32_t part = d_node_add(_tree, _guard, _type);

    if (part == D_DSS_NO_INDEX)
    {
        return;
    }

    struct d_node* const node = d_node_at(_tree, part);

    node->line         = _line;
    node->start_column = _column;
    node->width        = _symbol ? (uint32_t)strlen(_symbol) : 0u;
    node->end_column   = _column ? (_column + node->width - 1u) : 0u;
    node->present      = (_symbol != NULL);

    if (_symbol)
    {
        (void)d_node_set_text(_tree, part, _symbol, strlen(_symbol));
    }

    return;
}


/*
d_guard_build
  Scans for the guard and emits its three parts.  The `#endif` comment is
matched loosely -- the symbol may follow `//` or be absent entirely -- because
the rule that governs its spelling is the sheet's business, not the scanner's.
*/
uint32_t
d_guard_build(
    struct d_node_tree* _tree,
    uint32_t            _file,
    const char*         _path
)
{
    // parameter validation first
    if ((!_tree) || (!_path))
    {
        return D_DSS_NO_INDEX;
    }

    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return D_DSS_NO_INDEX;
    }

    char buffer[D_BANNER_LINE_MAX];

    char     ifndef_symbol[256] = { 0 };
    char     define_symbol[256] = { 0 };
    char     endif_symbol[256]  = { 0 };
    uint32_t ifndef_line        = 0;
    uint32_t define_line        = 0;
    uint32_t endif_line         = 0;
    uint32_t ifndef_column      = 0;
    uint32_t define_column      = 0;
    uint32_t endif_column       = 0;
    size_t   endif_length       = 0;
    bool     endif_block        = false;
    uint32_t line               = 0;

    while (fgets(buffer, (int)sizeof(buffer), handle))
    {
        ++line;

        (void)d_edit_trim_end(buffer, strlen(buffer));

        if ((ifndef_line == 0) && (strncmp(buffer, "#ifndef", 7u) == 0))
        {
            (void)d_internal_symbol(buffer + 7, ifndef_symbol,
                                    sizeof(ifndef_symbol));
            ifndef_line   = line;
            ifndef_column = (uint32_t)(strstr(buffer, ifndef_symbol) - buffer)
                          + 1u;
            continue;
        }

        if ((ifndef_line != 0) && (define_line == 0) &&
            (strncmp(buffer, "#define", 7u) == 0))
        {
            (void)d_internal_symbol(buffer + 7, define_symbol,
                                    sizeof(define_symbol));
            define_line   = line;
            define_column = (uint32_t)(strstr(buffer, define_symbol) - buffer)
                          + 1u;
            continue;
        }

        // the last #endif in the file closes the guard
        if (strncmp(buffer, "#endif", 6u) == 0)
        {
            const char* const comment = strstr(buffer, "//");

            endif_line = line;

            // where a missing comment would go: after the directive's last
            // non-blank character
            endif_length = strlen(buffer);

            while ((endif_length > 0) &&
                   ((buffer[endif_length - 1u] == ' ') ||
                    (buffer[endif_length - 1u] == '\t')))
            {
                --endif_length;
            }

            endif_block = false;

            if (comment)
            {
                (void)d_internal_symbol(comment + 2, endif_symbol,
                                        sizeof(endif_symbol));
                endif_column =
                    (uint32_t)(strstr(buffer, endif_symbol) - buffer) + 1u;
            }
            else
            {
                endif_symbol[0] = '\0';
                endif_column    = 0;

                // A block comment holding one identifier and nothing else is
                // a guard name in the older style.  It is replaced by the
                // line-comment form rather than kept alongside it -- keeping
                // both leaves two names for one guard, one of them stale.
                const char* const open  = strstr(buffer, "/*");
                const char* const close = open ? strstr(open + 2, "*/")
                                               : NULL;

                if (close)
                {
                    const char* at = open + 2;

                    while ((*at == ' ') || (*at == '\t'))
                    {
                        ++at;
                    }

                    const char* const word = at;

                    while ( ((*at >= 'A') && (*at <= 'Z')) ||
                            ((*at >= 'a') && (*at <= 'z')) ||
                            ((*at >= '0') && (*at <= '9')) || (*at == '_') )
                    {
                        ++at;
                    }

                    const bool one_word = (at > word);

                    while ((*at == ' ') || (*at == '\t'))
                    {
                        ++at;
                    }

                    const char* tail = close + 2;

                    while ((*tail == ' ') || (*tail == '\t'))
                    {
                        ++tail;
                    }

                    endif_block = ((one_word) && (at == close) &&
                                   (*tail == '\0'));
                }
            }
        }
    }

    (void)fclose(handle);

    // a file with no #ifndef at all has no guard to describe
    if (ifndef_line == 0)
    {
        return D_DSS_NO_INDEX;
    }

    const uint32_t guard = d_node_add(_tree, _file, "guard");

    if (guard == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    d_internal_part(_tree, guard, "ifndef", ifndef_symbol, ifndef_line,
                    ifndef_column);
    d_internal_part(_tree, guard, "define",
                    (define_line != 0) ? define_symbol : NULL,
                    define_line, define_column);
    d_internal_part(_tree, guard, "endif",
                    (endif_symbol[0] != '\0') ? endif_symbol : NULL,
                    endif_line, endif_column);

    // An #endif with no comment is an absent part with a known place to put
    // one.  The extension owns the syntax -- it knows what a guard comment
    // looks like, two spaces and `//` by this tree's measured convention --
    // and the sheet's derivation owns the value.
    if ((endif_line != 0) && (endif_symbol[0] == '\0'))
    {
        struct d_node* const guard_node = d_node_at(_tree, guard);

        uint32_t endif = guard_node->first_child;

        while (d_node_at(_tree, endif)->next_sibling != D_DSS_NO_INDEX)
        {
            endif = d_node_at(_tree, endif)->next_sibling;
        }

        // a replaceable block comment is overwritten from the directive's
        // end; a bare #endif is appended to
        d_node_at(_tree, endif)->start_column =
            endif_block ? 7u : ((uint32_t)endif_length + 1u);

        (void)d_node_set_attribute(_tree, endif, "syntax", "  // ");

        if (endif_block)
        {
            (void)d_node_set_attribute(_tree, endif, "replaces", "block");
        }
    }

    return guard;
}
