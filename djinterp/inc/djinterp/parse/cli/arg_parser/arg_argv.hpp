/*******************************************************************************
* djinterp [parse]                                                  arg_argv.hpp
*
* The argument parser's C++ face: arguments from text.
*   The C library's splitting, quoting and conversion (arg_argv.h), with the
* standard library's strings: a line split into a std::vector of arguments by
* one of the d_arg_quoting rules, a word quoted so that it splits back
* whole, a program's name found at the start of a line -- CLI11's
* detail::split_program_name -- a response file's arguments read, and, for
* wmain, wide text made UTF-8, as CLI11's narrow() makes it, and UTF-8 made
* wide, as its widen() makes it.
*   The words a split gives are std::strings made from the C library's
* NUL-terminated copies, so a word holding a NUL byte ends there, as
* arg_argv.h says. Memory that runs out is std::bad_alloc, as the standard
* containers make it, when the strings are made, and a status the C library
* reports when it stores the words; either way, a function filling a vector
* leaves it as it was.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_argv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Split flags
         1.  split_flags
2.  SPLITTING
    ---------
    1.  Lines
         1.  split_line
         2.  quote_word
    2.  Program names
         1.  split_program_name
    3.  Response files
         1.  read_response_file
3.  ENCODING
    --------
    1.  Wide text
         1.  narrow
         2.  widen
4.  INTERNALS
    ---------
    1.  Lists
         1.  owned_strings
         2.  append_strings
5.  DEFINITIONS
    -----------
    1.  Splitting
    2.  Encoding
    3.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_ARGV_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_ARGV_HPP 1

// std
#include <cstddef>    // std::size_t
#include <cwchar>     // std::wcslen
#include <string>     // std::string, std::wstring
#include <utility>    // std::pair
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_text
#include "../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../env/env.h"             // language levels
#include "./arg_command.hpp"              // raise_out_of_memory
#include "./arg_common.hpp"               // status, text, quoting, scratch
#include "./arg_type.hpp"                 // widen_text
#include "./c/arg_argv.h"                 // d_arg_split_line and kin
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // uint32_t


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Split flags
//------------------------------------------------------------------------------
// 1.1.1
// split_flags
//   enum: the D_ARG_SPLIT_* switches split_line takes, combined with `|`:
// the line starts with a program's name; CLI11's rules for a command taking
// Windows-style options; line breaks separating words by the C runtime's
// rules.
struct split_flags
{
    enum value
    {
        none          = 0,
        program_name  = D_ARG_SPLIT_PROGRAM_NAME,
        windows_style = D_ARG_SPLIT_WINDOWS_STYLE,
        line_breaks   = D_ARG_SPLIT_LINE_BREAKS
    };
};


//==============================================================================
// 2.  SPLITTING
//==============================================================================


// 2.1    Lines
//------------------------------------------------------------------------------
// 2.1.1
// split_line
/**
 * @brief Splits a line into arguments by a set of rules;
 *        d_arg_split_line.
 *
 * @param[in]     _line     the text.
 * @param[in,out] _out      receives the words, appended; unchanged on any
 *                          failure.
 * @param[in]     _quoting  the rules; CLI11's, as its parse of a string
 *                          splits, by default.
 * @param[in]     _flags    split_flags values, combined; none by default.
 * @param[in,out] _error    receives a failure's description, appended;
 *                          NULL for none.
 * @return status_code::ok; invalid_error for text the rules refuse;
 *         out_of_memory when the C library could not store a word;
 *         bad_argument for rules that are not a quoting::value.
 *         std::bad_alloc, as the vector throws it, when the vector cannot
 *         hold the words, the vector then as it was.
 */
status split_line(const text&               _line,
                  std::vector<std::string>& _out,
                  quoting::value            _quoting = quoting::cli11,
                  re_std::uint32_t          _flags   = 0u,
                  std::string*              _error   = D_NULLPTR);

// 2.1.2
// quote_word
//   function: a word written so that split_line, by the same rules, gives it
// back as one argument; d_arg_quote_word. std::bad_alloc when it cannot be
// held; empty for rules that are not a quoting::value.
std::string quote_word(const text&    _word,
                       quoting::value _quoting = quoting::cli11);

