/*******************************************************************************
* bitwisedom [bitwisedom]                                           cli_rows.cpp
*
* The commands that rearrange a sheet's rows or change the rest of it:
* duplicate, remove, rename, move, sort, set, get and op.
*   Each reads its sheet, finds every row it names before it changes
* anything -- a name no row has stops it, nothing written -- makes the
* change with the model's own edits, the window's, and stores the sheet as
* every changing command does (cli_internal.hpp): a change that would give
* the sheet an error it does not have is refused unless forced.
*   op previews an operation as a table, its result; with --apply the sheet
* is the result, and the table is said about the work, to standard error,
* with -v or -n.
*
* path:      /src/djinterp/tools/bitwisedom/cli/cli_rows.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
#include "./cli_internal.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::find
#include <cctype>     // std::isalnum, std::tolower
#include <cstddef>    // std::size_t
#include <memory>     // std::make_shared
#include <string>     // std::string
#include <vector>     // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>      // format_hex
#include <djinterp/tools/bitwisedom/edit.hpp>      // duplicate_rows
#include <djinterp/tools/bitwisedom/json.hpp>      // json_value
#include <djinterp/tools/bitwisedom/ops.hpp>       // preview_op, apply_op_to
#include <djinterp/tools/bitwisedom/rename.hpp>    // rename_row
#include <djinterp/tools/bitwisedom/settings.hpp>  // sheet_settings


namespace bitwisedom
{
namespace cmd
{

namespace
{

// SORT_SETTING
//   constant: the setting whose words name the keys rows sort by.
const char* const SORT_SETTING = "grid.sort_by";

/*
rows_named
  Every ROW operand's row, by id, each once, in the order named; false, the
usage error said, at the first that names none.
*/
bool
rows_named(
    kit::program&                   _program,
    kit::command                    _command,
    const resolution&               _resolution,
    const std::vector<std::string>& _names,
    std::vector<int>&               _ids,
    std::string&                    _said
)
{
    for (const std::string& each : _names)
    {
        const int id = row_named(_program, _command, _resolution, each,
                                 std::string());

        if (id == 0)
        {
            return false;
        }

        // named twice, counted once
        if (std::find(_ids.begin(), _ids.end(), id) != _ids.end())
        {
            continue;
        }

        _ids.push_back(id);
        _said += (_said.empty() ? "" : ", ") +
                 shown_name(*_resolution.find(id));
    }

    return true;
}

// rows_request
//   struct: what duplicate and remove were asked: the sheet and its rows.
struct rows_request
{
    std::string              sheet;
    std::vector<std::string> rows;
    destination              where;
};


//------------------------------------------------------------------------------
// duplicate, remove
//------------------------------------------------------------------------------

/*
run_duplicate
  Every row found first, then each copied as the window's Duplicate copies
it: just after it -- a group's or a range's copy last in it -- named as it
is with _COPY after, made unique.
*/
int
run_duplicate(
    kit::program&       _program,
    kit::command        _command,
    const rows_request& _request
)
{
    source read;

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    std::vector<int> ids;
    std::string      said;

    if (!rows_named(_program, _command, before, _request.rows, ids, said))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const std::vector<int> copies = duplicate_rows(read.doc, ids);
    const resolution       after  = resolve(read.doc);
    std::string            named;

    for (const int each : copies)
    {
        const resolved_row* copy = after.find(each);

        named += (named.empty() ? "" : ", ") +
                 ((copy != nullptr) ? shown_name(*copy) : std::string("?"));
    }

    return store(_program, _command, read, _request.where,
                 said + " duplicated as " + named, before);
}

/*
run_remove
  Every row found first -- one that is not stops it, nothing removed -- then
all of them, and everything in them.
*/
int
run_remove(
    kit::program&       _program,
    kit::command        _command,
    const rows_request& _request
)
{
    source read;

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    std::vector<int> ids;
    std::string      said;

    if (!rows_named(_program, _command, before, _request.rows, ids, said))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    (void)remove_rows(read.doc, ids);

    return store(_program, _command, read, _request.where,
                 said + " removed", before);
}


