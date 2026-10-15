/*******************************************************************************
* bitwisedom [bitwisedom]                                               json.hpp
*
* A small JSON value, reader and writer -- enough for a sheet file.
*   Numbers are kept as the text they were read from, not as doubles: a
* sheet holds 64-bit values, and a double holds 53 bits of them. Objects keep
* their members in the order they were written, so a saved sheet reads the
* same way twice and a diff between two saves is a diff of what changed.
*   The reader takes RFC 8259 JSON. It is not a validator of the sheet: that
* is persist.hpp's job, on the value this produces.
*
* path:      /inc/djinterp/tools/bitwisedom/json.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE VALUE
    ---------
    1.  json_value, json_member
2.  READING AND WRITING
    -------------------
    1.  parse_json, write_json
*/

#ifndef BITWISEDOM_JSON_HPP
#define BITWISEDOM_JSON_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <vector>       // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./bits.hpp"  // u64, i64


namespace bitwisedom
{


//==============================================================================
// 1.  THE VALUE
//==============================================================================


// 1.1    json_value
//------------------------------------------------------------------------------
// 1.1.1
// json_type
//   enum: the six kinds of JSON value.
enum class json_type : int
{
    null = 0,
    boolean,
    number,
    string,
    array,
    object
};

// 1.1.2
// json_member
//   struct: one member of an object, declared ahead so json_value can hold
// a vector of them; defined below it.
struct json_member;

// 1.1.3
// json_value
//   struct: one JSON value. `text` is a string's contents or a number's
// spelling; `items` an array's elements; `members` an object's, in order.
struct json_value
{
    json_type                type = json_type::null;
    bool                     flag = false;
    std::string              text;
    std::vector<json_value>  items;
    std::vector<json_member> members;

    // makers -- one per kind
    D_NODISCARD static json_value boolean(bool _flag);
    D_NODISCARD static json_value number(i64 _number);
    D_NODISCARD static json_value real(double _number);
    D_NODISCARD static json_value string(const std::string& _text);
    D_NODISCARD static json_value array();
    D_NODISCARD static json_value object();

    // members -- find returns nullptr for a missing key or a non-object
    D_NODISCARD const json_value* find(const std::string& _key) const;
    void        set(const std::string& _key,
                    json_value         _value);
    void        push(json_value _value);

    // readers -- each returns _fallback for the wrong kind or a bad number
    D_NODISCARD bool         as_bool(bool _fallback) const;
    D_NODISCARD i64          as_int(i64 _fallback) const;
    D_NODISCARD double       as_real(double _fallback) const;
    D_NODISCARD std::string  as_string(const std::string& _fallback) const;
};

// 1.1.4
// json_member
//   struct: a member's name and its value.
struct json_member
{
    std::string key;
    json_value  value;
};


//==============================================================================
// 2.  READING AND WRITING
//==============================================================================


// 2.1    parse_json, write_json
//------------------------------------------------------------------------------
/**
 * @brief Reads one JSON value from text.
 *
 * @param[in]  _text   the text; blanks around the value are allowed,
 *                     anything else after it is an error.
 * @param[out] _value  the value read, when the text is JSON.
 * @param[out] _error  what is wrong, with its line and column, when not.
 * @return `true` if the text was one JSON value.
 */
D_NODISCARD bool         parse_json(std::string_view _text,
                                    json_value&      _value,
                                    std::string&     _error);
D_NODISCARD std::string  write_json(const json_value& _value);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_JSON_HPP
