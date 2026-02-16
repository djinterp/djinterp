/******************************************************************************
* djinterp [parse]                                                     parse.h
*
*   Comprehensive parsing framework foundation providing type-safe, flexible
* infrastructure for building parsers and parser generators. This module uses
* SFINAE-based type traits for compile-time polymorphism, avoiding virtual
* dispatch overhead while maintaining extensibility.
*
*   The framework supports arbitrary input sources (files, streams, memory,
* databases, network) and output targets (ASTs, containers, callbacks). All
* functionality adapts to the detected C++ standard via env.h.
*
* Features:
*   - Zero-overhead abstractions via SFINAE and type traits
*   - Configurable input/output adapters
*   - Rich error handling with source location tracking
*   - Composable parser combinators (when C++14+)
*   - Lookahead and backtracking support
*   - Memory-efficient parsing with minimal allocations
*
* 
* path:      \inc\parse\parse.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.11
******************************************************************************/

#ifndef DJINTERP_PARSE_
#define DJINTERP_PARSE_ 1

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include ".\djinterp.h"
#include ".\env.h"



// standard library includes conditional on C++ version
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <tuple>
    #include <memory>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <optional>
    #include <string_view>
    #include <variant>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include <concepts>
    #include <span>
    #include <ranges>
#endif


// =============================================================================
// 0.   CONFIGURATION
// =============================================================================

// D_CFG_PARSE_DEFAULT_BUFFER_SIZE
//   configuration: default buffer size for buffered input sources.
#ifndef D_CFG_PARSE_DEFAULT_BUFFER_SIZE
    #define D_CFG_PARSE_DEFAULT_BUFFER_SIZE 4096
#endif

// D_CFG_PARSE_MAX_LOOKAHEAD
//   configuration: maximum lookahead depth for predictive parsing.
#ifndef D_CFG_PARSE_MAX_LOOKAHEAD
    #define D_CFG_PARSE_MAX_LOOKAHEAD 64
#endif

// D_CFG_PARSE_ENABLE_SOURCE_TRACKING
//   configuration: enable line/column tracking (slight performance cost).
#ifndef D_CFG_PARSE_ENABLE_SOURCE_TRACKING
    #define D_CFG_PARSE_ENABLE_SOURCE_TRACKING 1
#endif

// D_CFG_PARSE_ENABLE_ERROR_RECOVERY
//   configuration: enable automatic error recovery mechanisms.
#ifndef D_CFG_PARSE_ENABLE_ERROR_RECOVERY
    #define D_CFG_PARSE_ENABLE_ERROR_RECOVERY 1
#endif


NS_DJINTERP
NS_INTERNAL

// =============================================================================
// I.   DETECTION TRAITS (SFINAE HELPERS)
// =============================================================================

// -----------------------------------------------------------------------------
// I.1  void_t implementation for pre-C++17
// -----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename... _Types>
    using void_t = std::void_t<_Types...>;
#else
    template<typename...>
    struct make_void
    {
        using type = void;
    };

    template<typename... _Types>
    using void_t = typename make_void<_Types...>::type;
#endif

// -----------------------------------------------------------------------------
// I.2  Detector idiom implementation
// -----------------------------------------------------------------------------

// nonesuch
//   type: represents detection failure (non-constructible sentinel type).
struct nonesuch
{
    nonesuch()                         = delete;
    ~nonesuch()                        = delete;
    nonesuch(const nonesuch&)          = delete;
    void operator=(const nonesuch&)    = delete;
};

// detector_base
//   type trait (internal): base case for detector (detection failure).
template<typename    _Default,
         typename    _AlwaysVoid,
         template<typename...> class _Op,
         typename... _Args>
struct detector_base
{
    using value_t = std::false_type;
    using type    = _Default;
};

// detector_base (specialization)
//   type trait (internal): detection success case.
template<typename    _Default,
         template<typename...> class _Op,
         typename... _Args>
struct detector_base<_Default, void_t<_Op<_Args...>>, _Op, _Args...>
{
    using value_t = std::true_type;
    using type    = _Op<_Args...>;
};

// is_detected
//   type trait: detects if _Op<_Args...> is well-formed.
template<template<typename...> class _Op,
         typename... _Args>
using is_detected = typename detector_base<nonesuch, void, _Op, _Args...>::value_t;

// detected_t
//   type alias: returns _Op<_Args...> if valid, nonesuch otherwise.
template<template<typename...> class _Op,
         typename... _Args>
using detected_t = typename detector_base<nonesuch, void, _Op, _Args...>::type;

// detected_or
//   type trait: returns _Op<_Args...> if valid, _Default otherwise.
template<typename    _Default,
         template<typename...> class _Op,
         typename... _Args>
using detected_or = detector_base<_Default, void, _Op, _Args...>;

// detected_or_t
//   type alias: convenience alias for detected_or::type.
template<typename    _Default,
         template<typename...> class _Op,
         typename... _Args>
using detected_or_t = typename detected_or<_Default, _Op, _Args...>::type;

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<template<typename...> class _Op,
             typename... _Args>
    constexpr bool is_detected_v = is_detected<_Op, _Args...>::value;
#endif

// -----------------------------------------------------------------------------
// I.3  Member detection helpers
// -----------------------------------------------------------------------------

// has_value_type_impl
//   type trait (internal): detects T::value_type.
template<typename _T>
using has_value_type_impl = typename _T::value_type;

// has_iterator_impl
//   type trait (internal): detects T::iterator.
template<typename _T>
using has_iterator_impl = typename _T::iterator;

// has_size_type_impl
//   type trait (internal): detects T::size_type.
template<typename _T>
using has_size_type_impl = typename _T::size_type;

// has_difference_type_impl
//   type trait (internal): detects T::difference_type.
template<typename _T>
using has_difference_type_impl = typename _T::difference_type;

// has_reference_impl
//   type trait (internal): detects T::reference.
template<typename _T>
using has_reference_impl = typename _T::reference;

// has_const_reference_impl
//   type trait (internal): detects T::const_reference.
template<typename _T>
using has_const_reference_impl = typename _T::const_reference;

// has_pointer_impl
//   type trait (internal): detects T::pointer.
template<typename _T>
using has_pointer_impl = typename _T::pointer;

NS_END  // internal


// =============================================================================
// II.  CORE TYPE TRAITS
// =============================================================================

// -----------------------------------------------------------------------------
// II.1  Input source detection traits
// -----------------------------------------------------------------------------

NS_INTERNAL

// read_expr
//   type trait (internal): expression for reading from a source.
template<typename _Source>
using read_expr = decltype(std::declval<_Source&>().read());

// peek_expr
//   type trait (internal): expression for peeking at a source.
template<typename _Source>
using peek_expr = decltype(std::declval<_Source&>().peek());

// advance_expr
//   type trait (internal): expression for advancing a source.
template<typename _Source>
using advance_expr = decltype(std::declval<_Source&>().advance());

