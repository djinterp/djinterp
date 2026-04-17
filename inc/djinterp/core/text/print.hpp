/******************************************************************************
* djinterp [core]                                                  print.hpp
*
* djinterp foundational print header:
*   This header provides a portable, multi-target printing infrastructure
* for the djinterp framework. All output is dispatched at compile-time via
* SFINAE traits from printer_traits.hpp — no tag types or virtual dispatch
* are used.
*
*   OUTPUT TARGETS:
*     - console  : stdout/stderr via std::ostream or std::fprintf
*     - file     : FILE* or std::ofstream
*     - string   : std::string (append-based)
*     - buffer   : raw char* + size_t (snprintf-based)
*
*   CORE INTERFACE:
*     write_to(_target, _str)              — write a C string
*     write_to(_target, _data, _len)       — write a sized buffer
*     write_value_to(_target, _value)      — write any printable type
*     write_line_to(_target, _str)         — write a C string + newline
*     write_newline(_target)               — write a newline character
*     write_indent(_target, _depth)        — write indentation
*
*   PORTABILITY:
*   This header adapts to the available C++ standard:
*     - C++11: enable_if-based SFINAE overloads
*     - C++14: variable templates for trait values
*     - C++17: if constexpr in write_value_to, string_view overloads
*     - C++20: concepts (future, not yet used)
*
* path:      /inc/cpp/io/print.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_PRINT_
#define DJINTERP_PRINT_ 1

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include "../djinterp.hpp"
#include "./printer_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif


NS_DJINTERP


// =============================================================================
// I.   FORWARD DECLARATIONS
// =============================================================================

// buffer_state
//   struct: tracks position within a raw char buffer for incremental
// writes. Used by the buffer_writer functions.
struct buffer_state
{
    char*       buffer;
    std::size_t capacity;
    std::size_t position;
};


// =============================================================================
// II.  WRITE PRIMITIVES — C STRING
// =============================================================================
// Each overload is gated by a single SFINAE trait from
// printer_traits.hpp, ensuring tagless dispatch.


// -----------------------------------------------------------------------------
// A.  write_to — std::ostream targets
// -----------------------------------------------------------------------------

// write_to
//   function: writes a null-terminated C string to an ostream-derived
// target. Returns the number of characters written.
template<typename _Target>
inline typename std::enable_if<
    is_ostream<_Target>::value,
    std::size_t
>::type
write_to(_Target& _target, const char* _str)
{
    if (!_str)
    {
        return 0;
    }

    std::size_t len;

    len = std::strlen(_str);
    _target.write(_str, static_cast<std::streamsize>(len));

    return len;
}


// -----------------------------------------------------------------------------
// B.  write_to — FILE* targets
// -----------------------------------------------------------------------------

// write_to
//   function: writes a null-terminated C string to a FILE* target.
// Returns the number of characters written.
inline std::size_t
write_to(std::FILE* _target, const char* _str)
{
    if ( (!_target) ||
         (!_str) )
    {
        return 0;
    }

    return std::fwrite(_str,
                       sizeof(char),
                       std::strlen(_str),
                       _target);
}


// -----------------------------------------------------------------------------
// C.  write_to — std::string targets
// -----------------------------------------------------------------------------

// write_to
//   function: appends a null-terminated C string to a std::string
// target. Returns the number of characters appended.
inline std::size_t
write_to(std::string& _target, const char* _str)
{
    if (!_str)
    {
        return 0;
    }

    std::size_t len;

    len = std::strlen(_str);
    _target.append(_str, len);

    return len;
}


// -----------------------------------------------------------------------------
// D.  write_to — buffer_state targets
// -----------------------------------------------------------------------------

// write_to
//   function: writes a null-terminated C string into a raw buffer,
// advancing the position cursor. Returns the number of characters
// written. Will not overflow; truncates if the buffer is full.
inline std::size_t
write_to(buffer_state& _target, const char* _str)
{
    if ( (!_target.buffer) ||
         (!_str)           ||
         (_target.position >= _target.capacity) )
    {
        return 0;
    }

    std::size_t remaining;
    std::size_t len;
    std::size_t to_write;

    remaining = _target.capacity - _target.position - 1;
    len       = std::strlen(_str);
    to_write  = (len < remaining) ? len : remaining;

    std::memcpy(_target.buffer + _target.position,
                _str,
                to_write);
    _target.position += to_write;
    _target.buffer[_target.position] = '\0';

    return to_write;
}


// =============================================================================
// III. WRITE PRIMITIVES — SIZED BUFFER
// =============================================================================
// Overloads that accept explicit length, avoiding strlen for
// pre-measured data.


// -----------------------------------------------------------------------------
// A.  write_n_to — std::ostream targets
// -----------------------------------------------------------------------------

// write_n_to
//   function: writes _len bytes from _data to an ostream-derived
// target.
template<typename _Target>
inline typename std::enable_if<
    is_ostream<_Target>::value,
    std::size_t
>::type
write_n_to(_Target&    _target,
           const char* _data,
           std::size_t _len)
{
    if ( (!_data) ||
         (_len == 0) )
    {
        return 0;
    }

    _target.write(_data, static_cast<std::streamsize>(_len));

    return _len;
}


// -----------------------------------------------------------------------------
// B.  write_n_to — FILE* targets
// -----------------------------------------------------------------------------

// write_n_to
//   function: writes _len bytes from _data to a FILE* target.
inline std::size_t
write_n_to(std::FILE*  _target,
           const char* _data,
           std::size_t _len)
{
    if ( (!_target) ||
         (!_data)   ||
         (_len == 0) )
    {
        return 0;
    }

    return std::fwrite(_data,
                       sizeof(char),
                       _len,
                       _target);
}


// -----------------------------------------------------------------------------
// C.  write_n_to — std::string targets
// -----------------------------------------------------------------------------

// write_n_to
//   function: appends _len bytes from _data to a std::string target.
inline std::size_t
write_n_to(std::string& _target,
           const char*  _data,
           std::size_t  _len)
{
    if ( (!_data) ||
         (_len == 0) )
    {
        return 0;
    }

    _target.append(_data, _len);

    return _len;
}


// -----------------------------------------------------------------------------
// D.  write_n_to — buffer_state targets
// -----------------------------------------------------------------------------

// write_n_to
//   function: writes _len bytes from _data into a buffer_state,
// truncating if necessary.
inline std::size_t
write_n_to(buffer_state& _target,
           const char*   _data,
           std::size_t   _len)
{
    if ( (!_target.buffer) ||
         (!_data)          ||
         (_len == 0)       ||
         (_target.position >= _target.capacity) )
    {
        return 0;
    }

    std::size_t remaining;
    std::size_t to_write;

    remaining = _target.capacity - _target.position - 1;
    to_write  = (_len < remaining) ? _len : remaining;

    std::memcpy(_target.buffer + _target.position,
                _data,
                to_write);
    _target.position += to_write;
    _target.buffer[_target.position] = '\0';

    return to_write;
}


// =============================================================================
// IV.  CONVENIENCE WRITERS
// =============================================================================


// -----------------------------------------------------------------------------
// A.  write_newline
// -----------------------------------------------------------------------------

// write_newline
//   function: writes a newline character to any supported target.
template<typename _Target>
inline std::size_t
write_newline(_Target& _target)
{
    return write_to(_target, "\n");
}

// write_newline
//   function: FILE* overload for write_newline.
inline std::size_t
write_newline(std::FILE* _target)
{
    return write_to(_target, "\n");
}


// -----------------------------------------------------------------------------
// B.  write_line_to
// -----------------------------------------------------------------------------

// write_line_to
//   function: writes a C string followed by a newline to any
// supported target.
template<typename _Target>
inline std::size_t
write_line_to(_Target& _target, const char* _str)
{
    std::size_t written;

    written  = write_to(_target, _str);
    written += write_newline(_target);

    return written;
}

// write_line_to
//   function: FILE* overload for write_line_to.
inline std::size_t
write_line_to(std::FILE* _target, const char* _str)
{
    std::size_t written;

    written  = write_to(_target, _str);
    written += write_newline(_target);

    return written;
}


// -----------------------------------------------------------------------------
// C.  write_indent
// -----------------------------------------------------------------------------

// write_indent
//   function: writes _depth levels of indentation (using D_INDENT)
// to any supported target.
template<typename _Target>
inline std::size_t
write_indent(_Target&    _target,
             std::size_t _depth)
{
    std::size_t written;

    written = 0;
    for (std::size_t i = 0; i < _depth; ++i)
    {
        written += write_to(_target, D_INDENT);
    }

    return written;
}

// write_indent
//   function: FILE* overload for write_indent.
inline std::size_t
write_indent(std::FILE*  _target,
             std::size_t _depth)
{
    std::size_t written;

    written = 0;
    for (std::size_t i = 0; i < _depth; ++i)
    {
        written += write_to(_target, D_INDENT);
    }

    return written;
}


// =============================================================================
// V.   TYPED VALUE WRITERS
// =============================================================================
// These functions convert printable values to text and write them to a
// target. Dispatch is via SFINAE on the value type, not the target.


// D_PRINT_INT_BUFFER_SIZE
//   constant: maximum characters needed for any 64-bit integer
// including sign and null terminator.
#define D_PRINT_INT_BUFFER_SIZE     21

// D_PRINT_FLOAT_BUFFER_SIZE
//   constant: maximum characters needed for a double in %g format
// including sign, exponent, and null terminator.
#define D_PRINT_FLOAT_BUFFER_SIZE   64


// -----------------------------------------------------------------------------
// A.  write_value_to — C string passthrough
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes a C string value to a target (passthrough to
// write_to).
template<typename _Target>
inline std::size_t
write_value_to(_Target& _target, const char* _value)
{
    return write_to(_target, _value);
}

// write_value_to
//   function: FILE* overload for C string passthrough.
inline std::size_t
write_value_to(std::FILE* _target, const char* _value)
{
    return write_to(_target, _value);
}


// -----------------------------------------------------------------------------
// B.  write_value_to — std::string values
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes a std::string value to any target by extracting
// its .c_str().
template<typename _Target>
inline std::size_t
write_value_to(_Target& _target, const std::string& _value)
{
    return write_n_to(_target,
                      _value.data(),
                      _value.size());
}

// write_value_to
//   function: FILE* overload for std::string values.
inline std::size_t
write_value_to(std::FILE* _target, const std::string& _value)
{
    return write_n_to(_target,
                      _value.data(),
                      _value.size());
}


// -----------------------------------------------------------------------------
// C.  write_value_to — string_view values (C++17)
// -----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// write_value_to
//   function: writes a std::string_view value to any target.
template<typename _Target>
inline std::size_t
write_value_to(_Target& _target, std::string_view _value)
{
    return write_n_to(_target,
                      _value.data(),
                      _value.size());
}

// write_value_to
//   function: FILE* overload for std::string_view values.
inline std::size_t
write_value_to(std::FILE* _target, std::string_view _value)
{
    return write_n_to(_target,
                      _value.data(),
                      _value.size());
}

#endif  // C++17


// -----------------------------------------------------------------------------
// D.  write_value_to — integral types
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes an integral value to any target, converting
// via snprintf. Excluded for bool, char, and pointer types.
template<typename _Target,
         typename _IntType>
inline typename std::enable_if<
    ( std::is_integral<_IntType>::value  &&
      !std::is_same<_IntType, bool>::value &&
      !std::is_same<_IntType, char>::value ),
    std::size_t
>::type
write_value_to(_Target& _target, _IntType _value)
{
    char buf[D_PRINT_INT_BUFFER_SIZE];
    int  len;

    len = std::snprintf(buf,
                        D_PRINT_INT_BUFFER_SIZE,
                        "%lld",
                        static_cast<long long>(_value));

    if (len <= 0)
    {
        return 0;
    }

    return write_n_to(_target,
                      buf,
                      static_cast<std::size_t>(len));
}


// -----------------------------------------------------------------------------
// E.  write_value_to — unsigned integral types
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes an unsigned integral value to any target.
template<typename _Target,
         typename _UIntType>
inline typename std::enable_if<
    ( std::is_integral<_UIntType>::value  &&
      std::is_unsigned<_UIntType>::value  &&
      !std::is_same<_UIntType, bool>::value &&
      !std::is_same<_UIntType, char>::value ),
    std::size_t
>::type
write_unsigned_value_to(_Target& _target, _UIntType _value)
{
    char buf[D_PRINT_INT_BUFFER_SIZE];
    int  len;

    len = std::snprintf(buf,
                        D_PRINT_INT_BUFFER_SIZE,
                        "%llu",
                        static_cast<unsigned long long>(_value));

    if (len <= 0)
    {
        return 0;
    }

    return write_n_to(_target,
                      buf,
                      static_cast<std::size_t>(len));
}


// -----------------------------------------------------------------------------
// F.  write_value_to — floating-point types
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes a floating-point value to any target using %g
// format.
template<typename _Target,
         typename _FloatType>
inline typename std::enable_if<
    std::is_floating_point<_FloatType>::value,
    std::size_t
>::type
write_value_to(_Target& _target, _FloatType _value)
{
    char buf[D_PRINT_FLOAT_BUFFER_SIZE];
    int  len;

    len = std::snprintf(buf,
                        D_PRINT_FLOAT_BUFFER_SIZE,
                        "%g",
                        static_cast<double>(_value));

    if (len <= 0)
    {
        return 0;
    }

    return write_n_to(_target,
                      buf,
                      static_cast<std::size_t>(len));
}


// -----------------------------------------------------------------------------
// G.  write_value_to — bool
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes a boolean value as "true" or "false".
template<typename _Target>
inline std::size_t
write_value_to(_Target& _target, bool _value)
{
    return write_to(_target, _value ? "true" : "false");
}

// write_value_to
//   function: FILE* overload for boolean values.
inline std::size_t
write_value_to(std::FILE* _target, bool _value)
{
    return write_to(_target, _value ? "true" : "false");
}


// -----------------------------------------------------------------------------
// H.  write_value_to — char
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes a single character.
template<typename _Target>
inline std::size_t
write_value_to(_Target& _target, char _value)
{
    char buf[2];

    buf[0] = _value;
    buf[1] = '\0';

    return write_to(_target, buf);
}

// write_value_to
//   function: FILE* overload for single character values.
inline std::size_t
write_value_to(std::FILE* _target, char _value)
{
    if (!_target)
    {
        return 0;
    }

    return (std::fputc(_value, _target) != EOF) ? 1 : 0;
}


// -----------------------------------------------------------------------------
// I.  write_value_to — types with .to_string()
// -----------------------------------------------------------------------------

// write_value_to
//   function: writes a value by calling its .to_string() member.
// Only enabled for types that have .to_string() but are not
// otherwise directly printable (avoids ambiguity with string types).
template<typename _Target,
         typename _Type>
inline typename std::enable_if<
    ( has_to_string<_Type>::value           &&
      !is_string_like<_Type>::value         &&
      !std::is_arithmetic<_Type>::value ),
    std::size_t
>::type
write_value_to(_Target& _target, const _Type& _value)
{
    return write_value_to(_target, _value.to_string());
}


// =============================================================================
// VI.  KEY-VALUE PAIR WRITER
// =============================================================================
// Convenience for printing labeled values, common in env_printer and
// diagnostic output.


// write_kv
//   function: writes a key-value pair in the format "key: value\n"
// to any supported target.
template<typename _Target,
         typename _ValueType>
inline std::size_t
write_kv(_Target&    _target,
         const char* _key,
         _ValueType  _value,
         std::size_t _indent_depth = 0)
{
    std::size_t written;

    written  = write_indent(_target, _indent_depth);
    written += write_to(_target, _key);
    written += write_to(_target, ": ");
    written += write_value_to(_target, _value);
    written += write_newline(_target);

    return written;
}

// write_kv
//   function: FILE* overload for key-value pair writing.
template<typename _ValueType>
inline std::size_t
write_kv(std::FILE*  _target,
         const char* _key,
         _ValueType  _value,
         std::size_t _indent_depth = 0)
{
    std::size_t written;

    written  = write_indent(_target, _indent_depth);
    written += write_to(_target, _key);
    written += write_to(_target, ": ");
    written += write_value_to(_target, _value);
    written += write_newline(_target);

    return written;
}


// =============================================================================
// VII. SECTION HEADER WRITER
// =============================================================================
// Convenience for printing section headers with separator lines.


// D_PRINT_SEPARATOR_CHAR
//   constant: character used for separator lines in section headers.
#ifndef D_PRINT_SEPARATOR_CHAR
    #define D_PRINT_SEPARATOR_CHAR '-'
#endif

// D_PRINT_SEPARATOR_WIDTH
//   constant: width of separator lines in section headers.
#ifndef D_PRINT_SEPARATOR_WIDTH
    #define D_PRINT_SEPARATOR_WIDTH 60
#endif

// write_section_header
//   function: writes a section header with separator lines to any
// supported target. Format:
//   ------ <title> ------
template<typename _Target>
inline std::size_t
write_section_header(_Target&    _target,
                     const char* _title,
                     std::size_t _indent_depth = 0)
{
    std::size_t written;
    std::size_t title_len;
    std::size_t pad;

    written   = write_indent(_target, _indent_depth);
    title_len = _title ? std::strlen(_title) : 0;

    // calculate padding for centered title
    if (title_len + 4 < D_PRINT_SEPARATOR_WIDTH)
    {
        pad = (D_PRINT_SEPARATOR_WIDTH - title_len - 2) / 2;
    }
    else
    {
        pad = 3;
    }

    // leading separator
    for (std::size_t i = 0; i < pad; ++i)
    {
        written += write_value_to(_target, D_PRINT_SEPARATOR_CHAR);
    }

    written += write_to(_target, " ");
    written += write_to(_target, _title);
    written += write_to(_target, " ");

    // trailing separator
    for (std::size_t i = 0; i < pad; ++i)
    {
        written += write_value_to(_target, D_PRINT_SEPARATOR_CHAR);
    }

    written += write_newline(_target);

    return written;
}


// =============================================================================
// VIII. BUFFER STATE UTILITIES
// =============================================================================


// make_buffer_state
//   function: constructs a buffer_state from a raw char buffer and
// capacity. Initializes position to 0 and null-terminates the buffer.
inline buffer_state
make_buffer_state(char*       _buffer,
                  std::size_t _capacity)
{
    buffer_state state;

    state.buffer   = _buffer;
    state.capacity = _capacity;
    state.position = 0;

    if ( (_buffer) &&
         (_capacity > 0) )
    {
        _buffer[0] = '\0';
    }

    return state;
}


NS_END  // djinterp


#endif  // DJINTERP_PRINT_