//------------------------------------------------------------------------------
// rename
//------------------------------------------------------------------------------

// rename_request
//   struct: what `rename` was asked.
struct rename_request
{
    std::string sheet;
    std::string row;
    std::string name;
    destination where;
};

/*
run_rename
  The model's rename, which rewrites every reference, or says why not.
*/
int
run_rename(
    kit::program&         _program,
    kit::command          _command,
    const rename_request& _request
)
{
    source read;

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    const int        id     = row_named(_program, _command, before,
                                        _request.row, std::string());

    if (id == 0)
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const std::string old_name = shown_name(*before.find(id));
    const renamed     done     = rename_row(read.doc, id, _request.name);

    if (!done.ok)
    {
        _program.report(kit::severity::error, done.error);

        return D_CLI_EXIT_TROUBLE;
    }

    const resolved_row* now = resolve(read.doc).find(id);

    return store(_program, _command, read, _request.where,
                 old_name + " renamed " +
                 ((now != nullptr) ? shown_name(*now) : _request.name) +
                 "; " + plural(done.references, "reference") +
                 " rewritten",
                 before);
}


//------------------------------------------------------------------------------
// move
//------------------------------------------------------------------------------

// move_request
//   struct: what `move` was asked: the row, and where -- one of the five.
struct move_request
{
    std::string sheet;
    std::string row;
    bool        up   = false;
    bool        down = false;
    std::string before;
    std::string after;
    std::string into;
    destination where;
};

/*
place_in
  A row's group and its place among the group's rows.
*/
bool
place_in(
    sheet& _sheet,
    int    _id,
    int&   _parent,
    int&   _index
)
{
    const row* parent = parent_of(_sheet, _id);

    for (std::size_t i = 0;
         ( (parent != nullptr) && (i < parent->children.size()) );
         ++i)
    {
        if (parent->children[i].id == _id)
        {
            _parent = parent->id;
            _index  = static_cast<int>(i);

            return true;
        }
    }

    return false;
}

/*
move_beside
  Before or after another row, among its group's rows, or last in a group:
as the window's tree drops a row. Returns where it went, as a note says it;
"" -- the error said -- when it could not go there.
*/
std::string
move_beside(
    kit::program&       _program,
    kit::command        _command,
    const move_request& _request,
    const resolution&   _before,
    int                 _id,
    sheet&              _sheet
)
{
    const bool         into   = (!_request.into.empty());
    const bool         after  = (!_request.after.empty());
    const std::string& other  = into ? _request.into
                              : after ? _request.after
                                      : _request.before;
    const int          target = row_named(_program, _command, _before, other,
                                          into ? "--into"
                                          : after ? "--after"
                                                  : "--before");
    int                parent = target;
    int                index  = -1;

    if (target == 0)
    {
        return std::string();
    }

    // beside a row: in its group, before or after it
    if ( (!into) &&
         place_in(_sheet, target, parent, index) )
    {
        index += after ? 1 : 0;
    }

    if (!move_row(_sheet, _id, parent, index))
    {
        _program.report(kit::severity::error,
                        shown_name(*_before.find(_id)) + " cannot go there: " +
                        (into ? other + " holds no rows, or is it or in it"
                              : std::string("not beside itself, nor in "
                                            "itself")));

        return std::string();
    }

    return std::string(into ? "into " : after ? "after " : "before ") +
           other;
}

