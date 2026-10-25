/*******************************************************************************
* djinterp [parse]                                                  arg_type.hpp
*
* The argument parser's C++ face: value types.
*   A C++ type becomes the d_arg_type descriptor the C library converts
* through, at compile time, by cli::type_traits<T>: descriptor() is the
* descriptor, target() the address the C converter writes and source() the
* one its writer reads. The arithmetic types, `bool`, the character types,
* `const char*` and text map onto the C library's own descriptors, so an
* `int` bound from C++ converts through the descriptor C binds one through,
* at the same cost. The face gives the rest descriptors of its own, as CLI11
* classifies a type: std::string; the standard containers -- vector, deque,
* list, the sets and the maps, and from C++11 the unordered ones; from
* C++11, enumerations; the tuple-like types -- std::pair, and from C++11
* std::tuple and std::array -- which take one value for each part; complex
* numbers; from C++17, std::optional and std::string_view; a type made from
* a std::string; with wide text, a type made from a std::wstring --
* std::wstring itself, and, where the system's own text is wide, as on
* Windows, one made from either, as std::filesystem::path is; from C++11, a
* type made from a number, and a std::chrono duration, read and written as
* its count; and, by default, a type read with operator>> and written with
* operator<<, as CLI11 reads and writes a type it knows nothing else of.
*   The conversions are CLI11's: an empty value is the type's
* value-initialized object; a container takes one element from each value,
* or from each group of values for elements taking more -- a map's pairs --
* is emptied by "{}", and its elements convert as the element type does; a
* streamed value must be read whole; a value made wide must be UTF-8. As the
* C library's converters do, the face's write their target only when they
* succeed. Below C++11, a type made from a string is known only by an
* implicit conversion, and a streamed type needs both operators; from C++11,
* the standard's traits tell, and a type without operator<< has no text
* form, which CLI11 writes as nothing. A duration is its count, which CLI11
* reads as an `int` and writes as a stream writes the duration -- nothing,
* or, from C++20, `9s`, which it cannot read back.
*   A type of the caller's own is given its own conversions by a full
* specialization of type_traits; a container of the caller's own by one
* deriving from container_traits.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRAITS
    ------
    1.  The trait
         1.  type_traits
    2.  Containers
         1.  container_traits
2.  USE
    ---
    1.  Converting
         1.  type_name
         2.  convert / to_text
3.  INTERNALS
    ---------
    1.  Selection
         1.  enable_if
         2.  builtin_of / builtin_fallback
         3.  is_container
         4.  remove_const
         5.  is_const
         6.  tuple_access / tuple_part
         7.  mutable_type
         8.  is_complex
         9.  is_optional
         10. text_abilities / wide_abilities
         11. number_category
         12. stream_abilities
         13. duration_of
         14. kind_of / is_kind
    2.  Failures
         1.  conversion_failure
    3.  Conversions
         1.  first_value / is_separator / quiet_context
         2.  string_assign / string_convert / string_format
         3.  stream_read and kin
         4.  container_fill / container_convert / container_format
         5.  container_descriptor
         6.  tuple_walk and kin
         7.  complex_fill / complex_convert
         8.  text_assign and kin
         9.  tuple_converter and kin
         10. number_fill and kin
         11. duration_fill and kin
         12. optional_fill / optional_convert / optional_format
         13. view_convert / view_format
         14. widen_text and kin
    4.  The default traits
         1.  default_traits
         2.  The built-in types
         3.  The text types
         4.  std::string
         5.  The standard containers
         6.  Enumerations
         7.  value_address
         8.  Tuples
         9.  Complex numbers
         10. Text-like types
         11. Number-like types
         12. Durations
         13. Optional values
         14. std::string_view
         15. Wide text-like types
4.  DEFINITIONS
    -----------
    1.  type_traits
    2.  container_traits
    3.  Use
    4.  Failures
    5.  Conversions
    6.  The default traits
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TYPE_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TYPE_HPP 1

// std
#include <complex>    // std::complex
#include <cstddef>    // std::size_t, std::ptrdiff_t
#include <deque>      // std::deque
#include <exception>  // std::exception
#include <istream>    // std::istream
#include <list>       // std::list
#include <map>        // std::map, std::multimap
#include <ostream>    // std::ostream
#include <set>        // std::set, std::multiset
#include <sstream>    // std::istringstream, std::ostringstream
#include <string>     // std::string
#include <utility>    // std::pair
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../env/env.h"             // language levels,
                                          // D_ENV_HAS_LONG_LONG
#include "../../../env/parse/cli/arg_parser/env_arg_parser.h"  // D_ENV_ARG_PARSER_WIDE_NATIVE
#include "./arg_common.hpp"               // status, text, guard, scratch
#include "./c/arg_argv.h"                 // d_arg_wide_from_utf8,
                                          // d_arg_utf8_from_wide
#include "./c/arg_type.h"                 // d_arg_type, d_arg_builtin,
                                          // d_arg_convert, d_arg_format
// std, from C++11
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <array>          // std::array
    #include <chrono>         // std::chrono::duration
    #include <tuple>          // std::tuple, std::get
    #include <type_traits>    // std::is_enum, std::underlying_type,
                              // std::is_assignable, std::is_constructible
    #include <unordered_map>  // std::unordered_map, std::unordered_multimap
    #include <unordered_set>  // std::unordered_set, std::unordered_multiset
#endif
// std, from C++17
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <optional>       // std::optional
    #include <string_view>    // std::string_view
#endif


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  TRAITS
//==============================================================================


// 1.1    The trait
//------------------------------------------------------------------------------
// 1.1.1
// type_traits
//   trait: how values of Type are read and written: `descriptor()`, the
// d_arg_type the C library converts through, which lives as long as the
// program; `target(Type&)`, the address its converter writes; and
// `source(const Type&)`, the address its writer reads. The default is
// internal::default_traits (section 3.4). A full specialization of the
// caller's own replaces it for a type -- one deriving from
// container_traits, for a container -- and is then used wherever the type
// is, the elements of a container included.
template<typename Type>
struct type_traits;

NS_INTERNAL

    // default_traits
    //   trait: type_traits' default; see 3.4.1.
    template<typename Type,
             typename Enable = void>
    struct default_traits;

NS_END  // internal

// 1.2    Containers
//------------------------------------------------------------------------------
// 1.2.1
// container_traits
//   trait: a container's descriptor: CLI11's conversion of a container of
// single values -- each value one element, converted as type_traits<value_type>
// converts it, inserted at the end; "{}" alone an empty container -- and its
// writing, "[a,b]", or "{}" when empty. The descriptor is named, sized and
// flagged as its element is, with D_ARG_TYPE_FLAG_CONTAINER beside the
// element's flags, and expects an unlimited number of values; it is made the
// first time it is asked for. Container needs `value_type`, `begin()`, `end()`,
// `empty()`, `swap()`, and `insert(end(), value)`, as CLI11's mutable
// containers do.
template<typename Container>
struct container_traits
{
    static const ::d_arg_type* descriptor();
    static void*               target(Container& _value) D_NOEXCEPT;
    static const void*         source(const Container& _value) D_NOEXCEPT;
};


//==============================================================================
// 2.  USE
//==============================================================================


// 2.1    Converting
//------------------------------------------------------------------------------
// 2.1.1
// type_name
//   function: the name help shows for Type -- "INT", "FLOAT", "TEXT" -- as
// its descriptor names it. Neither allocates nor fails.
template<typename Type>
const char* type_name();

// 2.1.2
// convert / to_text
/**
 * @brief Converts a value as an option of the type would; CLI11's
 *        lexical_cast.
 *
 * @note An empty value is the type's value-initialized object, as CLI11's
 *       lexical_assign makes it. Integers read under the configured rule,
 *       D_INTERNAL_ARG_PARSER_INTEGER_RULE, or the one given.
 *
 * @param[in]  _value   the value.
 * @param[out] _target  receives the converted value; written only on
 *                      success.
 * @return status_code::ok, or why the value does not convert:
 *         status_code::conversion_error, status_code::out_of_memory.
 */
template<typename Type>
status      convert(const text& _value,
                    Type&       _target);
/**
 * @brief Converts a value under an integer rule, as convert(value, target).
 *
 * @param[in]  _value   the value.
 * @param[in]  _rule    the rule integers with a leading zero read by.
 * @param[out] _target  receives the converted value; written only on
 *                      success.
 * @return a status, as convert(value, target)'s.
 */
template<typename Type>
status      convert(const text&         _value,
                    integer_rule::value _rule,
                    Type&               _target);
/**
 * @brief Appends a value's text, as the type writes it for help and
 *        configuration files.
 *
 * @param[in]     _value  the value.
 * @param[in,out] _out    receives the text, appended.
 * @return status_code::ok; status_code::unsupported for a type that has no
 *         text form; status_code::file_error when the string could not
 *         grow.
 */
template<typename Type>
status      to_text(const Type&  _value,
                    std::string& _out);


//==============================================================================
// 3.  INTERNALS
//==============================================================================


// 3.1    Selection
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.1.1
    // enable_if
    //   trait: std::enable_if, at every language level: `type` is Type when
    //   Condition holds, and absent otherwise.
    template<bool     Condition,
             typename Type = void>
    struct enable_if
    {};

    template<typename Type>
    struct enable_if<true, Type>
    {
        typedef Type type;
    };

    // 3.1.2
    // builtin_of / builtin_fallback
    //   traits: the d_arg_builtin that describes Type, as `value`, or -1 for
    //   a type that none describes. The fixed-width and `size_t` types are
    //   these types under other names, and are found as them; where
    //   std::size_t or std::ptrdiff_t is a type no specialization names --
    //   `unsigned long long` and `long long` where C++98 strict has no such
    //   types, on a system whose `long` is narrower than a pointer -- it is
    //   found by builtin_fallback, as the C library's SIZE and PTRDIFF.
    template<typename Type>
    struct builtin_fallback
    {
        enum { value = -1 };
    };

    template<>
    struct builtin_fallback<std::size_t>
    {
        enum { value = D_ARG_BUILTIN_SIZE };
    };

    template<>
    struct builtin_fallback<std::ptrdiff_t>
    {
        enum { value = D_ARG_BUILTIN_PTRDIFF };
    };

    template<typename Type>
    struct builtin_of
    {
        enum { value = builtin_fallback<Type>::value };
    };

    template<>
    struct builtin_of<bool>
    {
        enum { value = D_ARG_BUILTIN_BOOL };
    };

    template<>
    struct builtin_of<char>
    {
        enum { value = D_ARG_BUILTIN_CHAR };
    };

    template<>
    struct builtin_of<signed char>
    {
        enum { value = D_ARG_BUILTIN_SIGNED_CHAR };
    };

    template<>
    struct builtin_of<unsigned char>
    {
        enum { value = D_ARG_BUILTIN_UNSIGNED_CHAR };
    };

    template<>
    struct builtin_of<short>
    {
        enum { value = D_ARG_BUILTIN_SHORT };
    };

    template<>
    struct builtin_of<unsigned short>
    {
        enum { value = D_ARG_BUILTIN_UNSIGNED_SHORT };
    };

    template<>
    struct builtin_of<int>
    {
        enum { value = D_ARG_BUILTIN_INT };
    };

    template<>
    struct builtin_of<unsigned int>
    {
        enum { value = D_ARG_BUILTIN_UNSIGNED_INT };
    };

    template<>
    struct builtin_of<long>
    {
        enum { value = D_ARG_BUILTIN_LONG };
    };

    template<>
    struct builtin_of<unsigned long>
    {
        enum { value = D_ARG_BUILTIN_UNSIGNED_LONG };
    };

#if D_ENV_HAS_LONG_LONG
D_LONG_LONG_DIAG_PUSH

    template<>
    struct builtin_of<long long>
    {
        enum { value = D_ARG_BUILTIN_LONG_LONG };
    };

    template<>
    struct builtin_of<unsigned long long>
    {
        enum { value = D_ARG_BUILTIN_UNSIGNED_LONG_LONG };
    };

