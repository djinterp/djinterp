/*******************************************************************************
* djinterp [c]                                                       db_face.hpp
*
* The C++ face of the database layers: what each layer's face shares.
*   A layer's face -- sqlite_face.hpp, mariadb_face.hpp, postgres_face.hpp --
* is a class that owns the layer's connection, closing it when the object
* goes, and has every function of the layer as a member of the same name,
* the connection argument dropped: d_sqlite_has_table(db, schema, table) is
* db.has_table(schema, table). The contract of each member is its C
* function's, which the face neither restates nor changes: the same results,
* the same NULL for the current schema, and no exceptions. What the face
* adds is what C cannot say:
*   - ownership: the connection, a prepared statement, and the lists, rows
*     and text a layer allocates, are released by destructors (name_list,
*     column_list, rows, text, each layer's statement), so no path out of a
*     scope leaks them;
*   - std::string and std::vector: a string argument is a C string or a
*     std::string (string_arg); a list fills a name_list or a
*     std::vector<std::string>; a text result fills a text, which keeps SQL
*     NULL apart from '', or a std::string, which cannot;
*   - scope: transaction and savepoint guards, which roll back what was not
*     committed or released when the scope is left, however it is left.
*   The C99 layer is the floor, and the face costs nothing over it: a face is
* its handle, a list or rows their C struct, text its pointer (section 6
* asserts each),
* every member is its C call, inline -- noexcept costing nothing, since the
* layers declare their functions D_NOTHROW -- and nothing allocates that the
* caller did not ask for: a std::string or a std::vector form, which the
* exact forms beside them never need. tools/check_db_face_cost.py checks
* that a call through a face compiles to the instructions the C call does.
* Each language level adds what it can say, where the compiler can say it:
*   - C++98: everything above;
*   - C++11: moves, an explicit bool beside is_open, constexpr construction,
*     and each layer's printf-like `f` forms as variadic templates, whose
*     arguments are checked to be what C's `...` can carry;
*   - C++17: [[nodiscard]] on what only answers, std::optional forms of text
*     results, and guards that deduce their face (face::transaction work(db))
*     and are warned of when made and dropped in one expression;
*   - C++20: literal formats checked against their arguments at compile time
*     (cfg_db_face.h), concepts for generic code (face::connection),
*     std::span column arrays, and char8_t strings;
*   - C++23: std::expected forms, which return the value or the result;
*   - C++26: deleted members that say why.
* A feature exists from the level and the compiler that have it, both, and
* is absent below; none changes a type's layout. Section 1 names each.
*   The face is for C++98 and later, ISO strict C++98 included, and works
* over either build of a layer: over the stubs every member reports what
* the stub does. It wraps the C types, never redeclaring their fields, as
* functional_face.hpp does. Like the layers, it needs a 64-bit integer type,
* and below that floor compiles to nothing.
*
*
* path:      /inc/djinterp/c/db/db_face.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LEVELS
    ------
    1.  Features
         1.  D_INTERNAL_DB_FACE_OPTIONAL
         2.  D_INTERNAL_DB_FACE_CONCEPTS
         3.  D_INTERNAL_DB_FACE_SPAN
         4.  D_INTERNAL_DB_FACE_CHAR8
         5.  D_INTERNAL_DB_FACE_FORMAT_CHECK
         6.  D_INTERNAL_DB_FACE_EXPECTED
         7.  D_INTERNAL_DB_FACE_NODISCARD_GUARD
2.  ARGUMENTS
    ---------
    1.  Strings
         1.  string_arg
    2.  Variadic forms
         1.  internal::vararg_safe
         2.  internal::check_varargs
    3.  Formats
         1.  runtime_format_text
         2.  runtime_format
         3.  internal::shared_dialect
         4.  internal::sqlite_dialect
    4.  Format checking
         1.  internal::arg_kind
         2.  internal::kind_of
         3.  internal::arg_kinds
         4.  internal::names_shared
         5.  internal::check_shared
         6.  internal::check_mprintf
         7.  checked_format
    5.  Format parameters
         1.  unchecked_format
         2.  format_arg
3.  RESULTS
    -------
    1.  Text
         1.  text
    2.  Lists
         1.  name_list
         2.  column_list
    3.  Rows
         1.  rows
         2.  Conversions
    4.  Descriptions
4.  CONCEPTS
    --------
    1.  Faces
         1.  connection
5.  SCOPES
    ------
    1.  Transactions
         1.  transaction
    2.  Savepoints
         1.  savepoint
    3.  Deduction
6.  COST
    ----
    1.  Sizes
    2.  Moves
*/

#ifndef DJINTERP_C_DB_DB_FACE_HPP
#define DJINTERP_C_DB_DB_FACE_HPP 1

// std
#include <algorithm>  // std::swap, below C++11
#include <cstddef>    // std::size_t; std::nullptr_t, from C++11
#include <cstring>    // std::strcmp
#include <string>     // std::string; std::u8string, from C++20
#include <utility>    // std::swap and std::declval, from C++11
#include <vector>     // std::vector
// djinterp
#include "../../djinterp.hpp"                  // framework root
#include "../../config/c/db/cfg_db_face.h"     // D_CFG_DB_FACE_FORMAT_CHECK
#include "./db_common.h"                       // the vocabulary, the lists
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // INT64_MAX: the floor


//==============================================================================
// 1.  LEVELS
//==============================================================================
// Each feature above C++11 is used where the level guarantees it and the
// compiler and library report it (env_cpp_features.h), both: a compiler that
// reports a feature below its level, as Clang does several, is held to the
// level, which is what -pedantic-errors accepts.


// 1.1    Features
//------------------------------------------------------------------------------
// 1.1.1
// D_INTERNAL_DB_FACE_OPTIONAL
//   constant: 1 from C++17 where the library has std::optional: the text
// results' std::optional<std::string> forms.
#if ( (D_ENV_LANG_IS_CPP17_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_STL_OPTIONAL) )
    #define D_INTERNAL_DB_FACE_OPTIONAL 1
#else
    #define D_INTERNAL_DB_FACE_OPTIONAL 0
#endif

// 1.1.2
// D_INTERNAL_DB_FACE_CONCEPTS
//   constant: 1 from C++20 where the language and the library have concepts:
// face::connection, and the guards' check of the face they are given.
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER)   &&                                    \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                                    \
      (D_ENV_CPP_FEATURE_STL_CONCEPTS) )
    #define D_INTERNAL_DB_FACE_CONCEPTS 1
#else
    #define D_INTERNAL_DB_FACE_CONCEPTS 0
#endif

// 1.1.3
// D_INTERNAL_DB_FACE_SPAN
//   constant: 1 from C++20 where the library has std::span: column arrays as
// a std::span<const d_db_column>.
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_STL_SPAN) )
    #define D_INTERNAL_DB_FACE_SPAN 1
#else
    #define D_INTERNAL_DB_FACE_SPAN 0
#endif

// 1.1.4
// D_INTERNAL_DB_FACE_CHAR8
//   constant: 1 from C++20 where the language and the library have char8_t:
// string arguments as UTF-8 literals (u8"...") and std::u8string.
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CHAR8_T) &&                                     \
      (D_ENV_CPP_FEATURE_STL_CHAR8_T) )
    #define D_INTERNAL_DB_FACE_CHAR8 1
#else
    #define D_INTERNAL_DB_FACE_CHAR8 0
#endif

// 1.1.5
// D_INTERNAL_DB_FACE_FORMAT_CHECK
//   constant: 1 when the `f` forms check a literal format against their
// arguments at compile time: the knob on (cfg_db_face.h), and C++20 with
// consteval and concepts.
#if ( (D_CFG_DB_FACE_FORMAT_CHECK)       &&                                   \
      (D_INTERNAL_DB_FACE_CONCEPTS)      &&                                   \
      (D_ENV_CPP_FEATURE_LANG_CONSTEVAL) )
    #define D_INTERNAL_DB_FACE_FORMAT_CHECK 1
#else
    #define D_INTERNAL_DB_FACE_FORMAT_CHECK 0
#endif

