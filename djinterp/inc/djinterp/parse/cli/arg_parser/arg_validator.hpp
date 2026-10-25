/*******************************************************************************
* djinterp [parse]                                             arg_validator.hpp
*
* The argument parser's C++ face: validators.
*   `cli::validator` is CLI11's Validator as a C++ value: it derives from
* struct d_arg_validator and adds nothing, owns what the C validator owns,
* copies it whole -- d_arg_validator_copy -- and releases it when it goes,
* so a validator is made, kept, copied and combined as any value is, and an
* option given one keeps a copy, or, from C++11, takes one that is moved.
* `&`, `|` and `!` combine validators as CLI11's operators do, through
* d_arg_validator_and, _or and _not.
*   Any function object that takes a std::string& and returns a std::string
* -- the empty string to pass the value, a reason to reject it -- makes a
* validator, as CLI11's Validator(op, description, name) does: the
* validator owns a copy of it, which a copied validator copies, so a lambda
* costs what it captures. What it throws, in a build with exceptions, stops
* the validation with what() as the reason, as CLI11's throw does.
*   CLI11's validators are functions here, named as CLI11 names them in lower
* case -- range(), is_member(), existing_file() -- each returning the C
* library's validator. Those of arg_validator_extra.h and
* arg_validator_path.h exist with their knobs,
* D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS and _PATH_VALIDATORS. A filter --
* ignore_case, ignore_underscore, ignore_space, or a function of the caller's
* -- is a cli::filter, so that is_member(set, cli::ignore_case) reads as
* CLI11's IsMember does; filters exist with the extra validators, as the C
* library's do.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_validator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALIDATORS
    ----------
    1.  The value
         1.  validator
    2.  Composition
         1.  operator& / operator| / operator!
    3.  Layout
         1.  The validator adds no member
    4.  Validators an option holds
         1.  validator_handle
2.  FILTERS
    -------
    1.  The value
         1.  filter
    2.  CLI11's filters
         1.  ignore_case / ignore_underscore / ignore_space
         2.  operator| (filters)
3.  CLI11'S VALIDATORS
    ------------------
    1.  Default validators
         1.  range
         2.  positive_number / non_negative_number
         3.  escaped_string
    2.  Extra validators
         1.  type_validator / number
         2.  bound
         3.  valid_ipv4
         4.  is_member
         5.  transformer / checked_transformer
         6.  unit_options
         7.  as_number_with_unit / as_size_value
    3.  Path validators
         1.  existing_file and kin
         2.  file_on_default_path
         3.  read_permissions and kin
         4.  file_size / non_empty_file
4.  INTERNALS
    ---------
    1.  Owned contexts
         1.  copy_context / release_context
         2.  copy_new
    2.  Functions as validators
         1.  validation_failure
         2.  run_function / validate_function
         3.  type_check
    3.  Functions as filters
         1.  run_filter / filter_function
         2.  composed_filter / compose_filter
         3.  failed_filter
    4.  Sets and mappings
         1.  compare_type
         2.  member_of
         3.  member_texts / mapping_texts
         4.  mapping_validator
         5.  failed_validator
    5.  Running
         1.  describe_validator / run_validator
    6.  Ranges
         1.  range_number
         2.  name_enumeration
5.  DEFINITIONS
    -----------
    1.  validator
    2.  Composition
    3.  validator_handle
    4.  filter
    5.  CLI11's validators
    6.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_VALIDATOR_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_VALIDATOR_HPP 1

// std
#include <cstddef>    // std::size_t
#include <exception>  // std::exception
#include <map>        // std::map
#include <new>        // std::nothrow
#include <string>     // std::string
#include <utility>    // std::pair
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_text
#include "../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../env/env.h"             // language levels
#include "./arg_common.hpp"               // status, text, c_string, guard
#include "./arg_type.hpp"                 // type_traits, to_text
#include "./c/arg_validator.h"            // d_arg_validator, d_arg_validation
#include "./c/arg_validator_extra.h"      // CLI11's extra validators
#include "./c/arg_validator_path.h"       // CLI11's path validators
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t,
                                                   // re_std::uint64_t
// std, from C++11
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <initializer_list>  // std::initializer_list
    #include <type_traits>       // std::is_enum
#endif


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  VALIDATORS
//==============================================================================


// 1.1    The value
//------------------------------------------------------------------------------
// 1.1.1
// validator
//   class: CLI11's Validator: a d_arg_validator owned as a value. A copy is
// whole -- the name, the description and the context, through the
// context's own copy function -- and may fail as d_arg_validator_copy
// fails: the copy is then a failed validator, whose status get_status()
// gives, and which an option it is given to records as a definition error,
// as CLI11 would throw. A validator whose function is a C++ object holds a
// copy of it, which its copies copy. The C struct's fields may be read; a
// member function hides the field of its name.
class validator : public ::d_arg_validator
{
public:
    // making: CLI11's default Validator, which passes every value and
    // describes nothing; one described by the text alone, as CLI11's
    // Validator(desc); and one over a C validator, which this one takes
    // over -- the caller must no longer release it
    validator() D_NOEXCEPT;
    explicit validator(c_string _description);
    explicit validator(const ::d_arg_validator& _owned) D_NOEXCEPT;

    /**
     * @brief Makes a validator from a function object; CLI11's
     *        Validator(op, description, name).
     *
     * @note `_function` is called with the value, as a std::string& it may
     *       change, and returns the empty string to pass it or the reason it
     *       is rejected. The validator owns a copy of it, made with `new`.
     *       An exception the function lets out, in a build with exceptions,
     *       stops the validation with what() as the reason, as CLI11's
     *       ValidationError does; std::bad_alloc stops it with
     *       status_code::out_of_memory.
     *
     * @param[in] _function     the function object; copied.
     * @param[in] _description  the description help shows; copied.
     * @param[in] _name         the name an option finds it by; copied, NULL
     *                          for none.
     * @post   std::bad_alloc, or the function's copy constructor's
     *         exception, propagates before anything is made; a description
     *         or name that could not be copied makes a failed validator.
     */
    template<typename Function>
    validator(Function _function,
              c_string _description,
              c_string _name = c_string());

    // copying, moving and releasing: a copy is whole, or a failed
    // validator; a move, from C++11, takes everything and leaves the source
    // as the default validator
    validator(const validator& _other);
    validator& operator=(const validator& _other);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    validator(validator&& _other) D_NOEXCEPT;
    validator& operator=(validator&& _other) D_NOEXCEPT;
#endif
    ~validator();

    // settings: CLI11's description(), name(), active(), non_modifying()
    // and application_index(); a copy that fails marks the validator failed
    validator& description(c_string _description);
    validator& name(c_string _name);
    validator& active(bool _active = true) D_NOEXCEPT;
    validator& non_modifying(bool _no_modify = true) D_NOEXCEPT;
    validator& application_index(int _index) D_NOEXCEPT;

    // reading: get_description() works the description out, as CLI11's
    // does, and is empty when it cannot be built; get_name() is never NULL
    std::string get_description() const;
    const char* get_name() const D_NOEXCEPT;
    bool        get_active() const D_NOEXCEPT;
    bool        get_modifying() const D_NOEXCEPT;
    int         get_application_index() const D_NOEXCEPT;
    cli::status get_status() const D_NOEXCEPT;

    /**
     * @brief Runs the validator over a value; CLI11's operator().
     *
     * @note A modifying validator rewrites `_value` as it rewrites the value
     *       it validates; an inactive one passes it untouched.
     *
     * @param[in,out] _value  the value.
     * @return the empty string when the value passes; the reason it is
     *         rejected; or, when the validator stopped with an error, the
     *         message it recorded, or the error's own text. std::bad_alloc
     *         propagates, as std::string throws it.
     */
    std::string operator()(std::string& _value) const;
    /**
     * @brief Runs the validator over a copy of a value.
     *
     * @param[in] _value  the value.
     * @return the result, as the other overload's.
     */
    std::string operator()(const std::string& _value) const;

    // handing over: the C validator, which the caller -- a C call that
    // consumes it -- now owns; this one is left the default validator
    ::d_arg_validator detach() D_NOEXCEPT;

    // swapping: two validators exchange what they hold; neither fails
    void swap(validator& _other) D_NOEXCEPT;
};