D_LONG_LONG_DIAG_POP
#endif  // D_ENV_HAS_LONG_LONG

    template<>
    struct builtin_of<float>
    {
        enum { value = D_ARG_BUILTIN_FLOAT };
    };

    template<>
    struct builtin_of<double>
    {
        enum { value = D_ARG_BUILTIN_DOUBLE };
    };

    template<>
    struct builtin_of<long double>
    {
        enum { value = D_ARG_BUILTIN_LONG_DOUBLE };
    };

    template<>
    struct builtin_of<const char*>
    {
        enum { value = D_ARG_BUILTIN_STRING };
    };

    template<>
    struct builtin_of< ::d_pack_text>
    {
        enum { value = D_ARG_BUILTIN_TEXT };
    };

    // 3.1.3
    // is_container
    //   trait: whether Type is one of the standard containers the face gives
    //   container_traits to, as `value`: the sequences and sets, and the
    //   maps, whose elements are pairs, as CLI11's mutable containers are.
    template<typename Type>
    struct is_container
    {
        enum { value = 0 };
    };

    template<typename Element,
             typename Allocator>
    struct is_container<std::vector<Element, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Element,
             typename Allocator>
    struct is_container<std::deque<Element, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Element,
             typename Allocator>
    struct is_container<std::list<Element, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Element,
             typename Compare,
             typename Allocator>
    struct is_container<std::set<Element, Compare, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Element,
             typename Compare,
             typename Allocator>
    struct is_container<std::multiset<Element, Compare, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Key,
             typename Value,
             typename Compare,
             typename Allocator>
    struct is_container<std::map<Key, Value, Compare, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Key,
             typename Value,
             typename Compare,
             typename Allocator>
    struct is_container<std::multimap<Key, Value, Compare, Allocator> >
    {
        enum { value = 1 };
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    template<typename Element,
             typename Hash,
             typename Equal,
             typename Allocator>
    struct is_container<std::unordered_set<Element, Hash, Equal, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Element,
             typename Hash,
             typename Equal,
             typename Allocator>
    struct is_container<
               std::unordered_multiset<Element, Hash, Equal, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Key,
             typename Value,
             typename Hash,
             typename Equal,
             typename Allocator>
    struct is_container<
               std::unordered_map<Key, Value, Hash, Equal, Allocator> >
    {
        enum { value = 1 };
    };

    template<typename Key,
             typename Value,
             typename Hash,
             typename Equal,
             typename Allocator>
    struct is_container<
               std::unordered_multimap<Key, Value, Hash, Equal, Allocator> >
    {
        enum { value = 1 };
    };

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.1.4
    // remove_const
    //   trait: std::remove_const, at every language level: a map's keys are
    //   const, and are converted and written as the type they hold.
    template<typename Type>
    struct remove_const
    {
        typedef Type type;
    };

    template<typename Type>
    struct remove_const<const Type>
    {
        typedef Type type;
    };

    // 3.1.5
    // is_const
    //   trait: std::is_const, at every language level, as `value`: a tuple
    //   whose parts are const -- a map's pair -- is written, not converted
    //   into; and an argument after a name is a description, as CLI11 tells
    //   one from a variable to bind.
    template<typename Type>
    struct is_const
    {
        enum { value = 0 };
    };

    template<typename Type>
    struct is_const<const Type>
    {
        enum { value = 1 };
    };

    template<typename Type,
             std::size_t Size>
    struct is_const<const Type[Size]>
    {
        enum { value = 1 };
    };

    // 3.1.6
    // tuple_access / tuple_part
    //   traits: CLI11's tuple-like types -- std::pair, and from C++11
    //   std::tuple and std::array -- as `size`, the number of parts, 0 for
    //   any other type, and `writable`, whether no part is const; and part
    //   Index of one, as `type` and `get()`.
    template<typename Type>
    struct tuple_access
    {
        enum { size = 0, writable = 0 };
    };

    template<typename Type,
             std::size_t Index>
    struct tuple_part;

    template<typename First,
             typename Second>
    struct tuple_access<std::pair<First, Second> >
    {
        enum
        {
            size     = 2,
            writable = ( (!is_const<First>::value) &&
                         (!is_const<Second>::value) )
        };
    };

    template<typename First,
             typename Second>
    struct tuple_part<std::pair<First, Second>, 0u>
    {
        typedef First type;

        static type&       get(std::pair<First, Second>& _value) D_NOEXCEPT;
        static const type& get(const std::pair<First, Second>& _value)
                               D_NOEXCEPT;
    };

    template<typename First,
             typename Second>
    struct tuple_part<std::pair<First, Second>, 1u>
    {
        typedef Second type;

        static type&       get(std::pair<First, Second>& _value) D_NOEXCEPT;
        static const type& get(const std::pair<First, Second>& _value)
                               D_NOEXCEPT;
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    template<typename... Parts>
    struct all_writable;

    template<>
    struct all_writable<>
    {
        enum { value = 1 };
    };

    template<typename Part,
             typename... Rest>
    struct all_writable<Part, Rest...>
    {
        enum
        {
            value = ( (!is_const<Part>::value) &&
                      (all_writable<Rest...>::value) )
        };
    };

    template<typename... Parts>
    struct tuple_access<std::tuple<Parts...> >
    {
        enum
        {
            size     = sizeof...(Parts),
            writable = all_writable<Parts...>::value
        };
    };

    template<std::size_t Index,
             typename... Parts>
    struct tuple_part<std::tuple<Parts...>, Index>
    {
        typedef typename std::tuple_element<Index,
                                            std::tuple<Parts...> >::type type;

        static type&       get(std::tuple<Parts...>& _value) D_NOEXCEPT;
        static const type& get(const std::tuple<Parts...>& _value) D_NOEXCEPT;
    };

    template<typename Element,
             std::size_t Size>
    struct tuple_access<std::array<Element, Size> >
    {
        enum
        {
            size     = Size,
            writable = (!is_const<Element>::value)
        };
    };

    template<std::size_t Index,
             typename Element,
             std::size_t Size>
    struct tuple_part<std::array<Element, Size>, Index>
    {
        typedef Element type;

        static type&       get(std::array<Element, Size>& _value) D_NOEXCEPT;
        static const type& get(const std::array<Element, Size>& _value)
                               D_NOEXCEPT;
    };

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.1.7
    // mutable_type
    //   trait: the type a container's element is converted into before it is
    //   inserted: a map's std::pair<const Key, Value> as std::pair<Key,
    //   Value>, any other as itself.
    template<typename Type>
    struct mutable_type
    {
        typedef Type type;
    };

    template<typename Key,
             typename Value>
    struct mutable_type<std::pair<const Key, Value> >
    {
        typedef std::pair<Key, Value> type;
    };

    // 3.1.8
    // is_complex
    //   trait: whether Type is a std::complex, as `value`.
    template<typename Type>
    struct is_complex
    {
        enum { value = 0 };
    };

    template<typename Part>
    struct is_complex<std::complex<Part> >
    {
        enum { value = 1 };
    };

    // 3.1.9
    // is_optional
    //   trait: whether Type is a std::optional, from C++17, as `value`: CLI11's
    //   wrapper type.
    template<typename Type>
    struct is_optional
    {
        enum { value = 0 };
    };

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    template<typename Inner>
    struct is_optional<std::optional<Inner> >
    {
        enum { value = 1 };
    };

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    // 3.1.10
    // text_abilities / wide_abilities
    //   traits: how Type meets a string -- a std::string, and, for
    //   wide_abilities, a std::wstring -- as CLI11 classifies it: `from`, it
    //   is assigned or made from one, CLI11's string_assignable and
    //   string_constructible, or wstring_assignable and
    //   wstring_constructible, with `assigns` telling which; and `to`, it
    //   converts to one, as the type's text is written. From C++11 by the
    //   standard's traits; below, by implicit conversion alone, which an
    //   explicit constructor or conversion does not give. Wide text exists
    //   with D_INTERNAL_ARG_PARSER_WIDE_ARGV; without it, no type is made
    //   from a std::wstring.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    template<typename Type,
             typename String>
    struct string_abilities
    {
        enum
        {
            assigns = std::is_assignable<Type&, const String&>::value,
            from    = ( (assigns) ||
                        (std::is_constructible<Type, const String&>::value) ),
            to      = std::is_constructible<String, const Type&>::value
        };
    };

#else

    template<typename Type,
             typename String>
    struct string_abilities
    {
        static char (&from_probe(const Type& _value))[1];
        static char (&from_probe(...))[2];
        static char (&to_probe(const String& _value))[1];
        static char (&to_probe(...))[2];
        static const String& make_string();
        static const Type&   make_value();

        enum
        {
            assigns = 0,
            from    = (sizeof(from_probe(make_string())) == 1u),
            to      = (sizeof(to_probe(make_value())) == 1u)
        };
    };

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    template<typename Type>
    struct text_abilities
        : public string_abilities<Type, std::string>
    {};

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

    template<typename Type>
    struct wide_abilities
        : public string_abilities<Type, std::wstring>
    {};

#else

    template<typename Type>
    struct wide_abilities
    {
        enum
        {
            assigns = 0,
            from    = 0,
            to      = 0
        };
    };

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.1.11
    // number_category
    //   trait: from C++11, how a type that is neither text nor a number of the
    //   C library's is made from a number, as `value`: CLI11's
    //   number_constructible (both), integer_constructible (an `int`) and
    //   double_constructible (a `double`), or none. An aggregate is made from
    //   neither -- C++20 would make `struct { int x, y; }` from an `int`, as
    //   CLI11 then does -- so that it is read with its stream operators at
    //   every level.
    struct number_kind
    {
        enum value
        {
            none    = 0,
            both    = 1,
            integer = 2,
            real    = 3
        };
    };

    template<typename Type>
    struct aggregate_of
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        enum { value = std::is_aggregate<Type>::value };
#else
        enum { value = 0 };
#endif
    };

    template<typename Type>
    struct number_category
    {
        enum
        {
            from_int    = ( (std::is_constructible<Type, int>::value) &&
                            (!aggregate_of<Type>::value) ),
            from_double = ( (std::is_constructible<Type, double>::value) &&
                            (!aggregate_of<Type>::value) ),
            value       = (from_int && from_double)
                          ? number_kind::both
                          : from_int
                            ? number_kind::integer
                            : from_double
                              ? number_kind::real
                              : number_kind::none
        };
    };

    // 3.1.12
    // stream_abilities
    //   trait: whether Type is read with operator>>, as `readable`, and
    //   written with operator<<, as `writable`: CLI11's is_istreamable and
    //   is_ostreamable, from C++11. Below, both are assumed, as the stream
    //   default needs them.
    template<typename Type,
             typename Enable = void>
    struct stream_readable
    {
        enum { value = 0 };
    };

    template<typename Type>
    struct stream_readable<
               Type,
               typename enable_if<
                   (sizeof(std::declval<std::istream&>() >>
                           std::declval<Type&>()) != 0u)>::type>
    {
        enum { value = 1 };
    };

    template<typename Type,
             typename Enable = void>
    struct stream_writable
    {
        enum { value = 0 };
    };

    template<typename Type>
    struct stream_writable<
               Type,
               typename enable_if<
                   (sizeof(std::declval<std::ostream&>() <<
                           std::declval<const Type&>()) != 0u)>::type>
    {
        enum { value = 1 };
    };

    template<typename Type>
    struct stream_abilities
    {
        enum
        {
            readable = stream_readable<Type>::value,
            writable = stream_writable<Type>::value
        };
    };

#else

    template<typename Type>
    struct stream_abilities
    {
        enum
        {
            readable = 1,
            writable = 1
        };
    };

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.1.13
    // duration_of
    //   trait: whether Type is a std::chrono::duration, as `value`, and, for
    //   one, the type of its count, as `count_type`. A duration is made from
    //   a number, as CLI11 makes one, but is read and written as its count,
    //   through the count's own descriptor, so that the text it is written
    //   as reads back as it.
    template<typename Type>
    struct duration_of
    {
        enum { value = 0 };
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    template<typename Count,
             typename Period>
    struct duration_of<std::chrono::duration<Count, Period> >
    {
        enum { value = 1 };

        typedef Count count_type;
    };

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.1.14
    // kind_of / is_kind
    //   traits: which of the default traits describes Type, as `value`, in
    //   CLI11's order of classification: a type of the C library's, a
    //   standard container, an enumeration, a tuple, a complex number, an
    //   optional value, text-like, made from wide text, a duration,
    //   number-like, or, by default, a type read and written with streams;
    //   and whether it is the kind given. A type made from both a
    //   std::string and a std::wstring is made from the wide one where the
    //   system's own text is wide (D_ENV_ARG_PARSER_WIDE_NATIVE), as CLI11
    //   makes it under MSVC, so that UTF-8 becomes a Windows path exactly.
    struct type_kind
    {
        enum value
        {
            stream      = 0,
            builtin     = 1,
            container   = 2,
            enumeration = 3,
            tuple       = 4,
            complex     = 5,
            optional    = 6,
            text_like   = 7,
            number_like = 8,
            wide_text   = 9,
            duration    = 10
        };
    };

    template<typename Type>
    struct enum_of
    {
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        enum { value = std::is_enum<Type>::value };
#else
        enum { value = 0 };
#endif
    };

    template<typename Type>
    struct number_of
    {
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        enum
        {
            value = (static_cast<int>(number_category<Type>::value) !=
                     static_cast<int>(number_kind::none))
        };
#else
        enum { value = 0 };
#endif
    };

    template<typename Type>
    struct narrow_of
    {
        enum
        {
            value = ( (text_abilities<Type>::from == 1) &&
                      ( (D_ENV_ARG_PARSER_WIDE_NATIVE == 0) ||
                        (wide_abilities<Type>::from == 0) ) )
        };
    };

    template<typename Type>
    struct kind_of
    {
        enum
        {
            value = (builtin_of<Type>::value >= 0)
                    ? type_kind::builtin
                    : (is_container<Type>::value == 1)
                      ? type_kind::container
                      : (enum_of<Type>::value == 1)
                        ? type_kind::enumeration
                        : (tuple_access<Type>::size > 0)
                          ? type_kind::tuple
                          : (is_complex<Type>::value == 1)
                            ? type_kind::complex
                            : (is_optional<Type>::value == 1)
                              ? type_kind::optional
                              : (narrow_of<Type>::value == 1)
                                ? type_kind::text_like
                                : (wide_abilities<Type>::from == 1)
                                  ? type_kind::wide_text
                                  : (duration_of<Type>::value == 1)
                                    ? type_kind::duration
                                    : (number_of<Type>::value == 1)
                                      ? type_kind::number_like
                                      : type_kind::stream
        };
    };

    template<typename Type,
             int Kind>
    struct is_kind
    {
        enum { value = (static_cast<int>(kind_of<Type>::value) == Kind) };
    };

NS_END  // internal

// 3.2    Failures
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.2.1
    // conversion_failure
    //   class: the handler of guard for a converter or a writer: memory that
    //   ran out is status_code::out_of_memory, as the C library reports a
    //   refused allocation; any other exception is a conversion error, its
    //   what(), when it holds anything, kept as the reason when the
    //   conversion asked for one.
    class conversion_failure
    {
    public:
        typedef int result_type;

        explicit conversion_failure(
            const ::d_arg_convert_context* _context) D_NOEXCEPT;

        int operator()(const std::exception* _exception,
                       bool                  _out_of_memory) const;

    private:
        const ::d_arg_convert_context* m_context;
    };

NS_END  // internal

// 3.3    Conversions
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.3.1
    // first_value / is_separator / quiet_context
    //   functions: the first of a converter's values, or the empty text when
    //   it was given none, as the C library's scalar converters read them;
    //   whether a value separates groups -- empty, or "%%" -- as CLI11's
    //   is_separator judges; and a converter's context without its buffer
    //   for reasons, for a conversion tried as one way among others, whose
    //   failure is no error -- the configured integer rule without a
    //   context. None allocates or fails.
    text first_value(const ::d_pack_text* _values,
                     std::size_t          _count) D_NOEXCEPT;
    bool is_separator(const text& _value) D_NOEXCEPT;
    ::d_arg_convert_context quiet_context(
        const ::d_arg_convert_context* _context) D_NOEXCEPT;

    // 3.3.2
    // string_assign / string_convert / string_format
    //   class, functions: std::string's conversion -- the first value,
    //   copied, as CLI11's lexical_assign copies it -- run under guard, and
    //   its writing, the string itself.
    class string_assign
    {
    public:
        typedef int result_type;

        string_assign(const ::d_pack_text* _values,
                      std::size_t          _count,
                      void*                _target) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text* m_values;
        std::size_t          m_count;
        std::string*         m_target;
    };
    int  string_convert(const ::d_arg_type*            _type,
                        const ::d_pack_text*           _values,
                        std::size_t                    _count,
                        void*                          _target,
                        const ::d_arg_convert_context* _context);
    int  string_format(const ::d_arg_type* _type,
                       const void*         _value,
                       ::d_pack_sink       _sink);

    // 3.3.3
    // stream_read and kin
    //   classes, functions: a type known by its stream operators, as CLI11's
    //   from_stream and to_string know it: an empty value is Type(); any
    //   other is read with operator>> into a Type() and must be read whole,
    //   nothing failing and nothing left -- stream_parse, which the other
    //   conversions fall back to, as CLI11's do -- written with operator<<.
    template<typename Type>
    class stream_read
    {
    public:
        typedef int result_type;

        stream_read(const ::d_pack_text* _values,
                    std::size_t          _count,
                    void*                _target) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text* m_values;
        std::size_t          m_count;
        Type*                m_target;
    };
    template<typename Type>
    int  stream_convert(const ::d_arg_type*            _type,
                        const ::d_pack_text*           _values,
                        std::size_t                    _count,
                        void*                          _target,
                        const ::d_arg_convert_context* _context);
    template<typename Type>
    class stream_write
    {
    public:
        typedef int result_type;

        stream_write(const void*   _value,
                     ::d_pack_sink _sink) D_NOEXCEPT;

        int operator()() const;

    private:
        const Type*   m_value;
        ::d_pack_sink m_sink;
    };
    template<typename Type>
    int  stream_format(const ::d_arg_type* _type,
                       const void*         _value,
                       ::d_pack_sink       _sink);
    template<typename Type>
    int  stream_parse(const text& _value,
                      Type&       _target);

    // 3.3.4
    // container_fill / container_convert / container_format
    //   class, functions: a container's conversion, as CLI11's
    //   lexical_conversion of a container: of single values, no values, or
    //   "{}" alone, an empty container; otherwise each value an element,
    //   converted through the element's descriptor and inserted at the end,
    //   but for "{}" followed by a separator, which makes one element and
    //   stops. Of elements taking more -- pairs, a map's included, and other
    //   tuples -- each group of values an element: a pair's two, the last
    //   left alone failing; a larger tuple's as many as it takes, a
    //   separator ending a group early. A map's element converts as a pair
    //   whose key is not const. The elements are built in a container of the
    //   converter's own, swapped into the target only when every one
    //   converted. Writing is "[a,b]", each element as its type writes it,
    //   or "{}" when empty.
    template<typename Container>
    class container_fill
    {
    public:
        typedef int result_type;

        container_fill(const ::d_pack_text*           _values,
                       std::size_t                    _count,
                       void*                          _target,
                       const ::d_arg_convert_context* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        int         single(const ::d_arg_type* _element) const;
        int         grouped(const ::d_arg_type* _element,
                            std::size_t         _step) const;
        std::size_t group_size(std::size_t _index,
                               std::size_t _step) const;
        int         keep(Container& _filled) const;

        const ::d_pack_text*           m_values;
        std::size_t                    m_count;
        Container*                     m_target;
        const ::d_arg_convert_context* m_context;
    };
    template<typename Container>
    int  container_convert(const ::d_arg_type*            _type,
                           const ::d_pack_text*           _values,
                           std::size_t                    _count,
                           void*                          _target,
                           const ::d_arg_convert_context* _context);
    template<typename Container>
    int  container_format(const ::d_arg_type* _type,
                          const void*         _value,
                          ::d_pack_sink       _sink);

    // 3.3.5
    // container_descriptor
    //   function: a container's descriptor, made from its element's: the
    //   element's name, sizes and flags, D_ARG_TYPE_FLAG_CONTAINER added,
    //   the element's descriptor as `data`, and an unlimited number of
    //   values expected. Neither allocates nor fails.
    template<typename Container>
    ::d_arg_type container_descriptor() D_NOEXCEPT;

    // 3.3.6
    // tuple_walk and kin
    //   structs, class, functions: a tuple-like type's conversion, as
    //   CLI11's tuple_conversion: each part in turn takes as many values as
    //   its type does -- its descriptor's most -- converted through its own
    //   descriptor, the values running out leaving the parts after them as
    //   made, into a Tuple() of the converter's own that replaces the target
    //   only when every part converted; its writing, "[a,b]", each part as
    //   its type writes it; and its shape: the parts' names as CLI11's
    //   tuple_name joins them, "[INT,TEXT]", and the sums of their sizes,
    //   kept with the descriptor made from them.
    template<typename Tuple,
             std::size_t Index,
             std::size_t Size>
    struct tuple_walk
    {
        static int  convert(const ::d_pack_text*           _values,
                            std::size_t                    _count,
                            std::size_t                    _at,
                            Tuple&                         _target,
                            const ::d_arg_convert_context* _context);
        static int  format(const Tuple&  _value,
                           ::d_pack_sink _sink);
        static void shape(std::string& _name,
                          int&         _minimum,
                          int&         _maximum);
    };

    template<typename Tuple,
             std::size_t Size>
    struct tuple_walk<Tuple, Size, Size>
    {
        static int  convert(const ::d_pack_text*           _values,
                            std::size_t                    _count,
                            std::size_t                    _at,
                            Tuple&                         _target,
                            const ::d_arg_convert_context* _context)
                        D_NOEXCEPT;
        static int  format(const Tuple&  _value,
                           ::d_pack_sink _sink) D_NOEXCEPT;
        static void shape(std::string& _name,
                          int&         _minimum,
                          int&         _maximum) D_NOEXCEPT;
    };

    template<typename Tuple>
    class tuple_fill
    {
    public:
        typedef int result_type;

        tuple_fill(const ::d_pack_text*           _values,
                   std::size_t                    _count,
                   void*                          _target,
                   const ::d_arg_convert_context* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text*           m_values;
        std::size_t                    m_count;
        Tuple*                         m_target;
        const ::d_arg_convert_context* m_context;
    };
    template<typename Tuple>
    int  tuple_convert(const ::d_arg_type*            _type,
                       const ::d_pack_text*           _values,
                       std::size_t                    _count,
                       void*                          _target,
                       const ::d_arg_convert_context* _context);
    template<typename Tuple>
    int  tuple_format(const ::d_arg_type* _type,
                      const void*         _value,
                      ::d_pack_sink       _sink);
    template<typename Tuple>
    void tuple_shape(std::string& _name,
                     int&         _minimum,
                     int&         _maximum);
    template<typename Tuple>
    struct tuple_description
    {
        tuple_description();

        std::string  name;
        ::d_arg_type descriptor;

    private:
        D_DELETED_FN(tuple_description(const tuple_description& _other))
        D_DELETED_FN(tuple_description& operator=(
                         const tuple_description& _other))
    };

    // 3.3.7
    // complex_fill / complex_convert
    //   class, function: a std::complex's conversion, as CLI11's: two values,
    //   the real part and the imaginary, an `i` or `j` after it dropped; or
    //   one -- "1+2i", "-3j", "4" -- split at its last sign, an imaginary
    //   part known by its `i` or `j`; else read with operator>>, as "(1,2)"
    //   is; an empty value is (0,0). The parts convert as their type does.
    //   It is written with operator<<, as CLI11 writes one.
    template<typename Complex>
    class complex_fill
    {
    public:
        typedef int result_type;

        complex_fill(const ::d_pack_text*           _values,
                     std::size_t                    _count,
                     void*                          _target,
                     const ::d_arg_convert_context* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        bool part(const text&                   _value,
                  typename Complex::value_type& _out) const;
        bool imaginary(text                          _value,
                       typename Complex::value_type& _out) const;
        bool one(const text&                   _value,
                 typename Complex::value_type& _real,
                 typename Complex::value_type& _imag) const;

        const ::d_pack_text*           m_values;
        std::size_t                    m_count;
        Complex*                       m_target;
        const ::d_arg_convert_context* m_context;
    };
    template<typename Complex>
    int  complex_convert(const ::d_arg_type*            _type,
                         const ::d_pack_text*           _values,
                         std::size_t                    _count,
                         void*                          _target,
                         const ::d_arg_convert_context* _context);

    // 3.3.8
    // text_assign and kin
    //   classes, functions: a type made from text -- CLI11's
    //   string_assignable and string_constructible -- given the value as a
    //   std::string, assigned where it may be -- from C++11, as the
    //   standard's traits tell -- else made from it; and written as the
    //   std::string it converts to.
    template<typename Type>
    class text_assign
    {
    public:
        typedef int result_type;

        text_assign(const ::d_pack_text* _values,
                    std::size_t          _count,
                    void*                _target) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text* m_values;
        std::size_t          m_count;
        Type*                m_target;
    };
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename Type,
             typename String,
             bool Assigns>
    struct text_store
    {
        static void store(Type&         _target,
                          const String& _value);
    };

    template<typename Type,
             typename String>
    struct text_store<Type, String, false>
    {
        static void store(Type&         _target,
                          const String& _value);
    };
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename Type>
    int  text_like_convert(const ::d_arg_type*            _type,
                           const ::d_pack_text*           _values,
                           std::size_t                    _count,
                           void*                          _target,
                           const ::d_arg_convert_context* _context);
    template<typename Type>
    class text_write
    {
    public:
        typedef int result_type;

        text_write(const void*   _value,
                   ::d_pack_sink _sink) D_NOEXCEPT;

        int operator()() const;

    private:
        const Type*   m_value;
        ::d_pack_sink m_sink;
    };
    template<typename Type>
    int  text_like_format(const ::d_arg_type* _type,
                          const void*         _value,
                          ::d_pack_sink       _sink);

    // 3.3.9
    // tuple_converter and kin
    //   traits: a descriptor's functions where a type may lack one, as `get()`:
    //   the function, or NULL when the type cannot have it, so that a
    //   function the type cannot have is never made -- a tuple with a const
    //   part, a map's pair, is written but not converted into; a type
    //   without operator<<, or without a std::string to become, has no text
    //   form, as CLI11 writes it as nothing.
    template<typename Tuple,
             bool Writable>
    struct tuple_converter
    {
        static ::d_arg_convert_fn get() D_NOEXCEPT;
    };

    template<typename Tuple>
    struct tuple_converter<Tuple, false>
    {
        static ::d_arg_convert_fn get() D_NOEXCEPT;
    };

    template<typename Type,
             bool Readable>
    struct stream_converter
    {
        static ::d_arg_convert_fn get() D_NOEXCEPT;
    };

    template<typename Type>
    struct stream_converter<Type, false>
    {
        static ::d_arg_convert_fn get() D_NOEXCEPT;
    };

    template<typename Type,
             bool Writable>
    struct stream_formatter
    {
        static ::d_arg_format_fn get() D_NOEXCEPT;
    };

    template<typename Type>
    struct stream_formatter<Type, false>
    {
        static ::d_arg_format_fn get() D_NOEXCEPT;
    };

    template<typename Type,
             bool Converts>
    struct text_formatter
    {
        static ::d_arg_format_fn get() D_NOEXCEPT;
    };

    template<typename Type>
    struct text_formatter<Type, false>
    {
        static ::d_arg_format_fn get() D_NOEXCEPT;
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.3.10
    // number_fill and kin
    //   class, traits, function: from C++11, a type made from a number --
    //   CLI11's number_constructible, integer_constructible and
    //   double_constructible -- given the value read as an `int`, or as a
    //   `double`, as its category allows and in that order, else read with
    //   operator>> where it can be; an empty value is Type(). Each trait
    //   tries one way, or, where the type cannot be made that way, none.
    template<typename Type>
    class number_fill
    {
    public:
        typedef int result_type;

        number_fill(const ::d_pack_text*           _values,
                    std::size_t                    _count,
                    void*                          _target,
                    const ::d_arg_convert_context* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text*           m_values;
        std::size_t                    m_count;
        Type*                          m_target;
        const ::d_arg_convert_context* m_context;
    };
    template<typename Type,
             bool Able>
    struct from_int
    {
        static bool store(const text&                    _value,
                          Type&                          _target,
                          const ::d_arg_convert_context& _context);
    };

    template<typename Type>
    struct from_int<Type, false>
    {
        static bool store(const text&                    _value,
                          Type&                          _target,
                          const ::d_arg_convert_context& _context) D_NOEXCEPT;
    };

    template<typename Type,
             bool Able>
    struct from_double
    {
        static bool store(const text&                    _value,
                          Type&                          _target,
                          const ::d_arg_convert_context& _context);
    };

    template<typename Type>
    struct from_double<Type, false>
    {
        static bool store(const text&                    _value,
                          Type&                          _target,
                          const ::d_arg_convert_context& _context) D_NOEXCEPT;
    };

    template<typename Type,
             bool Able>
    struct from_stream
    {
        static int store(const text& _value,
                         Type&       _target);
    };

    template<typename Type>
    struct from_stream<Type, false>
    {
        static int store(const text& _value,
                         Type&       _target) D_NOEXCEPT;
    };
    template<typename Type>
    int  number_convert(const ::d_arg_type*            _type,
                        const ::d_pack_text*           _values,
                        std::size_t                    _count,
                        void*                          _target,
                        const ::d_arg_convert_context* _context);

    // 3.3.11
    // duration_fill and kin
    //   classes, functions: from C++11, a std::chrono duration's conversion
    //   -- its count, read through the count's descriptor, made into the
    //   duration; an empty value Duration() -- and its writing, the count as
    //   the count's descriptor writes it.
    template<typename Duration>
    class duration_fill
    {
    public:
        typedef int result_type;

        duration_fill(const ::d_pack_text*           _values,
                      std::size_t                    _count,
                      void*                          _target,
                      const ::d_arg_convert_context* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text*           m_values;
        std::size_t                    m_count;
        Duration*                      m_target;
        const ::d_arg_convert_context* m_context;
    };
    template<typename Duration>
    int  duration_convert(const ::d_arg_type*            _type,
                          const ::d_pack_text*           _values,
                          std::size_t                    _count,
                          void*                          _target,
                          const ::d_arg_convert_context* _context);
    template<typename Duration>
    class duration_write
    {
    public:
        typedef int result_type;

        duration_write(const void*   _value,
                       ::d_pack_sink _sink) D_NOEXCEPT;

        int operator()() const;

    private:
        const Duration* m_value;
        ::d_pack_sink   m_sink;
    };
    template<typename Duration>
    int  duration_format(const ::d_arg_type* _type,
                         const void*         _value,
                         ::d_pack_sink       _sink);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // 3.3.12
    // optional_fill / optional_convert / optional_format
    //   class, functions: from C++17, a std::optional's conversion, as CLI11's
    //   wrapper conversion: no values, or an empty first value, empty it;
    //   any others convert as the type it holds, into a value of its own
    //   that replaces the target's only then. It is written as the value it
    //   holds, or as nothing when empty.
    template<typename Optional>
    class optional_fill
    {
    public:
        typedef int result_type;

        optional_fill(const ::d_pack_text*           _values,
                      std::size_t                    _count,
                      void*                          _target,
                      const ::d_arg_convert_context* _context) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text*           m_values;
        std::size_t                    m_count;
        Optional*                      m_target;
        const ::d_arg_convert_context* m_context;
    };
    template<typename Optional>
    int  optional_convert(const ::d_arg_type*            _type,
                          const ::d_pack_text*           _values,
                          std::size_t                    _count,
                          void*                          _target,
                          const ::d_arg_convert_context* _context);
    template<typename Optional>
    int  optional_format(const ::d_arg_type* _type,
                         const void*         _value,
                         ::d_pack_sink       _sink);

    // 3.3.13
    // view_convert / view_format
    //   functions: from C++17, a std::string_view: a view of the first value,
    //   which the parser keeps -- as the C library's TEXT descriptor's spans
    //   -- until its next parse, clear or release; written as the text it
    //   views.
    int  view_convert(const ::d_arg_type*            _type,
                      const ::d_pack_text*           _values,
                      std::size_t                    _count,
                      void*                          _target,
                      const ::d_arg_convert_context* _context) D_NOEXCEPT;
    int  view_format(const ::d_arg_type* _type,
                     const void*         _value,
                     ::d_pack_sink       _sink) D_NOEXCEPT;

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

    // 3.3.14
    // widen_text and kin
    //   functions, classes: with wide text, UTF-8 made a std::wstring --
    //   d_arg_wide_from_utf8's status, `_out` changed only for D_ARG_OK --
    //   and wide text written to a sink as UTF-8; and a type made from wide
    //   text -- CLI11's wstring_assignable and wstring_constructible -- given
    //   the value widened, as CLI11's widen() makes it, assigned where it may
    //   be, else made from it, a value that is not UTF-8 refused as one that
    //   does not convert; and written as the std::wstring it converts to,
    //   made UTF-8, else as the std::string it converts to, else not at all
    //   -- wide_formatter's `get()` giving the function, or NULL.
    int  widen_text(const text&   _text,
                    std::wstring& _out);
    int  narrow_text(const std::wstring& _text,
                     ::d_pack_sink       _sink);
    template<typename Type>
    class wide_assign
    {
    public:
        typedef int result_type;

        wide_assign(const ::d_pack_text* _values,
                    std::size_t          _count,
                    void*                _target) D_NOEXCEPT;

        int operator()() const;

    private:
        const ::d_pack_text* m_values;
        std::size_t          m_count;
        Type*                m_target;
    };
    template<typename Type>
    int  wide_like_convert(const ::d_arg_type*            _type,
                           const ::d_pack_text*           _values,
                           std::size_t                    _count,
                           void*                          _target,
                           const ::d_arg_convert_context* _context);
    template<typename Type>
    class wide_write
    {
    public:
        typedef int result_type;

        wide_write(const void*   _value,
                   ::d_pack_sink _sink) D_NOEXCEPT;

        int operator()() const;

    private:
        const Type*   m_value;
        ::d_pack_sink m_sink;
    };
    template<typename Type>
    int  wide_like_format(const ::d_arg_type* _type,
                          const void*         _value,
                          ::d_pack_sink       _sink);
    template<typename Type,
             bool Converts>
    struct wide_formatter
    {
        static ::d_arg_format_fn get() D_NOEXCEPT;
    };

    template<typename Type>
    struct wide_formatter<Type, false>
    {
        static ::d_arg_format_fn get() D_NOEXCEPT;
    };

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

NS_END  // internal

// 3.4    The default traits
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.4.1
    // default_traits
    //   trait: type_traits' default, by what Type is (kind_of): one of the C
    //   library's built-in types, a text type, std::string, a standard
    //   container, an enumeration from C++11, a tuple, a complex number, an
    //   optional value from C++17, a type made from text, one made from wide
    //   text with wide text, one made from a number from C++11, or, by
    //   default, a type read and written with its stream operators. Each has
    //   the three members type_traits describes; the stream default's
    //   descriptor is named "TEXT" and has no flags, and, from C++11, no
    //   writer for a type without operator<< and no converter for one
    //   without operator>>, whose target() is then refused at compile time,
    //   as CLI11 refuses the type.
    template<typename Type,
             typename Enable>
    struct default_traits
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
        static void*               target(Type& _value) D_NOEXCEPT;
        static const void*         source(const Type& _value) D_NOEXCEPT;
    };

    // 3.4.2
    // The built-in types
    //   trait: the C library's own descriptor, d_arg_type_builtin's, for
    //   every type builtin_of names, the value's own address the target.
    //   `char*` has none: a parse's values are not the caller's to write.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<(builtin_of<Type>::value >= 0)>::type>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
        static void*               target(Type& _value) D_NOEXCEPT;
        static const void*         source(const Type& _value) D_NOEXCEPT;
    };

    template<>
    struct default_traits<char*>;

    // 3.4.3
    // The text types
    //   trait: cli::text binds as the d_pack_text it derives from, through
    //   the built-in TEXT descriptor, which writes its base; the span it
    //   holds after a parse is valid until the next parse, clear or release,
    //   as the descriptor's are.
    template<>
    struct default_traits<text>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
        static void*               target(text& _value) D_NOEXCEPT;
        static const void*         source(const text& _value) D_NOEXCEPT;
    };

    // 3.4.4
    // std::string
    //   trait: a copy of the first value; named "TEXT" and flagged
    //   D_ARG_TYPE_FLAG_STRING, as the C text types are.
    template<>
    struct default_traits<std::string>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
        static void*               target(std::string& _value) D_NOEXCEPT;
        static const void*         source(const std::string& _value)
                                       D_NOEXCEPT;
    };

    // 3.4.5
    // The standard containers
    //   trait: container_traits for every type is_container names.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<(is_container<Type>::value == 1)>::type>
        : public container_traits<Type>
    {};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.4.6
    // Enumerations
    //   trait: from C++11, an enumeration converts and writes as its
    //   underlying type does -- as a number when that type is a character
    //   type, as CLI11's integral_conversion reads one -- and is named
    //   "ENUM", flagged D_ARG_TYPE_FLAG_ENUM, and D_ARG_TYPE_FLAG_SIGNED
    //   when it holds negative values. It is not flagged an integer: CLI11
    //   does not count an enumeration's flag, as it counts an integer's.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<std::is_enum<Type>::value>::type>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
        static void*               target(Type& _value) D_NOEXCEPT;
        static const void*         source(const Type& _value) D_NOEXCEPT;
    };

    // enum_integer / enum_number
    //   traits: the type an enumeration's values convert through: its
    //   underlying type, but for a character type the integer of its size
    //   and sign, so that `enum : char` reads "65" as 65, not as '6'.
    template<typename Number>
    struct enum_integer
    {
        typedef Number type;
    };

    template<>
    struct enum_integer<char>
    {
        typedef std::conditional<std::is_signed<char>::value,
                                 signed char,
                                 unsigned char>::type type;
    };

    template<>
    struct enum_integer<wchar_t>
    {
        typedef std::conditional<std::is_signed<wchar_t>::value,
                                 std::make_signed<wchar_t>::type,
                                 std::make_unsigned<wchar_t>::type>::type type;
    };

    template<>
    struct enum_integer<char16_t>
    {
        typedef std::make_unsigned<char16_t>::type type;
    };

    template<>
    struct enum_integer<char32_t>
    {
        typedef std::make_unsigned<char32_t>::type type;
    };

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    template<>
    struct enum_integer<char8_t>
    {
        typedef unsigned char type;
    };
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename Type>
    struct enum_number
    {
        typedef typename enum_integer<
            typename std::underlying_type<Type>::type>::type type;
    };

    // enum_convert / enum_format
    //   functions: an enumeration converted into its number, then cast, the
    //   target written only on success; written as its number.
    template<typename Type>
    int  enum_convert(const ::d_arg_type*            _type,
                      const ::d_pack_text*           _values,
                      std::size_t                    _count,
                      void*                          _target,
                      const ::d_arg_convert_context* _context);
    template<typename Type>
    int  enum_format(const ::d_arg_type* _type,
                     const void*         _value,
                     ::d_pack_sink       _sink);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.4.7
    // value_address
    //   trait: the target and source most types have: the value's own
    //   address, from which the traits below derive.
    template<typename Type>
    struct value_address
    {
        static void*       target(Type& _value) D_NOEXCEPT;
        static const void* source(const Type& _value) D_NOEXCEPT;
    };

    // 3.4.8
    // Tuples
    //   trait: a tuple-like type -- std::pair, std::tuple, std::array --
    //   converts and writes part by part, named as CLI11 names it,
    //   "[INT,TEXT]", sized by the sums of its parts' sizes, expecting one
    //   group of values, without flags; made the first time it is asked
    //   for. One with a const part has no converter.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<
                   (is_kind<Type, type_kind::tuple>::value)>::type>
        : public value_address<Type>
    {
        static const ::d_arg_type* descriptor();
    };

    // 3.4.9
    // Complex numbers
    //   trait: a std::complex converts as complex_fill says and writes with
    //   operator<<, named "COMPLEX", taking one or two values, as CLI11's.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<
                   (is_kind<Type, type_kind::complex>::value)>::type>
        : public value_address<Type>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
    };

    // 3.4.10
    // Text-like types
    //   trait: a type made from a std::string -- CLI11's string_assignable
    //   and string_constructible -- converts as text_assign says and writes
    //   as the std::string it converts to, or not at all; named "TEXT" and
    //   flagged D_ARG_TYPE_FLAG_STRING, as std::string is.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<
                   (is_kind<Type, type_kind::text_like>::value)>::type>
        : public value_address<Type>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 3.4.11
    // Number-like types
    //   trait: from C++11, a type made from a number converts as number_fill
    //   says and writes with operator<<, or not at all; named "INT" when made
    //   from an `int` alone and "FLOAT" otherwise, as CLI11 names one, and
    //   without flags.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<
                   (is_kind<Type, type_kind::number_like>::value)>::type>
        : public value_address<Type>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
    };

    // 3.4.12
    // Durations
    //   trait: from C++11, a std::chrono duration converts and writes as
    //   duration_fill says, named as a number-like type is, by the numbers
    //   it is made from, and without flags.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<
                   (is_kind<Type, type_kind::duration>::value)>::type>
        : public value_address<Type>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
    };

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // 3.4.13
    // Optional values
    //   trait: from C++17, a std::optional converts and writes as
    //   optional_fill says, named, sized, expecting and flagged as the type
    //   it holds, which is its descriptor's `data`; made the first time it is
    //   asked for.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<
                   (is_kind<Type, type_kind::optional>::value)>::type>
        : public value_address<Type>
    {
        static const ::d_arg_type* descriptor();
    };

    // 3.4.14
    // std::string_view
    //   trait: from C++17, a view of the value, named "TEXT" and flagged
    //   D_ARG_TYPE_FLAG_STRING; see view_convert.
    template<>
    struct default_traits<std::string_view>
        : public value_address<std::string_view>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
    };

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

    // 3.4.15
    // Wide text-like types
    //   trait: with wide text, a type made from a std::wstring -- CLI11's
    //   wstring_assignable and wstring_constructible, std::wstring itself
    //   among them, and, where the system's own text is wide, a type made
    //   from both a std::string and a std::wstring, such as
    //   std::filesystem::path -- converts as wide_assign says and writes as
    //   the UTF-8 of the std::wstring it converts to, else as the std::string
    //   it converts to, or not at all; named "TEXT" and flagged
    //   D_ARG_TYPE_FLAG_STRING, as std::string is.
    template<typename Type>
    struct default_traits<
               Type,
               typename enable_if<
                   (is_kind<Type, type_kind::wide_text>::value)>::type>
        : public value_address<Type>
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
    };

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

