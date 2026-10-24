/*******************************************************************************
* bitwisedom [bitwisedom]                                               edit.cpp
*
* Edits as the window and the command line make them alike.
*   A trial is the whole sheet copied, the row inserted or changed, and the
* copy resolved: what the row's name comes to depends on every group above
* it, so nothing short of resolving the copy says whether it is a C
* identifier and nobody else's. The copy is then the sheet to keep, so a
* row is never inserted twice.
*   A sort is the window's: display_order says where each row is shown, and
* each group's rows are put in that order.
*
* path:      /src/djinterp/tools/bitwisedom/edit.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
#include <djinterp/tools/bitwisedom/edit.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>   // std::clamp, std::find, std::stable_sort
#include <cstddef>     // std::size_t
#include <functional>  // std::function
#include <map>         // std::map
#include <utility>     // std::move, std::swap
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>     // format_dec
#include <djinterp/tools/bitwisedom/resolve.hpp>  // resolve, is_identifier


namespace bitwisedom
{

namespace
{

// BASES
//   constant: the names a new symbol takes, by its definition, in
// def_kind's order.
const char* const BASES[DEF_KIND_COUNT] = {
    "FLAG", "FIELD", "VALUE", "VALUE", "COMBINED", "MACRO" };

/*
judge
  The row judged in the copy, resolved: nowhere to be found is nowhere to
put it; a symbol's macro name must be a C identifier and no other symbol's.
The issues the copy finds with the row go with it.
*/
void
judge(
    trial& _trial
)
{
    const resolved_row* found = _trial.named.find(_trial.added);

    for (const issue& each : _trial.named.issues)
    {
        if (each.row == _trial.added)
        {
            _trial.issues.push_back(each);
        }
    }

    // nowhere to put it
    if ( (_trial.added == 0) ||
         (found == nullptr) )
    {
        _trial.refused = "there is no group or range to add it to";

        return;
    }

    // a group's name is in no macro name
    if (found->kind != row_kind::symbol)
    {
        return;
    }

    if (!is_identifier(found->full))
    {
        _trial.refused = "'" + found->full + "' is not a C identifier";

        return;
    }

    for (const resolved_row& each : _trial.named.rows)
    {
        if ( (each.id != _trial.added)        &&
             (each.kind == row_kind::symbol)  &&
             (each.full == found->full) )
        {
            _trial.refused = found->full + " is already another symbol's "
                             "name";

            break;
        }
    }

    return;
}

/*
shown_places
  Where each row is shown, by its id, with the grid sorting by a key: the
sheet copied, its sort set, and display_order's places read off.
*/
std::map<int, std::size_t>
shown_places(
    const sheet& _sheet,
    sort_key     _key,
    bool         _descending
)
{
    sheet shown = _sheet;

    shown.grid.sort_by         = _key;
    shown.grid.sort_descending = _descending;

    const resolution               named = resolve(shown);
    const std::vector<std::size_t> order = display_order(shown,
                                                         named);
    std::map<int, std::size_t>     place;

    for (std::size_t i = 0; i < order.size(); ++i)
    {
        place[named.rows[order[i]].id] = i;
    }

    return place;
}

/*
holds_rows
  Whether a parent can take a row: the sheet's own rows, a group or a range,
or a symbol, beside which insert_row puts it.
*/
bool
holds_rows(
    const sheet& _sheet,
    int          _parent
)
{
    return ( (_parent == 0) ||
             (find_row(_sheet, _parent) != nullptr) );
}

}  // anonymous namespace


/*
unique_name
  Every row's own name gathered once, then the first of _base, _base_2,
_base_3 that none has.
*/
std::string
unique_name(
    const sheet&       _sheet,
    const std::string& _base
)
{
    std::vector<std::string> taken;

    walk(_sheet,
         [&taken](const row& _row, const row&, int)
         {
             taken.push_back(_row.name);
         });

    std::string name  = _base;
    int         count = 1;

    while (std::find(taken.begin(), taken.end(), name) != taken.end())
    {
        name = _base + "_" + format_dec(static_cast<u64>(++count));
    }

    return name;
}

