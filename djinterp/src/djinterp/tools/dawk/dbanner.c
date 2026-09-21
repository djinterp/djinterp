/*******************************************************************************
* djinterp [dawk]                                                      dbanner.c
*
* Banner extension:
*   The reference extension.  It supplies node types, attributes and source
* geometry, and nothing else in the tool knows what a banner is.  Everything a
* stylesheet can say about a banner is a consequence of those three.
*
* path:      /src/djinterp/tools/dawk/dbanner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fgets
#include <string.h>  // strlen, strchr, strrchr, strncmp, strstr, memcpy
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dedit.h"  // d_edit_trim_end

/*
d_internal_field
  Adds one `* label: value` banner field as a node with its label recorded as
an attribute and the value's columns measured.  The value's start column is
what the guide's "values all begin on character 14" rule compares against.
*/
static void
d_internal_field(
    struct d_node_tree* _tree,
    uint32_t            _banner,
    const char*         _name,
    const char*         _line,
    size_t              _length,
    size_t              _value_start,
    uint32_t            _line_number
)
{
    const uint32_t field = d_node_add(_tree, _banner, "field");

    if (field == D_DSS_NO_INDEX)
    {
        return;
    }

    (void)d_node_set_attribute(_tree, field, "name", _name);

    struct d_node* const field_node = d_node_at(_tree, field);

    field_node->width      = (uint32_t)_length;
    field_node->end_column = (uint32_t)_length;
    field_node->line       = _line_number;

    const uint32_t value = d_node_add(_tree, field, "value");

    if (value == D_DSS_NO_INDEX)
    {
        return;
    }

    size_t end = _length;

    while ((end > _value_start) && (_line[end - 1u] == ' '))
    {
        --end;
    }

    struct d_node* const value_node = d_node_at(_tree, value);

    value_node->start_column = (uint32_t)(_value_start + 1u);
    value_node->end_column   = (uint32_t)end;
    value_node->width        = (uint32_t)(end - _value_start);
    value_node->line         = _line_number;

    (void)d_node_set_text(_tree, value, _line + _value_start,
                          end - _value_start);

    return;
}