// eof_expr
//   type trait (internal): expression for checking end-of-input.
template<typename _Source>
using eof_expr = decltype(std::declval<const _Source&>().eof());

// position_expr
//   type trait (internal): expression for getting current position.
template<typename _Source>
using position_expr = decltype(std::declval<const _Source&>().position());

// mark_expr
//   type trait (internal): expression for marking a position.
template<typename _Source>
using mark_expr = decltype(std::declval<_Source&>().mark());

// restore_expr
//   type trait (internal): expression for restoring to a marked position.
template<typename _Source>
using restore_expr = decltype(std::declval<_Source&>().restore(
                                  std::declval<typename _Source::mark_type>()));

// commit_expr
//   type trait (internal): expression for committing after successful parse.
template<typename _Source>
using commit_expr = decltype(std::declval<_Source&>().commit(
                                 std::declval<typename _Source::mark_type>()));

NS_END  // internal

// has_read
//   type trait: detects if _Source has a read() method.
template<typename _Source>
struct has_read : internal::is_detected<internal::read_expr, _Source>
{};

// has_peek
//   type trait: detects if _Source has a peek() method.
template<typename _Source>
struct has_peek : internal::is_detected<internal::peek_expr, _Source>
{};

// has_advance
//   type trait: detects if _Source has an advance() method.
template<typename _Source>
struct has_advance : internal::is_detected<internal::advance_expr, _Source>
{};

// has_eof
//   type trait: detects if _Source has an eof() method.
template<typename _Source>
struct has_eof : internal::is_detected<internal::eof_expr, _Source>
{};

// has_position
//   type trait: detects if _Source has a position() method.
template<typename _Source>
struct has_position : internal::is_detected<internal::position_expr, _Source>
{};

// has_backtrack_support
//   type trait: detects if _Source supports mark/restore for backtracking.
template<typename _Source>
struct has_backtrack_support
    : std::integral_constant<bool,
                             internal::is_detected<internal::mark_expr,
                                                   _Source>::value &&
                             internal::is_detected<internal::restore_expr,
                                                   _Source>::value>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<typename _Source>
    constexpr bool has_read_v              = has_read<_Source>::value;

    template<typename _Source>
    constexpr bool has_peek_v              = has_peek<_Source>::value;

    template<typename _Source>
    constexpr bool has_advance_v           = has_advance<_Source>::value;

    template<typename _Source>
    constexpr bool has_eof_v               = has_eof<_Source>::value;

    template<typename _Source>
    constexpr bool has_position_v          = has_position<_Source>::value;

    template<typename _Source>
    constexpr bool has_backtrack_support_v = has_backtrack_support<_Source>::value;
#endif

// -----------------------------------------------------------------------------
// II.2  Output sink detection traits
// -----------------------------------------------------------------------------

NS_INTERNAL

// emit_expr
//   type trait (internal): expression for emitting to a sink.
template<typename _Sink,
         typename _Value>
using emit_expr = decltype(std::declval<_Sink&>().emit(std::declval<_Value>()));

// push_back_expr
//   type trait (internal): expression for push_back on containers.
template<typename _Container,
         typename _Value>
using push_back_expr = decltype(std::declval<_Container&>().push_back(
                                    std::declval<_Value>()));

// insert_expr
//   type trait (internal): expression for insert on containers.
template<typename _Container,
         typename _Value>
using insert_expr = decltype(std::declval<_Container&>().insert(
                                 std::declval<_Value>()));

// callable_expr
//   type trait (internal): expression for callable invocation.
template<typename _Callable,
         typename... _Args>
using callable_expr = decltype(std::declval<_Callable&>()(
                                   std::declval<_Args>()...));

NS_END  // internal

// has_emit
//   type trait: detects if _Sink has an emit(_Value) method.
template<typename _Sink,
         typename _Value>
struct has_emit : internal::is_detected<internal::emit_expr, _Sink, _Value>
{};

// has_push_back
//   type trait: detects if _Container has push_back(_Value).
template<typename _Container,
         typename _Value>
struct has_push_back : internal::is_detected<internal::push_back_expr,
                                             _Container,
                                             _Value>
{};

// has_insert
//   type trait: detects if _Container has insert(_Value).
template<typename _Container,
         typename _Value>
struct has_insert : internal::is_detected<internal::insert_expr,
                                          _Container,
                                          _Value>
{};

// is_callable
//   type trait: detects if _Callable can be called with _Args.
template<typename _Callable,
         typename... _Args>
struct is_callable : internal::is_detected<internal::callable_expr,
                                           _Callable,
                                           _Args...>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<typename _Sink,
             typename _Value>
    constexpr bool has_emit_v = has_emit<_Sink, _Value>::value;

    template<typename _Container,
             typename _Value>
    constexpr bool has_push_back_v = has_push_back<_Container, _Value>::value;

    template<typename _Container,
             typename _Value>
    constexpr bool has_insert_v = has_insert<_Container, _Value>::value;

    template<typename _Callable,
             typename... _Args>
    constexpr bool is_callable_v = is_callable<_Callable, _Args...>::value;
#endif

// -----------------------------------------------------------------------------
// II.3  Parser detection traits
// -----------------------------------------------------------------------------

NS_INTERNAL

// parse_expr
//   type trait (internal): expression for parse method.
template<typename _Parser,
         typename _Source>
using parse_expr = decltype(std::declval<_Parser&>().parse(
                                std::declval<_Source&>()));

// result_type_expr
//   type trait (internal): extracts parser result type.
template<typename _Parser>
using result_type_expr = typename _Parser::result_type;

// input_type_expr
//   type trait (internal): extracts parser input type.
template<typename _Parser>
using input_type_expr = typename _Parser::input_type;

NS_END  // internal

// has_parse_method
//   type trait: detects if _Parser has parse(_Source&) method.
template<typename _Parser,
         typename _Source>
struct has_parse_method : internal::is_detected<internal::parse_expr,
                                                _Parser,
                                                _Source>
{};

// has_result_type
//   type trait: detects if _Parser defines result_type.
template<typename _Parser>
struct has_result_type : internal::is_detected<internal::result_type_expr,
                                               _Parser>
{};

// has_input_type
//   type trait: detects if _Parser defines input_type.
template<typename _Parser>
struct has_input_type : internal::is_detected<internal::input_type_expr,
                                              _Parser>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<typename _Parser,
             typename _Source>
    constexpr bool has_parse_method_v = has_parse_method<_Parser, _Source>::value;

    template<typename _Parser>
    constexpr bool has_result_type_v = has_result_type<_Parser>::value;

    template<typename _Parser>
    constexpr bool has_input_type_v = has_input_type<_Parser>::value;
#endif

// -----------------------------------------------------------------------------
// II.4  Iterator category detection
// -----------------------------------------------------------------------------

NS_INTERNAL

// iterator_category_impl
//   type trait (internal): extracts iterator category.
template<typename _Iterator>
using iterator_category_impl = typename std::iterator_traits<_Iterator>::iterator_category;

