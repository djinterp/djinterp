/*******************************************************************************
* bitwisedom [bitwisedom]                                             rename.cpp
*
* Renaming a row, and everything that refers to it.
*   The sheet is resolved once, before the rename, and every reference is
* judged by that resolution: a name in a row means the symbol the row would
* have found by it. Each text is rewritten from its last reference to its
* first, so the offsets the scan found stay true while it is rewritten.
*
* path:      /src/djinterp/tools/bitwisedom/rename.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/
#include <djinterp/tools/bitwisedom/rename.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::find
#include <cctype>     // std::isalnum, std::isalpha, std::isdigit
#include <cstddef>    // std::size_t
#include <functional> // std::function
// bitwisedom
#include <djinterp/tools/bitwisedom/cenv.hpp>     // read_params
#include <djinterp/tools/bitwisedom/clex.hpp>     // lex_c
#include <djinterp/tools/bitwisedom/resolve.hpp>  // resolve, find_name


namespace bitwisedom
{

namespace
{

// edit
//   struct: one reference to rewrite: where it is, and what it becomes.
struct edit
{
    std::size_t at     = 0;
    std::size_t length = 0;
    std::string text;
};

// rewrite
//   a text's edits made, the last first.
std::string
rewrite(
    const std::string&       _text,
    const std::vector<edit>& _edits
)
{
    std::string out = _text;

    for (std::size_t i = _edits.size(); i-- > 0; )
    {
        out.replace(_edits[i].at, _edits[i].length, _edits[i].text);
    }

    return out;
}

// trimmed
//   a name without the blanks around it.
std::string
trimmed(
    const std::string& _text
)
{
    const std::size_t first = _text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
    {
        return std::string();
    }

    const std::size_t last = _text.find_last_not_of(" \t\r\n");

    return _text.substr(first, last - first + 1);
}

/*
expression_edits
  An expression's names that mean the renamed row: its macro name, or its
own name where the expression would find the row by it. The expression
language has no strings or comments, so a scan for names suffices; a
number's letters (0x1Fu) are skipped with the number.
*/
std::vector<edit>
expression_edits(
    const std::string& _text,
    const resolution&  _before,
    int                _from,
    int                _id,
    const std::string& _old_local,
    const std::string& _old_full,
    const std::string& _new_local,
    const std::string& _new_full
)
{
    std::vector<edit> out;
    std::size_t       at = 0;

    while (at < _text.size())
    {
        const unsigned char c = static_cast<unsigned char>(_text[at]);

        if ( (!std::isalnum(c)) &&
             (_text[at] != '_') )
        {
            ++at;

            continue;
        }

        const std::size_t start = at;

        while ( (at < _text.size()) &&
                ( (std::isalnum(static_cast<unsigned char>(_text[at]))) ||
                  (_text[at] == '_') ||
                  ( (std::isdigit(c)) && (_text[at] == '\'') ) ) )
        {
            ++at;
        }

        if (std::isdigit(c))
        {
            continue;
        }

        const std::string name = _text.substr(start, at - start);
        std::string       error;

        if (name == _old_full)
        {
            out.push_back(edit{ start, name.size(), _new_full });
        }
        else if ( (name == _old_local) &&
                  (find_name(_before, name, _from, error) == _id) )
        {
            out.push_back(edit{ start, name.size(), _new_local });
        }
    }

    return out;
}