NS_END  // internal


//==============================================================================
// 4.  DEFINITIONS
//==============================================================================


// 4.1    type_traits
//------------------------------------------------------------------------------
// type_traits
//   The default, unless the caller specializes it.
template<typename Type>
struct type_traits : public internal::default_traits<Type>
{};

// 4.2    container_traits
//------------------------------------------------------------------------------
// container_traits::descriptor
//   Made from the element's descriptor the first time it is asked for, and
// kept for the program's life, as every descriptor must be.
template<typename Container>
inline const ::d_arg_type*
container_traits<Container>::descriptor()
{
    static const ::d_arg_type DESCRIPTOR =
        internal::container_descriptor<Container>();

    return &DESCRIPTOR;
}

// container_traits::target / source
//   The container itself.
template<typename Container>
inline void*
container_traits<Container>::target(
    Container& _value
) D_NOEXCEPT
{
    return &_value;
}

template<typename Container>
inline const void*
container_traits<Container>::source(
    const Container& _value
) D_NOEXCEPT
{
    return &_value;
}

// 4.3    Use
//------------------------------------------------------------------------------
// type_name
//   The descriptor's name.
template<typename Type>
inline const char*
type_name()
{
    return type_traits<Type>::descriptor()->name;
}

// convert
//   d_arg_convert over the one value: with no context, the configured rule;
// with a rule, a context holding it and nothing else.
template<typename Type>
inline status
convert(
    const text& _value,
    Type&       _target
)
{
    return status(d_arg_convert(type_traits<Type>::descriptor(),
                                &_value,
                                1u,
                                type_traits<Type>::target(_target),
                                D_NULLPTR));
}