NS_END  // internal

// is_input_iterator
//   type trait: detects if _Iterator is at least an input iterator.
template<typename _Iterator,
         typename = void>
struct is_input_iterator : std::false_type
{};

template<typename _Iterator>
struct is_input_iterator<_Iterator,
                         internal::void_t<internal::iterator_category_impl<_Iterator>>>
    : std::is_base_of<std::input_iterator_tag,
                      typename std::iterator_traits<_Iterator>::iterator_category>
{};

// is_forward_iterator
//   type trait: detects if _Iterator is at least a forward iterator.
template<typename _Iterator,
         typename = void>
struct is_forward_iterator : std::false_type
{};

template<typename _Iterator>
struct is_forward_iterator<_Iterator,
                           internal::void_t<internal::iterator_category_impl<_Iterator>>>
    : std::is_base_of<std::forward_iterator_tag,
                      typename std::iterator_traits<_Iterator>::iterator_category>
{};

// is_random_access_iterator
//   type trait: detects if _Iterator is a random access iterator.
template<typename _Iterator,
         typename = void>
struct is_random_access_iterator : std::false_type
{};

template<typename _Iterator>
struct is_random_access_iterator<_Iterator,
                                 internal::void_t<internal::iterator_category_impl<_Iterator>>>
    : std::is_base_of<std::random_access_iterator_tag,
                      typename std::iterator_traits<_Iterator>::iterator_category>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<typename _Iterator>
    constexpr bool is_input_iterator_v = is_input_iterator<_Iterator>::value;

    template<typename _Iterator>
    constexpr bool is_forward_iterator_v = is_forward_iterator<_Iterator>::value;

    template<typename _Iterator>
    constexpr bool is_random_access_iterator_v = 
        is_random_access_iterator<_Iterator>::value;
#endif


// =============================================================================
// III. SOURCE LOCATION
// =============================================================================

// d_source_location
//   struct: represents a position in the input source.
// Tracks line, column, and byte offset for precise error reporting.
struct d_source_location
{
    using size_type = std::size_t;

    size_type line;
    size_type column;
    size_type offset;

    // d_source_location (default constructor)
    //   constructor: initializes to beginning of input (1:1:0).
    d_source_location()
        : line(1)
        , column(1)
        , offset(0)
    {}

    // d_source_location (parameterized constructor)
    //   constructor: initializes with specified position.
    d_source_location(size_type _line,
                      size_type _column,
                      size_type _offset)
        : line(_line)
        , column(_column)
        , offset(_offset)
    {}

    // advance_char
    //   updates position after consuming a character.
    void
    advance_char
    (
        char _ch
    )
    {
        ++offset;

        // handle newlines
        if (_ch == '\n')
        {
            ++line;
            column = 1;
        }
        else if (_ch == '\r')
        {
            // carriage return alone advances line
            ++line;
            column = 1;
        }
        else if (_ch == '\t')
        {
            // tab advances to next 8-space boundary
            column = ((column - 1) / 8 + 1) * 8 + 1;
        }
        else
        {
            ++column;
        }

        return;
    }

    // advance_bytes
    //   updates offset by byte count (for binary data).
    void
    advance_bytes
    (
        size_type _count
    )
    {
        offset += _count;

        return;
    }
};

// operator==
//   compares two source locations for equality.
inline bool
operator==
(
    const d_source_location& _lhs,
    const d_source_location& _rhs
)
{
    return ( (_lhs.line   == _rhs.line)   &&
             (_lhs.column == _rhs.column) &&
             (_lhs.offset == _rhs.offset) );
}

// operator!=
//   compares two source locations for inequality.
inline bool
operator!=
(
    const d_source_location& _lhs,
    const d_source_location& _rhs
)
{
    return !(_lhs == _rhs);
}

// operator<
//   compares two source locations by offset.
inline bool
operator<
(
    const d_source_location& _lhs,
    const d_source_location& _rhs
)
{
    return _lhs.offset < _rhs.offset;
}


// =============================================================================
// IV.  PARSE RESULT
// =============================================================================

// d_parse_status
//   enum: represents the outcome of a parse operation.
enum class d_parse_status : std::uint8_t
{
    success         = 0,    // parse succeeded
    failure         = 1,    // parse failed (recoverable)
    error           = 2,    // parse error (potentially unrecoverable)
    incomplete      = 3,    // need more input
    end_of_input    = 4     // reached end of input
};

// d_parse_result
//   class: represents the result of a parse operation.
// Contains either a successfully parsed value or error information.
template<typename _ValueType>
class d_parse_result
{
public:
    using value_type    = _ValueType;
    using size_type     = std::size_t;

private:
    // storage for value (using aligned storage for delayed construction)
    typename std::aligned_storage<sizeof(value_type),
                                  alignof(value_type)>::type m_storage;
    d_parse_status   m_status;
    d_source_location m_location;
    const char*      m_error_message;
    bool             m_has_value;

public:
    // d_parse_result (default constructor)
    //   constructor: creates a failed result with no value.
    d_parse_result()
        : m_status(d_parse_status::failure)
        , m_location()
        , m_error_message(nullptr)
        , m_has_value(false)
    {}

    // d_parse_result (success constructor)
    //   constructor: creates a successful result with value.
    explicit d_parse_result(const value_type& _value)
        : m_status(d_parse_status::success)
        , m_location()
        , m_error_message(nullptr)
        , m_has_value(true)
    {
        new (&m_storage) value_type(_value);
    }

    // d_parse_result (move constructor for value)
    //   constructor: creates a successful result with moved value.
    explicit d_parse_result(value_type&& _value)
        : m_status(d_parse_status::success)
        , m_location()
        , m_error_message(nullptr)
        , m_has_value(true)
    {
        new (&m_storage) value_type(std::move(_value));
    }

    // d_parse_result (error constructor)
    //   constructor: creates an error result with message and location.
    d_parse_result(d_parse_status         _status,
                   const char*            _error_message,
                   const d_source_location& _location = d_source_location())
        : m_status(_status)
        , m_location(_location)
        , m_error_message(_error_message)
        , m_has_value(false)
    {}

    // d_parse_result (copy constructor)
    //   constructor: copies another result.
    d_parse_result(const d_parse_result& _other)
        : m_status(_other.m_status)
        , m_location(_other.m_location)
        , m_error_message(_other.m_error_message)
        , m_has_value(_other.m_has_value)
    {
        if (m_has_value)
        {
            new (&m_storage) value_type(_other.value());
        }
    }

    // d_parse_result (move constructor)
    //   constructor: moves another result.
    d_parse_result(d_parse_result&& _other)
        : m_status(_other.m_status)
        , m_location(_other.m_location)
        , m_error_message(_other.m_error_message)
        , m_has_value(_other.m_has_value)
    {
        if (m_has_value)
        {
            new (&m_storage) value_type(std::move(_other.value()));
            _other.m_has_value = false;
        }
    }