// 1.1.6
// D_INTERNAL_DB_FACE_EXPECTED
//   constant: 1 from C++23 where the library has std::expected: the forms
// that return the value or the layer's result instead of writing through a
// parameter.
#if ( (D_ENV_LANG_IS_CPP23_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_STL_EXPECTED) )
    #define D_INTERNAL_DB_FACE_EXPECTED 1
#else
    #define D_INTERNAL_DB_FACE_EXPECTED 0
#endif

// 1.1.7
// D_INTERNAL_DB_FACE_NODISCARD_GUARD
//   attribute: [[nodiscard]] from C++17, on the guards' classes and their
// constructors, so that a guard made and dropped in one expression --
// face::transaction<F>{db};, which begins and ends at once -- is warned of
// (C++20 gives a constructor the attribute, and the compilers give it from
// C++17, as a defect report; GCC reads it on the constructor, Clang on
// either); empty below C++17, whose spellings of it fit neither.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_INTERNAL_DB_FACE_NODISCARD_GUARD [[nodiscard]]
#else
    #define D_INTERNAL_DB_FACE_NODISCARD_GUARD
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
    #include <type_traits>  // std::decay, std::is_arithmetic, std::is_enum
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_INTERNAL_DB_FACE_CONCEPTS
    // std
    #include <concepts>  // std::same_as, std::convertible_to
#endif  // D_INTERNAL_DB_FACE_CONCEPTS

#if D_INTERNAL_DB_FACE_EXPECTED
    // std
    #include <expected>  // std::expected, std::unexpected
#endif  // D_INTERNAL_DB_FACE_EXPECTED

// 64-bit floor: the layers' query headers need a 64-bit integer type, which
// dstdint.h declares only where the build can spell one. Below it -- ISO
// strict C++98 where long is narrower: 32-bit targets, and 64-bit Windows --
// the face compiles to nothing, as the layers do.
#if defined(INT64_MAX)


namespace djinterp
{
namespace face
{


//==============================================================================
// 2.  ARGUMENTS
//==============================================================================


// 2.1    Strings
//------------------------------------------------------------------------------
// 2.1.1
// string_arg
//   class: a string argument, given as a C string or as a std::string, which
// the face hands to the layer as a C string; from C++20 a UTF-8 literal or a
// std::u8string too, which are the same bytes. A NULL C string stays NULL
// -- the current schema, where a layer reads it so -- and a default-made
// string_arg is NULL. It borrows: a std::string it was made from must
// outlive it, which a temporary does for the call it is an argument to. It
// takes no std::string_view, whose text has no terminator for the layer.
class string_arg
{
public:
    D_CONSTEXPR string_arg() D_NOEXCEPT
        : m_text(D_NULLPTR)
    {}

    D_CONSTEXPR string_arg(
        const char* _text
    ) D_NOEXCEPT
        : m_text(_text)
    {}

    string_arg(
        const std::string& _text
    ) D_NOEXCEPT
        : m_text(_text.c_str())
    {}

#if D_INTERNAL_DB_FACE_CHAR8
    // UTF-8 -- a char8_t string is char's bytes; a template, so that a null
    // pointer constant still means the C string, as below C++20
    template<typename Char,
             typename = std::enable_if_t<std::is_same_v<Char, char8_t>>>
    string_arg(
        const Char* _text
    ) noexcept
        : m_text(reinterpret_cast<const char*>(_text))
    {}

    string_arg(
        const std::u8string& _text
    ) noexcept
        : m_text(reinterpret_cast<const char*>(_text.c_str()))
    {}
#endif  // D_INTERNAL_DB_FACE_CHAR8

    // c_str -- the C string, or NULL
    D_CONSTEXPR const char*
    c_str() const D_NOEXCEPT
    {
        return m_text;
    }

private:
    const char* m_text;
};

// 2.2    Variadic forms
//------------------------------------------------------------------------------
// A layer's `f` form takes its values through C's `...`, which carries
// numbers, unscoped enumerations and pointers, and nothing with a
// constructor. From C++11 the face offers the `f` forms as variadic
// templates, and refuses at compile time any argument `...` cannot carry --
// a std::string above all, which passes as its c_str(). From C++20 a
// literal format is checked as well (2.4).
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

namespace internal
{

    // 2.2.1
    // vararg_safe
    //   trait: true when every type can pass through C's `...`.
    template<typename... Types>
    struct vararg_safe;

    template<>
    struct vararg_safe<>
    {
        static constexpr bool value = true;
    };

    template<typename    Type,
             typename... Types>
    struct vararg_safe<Type, Types...>
    {
    private:
        using plain = typename std::decay<Type>::type;

    public:
        static constexpr bool value =
            ( ( (std::is_arithmetic<plain>::value)                 ||
                (std::is_pointer<plain>::value)                    ||
                (std::is_same<plain, std::nullptr_t>::value)       ||
                ( (std::is_enum<plain>::value) &&
                  (std::is_convertible<plain, int>::value) ) ) &&
              (vararg_safe<Types...>::value) );
    };

    // 2.2.2
    // check_varargs
    /**
     * @brief Refuses, at compile time, any argument C's `...` cannot carry,
     *        with a message that says what to pass instead.
     */
    template<typename... Types>
    inline void
    check_varargs() noexcept
    {
        static_assert(vararg_safe<Types...>::value,
                      "an f form takes numbers, enums and pointers; "
                      "pass a std::string as its c_str()");

        return;
    }

}  // namespace internal

// 2.3    Formats
//------------------------------------------------------------------------------
// An `f` form's format is the layer's. MariaDB's, MySQL's and PostgreSQL's
// layers read the shared dialect: %Q a value, %I a name, %T a qualifier and
// a name, %s, %c, %d, %i, %u and their l, ll and (%zu) z forms, %f and %g.
// SQLite's layer reads the shared dialect in a format that names %I or %T,
// and sqlite3_mprintf's (%q, %Q, %w, with flags and widths) in any other.
// Code written for every face keeps to the shared dialect and names a name
// with %I. A format is a literal: from C++20 it is checked against its
// arguments. A format built at run time goes through runtime_format: a
// std::string at every level, and from C++20 a char pointer too, which
// below C++20 is taken as it stands, since nothing there can tell it from a
// literal. Code that marks every format it builds compiles at every level.

// 2.3.1
// runtime_format_text
//   class: a format the face takes unchecked, because it was built at run
// time; made by runtime_format. A string_arg, and no more.
class runtime_format_text : public string_arg
{
public:
    explicit runtime_format_text(
        string_arg _format
    ) noexcept
        : string_arg(_format)
    {}
};

// 2.3.2
// runtime_format
/**
 * @brief Marks a format built at run time, which an `f` form then takes
 *        without the compile-time check a literal gets from C++20.
 *
 * @param[in] _format  the format; borrowed, as by string_arg.
 * @return the format, marked.
 */
inline runtime_format_text
runtime_format(
    string_arg _format
) noexcept
{
    return runtime_format_text(_format);
}

namespace internal
{

    // 2.3.3
    // shared_dialect
    //   type: the formats MariaDB's, MySQL's and PostgreSQL's layers read
    // (db_internal.h, 3.2); from C++20, its check.
    struct shared_dialect;

    // 2.3.4
    // sqlite_dialect
    //   type: the formats SQLite's layer reads: the shared dialect when the
    // format names %I or %T, and sqlite3_mprintf's otherwise; from C++20,
    // its check.
    struct sqlite_dialect;

}  // namespace internal

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.4    Format checking
//------------------------------------------------------------------------------
// From C++20 an `f` form's format parameter is a checked_format, whose
// constructor runs at compile time: it reads the format as the layer will,
// and matches each conversion against the argument it takes, as C's `...`
// passes it -- an int for %d, a long long for %lld, a double for %g, a
// string for %s or %Q, two strings for %T. A mismatch, a conversion the
// dialect lacks, or one argument too many or too few stops the compile at
// a function whose name says which (the_format_*). The checks are strict
// where printf is loose only across platforms: %lld takes a long long, not
// an int64_t, which is long on some platforms and long long on others.
#if D_INTERNAL_DB_FACE_FORMAT_CHECK

namespace internal
{