/*
run_move
  One step up or down among its group's rows, or beside another row, or
into a group.
*/
int
run_move(
    kit::program&       _program,
    kit::command        _command,
    const move_request& _request
)
{
    const int asked = (_request.up ? 1 : 0) + (_request.down ? 1 : 0) +
                      (_request.before.empty() ? 0 : 1) +
                      (_request.after.empty() ? 0 : 1) +
                      (_request.into.empty() ? 0 : 1);
    source    read;

    if (asked != 1)
    {
        return _program.usage_error(_command,
                                    "say where, once: --up, --down, "
                                    "--before, --after or --into");
    }

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    const int        id     = row_named(_program, _command, before,
                                        _request.row, std::string());
    const bool       step   = (_request.up || _request.down);

    if (id == 0)
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const std::string name  = shown_name(*before.find(id));
    const bool        moved = step && move_by(read.doc, id,
                                              _request.up ? -1 : 1);
    const std::string where = step ? (_request.up ? "up" : "down")
                                   : move_beside(_program, _command,
                                                 _request, before, id,
                                                 read.doc);

    if (step && (!moved))
    {
        _program.report(kit::severity::error,
                        name + " is already " +
                        (_request.up ? "first" : "last") +
                        " among its group's rows");
    }

    return ( (step && (!moved)) || where.empty() )
           ? D_CLI_EXIT_TROUBLE
           : store(_program, _command, read, _request.where,
                   name + " moved " + where, before);
}


//------------------------------------------------------------------------------
// sort
//------------------------------------------------------------------------------

// sort_request
//   struct: what `sort` was asked.
struct sort_request
{
    std::string sheet;
    std::string key;
    bool        descending = false;
    destination where;
};

/*
sort_words
  The keys rows sort by, as the setting names them, less the sheet's own
order, which is no sort.
*/
std::vector<std::string>
sort_words()
{
    const sheet_setting*     setting = find_sheet_setting(SORT_SETTING);
    std::vector<std::string> out;

    for (std::size_t i = 1;
         ( (setting != nullptr) && (i < setting->words.size()) );
         ++i)
    {
        out.push_back(setting->words[i]);
    }

    return out;
}

/*
run_sort
  The key given -- or, without one, the order the window shows the rows
in, and its way round -- and the model's sort_rows.
*/
int
run_sort(
    kit::program&       _program,
    kit::command        _command,
    const sort_request& _request
)
{
    const std::vector<std::string> words = sort_words();
    const int chosen = _request.key.empty() ? -1
                                            : choice_of(_request.key, words);
    source    read;

    if ( (!_request.key.empty()) &&
         (chosen < 0) )
    {
        return bad_word(_program, _command, "KEY", _request.key, words);
    }

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const sort_key key = (chosen >= 0) ? static_cast<sort_key>(chosen + 1)
                                       : read.doc.grid.sort_by;
    const bool     down = (chosen >= 0)
                          ? _request.descending
                          : (_request.descending ||
                             read.doc.grid.sort_descending);

    if (key == sort_key::sheet)
    {
        return _program.usage_error(_command,
                                    "the sheet shows its rows in its own "
                                    "order: name a KEY (" +
                                    word_list(words) + ")");
    }

    const resolution before = resolve(read.doc);
    const int        groups = sort_rows(read.doc, key, down);

    return store(_program, _command, read, _request.where,
                 "rows sorted by " + words[static_cast<std::size_t>(key) -
                                           1u] +
                 (down ? ", descending" : "") +
                 ((groups == 0) ? " -- they were in that order already"
                                : ""),
                 before);
}


//------------------------------------------------------------------------------
// set, get
//------------------------------------------------------------------------------

// set_request / get_request
//   structs: what `set` and `get` were asked.
struct set_request
{
    std::string              sheet;
    std::vector<std::string> settings;
    destination              where;
};

struct get_request
{
    std::string              sheet;
    std::vector<std::string> keys;
    std::string              format = "text";
};

/*
unknown_setting
  The usage error of a path no setting has, with the nearest that one has.
*/
int
unknown_setting(
    kit::program&      _program,
    kit::command       _command,
    const std::string& _path
)
{
    std::vector<std::string> paths;

    for (const sheet_setting& each : sheet_settings())
    {
        paths.push_back(each.path);
    }

    const std::string near = nearest(_path, paths);

    return _program.usage_error(_command,
                                "'" + _path + "' is not a setting" +
                                (near.empty()
                                     ? std::string("; 'bitwisedom get' "
                                                   "lists them")
                                     : "; did you mean '" + near + "'?"));
}