    // ~d_parse_result
    //   destructor: destroys contained value if present.
    ~d_parse_result()
    {
        if (m_has_value)
        {
            reinterpret_cast<value_type*>(&m_storage)->~value_type();
        }
    }

    // operator=
    //   assignment: copy assignment.
    d_parse_result&
    operator=
    (
        const d_parse_result& _other
    )
    {
        if (this != &_other)
        {
            // destroy existing value
            if (m_has_value)
            {
                reinterpret_cast<value_type*>(&m_storage)->~value_type();
            }

            m_status        = _other.m_status;
            m_location      = _other.m_location;
            m_error_message = _other.m_error_message;
            m_has_value     = _other.m_has_value;

            // copy new value
            if (m_has_value)
            {
                new (&m_storage) value_type(_other.value());
            }
        }

        return *this;
    }

    // operator=
    //   assignment: move assignment.
    d_parse_result&
    operator=
    (
        d_parse_result&& _other
    )
    {
        if (this != &_other)
        {
            // destroy existing value
            if (m_has_value)
            {
                reinterpret_cast<value_type*>(&m_storage)->~value_type();
            }

            m_status        = _other.m_status;
            m_location      = _other.m_location;
            m_error_message = _other.m_error_message;
            m_has_value     = _other.m_has_value;

            // move new value
            if (m_has_value)
            {
                new (&m_storage) value_type(std::move(_other.value()));
                _other.m_has_value = false;
            }
        }

        return *this;
    }

    // success
    //   returns true if parse succeeded.
    bool
    success() const
    {
        return m_status == d_parse_status::success;
    }

    // failed
    //   returns true if parse failed.
    bool
    failed() const
    {
        return m_status != d_parse_status::success;
    }

    // operator bool
    //   conversion: returns true if parse succeeded.
    explicit
    operator bool() const
    {
        return success();
    }

    // status
    //   returns the parse status.
    d_parse_status
    status() const
    {
        return m_status;
    }

    // has_value
    //   returns true if result contains a value.
    bool
    has_value() const
    {
        return m_has_value;
    }

    // value
    //   returns reference to the parsed value.
    // Precondition: has_value() must be true.
    value_type&
    value()
    {
        return *reinterpret_cast<value_type*>(&m_storage);
    }

    // value (const)
    //   returns const reference to the parsed value.
    const value_type&
    value() const
    {
        return *reinterpret_cast<const value_type*>(&m_storage);
    }

    // value_or
    //   returns the value if present, otherwise the default.
    template<typename _Default>
    value_type
    value_or
    (
        _Default&& _default
    ) const
    {
        if (m_has_value)
        {
            return value();
        }

        return static_cast<value_type>(std::forward<_Default>(_default));
    }

    // location
    //   returns the source location.
    const d_source_location&
    location() const
    {
        return m_location;
    }

    // set_location
    //   sets the source location.
    void
    set_location
    (
        const d_source_location& _location
    )
    {
        m_location = _location;

        return;
    }

    // error_message
    //   returns the error message if any.
    const char*
    error_message() const
    {
        return m_error_message;
    }

    // make_success
    //   factory: creates a successful result.
    static d_parse_result
    make_success
    (
        const value_type& _value
    )
    {
        return d_parse_result(_value);
    }

    // make_success (move)
    //   factory: creates a successful result with moved value.
    static d_parse_result
    make_success
    (
        value_type&& _value
    )
    {
        return d_parse_result(std::move(_value));
    }

    // make_failure
    //   factory: creates a failure result.
    static d_parse_result
    make_failure
    (
        const char*            _message  = nullptr,
        const d_source_location& _location = d_source_location()
    )
    {
        return d_parse_result(d_parse_status::failure, _message, _location);
    }

    // make_error
    //   factory: creates an error result.
    static d_parse_result
    make_error
    (
        const char*            _message,
        const d_source_location& _location = d_source_location()
    )
    {
        return d_parse_result(d_parse_status::error, _message, _location);
    }

    // make_incomplete
    //   factory: creates an incomplete result.
    static d_parse_result
    make_incomplete
    (
        const d_source_location& _location = d_source_location()
    )
    {
        return d_parse_result(d_parse_status::incomplete, nullptr, _location);
    }

    // make_end_of_input
    //   factory: creates an end-of-input result.
    static d_parse_result
    make_end_of_input
    (
        const d_source_location& _location = d_source_location()
    )
    {
        return d_parse_result(d_parse_status::end_of_input, nullptr, _location);
    }
};

// d_parse_result<void>
//   class (specialization): result type for parsers that don't produce values.
template<>
class d_parse_result<void>
{
public:
    using value_type = void;
    using size_type  = std::size_t;

private:
    d_parse_status   m_status;
    d_source_location m_location;
    const char*      m_error_message;

public:
    // d_parse_result (default constructor)
    //   constructor: creates a successful void result.
    d_parse_result()
        : m_status(d_parse_status::success)
        , m_location()
        , m_error_message(nullptr)
    {}

    // d_parse_result (status constructor)
    //   constructor: creates a result with specified status.
    explicit d_parse_result(d_parse_status _status)
        : m_status(_status)
        , m_location()
        , m_error_message(nullptr)
    {}

    // d_parse_result (error constructor)
    //   constructor: creates an error result.
    d_parse_result(d_parse_status         _status,
                   const char*            _error_message,
                   const d_source_location& _location = d_source_location())
        : m_status(_status)
        , m_location(_location)
        , m_error_message(_error_message)
    {}

    // success
    //   returns true if parse succeeded.
    bool
    success() const
    {
        return m_status == d_parse_status::success;
    }

    // failed
    //   returns true if parse failed.
    bool
    failed() const
    {
        return m_status != d_parse_status::success;
    }

    // operator bool
    //   conversion: returns true if parse succeeded.
    explicit
    operator bool() const
    {
        return success();
    }

    // status
    //   returns the parse status.
    d_parse_status
    status() const
    {
        return m_status;
    }

    // has_value
    //   always returns false for void result.
    bool
    has_value() const
    {
        return false;
    }

    // location
    //   returns the source location.
    const d_source_location&
    location() const
    {
        return m_location;
    }

    // set_location
    //   sets the source location.
    void
    set_location
    (
        const d_source_location& _location
    )
    {
        m_location = _location;

        return;
    }

    // error_message
    //   returns the error message if any.
    const char*
    error_message() const
    {
        return m_error_message;
    }

    // make_success
    //   factory: creates a successful result.
    static d_parse_result
    make_success()
    {
        return d_parse_result(d_parse_status::success);
    }

    // make_failure
    //   factory: creates a failure result.
    static d_parse_result
    make_failure
    (
        const char*            _message  = nullptr,
        const d_source_location& _location = d_source_location()
    )
    {
        return d_parse_result(d_parse_status::failure, _message, _location);
    }

    // make_error
    //   factory: creates an error result.
    static d_parse_result
    make_error
    (
        const char*            _message,
        const d_source_location& _location = d_source_location()
    )
    {
        return d_parse_result(d_parse_status::error, _message, _location);
    }
};