template<typename Type>
inline status
convert(
    const text&         _value,
    integer_rule::value _rule,
    Type&               _target
)
{
    const ::d_arg_convert_context context = { static_cast<int>(_rule),
                                              D_NULLPTR,
                                              D_NULLPTR };

    return status(d_arg_convert(type_traits<Type>::descriptor(),
                                &_value,
                                1u,
                                type_traits<Type>::target(_target),
                                &context));
}

// to_text
//   d_arg_format into a string sink.
template<typename Type>
inline status
to_text(
    const Type&  _value,
    std::string& _out
)
{
    return status(d_arg_format(type_traits<Type>::descriptor(),
                               type_traits<Type>::source(_value),
                               string_sink(_out)));
}

// 4.4    Failures
//------------------------------------------------------------------------------
// internal::conversion_failure
//   The context kept; an exception answered as the C library answers its
// own failures. The reason is appended only when there is a buffer for it,
// and an append that fails leaves the status as it is.
inline
internal::conversion_failure::conversion_failure(
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
    : m_context(_context)
{}

inline int
internal::conversion_failure::operator()(
    const std::exception* _exception,
    bool                  _out_of_memory
) const
{
    // a refused allocation is the C library's out-of-memory error
    if (_out_of_memory)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const char* const words = internal::exception_words(_exception);

    // the exception's own words, when it has any and a reason is asked for
    if ( (words != D_NULLPTR)               &&
         (m_context != D_NULLPTR)           &&
         (m_context->message != D_NULLPTR) )
    {
        (void)d_arg_buffer_append_cstr(m_context->message,
                                       words);
    }

    return D_ARG_ERROR_CONVERSION;
}

// 4.5    Conversions
//------------------------------------------------------------------------------
// internal::first_value / is_separator / quiet_context
//   The first value or nothing; empty or "%%"; the context's rule and
// option, without its buffer.
inline text
internal::first_value(
    const ::d_pack_text* _values,
    std::size_t          _count
) D_NOEXCEPT
{
    return ( (_count == 0u) ||
             (_values == D_NULLPTR) )
           ? text()
           : text(_values[0]);
}

inline bool
internal::is_separator(
    const text& _value
) D_NOEXCEPT
{
    return ( (_value.empty()) ||
             (_value == "%%") );
}

inline ::d_arg_convert_context
internal::quiet_context(
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
{
    const ::d_arg_convert_context quiet =
    {
        (_context != D_NULLPTR)
        ? _context->integer_rule
        : D_INTERNAL_ARG_PARSER_INTEGER_RULE,
        (_context != D_NULLPTR) ? _context->option : D_NULLPTR,
        D_NULLPTR
    };

    return quiet;
}

// internal::string_assign / string_convert / string_format
//   assign() replaces the string whole or, refused, leaves it as it was.
inline
internal::string_assign::string_assign(
    const ::d_pack_text* _values,
    std::size_t          _count,
    void*                _target
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<std::string*>(_target))
{}

inline int
internal::string_assign::operator()() const
{
    const text value = internal::first_value(m_values,
                                             m_count);

    // an empty value is the empty string, which needs no pointer
    if (value.empty())
    {
        m_target->clear();

        return D_ARG_OK;
    }

    m_target->assign(value.data(),
                     value.size());

    return D_ARG_OK;
}

inline int
internal::string_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::string_assign(_values,
                                                   _count,
                                                   _target),
                           internal::conversion_failure(_context));
}