    // 2.4.1
    // arg_kind
    //   enum: what C's `...` carries an argument as: an integer of one of
    // four ranks, each with its sign; a double, or a long double; a char
    // string, the null pointer, a pointer to int, or another pointer; or
    // nothing it can carry.
    enum class arg_kind
    {
        none,
        signed_int,
        unsigned_int,
        signed_long,
        unsigned_long,
        signed_long_long,
        unsigned_long_long,
        real,
        long_real,
        text,
        null,
        int_pointer,
        pointer
    };

    // 2.4.2
    // kind_of
    /**
     * @brief How C's `...` carries an argument of a type: integers and
     *        unscoped enumerations promoted, as the language promotes them,
     *        and a float as a double.
     *
     * @return the argument's kind; arg_kind::none for a type `...` cannot
     *         carry.
     */
    template<typename Type>
    consteval arg_kind
    kind_of() noexcept
    {
        using plain = std::remove_cv_t<std::decay_t<Type>>;

        // the null pointer, and pointers: a char string, an int's, any other
        if constexpr (std::is_same_v<plain, std::nullptr_t>)
        {
            return arg_kind::null;
        }
        else if constexpr (std::is_pointer_v<plain>)
        {
            using pointee = std::remove_cv_t<std::remove_pointer_t<plain>>;

            if constexpr ( (std::is_same_v<pointee, char>)        ||
                           (std::is_same_v<pointee, signed char>) ||
                           (std::is_same_v<pointee, unsigned char>) )
            {
                return arg_kind::text;
            }
#if D_INTERNAL_DB_FACE_CHAR8
            else if constexpr (std::is_same_v<pointee, char8_t>)
            {
                return arg_kind::text;
            }
#endif  // D_INTERNAL_DB_FACE_CHAR8
            else if constexpr (std::is_same_v<pointee, int>)
            {
                return arg_kind::int_pointer;
            }
            else
            {
                return arg_kind::pointer;
            }
        }
        else if constexpr (std::is_floating_point_v<plain>)
        {
            return (std::is_same_v<plain, long double>) ? arg_kind::long_real
                                                        : arg_kind::real;
        }
        else if constexpr ( (std::is_integral_v<plain>) ||
                            ( (std::is_enum_v<plain>) &&
                              (std::is_convertible_v<plain, int>) ) )
        {
            // the type the language promotes it to, as `...` does
            using promoted = decltype(+std::declval<plain>());

            if constexpr (std::is_same_v<promoted, int>)
            {
                return arg_kind::signed_int;
            }
            else if constexpr (std::is_same_v<promoted, unsigned int>)
            {
                return arg_kind::unsigned_int;
            }
            else if constexpr (std::is_same_v<promoted, long>)
            {
                return arg_kind::signed_long;
            }
            else if constexpr (std::is_same_v<promoted, unsigned long>)
            {
                return arg_kind::unsigned_long;
            }
            else if constexpr (std::is_same_v<promoted, long long>)
            {
                return arg_kind::signed_long_long;
            }
            else if constexpr (std::is_same_v<promoted, unsigned long long>)
            {
                return arg_kind::unsigned_long_long;
            }
            else
            {
                return arg_kind::none;
            }
        }
        else
        {
            return arg_kind::none;
        }
    }

    // 2.4.3
    // arg_kinds
    //   trait: the kinds of a call's arguments, in order; one more element
    // than there are arguments, so that no call has an empty array.
    template<typename... Args>
    struct arg_kinds
    {
        static constexpr arg_kind values[sizeof...(Args) + 1u] =
        {
            kind_of<Args>()...,
            arg_kind::none
        };
    };

    // the check's failures -- functions no constant expression may call,
    // so that a format that reaches one stops the compile, naming it; never
    // called at run time, since the check runs only at compile time
    inline void
    the_format_has_too_few_arguments() noexcept
    {
        return;
    }

    inline void
    the_format_has_too_many_arguments() noexcept
    {
        return;
    }

    inline void
    the_format_argument_does_not_match_its_conversion() noexcept
    {
        return;
    }

    inline void
    the_format_has_a_conversion_its_dialect_lacks() noexcept
    {
        return;
    }

    // a conversion's demand on its argument, for want
    enum class arg_want
    {
        integer,      // an integer of the rank given, either sign
        character,    // an int, which a character is promoted to
        real,         // a double
        text,         // a char string, or the null pointer
        name,         // a char string, never null
        pointer,      // any pointer
        int_pointer   // a pointer to int
    };

    /**
     * @brief Whether an argument of one kind meets a conversion's demand.
     *
     * @param[in] _want  the demand.
     * @param[in] _rank  for an integer, the rank: 0 int, 1 long, 2 long
     *                   long, 3 size_t.
     * @param[in] _kind  the argument's kind.
     * @return `true` when `...` carries the argument as the conversion
     *         reads it.
     */
    consteval bool
    meets(
        arg_want    _want,
        std::size_t _rank,
        arg_kind    _kind
    ) noexcept
    {
        // size_t's rank is the rank of the type it is
        if (_rank == 3u)
        {
            const arg_kind size = kind_of<std::size_t>();

            _rank = ( (size == arg_kind::unsigned_long_long) ? 2u
                    : (size == arg_kind::unsigned_long)      ? 1u
                                                             : 0u );
        }

        switch (_want)
        {
            case arg_want::integer:
            {
                return ( ( (_rank == 0u) &&
                           ( (_kind == arg_kind::signed_int) ||
                             (_kind == arg_kind::unsigned_int) ) )      ||
                         ( (_rank == 1u) &&
                           ( (_kind == arg_kind::signed_long) ||
                             (_kind == arg_kind::unsigned_long) ) )    ||
                         ( (_rank == 2u) &&
                           ( (_kind == arg_kind::signed_long_long) ||
                             (_kind == arg_kind::unsigned_long_long) ) ) );
            }
            case arg_want::character:
            {
                return ( (_kind == arg_kind::signed_int) ||
                         (_kind == arg_kind::unsigned_int) );
            }
            case arg_want::real:
            {
                return (_kind == arg_kind::real);
            }
            case arg_want::text:
            {
                return ( (_kind == arg_kind::text) ||
                         (_kind == arg_kind::null) );
            }
            case arg_want::name:
            {
                return (_kind == arg_kind::text);
            }
            case arg_want::pointer:
            {
                return ( (_kind == arg_kind::text)        ||
                         (_kind == arg_kind::null)        ||
                         (_kind == arg_kind::int_pointer) ||
                         (_kind == arg_kind::pointer) );
            }
            case arg_want::int_pointer:
            {
                return (_kind == arg_kind::int_pointer);
            }
        }

        return false;
    }

    /**
     * @brief Takes the next argument for a conversion, and stops the compile
     *        when there is none, or when it does not meet the demand.
     *
     * @param[in]     _kinds  the arguments' kinds.
     * @param[in]     _count  how many arguments.
     * @param[in,out] _next   the next argument's index; advanced.
     * @param[in]     _want   the conversion's demand.
     * @param[in]     _rank   for an integer, its rank, as meets reads it.
     */
    consteval void
    take(
        const arg_kind* _kinds,
        std::size_t     _count,
        std::size_t&    _next,
        arg_want        _want,
        std::size_t     _rank
    ) noexcept
    {
        // an argument for the conversion
        if (_next >= _count)
        {
            the_format_has_too_few_arguments();
        }

        // of the kind it reads
        if (!meets(_want, _rank, _kinds[_next]))
        {
            the_format_argument_does_not_match_its_conversion();
        }

        ++_next;

        return;
    }

    // 2.4.4
    // names_shared
    /**
     * @brief Whether a format names %I or %T, read as the shared formatter
     *        reads one -- as SQLite's layer decides its dialect
     *        (sqlite_internal.c).
     *
     * @param[in] _format  the format.
     * @return `true` when a conversion's letter is I or T.
     */
    consteval bool
    names_shared(
        const char* _format
    ) noexcept
    {
        std::size_t at = 0u;

        // each conversion, and what follows its percent sign
        while (_format[at] != '\0')
        {
            // text, up to a conversion
            if (_format[at] != '%')
            {
                ++at;

                continue;
            }

            ++at;

            // the length modifier, ll before l, which it begins with
            if ( (_format[at] == 'l') &&
                 (_format[at + 1u] == 'l') )
            {
                at += 2u;
            }
            else if ( (_format[at] == 'l') ||
                      (_format[at] == 'z') )
            {
                ++at;
            }

            // the letter that names the dialect
            if ( (_format[at] == 'I') ||
                 (_format[at] == 'T') )
            {
                return true;
            }

            // past the letter, unless the format ends first
            if (_format[at] != '\0')
            {
                ++at;
            }
        }

        return false;
    }