// =============================================================================
// V.   INPUT SOURCE ADAPTERS
// =============================================================================

// -----------------------------------------------------------------------------
// V.1  Iterator-based input source
// -----------------------------------------------------------------------------

// d_iterator_source
//   class: wraps an iterator range as a parse input source.
template<typename _Iterator>
class d_iterator_source
{
public:
    using iterator_type   = _Iterator;
    using value_type      = typename std::iterator_traits<_Iterator>::value_type;
    using difference_type = typename std::iterator_traits<_Iterator>::difference_type;
    using size_type       = std::size_t;

    // mark_type
    //   type: represents a marked position for backtracking.
    struct mark_type
    {
        iterator_type      iterator;
        d_source_location  location;
    };

private:
    iterator_type     m_current;
    iterator_type     m_end;
    d_source_location m_location;

public:
    // d_iterator_source
    //   constructor: creates source from iterator range.
    d_iterator_source(iterator_type _begin,
                      iterator_type _end)
        : m_current(_begin)
        , m_end(_end)
        , m_location()
    {}

    // eof
    //   returns true if at end of input.
    bool
    eof() const
    {
        return m_current == m_end;
    }

    // peek
    //   returns current element without advancing.
    value_type
    peek() const
    {
        // check for eof
        if (eof())
        {
            return value_type{};
        }

        return *m_current;
    }

    // peek_n
    //   returns element n positions ahead (0 = current).
    // Requires forward iterator or better.
    template<typename _It = iterator_type>
    typename std::enable_if<is_forward_iterator<_It>::value, value_type>::type
    peek_n
    (
        size_type _n
    ) const
    {
        iterator_type it;
        size_type     count;

        it    = m_current;
        count = 0;

        while ( (it != m_end) && (count < _n) )
        {
            ++it;
            ++count;
        }

        // check if we reached the target position
        if (it == m_end)
        {
            return value_type{};
        }

        return *it;
    }

    // read
    //   returns current element and advances.
    value_type
    read()
    {
        value_type result;

        // check for eof
        if (eof())
        {
            return value_type{};
        }

        result = *m_current;
        advance();

        return result;
    }

    // advance
    //   advances to next element.
    void
    advance()
    {
        // check for eof
        if (eof())
        {
            return;
        }

#if D_CFG_PARSE_ENABLE_SOURCE_TRACKING
        m_location.advance_char(static_cast<char>(*m_current));
#endif

        ++m_current;

        return;
    }

    // advance_n
    //   advances by n elements.
    void
    advance_n
    (
        size_type _n
    )
    {
        size_type i;

        for (i = 0; i < _n && !eof(); ++i)
        {
            advance();
        }

        return;
    }

    // position
    //   returns current source location.
    const d_source_location&
    position() const
    {
        return m_location;
    }

    // current
    //   returns current iterator.
    iterator_type
    current() const
    {
        return m_current;
    }

    // end
    //   returns end iterator.
    iterator_type
    end() const
    {
        return m_end;
    }

    // remaining
    //   returns count of remaining elements (for random access iterators).
    template<typename _It = iterator_type>
    typename std::enable_if<is_random_access_iterator<_It>::value, size_type>::type
    remaining() const
    {
        return static_cast<size_type>(m_end - m_current);
    }

    // mark
    //   creates a mark for later restoration.
    mark_type
    mark() const
    {
        mark_type result;

        result.iterator = m_current;
        result.location = m_location;

        return result;
    }

    // restore
    //   restores to a previously marked position.
    void
    restore
    (
        const mark_type& _mark
    )
    {
        m_current  = _mark.iterator;
        m_location = _mark.location;

        return;
    }

    // commit
    //   commits progress (no-op for iterator source, but required interface).
    void
    commit
    (
        const mark_type& _mark
    )
    {
        // nothing to do for simple iterator source
        (void)_mark;

        return;
    }
};

// make_iterator_source
//   factory: creates an iterator source from a range.
template<typename _Iterator>
d_iterator_source<_Iterator>
make_iterator_source
(
    _Iterator _begin,
    _Iterator _end
)
{
    return d_iterator_source<_Iterator>(_begin, _end);
}

// -----------------------------------------------------------------------------
// V.2  Pointer-based input source (optimized for contiguous memory)
// -----------------------------------------------------------------------------

// d_memory_source
//   class: optimized input source for contiguous memory regions.
template<typename _T = char>
class d_memory_source
{
public:
    using value_type      = _T;
    using pointer         = const _T*;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    // mark_type
    //   type: represents a marked position for backtracking.
    struct mark_type
    {
        pointer           ptr;
        d_source_location location;
    };

private:
    pointer           m_current;
    pointer           m_end;
    d_source_location m_location;

public:
    // d_memory_source
    //   constructor: creates source from pointer and size.
    d_memory_source(pointer   _data,
                    size_type _size)
        : m_current(_data)
        , m_end(_data + _size)
        , m_location()
    {}

    // d_memory_source
    //   constructor: creates source from pointer range.
    d_memory_source(pointer _begin,
                    pointer _end)
        : m_current(_begin)
        , m_end(_end)
        , m_location()
    {}

    // eof
    //   returns true if at end of input.
    bool
    eof() const
    {
        return m_current >= m_end;
    }

    // peek
    //   returns current element without advancing.
    value_type
    peek() const
    {
        // check for eof
        if (eof())
        {
            return value_type{};
        }

        return *m_current;
    }

    // peek_n
    //   returns element n positions ahead (0 = current).
    value_type
    peek_n
    (
        size_type _n
    ) const
    {
        // check if n is within bounds
        if (m_current + _n >= m_end)
        {
            return value_type{};
        }

        return m_current[_n];
    }

    // read
    //   returns current element and advances.
    value_type
    read()
    {
        value_type result;

        // check for eof
        if (eof())
        {
            return value_type{};
        }

        result = *m_current;
        advance();

        return result;
    }

    // advance
    //   advances to next element.
    void
    advance()
    {
        // check for eof
        if (eof())
        {
            return;
        }

#if D_CFG_PARSE_ENABLE_SOURCE_TRACKING
        m_location.advance_char(static_cast<char>(*m_current));
#endif

        ++m_current;

        return;
    }

    // advance_n
    //   advances by n elements.
    void
    advance_n
    (
        size_type _n
    )
    {
#if D_CFG_PARSE_ENABLE_SOURCE_TRACKING
        size_type i;
        for (i = 0; i < _n && !eof(); ++i)
        {
            advance();
        }
#else
        // optimize: direct pointer arithmetic when not tracking
        size_type remaining_amount;

        remaining_amount = remaining();
        if (_n > remaining_amount)
        {
            _n = remaining_amount;
        }

        m_current += _n;
#endif

        return;
    }

    // position
    //   returns current source location.
    const d_source_location&
    position() const
    {
        return m_location;
    }

    // data
    //   returns pointer to current position.
    pointer
    data() const
    {
        return m_current;
    }