/*
set_one
  One KEY=VALUE applied to the sheet in memory, and said; a usage error for
anything else.
*/
int
set_one(
    kit::program&      _program,
    kit::command       _command,
    const std::string& _setting,
    sheet&             _sheet,
    std::string&       _said
)
{
    const std::size_t equals = _setting.find('=');

    if ( (equals == std::string::npos) ||
         (equals == 0u) )
    {
        return _program.usage_error(_command,
                                    "'" + _setting + "' is not KEY=VALUE");
    }

    const std::string    path  = _setting.substr(0, equals);
    const sheet_setting* found = find_sheet_setting(path);
    std::string          error;

    if (found == nullptr)
    {
        return unknown_setting(_program, _command, path);
    }

    const std::string value = _setting.substr(equals + 1u);

    // a choice's word: the words it takes, and the one meant
    if ( (!found->words.empty()) &&
         (choice_of(value, found->words) < 0) )
    {
        return bad_word(_program, _command, path, value, found->words);
    }

    if (!set_setting(_sheet, *found, value, error))
    {
        return _program.usage_error(_command, path + ": " + error);
    }

    _said += (_said.empty() ? "" : ", ") + path + "=" +
             setting_text(*found, _sheet);

    return D_CLI_EXIT_OK;
}

/*
run_set
  Each KEY=VALUE applied in turn -- the first mistake stops it, nothing
written -- then the sheet stored.
*/
int
run_set(
    kit::program&      _program,
    kit::command       _command,
    const set_request& _request
)
{
    source read;

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    std::string      said;
    int              status = D_CLI_EXIT_OK;

    for (std::size_t i = 0;
         ( (status == D_CLI_EXIT_OK) && (i < _request.settings.size()) );
         ++i)
    {
        status = set_one(_program, _command, _request.settings[i], read.doc,
                         said);
    }

    return (status != D_CLI_EXIT_OK)
           ? status
           : store(_program, _command, read, _request.where, "set " + said,
                   before);
}

/*
settings_asked
  The settings the KEYs name, every one known before anything is read --
every setting, without a KEY; false, the usage error said, at a KEY no
setting has.
*/
bool
settings_asked(
    kit::program&                      _program,
    kit::command                       _command,
    const std::vector<std::string>&    _keys,
    std::vector<const sheet_setting*>& _asked,
    int&                               _status
)
{
    for (const std::string& each : _keys)
    {
        const sheet_setting* found = find_sheet_setting(each);

        if (found == nullptr)
        {
            _status = unknown_setting(_program, _command, each);

            return false;
        }

        _asked.push_back(found);
    }

    for (std::size_t i = 0;
         ( _keys.empty() && (i < sheet_settings().size()) );
         ++i)
    {
        _asked.push_back(&sheet_settings()[i]);
    }

    return true;
}