// 1.2    Composition
//------------------------------------------------------------------------------
// 1.2.1
// operator& / operator| / operator!
//   functions: CLI11's `a & b`, `a | b` and `!a`, through
// d_arg_validator_and, _or and _not, with CLI11's descriptions and messages.
// The operands are copied; from C++11, an operand that is an rvalue is
// moved instead. A copy that fails makes the result a failed validator.
validator operator&(const validator& _first,
                    const validator& _second);
validator operator|(const validator& _first,
                    const validator& _second);
validator operator!(const validator& _operand);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
validator operator&(validator&& _first,
                    validator&& _second);
validator operator&(validator&&       _first,
                    const validator& _second);
validator operator&(const validator& _first,
                    validator&&       _second);
validator operator|(validator&& _first,
                    validator&& _second);
validator operator|(validator&&       _first,
                    const validator& _second);
validator operator|(const validator& _first,
                    validator&&       _second);
validator operator!(validator&& _operand);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 1.3    Layout
//------------------------------------------------------------------------------
// 1.3.1
// The validator adds no member
//   assertion: a validator is its d_arg_validator and nothing more.
D_STATIC_ASSERT( (sizeof(validator) == sizeof(::d_arg_validator)),
                 "cli::validator adds no member to struct d_arg_validator" );

// 1.4    Validators an option holds
//------------------------------------------------------------------------------
// 1.4.1
// validator_handle
//   class: a handle on a validator an option holds -- one pointer, as CLI11's
// get_validator() gives a Validator* -- with the validator's settings and
// readers; a NULL handle, which every call ignores, when there is none. The
// option owns the validator: the handle is valid until the option is
// released or the validator removed, and copy() makes an owned one.
class validator_handle
{
public:
    validator_handle() D_NOEXCEPT;
    explicit validator_handle(::d_arg_validator* _validator) D_NOEXCEPT;

    // the handle: CLI11's ports read `v->x()`
    validator_handle*       operator->() D_NOEXCEPT;
    const validator_handle* operator->() const D_NOEXCEPT;
    ::d_arg_validator*      get() const D_NOEXCEPT;
    bool                    valid() const D_NOEXCEPT;

    // settings, as validator's
    validator_handle description(c_string _description);
    validator_handle name(c_string _name);
    validator_handle active(bool _active = true) D_NOEXCEPT;
    validator_handle non_modifying(bool _no_modify = true) D_NOEXCEPT;
    validator_handle application_index(int _index) D_NOEXCEPT;

    // reading and running, as validator's; a NULL handle reads as the
    // default validator, and copies as one
    std::string get_description() const;
    const char* get_name() const D_NOEXCEPT;
    bool        get_active() const D_NOEXCEPT;
    bool        get_modifying() const D_NOEXCEPT;
    int         get_application_index() const D_NOEXCEPT;
    cli::status get_status() const D_NOEXCEPT;
    std::string operator()(std::string& _value) const;
    validator   copy() const;

private:
    ::d_arg_validator* m_validator;
};


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// 2.  FILTERS
//==============================================================================


// 2.1    The value
//------------------------------------------------------------------------------
// 2.1.1
// filter
//   class: how a set or a mapping compares a value with its members: a
// d_arg_filter owned as a value. CLI11's three filters are flags, which the
// C library applies first, in any order; a function object of the caller's
// -- taking a std::string and returning one, as CLI11's filters do -- runs
// after them, a copy of it owned by the filter and copied with it. A filter
// is made from a function implicitly, so that is_member(set, function)
// reads as CLI11's IsMember does, and from CLI11's filters as functions,
// cli::ignore_case as well as cli::ignore_case(). Filters combine with `|`.
// A copy whose function could not be copied filters every value with
// status_code::out_of_memory, so that the failure is reported, not lost.
class filter : public ::d_arg_filter
{
public:
    // making: no filtering; one of CLI11's filters, or the flags given; a
    // function of the caller's, copied with `new`
    filter() D_NOEXCEPT;
    filter(filter (*_make)());
    explicit filter(re_std::uint32_t _flags) D_NOEXCEPT;
    template<typename Function>
    filter(Function _function);

    // copying, moving and releasing: a copy duplicates the function with
    // the context's copy function, and fails as the class says when that
    // fails
    filter(const filter& _other);
    filter& operator=(const filter& _other);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    filter(filter&& _other) D_NOEXCEPT;
    filter& operator=(filter&& _other) D_NOEXCEPT;
#endif
    ~filter();

    // reading and swapping
    re_std::uint32_t get_flags() const D_NOEXCEPT;
    bool             has_function() const D_NOEXCEPT;
    void             swap(filter& _other) D_NOEXCEPT;
};

// 2.2    CLI11's filters
//------------------------------------------------------------------------------
// 2.2.1
// ignore_case / ignore_underscore / ignore_space
//   functions: CLI11's three filters: ASCII letters compared as lower case;
// `_` removed; spaces and tabs removed. None allocates or fails; none is
// declared noexcept, so that each is a `filter (*)()` -- which filter's
// constructor takes -- at every language level, C++17's included.
filter ignore_case();
filter ignore_underscore();
filter ignore_space();

// 2.2.2
// operator| (filters)
//   function: both filters, as CLI11's nested filters: their flags together
// and, when both have a function, the first's then the second's. A function
// that could not be copied leaves the result without it.
filter operator|(const filter& _first,
                 const filter& _second);

D_STATIC_ASSERT( (sizeof(filter) == sizeof(::d_arg_filter)),
                 "cli::filter adds no member to struct d_arg_filter" );

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS


//==============================================================================
// 3.  CLI11'S VALIDATORS
//==============================================================================
// Each returns the C library's validator, whose contract (arg_validator.h,
// arg_validator_extra.h, arg_validator_path.h) says what it accepts,
// rewrites, rejects with and is described as. A construction that fails --
// a bad argument, a copy refused -- returns a failed validator, which an
// option records as a definition error.


// 3.1    Default validators
//------------------------------------------------------------------------------
// 3.1.1
// range
//   functions: CLI11's Range(min, max) and Range(max), from 0, over an
// arithmetic type -- one of the C library's built-in types -- or, from
// C++11, an enumeration, compared as the number it converts through;
// described, unless a description is given, as "<TYPE> in [<min> - <max>]",
// an enumeration's TYPE "ENUM" and its bounds numbers, as CLI11 writes an
// unscoped one's.
template<typename Number>
validator range(Number   _minimum,
                Number   _maximum,
                c_string _description = c_string());
template<typename Number>
validator range(Number   _maximum,
                c_string _description = c_string());

// 3.1.2
// positive_number / non_negative_number
//   functions: CLI11's PositiveNumber and NonNegativeNumber.
validator positive_number();
validator non_negative_number();

// 3.1.3
// escaped_string
//   function: CLI11's EscapedString.
validator escaped_string();

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 3.2    Extra validators
//------------------------------------------------------------------------------
// 3.2.1
// type_validator / number
//   functions: CLI11's TypeValidator<T> -- the value must convert to the
// type, "Failed parsing <value> as a <TYPE>" when it does not -- and
// Number. A built-in type is the C library's d_arg_validator_type; any
// other converts into an object of its own, through type_traits.
template<typename Type>
validator type_validator();
validator number();

// 3.2.2
// bound
//   functions: CLI11's Bound(min, max) and Bound(max), from 0, over an
// arithmetic type or, from C++11, an enumeration, as range(): a value past a
// bound becomes the bound, an enumeration's written as its number.
template<typename Number>
validator bound(Number _minimum,
                Number _maximum);
template<typename Number>
validator bound(Number _maximum);

// 3.2.3
// valid_ipv4
//   function: CLI11's ValidIPV4.
validator valid_ipv4();

// 3.2.4
// is_member
//   functions: CLI11's IsMember over the elements of a container, an array
// or, from C++11, a braced list, and up to three filters, combined as `|`
// combines them. An element that is a pair -- a map's included -- is its
// first, as CLI11 takes a map's keys. Members of a built-in type or an
// enumeration are compared as values of their type, as CLI11 compares
// them; any other as the text type_traits writes them as. Each member is
// written as text, which the C library copies.
template<typename Container>
validator is_member(const Container& _values,
                    const filter&    _first  = filter(),
                    const filter&    _second = filter(),
                    const filter&    _third  = filter());
template<typename Element,
         std::size_t Count>