// 2.2    Program names
//------------------------------------------------------------------------------
// 2.2.1
// split_program_name
//   function: CLI11's detail::split_program_name: the program's name at the
// start of a command line -- the shortest run of its first words naming
// something that exists, else a quoted or a first word -- and the rest of
// the line after it; d_arg_split_program_name. std::bad_alloc when either
// cannot be held.
std::pair<std::string, std::string> split_program_name(const text& _line);

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 2.3    Response files
//------------------------------------------------------------------------------
// 2.3.1
// read_response_file
/**
 * @brief Reads the arguments a response file holds;
 *        d_arg_read_response_file.
 *
 * @param[in]     _path     the file's path.
 * @param[in,out] _out      receives the arguments, appended; unchanged on
 *                          any failure.
 * @param[in]     _quoting  the rules; the platform's for response files by
 *                          default.
 * @param[in,out] _error    receives a failure's description, appended;
 *                          NULL for none.
 * @return status_code::ok; file_error for a file that could not be read; or
 *         a status of split_line.
 */
status read_response_file(c_string                  _path,
                          std::vector<std::string>& _out,
                          quoting::value            _quoting =
                              quoting::native_response,
                          std::string*              _error = D_NULLPTR);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES


//==============================================================================
// 3.  ENCODING
//==============================================================================


#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 3.1    Wide text
//------------------------------------------------------------------------------
// 3.1.1
// narrow
//   functions: CLI11's narrow(): wide text as UTF-8, `wchar_t` read as UTF-16
// or UTF-32 by its width, an unpaired surrogate or a value past U+10FFFF
// becoming U+FFFD; d_arg_utf8_from_wide. std::bad_alloc when the text cannot
// be held. A NULL C string is empty text.
std::string narrow(const wchar_t* _text,
                   std::size_t    _length);
std::string narrow(const wchar_t* _text);
std::string narrow(const std::wstring& _text);

// 3.1.2
// widen
/**
 * @brief Converts UTF-8 to wide text; CLI11's widen().
 *
 * @note `wchar_t` is written as UTF-16 or UTF-32 by its width, as
 *       d_arg_wide_from_utf8 writes it. Text that is not well-formed UTF-8,
 *       for which CLI11's throws std::range_error, gives the empty string;
 *       the status form tells the two apart.
 *
 * @param[in] _text  the UTF-8: a C string -- NULL is empty text -- a
 *                   std::string or, from C++17, a std::string_view
 *                   converts.
 * @return the wide text, or the empty string; std::bad_alloc is thrown, as
 *         std::wstring throws it, when the text cannot be held.
 */
std::wstring widen(const text& _text);

// a run of bytes and its length, as CLI11's widen(const char*, size) takes
// it; as widen(text)
std::wstring widen(const char* _text,
                   std::size_t _length);

/**
 * @brief Converts UTF-8 to wide text, saying whether it could.
 *
 * @param[in]  _text  the UTF-8.
 * @param[out] _out   receives the wide text; unchanged on failure.
 * @return status_code::ok; conversion_error for text that is not
 *         well-formed UTF-8. std::bad_alloc is thrown, as std::wstring
 *         throws it, when the text cannot be held.
 */
status       widen(const text&   _text,
                   std::wstring& _out);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV


//==============================================================================
// 4.  INTERNALS
//==============================================================================


// 4.1    Lists
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.1.1
    // owned_strings
    //   class: a d_arg_string_list the C library fills, released when the
    //   object is, whatever leaves the scope.
    class owned_strings
    {
    public:
        owned_strings() D_NOEXCEPT;
        ~owned_strings();

        ::d_arg_string_list* get() D_NOEXCEPT;
        std::size_t          size() const D_NOEXCEPT;
        const char*          operator[](std::size_t _index) const D_NOEXCEPT;

    private:
        D_DELETED_FN(owned_strings(const owned_strings& _other))
        D_DELETED_FN(owned_strings& operator=(const owned_strings& _other))

        ::d_arg_string_list m_list;
    };

    // 4.1.2
    // append_strings
    //   function: the words of a list a C call filled, appended to a vector
    //   only once all are made -- std::bad_alloc, as the vector throws it,
    //   leaving it as it was -- and the description of the call's failure to
    //   a string; the call's status.
    status append_strings(int                       _status,
                          const owned_strings&      _words,
                          const scratch&            _described,
                          std::vector<std::string>& _out,
                          std::string*              _error);

NS_END  // internal


//==============================================================================
// 5.  DEFINITIONS
//==============================================================================