/*
run_get
  The settings asked for as KEY=VALUE lines; one KEY alone, its value alone,
for a script to take; or as JSON.
*/
int
run_get(
    kit::program&      _program,
    kit::command       _command,
    const get_request& _request
)
{
    bool                              ok     = false;
    const bool                        json   = json_wanted(_program,
                                                           _command,
                                                           _request.format,
                                                           ok);
    int                               status = D_CLI_EXIT_TROUBLE;
    std::vector<const sheet_setting*> asked;
    source                            read;

    if ( (!ok) ||
         (!settings_asked(_program, _command, _request.keys, asked,
                          status)) ||
         (!read_source(_program, _request.sheet, read)) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    json_value  made = json_value::object();
    std::string text;

    for (const sheet_setting* each : asked)
    {
        const std::string value = setting_text(*each, read.doc);

        made.set(each->path, json_value::string(value));
        text += (_request.keys.size() == 1u)
                ? (value + "\n")
                : (each->path + "=" + value + "\n");
    }

    emit(_program, json ? write_json(made) : text);

    return D_CLI_EXIT_OK;
}


//------------------------------------------------------------------------------
// op
//------------------------------------------------------------------------------

// op_request
//   struct: what `op` was asked.
struct op_request
{
    std::string              sheet;
    std::string              operation;
    std::string              operand;
    std::vector<std::string> rows;
    bool                     probe = false;
    bool                     apply = false;
    destination              where;
};

// word_of
//   a name as a word: lower case, each run of anything else one `_`.
std::string
word_of(
    const char* _name
)
{
    std::string word;
    bool        gap = false;

    for (const char* at = _name; *at != '\0'; ++at)
    {
        const unsigned char c = static_cast<unsigned char>(*at);

        if (std::isalnum(c) == 0)
        {
            gap = true;

            continue;
        }

        word += (gap && (!word.empty())) ? "_" : "";
        word += static_cast<char>(std::tolower(c));
        gap   = false;
    }

    return word;
}

// op_words
//   the operations' words, in op_kind's order: shift_left ... toggle_bit.
std::vector<std::string>
op_words()
{
    std::vector<std::string> out;

    for (int i = 0; i < OP_COUNT; ++i)
    {
        out.push_back(word_of(describe(static_cast<op_kind>(i)).name));
    }

    return out;
}

/*
op_operation
  The operation by its word, and its operand, read as the grammar writes an
integer where it takes one; a usage error for a word it is not, an operand
it lacks, or one it does not take.
*/
int
op_operation(
    kit::program&     _program,
    kit::command      _command,
    const op_request& _request,
    op_kind&          _op,
    wide&             _operand
)
{
    const std::vector<std::string> words    = op_words();
    const int                      chosen   = choice_of(_request.operation,
                                                        words);
    bool                           negative = false;
    std::string                    error;

    if (chosen < 0)
    {
        return bad_word(_program, _command, "OPERATION", _request.operation,
                        words);
    }

    _op = static_cast<op_kind>(chosen);

    const op_info info  = describe(_op);
    const bool    takes = (info.operand[0] != '\0');

    if (takes == _request.operand.empty())
    {
        return _program.usage_error(_command,
                                    _request.operation +
                                    (takes ? " takes an OPERAND: a " +
                                             std::string(info.operand)
                                           : std::string(" takes no "
                                                         "OPERAND")));
    }

    return ( (!takes) ||
             read_value(_request.operand, _operand, negative, error) )
           ? D_CLI_EXIT_OK
           : _program.usage_error(_command, "OPERAND: " + error);
}

/*
op_table
  Each row's bits before and after, what it lost and gained, and how its
definition is rewritten; the probe's, asked for, last.
*/
std::string
op_table(
    const op_preview& _preview,
    const resolution& _resolution,
    int               _width,
    bool              _probe
)
{
    const int                             digits = (_width + 3) / 4;
    std::vector<std::vector<std::string>> cells;

    for (const op_row& each : _preview.rows)
    {
        const resolved_row* found = _resolution.find(each.id);
        const std::string   lost  = (each.lost > 0)
                                    ? "lost " + std::to_string(each.lost)
                                    : std::string();
        const std::string   added = (each.gained > 0)
                                    ? "gained " + std::to_string(each.gained)
                                    : std::string();

        cells.push_back({ (found != nullptr) ? shown_name(*found)
                                             : std::string("?"),
                          "0x" + format_hex(each.before, digits, true),
                          "->",
                          "0x" + format_hex(each.after, digits, true),
                          lost + ((lost.empty() || added.empty()) ? ""
                                                                  : ", ") +
                          added,
                          each.rewrite });
    }

    if (_probe)
    {
        cells.push_back({ "(probe)",
                          "0x" + format_hex(_preview.probe_before, digits,
                                            true),
                          "->",
                          "0x" + format_hex(_preview.probe_after, digits,
                                            true),
                          "",
                          "" });
    }

    return table(cells);
}

/*
run_op
  The operation previewed over the rows -- the whole sheet when none are
named -- as a table; with --apply, each target's definition rewritten and
the sheet stored, the table said about it.
*/
int
run_op(
    kit::program&     _program,
    kit::command      _command,
    const op_request& _request
)
{
    op_kind   op = op_kind::shift_left;
    wide      operand;
    source    read;
    const int status = op_operation(_program, _command, _request, op,
                                    operand);

    if ( (status != D_CLI_EXIT_OK) ||
         (!read_source(_program, _request.sheet, read)) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    std::vector<int> targets;
    std::string      said;

    if (!rows_named(_program, _command, before, _request.rows, targets,
                    said))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    // the whole sheet
    targets = targets.empty() ? std::vector<int>(1u, 0) : targets;

    const std::string shown = op_table(preview_op(read.doc, before, targets,
                                                  op, operand,
                                                  _request.probe),
                                       before, read.doc.width,
                                       _request.probe);

    if (!_request.apply)
    {
        emit(_program, shown);

        return D_CLI_EXIT_OK;
    }

    // the sheet is the result; the table is said about it
    if ( _request.where.dry_run ||
         (_program.verbosity() > 0) )
    {
        say(_program, shown);
    }

    const int changed = apply_op_to(read.doc, before, targets, op, operand,
                                    _request.probe);

    return store(_program, _command, read, _request.where,
                 std::string(describe(op).name) + " applied to " +
                 plural(changed, "row"),
                 before);
}


//------------------------------------------------------------------------------
// the commands
//------------------------------------------------------------------------------

// add_rows
//   definition: the ROW operands of duplicate and remove.
void
add_rows(
    kit::command              _command,
    std::vector<std::string>& _rows
)
{
    _command.add_option("ROW",
                        _rows,
                        "a row: its macro name, its own name where one row "
                        "has it, or a group's name")
            .required();

    return;
}

/*
define_duplicate_remove
  duplicate and remove: rows named, copied or taken out.
*/
void
define_duplicate_remove(
    kit::program& _program
)
{
    const auto   copies = std::make_shared<rows_request>();
    kit::command copy   = _program.add_command(
        "duplicate",
        "copy rows, each just after itself, named as it is with _COPY after",
        [copies](kit::program& _p, kit::command _c)
        {
            return run_duplicate(_p, _c, *copies);
        });

    copy.group(CHANGING);
    add_sheet(copy, copies->sheet);
    add_rows(copy, copies->rows);
    add_destination(copy, copies->where);

    const auto   gone   = std::make_shared<rows_request>();
    kit::command remove = _program.add_command(
        "remove",
        "remove rows, and everything in them",
        [gone](kit::program& _p, kit::command _c)
        {
            return run_remove(_p, _c, *gone);
        });

    remove.group(CHANGING);
    add_sheet(remove, gone->sheet);
    add_rows(remove, gone->rows);
    add_destination(remove, gone->where);

    return;
}

/*
define_rename
  rename: a row's own name, every reference following.
*/
void
define_rename(
    kit::program& _program
)
{
    const auto   request = std::make_shared<rename_request>();
    kit::command made    = _program.add_command(
        "rename",
        "rename a row; every expression that names it follows",
        [request](kit::program& _p, kit::command _c)
        {
            return run_rename(_p, _c, *request);
        });

    made.group(CHANGING);
    add_sheet(made, request->sheet);
    made.add_option("ROW", request->row, "the row, as remove names one")
        .required();
    made.add_option("NAME", request->name,
                    "its new name, its own, without the affixes")
        .required();
    add_destination(made, request->where);

    return;
}

/*
define_move
  move: one of five places.
*/
void
define_move(
    kit::program& _program
)
{
    const auto   request = std::make_shared<move_request>();
    kit::command made    = _program.add_command(
        "move",
        "move a row: up, down, beside another, or into a group",
        [request](kit::program& _p, kit::command _c)
        {
            return run_move(_p, _c, *request);
        });

    made.group(CHANGING);
    add_sheet(made, request->sheet);
    made.add_option("ROW", request->row, "the row, as remove names one")
        .required();
    made.add_flag("--up", request->up, "one place up among its group's rows")
        .group("Where");
    made.add_flag("--down", request->down,
                  "one place down among its group's rows")
        .group("Where");
    made.add_option("--before", request->before, "just before another row")
        .type_name("ROW")
        .group("Where");
    made.add_option("--after", request->after, "just after another row")
        .type_name("ROW")
        .group("Where");
    made.add_option("--into", request->into, "last in a group or a range")
        .type_name("ROW")
        .group("Where");
    add_destination(made, request->where);

    return;
}

/*
define_sort
  sort: the order rows are shown in, made the sheet's own.
*/
void
define_sort(
    kit::program& _program
)
{
    const auto   request = std::make_shared<sort_request>();
    kit::command made    = _program.add_command(
        "sort",
        "put each group's rows in order, for the header to follow",
        [request](kit::program& _p, kit::command _c)
        {
            return run_sort(_p, _c, *request);
        });

    made.group(CHANGING);
    add_sheet(made, request->sheet);
    made.add_option("KEY", request->key,
                    "what to sort by: " + word_list(sort_words()) +
                    "; groups first among their rows (default: the order "
                    "the window shows them in, " + SORT_SETTING + ")");
    made.add_flag("--descending", request->descending,
                  "from the last to the first: Z to A, the largest first");
    add_destination(made, request->where);

    return;
}

/*
define_set_get
  set and get: the settings, by the paths the window's Options show.
*/
void
define_set_get(
    kit::program& _program
)
{
    const auto   sets = std::make_shared<set_request>();
    kit::command set  = _program.add_command(
        "set",
        "change settings, by the paths the window's Options show",
        [sets](kit::program& _p, kit::command _c)
        {
            return run_set(_p, _c, *sets);
        });

    set.group(CHANGING);
    add_sheet(set, sets->sheet);
    set.add_option("SETTING", sets->settings,
                   "KEY=VALUE: gen.level=c11, grid.lines=false")
       .required();
    add_destination(set, sets->where);

    const auto   gets = std::make_shared<get_request>();
    kit::command get  = _program.add_command(
        "get",
        "print settings; every one, without a KEY",
        [gets](kit::program& _p, kit::command _c)
        {
            return run_get(_p, _c, *gets);
        });

    get.group(CHANGING);
    get.add_option("SHEET", gets->sheet, "the sheet, as gen reads one")
       .required();
    get.add_option("KEY", gets->keys,
                   "a setting's path; one KEY prints its value alone");
    add_format(get, gets->format);

    return;
}

/*
define_op
  op: an operation over rows, previewed or applied.
*/
void
define_op(
    kit::program& _program
)
{
    const auto   request = std::make_shared<op_request>();
    kit::command made    = _program.add_command(
        "op",
        "preview an operation over rows -- a shift, a rotation, a byte "
        "swap, arithmetic; --apply applies it",
        [request](kit::program& _p, kit::command _c)
        {
            return run_op(_p, _c, *request);
        });

    made.group(CHANGING);
    add_sheet(made, request->sheet);
    made.add_option("OPERATION", request->operation, word_list(op_words()))
        .required();
    made.add_option("OPERAND", request->operand,
                    "the count, value or bit the operation takes");
    made.add_option("--row", request->rows,
                    "a row to operate on, a group standing for every symbol "
                    "in it; the whole sheet when none is named")
        .type_name("ROW");
    made.add_flag("--probe", request->probe, "the probe value too");
    made.add_flag("--apply", request->apply,
                  "rewrite each row's definition, and write the sheet back; "
                  "the table goes to standard error, with -v or -n");
    add_destination(made, request->where);

    return;
}

}  // anonymous namespace


/*
add_row_commands
  duplicate, remove, rename, move, sort, set, get and op, in the order the
grammar lists them.
*/
void
add_row_commands(
    kit::program& _program
)
{
    define_duplicate_remove(_program);
    define_rename(_program);
    define_move(_program);
    define_sort(_program);
    define_set_get(_program);
    define_op(_program);

    return;
}

}  // namespace cmd
}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