validator is_member(const Element (&_values)[Count],
                    const filter& _first  = filter(),
                    const filter& _second = filter(),
                    const filter& _third  = filter());
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Element>
validator is_member(std::initializer_list<Element> _values,
                    const filter&                  _first  = filter(),
                    const filter&                  _second = filter(),
                    const filter&                  _third  = filter());
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 3.2.5
// transformer / checked_transformer
//   functions: CLI11's Transformer and CheckedTransformer over a mapping --
// a container of pairs, std::map among them, or, from C++11, a braced list
// of std::pair<std::string, std::string> -- and up to three filters. Keys
// compare as is_member's elements do; each key and value is written as
// text, which the C library copies.
template<typename Mapping>
validator transformer(const Mapping& _pairs,
                      const filter&  _first  = filter(),
                      const filter&  _second = filter(),
                      const filter&  _third  = filter());
template<typename Mapping>
validator checked_transformer(const Mapping& _pairs,
                              const filter&  _first  = filter(),
                              const filter&  _second = filter(),
                              const filter&  _third  = filter());
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
validator transformer(
    std::initializer_list<std::pair<std::string, std::string> > _pairs,
    const filter& _first  = filter(),
    const filter& _second = filter(),
    const filter& _third  = filter());
validator checked_transformer(
    std::initializer_list<std::pair<std::string, std::string> > _pairs,
    const filter& _first  = filter(),
    const filter& _second = filter(),
    const filter& _third  = filter());
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 3.2.6
// unit_options
//   enum: CLI11's AsNumberWithUnit::Options, the D_ARG_UNIT_* flags,
// combined with `|` as integers: defaults is CLI11's DEFAULT.
struct unit_options
{
    enum value
    {
        case_sensitive   = D_ARG_UNIT_CASE_SENSITIVE,
        case_insensitive = D_ARG_UNIT_CASE_INSENSITIVE,
        unit_optional    = D_ARG_UNIT_OPTIONAL,
        unit_required    = D_ARG_UNIT_REQUIRED,
        defaults         = D_ARG_UNIT_DEFAULT
    };
};

// 3.2.7
// as_number_with_unit / as_size_value
//   functions: CLI11's AsNumberWithUnit over a std::map of units to the
// factors of an arithmetic type, with unit_options and the unit's name in
// help ("UNIT" when none is given), and AsSizeValue.
template<typename Number>
validator as_number_with_unit(
    const std::map<std::string, Number>& _units,
    re_std::uint32_t                     _options = unit_options::defaults,
    c_string                             _name    = c_string());
validator as_size_value(bool _kb_is_1000);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// 3.3    Path validators
//------------------------------------------------------------------------------
// 3.3.1
// existing_file and kin
//   functions: CLI11's ExistingFile, ExistingDirectory, ExistingPath and
// NonexistentPath.
validator existing_file();
validator existing_directory();
validator existing_path();
validator nonexistent_path();

// 3.3.2
// file_on_default_path
//   function: CLI11's FileOnDefaultPath(default_path, enableErrorReturn).
validator file_on_default_path(c_string _default_path,
                               bool     _error_return = true);

// 3.3.3
// read_permissions and kin
//   functions: CLI11's ReadPermissions, WritePermissions and
// ExecPermissions.
validator read_permissions();
validator write_permissions();
validator exec_permissions();

// 3.3.4
// file_size / non_empty_file
//   functions: CLI11's FileSizeValidator(min, max), where dstdint.h
// declares a 64-bit integer, and NonEmptyFile.
#if defined(UINT64_MAX)
validator file_size(re_std::uint64_t _minimum,
                    re_std::uint64_t _maximum = 0u);
#endif  // UINT64_MAX
validator non_empty_file();

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS


//==============================================================================
// 4.  INTERNALS
//==============================================================================


// 4.1    Owned contexts
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.1.1
    // copy_context / release_context
    //   functions: the copy and release functions of a context that is a C++
    //   object, as d_arg_copy_fn and d_arg_release_fn: a copy made with
    //   `new (std::nothrow)`, NULL when it could not be made -- the
    //   allocation refused, or the copy constructor throwing, which is
    //   caught -- and `delete`.
    template<typename Context>
    void* copy_context(const void* _context);
    template<typename Context>
    void  release_context(void* _context);

    // 4.1.2
    // copy_new
    //   class: the work copy_context guards.
    template<typename Context>
    class copy_new
    {
    public:
        typedef void* result_type;

        explicit copy_new(const void* _context) D_NOEXCEPT;

        void* operator()() const;

    private:
        const Context* m_context;
    };

NS_END  // internal

// 4.2    Functions as validators
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.2.1
    // validation_failure
    //   class: the handler of guard for a validator's function: memory that
    //   ran out stops the validation as status_code::out_of_memory; any
    //   other exception rejects the value and stops the validation as
    //   status_code::validation_error, with what() as the reason, or, with
    //   no words, the status's own -- an exception never passes the value,
    //   as an empty reason would -- and a reason that cannot be kept stops
    //   it with that failure.
    class validation_failure
    {
    public:
        typedef int result_type;

        explicit validation_failure(
            ::d_arg_validation* _validation) D_NOEXCEPT;

        int operator()(const std::exception* _exception,
                       bool                  _out_of_memory) const;

    private:
        ::d_arg_validation* m_validation;
    };

    // 4.2.2
    // run_function / validate_function
    //   class, function: a function validator's work, as d_arg_validate_fn:
    //   the value as a std::string, given to the function, which may change
    //   it; a change replaces the value, and a reason returned rejects it.
    template<typename Function>
    class run_function
    {
    public:
        typedef int result_type;

        run_function(const ::d_arg_validator* _validator,
                     ::d_arg_validation*      _validation) D_NOEXCEPT;

        int operator()() const;

    private:
        Function*           m_function;
        ::d_arg_validation* m_validation;
    };
    template<typename Function>
    int validate_function(const ::d_arg_validator* _validator,
                          ::d_arg_validation*      _validation);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

    // 4.2.3
    // type_check
    //   class: the function of a type validator for a type the C library
    //   cannot make objects of: convert() into a Type of its own, rejecting
    //   with CLI11's "Failed parsing <value> as a <TYPE>".
    template<typename Type>
    class type_check
    {
    public:
        std::string operator()(std::string& _value) const;
    };

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

NS_END  // internal

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 4.3    Functions as filters
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.3.1
    // run_filter / filter_function
    //   class, function: a filter function's work, as d_arg_filter_fn: the
    //   value as a std::string, given to the function, whose result is
    //   appended to the buffer. An exception is answered as a converter's.
    template<typename Function>
    class run_filter
    {
    public:
        typedef int result_type;

        run_filter(::d_pack_text        _value,
                   ::d_arg_buffer*      _out,
                   void*                _context) D_NOEXCEPT;

        int operator()() const;

    private:
        ::d_pack_text   m_value;
        ::d_arg_buffer* m_out;
        Function*       m_function;
    };
    template<typename Function>
    int filter_function(::d_pack_text   _value,
                        ::d_arg_buffer* _out,
                        void*           _context);

    // 4.3.2
    // composed_filter / compose_filter
    //   class, function: two filters' functions run one after the other, the
    //   context of a combined filter's function: the first's into a buffer
    //   of its own, then the second's over that. It holds the two filters,
    //   so it copies and releases their functions as they do.
    class composed_filter
    {
    public:
        composed_filter(const filter& _first,
                        const filter& _second);

        int run(::d_pack_text   _value,
                ::d_arg_buffer* _out) const;

    private:
        filter m_first;
        filter m_second;
    };
    int compose_filter(::d_pack_text   _value,
                       ::d_arg_buffer* _out,
                       void*           _context);

    // 4.3.3
    // failed_filter
    //   function: the function of a filter whose own could not be copied:
    //   every value fails with D_ARG_ERROR_OUT_OF_MEMORY.
    int failed_filter(::d_pack_text   _value,
                      ::d_arg_buffer* _out,
                      void*           _context);

NS_END  // internal