inline int
internal::string_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    const std::string* const value = static_cast<const std::string*>(_value);

    (void)_type;

    return d_arg_sink_write(_sink,
                            value->data(),
                            value->size());
}

// internal::stream_read / stream_parse / stream_convert
//   CLI11's from_stream: read whole -- nothing failed and nothing left in
// the buffer -- into a Type() of its own, which replaces the target only
// then; an empty value is Type() itself, as lexical_assign makes it.
template<typename Type>
inline
internal::stream_read<Type>::stream_read(
    const ::d_pack_text* _values,
    std::size_t          _count,
    void*                _target
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Type*>(_target))
{}

template<typename Type>
inline int
internal::stream_read<Type>::operator()() const
{
    const text value = internal::first_value(m_values,
                                             m_count);

    // an empty value is the type's value-initialized object
    if (value.empty())
    {
        *m_target = Type();

        return D_ARG_OK;
    }

    return internal::stream_parse(value,
                                  *m_target);
}

template<typename Type>
inline int
internal::stream_parse(
    const text& _value,
    Type&       _target
)
{
    std::istringstream stream(_value.str());
    Type               read = Type();

    stream >> read;

    // a value read in part, or not at all, does not convert
    if ( (stream.fail()) ||
         (stream.rdbuf()->in_avail() != 0) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    _target = read;

    return D_ARG_OK;
}

template<typename Type>
inline int
internal::stream_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::stream_read<Type>(_values,
                                                       _count,
                                                       _target),
                           internal::conversion_failure(_context));
}

// internal::stream_write / stream_format
//   operator<< into a string, then the string to the sink.
template<typename Type>
inline
internal::stream_write<Type>::stream_write(
    const void*   _value,
    ::d_pack_sink _sink
) D_NOEXCEPT
    : m_value(static_cast<const Type*>(_value)),
      m_sink(_sink)
{}

template<typename Type>
inline int
internal::stream_write<Type>::operator()() const
{
    std::ostringstream stream;

    stream << *m_value;

    const std::string written = stream.str();

    return d_arg_sink_write(m_sink,
                            written.data(),
                            written.size());
}

template<typename Type>
inline int
internal::stream_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    (void)_type;

    return internal::guard(internal::stream_write<Type>(_value,
                                                        _sink),
                           internal::conversion_failure(D_NULLPTR));
}

// internal::container_fill / container_convert
//   CLI11's container conversion, into a container of its own, swapped into
// the target once every element converted: "{}" alone empties it; "{}"
// with a separator after it makes one element, of "{}", and stops; a
// container left empty by its values does not convert. group_size gives
// how many values the element at an index takes, 0 for a pair's value left
// without its partner; keep ends a conversion with what it filled.
template<typename Container>
inline
internal::container_fill<Container>::container_fill(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Container*>(_target)),
      m_context(_context)
{}

template<typename Container>
inline int
internal::container_fill<Container>::operator()() const
{
    typedef typename internal::mutable_type<
        typename Container::value_type>::type element_type;

    const ::d_arg_type* const element = type_traits<element_type>::descriptor();
    const std::size_t         step    =
        (element->type_size_max > 1)
        ? static_cast<std::size_t>(element->type_size_max)
        : 1u;

    return (step == 1u)
           ? single(element)
           : grouped(element,
                     step);
}

template<typename Container>
inline int
internal::container_fill<Container>::single(
    const ::d_arg_type* _element
) const
{
    typedef typename internal::mutable_type<
        typename Container::value_type>::type element_type;

    Container filled;

    // no values, or "{}" alone, is the empty container
    if ( (m_count == 0u) ||
         ( (m_count == 1u) &&
           (text(m_values[0]) == "{}") ) )
    {
        m_target->swap(filled);

        return D_ARG_OK;
    }

    const bool skip_remaining = ( (m_count == 2u)                  &&
                                  (text(m_values[0]) == "{}")      &&
                                  (internal::is_separator(m_values[1])) );

    // each value an element, converted as its own type converts
    for (std::size_t index = 0u;
         index < m_count;
         ++index)
    {
        element_type element   = element_type();
        const int    converted = d_arg_convert(
            _element,
            &m_values[index],
            1u,
            type_traits<element_type>::target(element),
            m_context);

        // an element that does not convert fails the container
        if (converted != D_ARG_OK)
        {
            return converted;
        }

        filled.insert(filled.end(),
                      element);

        // "{}" and a separator: one element, no more
        if (skip_remaining)
        {
            break;
        }
    }

    return keep(filled);
}

template<typename Container>
inline int
internal::container_fill<Container>::grouped(
    const ::d_arg_type* _element,
    std::size_t         _step
) const
{
    typedef typename internal::mutable_type<
        typename Container::value_type>::type element_type;

    Container   filled;
    std::size_t index = 0u;

    // each group of values an element
    while (index < m_count)
    {
        const std::size_t taken = group_size(index,
                                             _step);

        // a value left without its partner does not convert
        if (taken == 0u)
        {
            return D_ARG_ERROR_CONVERSION;
        }

        element_type element   = element_type();
        const int    converted = d_arg_convert(
            _element,
            &m_values[index],
            taken,
            type_traits<element_type>::target(element),
            m_context);

        // an element that does not convert fails the container
        if (converted != D_ARG_OK)
        {
            return converted;
        }

        filled.insert(filled.end(),
                      element);
        index += taken;
    }

    return keep(filled);
}

template<typename Container>
inline std::size_t
internal::container_fill<Container>::group_size(
    std::size_t _index,
    std::size_t _step
) const
{
    // a pair takes its two values, as CLI11 takes them, both or none
    if (_step == 2u)
    {
        return ((m_count - _index) < 2u) ? 0u
                                         : 2u;
    }

    std::size_t taken = 0u;

    // as many as the element takes, a separator ending them early
    while ( (taken < _step) &&
            ((_index + taken) < m_count) )
    {
        ++taken;

        // the group ends with a separator, which it keeps
        if (internal::is_separator(m_values[_index + taken - 1u]))
        {
            break;
        }
    }

    return taken;
}

template<typename Container>
inline int
internal::container_fill<Container>::keep(
    Container& _filled
) const
{
    // an empty result is no conversion, as CLI11 judges one
    if (_filled.empty())
    {
        return D_ARG_ERROR_CONVERSION;
    }

    m_target->swap(_filled);

    return D_ARG_OK;
}

template<typename Container>
inline int
internal::container_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::container_fill<Container>(_values,
                                                               _count,
                                                               _target,
                                                               _context),
                           internal::conversion_failure(_context));
}