    // 2.4.5
    // check_shared
    /**
     * @brief Checks a format in the shared dialect against its arguments,
     *        and stops the compile at the first fault.
     *
     * @param[in] _format  the format.
     * @param[in] _kinds   the arguments' kinds.
     * @param[in] _count   how many arguments.
     */
    consteval void
    check_shared(
        const char*     _format,
        const arg_kind* _kinds,
        std::size_t     _count
    ) noexcept
    {
        std::size_t at   = 0u;
        std::size_t next = 0u;

        // each conversion
        while (_format[at] != '\0')
        {
            // text, up to a conversion
            if (_format[at] != '%')
            {
                ++at;

                continue;
            }

            ++at;

            // the length modifier: 0 none, 1 l, 2 ll, 3 z
            std::size_t rank = 0u;

            if ( (_format[at] == 'l') &&
                 (_format[at + 1u] == 'l') )
            {
                rank  = 2u;
                at   += 2u;
            }
            else if (_format[at] == 'l')
            {
                rank = 1u;
                ++at;
            }
            else if (_format[at] == 'z')
            {
                rank = 3u;
                ++at;
            }

            const char letter = _format[at];

            // a modifier only the integers take, and z only %u
            if ( ( (rank != 0u)    &&
                   (letter != 'd') &&
                   (letter != 'i') &&
                   (letter != 'u') ) ||
                 ( (rank == 3u) &&
                   (letter != 'u') ) )
            {
                the_format_has_a_conversion_its_dialect_lacks();
            }

            switch (letter)
            {
                case 'd':
                case 'i':
                case 'u':
                {
                    take(_kinds, _count, next, arg_want::integer, rank);

                    break;
                }
                case 'f':
                case 'g':
                {
                    take(_kinds, _count, next, arg_want::real, 0u);

                    break;
                }
                case 'c':
                {
                    take(_kinds, _count, next, arg_want::character, 0u);

                    break;
                }
                case 's':
                case 'Q':
                {
                    take(_kinds, _count, next, arg_want::text, 0u);

                    break;
                }
                case 'I':
                {
                    take(_kinds, _count, next, arg_want::name, 0u);

                    break;
                }
                case 'T':
                {
                    take(_kinds, _count, next, arg_want::text, 0u);
                    take(_kinds, _count, next, arg_want::name, 0u);

                    break;
                }
                case '%':
                {
                    break;
                }
                default:
                {
                    the_format_has_a_conversion_its_dialect_lacks();

                    return;
                }
            }

            ++at;
        }

        // every argument read
        if (next != _count)
        {
            the_format_has_too_many_arguments();
        }

        return;
    }

    // 2.4.6
    // check_mprintf
    /**
     * @brief Checks a format in sqlite3_mprintf's dialect against its
     *        arguments, and stops the compile at the first fault.
     *
     * @param[in] _format  the format.
     * @param[in] _kinds   the arguments' kinds.
     * @param[in] _count   how many arguments.
     *
     * The grammar is SQLite's own: flags (-+ #!0,), a width or `*`, a
     * precision after `.` or `*`, an l or ll, and the letter. A `%` ending
     * the format is written as it is.
     */
    consteval void
    check_mprintf(
        const char*     _format,
        const arg_kind* _kinds,
        std::size_t     _count
    ) noexcept
    {
        std::size_t at   = 0u;
        std::size_t next = 0u;

        // each conversion
        while (_format[at] != '\0')
        {
            // text, up to a conversion
            if (_format[at] != '%')
            {
                ++at;

                continue;
            }

            ++at;

            // the flags
            while ( (_format[at] == '-') ||
                    (_format[at] == '+') ||
                    (_format[at] == ' ') ||
                    (_format[at] == '#') ||
                    (_format[at] == '!') ||
                    (_format[at] == '0') ||
                    (_format[at] == ',') )
            {
                ++at;
            }

            // the width, written or an int argument
            if (_format[at] == '*')
            {
                take(_kinds, _count, next, arg_want::character, 0u);
                ++at;
            }

            while ( (_format[at] >= '0') &&
                    (_format[at] <= '9') )
            {
                ++at;
            }

            // the precision, likewise
            if (_format[at] == '.')
            {
                ++at;

                // an int argument, or digits
                if (_format[at] == '*')
                {
                    take(_kinds, _count, next, arg_want::character, 0u);
                    ++at;
                }

                while ( (_format[at] >= '0') &&
                        (_format[at] <= '9') )
                {
                    ++at;
                }
            }

            // the length modifier
            std::size_t rank = 0u;

            if (_format[at] == 'l')
            {
                rank = 1u;
                ++at;

                // ll
                if (_format[at] == 'l')
                {
                    rank = 2u;
                    ++at;
                }
            }

            const char letter = _format[at];

            switch (letter)
            {
                case 'd':
                case 'i':
                case 'r':
                case 'u':
                case 'o':
                case 'x':
                case 'X':
                {
                    take(_kinds, _count, next, arg_want::integer, rank);

                    break;
                }
                case 'f':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                {
                    // l reads a double, as printf's %lf does; ll nothing
                    if (rank == 2u)
                    {
                        the_format_has_a_conversion_its_dialect_lacks();
                    }

                    take(_kinds, _count, next, arg_want::real, 0u);

                    break;
                }
                case 'c':
                case 's':
                case 'z':
                case 'q':
                case 'Q':
                case 'w':
                case 'j':
                case 'J':
                case 'p':
                case 'n':
                case '%':
                {
                    // no length modifier for these
                    if (rank != 0u)
                    {
                        the_format_has_a_conversion_its_dialect_lacks();
                    }

                    // the argument each reads, if any
                    if (letter == 'c')
                    {
                        take(_kinds, _count, next, arg_want::character, 0u);
                    }
                    else if (letter == 'p')
                    {
                        take(_kinds, _count, next, arg_want::pointer, 0u);
                    }
                    else if (letter == 'n')
                    {
                        take(_kinds, _count, next, arg_want::int_pointer, 0u);
                    }
                    else if (letter != '%')
                    {
                        take(_kinds, _count, next, arg_want::text, 0u);
                    }

                    break;
                }
                case '\0':
                {
                    // a percent sign ending the format is written as it is
                    if (rank == 0u)
                    {
                        break;
                    }

                    the_format_has_a_conversion_its_dialect_lacks();

                    return;
                }
                default:
                {
                    the_format_has_a_conversion_its_dialect_lacks();

                    return;
                }
            }

            // past the letter, unless the format ended at its percent sign
            if (letter != '\0')
            {
                ++at;
            }
        }

        // every argument read
        if (next != _count)
        {
            the_format_has_too_many_arguments();
        }

        return;
    }

    // shared_dialect -- 2.3.3's definition: its check
    struct shared_dialect
    {
        // check -- the shared dialect's
        template<typename... Args>
        static consteval void
        check(
            const char* _format
        ) noexcept
        {
            check_shared(_format, arg_kinds<Args...>::values, sizeof...(Args));

            return;
        }
    };

    // sqlite_dialect -- 2.3.4's definition: its check
    struct sqlite_dialect
    {
        // check -- the shared dialect's when the format names %I or %T, and
        // sqlite3_mprintf's otherwise, as SQLite's layer reads it
        template<typename... Args>
        static consteval void
        check(
            const char* _format
        ) noexcept
        {
            // the dialect the format is written in
            if (names_shared(_format))
            {
                check_shared(_format,
                             arg_kinds<Args...>::values,
                             sizeof...(Args));
            }
            else
            {
                check_mprintf(_format,
                              arg_kinds<Args...>::values,
                              sizeof...(Args));
            }

            return;
        }
    };

}  // namespace internal

// 2.4.7
// checked_format
//   class template: an `f` form's format, read at compile time when it is a
// literal, and taken as it is when runtime_format marked it. A pointer, and
// no more.
template<typename    Dialect,
         typename... Args>
class checked_format
{
public:
    // checked_format
    //   constructor: the compile-time check, of anything a C string can be
    // made from; one made at run time stops the compile, since its text is
    // not a constant: it goes through runtime_format
    template<typename Text>
        requires std::convertible_to<const Text&, const char*>
    consteval checked_format(
        const Text& _format
    ) noexcept
        : m_text(_format)
    {
        // what `...` cannot carry, refused first, in the words the forms
        // below C++20 use
        static_assert(internal::vararg_safe<Args...>::value,
                      "an f form takes numbers, enums and pointers; "
                      "pass a std::string as its c_str()");

        Dialect::template check<Args...>(m_text);
    }

