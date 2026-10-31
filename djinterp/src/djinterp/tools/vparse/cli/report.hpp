/*******************************************************************************
* vparse [cli]                                                        report.hpp
*
* What the command line prints, in its two formats: aligned text for a
* reader, JSON for a program.
*   The text format is the model's own: a run's parts are its listings as
* listing_text() lays them out, the same text a saved run's part files hold,
* so what a terminal shows and what a run folder keeps can be compared with
* any diff. The JSON format carries the same listings as data -- columns,
* rows, the note an empty listing has -- beside the run's outcome as fields,
* so a script reads a run without parsing a table.
*   JSON TEXT IS UTF-8. Every string is written as UTF-8 with JSON's escapes;
* a byte that does not belong to a UTF-8 sequence -- a grammar or an input
* can hold any byte -- is written as the visible escape `\xHH` the listings
* already use for control characters, since JSON has no way to carry a raw
* byte, and a document with one in it would not parse.
*
* path:      /src/djinterp/tools/vparse/cli/report.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  JSON
    ----
    1.  Values
         1.  json_quote
         2.  json_strings
    2.  Documents
         1.  json_object
         2.  json_array
    3.  Listings
         1.  json_listing
2.  TEXT
    ----
    1.  Parts
         1.  parts_text
    2.  Names
         1.  clipped
*/

#ifndef VPARSE_CLI_REPORT_HPP
#define VPARSE_CLI_REPORT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as
// the model it reports on is.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <utility>  // std::pair
#include <vector>   // std::vector
// vparse
#include "../../../../../inc/djinterp/tools/vparse/run.hpp"  // listing,
                                                             // run_record


namespace vparse
{
namespace cli
{


//==============================================================================
// 1.  JSON
//==============================================================================


// 1.1    Values
//------------------------------------------------------------------------------
// 1.1.1
// json_quote
//   function: `_text` as a JSON string, quotes included: `"` and `\` escaped,
// control characters as JSON's escapes, UTF-8 kept, and any other byte as
// the text `\xHH` (see the banner). Does not fail.
std::string json_quote(
    const std::string& _text
);

// 1.1.2
// json_strings
//   function: a JSON array of strings on one line, `["a", "b"]`; `[]` when
// there are none.
std::string json_strings(
    const std::vector<std::string>& _texts
);

// 1.2    Documents
//------------------------------------------------------------------------------
// 1.2.1
// json_object
//   class: a JSON object, one member a line, built member by member from
// values that are already JSON. A value that spans lines -- an object or an
// array made with the indent of the member it becomes -- nests as it is.
class json_object
{
public:
    // add
    //   function: appends a member whose value is JSON text, which is kept
    // as it is, and returns the object for the next member.
    json_object& add(
        const std::string& _key,
        const std::string& _json
    );

    // add_all
    //   function: appends every member of another object, in its order,
    // and returns this object for the next member.
    json_object& add_all(
        const json_object& _other
    );

    // add_string / add_bool / add_count
    //   functions: append a member whose value is a string, quoted as
    // json_quote quotes it; a truth value; or a count, which is never
    // negative.
    json_object& add_string(
        const std::string& _key,
        const std::string& _text
    );
    json_object& add_bool(
        const std::string& _key,
        bool               _value
    );
    json_object& add_count(
        const std::string& _key,
        std::size_t        _count
    );

    // text
    //   function: the object, `{` on its first line, each member on a line
    // of its own `_indent + 2` columns in, and `}` `_indent` columns in; `{}`
    // when it has no member.
    std::string text(
        std::size_t _indent
    ) const;

    // line
    //   function: the object on one line, its members separated by a comma
    // and a space: for the records of a list, a record a line.
    std::string line() const;

private:
    std::vector<std::pair<std::string, std::string>> m_members;
};

// 1.2.2
// json_array
//   class: a JSON array, one element a line, built from values that are
// already JSON, as json_object's members are.
class json_array
{
public:
    // add
    //   function: appends an element, which is kept as it is.
    json_array& add(
        const std::string& _json
    );

    // text
    //   function: the array, `[` on its first line, each element on a line of
    // its own `_indent + 2` columns in, and `]` `_indent` columns in; `[]`
    // when it has no element.
    std::string text(
        std::size_t _indent
    ) const;

    // empty
    //   function: whether the array has no element.
    bool empty() const;

private:
    std::vector<std::string> m_elements;
};

// 1.3    Listings
//------------------------------------------------------------------------------
// 1.3.1
// json_listing
//   function: one part of a run as a JSON object -- its title, its columns,
// its rows (one row a line, each an array of cells), and its note -- laid
// out for a member `_indent` columns in.
std::string json_listing(
    const workbench::listing& _listing,
    const std::string&        _title,
    std::size_t               _indent
);


//==============================================================================
// 2.  TEXT
//==============================================================================


// 2.1    Parts
//------------------------------------------------------------------------------
// 2.1.1
// parts_text
//   function: parts of a run as listing_text() lays them out, in the order
// given. One part is its listing alone -- what a saved run's part file holds
// -- and several are each under their title, underlined, with a blank line
// between one and the next.
std::string parts_text(
    const workbench::run_record&    _run,
    const std::vector<std::string>& _parts
);

// 2.2    Names
//------------------------------------------------------------------------------
// 2.2.1
// clipped
//   function: `_text` cut to at most `_most` bytes, the last three of them
// "..." when it was cut, and never inside a UTF-8 character. `_most` is at
// least 3.
std::string clipped(
    const std::string& _text,
    std::size_t        _most
);

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_CLI_REPORT_HPP