// internal::container_format
//   "{}", or "[" and the elements, comma-separated, as their own type writes
// them, and "]"; the first failure stops it. Nothing here throws: the
// elements' writers guard their own work.
template<typename Container>
inline int
internal::container_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    typedef typename Container::value_type     element_type;
    typedef typename Container::const_iterator iterator;

    const Container* const container = static_cast<const Container*>(_value);

    (void)_type;

    // an empty container is written as CLI11 writes one
    if (container->empty())
    {
        return d_arg_sink_cstr(_sink,
                               "{}");
    }

    int written = d_arg_sink_cstr(_sink,
                                  "[");

    // each element, a comma before all but the first
    for (iterator element = container->begin();
         ( (written == D_ARG_OK) &&
           (element != container->end()) );
         ++element)
    {
        written = (element == container->begin())
                  ? D_ARG_OK
                  : d_arg_sink_cstr(_sink,
                                    ",");
        written = (written == D_ARG_OK)
                  ? d_arg_format(type_traits<element_type>::descriptor(),
                                 type_traits<element_type>::source(*element),
                                 _sink)
                  : written;
    }

    return (written == D_ARG_OK)
           ? d_arg_sink_cstr(_sink,
                             "]")
           : written;
}

// internal::container_descriptor
//   The element's descriptor, read for its name, sizes and flags: the one
// it converts as, a map's pair with a key that is not const.
template<typename Container>
inline ::d_arg_type
internal::container_descriptor() D_NOEXCEPT
{
    typedef typename internal::mutable_type<
        typename Container::value_type>::type element_type;

    const ::d_arg_type* const element =
        type_traits<element_type>::descriptor();
    const ::d_arg_type        descriptor =
    {
        element->name,
        &internal::container_convert<Container>,
        &internal::container_format<Container>,
        element,
        sizeof(Container),
        element->type_size_min,
        element->type_size_max,
        D_ARG_EXPECTED_UNLIMITED,
        (element->flags | D_ARG_TYPE_FLAG_CONTAINER)
    };

    return descriptor;
}

// internal::tuple_part
//   The pair's members; std::get for a tuple's and an array's parts.
template<typename First,
         typename Second>
inline First&
internal::tuple_part<std::pair<First, Second>, 0u>::get(
    std::pair<First, Second>& _value
) D_NOEXCEPT
{
    return _value.first;
}

template<typename First,
         typename Second>
inline const First&
internal::tuple_part<std::pair<First, Second>, 0u>::get(
    const std::pair<First, Second>& _value
) D_NOEXCEPT
{
    return _value.first;
}

template<typename First,
         typename Second>
inline Second&
internal::tuple_part<std::pair<First, Second>, 1u>::get(
    std::pair<First, Second>& _value
) D_NOEXCEPT
{
    return _value.second;
}

template<typename First,
         typename Second>
inline const Second&
internal::tuple_part<std::pair<First, Second>, 1u>::get(
    const std::pair<First, Second>& _value
) D_NOEXCEPT
{
    return _value.second;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<std::size_t Index,
         typename... Parts>
inline typename internal::tuple_part<std::tuple<Parts...>, Index>::type&
internal::tuple_part<std::tuple<Parts...>, Index>::get(
    std::tuple<Parts...>& _value
) D_NOEXCEPT
{
    return std::get<Index>(_value);
}

template<std::size_t Index,
         typename... Parts>
inline const typename internal::tuple_part<std::tuple<Parts...>, Index>::type&
internal::tuple_part<std::tuple<Parts...>, Index>::get(
    const std::tuple<Parts...>& _value
) D_NOEXCEPT
{
    return std::get<Index>(_value);
}

template<std::size_t Index,
         typename Element,
         std::size_t Size>
inline Element&
internal::tuple_part<std::array<Element, Size>, Index>::get(
    std::array<Element, Size>& _value
) D_NOEXCEPT
{
    return std::get<Index>(_value);
}

template<std::size_t Index,
         typename Element,
         std::size_t Size>
inline const Element&
internal::tuple_part<std::array<Element, Size>, Index>::get(
    const std::array<Element, Size>& _value
) D_NOEXCEPT
{
    return std::get<Index>(_value);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// internal::tuple_walk
//   One part at a time, then the rest: converted in place, from as many
// values as its type takes, or as remain; written after a comma but for the
// first; its name and sizes added. The end does nothing.
template<typename Tuple,
         std::size_t Index,
         std::size_t Size>
inline int
internal::tuple_walk<Tuple, Index, Size>::convert(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    std::size_t                    _at,
    Tuple&                         _target,
    const ::d_arg_convert_context* _context
)
{
    typedef typename internal::tuple_part<Tuple, Index>::type part_type;

    // the values ran out: this part and the ones after it stay as made
    if (_at >= _count)
    {
        return D_ARG_OK;
    }

    const ::d_arg_type* const descriptor = type_traits<part_type>::descriptor();
    const std::size_t         wanted     =
        (descriptor->type_size_max > 1)
        ? static_cast<std::size_t>(descriptor->type_size_max)
        : 1u;
    const std::size_t         taken      = ((_count - _at) < wanted)
                                           ? (_count - _at)
                                           : wanted;
    const int                 converted  = d_arg_convert(
        descriptor,
        &_values[_at],
        taken,
        type_traits<part_type>::target(
            internal::tuple_part<Tuple, Index>::get(_target)),
        _context);

    return (converted == D_ARG_OK)
           ? internal::tuple_walk<Tuple, Index + 1u, Size>::convert(
                 _values,
                 _count,
                 _at + taken,
                 _target,
                 _context)
           : converted;
}

template<typename Tuple,
         std::size_t Index,
         std::size_t Size>
inline int
internal::tuple_walk<Tuple, Index, Size>::format(
    const Tuple&  _value,
    ::d_pack_sink _sink
)
{
    typedef typename internal::remove_const<
        typename internal::tuple_part<Tuple, Index>::type>::type part_type;

    int written = (Index == 0u)
                  ? D_ARG_OK
                  : d_arg_sink_cstr(_sink,
                                    ",");

    written = (written == D_ARG_OK)
              ? d_arg_format(
                    type_traits<part_type>::descriptor(),
                    type_traits<part_type>::source(
                        internal::tuple_part<Tuple, Index>::get(_value)),
                    _sink)
              : written;

    return (written == D_ARG_OK)
           ? internal::tuple_walk<Tuple, Index + 1u, Size>::format(_value,
                                                                   _sink)
           : written;
}

template<typename Tuple,
         std::size_t Index,
         std::size_t Size>
inline void
internal::tuple_walk<Tuple, Index, Size>::shape(
    std::string& _name,
    int&         _minimum,
    int&         _maximum
)
{
    typedef typename internal::remove_const<
        typename internal::tuple_part<Tuple, Index>::type>::type part_type;

    const ::d_arg_type* const descriptor = type_traits<part_type>::descriptor();

    // a comma between the parts' names
    if (Index != 0u)
    {
        _name += ',';
    }

    _name    += (descriptor->name != D_NULLPTR) ? descriptor->name : "";
    _minimum += descriptor->type_size_min;
    _maximum += descriptor->type_size_max;
    internal::tuple_walk<Tuple, Index + 1u, Size>::shape(_name,
                                                         _minimum,
                                                         _maximum);

    return;
}

template<typename Tuple,
         std::size_t Size>
inline int
internal::tuple_walk<Tuple, Size, Size>::convert(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    std::size_t                    _at,
    Tuple&                         _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
{
    (void)_values;
    (void)_count;
    (void)_at;
    (void)_target;
    (void)_context;

    return D_ARG_OK;
}

template<typename Tuple,
         std::size_t Size>
inline int
internal::tuple_walk<Tuple, Size, Size>::format(
    const Tuple&  _value,
    ::d_pack_sink _sink
) D_NOEXCEPT
{
    (void)_value;
    (void)_sink;

    return D_ARG_OK;
}

template<typename Tuple,
         std::size_t Size>
inline void
internal::tuple_walk<Tuple, Size, Size>::shape(
    std::string& _name,
    int&         _minimum,
    int&         _maximum
) D_NOEXCEPT
{
    (void)_name;
    (void)_minimum;
    (void)_maximum;

    return;
}

// internal::tuple_fill / tuple_convert / tuple_format / tuple_shape /
// tuple_description
//   The parts converted into a Tuple() of the converter's own, which
// replaces the target only when every part converted; written between
// brackets; the shape between brackets, and the descriptor made from it,
// its name the kept string's.
template<typename Tuple>
inline
internal::tuple_fill<Tuple>::tuple_fill(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Tuple*>(_target)),
      m_context(_context)
{}

template<typename Tuple>
inline int
internal::tuple_fill<Tuple>::operator()() const
{
    Tuple     filled    = Tuple();
    const int converted = internal::tuple_walk<
        Tuple,
        0u,
        internal::tuple_access<Tuple>::size>::convert(m_values,
                                                      m_count,
                                                      0u,
                                                      filled,
                                                      m_context);

    // the target is written only once every part converted
    if (converted == D_ARG_OK)
    {
        *m_target = filled;
    }

    return converted;
}

template<typename Tuple>
inline int
internal::tuple_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::tuple_fill<Tuple>(_values,
                                                       _count,
                                                       _target,
                                                       _context),
                           internal::conversion_failure(_context));
}

template<typename Tuple>
inline int
internal::tuple_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    (void)_type;

    int written = d_arg_sink_cstr(_sink,
                                  "[");

    written = (written == D_ARG_OK)
              ? internal::tuple_walk<
                    Tuple,
                    0u,
                    internal::tuple_access<Tuple>::size>::format(
                        *static_cast<const Tuple*>(_value),
                        _sink)
              : written;

    return (written == D_ARG_OK)
           ? d_arg_sink_cstr(_sink,
                             "]")
           : written;
}

template<typename Tuple>
inline void
internal::tuple_shape(
    std::string& _name,
    int&         _minimum,
    int&         _maximum
)
{
    _name    = "[";
    _minimum = 0;
    _maximum = 0;
    internal::tuple_walk<Tuple,
                         0u,
                         internal::tuple_access<Tuple>::size>::shape(_name,
                                                                     _minimum,
                                                                     _maximum);
    _name += ']';

    return;
}

template<typename Tuple>
inline
internal::tuple_description<Tuple>::tuple_description()
    : name()
{
    int minimum = 0;
    int maximum = 0;

    internal::tuple_shape<Tuple>(name,
                                 minimum,
                                 maximum);

    const ::d_arg_type made =
    {
        name.c_str(),
        internal::tuple_converter<
            Tuple,
            (internal::tuple_access<Tuple>::writable != 0)>::get(),
        &internal::tuple_format<Tuple>,
        D_NULLPTR,
        sizeof(Tuple),
        minimum,
        maximum,
        1,
        0u
    };

    descriptor = made;
}

// internal::complex_fill / complex_convert
//   CLI11's rules: two values; else one split at its last sign, when that
// is not its first character; else one part alone, imaginary by its `i` or
// `j` -- one's rules, for a value that is not empty; and what none of them
// reads, operator>>. Each part converts through its type's descriptor,
// without a reason, as CLI11's lexical_cast tries it.
template<typename Complex>
inline
internal::complex_fill<Complex>::complex_fill(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Complex*>(_target)),
      m_context(_context)
{}

template<typename Complex>
inline bool
internal::complex_fill<Complex>::part(
    const text&                   _value,
    typename Complex::value_type& _out
) const
{
    typedef typename Complex::value_type part_type;

    const ::d_arg_convert_context quiet = internal::quiet_context(m_context);

    return ( (!_value.empty()) &&
             (d_arg_convert(type_traits<part_type>::descriptor(),
                            &_value,
                            1u,
                            type_traits<part_type>::target(_out),
                            &quiet) == D_ARG_OK) );
}

template<typename Complex>
inline bool
internal::complex_fill<Complex>::imaginary(
    text                          _value,
    typename Complex::value_type& _out
) const
{
    // an `i` or a `j` marks the imaginary part, and is not part of it
    if ( (!_value.empty()) &&
         ( (_value[_value.size() - 1u] == 'i') ||
           (_value[_value.size() - 1u] == 'j') ) )
    {
        _value = text(_value.data(),
                      _value.size() - 1u);
    }

    return part(_value,
                _out);
}

template<typename Complex>
inline int
internal::complex_fill<Complex>::operator()() const
{
    typedef typename Complex::value_type part_type;

    part_type real = part_type();
    part_type imag = part_type();

    // two values: the real part and the imaginary
    if ( (m_count >= 2u) &&
         (!text(m_values[1]).empty()) )
    {
        // either part not a number is no conversion
        if ( (!part(text(m_values[0]),
                    real)) ||
             (!imaginary(text(m_values[1]),
                         imag)) )
        {
            return D_ARG_ERROR_CONVERSION;
        }

        *m_target = Complex(real,
                            imag);

        return D_ARG_OK;
    }

    const text value = internal::first_value(m_values,
                                             m_count);

    // an empty value is zero, as CLI11's lexical_assign makes it
    if (value.empty())
    {
        *m_target = Complex();

        return D_ARG_OK;
    }

    // what the rules read
    if (one(value,
            real,
            imag))
    {
        *m_target = Complex(real,
                            imag);

        return D_ARG_OK;
    }

    return internal::stream_parse(value,
                                  *m_target);
}