/*
fresh_name
  The kind's base, made unique.
*/
std::string
fresh_name(
    const sheet& _sheet,
    row_kind     _kind,
    def_kind     _def
)
{
    const int index = std::clamp(static_cast<int>(_def),
                                 0,
                                 DEF_KIND_COUNT - 1);

    return unique_name(_sheet,
                       (_kind == row_kind::group)
                           ? std::string("Group")
                           : std::string(BASES[index]));
}

/*
range_choices
  The ranges, in the tree's order.
*/
std::vector<int>
range_choices(
    const sheet& _sheet
)
{
    std::vector<int> ids;

    walk(_sheet,
         [&ids](const row& _row, const row&, int)
         {
             if ( (_row.kind == row_kind::symbol) &&
                  (_row.def == def_kind::range) )
             {
                 ids.push_back(_row.id);
             }
         });

    return ids;
}

/*
container_choices
  The sheet's own rows, then each row that can contain others.
*/
std::vector<int>
container_choices(
    const sheet& _sheet
)
{
    std::vector<int> ids = { 0 };

    walk(_sheet,
         [&ids](const row& _row, const row&, int)
         {
             if (can_contain(_row))
             {
                 ids.push_back(_row.id);
             }
         });

    return ids;
}

/*
place_bit
  next_free_bit, and where it finds none, the range's own lowest bit.
*/
int
place_bit(
    const sheet& _sheet,
    int          _parent
)
{
    const int  free   = next_free_bit(_sheet,
                                      _parent);
    const row* holder = find_row(_sheet,
                                 _parent);

    // a bit free
    if (free >= 0)
    {
        return free;
    }

    return ( (holder != nullptr) &&
             (holder->kind == row_kind::symbol) )
           ? holder->lsb
           : 0;
}

/*
try_insert
  The copy, the row in it, the copy resolved and the row judged.
*/
trial
try_insert(
    const sheet& _sheet,
    int          _parent,
    int          _index,
    const row&   _row
)
{
    trial made;

    made.result = _sheet;
    made.added  = holds_rows(_sheet,
                             _parent)
                  ? insert_row(made.result,
                               _parent,
                               _index,
                               _row)
                  : 0;
    made.named  = resolve(made.result);
    judge(made);

    return made;
}

trial
try_insert(
    const sheet&           _sheet,
    int                    _parent,
    int                    _index,
    const row&             _row,
    const colour_settings& _look
)
{
    trial made;

    made.result = _sheet;
    made.added  = holds_rows(_sheet,
                             _parent)
                  ? insert_row(made.result,
                               _parent,
                               _index,
                               _row)
                  : 0;
    made.named  = resolve(made.result,
                          _look);
    judge(made);

    return made;
}

/*
try_change
  The copy, the row's fields replaced in it -- its id and its rows kept --
the copy resolved and the row judged.
*/
trial
try_change(
    const sheet& _sheet,
    int          _id,
    const row&   _row
)
{
    trial made;

    made.result = _sheet;

    row* const target = (_id != 0) ? find_row(made.result,
                                              _id)
                                   : nullptr;

    // no such row: the sheet's own rows are not one
    if (target == nullptr)
    {
        made.named   = resolve(made.result);
        made.refused = "there is no such row to change";

        return made;
    }

    const std::string name = target->name;

    // a group stays a group, and a symbol a symbol
    if (_row.kind != target->kind)
    {
        made.named   = resolve(made.result);
        made.refused = name + " is a " +
                       ( (target->kind == row_kind::group) ? "group"
                                                           : "symbol" ) +
                       ", and stays one";

        return made;
    }

    // the rows in it would be in nothing
    if ( (!target->children.empty()) &&
         (!can_contain(_row)) )
    {
        made.named   = resolve(made.result);
        made.refused = name + " holds rows: it stays a range until they are "
                       "moved out of it or removed";

        return made;
    }

    std::vector<row> held = std::move(target->children);

    *target          = _row;
    target->id       = _id;
    target->children = std::move(held);
    made.added       = _id;
    made.named       = resolve(made.result);
    judge(made);

    return made;
}