    // checked_format
    //   constructor: a format runtime_format marked, unchecked
    checked_format(
        const runtime_format_text& _format
    ) noexcept
        : m_text(_format.c_str())
    {}

    // c_str -- the format
    constexpr const char*
    c_str() const noexcept
    {
        return m_text;
    }

private:
    const char* m_text;
};

#endif  // D_INTERNAL_DB_FACE_FORMAT_CHECK

// 2.5    Format parameters
//------------------------------------------------------------------------------
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.5.1
// unchecked_format
//   class: an `f` form's format where nothing checks it -- below C++20, or
// with the check off: a C string, or a format runtime_format marked. Never a
// std::string, at any level: that goes through runtime_format, as it must
// where formats are checked, and a format parameter that took one could be
// taken for the std::string a text result is written to. A pointer, and no
// more.
class unchecked_format
{
public:
    D_CONSTEXPR unchecked_format(
        const char* _format
    ) D_NOEXCEPT
        : m_text(_format)
    {}

    unchecked_format(
        const runtime_format_text& _format
    ) D_NOEXCEPT
        : m_text(_format.c_str())
    {}

    // c_str -- the format
    D_CONSTEXPR const char*
    c_str() const D_NOEXCEPT
    {
        return m_text;
    }

private:
    const char* m_text;
};

// 2.5.2
// format_arg
//   type: an `f` form's format parameter, for a dialect and the arguments
// that follow it: from C++20 a checked_format, which reads a literal at
// compile time; below, or with the check off, an unchecked_format. The
// arguments are not deduced from it, only from themselves.
#if D_INTERNAL_DB_FACE_FORMAT_CHECK
template<typename    Dialect,
         typename... Args>
using format_arg = checked_format<Dialect, std::type_identity_t<Args>...>;
#else
template<typename    Dialect,
         typename... Args>
using format_arg = unchecked_format;
#endif  // D_INTERNAL_DB_FACE_FORMAT_CHECK

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


//==============================================================================
// 3.  RESULTS
//==============================================================================


// 3.1    Text
//------------------------------------------------------------------------------
// 3.1.1
// text
//   class: text a layer allocated -- a query's text value, a setting -- which
// it releases with d_db_free when it goes. Null for SQL NULL, for a failure,
// and when made empty; std::string has no null, which is why text exists.
// Not copyable; movable from C++11.
class text
{
public:
    D_CONSTEXPR text() D_NOEXCEPT
        : m_text(D_NULLPTR)
    {}

    /**
     * @brief Makes a text that owns text a layer returned.
     *
     * @param[in] _owned  the text, or `NULL` for none.
     * @post The text releases `_owned` with d_db_free; the caller no longer
     *       does.
     */
    explicit D_CONSTEXPR text(
        char* _owned
    ) D_NOEXCEPT
        : m_text(_owned)
    {}