    // remaining
    //   returns count of remaining elements.
    size_type
    remaining() const
    {
        return static_cast<size_type>(m_end - m_current);
    }

    // match_bytes
    //   attempts to match a sequence of bytes.
    bool
    match_bytes
    (
        pointer   _pattern,
        size_type _length
    )
    {
        size_type i;

        // check if enough data remains
        if (remaining() < _length)
        {
            return false;
        }

        // compare bytes
        for (i = 0; i < _length; ++i)
        {
            if (m_current[i] != _pattern[i])
            {
                return false;
            }
        }

        // advance past matched bytes
        advance_n(_length);

        return true;
    }

    // mark
    //   creates a mark for later restoration.
    mark_type
    mark() const
    {
        mark_type result;

        result.ptr      = m_current;
        result.location = m_location;

        return result;
    }

    // restore
    //   restores to a previously marked position.
    void
    restore
    (
        const mark_type& _mark
    )
    {
        m_current  = _mark.ptr;
        m_location = _mark.location;

        return;
    }

    // commit
    //   commits progress (no-op for memory source).
    void
    commit
    (
        const mark_type& _mark
    )
    {
        // nothing to do for simple memory source
        (void)_mark;

        return;
    }
};

// make_memory_source
//   factory: creates a memory source from pointer and size.
template<typename _T>
d_memory_source<_T>
make_memory_source
(
    const _T*   _data,
    std::size_t _size
)
{
    return d_memory_source<_T>(_data, _size);
}

// make_memory_source (C-string overload)
//   factory: creates a memory source from null-terminated string.
inline d_memory_source<char>
make_memory_source
(
    const char* _str
)
{
    std::size_t length;

    // calculate string length
    length = 0;
    while (_str[length] != '\0')
    {
        ++length;
    }

    return d_memory_source<char>(_str, length);
}

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
// make_memory_source (string_view overload)
//   factory: creates a memory source from string_view.
inline d_memory_source<char>
make_memory_source
(
    std::string_view _sv
)
{
    return d_memory_source<char>(_sv.data(), _sv.size());
}
#endif


// =============================================================================
// VI.  OUTPUT SINK ADAPTERS
// =============================================================================

// -----------------------------------------------------------------------------
// VI.1 Container sink
// -----------------------------------------------------------------------------

// d_container_sink
//   class: output sink that appends to a container.
template<typename _Container>
class d_container_sink
{
public:
    using container_type = _Container;
    using value_type     = typename _Container::value_type;

private:
    container_type* m_container;

public:
    // d_container_sink
    //   constructor: wraps a container reference.
    explicit d_container_sink(container_type& _container)
        : m_container(&_container)
    {}

    // emit
    //   adds a value to the container.
    template<typename _Value>
    typename std::enable_if<has_push_back<container_type, _Value>::value>::type
    emit
    (
        _Value&& _value
    )
    {
        m_container->push_back(std::forward<_Value>(_value));

        return;
    }

    // emit (insert fallback)
    //   adds a value using insert for containers without push_back.
    template<typename _Value>
    typename std::enable_if<!has_push_back<container_type, _Value>::value &&
                            has_insert<container_type, _Value>::value>::type
    emit
    (
        _Value&& _value
    )
    {
        m_container->insert(std::forward<_Value>(_value));

        return;
    }

    // container
    //   returns reference to the underlying container.
    container_type&
    container()
    {
        return *m_container;
    }

    // container (const)
    //   returns const reference to the underlying container.
    const container_type&
    container() const
    {
        return *m_container;
    }
};

// make_container_sink
//   factory: creates a container sink.
template<typename _Container>
d_container_sink<_Container>
make_container_sink
(
    _Container& _container
)
{
    return d_container_sink<_Container>(_container);
}

// -----------------------------------------------------------------------------
// VI.2 Callback sink
// -----------------------------------------------------------------------------

// d_callback_sink
//   class: output sink that invokes a callback for each value.
template<typename _Callback>
class d_callback_sink
{
public:
    using callback_type = _Callback;

private:
    callback_type m_callback;

public:
    // d_callback_sink
    //   constructor: wraps a callback.
    explicit d_callback_sink(callback_type _callback)
        : m_callback(std::move(_callback))
    {}

    // emit
    //   invokes the callback with the value.
    template<typename _Value>
    void
    emit
    (
        _Value&& _value
    )
    {
        m_callback(std::forward<_Value>(_value));

        return;
    }
};

// make_callback_sink
//   factory: creates a callback sink.
template<typename _Callback>
d_callback_sink<_Callback>
make_callback_sink
(
    _Callback&& _callback
)
{
    return d_callback_sink<typename std::decay<_Callback>::type>(
               std::forward<_Callback>(_callback));
}

// -----------------------------------------------------------------------------
// VI.3 Null sink (discards output)
// -----------------------------------------------------------------------------

// d_null_sink
//   class: output sink that discards all values.
struct d_null_sink
{
    // emit
    //   discards the value.
    template<typename _Value>
    void
    emit
    (
        _Value&&
    )
    {
        return;
    }
};


// =============================================================================
// VII. PARSER BASE AND CONCEPTS
// =============================================================================

// -----------------------------------------------------------------------------
// VII.1 Parser concept traits
// -----------------------------------------------------------------------------

// is_parser
//   type trait: detects if _T is a valid parser for _Source.
template<typename _T,
         typename _Source,
         typename = void>
struct is_parser : std::false_type
{};

template<typename _T,
         typename _Source>
struct is_parser<_T,
                 _Source,
                 internal::void_t<typename _T::result_type,
                                  decltype(std::declval<_T&>().parse(
                                               std::declval<_Source&>()))>>
    : std::true_type
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<typename _T,
             typename _Source>
    constexpr bool is_parser_v = is_parser<_T, _Source>::value;
#endif

// parser_result_t
//   type alias: extracts the result type of a parser.
template<typename _Parser>
using parser_result_t = typename _Parser::result_type;

// -----------------------------------------------------------------------------
// VII.2 Parser CRTP base
// -----------------------------------------------------------------------------

// d_parser_base
//   class: CRTP base class providing common parser functionality.
template<typename _Derived,
         typename _Result,
         typename _Input = char>
class d_parser_base
{
public:
    using derived_type = _Derived;
    using result_type  = _Result;
    using input_type   = _Input;
    using parse_result = d_parse_result<result_type>;

protected:
    // derived
    //   returns reference to the derived class.
    derived_type&
    derived()
    {
        return static_cast<derived_type&>(*this);
    }

    // derived (const)
    //   returns const reference to the derived class.
    const derived_type&
    derived() const
    {
        return static_cast<const derived_type&>(*this);
    }

public:
    // parse
    //   parses input from source; delegates to derived implementation.
    template<typename _Source>
    parse_result
    parse
    (
        _Source& _source
    )
    {
        return derived().parse_impl(_source);
    }