// 5.1    Splitting
//------------------------------------------------------------------------------
// split_line / read_response_file
//   The C calls into an owned list, then the words appended.
inline status
split_line(
    const text&               _line,
    std::vector<std::string>& _out,
    quoting::value            _quoting,
    re_std::uint32_t          _flags,
    std::string*              _error
)
{
    internal::owned_strings words;
    internal::scratch       described;
    const int               result = d_arg_split_line(
                                         _line,
                                         static_cast<int>(_quoting),
                                         _flags,
                                         words.get(),
                                         described.get());

    return internal::append_strings(result,
                                    words,
                                    described,
                                    _out,
                                    _error);
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

inline status
read_response_file(
    c_string                  _path,
    std::vector<std::string>& _out,
    quoting::value            _quoting,
    std::string*              _error
)
{
    internal::owned_strings words;
    internal::scratch       described;
    const int               result = d_arg_read_response_file(
                                         _path.get(),
                                         static_cast<int>(_quoting),
                                         words.get(),
                                         described.get());

    return internal::append_strings(result,
                                    words,
                                    described,
                                    _out,
                                    _error);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

// quote_word / split_program_name
//   The C calls into scratch buffers, copied out; memory that ran out
// std::bad_alloc.
inline std::string
quote_word(
    const text&    _word,
    quoting::value _quoting
)
{
    internal::scratch written;
    const int         result = d_arg_quote_word(_word,
                                                static_cast<int>(_quoting),
                                                written.get());

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written.str() : std::string();
}

inline std::pair<std::string, std::string>
split_program_name(
    const text& _line
)
{
    internal::scratch   name;
    ::d_pack_text       rest   = text();
    const int           result = d_arg_split_program_name(_line,
                                                          name.get(),
                                                          &rest);

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK)
           ? std::pair<std::string, std::string>(name.str(),
                                                 text(rest).str())
           : std::pair<std::string, std::string>();
}

// 5.2    Encoding
//------------------------------------------------------------------------------
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// narrow
//   d_arg_utf8_from_wide into a scratch buffer, copied out; the C string's
// length measured first.
inline std::string
narrow(
    const wchar_t* _text,
    std::size_t    _length
)
{
    internal::scratch written;
    const int         result = d_arg_utf8_from_wide(_text,
                                                    _length,
                                                    written.get());

    internal::raise_out_of_memory(result);

    return (result == D_ARG_OK) ? written.str() : std::string();
}

inline std::string
narrow(
    const wchar_t* _text
)
{
    return (_text != D_NULLPTR)
           ? narrow(_text,
                    std::wcslen(_text))
           : std::string();
}

inline std::string
narrow(
    const std::wstring& _text
)
{
    return narrow(_text.data(),
                  _text.size());
}

// widen
//   internal::widen_text into a string returned, empty when the text did
// not convert, or into the caller's.
inline std::wstring
widen(
    const text& _text
)
{
    std::wstring wide;

    (void)internal::widen_text(_text,
                               wide);

    return wide;
}

inline std::wstring
widen(
    const char* _text,
    std::size_t _length
)
{
    return widen(text(_text,
                      _length));
}

inline status
widen(
    const text&   _text,
    std::wstring& _out
)
{
    return status(internal::widen_text(_text,
                                       _out));
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// 5.3    Internals
//------------------------------------------------------------------------------
// internal::owned_strings
//   An empty list; d_arg_string_list_release.
inline
internal::owned_strings::owned_strings() D_NOEXCEPT
{
    const ::d_arg_string_list empty = D_ARG_LIST_INIT;

    m_list = empty;
}

inline
internal::owned_strings::~owned_strings()
{
    d_arg_string_list_release(&m_list);
}

inline ::d_arg_string_list*
internal::owned_strings::get() D_NOEXCEPT
{
    return &m_list;
}

inline std::size_t
internal::owned_strings::size() const D_NOEXCEPT
{
    return m_list.count;
}

inline const char*
internal::owned_strings::operator[](
    std::size_t _index
) const D_NOEXCEPT
{
    return m_list.items[_index];
}

// internal::append_strings
//   The words made into a vector of their own, then given to the caller's:
// by swap when it is empty, else into a copy that replaces it, so that a
// std::bad_alloc on the way leaves it as it was.
inline status
internal::append_strings(
    int                       _status,
    const owned_strings&      _words,
    const scratch&            _described,
    std::vector<std::string>& _out,
    std::string*              _error
)
{
    // the description of a failure, for a caller that asked for one
    if ( (_error != D_NULLPTR) &&
         (_status != D_ARG_OK) )
    {
        _error->append(_described.str());
    }

    // a failure leaves the vector as it was
    if (_status != D_ARG_OK)
    {
        return status(_status);
    }

    std::vector<std::string> words;

    words.reserve(_words.size());

    // each word, as the NUL-terminated copy reads
    for (std::size_t index = 0u;
         index < _words.size();
         ++index)
    {
        words.push_back(std::string(_words[index]));
    }

    // the words given over whole
    if (_out.empty())
    {
        _out.swap(words);
    }
    else
    {
        std::vector<std::string> combined(_out);

        combined.insert(combined.end(),
                        words.begin(),
                        words.end());
        _out.swap(combined);
    }

    return status();
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_ARGV_HPP