/*
d_banner_build
  Reads a header's banner block and builds the node tree for it.  Returns the
banner node, or D_DSS_NO_INDEX when the file has no banner at all -- which is
itself a finding, and is counted by the caller rather than reported here.
*/
uint32_t
d_banner_build(
    struct d_node_tree* _tree,
    const char*         _path,
    const char*         _root
)
{
    FILE* const handle = fopen(_path, "rb");

    if (!handle)
    {
        return D_DSS_NO_INDEX;
    }

    char buffer[D_BANNER_LINE_MAX];

    if (!fgets(buffer, (int)sizeof(buffer), handle))
    {
        (void)fclose(handle);
        return D_DSS_NO_INDEX;
    }

    size_t length = d_edit_trim_end(buffer, strlen(buffer));

    // a file whose first line is not an opening rule has no banner
    if ((length < 2u) || (buffer[0] != '/') || (buffer[1] != '*'))
    {
        (void)fclose(handle);
        return D_DSS_NO_INDEX;
    }

    // The banner hangs beneath a file node rather than standing alone, so a
    // derivation such as top-level-dir() reads a fact off an ancestor rather
    // than being handed the path out of band.  It is also the shape the tree
    // has to take once anything below the banner is modelled.
    const uint32_t file = d_node_add(_tree, D_DSS_NO_INDEX, "file");

    size_t root_length = strlen(_root);

    while ((root_length > 0) && (_root[root_length - 1u] == '/'))
    {
        --root_length;
    }

    const char* relative = _path;

    if ((strncmp(_path, _root, root_length) == 0) &&
        (_path[root_length] == '/'))
    {
        relative = _path + root_length + 1u;
    }

    (void)d_node_set_attribute(_tree, file, "path", relative);

    const char* const slash = strrchr(relative, '/');

    (void)d_node_set_attribute(_tree, file, "name",
                               slash ? (slash + 1) : relative);

    char top[256];

    const char* const first = strchr(relative, '/');

    if ((first) && ((size_t)(first - relative) < sizeof(top)))
    {
        memcpy(top, relative, (size_t)(first - relative));
        top[first - relative] = '\0';
    }
    else
    {
        top[0] = '\0';
    }

    (void)d_node_set_attribute(_tree, file, "top", top);

    const uint32_t banner = d_node_add(_tree, file, "banner");

    uint32_t open_rule = d_node_add(_tree, banner, "rule");

    struct d_node* rule_node = d_node_at(_tree, open_rule);

    rule_node->width      = (uint32_t)length;
    rule_node->end_column = (uint32_t)length;
    rule_node->line       = 1;

    (void)d_node_set_attribute(_tree, open_rule, "position", "open");
    (void)d_node_set_text(_tree, open_rule, buffer, length);

    uint32_t line_number = 1;

    while (fgets(buffer, (int)sizeof(buffer), handle))
    {
        ++line_number;

        length = d_edit_trim_end(buffer, strlen(buffer));

        // The closing rule is a run of '*' then '/', and nothing else.  A
        // looser test ends the banner on any prose line that happens to end
        // in a slash, of which this tree has 158; the banner is then
        // truncated and every field after the cut reads as missing.
        bool is_close = ((length >= 2u) && (buffer[length - 1u] == '/'));

        for (size_t at = 0; (is_close) && (at < (length - 1u)); ++at)
        {
            if (buffer[at] != '*')
            {
                is_close = false;
            }
        }

        if (is_close)
        {
            const uint32_t close_rule = d_node_add(_tree, banner, "rule");

            struct d_node* const close_node = d_node_at(_tree, close_rule);

            close_node->width      = (uint32_t)length;
            close_node->end_column = (uint32_t)length;
            close_node->line       = line_number;

            (void)d_node_set_attribute(_tree, close_rule, "position", "close");
            (void)d_node_set_text(_tree, close_rule, buffer, length);
            break;
        }

        // the title line carries the subsystem tag and the file name
        if (line_number == 2)
        {
            const uint32_t title = d_node_add(_tree, banner, "title");

            struct d_node* const title_node = d_node_at(_tree, title);

            title_node->width      = (uint32_t)length;
            title_node->end_column = (uint32_t)length;
            title_node->line       = line_number;

            (void)d_node_set_text(_tree, title, buffer, length);

            const char* const open_bracket  = strchr(buffer, '[');
            const char* const close_bracket = open_bracket
                                            ? strchr(open_bracket, ']')
                                            : NULL;

            if (close_bracket)
            {
                const uint32_t tag = d_node_add(_tree, title, "tag");

                struct d_node* const tag_node = d_node_at(_tree, tag);

                tag_node->start_column =
                    (uint32_t)(open_bracket - buffer) + 2u;
                tag_node->width        =
                    (uint32_t)(close_bracket - open_bracket) - 1u;
                tag_node->end_column   =
                    (uint32_t)(close_bracket - buffer);
                tag_node->line         = line_number;

                (void)d_node_set_text(_tree, tag, open_bracket + 1,
                                      (size_t)(close_bracket - open_bracket)
                                      - 1u);
            }

            size_t name_start = length;

            while ((name_start > 0) && (buffer[name_start - 1u] != ' '))
            {
                --name_start;
            }

            if (name_start < length)
            {
                const uint32_t name = d_node_add(_tree, title, "name");

                struct d_node* const name_node = d_node_at(_tree, name);

                name_node->start_column = (uint32_t)(name_start + 1u);
                name_node->end_column   = (uint32_t)length;
                name_node->width        = (uint32_t)(length - name_start);
                name_node->line         = line_number;

                (void)d_node_set_text(_tree, name, buffer + name_start,
                                      length - name_start);
            }

            continue;
        }

        static const struct
        {
            const char* prefix;
            const char* name;
        }
        fields[] =
        {
            { "* path:",      "path"   },
            { "* link(s):",   "link"   },
            { "* author(s):", "author" }
        };

        bool handled = false;

        for (size_t which = 0; which < 3u; ++which)
        {
            const size_t prefix_length = strlen(fields[which].prefix);

            if (strncmp(buffer, fields[which].prefix, prefix_length) != 0)
            {
                continue;
            }

            size_t value_start = prefix_length;

            while ((value_start < length) && (buffer[value_start] == ' '))
            {
                ++value_start;
            }

            d_internal_field(_tree, banner, fields[which].name, buffer,
                                 length, value_start, line_number);

            handled = true;
            break;
        }

        // created: and revised: sit inside other lines, not at their start
        const char* const created = strstr(buffer, "created:");
        const char* const revised = strstr(buffer, "revised:");

        if (created)
        {
            const size_t at = (size_t)(created - buffer) + strlen("created:");
            size_t       vs = at;

            while ((vs < length) && (buffer[vs] == ' '))
            {
                ++vs;
            }

            d_internal_field(_tree, banner, "created", buffer, length, vs,
                                 line_number);
            handled = true;
        }

        if (revised)
        {
            const size_t at = (size_t)(revised - buffer) + strlen("revised:");
            size_t       vs = at;

            while ((vs < length) && (buffer[vs] == ' '))
            {
                ++vs;
            }

            d_internal_field(_tree, banner, "revised", buffer, length, vs,
                                 line_number);
            handled = true;
        }

        (void)handled;
    }

    (void)fclose(handle);

    // a field the guide requires but the banner never carried is emitted as
    // an absent node.  A rule cannot match a node that does not exist, so
    // `required` would otherwise assert nothing at all.
    static const char* const mandatory[] =
    {
        "path", "link", "author", "created", "revised"
    };

    for (size_t which = 0; which < 5u; ++which)
    {
        bool seen = false;

        const uint32_t wanted = d_node_find_id(_tree, mandatory[which]);

        for (uint32_t at = 0; at < (uint32_t)d_node_count(_tree); ++at)
        {
            const struct d_node* const node = d_node_at(_tree, at);

            if ((!node) || (node->parent != banner))
            {
                continue;
            }

            for (uint32_t which_attribute = 0;
                 which_attribute < node->attribute_count;
                 ++which_attribute)
            {
                const struct d_node_attribute* const attribute =
                    d_node_attribute_at(_tree,
                                        node->first_attribute
                                        + which_attribute);

                if ((attribute) && (attribute->value == wanted))
                {
                    seen = true;
                    break;
                }
            }

            if (seen)
            {
                break;
            }
        }

        if (seen)
        {
            continue;
        }

        const uint32_t absent = d_node_add(_tree, banner, "field");

        if (absent != D_DSS_NO_INDEX)
        {
            (void)d_node_set_attribute(_tree, absent, "name",
                                       mandatory[which]);

            d_node_at(_tree, absent)->present = false;
        }
    }

    return banner;
}