    // operator()
    //   function call operator for convenient parsing.
    template<typename _Source>
    parse_result
    operator()
    (
        _Source& _source
    )
    {
        return parse(_source);
    }

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // try_parse
    //   attempts to parse with automatic backtracking on failure.
    template<typename _Source>
    parse_result
    try_parse
    (
        _Source& _source
    )
    {
        typename _Source::mark_type mark;
        parse_result                result;

        mark   = _source.mark();
        result = parse(_source);

        // restore on failure
        if (result.failed())
        {
            _source.restore(mark);
        }
        else
        {
            _source.commit(mark);
        }

        return result;
    }
#endif
};


// =============================================================================
// VIII. FUNDAMENTAL PARSERS
// =============================================================================

// -----------------------------------------------------------------------------
// VIII.1 Single element parser
// -----------------------------------------------------------------------------

// d_any_parser
//   class: parser that accepts any single input element.
template<typename _Input = char>
class d_any_parser : public d_parser_base<d_any_parser<_Input>, _Input, _Input>
{
public:
    using base_type    = d_parser_base<d_any_parser<_Input>, _Input, _Input>;
    using result_type  = _Input;
    using input_type   = _Input;
    using parse_result = typename base_type::parse_result;

    // parse_impl
    //   implementation: reads any single element.
    template<typename _Source>
    parse_result
    parse_impl
    (
        _Source& _source
    )
    {
        // check for eof
        if (_source.eof())
        {
            return parse_result::make_end_of_input(_source.position());
        }

        return parse_result::make_success(_source.read());
    }
};

// any
//   factory: creates an any-element parser.
template<typename _Input = char>
d_any_parser<_Input>
any()
{
    return d_any_parser<_Input>();
}

// -----------------------------------------------------------------------------
// VIII.2 Literal parser
// -----------------------------------------------------------------------------

// d_literal_parser
//   class: parser that matches a specific value.
template<typename _Input = char>
class d_literal_parser : public d_parser_base<d_literal_parser<_Input>,
                                              _Input,
                                              _Input>
{
public:
    using base_type    = d_parser_base<d_literal_parser<_Input>, _Input, _Input>;
    using result_type  = _Input;
    using input_type   = _Input;
    using parse_result = typename base_type::parse_result;

private:
    input_type m_expected;

public:
    // d_literal_parser
    //   constructor: creates parser for specific value.
    explicit d_literal_parser(input_type _expected)
        : m_expected(_expected)
    {}

    // parse_impl
    //   implementation: matches the expected value.
    template<typename _Source>
    parse_result
    parse_impl
    (
        _Source& _source
    )
    {
        // check for eof
        if (_source.eof())
        {
            return parse_result::make_end_of_input(_source.position());
        }

        // check if current element matches
        if (_source.peek() != m_expected)
        {
            return parse_result::make_failure("literal mismatch",
                                              _source.position());
        }

        return parse_result::make_success(_source.read());
    }

    // expected
    //   returns the expected value.
    input_type
    expected() const
    {
        return m_expected;
    }
};

// literal
//   factory: creates a literal parser.
template<typename _Input>
d_literal_parser<_Input>
literal
(
    _Input _value
)
{
    return d_literal_parser<_Input>(_value);
}

// -----------------------------------------------------------------------------
// VIII.3 Predicate parser
// -----------------------------------------------------------------------------

// d_predicate_parser
//   class: parser that matches elements satisfying a predicate.
template<typename _Predicate,
         typename _Input = char>
class d_predicate_parser : public d_parser_base<d_predicate_parser<_Predicate, _Input>,
                                                _Input,
                                                _Input>
{
public:
    using base_type      = d_parser_base<d_predicate_parser<_Predicate, _Input>,
                                         _Input,
                                         _Input>;
    using predicate_type = _Predicate;
    using result_type    = _Input;
    using input_type     = _Input;
    using parse_result   = typename base_type::parse_result;

private:
    predicate_type m_predicate;

public:
    // d_predicate_parser
    //   constructor: creates parser with predicate.
    explicit d_predicate_parser(predicate_type _predicate)
        : m_predicate(std::move(_predicate))
    {}

    // parse_impl
    //   implementation: matches elements satisfying the predicate.
    template<typename _Source>
    parse_result
    parse_impl
    (
        _Source& _source
    )
    {
        input_type current;

        // check for eof
        if (_source.eof())
        {
            return parse_result::make_end_of_input(_source.position());
        }

        current = _source.peek();

        // check if predicate is satisfied
        if (!m_predicate(current))
        {
            return parse_result::make_failure("predicate not satisfied",
                                              _source.position());
        }

        _source.advance();

        return parse_result::make_success(current);
    }
};

// satisfy
//   factory: creates a predicate parser.
template<typename _Predicate,
         typename _Input = char>
d_predicate_parser<typename std::decay<_Predicate>::type, _Input>
satisfy
(
    _Predicate&& _predicate
)
{
    return d_predicate_parser<typename std::decay<_Predicate>::type, _Input>(
               std::forward<_Predicate>(_predicate));
}

// -----------------------------------------------------------------------------
// VIII.4 End-of-input parser
// -----------------------------------------------------------------------------

// d_eof_parser
//   class: parser that succeeds only at end of input.
template<typename _Input = char>
class d_eof_parser : public d_parser_base<d_eof_parser<_Input>, void, _Input>
{
public:
    using base_type    = d_parser_base<d_eof_parser<_Input>, void, _Input>;
    using result_type  = void;
    using input_type   = _Input;
    using parse_result = typename base_type::parse_result;

    // parse_impl
    //   implementation: succeeds only at end of input.
    template<typename _Source>
    parse_result
    parse_impl
    (
        _Source& _source
    )
    {
        // check for eof
        if (!_source.eof())
        {
            return parse_result::make_failure("expected end of input",
                                              _source.position());
        }

        return parse_result::make_success();
    }
};

// eof
//   factory: creates an end-of-input parser.
template<typename _Input = char>
d_eof_parser<_Input>
eof()
{
    return d_eof_parser<_Input>();
}

// -----------------------------------------------------------------------------
// VIII.5 Success/Failure parsers
// -----------------------------------------------------------------------------

// d_success_parser
//   class: parser that always succeeds with a given value.
template<typename _Result>
class d_success_parser : public d_parser_base<d_success_parser<_Result>,
                                              _Result,
                                              char>
{
public:
    using base_type    = d_parser_base<d_success_parser<_Result>, _Result, char>;
    using result_type  = _Result;
    using parse_result = typename base_type::parse_result;

private:
    result_type m_value;

public:
    // d_success_parser
    //   constructor: creates always-succeeding parser.
    explicit d_success_parser(result_type _value)
        : m_value(std::move(_value))
    {}

    // parse_impl
    //   implementation: always returns success.
    template<typename _Source>
    parse_result
    parse_impl
    (
        _Source&
    )
    {
        return parse_result::make_success(m_value);
    }
};