template<typename Complex>
inline bool
internal::complex_fill<Complex>::one(
    const text&                   _value,
    typename Complex::value_type& _real,
    typename Complex::value_type& _imag
) const
{
    std::size_t sign = _value.size();

    // the last sign that is not the first character
    for (std::size_t index = _value.size() - 1u;
         index > 0u;
         --index)
    {
        // a sign splits the parts
        if ( (_value[index] == '+') ||
             (_value[index] == '-') )
        {
            sign = index;

            break;
        }
    }

    const char last = _value[_value.size() - 1u];

    // a real part and an imaginary one
    if (sign < _value.size())
    {
        return ( (part(text(_value.data(),
                            sign),
                       _real)) &&
                 (imaginary(text(_value.data() + sign,
                                 _value.size() - sign),
                            _imag)) );
    }

    // an imaginary part alone, or a real part alone
    return ( (last == 'i') ||
             (last == 'j') )
           ? imaginary(_value,
                       _imag)
           : part(_value,
                  _real);
}

template<typename Complex>
inline int
internal::complex_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::complex_fill<Complex>(_values,
                                                           _count,
                                                           _target,
                                                           _context),
                           internal::conversion_failure(_context));
}

// internal::text_assign / text_like_convert
//   The value as a std::string, assigned where the type is assigned one,
// else made into the type and assigned, as CLI11's lexical_cast does.
template<typename Type>
inline
internal::text_assign<Type>::text_assign(
    const ::d_pack_text* _values,
    std::size_t          _count,
    void*                _target
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Type*>(_target))
{}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename Type,
         typename String,
         bool Assigns>
inline void
internal::text_store<Type, String, Assigns>::store(
    Type&         _target,
    const String& _value
)
{
    _target = _value;

    return;
}

template<typename Type,
         typename String>
inline void
internal::text_store<Type, String, false>::store(
    Type&         _target,
    const String& _value
)
{
    _target = Type(_value);

    return;
}

template<typename Type>
inline int
internal::text_assign<Type>::operator()() const
{
    const std::string value = internal::first_value(m_values,
                                                    m_count).str();

    internal::text_store<
        Type,
        std::string,
        (internal::text_abilities<Type>::assigns != 0)>::store(*m_target,
                                                               value);

    return D_ARG_OK;
}

#else

template<typename Type>
inline int
internal::text_assign<Type>::operator()() const
{
    const std::string value = internal::first_value(m_values,
                                                    m_count).str();

    *m_target = Type(value);

    return D_ARG_OK;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename Type>
inline int
internal::text_like_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::text_assign<Type>(_values,
                                                       _count,
                                                       _target),
                           internal::conversion_failure(_context));
}

// internal::text_write / text_like_format
//   The value made a std::string, then written.
template<typename Type>
inline
internal::text_write<Type>::text_write(
    const void*   _value,
    ::d_pack_sink _sink
) D_NOEXCEPT
    : m_value(static_cast<const Type*>(_value)),
      m_sink(_sink)
{}

template<typename Type>
inline int
internal::text_write<Type>::operator()() const
{
    const std::string written(*m_value);

    return d_arg_sink_write(m_sink,
                            written.data(),
                            written.size());
}

template<typename Type>
inline int
internal::text_like_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    (void)_type;

    return internal::guard(internal::text_write<Type>(_value,
                                                      _sink),
                           internal::conversion_failure(D_NULLPTR));
}

// internal::tuple_converter / stream_converter / stream_formatter /
// text_formatter
//   The function, or NULL.
template<typename Tuple,
         bool Writable>
inline ::d_arg_convert_fn
internal::tuple_converter<Tuple, Writable>::get() D_NOEXCEPT
{
    return &internal::tuple_convert<Tuple>;
}

template<typename Tuple>
inline ::d_arg_convert_fn
internal::tuple_converter<Tuple, false>::get() D_NOEXCEPT
{
    return D_NULLPTR;
}

template<typename Type,
         bool Readable>
inline ::d_arg_convert_fn
internal::stream_converter<Type, Readable>::get() D_NOEXCEPT
{
    return &internal::stream_convert<Type>;
}

template<typename Type>
inline ::d_arg_convert_fn
internal::stream_converter<Type, false>::get() D_NOEXCEPT
{
    return D_NULLPTR;
}

template<typename Type,
         bool Writable>
inline ::d_arg_format_fn
internal::stream_formatter<Type, Writable>::get() D_NOEXCEPT
{
    return &internal::stream_format<Type>;
}

template<typename Type>
inline ::d_arg_format_fn
internal::stream_formatter<Type, false>::get() D_NOEXCEPT
{
    return D_NULLPTR;
}

template<typename Type,
         bool Converts>
inline ::d_arg_format_fn
internal::text_formatter<Type, Converts>::get() D_NOEXCEPT
{
    return &internal::text_like_format<Type>;
}

template<typename Type>
inline ::d_arg_format_fn
internal::text_formatter<Type, false>::get() D_NOEXCEPT
{
    return D_NULLPTR;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// internal::number_fill / number_convert
//   An `int`, then a `double`, through the C library's descriptors, as the
// category allows; then operator>>, where the type has it; each made into
// the type, which replaces the target.
template<typename Type>
inline
internal::number_fill<Type>::number_fill(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Type*>(_target)),
      m_context(_context)
{}

template<typename Type>
inline int
internal::number_fill<Type>::operator()() const
{
    const text value = internal::first_value(m_values,
                                             m_count);

    // an empty value is the type's value-initialized object
    if (value.empty())
    {
        *m_target = Type();

        return D_ARG_OK;
    }

    const ::d_arg_convert_context quiet = internal::quiet_context(m_context);

    // an `int`, then a `double`, as the type is made from them
    if ( (internal::from_int<Type,
                             (internal::number_category<Type>::from_int != 0)>::
              store(value,
                    *m_target,
                    quiet)) ||
         (internal::from_double<Type,
                                (internal::number_category<Type>::from_double
                                 != 0)>::
              store(value,
                    *m_target,
                    quiet)) )
    {
        return D_ARG_OK;
    }

    return internal::from_stream<Type,
                                 (internal::stream_abilities<Type>::readable
                                  != 0)>::store(value,
                                                *m_target);
}

// internal::from_int / from_double / from_stream
//   The C library's `int` and `double`, without a reason, each made into the
// type on success; operator>> as CLI11's from_stream reads; or nothing.
template<typename Type,
         bool Able>
inline bool
internal::from_int<Type, Able>::store(
    const text&                    _value,
    Type&                          _target,
    const ::d_arg_convert_context& _context
)
{
    int number = 0;

    // the value as an `int`, made into the type
    if (d_arg_convert(d_arg_type_builtin(D_ARG_BUILTIN_INT),
                      &_value,
                      1u,
                      &number,
                      &_context) == D_ARG_OK)
    {
        _target = Type(number);

        return true;
    }

    return false;
}

template<typename Type>
inline bool
internal::from_int<Type, false>::store(
    const text&                    _value,
    Type&                          _target,
    const ::d_arg_convert_context& _context
) D_NOEXCEPT
{
    (void)_value;
    (void)_target;
    (void)_context;

    return false;
}

template<typename Type,
         bool Able>
inline bool
internal::from_double<Type, Able>::store(
    const text&                    _value,
    Type&                          _target,
    const ::d_arg_convert_context& _context
)
{
    double number = 0.0;

    // the value as a `double`, made into the type
    if (d_arg_convert(d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE),
                      &_value,
                      1u,
                      &number,
                      &_context) == D_ARG_OK)
    {
        _target = Type(number);

        return true;
    }

    return false;
}

template<typename Type>
inline bool
internal::from_double<Type, false>::store(
    const text&                    _value,
    Type&                          _target,
    const ::d_arg_convert_context& _context
) D_NOEXCEPT
{
    (void)_value;
    (void)_target;
    (void)_context;

    return false;
}

template<typename Type,
         bool Able>
inline int
internal::from_stream<Type, Able>::store(
    const text& _value,
    Type&       _target
)
{
    return internal::stream_parse(_value,
                                  _target);
}

template<typename Type>
inline int
internal::from_stream<Type, false>::store(
    const text& _value,
    Type&       _target
) D_NOEXCEPT
{
    (void)_value;
    (void)_target;

    return D_ARG_ERROR_CONVERSION;
}

template<typename Type>
inline int
internal::number_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::number_fill<Type>(_values,
                                                       _count,
                                                       _target,
                                                       _context),
                           internal::conversion_failure(_context));
}

// internal::duration_fill / duration_convert
//   The count, read through its own descriptor -- under the context's rule,
// without its buffer, as a number-like type's numbers are -- and made into
// the duration, which replaces the target only then.
template<typename Duration>
inline
internal::duration_fill<Duration>::duration_fill(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Duration*>(_target)),
      m_context(_context)
{}

template<typename Duration>
inline int
internal::duration_fill<Duration>::operator()() const
{
    typedef typename internal::duration_of<Duration>::count_type count_type;

    const text value = internal::first_value(m_values,
                                             m_count);

    // an empty value is the type's value-initialized object
    if (value.empty())
    {
        *m_target = Duration();

        return D_ARG_OK;
    }

    const ::d_arg_convert_context quiet = internal::quiet_context(m_context);
    count_type                    count = count_type();
    const int                     read  = d_arg_convert(
        type_traits<count_type>::descriptor(),
        &value,
        1u,
        type_traits<count_type>::target(count),
        &quiet);

    // the count made into the duration
    if (read == D_ARG_OK)
    {
        *m_target = Duration(count);
    }

    return read;
}

template<typename Duration>
inline int
internal::duration_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::duration_fill<Duration>(_values,
                                                             _count,
                                                             _target,
                                                             _context),
                           internal::conversion_failure(_context));
}

// internal::duration_write / duration_format
//   The count, written by its own descriptor.
template<typename Duration>
inline
internal::duration_write<Duration>::duration_write(
    const void*   _value,
    ::d_pack_sink _sink
) D_NOEXCEPT
    : m_value(static_cast<const Duration*>(_value)),
      m_sink(_sink)
{}

template<typename Duration>
inline int
internal::duration_write<Duration>::operator()() const
{
    typedef typename internal::duration_of<Duration>::count_type count_type;

    const count_type count = m_value->count();

    return d_arg_format(type_traits<count_type>::descriptor(),
                        type_traits<count_type>::source(count),
                        m_sink);
}

template<typename Duration>
inline int
internal::duration_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    (void)_type;

    return internal::guard(internal::duration_write<Duration>(_value,
                                                              _sink),
                           internal::conversion_failure(D_NULLPTR));
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// internal::optional_fill / optional_convert / optional_format
//   Emptied, or the held type's conversion into a value of its own, moved
// in on success; the held value written through its type, or nothing.
template<typename Optional>
inline
internal::optional_fill<Optional>::optional_fill(
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Optional*>(_target)),
      m_context(_context)
{}

template<typename Optional>
inline int
internal::optional_fill<Optional>::operator()() const
{
    typedef typename Optional::value_type held_type;

    // no values, or an empty first one, is no value
    if (internal::first_value(m_values,
                              m_count).empty())
    {
        m_target->reset();

        return D_ARG_OK;
    }

    held_type held      = held_type();
    const int converted = d_arg_convert(type_traits<held_type>::descriptor(),
                                        m_values,
                                        m_count,
                                        type_traits<held_type>::target(held),
                                        m_context);

    // the target takes the value only once it converted
    if (converted == D_ARG_OK)
    {
        *m_target = std::move(held);
    }

    return converted;
}

template<typename Optional>
inline int
internal::optional_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::optional_fill<Optional>(_values,
                                                             _count,
                                                             _target,
                                                             _context),
                           internal::conversion_failure(_context));
}

template<typename Optional>
inline int
internal::optional_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    typedef typename Optional::value_type held_type;

    const Optional* const value = static_cast<const Optional*>(_value);

    (void)_type;

    return value->has_value()
           ? d_arg_format(type_traits<held_type>::descriptor(),
                          type_traits<held_type>::source(**value),
                          _sink)
           : D_ARG_OK;
}

// internal::view_convert / view_format
//   The first value's span; the view's bytes.
inline int
internal::view_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
) D_NOEXCEPT
{
    const text value = internal::first_value(_values,
                                             _count);

    (void)_type;
    (void)_context;

    *static_cast<std::string_view*>(_target) =
        std::string_view(value.data(),
                         value.size());

    return D_ARG_OK;
}