// 4.4    Sets and mappings
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.4.1
    // compare_type
    //   trait: the descriptor a set compares its members as: the type's own
    //   for one of the C library's built-in types other than the text types
    //   and, from C++11, for an enumeration -- objects the C library may make
    //   without constructing them -- and NULL, comparing text, for any other.
    template<typename Type>
    struct compare_type
    {
        static const ::d_arg_type* descriptor() D_NOEXCEPT;
    };

    // 4.4.2
    // member_of
    //   trait: the part of an element a set compares, as CLI11's
    //   pair_adaptor reads one: a pair's first -- a map's key -- and any
    //   other element whole; `type` is that part's type, without the const
    //   of a map's key, and `get()` reaches it.
    template<typename Element>
    struct member_of
    {
        typedef Element type;

        static const Element& get(const Element& _element) D_NOEXCEPT;
    };

    template<typename First,
             typename Second>
    struct member_of<std::pair<First, Second> >
    {
        typedef typename remove_const<First>::type type;

        static const First& get(
            const std::pair<First, Second>& _element) D_NOEXCEPT;
    };

    // 4.4.3
    // member_texts / mapping_texts
    //   classes: the text of every element of a set, or of every key and
    //   value of a mapping, as type_traits writes them, kept in strings, with
    //   the C pointers to them that the C constructors take. `status` is the
    //   first element that could not be written's.
    class member_texts
    {
    public:
        member_texts();

        template<typename Element>
        void add(const Element& _element);

        const char* const* items();
        std::size_t        count() const D_NOEXCEPT;
        cli::status        get_status() const D_NOEXCEPT;

    private:
        std::vector<std::string> m_texts;
        std::vector<const char*> m_items;
        cli::status              m_status;
    };
    class mapping_texts
    {
    public:
        mapping_texts();

        template<typename Key,
                 typename Value>
        void add(const Key&   _key,
                 const Value& _value);

        const ::d_arg_mapping* items();
        std::size_t            count() const D_NOEXCEPT;
        cli::status            get_status() const D_NOEXCEPT;

    private:
        std::vector<std::string>      m_texts;
        std::vector< ::d_arg_mapping> m_items;
        cli::status                   m_status;
    };

    // 4.4.4
    // mapping_validator
    //   function: the work transformer() and checked_transformer() share:
    //   each key and value written as text, then the C constructor, checked
    //   or not.
    template<typename Mapping>
    validator mapping_validator(const Mapping& _pairs,
                                const filter&  _filter,
                                bool           _checked);

    // 4.4.5
    // failed_validator
    //   function: a validator that failed with a status, for a set whose
    //   members could not be written: the C library's failed validator --
    //   which owns nothing -- carrying the status.
    validator failed_validator(const cli::status& _status);

NS_END  // internal

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// 4.5    Running
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.5.1
    // describe_validator / run_validator
    //   functions: the work of validator's get_description() and
    //   operator(), shared with validator_handle's, over any C validator:
    //   the description worked out, empty when it cannot be built; and a run
    //   over a std::string, rewriting it as a modifying validator does and
    //   returning the reason it was rejected, or, for an error with none,
    //   the error's own text. std::bad_alloc propagates, as std::string
    //   throws it; the validation is released however the strings fare.
    std::string describe_validator(const ::d_arg_validator* _validator);
    std::string run_validator(const ::d_arg_validator* _validator,
                              std::string&             _value);

NS_END  // internal