// success
//   factory: creates an always-succeeding parser.
template<typename _Result>
d_success_parser<typename std::decay<_Result>::type>
success
(
    _Result&& _value
)
{
    return d_success_parser<typename std::decay<_Result>::type>(
               std::forward<_Result>(_value));
}

// d_failure_parser
//   class: parser that always fails.
template<typename _Result>
class d_failure_parser : public d_parser_base<d_failure_parser<_Result>,
                                              _Result,
                                              char>
{
public:
    using base_type    = d_parser_base<d_failure_parser<_Result>, _Result, char>;
    using result_type  = _Result;
    using parse_result = typename base_type::parse_result;

private:
    const char* m_message;

public:
    // d_failure_parser
    //   constructor: creates always-failing parser.
    explicit d_failure_parser(const char* _message = "parse failure")
        : m_message(_message)
    {}

    // parse_impl
    //   implementation: always returns failure.
    template<typename _Source>
    parse_result
    parse_impl
    (
        _Source& _source
    )
    {
        return parse_result::make_failure(m_message, _source.position());
    }
};

// failure
//   factory: creates an always-failing parser.
template<typename _Result>
d_failure_parser<_Result>
failure
(
    const char* _message = "parse failure"
)
{
    return d_failure_parser<_Result>(_message);
}


// =============================================================================
// IX.  CHARACTER CLASS PARSERS
// =============================================================================

// d_char_class
//   namespace: contains character classification predicates.
namespace d_char_class
{
    // is_digit
    //   predicate: returns true for ASCII digits ('0'-'9').
    struct is_digit
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return (_ch >= '0') && (_ch <= '9');
        }
    };

    // is_alpha
    //   predicate: returns true for ASCII letters.
    struct is_alpha
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return ( ((_ch >= 'a') && (_ch <= 'z')) ||
                     ((_ch >= 'A') && (_ch <= 'Z')) );
        }
    };

    // is_alnum
    //   predicate: returns true for ASCII alphanumeric.
    struct is_alnum
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return ( ((_ch >= '0') && (_ch <= '9')) ||
                     ((_ch >= 'a') && (_ch <= 'z')) ||
                     ((_ch >= 'A') && (_ch <= 'Z')) );
        }
    };

    // is_space
    //   predicate: returns true for whitespace characters.
    struct is_space
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return ( (_ch == ' ')  ||
                     (_ch == '\t') ||
                     (_ch == '\n') ||
                     (_ch == '\r') ||
                     (_ch == '\f') ||
                     (_ch == '\v') );
        }
    };

    // is_hex
    //   predicate: returns true for hexadecimal digits.
    struct is_hex
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return ( ((_ch >= '0') && (_ch <= '9')) ||
                     ((_ch >= 'a') && (_ch <= 'f')) ||
                     ((_ch >= 'A') && (_ch <= 'F')) );
        }
    };

    // is_upper
    //   predicate: returns true for uppercase letters.
    struct is_upper
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return (_ch >= 'A') && (_ch <= 'Z');
        }
    };

    // is_lower
    //   predicate: returns true for lowercase letters.
    struct is_lower
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return (_ch >= 'a') && (_ch <= 'z');
        }
    };

    // is_print
    //   predicate: returns true for printable ASCII characters.
    struct is_print
    {
        bool
        operator()
        (
            char _ch
        ) const
        {
            return (_ch >= 0x20) && (_ch <= 0x7E);
        }
    };

    // is_one_of
    //   predicate: returns true if character is in the set.
    class is_one_of
    {
    private:
        const char* m_chars;
        std::size_t m_length;

    public:
        is_one_of(const char* _chars,
                  std::size_t _length)
            : m_chars(_chars)
            , m_length(_length)
        {}

        bool
        operator()
        (
            char _ch
        ) const
        {
            std::size_t i;

            for (i = 0; i < m_length; ++i)
            {
                if (m_chars[i] == _ch)
                {
                    return true;
                }
            }

            return false;
        }
    };

    // is_none_of
    //   predicate: returns true if character is not in the set.
    class is_none_of
    {
    private:
        const char* m_chars;
        std::size_t m_length;

    public:
        is_none_of(const char* _chars,
                   std::size_t _length)
            : m_chars(_chars)
            , m_length(_length)
        {}

        bool
        operator()
        (
            char _ch
        ) const
        {
            std::size_t i;

            for (i = 0; i < m_length; ++i)
            {
                if (m_chars[i] == _ch)
                {
                    return false;
                }
            }

            return true;
        }
    };

    // is_in_range
    //   predicate: returns true if character is in inclusive range.
    class is_in_range
    {
    private:
        char m_low;
        char m_high;

    public:
        is_in_range(char _low,
                    char _high)
            : m_low(_low)
            , m_high(_high)
        {}

        bool
        operator()
        (
            char _ch
        ) const
        {
            return (_ch >= m_low) && (_ch <= m_high);
        }
    };

}  // namespace d_char_class

// digit
//   factory: creates a parser that matches ASCII digits.
inline d_predicate_parser<d_char_class::is_digit, char>
digit()
{
    return satisfy<d_char_class::is_digit, char>(d_char_class::is_digit());
}

// alpha
//   factory: creates a parser that matches ASCII letters.
inline d_predicate_parser<d_char_class::is_alpha, char>
alpha()
{
    return satisfy<d_char_class::is_alpha, char>(d_char_class::is_alpha());
}

// alnum
//   factory: creates a parser that matches alphanumeric characters.
inline d_predicate_parser<d_char_class::is_alnum, char>
alnum()
{
    return satisfy<d_char_class::is_alnum, char>(d_char_class::is_alnum());
}

// space
//   factory: creates a parser that matches whitespace.
inline d_predicate_parser<d_char_class::is_space, char>
space()
{
    return satisfy<d_char_class::is_space, char>(d_char_class::is_space());
}

// hex_digit
//   factory: creates a parser that matches hexadecimal digits.
inline d_predicate_parser<d_char_class::is_hex, char>
hex_digit()
{
    return satisfy<d_char_class::is_hex, char>(d_char_class::is_hex());
}

// one_of
//   factory: creates a parser that matches one of the given characters.
inline d_predicate_parser<d_char_class::is_one_of, char>
one_of
(
    const char* _chars,
    std::size_t _length
)
{
    return satisfy<d_char_class::is_one_of, char>(
               d_char_class::is_one_of(_chars, _length));
}

// none_of
//   factory: creates a parser that matches none of the given characters.
inline d_predicate_parser<d_char_class::is_none_of, char>
none_of
(
    const char* _chars,
    std::size_t _length
)
{
    return satisfy<d_char_class::is_none_of, char>(
               d_char_class::is_none_of(_chars, _length));
}

// char_range
//   factory: creates a parser that matches characters in a range.
inline d_predicate_parser<d_char_class::is_in_range, char>
char_range
(
    char _low,
    char _high
)
{
    return satisfy<d_char_class::is_in_range, char>(
               d_char_class::is_in_range(_low, _high));
}


NS_END  // djinterp


#endif  // DJINTERP_PARSE_