/*
body_edits
  A macro body's references: an identifier that is the old macro name, and
a {placeholder} that names the row -- but not one of the macro's own
parameters, which only share the spelling.
*/
std::vector<edit>
body_edits(
    const row&         _macro,
    standard           _level,
    const resolution&  _before,
    int                _id,
    const std::string& _old_full,
    const std::string& _new_local,
    const std::string& _new_full
)
{
    std::vector<edit> out;
    const c_lexed     scanned = lex_c(_macro.body, _level, true);
    const auto&       tokens  = scanned.tokens;
    c_macro           shape;
    std::string       ignored;

    (void)read_params(_macro.params, shape, ignored);

    for (std::size_t i = 0; i < tokens.size(); ++i)
    {
        const c_token& each = tokens[i];

        if ( (each.kind != c_class::identifier) ||
             (std::find(shape.params.begin(),
                        shape.params.end(),
                        each.text) != shape.params.end()) ||
             ( (shape.variadic) &&
               (each.text == shape.variadic_name) ) )
        {
            continue;
        }

        const bool placeholder = ( (i > 0)                       &&
                                   (tokens[i - 1].is("{"))       &&
                                   (i + 1 < tokens.size())       &&
                                   (tokens[i + 1].is("}")) );

        if (placeholder)
        {
            std::string error;

            if (find_name(_before, each.text, _macro.id, error) == _id)
            {
                out.push_back(edit{ each.offset,
                                    each.length,
                                    (each.text == _old_full) ? _new_full
                                                             : _new_local });
            }

            continue;
        }

        if (each.text == _old_full)
        {
            out.push_back(edit{ each.offset, each.length, _new_full });
        }
    }

    return out;
}

// each_row
//   every row below a group, depth first.
void
each_row(
    row&                             _group,
    const std::function<void(row&)>& _visit
)
{
    for (row& child : _group.children)
    {
        _visit(child);
        each_row(child, _visit);
    }

    return;
}

}  // anonymous namespace


renamed
rename_row(
    sheet&             _sheet,
    int                _id,
    const std::string& _name
)
{
    renamed     out;
    row*        target = find_row(_sheet, _id);
    std::string name   = trimmed(_name);

    if ( (target == nullptr) ||
         (_id == 0) )
    {
        out.error = "there is no such row to rename";

        return out;
    }

    if (target->kind == row_kind::group)
    {
        if (name.empty())
        {
            out.error = "a group needs a name";

            return out;
        }

        target->name = name;
        out.ok       = true;

        return out;
    }

    const resolution    before = resolve(_sheet);
    const resolved_row* old    = before.find(_id);

    if (old == nullptr)
    {
        out.error = "there is no such row to rename";

        return out;
    }

    const std::string old_local = target->name;
    const std::string old_full  = old->full;
    const std::string new_full  = old->prefix + name + old->suffix;

    if (name == old_local)
    {
        out.ok = true;

        return out;
    }

    if ( (name.empty()) ||
         (!is_identifier(new_full)) )
    {
        out.error = "'" + new_full + "' is not a C identifier";

        return out;
    }

    const auto taken = before.by_full.find(new_full);

    if ( (taken != before.by_full.end()) &&
         (taken->second != _id) )
    {
        out.error = new_full + " is already another symbol's name";

        return out;
    }

    // every reference, by the resolution before the rename
    const standard level = _sheet.gen.level;

    each_row(_sheet.root,
             [&](row& _each)
             {
                 if (_each.kind != row_kind::symbol)
                 {
                     return;
                 }

                 std::vector<edit> edits;

                 if (_each.def == def_kind::expression)
                 {
                     edits = expression_edits(_each.expr, before, _each.id,
                                              _id, old_local, old_full,
                                              name, new_full);

                     if (!edits.empty())
                     {
                         _each.expr = rewrite(_each.expr, edits);
                     }
                 }
                 else if (_each.def == def_kind::macro)
                 {
                     edits = body_edits(_each, level, before, _id, old_full,
                                        name, new_full);

                     if (!edits.empty())
                     {
                         _each.body = rewrite(_each.body, edits);
                     }
                 }

                 if (!edits.empty())
                 {
                     out.references += static_cast<int>(edits.size());
                     out.rows.push_back(_each.id);
                 }
             });

    // the lookup above may have moved nothing, but find the row afresh
    target = find_row(_sheet, _id);

    if (target != nullptr)
    {
        target->name = name;
    }

    out.ok = true;

    return out;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