// 4.6    Ranges
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.6.1
    // range_number
    //   trait: the type a range or a bound compares as, `type`: the type
    //   itself, or, from C++11, the number an enumeration converts through
    //   (enum_number), `enumeration` telling which.
    template<typename Number,
             typename Enable = void>
    struct range_number
    {
        typedef Number type;

        static const bool enumeration = false;
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename Number>
    struct range_number<
               Number,
               typename enable_if<std::is_enum<Number>::value>::type>
    {
        typedef typename enum_number<Number>::type type;

        static const bool enumeration = true;
    };
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // 4.6.2
    // name_enumeration
    //   function: gives a range's or a bound's description the name CLI11's
    //   type_name gives an enumeration, "ENUM", in place of the name of the
    //   number it compares as; a description that could not be worked out is
    //   left as it is.
    void name_enumeration(validator& _validator);

NS_END  // internal


//==============================================================================
// 5.  DEFINITIONS
//==============================================================================


// 5.1    validator
//------------------------------------------------------------------------------
// validator::validator
//   d_arg_validator_init; a custom validator with no function; a C
// validator's bits, taken over.
inline
validator::validator() D_NOEXCEPT
{
    d_arg_validator_init(this);
}

inline
validator::validator(
    c_string _description
)
{
    static_cast< ::d_arg_validator&>(*this) =
        d_arg_validator_custom(D_NULLPTR,
                               D_NULLPTR,
                               _description.get());
}

inline
validator::validator(
    const ::d_arg_validator& _owned
) D_NOEXCEPT
{
    static_cast< ::d_arg_validator&>(*this) = _owned;
}

// validator::validator (function)
//   The function copied to the heap, made the context of a custom validator,
// then given to it to own: a description that could not be copied leaves
// the validator failed, holding nothing, so the copy is deleted here.
template<typename Function>
inline
validator::validator(
    Function _function,
    c_string _description,
    c_string _name
)
{
    Function* const      owned = new Function(_function);
    ::d_arg_validator    made  =
        d_arg_validator_custom(&internal::validate_function<Function>,
                               owned,
                               _description.get());

    // a failed validator did not take the context
    if (made.status != D_ARG_OK)
    {
        delete owned;
    }
    else
    {
        d_arg_validator_own_context(&made,
                                    &internal::copy_context<Function>,
                                    &internal::release_context<Function>);
    }

    static_cast< ::d_arg_validator&>(*this) = made;

    // a name, when one is given
    if (_name.get() != D_NULLPTR)
    {
        (void)d_arg_validator_set_name(this,
                                       _name.get());
    }
}

// validator::validator (copy) / operator= (copy)
//   d_arg_validator_copy, whose failure is a failed validator; assignment by
// copy and swap, so a failed copy leaves the target as it was.
inline
validator::validator(
    const validator& _other
)
    : ::d_arg_validator(d_arg_validator_copy(&_other))
{}

inline validator&
validator::operator=(
    const validator& _other
)
{
    validator duplicate(_other);

    swap(duplicate);

    return *this;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// validator::validator (move) / operator= (move)
//   The bits taken, the source made the default validator, which owns
// nothing; assignment by swap, the old one released with the source.
inline
validator::validator(
    validator&& _other
) D_NOEXCEPT
    : ::d_arg_validator(_other.detach())
{}

inline validator&
validator::operator=(
    validator&& _other
) D_NOEXCEPT
{
    validator taken(static_cast<validator&&>(_other));

    swap(taken);

    return *this;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// validator::~validator
//   d_arg_validator_release.
inline
validator::~validator()
{
    d_arg_validator_release(this);
}

// validator::description / name / active / non_modifying /
// application_index
//   The C settings.
inline validator&
validator::description(
    c_string _description
)
{
    (void)d_arg_validator_set_description(this,
                                          _description.get());

    return *this;
}

inline validator&
validator::name(
    c_string _name
)
{
    (void)d_arg_validator_set_name(this,
                                   _name.get());

    return *this;
}

inline validator&
validator::active(
    bool _active
) D_NOEXCEPT
{
    d_arg_validator_set_active(this,
                               _active);

    return *this;
}

inline validator&
validator::non_modifying(
    bool _no_modify
) D_NOEXCEPT
{
    d_arg_validator_set_modifying(this,
                                  !_no_modify);

    return *this;
}

inline validator&
validator::application_index(
    int _index
) D_NOEXCEPT
{
    d_arg_validator_set_application_index(this,
                                          _index);

    return *this;
}

// validator::get_description
//   d_arg_validator_describe into a buffer, then a string of what it holds;
// empty when the buffer could not grow.
inline std::string
validator::get_description() const
{
    return internal::describe_validator(this);
}

// validator::get_name / get_active / get_modifying / get_application_index /
// get_status
//   The fields.
inline const char*
validator::get_name() const D_NOEXCEPT
{
    return (this->d_arg_validator::name != D_NULLPTR)
           ? this->d_arg_validator::name
           : "";
}

inline bool
validator::get_active() const D_NOEXCEPT
{
    return ((this->d_arg_validator::flags & D_ARG_VALIDATOR_ACTIVE) != 0u);
}

inline bool
validator::get_modifying() const D_NOEXCEPT
{
    return ((this->d_arg_validator::flags & D_ARG_VALIDATOR_MODIFYING) != 0u);
}

inline int
validator::get_application_index() const D_NOEXCEPT
{
    return this->d_arg_validator::application_index;
}

inline cli::status
validator::get_status() const D_NOEXCEPT
{
    return cli::status(this->d_arg_validator::status);
}

// validator::operator()
//   internal::run_validator over this one.
inline std::string
validator::operator()(
    std::string& _value
) const
{
    return internal::run_validator(this,
                                   _value);
}

inline std::string
validator::operator()(
    const std::string& _value
) const
{
    std::string value(_value);

    return (*this)(value);
}

// validator::detach / swap
//   The bits handed out and the default made in their place; the bits
// exchanged.
inline ::d_arg_validator
validator::detach() D_NOEXCEPT
{
    const ::d_arg_validator detached = *this;

    d_arg_validator_init(this);

    return detached;
}

inline void
validator::swap(
    validator& _other
) D_NOEXCEPT
{
    const ::d_arg_validator held = *this;

    static_cast< ::d_arg_validator&>(*this)  = _other;
    static_cast< ::d_arg_validator&>(_other) = held;

    return;
}

// 5.2    Composition
//------------------------------------------------------------------------------
// operator& / operator| / operator!
//   Copies of the operands, handed to the C operator, which consumes them;
// rvalues handed over as they are.
inline validator
operator&(
    const validator& _first,
    const validator& _second
)
{
    validator first(_first);
    validator second(_second);

    return validator(d_arg_validator_and(first.detach(),
                                         second.detach()));
}

inline validator
operator|(
    const validator& _first,
    const validator& _second
)
{
    validator first(_first);
    validator second(_second);

    return validator(d_arg_validator_or(first.detach(),
                                        second.detach()));
}

inline validator
operator!(
    const validator& _operand
)
{
    validator operand(_operand);

    return validator(d_arg_validator_not(operand.detach()));
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

inline validator
operator&(
    validator&& _first,
    validator&& _second
)
{
    return validator(d_arg_validator_and(_first.detach(),
                                         _second.detach()));
}

inline validator
operator&(
    validator&&      _first,
    const validator& _second
)
{
    validator second(_second);

    return validator(d_arg_validator_and(_first.detach(),
                                         second.detach()));
}

inline validator
operator&(
    const validator& _first,
    validator&&      _second
)
{
    validator first(_first);

    return validator(d_arg_validator_and(first.detach(),
                                         _second.detach()));
}

inline validator
operator|(
    validator&& _first,
    validator&& _second
)
{
    return validator(d_arg_validator_or(_first.detach(),
                                        _second.detach()));
}

inline validator
operator|(
    validator&&      _first,
    const validator& _second
)
{
    validator second(_second);

    return validator(d_arg_validator_or(_first.detach(),
                                        second.detach()));
}

inline validator
operator|(
    const validator& _first,
    validator&&      _second
)
{
    validator first(_first);

    return validator(d_arg_validator_or(first.detach(),
                                        _second.detach()));
}

inline validator
operator!(
    validator&& _operand
)
{
    return validator(d_arg_validator_not(_operand.detach()));
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 5.3    validator_handle
//------------------------------------------------------------------------------
// validator_handle
//   The pointer; the C settings, which ignore NULL; the shared readers over
// it, and the defaults for NULL.
inline
validator_handle::validator_handle() D_NOEXCEPT
    : m_validator(D_NULLPTR)
{}

inline
validator_handle::validator_handle(
    ::d_arg_validator* _validator
) D_NOEXCEPT
    : m_validator(_validator)
{}

inline validator_handle*
validator_handle::operator->() D_NOEXCEPT
{
    return this;
}

inline const validator_handle*
validator_handle::operator->() const D_NOEXCEPT
{
    return this;
}

inline ::d_arg_validator*
validator_handle::get() const D_NOEXCEPT
{
    return m_validator;
}

inline bool
validator_handle::valid() const D_NOEXCEPT
{
    return (m_validator != D_NULLPTR);
}

inline validator_handle
validator_handle::description(
    c_string _description
)
{
    (void)d_arg_validator_set_description(m_validator,
                                          _description.get());

    return *this;
}

inline validator_handle
validator_handle::name(
    c_string _name
)
{
    (void)d_arg_validator_set_name(m_validator,
                                   _name.get());

    return *this;
}

inline validator_handle
validator_handle::active(
    bool _active
) D_NOEXCEPT
{
    d_arg_validator_set_active(m_validator,
                               _active);

    return *this;
}

inline validator_handle
validator_handle::non_modifying(
    bool _no_modify
) D_NOEXCEPT
{
    d_arg_validator_set_modifying(m_validator,
                                  !_no_modify);

    return *this;
}

inline validator_handle
validator_handle::application_index(
    int _index
) D_NOEXCEPT
{
    d_arg_validator_set_application_index(m_validator,
                                          _index);

    return *this;
}

inline std::string
validator_handle::get_description() const
{
    return (m_validator != D_NULLPTR)
           ? internal::describe_validator(m_validator)
           : std::string();
}

inline const char*
validator_handle::get_name() const D_NOEXCEPT
{
    return ( (m_validator != D_NULLPTR) &&
             (m_validator->name != D_NULLPTR) )
           ? m_validator->name
           : "";
}

inline bool
validator_handle::get_active() const D_NOEXCEPT
{
    return ( (m_validator == D_NULLPTR) ||
             ((m_validator->flags & D_ARG_VALIDATOR_ACTIVE) != 0u) );
}

inline bool
validator_handle::get_modifying() const D_NOEXCEPT
{
    return ( (m_validator == D_NULLPTR) ||
             ((m_validator->flags & D_ARG_VALIDATOR_MODIFYING) != 0u) );
}

inline int
validator_handle::get_application_index() const D_NOEXCEPT
{
    return (m_validator != D_NULLPTR)
           ? m_validator->application_index
           : -1;
}

inline cli::status
validator_handle::get_status() const D_NOEXCEPT
{
    return (m_validator != D_NULLPTR)
           ? cli::status(m_validator->status)
           : cli::status();
}

inline std::string
validator_handle::operator()(
    std::string& _value
) const
{
    return (m_validator != D_NULLPTR)
           ? internal::run_validator(m_validator,
                                     _value)
           : std::string();
}

inline validator
validator_handle::copy() const
{
    return (m_validator != D_NULLPTR)
           ? validator(d_arg_validator_copy(m_validator))
           : validator();
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 5.4    filter
//------------------------------------------------------------------------------
// filter::filter
//   No flags and no function; the flags of a filter one of CLI11's filter
// functions makes; the flags given; a function copied to the heap with its
// copy and release functions. A failed copy of a function leaves none.
inline
filter::filter() D_NOEXCEPT
{
    this->d_arg_filter::flags    = 0u;
    this->d_arg_filter::function = D_NULLPTR;
    this->d_arg_filter::context  = D_NULLPTR;
    this->d_arg_filter::copy     = D_NULLPTR;
    this->d_arg_filter::release  = D_NULLPTR;
}

inline
filter::filter(
    filter (*_make)()
)
{
    filter made = _make();

    this->d_arg_filter::flags    = 0u;
    this->d_arg_filter::function = D_NULLPTR;
    this->d_arg_filter::context  = D_NULLPTR;
    this->d_arg_filter::copy     = D_NULLPTR;
    this->d_arg_filter::release  = D_NULLPTR;
    swap(made);
}

inline
filter::filter(
    re_std::uint32_t _flags
) D_NOEXCEPT
{
    this->d_arg_filter::flags    = _flags;
    this->d_arg_filter::function = D_NULLPTR;
    this->d_arg_filter::context  = D_NULLPTR;
    this->d_arg_filter::copy     = D_NULLPTR;
    this->d_arg_filter::release  = D_NULLPTR;
}

template<typename Function>
inline
filter::filter(
    Function _function
)
{
    this->d_arg_filter::flags    = 0u;
    this->d_arg_filter::function = &internal::filter_function<Function>;
    this->d_arg_filter::context  = new Function(_function);
    this->d_arg_filter::copy     = &internal::copy_context<Function>;
    this->d_arg_filter::release  = &internal::release_context<Function>;
}

// filter::filter (copy) / operator= (copy)
//   The flags and functions copied, the context through its copy function;
// one that could not be copied leaves the failing function in its place.
inline
filter::filter(
    const filter& _other
)
{
    this->d_arg_filter::flags    = _other.d_arg_filter::flags;
    this->d_arg_filter::function = _other.d_arg_filter::function;
    this->d_arg_filter::copy     = _other.d_arg_filter::copy;
    this->d_arg_filter::release  = _other.d_arg_filter::release;
    this->d_arg_filter::context  =
        ( (_other.d_arg_filter::context != D_NULLPTR) &&
          (_other.d_arg_filter::copy != D_NULLPTR) )
        ? _other.d_arg_filter::copy(_other.d_arg_filter::context)
        : _other.d_arg_filter::context;

    // a function whose context could not be copied fails every value
    if ( (_other.d_arg_filter::context != D_NULLPTR) &&
         (this->d_arg_filter::context == D_NULLPTR) )
    {
        this->d_arg_filter::function = &internal::failed_filter;
        this->d_arg_filter::copy     = D_NULLPTR;
        this->d_arg_filter::release  = D_NULLPTR;
    }
}

inline filter&
filter::operator=(
    const filter& _other
)
{
    filter duplicate(_other);

    swap(duplicate);

    return *this;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// filter::filter (move) / operator= (move)
//   The bits taken, the source left empty.
inline
filter::filter(
    filter&& _other
) D_NOEXCEPT
    : ::d_arg_filter(_other)
{
    _other.d_arg_filter::function = D_NULLPTR;
    _other.d_arg_filter::context  = D_NULLPTR;
    _other.d_arg_filter::copy     = D_NULLPTR;
    _other.d_arg_filter::release  = D_NULLPTR;
}

inline filter&
filter::operator=(
    filter&& _other
) D_NOEXCEPT
{
    filter taken(static_cast<filter&&>(_other));

    swap(taken);

    return *this;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// filter::~filter
//   The context released when the filter owns it.
inline
filter::~filter()
{
    // an owned context goes with the filter
    if ( (this->d_arg_filter::context != D_NULLPTR) &&
         (this->d_arg_filter::release != D_NULLPTR) )
    {
        this->d_arg_filter::release(this->d_arg_filter::context);
    }
}

// filter::get_flags / has_function / swap
//   The fields; the bits exchanged.
inline re_std::uint32_t
filter::get_flags() const D_NOEXCEPT
{
    return this->d_arg_filter::flags;
}

inline bool
filter::has_function() const D_NOEXCEPT
{
    return (this->d_arg_filter::function != D_NULLPTR);
}

inline void
filter::swap(
    filter& _other
) D_NOEXCEPT
{
    const ::d_arg_filter held = *this;

    static_cast< ::d_arg_filter&>(*this)  = _other;
    static_cast< ::d_arg_filter&>(_other) = held;

    return;
}

// ignore_case / ignore_underscore / ignore_space
//   The flags.
inline filter
ignore_case()
{
    return filter(D_ARG_FILTER_IGNORE_CASE);
}

inline filter
ignore_underscore()
{
    return filter(D_ARG_FILTER_IGNORE_UNDERSCORE);
}

inline filter
ignore_space()
{
    return filter(D_ARG_FILTER_IGNORE_SPACE);
}

// operator| (filters)
//   The flags together; one function kept as it is, or two composed in a
// context of their own, which copies them.
inline filter
operator|(
    const filter& _first,
    const filter& _second
)
{
    // with at most one function, that one is kept
    if (!( (_first.has_function()) &&
           (_second.has_function()) ))
    {
        filter combined(_first.has_function() ? _first : _second);

        combined.d_arg_filter::flags = (_first.get_flags() |
                                        _second.get_flags());

        return combined;
    }

    filter combined(_first.get_flags() | _second.get_flags());

    combined.d_arg_filter::context  =
        new internal::composed_filter(_first,
                                      _second);
    combined.d_arg_filter::function = &internal::compose_filter;
    combined.d_arg_filter::copy     =
        &internal::copy_context<internal::composed_filter>;
    combined.d_arg_filter::release  =
        &internal::release_context<internal::composed_filter>;

    return combined;
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// 5.5    CLI11's validators
//------------------------------------------------------------------------------
// range
//   d_arg_validator_range over the type's descriptor, then the description
// given, if any.
template<typename Number>
inline validator
range(
    Number   _minimum,
    Number   _maximum,
    c_string _description
)
{
    typedef typename internal::range_number<Number>::type number_type;

    const number_type minimum = static_cast<number_type>(_minimum);
    const number_type maximum = static_cast<number_type>(_maximum);
    validator         made(d_arg_validator_range(
                               type_traits<number_type>::descriptor(),
                               &minimum,
                               &maximum));

    // a description given replaces the one worked out, which names an
    // enumeration as CLI11 does
    if (_description.get() != D_NULLPTR)
    {
        made.description(_description);
    }
    else if (internal::range_number<Number>::enumeration)
    {
        internal::name_enumeration(made);
    }

    return made;
}

template<typename Number>
inline validator
range(
    Number   _maximum,
    c_string _description
)
{
    return range(static_cast<Number>(0),
                 _maximum,
                 _description);
}

// positive_number / non_negative_number / escaped_string
//   The C constructors.
inline validator
positive_number()
{
    return validator(d_arg_validator_positive_number());
}

inline validator
non_negative_number()
{
    return validator(d_arg_validator_non_negative_number());
}

inline validator
escaped_string()
{
    return validator(d_arg_validator_escaped_string());
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// type_validator / number
//   A built-in type through d_arg_validator_type; any other through a
// function that converts into an object of the type, rejecting as CLI11's
// TypeValidator rejects.
template<typename Type>
inline validator
type_validator()
{
    return (internal::builtin_of<Type>::value >= 0)
           ? validator(d_arg_validator_type(type_traits<Type>::descriptor()))
           : validator(internal::type_check<Type>(),
                       type_name<Type>());
}

inline validator
number()
{
    return validator(d_arg_validator_number());
}

// bound
//   d_arg_validator_bound over the type's descriptor, or an enumeration's
// number's, renamed.
template<typename Number>
inline validator
bound(
    Number _minimum,
    Number _maximum
)
{
    typedef typename internal::range_number<Number>::type number_type;

    const number_type minimum = static_cast<number_type>(_minimum);
    const number_type maximum = static_cast<number_type>(_maximum);
    validator         made(d_arg_validator_bound(
                               type_traits<number_type>::descriptor(),
                               &minimum,
                               &maximum));

    // the description names an enumeration as CLI11 does
    if (internal::range_number<Number>::enumeration)
    {
        internal::name_enumeration(made);
    }

    return made;
}

template<typename Number>
inline validator
bound(
    Number _maximum
)
{
    return bound(static_cast<Number>(0),
                 _maximum);
}

// valid_ipv4
//   The C constructor.
inline validator
valid_ipv4()
{
    return validator(d_arg_validator_valid_ipv4());
}

// is_member
//   Each element's member -- a pair's first, any other element whole --
// written as text, then the C constructor over the texts, the member type's
// descriptor to compare as, and the filters combined.
template<typename Container>
inline validator
is_member(
    const Container& _values,
    const filter&    _first,
    const filter&    _second,
    const filter&    _third
)
{
    typedef typename Container::value_type                   element_type;
    typedef typename internal::member_of<element_type>::type member_type;
    typedef typename Container::const_iterator               iterator;

    internal::member_texts members;

    // each element's member as the text its type writes
    for (iterator element = _values.begin();
         element != _values.end();
         ++element)
    {
        members.add(internal::member_of<element_type>::get(*element));
    }

    // an element that could not be written fails the validator
    if (!members.get_status().ok())
    {
        return internal::failed_validator(members.get_status());
    }

    const filter combined = _first | _second | _third;

    return validator(d_arg_validator_is_member(
        internal::compare_type<member_type>::descriptor(),
        members.items(),
        members.count(),
        &combined));
}

template<typename Element,
         std::size_t Count>
inline validator
is_member(
    const Element (&_values)[Count],
    const filter& _first,
    const filter& _second,
    const filter& _third
)
{
    return is_member(std::vector<Element>(&_values[0],
                                          &_values[0] + Count),
                     _first,
                     _second,
                     _third);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

template<typename Element>
inline validator
is_member(
    std::initializer_list<Element> _values,
    const filter&                  _first,
    const filter&                  _second,
    const filter&                  _third
)
{
    return is_member(std::vector<Element>(_values),
                     _first,
                     _second,
                     _third);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// transformer / checked_transformer
//   internal::mapping_validator, with the filters combined.
template<typename Mapping>
inline validator
transformer(
    const Mapping& _pairs,
    const filter&  _first,
    const filter&  _second,
    const filter&  _third
)
{
    return internal::mapping_validator(_pairs,
                                       _first | _second | _third,
                                       false);
}

template<typename Mapping>
inline validator
checked_transformer(
    const Mapping& _pairs,
    const filter&  _first,
    const filter&  _second,
    const filter&  _third
)
{
    return internal::mapping_validator(_pairs,
                                       _first | _second | _third,
                                       true);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

inline validator
transformer(
    std::initializer_list<std::pair<std::string, std::string> > _pairs,
    const filter& _first,
    const filter& _second,
    const filter& _third
)
{
    return transformer(
        std::vector<std::pair<std::string, std::string> >(_pairs),
        _first,
        _second,
        _third);
}

inline validator
checked_transformer(
    std::initializer_list<std::pair<std::string, std::string> > _pairs,
    const filter& _first,
    const filter& _second,
    const filter& _third
)
{
    return checked_transformer(
        std::vector<std::pair<std::string, std::string> >(_pairs),
        _first,
        _second,
        _third);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// as_number_with_unit / as_size_value
//   Each factor written as text, then the C constructor.
template<typename Number>
inline validator
as_number_with_unit(
    const std::map<std::string, Number>& _units,
    re_std::uint32_t                     _options,
    c_string                             _name
)
{
    typedef typename std::map<std::string, Number>::const_iterator iterator;

    internal::mapping_texts units;

    // each unit and its factor as text
    for (iterator unit = _units.begin();
         unit != _units.end();
         ++unit)
    {
        units.add(unit->first,
                  unit->second);
    }

    // a factor that could not be written fails the validator
    if (!units.get_status().ok())
    {
        return internal::failed_validator(units.get_status());
    }

    return validator(d_arg_validator_as_number_with_unit(
        type_traits<Number>::descriptor(),
        units.items(),
        units.count(),
        _options,
        _name.get()));
}

inline validator
as_size_value(
    bool _kb_is_1000
)
{
    return validator(d_arg_validator_as_size_value(_kb_is_1000));
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// existing_file and kin / file_on_default_path / read_permissions and kin /
// file_size / non_empty_file
//   The C constructors.
inline validator
existing_file()
{
    return validator(d_arg_validator_existing_file());
}

inline validator
existing_directory()
{
    return validator(d_arg_validator_existing_directory());
}

inline validator
existing_path()
{
    return validator(d_arg_validator_existing_path());
}

inline validator
nonexistent_path()
{
    return validator(d_arg_validator_nonexistent_path());
}

inline validator
file_on_default_path(
    c_string _default_path,
    bool     _error_return
)
{
    return validator(d_arg_validator_file_on_default_path(_default_path.get(),
                                                          _error_return));
}

inline validator
read_permissions()
{
    return validator(d_arg_validator_read_permissions());
}

inline validator
write_permissions()
{
    return validator(d_arg_validator_write_permissions());
}

inline validator
exec_permissions()
{
    return validator(d_arg_validator_exec_permissions());
}

#if defined(UINT64_MAX)

inline validator
file_size(
    re_std::uint64_t _minimum,
    re_std::uint64_t _maximum
)
{
    return validator(d_arg_validator_file_size(_minimum,
                                               _maximum));
}

#endif  // UINT64_MAX

inline validator
non_empty_file()
{
    return validator(d_arg_validator_non_empty_file());
}

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// 5.6    Internals
//------------------------------------------------------------------------------
// internal::copy_new / copy_context / release_context
//   `new (std::nothrow)` of a copy, under the guard, NULL for any failure;
// `delete`.
template<typename Context>
inline
internal::copy_new<Context>::copy_new(
    const void* _context
) D_NOEXCEPT
    : m_context(static_cast<const Context*>(_context))
{}

template<typename Context>
inline void*
internal::copy_new<Context>::operator()() const
{
    return new (std::nothrow) Context(*m_context);
}

template<typename Context>
inline void*
internal::copy_context(
    const void* _context
)
{
    return internal::guard(internal::copy_new<Context>(_context),
                           internal::fallback<void*>(D_NULLPTR));
}

template<typename Context>
inline void
internal::release_context(
    void* _context
)
{
    delete static_cast<Context*>(_context);

    return;
}

// internal::validation_failure
//   Out of memory; or a validation error, the value rejected with the
// exception's words, or the status's own, as the reason; or the failure to
// keep the reason.
inline
internal::validation_failure::validation_failure(
    ::d_arg_validation* _validation
) D_NOEXCEPT
    : m_validation(_validation)
{}

inline int
internal::validation_failure::operator()(
    const std::exception* _exception,
    bool                  _out_of_memory
) const
{
    // a refused allocation is the C library's out-of-memory error
    if (_out_of_memory)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const char* const words    = internal::exception_words(_exception);
    // the exception's own words are the reason, or the status's own
    const int         rejected = d_arg_validation_reject(
                                     m_validation,
                                     (words != D_NULLPTR)
                                     ? words
                                     : d_arg_status_message(
                                           D_ARG_ERROR_VALIDATION));

    return (rejected != D_ARG_OK) ? rejected : D_ARG_ERROR_VALIDATION;
}

// internal::run_function / validate_function
//   The value as a string, given to the function: a changed value replaces
// it, and a reason rejects it, as CLI11's func_ does both.
template<typename Function>
inline
internal::run_function<Function>::run_function(
    const ::d_arg_validator* _validator,
    ::d_arg_validation*      _validation
) D_NOEXCEPT
    : m_function(static_cast<Function*>(_validator->data)),
      m_validation(_validation)
{}

template<typename Function>
inline int
internal::run_function<Function>::operator()() const
{
    std::string       value  = text(m_validation->value).str();
    const std::string reason = (*m_function)(value);

    // a rewritten value replaces the one under validation
    if (text(value) != text(m_validation->value))
    {
        const int replaced = d_arg_validation_replace(m_validation,
                                                      text(value));

        // a replacement that could not be stored stops the validation
        if (replaced != D_ARG_OK)
        {
            return replaced;
        }
    }

    return reason.empty()
           ? D_ARG_OK
           : d_arg_validation_reject(m_validation,
                                     reason.c_str());
}

template<typename Function>
inline int
internal::validate_function(
    const ::d_arg_validator* _validator,
    ::d_arg_validation*      _validation
)
{
    return internal::guard(internal::run_function<Function>(_validator,
                                                            _validation),
                           internal::validation_failure(_validation));
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// internal::run_filter / filter_function
//   The value as a string, given to the function, its result appended.
template<typename Function>
inline
internal::run_filter<Function>::run_filter(
    ::d_pack_text   _value,
    ::d_arg_buffer* _out,
    void*           _context
) D_NOEXCEPT
    : m_value(_value),
      m_out(_out),
      m_function(static_cast<Function*>(_context))
{}

template<typename Function>
inline int
internal::run_filter<Function>::operator()() const
{
    const std::string filtered = (*m_function)(text(m_value).str());

    return d_arg_buffer_append(m_out,
                               filtered.data(),
                               filtered.size());
}

template<typename Function>
inline int
internal::filter_function(
    ::d_pack_text   _value,
    ::d_arg_buffer* _out,
    void*           _context
)
{
    return internal::guard(internal::run_filter<Function>(_value,
                                                          _out,
                                                          _context),
                           internal::conversion_failure(D_NULLPTR));
}

// internal::composed_filter / compose_filter
//   Two filters, copied: the first's function into a buffer, the second's
// over the buffer's text into the output.
inline
internal::composed_filter::composed_filter(
    const filter& _first,
    const filter& _second
)
    : m_first(_first),
      m_second(_second)
{}

inline int
internal::composed_filter::run(
    ::d_pack_text   _value,
    ::d_arg_buffer* _out
) const
{
    ::d_arg_buffer middle = D_ARG_BUFFER_INIT;

    int filtered = m_first.d_arg_filter::function(
        _value,
        &middle,
        m_first.d_arg_filter::context);

    filtered = (filtered == D_ARG_OK)
               ? m_second.d_arg_filter::function(
                     d_arg_buffer_text(&middle),
                     _out,
                     m_second.d_arg_filter::context)
               : filtered;

    d_arg_buffer_release(&middle);

    return filtered;
}

inline int
internal::compose_filter(
    ::d_pack_text   _value,
    ::d_arg_buffer* _out,
    void*           _context
)
{
    const internal::composed_filter* const composed =
        static_cast<const internal::composed_filter*>(_context);

    return composed->run(_value,
                         _out);
}

// internal::failed_filter
//   Refuses every value as memory refused the copy.
inline int
internal::failed_filter(
    ::d_pack_text   _value,
    ::d_arg_buffer* _out,
    void*           _context
)
{
    (void)_value;
    (void)_out;
    (void)_context;

    return D_ARG_ERROR_OUT_OF_MEMORY;
}

// internal::compare_type
//   The type's descriptor for a built-in type that is not text and, from
// C++11, for an enumeration; NULL otherwise.
template<typename Type>
inline const ::d_arg_type*
internal::compare_type<Type>::descriptor() D_NOEXCEPT
{
    const ::d_arg_type* const own = type_traits<Type>::descriptor();
    const bool                plain =
        ( (internal::builtin_of<Type>::value >= 0) &&
          ((own->flags & D_ARG_TYPE_FLAG_STRING) == 0u) );

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    return ( (plain) ||
             (std::is_enum<Type>::value) )
           ? own
           : D_NULLPTR;
#else
    return plain ? own : D_NULLPTR;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
}

// internal::member_of::get
//   The element, or the pair's first.
template<typename Element>
inline const Element&
internal::member_of<Element>::get(
    const Element& _element
) D_NOEXCEPT
{
    return _element;
}

template<typename First,
         typename Second>
inline const First&
internal::member_of<std::pair<First, Second> >::get(
    const std::pair<First, Second>& _element
) D_NOEXCEPT
{
    return _element.first;
}

// internal::member_texts / mapping_texts
//   Strings kept whole, the C pointers taken once all are written, since a
// string's buffer may move while the vector grows.
inline
internal::member_texts::member_texts()
    : m_texts(),
      m_items(),
      m_status()
{}

template<typename Element>
inline void
internal::member_texts::add(
    const Element& _element
)
{
    std::string       member;
    const cli::status written = to_text(_element,
                                        member);

    // the first failure is kept
    if ( (m_status.ok()) &&
         (!written.ok()) )
    {
        m_status = written;
    }

    m_texts.push_back(member);
    m_items.clear();

    return;
}

inline const char* const*
internal::member_texts::items()
{
    m_items.clear();

    // the pointers taken once every string is in its place
    for (std::size_t index = 0u;
         index < m_texts.size();
         ++index)
    {
        m_items.push_back(m_texts[index].c_str());
    }

    return m_items.empty() ? D_NULLPTR : &m_items[0];
}

inline std::size_t
internal::member_texts::count() const D_NOEXCEPT
{
    return m_texts.size();
}

inline cli::status
internal::member_texts::get_status() const D_NOEXCEPT
{
    return m_status;
}

inline
internal::mapping_texts::mapping_texts()
    : m_texts(),
      m_items(),
      m_status()
{}

template<typename Key,
         typename Value>
inline void
internal::mapping_texts::add(
    const Key&   _key,
    const Value& _value
)
{
    std::string from;
    std::string to;

    const cli::status written_from = to_text(_key,
                                             from);
    const cli::status written_to   = to_text(_value,
                                             to);

    // the first failure is kept
    if ( (m_status.ok()) &&
         (!written_from.ok()) )
    {
        m_status = written_from;
    }

    if ( (m_status.ok()) &&
         (!written_to.ok()) )
    {
        m_status = written_to;
    }

    m_texts.push_back(from);
    m_texts.push_back(to);
    m_items.clear();

    return;
}

inline const ::d_arg_mapping*
internal::mapping_texts::items()
{
    m_items.clear();

    // the pairs taken once every string is in its place
    for (std::size_t index = 0u;
         (index + 1u) < m_texts.size();
         index += 2u)
    {
        const ::d_arg_mapping pair = { m_texts[index].c_str(),
                                       m_texts[index + 1u].c_str() };

        m_items.push_back(pair);
    }

    return m_items.empty() ? D_NULLPTR : &m_items[0];
}

inline std::size_t
internal::mapping_texts::count() const D_NOEXCEPT
{
    return (m_texts.size() / 2u);
}

inline cli::status
internal::mapping_texts::get_status() const D_NOEXCEPT
{
    return m_status;
}

// internal::type_check
//   convert() into an object of the type's own.
template<typename Type>
inline std::string
internal::type_check<Type>::operator()(
    std::string& _value
) const
{
    Type converted = Type();

    return cli::convert(text(_value),
                        converted).ok()
           ? std::string()
           : ( std::string("Failed parsing ") + _value + " as a " +
               type_name<Type>() );
}

// internal::mapping_validator
//   The pairs written, then the C transformer, checked or not, over them and
// the key type's descriptor to compare as.
template<typename Mapping>
inline validator
internal::mapping_validator(
    const Mapping& _pairs,
    const filter&  _filter,
    bool           _checked
)
{
    typedef typename Mapping::value_type                     pair_type;
    typedef typename remove_const<
        typename pair_type::first_type>::type                key_type;
    typedef typename Mapping::const_iterator                 iterator;

    internal::mapping_texts pairs;

    // each key and value as the text its type writes
    for (iterator pair = _pairs.begin();
         pair != _pairs.end();
         ++pair)
    {
        pairs.add(pair->first,
                  pair->second);
    }

    // a pair that could not be written fails the validator
    if (!pairs.get_status().ok())
    {
        return internal::failed_validator(pairs.get_status());
    }

    const ::d_arg_type* const as = compare_type<key_type>::descriptor();

    return validator(_checked
                     ? d_arg_validator_checked_transformer(as,
                                                           pairs.items(),
                                                           pairs.count(),
                                                           &_filter)
                     : d_arg_validator_transformer(as,
                                                   pairs.items(),
                                                   pairs.count(),
                                                   &_filter));
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// internal::describe_validator / run_validator
//   The description into a scratch buffer; a validation of the value, run,
// read back -- the value rewritten when it changed, and the reason, or, for
// an error with none recorded, the status's text -- and released.
inline std::string
internal::describe_validator(
    const ::d_arg_validator* _validator
)
{
    internal::scratch described;

    return (d_arg_validator_describe(_validator,
                                     described.get()) == D_ARG_OK)
           ? described.str()
           : std::string();
}

NS_INTERNAL

    // validation_scope
    //   class: a d_arg_validation released when it goes, so that a string
    //   throwing while its outcome is copied out leaks nothing.
    class validation_scope
    {
    public:
        explicit validation_scope(const text& _value) D_NOEXCEPT;
        ~validation_scope();

        ::d_arg_validation* get() D_NOEXCEPT;

    private:
        D_DELETED_FN(validation_scope(const validation_scope& _other))
        D_DELETED_FN(validation_scope& operator=(
            const validation_scope& _other))

        ::d_arg_validation m_validation;
    };

    inline
    validation_scope::validation_scope(
        const text& _value
    ) D_NOEXCEPT
    {
        d_arg_validation_init(&m_validation,
                              _value);
    }

    inline
    validation_scope::~validation_scope()
    {
        d_arg_validation_release(&m_validation);
    }

    inline ::d_arg_validation*
    validation_scope::get() D_NOEXCEPT
    {
        return &m_validation;
    }

NS_END  // internal

inline std::string
internal::run_validator(
    const ::d_arg_validator* _validator,
    std::string&             _value
)
{
    internal::validation_scope scope(_value);

    const int  run    = d_arg_validator_run(_validator,
                                            scope.get());
    const text value  = scope.get()->value;
    const text reason = d_arg_buffer_text(&scope.get()->message);

    std::string result = ( (run != D_ARG_OK) &&
                           (reason.empty()) )
                         ? std::string(d_arg_status_name(run))
                         : reason.str();

    // a rewritten value replaces the caller's
    if (value != text(_value))
    {
        _value.assign(value.begin(),
                      value.end());
    }

    return result;
}

// internal::name_enumeration
//   The description worked out, its first word -- the name of the type --
// replaced.
inline void
internal::name_enumeration(
    validator& _validator
)
{
    const std::string            described = _validator.get_description();
    const std::string::size_type named     = described.find(' ');

    // the type's name ends at the first space
    if (named != std::string::npos)
    {
        _validator.description(std::string("ENUM") +
                               described.substr(named));
    }

    return;
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// internal::failed_validator
//   A copy of no validator, the C library's failed validator with
// D_ARG_ERROR_BAD_ARGUMENT, which owns nothing; its status replaced with the
// failure's, which changes no ownership.
inline validator
internal::failed_validator(
    const cli::status& _status
)
{
    validator failed(d_arg_validator_copy(D_NULLPTR));

    failed.d_arg_validator::status = _status.code();

    return failed;
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_VALIDATOR_HPP