    ~text()
    {
        // text, when there is some; nothing calls for none
        if (m_text)
        {
            d_db_free(m_text);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the text passes to the object moved to, which releases what
    // it held; the object moved from is null
    text(
        text&& _other
    ) D_NOEXCEPT
        : m_text(_other.release())
    {}

    text&
    operator=(
        text&& _other
    ) D_NOEXCEPT
    {
        reset(_other.release());

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // text -- is_null is true for SQL NULL, for a failure, and when empty;
    // c_str is the text, or NULL; str is a copy, "" for NULL
    D_NODISCARD bool
    is_null() const D_NOEXCEPT
    {
        return (m_text == D_NULLPTR);
    }

    D_NODISCARD const char*
    c_str() const D_NOEXCEPT
    {
        return m_text;
    }

    D_NODISCARD std::string
    str() const
    {
        return (m_text) ? std::string(m_text)
                        : std::string();
    }

    /**
     * @brief Gives up the text without releasing it.
     *
     * @return the text, which the caller then releases with d_db_free;
     *         `NULL` when there is none.
     * @post The text is null.
     */
    D_NODISCARD char*
    release() D_NOEXCEPT
    {
        char* const held = m_text;

        m_text = D_NULLPTR;

        return held;
    }

    /**
     * @brief Releases the text held, if any, and takes other text.
     *
     * @param[in] _owned  the text to own, or `NULL` for none; the text
     *                    already held is kept, not released.
     * @post The text that was held, if other, is released, and every
     *       pointer obtained from c_str() is invalid.
     */
    void
    reset(
        char* _owned = D_NULLPTR
    ) D_NOEXCEPT
    {
        // what is held goes, unless it is what comes
        if (_owned != m_text)
        {
            // text, when there is some
            if (m_text)
            {
                d_db_free(m_text);
            }

            m_text = _owned;
        }

        return;
    }

    /**
     * @brief Empties the text and hands its slot to a layer's call to fill.
     *
     * @return where the layer writes the text it allocates; the text owns
     *         what is written there.
     * @post The text is null until the call fills the slot, and stays null
     *       when the call fails.
     */
    char**
    out() D_NOEXCEPT
    {
        reset();

        return &m_text;
    }

    // swap -- the two objects trade texts
    void
    swap(
        text& _other
    ) D_NOEXCEPT
    {
        std::swap(m_text, _other.m_text);

        return;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT
    {
        return !is_null();
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

private:
    D_DELETED_FN_MSG(text(const text&),
                     "a text owns what a layer allocated; move it")
    D_DELETED_FN_MSG(text& operator=(const text&),
                     "a text owns what a layer allocated; move it")

    char* m_text;
};

// 3.2    Lists
//------------------------------------------------------------------------------
// 3.2.1
// name_list
//   class: the names a layer listed, in its order, released when the object
// goes. Holds the C list, d_db_name_list, and restates nothing of it. Not
// copyable; movable from C++11.
class name_list
{
public:
    typedef const char* const* const_iterator;

    D_CONSTEXPR name_list() D_NOEXCEPT
        : m_list()
    {}

    ~name_list()
    {
        // names, when there are some
        if (m_list.names)
        {
            d_db_name_list_free(&m_list);
        }
    }

#if D_MOVE_ENABLED
    // moving -- the names pass to the list moved to, which releases what it
    // held; the list moved from is empty
    name_list(
        name_list&& _other
    ) D_NOEXCEPT
        : m_list()
    {
        swap(_other);
    }

    name_list&
    operator=(
        name_list&& _other
    ) D_NOEXCEPT
    {
        name_list taken(static_cast<name_list&&>(_other));

        swap(taken);

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // access -- the names, each valid while the list holds it
    D_NODISCARD std::size_t
    size() const D_NOEXCEPT
    {
        return m_list.count;
    }

    D_NODISCARD bool
    empty() const D_NOEXCEPT
    {
        return (m_list.count == 0u);
    }

    D_NODISCARD const char*
    operator[](
        std::size_t _index
    ) const D_NOEXCEPT
    {
        return m_list.names[_index];
    }

    D_NODISCARD const_iterator
    begin() const D_NOEXCEPT
    {
        return m_list.names;
    }

    D_NODISCARD const_iterator
    end() const D_NOEXCEPT
    {
        return m_list.names + m_list.count;
    }

    // contains -- whether a name is in the list, byte for byte; NULL is in
    // none
    D_NODISCARD bool
    contains(
        string_arg _name
    ) const D_NOEXCEPT
    {
        // nothing is NULL, so NULL is in no list
        if (!_name.c_str())
        {
            return false;
        }

        for (std::size_t i = 0u; i < m_list.count; ++i)
        {
            // byte for byte, as the layers match names
            if (std::strcmp(m_list.names[i], _name.c_str()) == 0)
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Replaces a vector's contents with copies of the names, in the
     *        list's order.
     *
     * @param[out] _out  the vector; its contents are replaced.
     */
    void
    to_vector(
        std::vector<std::string>& _out
    ) const
    {
        _out.assign(begin(), end());

        return;
    }

    /**
     * @brief Releases the names, leaving the list empty.
     *
     * @post Every name obtained from the list is invalid.
     */
    void
    clear() D_NOEXCEPT
    {
        // names, when there are some
        if (m_list.names)
        {
            d_db_name_list_free(&m_list);
        }

        return;
    }

    /**
     * @brief Empties the list and hands it to a layer's call to fill.
     *
     * @return the C list, for the layer to fill; the list owns what it is
     *         filled with.
     * @post Every name obtained from the list is invalid; the list is empty
     *       until the call fills it, and stays empty when the call fails.
     */
    struct d_db_name_list*
    out() D_NOEXCEPT
    {
        clear();

        return &m_list;
    }

    // core -- the C list, for the layer's own calls
    D_NODISCARD const struct d_db_name_list&
    core() const D_NOEXCEPT
    {
        return m_list;
    }

    // swap -- the two lists trade names
    void
    swap(
        name_list& _other
    ) D_NOEXCEPT
    {
        std::swap(m_list.count, _other.m_list.count);
        std::swap(m_list.names, _other.m_list.names);

        return;
    }

private:
    D_DELETED_FN_MSG(name_list(const name_list&),
                     "a name_list owns what a layer allocated; move it")
    D_DELETED_FN_MSG(name_list& operator=(const name_list&),
                     "a name_list owns what a layer allocated; move it")

    struct d_db_name_list m_list;
};

// 3.2.2
// column_list
//   class: the columns of a table, as a layer described them, released when
// the object goes. Holds the C list, d_db_column_list; each column is the
// C struct d_db_column_info, its strings valid while the list holds it. Not
// copyable; movable from C++11.
class column_list
{
public:
    typedef const struct d_db_column_info* const_iterator;

    D_CONSTEXPR column_list() D_NOEXCEPT
        : m_list()
    {}

    ~column_list()
    {
        // columns, when there are some
        if (m_list.columns)
        {
            d_db_column_list_free(&m_list);
        }
    }

#if D_MOVE_ENABLED
    // moving -- as name_list's
    column_list(
        column_list&& _other
    ) D_NOEXCEPT
        : m_list()
    {
        swap(_other);
    }

    column_list&
    operator=(
        column_list&& _other
    ) D_NOEXCEPT
    {
        column_list taken(static_cast<column_list&&>(_other));

        swap(taken);

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // access -- the columns, in the table's order
    D_NODISCARD std::size_t
    size() const D_NOEXCEPT
    {
        return m_list.count;
    }

    D_NODISCARD bool
    empty() const D_NOEXCEPT
    {
        return (m_list.count == 0u);
    }

    D_NODISCARD const struct d_db_column_info&
    operator[](
        std::size_t _index
    ) const D_NOEXCEPT
    {
        return m_list.columns[_index];
    }

    D_NODISCARD const_iterator
    begin() const D_NOEXCEPT
    {
        return m_list.columns;
    }

    D_NODISCARD const_iterator
    end() const D_NOEXCEPT
    {
        return m_list.columns + m_list.count;
    }

    // find -- the column of a name, byte for byte; NULL for none, and for a
    // NULL name
    D_NODISCARD const struct d_db_column_info*
    find(
        string_arg _name
    ) const D_NOEXCEPT
    {
        // nothing is NULL, so NULL names no column
        if (!_name.c_str())
        {
            return D_NULLPTR;
        }

        for (std::size_t i = 0u; i < m_list.count; ++i)
        {
            // byte for byte, as the layers match names
            if (std::strcmp(m_list.columns[i].name, _name.c_str()) == 0)
            {
                return &m_list.columns[i];
            }
        }

        return D_NULLPTR;
    }

    /**
     * @brief Releases the columns, leaving the list empty.
     *
     * @post Every column, and every string in one, obtained from the list is
     *       invalid.
     */
    void
    clear() D_NOEXCEPT
    {
        // columns, when there are some
        if (m_list.columns)
        {
            d_db_column_list_free(&m_list);
        }

        return;
    }

    /**
     * @brief Empties the list and hands it to a layer's call to fill.
     *
     * @return the C list, for the layer to fill; the list owns what it is
     *         filled with.
     * @post As clear's; the list is empty until the call fills it, and stays
     *       empty when the call fails.
     */
    struct d_db_column_list*
    out() D_NOEXCEPT
    {
        clear();

        return &m_list;
    }

    // core -- the C list, for the layer's own calls
    D_NODISCARD const struct d_db_column_list&
    core() const D_NOEXCEPT
    {
        return m_list;
    }

    // swap -- the two lists trade columns
    void
    swap(
        column_list& _other
    ) D_NOEXCEPT
    {
        std::swap(m_list.count, _other.m_list.count);
        std::swap(m_list.columns, _other.m_list.columns);

        return;
    }

private:
    D_DELETED_FN_MSG(column_list(const column_list&),
                     "a column_list owns what a layer allocated; move it")
    D_DELETED_FN_MSG(column_list& operator=(const column_list&),
                     "a column_list owns what a layer allocated; move it")

    struct d_db_column_list m_list;
};

// 3.3    Rows
//------------------------------------------------------------------------------
// 3.3.1
// rows
//   class: the rows a query or a statement returned, as a layer read them,
// released when the object goes. Holds the C rows, d_db_rows, and restates
// nothing of them: each column is the C struct d_db_field, each value the C
// struct d_db_value, their strings valid while the rows hold them. Not
// copyable; movable from C++11.
class rows
{
public:
    D_CONSTEXPR rows() D_NOEXCEPT
        : m_rows()
    {}

    ~rows()
    {
        // columns, and with them every value, when there are some
        if (m_rows.columns)
        {
            d_db_rows_free(&m_rows);
        }
    }

#if D_MOVE_ENABLED
    // moving -- as name_list's
    rows(
        rows&& _other
    ) D_NOEXCEPT
        : m_rows()
    {
        swap(_other);
    }

    rows&
    operator=(
        rows&& _other
    ) D_NOEXCEPT
    {
        rows taken(static_cast<rows&&>(_other));

        swap(taken);

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // access -- how many columns and rows; whether there are no rows; a
    // column's description, unchecked, as name_list's operator[] is; and
    // the value at a row and a column, as d_db_rows_value finds it: NULL
    // past the last row or column
    D_NODISCARD std::size_t
    column_count() const D_NOEXCEPT
    {
        return m_rows.column_count;
    }

    D_NODISCARD std::size_t
    row_count() const D_NOEXCEPT
    {
        return m_rows.row_count;
    }

    D_NODISCARD bool
    empty() const D_NOEXCEPT
    {
        return (m_rows.row_count == 0u);
    }

    D_NODISCARD const struct d_db_field&
    column(
        std::size_t _index
    ) const D_NOEXCEPT
    {
        return m_rows.columns[_index];
    }

    D_NODISCARD const struct d_db_value*
    value(
        std::size_t _row,
        std::size_t _column
    ) const D_NOEXCEPT
    {
        return d_db_rows_value(&m_rows, _row, _column);
    }

    // column_index -- a column by its name, as d_db_rows_column_index finds
    // it: exactly, or failing that without ASCII case; its result
    int
    column_index(
        string_arg   _name,
        std::size_t& _index
    ) const D_NOEXCEPT
    {
        return d_db_rows_column_index(&m_rows, _name.c_str(), &_index);
    }

#if D_INTERNAL_DB_FACE_EXPECTED
    D_NODISCARD std::expected<std::size_t, int>
    column_index(
        string_arg _name
    ) const D_NOEXCEPT
    {
        std::size_t index  = 0u;
        const int   result = d_db_rows_column_index(&m_rows, _name.c_str(),
                                                    &index);

        // the index, or the result when there is no such column
        if (result != 0)
        {
            return std::unexpected(result);
        }

        return index;
    }
#endif  // D_INTERNAL_DB_FACE_EXPECTED

    /**
     * @brief Releases the rows, leaving them empty.
     *
     * @post Every column, value and string obtained from the rows is
     *       invalid.
     */
    void
    clear() D_NOEXCEPT
    {
        // columns, and with them every value, when there are some
        if (m_rows.columns)
        {
            d_db_rows_free(&m_rows);
        }

        return;
    }

    /**
     * @brief Empties the rows and hands them to a layer's call to fill.
     *
     * @return the C rows, for the layer to fill; the rows own what they are
     *         filled with.
     * @post As clear's; the rows are empty until the call fills them, and
     *       stay empty when the call fails.
     */
    struct d_db_rows*
    out() D_NOEXCEPT
    {
        clear();

        return &m_rows;
    }

    // core -- the C rows, for the layer's own calls
    D_NODISCARD const struct d_db_rows&
    core() const D_NOEXCEPT
    {
        return m_rows;
    }

    // swap -- the two objects trade rows
    void
    swap(
        rows& _other
    ) D_NOEXCEPT
    {
        const struct d_db_rows held = m_rows;

        m_rows        = _other.m_rows;
        _other.m_rows = held;

        return;
    }

private:
    D_DELETED_FN_MSG(rows(const rows&),
                     "rows own what a layer allocated; move them")
    D_DELETED_FN_MSG(rows& operator=(const rows&),
                     "rows own what a layer allocated; move them")

    struct d_db_rows m_rows;
};

// 3.3.2
// Conversions
//   db_common.h's: a value as an integer, a real number or a boolean, where
// it is one or converts to it exactly, and D_DB_MISMATCH where it does not,
// NULL among them; the value written only on success. From C++23, forms that
// return the value or the result.
inline int
value_int64(
    const struct d_db_value* _value,
    re_std::int64_t&         _out
) D_NOEXCEPT
{
    return d_db_value_int64(_value, &_out);
}

inline int
value_double(
    const struct d_db_value* _value,
    double&                  _out
) D_NOEXCEPT
{
    return d_db_value_double(_value, &_out);
}

inline int
value_bool(
    const struct d_db_value* _value,
    bool&                    _out
) D_NOEXCEPT
{
    return d_db_value_bool(_value, &_out);
}

#if D_INTERNAL_DB_FACE_EXPECTED
D_NODISCARD inline std::expected<re_std::int64_t, int>
value_int64(
    const struct d_db_value* _value
) D_NOEXCEPT
{
    re_std::int64_t out    = 0;
    const int       result = d_db_value_int64(_value, &out);

    // the value, or why it does not convert
    if (result != 0)
    {
        return std::unexpected(result);
    }

    return out;
}

D_NODISCARD inline std::expected<double, int>
value_double(
    const struct d_db_value* _value
) D_NOEXCEPT
{
    double    out    = 0.0;
    const int result = d_db_value_double(_value, &out);

    // the value, or why it does not convert
    if (result != 0)
    {
        return std::unexpected(result);
    }

    return out;
}

D_NODISCARD inline std::expected<bool, int>
value_bool(
    const struct d_db_value* _value
) D_NOEXCEPT
{
    bool      out    = false;
    const int result = d_db_value_bool(_value, &out);

    // the value, or why it does not convert
    if (result != 0)
    {
        return std::unexpected(result);
    }

    return out;
}
#endif  // D_INTERNAL_DB_FACE_EXPECTED

// 3.4    Descriptions
//------------------------------------------------------------------------------
// descriptions -- db_common.h's words for its results, and the name of a
// d_db_type; neither is ever NULL, nor to be freed
D_NODISCARD inline const char*
status_message(
    int _status
) D_NOEXCEPT
{
    return d_db_status_message(_status);
}

D_NODISCARD inline const char*
type_name(
    enum d_db_type _type
) D_NOEXCEPT
{
    return d_db_type_name(_type);
}


//==============================================================================
// 4.  CONCEPTS
//==============================================================================


// 4.1    Faces
//------------------------------------------------------------------------------
#if D_INTERNAL_DB_FACE_CONCEPTS

// 4.1.1
// connection
//   concept: a face, as the guards and code written for every face use one:
// whether it is open, a transaction mode with a default, the transaction
// calls, and savepoints. Each layer's face is one.
template<typename Database>
concept connection =
    requires(Database&                           _db,
             const Database&                     _const_db,
             typename Database::transaction_mode _mode,
             string_arg                          _name)
    {
        { Database::default_transaction_mode() } ->
            std::same_as<typename Database::transaction_mode>;
        { _const_db.is_open() }             -> std::convertible_to<bool>;
        { _db.begin_transaction(_mode) }    -> std::convertible_to<int>;
        { _db.commit() }                    -> std::convertible_to<int>;
        { _db.rollback() }                  -> std::convertible_to<int>;
        { _db.in_transaction() }            -> std::convertible_to<bool>;
        { _db.create_savepoint(_name) }     -> std::convertible_to<int>;
        { _db.release_savepoint(_name) }    -> std::convertible_to<int>;
        { _db.rollback_to_savepoint(_name) } -> std::convertible_to<int>;
    };

#endif  // D_INTERNAL_DB_FACE_CONCEPTS


//==============================================================================
// 5.  SCOPES
//==============================================================================
// A guard begins something when it is made and ends it when it goes, on
// every path out of its scope; ending it early, by hand, stops it doing so.
// Each works with any face whose class has the members it calls, which from
// C++20 face::connection checks; from C++17 the guard deduces the face it is
// given: face::transaction work(db).


// 5.1    Transactions
//------------------------------------------------------------------------------
// 5.1.1
// transaction
//   class template: begins a transaction on a face when made, and rolls it
// back when it goes unless commit or rollback ended it first. result() is
// what beginning returned; a guard whose beginning failed is not active and
// ends nothing. Database is a face class, with begin_transaction,
// commit, rollback and in_transaction, a transaction_mode type, and a
// default_transaction_mode() function. Not copyable or movable: it belongs
// to its scope.
template<typename Database>
class D_INTERNAL_DB_FACE_NODISCARD_GUARD transaction
{
#if D_INTERNAL_DB_FACE_CONCEPTS
    static_assert(connection<Database>,
                  "face::transaction guards a face: a class with the "
                  "members face::connection names");
#endif  // D_INTERNAL_DB_FACE_CONCEPTS

public:
    /**
     * @brief Begins a transaction in the face's default mode.
     *
     * @param[in,out] _db  the face; it must outlive the guard.
     * @post result() is what beginning returned; the guard is active when it
     *       is 0.
     */
    D_INTERNAL_DB_FACE_NODISCARD_GUARD explicit transaction(
        Database& _db
    )
        : m_db(&_db),
          m_result(_db.begin_transaction(
                       Database::default_transaction_mode())),
          m_active(m_result == 0)
    {}

    /**
     * @brief Begins a transaction in a mode of the face's own.
     *
     * @param[in,out] _db    the face; it must outlive the guard.
     * @param[in]     _mode  how the transaction begins.
     * @post As for the constructor above.
     */
    D_INTERNAL_DB_FACE_NODISCARD_GUARD transaction(
        Database&                           _db,
        typename Database::transaction_mode _mode
    )
        : m_db(&_db),
          m_result(_db.begin_transaction(_mode)),
          m_active(m_result == 0)
    {}

    ~transaction()
    {
        // what was neither committed nor rolled back goes back
        if (m_active)
        {
            (void)m_db->rollback();
        }
    }

    // state -- what beginning returned, and whether the guard still has a
    // transaction to end
    D_NODISCARD int
    result() const D_NOEXCEPT
    {
        return m_result;
    }

    D_NODISCARD bool
    is_active() const D_NOEXCEPT
    {
        return m_active;
    }

    /**
     * @brief Commits the transaction, through the face's commit.
     *
     * @return the face's result, or D_DB_INVALID_ARGUMENT when the guard has
     *         nothing to end.
     * @post The guard is inactive, unless the commit failed and left the
     *       transaction open, as SQLite's busy commit does: then it stays
     *       active, to retry, or to roll back when it goes.
     */
    int
    commit()
    {
        return m_end(true);
    }

    /**
     * @brief Rolls the transaction back, through the face's rollback.
     *
     * @return the face's result, or D_DB_INVALID_ARGUMENT when the guard has
     *         nothing to end.
     * @post As for commit.
     */
    int
    rollback()
    {
        return m_end(false);
    }

private:
    D_DELETED_FN_MSG(transaction(const transaction&),
                     "a transaction guard belongs to its scope")
    D_DELETED_FN_MSG(transaction& operator=(const transaction&),
                     "a transaction guard belongs to its scope")

    // m_end -- commits or rolls back; the guard stays active only while the
    // face is still in the transaction it began
    int
    m_end(
        bool _commit
    )
    {
        // nothing to end
        if (!m_active)
        {
            return D_DB_INVALID_ARGUMENT;
        }

        const int ended = (_commit) ? m_db->commit()
                                    : m_db->rollback();

        m_active = ( (ended != 0) &&
                     (m_db->in_transaction()) );

        return ended;
    }

    Database* m_db;
    int       m_result;
    bool      m_active;
};

// 5.2    Savepoints
//------------------------------------------------------------------------------
// 5.2.1
// savepoint
//   class template: sets a savepoint on a face when made, and when it goes
// rolls back to it and releases it, unless release or rollback ended it
// first. result() is what setting it returned. The name is borrowed, as a
// string_view borrows, and must outlive the guard: a literal, or a string
// that does; from C++11 a temporary std::string is refused. Database is a
// face class, with create_savepoint, release_savepoint and
// rollback_to_savepoint. Not copyable or movable.
template<typename Database>
class D_INTERNAL_DB_FACE_NODISCARD_GUARD savepoint
{
#if D_INTERNAL_DB_FACE_CONCEPTS
    static_assert(connection<Database>,
                  "face::savepoint guards a face: a class with the "
                  "members face::connection names");
#endif  // D_INTERNAL_DB_FACE_CONCEPTS

public:
    /**
     * @brief Sets a savepoint.
     *
     * @param[in,out] _db    the face, in a transaction; it must outlive the
     *                       guard.
     * @param[in]     _name  the savepoint's name; borrowed, it must outlive
     *                       the guard.
     * @post result() is what setting it returned; the guard is active when
     *       it is 0.
     */
    D_INTERNAL_DB_FACE_NODISCARD_GUARD savepoint(
        Database&   _db,
        const char* _name
    )
        : m_db(&_db),
          m_name(_name),
          m_result(_db.create_savepoint(_name)),
          m_active(m_result == 0)
    {}

    /**
     * @brief Sets a savepoint named by a string that outlives the guard.
     *
     * @param[in,out] _db    the face, in a transaction; it must outlive the
     *                       guard.
     * @param[in]     _name  the savepoint's name; borrowed, it must outlive
     *                       the guard.
     * @post As for the constructor above.
     */
    D_INTERNAL_DB_FACE_NODISCARD_GUARD savepoint(
        Database&  _db,
        string_arg _name
    )
        : m_db(&_db),
          m_name(_name.c_str()),
          m_result(_db.create_savepoint(_name)),
          m_active(m_result == 0)
    {}

    ~savepoint()
    {
        // what was not released goes back, and the savepoint with it
        if (m_active)
        {
            (void)m_db->rollback_to_savepoint(m_name);
            (void)m_db->release_savepoint(m_name);
        }
    }

    // state -- as transaction's, and the name, as it was given
    D_NODISCARD int
    result() const D_NOEXCEPT
    {
        return m_result;
    }

    D_NODISCARD bool
    is_active() const D_NOEXCEPT
    {
        return m_active;
    }

    D_NODISCARD const char*
    name() const D_NOEXCEPT
    {
        return m_name;
    }

    /**
     * @brief Releases the savepoint, keeping what it covers.
     *
     * @return the face's result, or D_DB_INVALID_ARGUMENT when the guard has
     *         nothing to end.
     * @post The guard is inactive, unless the release failed.
     */
    int
    release()
    {
        // nothing to end
        if (!m_active)
        {
            return D_DB_INVALID_ARGUMENT;
        }

        const int released = m_db->release_savepoint(m_name);

        m_active = (released != 0);

        return released;
    }

    /**
     * @brief Rolls back to the savepoint, undoing what it covers, and
     *        releases it.
     *
     * @return the face's result, the first failure's; or
     *         D_DB_INVALID_ARGUMENT when the guard has nothing to end.
     * @post The guard is inactive, unless a step failed.
     */
    int
    rollback()
    {
        // nothing to end
        if (!m_active)
        {
            return D_DB_INVALID_ARGUMENT;
        }

        const int undone = m_db->rollback_to_savepoint(m_name);

        // undone, then released
        if (undone != 0)
        {
            return undone;
        }

        return release();
    }

private:
    D_DELETED_FN_MSG(savepoint(const savepoint&),
                     "a savepoint guard belongs to its scope")
    D_DELETED_FN_MSG(savepoint& operator=(const savepoint&),
                     "a savepoint guard belongs to its scope")
#if D_MOVE_ENABLED
    D_DELETED_FN_MSG(savepoint(Database& _db, std::string&& _name),
                     "a savepoint borrows its name, which a temporary "
                     "string would not outlive")
#endif  // D_MOVE_ENABLED

    Database*   m_db;
    const char* m_name;
    int         m_result;
    bool        m_active;
};

// 5.3    Deduction
//------------------------------------------------------------------------------
// From C++17 a guard deduces its face from the face it is given, one guide
// for each of its constructors: face::transaction work(db);.
#if ( (D_ENV_LANG_IS_CPP17_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES) )

template<typename Database>
transaction(Database&) -> transaction<Database>;

template<typename Database>
transaction(Database&,
            typename Database::transaction_mode) -> transaction<Database>;

template<typename Database>
savepoint(Database&, const char*) -> savepoint<Database>;

template<typename Database>
savepoint(Database&, string_arg) -> savepoint<Database>;

#endif  // C++17 deduction guides


//==============================================================================
// 6.  COST
//==============================================================================
// Zero overhead, asserted: each type the face adds is the C value it wraps,
// and no more, so holding one costs what holding the C value does; and from
// C++11 what owns moves without throwing, so a container of them moves them
// rather than failing to.


// 6.1    Sizes
//------------------------------------------------------------------------------
D_STATIC_ASSERT(sizeof(string_arg) == sizeof(const char*),
                "a string_arg is its pointer, and no more");
D_STATIC_ASSERT(sizeof(text) == sizeof(char*),
                "a text is its pointer, and no more");
D_STATIC_ASSERT(sizeof(name_list) == sizeof(struct d_db_name_list),
                "a name_list is its C list, and no more");
D_STATIC_ASSERT(sizeof(column_list) == sizeof(struct d_db_column_list),
                "a column_list is its C list, and no more");
D_STATIC_ASSERT(sizeof(rows) == sizeof(struct d_db_rows),
                "rows are their C rows, and no more");
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
D_STATIC_ASSERT(sizeof(runtime_format_text) == sizeof(const char*),
                "a runtime format is its pointer, and no more");
D_STATIC_ASSERT(sizeof(unchecked_format) == sizeof(const char*),
                "an unchecked format is its pointer, and no more");
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
#if D_INTERNAL_DB_FACE_FORMAT_CHECK
D_STATIC_ASSERT(
    sizeof(checked_format<internal::shared_dialect, int>) ==
        sizeof(const char*),
    "a checked format is its pointer, and no more");
#endif  // D_INTERNAL_DB_FACE_FORMAT_CHECK

// 6.2    Moves
//------------------------------------------------------------------------------
#if D_MOVE_ENABLED
D_STATIC_ASSERT( ( (std::is_nothrow_move_constructible<text>::value) &&
                   (std::is_nothrow_move_assignable<text>::value) ),
                 "a text moves without throwing");
D_STATIC_ASSERT( ( (std::is_nothrow_move_constructible<name_list>::value) &&
                   (std::is_nothrow_move_assignable<name_list>::value) ),
                 "a name_list moves without throwing");
D_STATIC_ASSERT(
    ( (std::is_nothrow_move_constructible<column_list>::value) &&
      (std::is_nothrow_move_assignable<column_list>::value) ),
    "a column_list moves without throwing");
D_STATIC_ASSERT( ( (std::is_nothrow_move_constructible<rows>::value) &&
                   (std::is_nothrow_move_assignable<rows>::value) ),
                 "rows move without throwing");
#endif  // D_MOVE_ENABLED


}  // namespace face
}  // namespace djinterp


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_DB_FACE_HPP