/*
duplicate_rows
  Each copy is inserted beside its row -- insert_row puts a row given a
symbol's id just after it, and a group's at the end of it, as the window's
Duplicate always has.
*/
std::vector<int>
duplicate_rows(
    sheet&                  _sheet,
    const std::vector<int>& _ids
)
{
    std::vector<int> copies;

    for (int id : _ids)
    {
        const row* found = find_row(_sheet,
                                    id);

        // gone, or never there
        if (found == nullptr)
        {
            continue;
        }

        row copy = *found;

        copy.name = unique_name(_sheet,
                                found->name + "_COPY");
        copies.push_back(insert_row(_sheet,
                                    id,
                                    -1,
                                    copy));
    }

    return copies;
}

/*
remove_rows
  Each row, then the rules over them alone, then the special rows settled.
*/
int
remove_rows(
    sheet&                  _sheet,
    const std::vector<int>& _ids
)
{
    int removed = 0;

    for (int id : _ids)
    {
        removed += remove_row(_sheet,
                              id)
                   ? 1
                   : 0;
    }

    (void)prune_rules(_sheet);
    (void)settle_specials(_sheet);

    return removed;
}

/*
move_by
  A swap with the sibling `_step` away, when there is one.
*/
bool
move_by(
    sheet& _sheet,
    int    _id,
    int    _step
)
{
    row* const parent = parent_of(_sheet,
                                  _id);

    // no such row, or the root
    if (parent == nullptr)
    {
        return false;
    }

    for (std::size_t i = 0; i < parent->children.size(); ++i)
    {
        if (parent->children[i].id != _id)
        {
            continue;
        }

        const long target = static_cast<long>(i) + _step;

        // past either end
        if ( (target < 0) ||
             (target >= static_cast<long>(parent->children.size())) ||
             (_step == 0) )
        {
            return false;
        }

        std::swap(parent->children[i],
                  parent->children[static_cast<std::size_t>(target)]);

        return true;
    }

    return false;
}

/*
sort_rows
  The sheet as the window would show it sorted by the key, each row's place
there by its id, and every group's rows -- the sheet's own among them --
stably put in those places.
*/
int
sort_rows(
    sheet&   _sheet,
    sort_key _key,
    bool     _descending
)
{
    // the sheet's own order is the order already
    if (_key == sort_key::sheet)
    {
        return 0;
    }

    const std::map<int, std::size_t> place = shown_places(_sheet, _key,
                                                          _descending);

    // at
    //   where a row is shown; a row the resolution does not have, last.
    auto at = [&place](const row& _row) -> std::size_t
    {
        const auto found = place.find(_row.id);

        return (found == place.end()) ? place.size()
                                      : found->second;
    };

    int                       changed  = 0;
    std::function<void(row&)> in_order = [&](row& _row)
    {
        std::vector<int> before;

        for (const row& child : _row.children)
        {
            before.push_back(child.id);
        }

        std::stable_sort(_row.children.begin(),
                         _row.children.end(),
                         [&at](const row& _a,
                               const row& _b)
                         {
                             return (at(_a) < at(_b));
                         });

        for (std::size_t i = 0; i < _row.children.size(); ++i)
        {
            if (_row.children[i].id != before[i])
            {
                ++changed;

                break;
            }
        }

        for (row& child : _row.children)
        {
            in_order(child);
        }
    };

    in_order(_sheet.root);
    _sheet.grid.sort_by = sort_key::sheet;

    return changed;
}

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