inline int
internal::view_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
) D_NOEXCEPT
{
    const std::string_view* const value =
        static_cast<const std::string_view*>(_value);

    (void)_type;

    return d_arg_sink_write(_sink,
                            value->data(),
                            value->size());
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// internal::widen_text / narrow_text
//   Room for a unit per byte, which always suffices, the units written into
// it, then the string made of them; the units' UTF-8 into a scratch buffer,
// then written.
inline int
internal::widen_text(
    const text&   _text,
    std::wstring& _out
)
{
    std::vector<wchar_t> units(_text.size());
    std::size_t          needed = 0u;
    const int            result = d_arg_wide_from_utf8(
                                      _text.data(),
                                      _text.size(),
                                      units.empty() ? D_NULLPTR : &units[0],
                                      units.size(),
                                      &needed);

    // the units, when the text converted
    if (result == D_ARG_OK)
    {
        _out.assign(units.begin(),
                    units.begin() + static_cast<std::ptrdiff_t>(needed));
    }

    return result;
}

inline int
internal::narrow_text(
    const std::wstring& _text,
    ::d_pack_sink       _sink
)
{
    internal::scratch written;
    const int         result = d_arg_utf8_from_wide(_text.data(),
                                                    _text.size(),
                                                    written.get());

    return (result == D_ARG_OK)
           ? d_arg_sink_write(_sink,
                              written.get()->data,
                              written.get()->length)
           : result;
}

// internal::wide_assign / wide_like_convert
//   The value widened, then assigned where the type is assigned wide text,
// else made into the type and assigned, as CLI11's lexical_cast does.
template<typename Type>
inline
internal::wide_assign<Type>::wide_assign(
    const ::d_pack_text* _values,
    std::size_t          _count,
    void*                _target
) D_NOEXCEPT
    : m_values(_values),
      m_count(_count),
      m_target(static_cast<Type*>(_target))
{}

template<typename Type>
inline int
internal::wide_assign<Type>::operator()() const
{
    std::wstring value;
    const int    result = internal::widen_text(
                              internal::first_value(m_values,
                                                    m_count),
                              value);

    // a value that is not UTF-8 converts to nothing
    if (result == D_ARG_OK)
    {
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        internal::text_store<
            Type,
            std::wstring,
            (internal::wide_abilities<Type>::assigns != 0)>::store(*m_target,
                                                                   value);
#else
        *m_target = Type(value);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
    }

    return result;
}

template<typename Type>
inline int
internal::wide_like_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    (void)_type;

    return internal::guard(internal::wide_assign<Type>(_values,
                                                       _count,
                                                       _target),
                           internal::conversion_failure(_context));
}

// internal::wide_write / wide_like_format / wide_formatter
//   The value made a std::wstring, then written as UTF-8; the function, or
// the text formatter's.
template<typename Type>
inline
internal::wide_write<Type>::wide_write(
    const void*   _value,
    ::d_pack_sink _sink
) D_NOEXCEPT
    : m_value(static_cast<const Type*>(_value)),
      m_sink(_sink)
{}

template<typename Type>
inline int
internal::wide_write<Type>::operator()() const
{
    const std::wstring written(*m_value);

    return internal::narrow_text(written,
                                 m_sink);
}

template<typename Type>
inline int
internal::wide_like_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    (void)_type;

    return internal::guard(internal::wide_write<Type>(_value,
                                                      _sink),
                           internal::conversion_failure(D_NULLPTR));
}

template<typename Type,
         bool Converts>
inline ::d_arg_format_fn
internal::wide_formatter<Type, Converts>::get() D_NOEXCEPT
{
    return &internal::wide_like_format<Type>;
}

template<typename Type>
inline ::d_arg_format_fn
internal::wide_formatter<Type, false>::get() D_NOEXCEPT
{
    return internal::text_formatter<
               Type,
               (internal::text_abilities<Type>::to != 0)>::get();
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// 4.6    The default traits
//------------------------------------------------------------------------------
// internal::default_traits (streams)
//   A descriptor made once, its functions those the type's operators allow;
// the value's own address, asked only to convert into it, which needs
// operator>>.
template<typename Type,
         typename Enable>
inline const ::d_arg_type*
internal::default_traits<Type, Enable>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        "TEXT",
        internal::stream_converter<
            Type,
            (internal::stream_abilities<Type>::readable != 0)>::get(),
        internal::stream_formatter<
            Type,
            (internal::stream_abilities<Type>::writable != 0)>::get(),
        D_NULLPTR,
        sizeof(Type),
        1,
        1,
        1,
        0u
    };

    return &DESCRIPTOR;
}

template<typename Type,
         typename Enable>
inline void*
internal::default_traits<Type, Enable>::target(
    Type& _value
) D_NOEXCEPT
{
    D_INTERNAL_ARG_ASSERT_DEPENDENT(
        (internal::stream_abilities<Type>::readable != 0),
        "a type an option converts into needs operator>>, or "
        "type_traits of its own" );

    return &_value;
}

template<typename Type,
         typename Enable>
inline const void*
internal::default_traits<Type, Enable>::source(
    const Type& _value
) D_NOEXCEPT
{
    return &_value;
}

// internal::default_traits (built-in types)
//   d_arg_type_builtin; the value's own address.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<(internal::builtin_of<Type>::value >= 0)>::type
>::descriptor() D_NOEXCEPT
{
    return d_arg_type_builtin(internal::builtin_of<Type>::value);
}

template<typename Type>
inline void*
internal::default_traits<
    Type,
    typename internal::enable_if<(internal::builtin_of<Type>::value >= 0)>::type
>::target(
    Type& _value
) D_NOEXCEPT
{
    return &_value;
}

template<typename Type>
inline const void*
internal::default_traits<
    Type,
    typename internal::enable_if<(internal::builtin_of<Type>::value >= 0)>::type
>::source(
    const Type& _value
) D_NOEXCEPT
{
    return &_value;
}

// internal::default_traits<text>
//   The TEXT descriptor; the d_pack_text the text derives from.
inline const ::d_arg_type*
internal::default_traits<text>::descriptor() D_NOEXCEPT
{
    return d_arg_type_builtin(D_ARG_BUILTIN_TEXT);
}

inline void*
internal::default_traits<text>::target(
    text& _value
) D_NOEXCEPT
{
    return static_cast< ::d_pack_text*>(&_value);
}

inline const void*
internal::default_traits<text>::source(
    const text& _value
) D_NOEXCEPT
{
    return static_cast<const ::d_pack_text*>(&_value);
}

// internal::default_traits<std::string>
//   A descriptor of constants; the string itself.
inline const ::d_arg_type*
internal::default_traits<std::string>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        "TEXT",
        &internal::string_convert,
        &internal::string_format,
        D_NULLPTR,
        sizeof(std::string),
        1,
        1,
        1,
        D_ARG_TYPE_FLAG_STRING
    };

    return &DESCRIPTOR;
}

inline void*
internal::default_traits<std::string>::target(
    std::string& _value
) D_NOEXCEPT
{
    return &_value;
}

inline const void*
internal::default_traits<std::string>::source(
    const std::string& _value
) D_NOEXCEPT
{
    return &_value;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// internal::default_traits (enumerations)
//   A descriptor of constants, flagged an enumeration, and signed as the
// number it converts through is; the value's own address.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<std::is_enum<Type>::value>::type
>::descriptor() D_NOEXCEPT
{
    typedef typename internal::enum_number<Type>::type number_type;

    static const ::d_arg_type DESCRIPTOR =
    {
        "ENUM",
        &internal::enum_convert<Type>,
        &internal::enum_format<Type>,
        D_NULLPTR,
        sizeof(Type),
        1,
        1,
        1,
        (std::is_signed<number_type>::value
         ? (D_ARG_TYPE_FLAG_ENUM | D_ARG_TYPE_FLAG_SIGNED)
         : D_ARG_TYPE_FLAG_ENUM)
    };

    return &DESCRIPTOR;
}

template<typename Type>
inline void*
internal::default_traits<
    Type,
    typename internal::enable_if<std::is_enum<Type>::value>::type
>::target(
    Type& _value
) D_NOEXCEPT
{
    return &_value;
}

template<typename Type>
inline const void*
internal::default_traits<
    Type,
    typename internal::enable_if<std::is_enum<Type>::value>::type
>::source(
    const Type& _value
) D_NOEXCEPT
{
    return &_value;
}

// internal::enum_convert / enum_format
//   Through the number: converted into one of its own, which is cast into
// the target only on success; written from a number cast from the value.
template<typename Type>
inline int
internal::enum_convert(
    const ::d_arg_type*            _type,
    const ::d_pack_text*           _values,
    std::size_t                    _count,
    void*                          _target,
    const ::d_arg_convert_context* _context
)
{
    typedef typename internal::enum_number<Type>::type number_type;

    number_type number    = number_type();
    const int   converted = d_arg_convert(
        type_traits<number_type>::descriptor(),
        _values,
        _count,
        &number,
        _context);

    (void)_type;

    // the enumeration is written only when its number converted
    if (converted == D_ARG_OK)
    {
        *static_cast<Type*>(_target) = static_cast<Type>(number);
    }

    return converted;
}

template<typename Type>
inline int
internal::enum_format(
    const ::d_arg_type* _type,
    const void*         _value,
    ::d_pack_sink       _sink
)
{
    typedef typename internal::enum_number<Type>::type number_type;

    const number_type number =
        static_cast<number_type>(*static_cast<const Type*>(_value));

    (void)_type;

    return d_arg_format(type_traits<number_type>::descriptor(),
                        &number,
                        _sink);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// internal::value_address
//   The value itself.
template<typename Type>
inline void*
internal::value_address<Type>::target(
    Type& _value
) D_NOEXCEPT
{
    return &_value;
}

template<typename Type>
inline const void*
internal::value_address<Type>::source(
    const Type& _value
) D_NOEXCEPT
{
    return &_value;
}

// internal::default_traits (tuples)
//   The description, names and descriptor, made the first time it is asked
// for and kept for the program's life.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<
        (internal::is_kind<Type,
                           internal::type_kind::tuple>::value)>::type
>::descriptor()
{
    static const internal::tuple_description<Type> DESCRIPTION;

    return &DESCRIPTION.descriptor;
}

// internal::default_traits (complex numbers)
//   A descriptor of constants: one or two values, written as streamed.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<
        (internal::is_kind<Type,
                           internal::type_kind::complex>::value)>::type
>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        "COMPLEX",
        &internal::complex_convert<Type>,
        &internal::stream_format<Type>,
        D_NULLPTR,
        sizeof(Type),
        1,
        2,
        1,
        0u
    };

    return &DESCRIPTOR;
}

// internal::default_traits (text-like types)
//   Converted as text, written as the std::string it becomes, or not.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<
        (internal::is_kind<Type,
                           internal::type_kind::text_like>::value)>::type
>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        "TEXT",
        &internal::text_like_convert<Type>,
        internal::text_formatter<
            Type,
            (internal::text_abilities<Type>::to != 0)>::get(),
        D_NULLPTR,
        sizeof(Type),
        1,
        1,
        1,
        D_ARG_TYPE_FLAG_STRING
    };

    return &DESCRIPTOR;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// internal::default_traits (number-like types)
//   Converted through the numbers it is made from, written as streamed, or
// not; named by them.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<
        (internal::is_kind<Type,
                           internal::type_kind::number_like>::value)>::type
>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        (static_cast<int>(internal::number_category<Type>::value) ==
         static_cast<int>(internal::number_kind::integer))
        ? "INT"
        : "FLOAT",
        &internal::number_convert<Type>,
        internal::stream_formatter<
            Type,
            (internal::stream_abilities<Type>::writable != 0)>::get(),
        D_NULLPTR,
        sizeof(Type),
        1,
        1,
        1,
        0u
    };

    return &DESCRIPTOR;
}

// internal::default_traits (durations)
//   Converted and written as their counts; named as a number-like type is,
// by the numbers it is made from.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<
        (internal::is_kind<Type,
                           internal::type_kind::duration>::value)>::type
>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        (static_cast<int>(internal::number_category<Type>::value) ==
         static_cast<int>(internal::number_kind::integer))
        ? "INT"
        : "FLOAT",
        &internal::duration_convert<Type>,
        &internal::duration_format<Type>,
        D_NULLPTR,
        sizeof(Type),
        1,
        1,
        1,
        0u
    };

    return &DESCRIPTOR;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// internal::default_traits (optional values)
//   The held type's descriptor, read for its name, sizes, count and flags,
// and kept as `data`, made the first time it is asked for.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<
        (internal::is_kind<Type,
                           internal::type_kind::optional>::value)>::type
>::descriptor()
{
    typedef typename Type::value_type held_type;

    static const ::d_arg_type* const HELD       =
        type_traits<held_type>::descriptor();
    static const ::d_arg_type        DESCRIPTOR =
    {
        HELD->name,
        &internal::optional_convert<Type>,
        &internal::optional_format<Type>,
        HELD,
        sizeof(Type),
        HELD->type_size_min,
        HELD->type_size_max,
        HELD->expected,
        HELD->flags
    };

    return &DESCRIPTOR;
}

// internal::default_traits<std::string_view>
//   A descriptor of constants.
inline const ::d_arg_type*
internal::default_traits<std::string_view>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        "TEXT",
        &internal::view_convert,
        &internal::view_format,
        D_NULLPTR,
        sizeof(std::string_view),
        1,
        1,
        1,
        D_ARG_TYPE_FLAG_STRING
    };

    return &DESCRIPTOR;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// internal::default_traits (wide text-like types)
//   Converted as wide text, written as the UTF-8 of the std::wstring it
// becomes, else as the std::string it becomes, or not.
template<typename Type>
inline const ::d_arg_type*
internal::default_traits<
    Type,
    typename internal::enable_if<
        (internal::is_kind<Type,
                           internal::type_kind::wide_text>::value)>::type
>::descriptor() D_NOEXCEPT
{
    static const ::d_arg_type DESCRIPTOR =
    {
        "TEXT",
        &internal::wide_like_convert<Type>,
        internal::wide_formatter<
            Type,
            (internal::wide_abilities<Type>::to != 0)>::get(),
        D_NULLPTR,
        sizeof(Type),
        1,
        1,
        1,
        D_ARG_TYPE_FLAG_STRING
    };

    return &DESCRIPTOR;
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TYPE_HPP
