/******************************************************************************
* djinterp [core]                                                   parse.hpp
*
* Common parsing primitives:
*   This header defines the foundational types, enumerations, and result
* wrappers shared by all parser modules within the djinterp framework.
* It is intentionally agnostic to the nature of the input being parsed
* (text, binary, or otherwise).
*
* Contents:
*   - parse_status       enum class for parse outcome classification
*   - parse_error        lightweight error descriptor
*   - parse_result       discriminated success/failure wrapper
*   - parse_state        generic cursor/progress tracker
*
*
* path:      /inc/parse/parse.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.11
******************************************************************************/

#ifndef DJINTERP_PARSE_
#define DJINTERP_PARSE_ 1

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include "../core/djinterp.hpp"


// D_KEYWORD_PARSE
//   keyword: resolves to `parse`.
// Used to specify that a unit of code pertains to the parsing
// subsystem.
#define D_KEYWORD_PARSE             parse

// NS_PARSE
//   namespace: the parse subsystem namespace.
#define NS_PARSE                    D_NAMESPACE(D_KEYWORD_PARSE)


NS_PARSE

// parseable
//   trait: associates a minor (token) type with a major (parsed
// aggregate) type for a given parseable domain.
template<typename _Minor,
         typename _Major>
struct parseable
{
    using minor = _Minor;
    using major = _Major;
};

// is_parseable
//   trait: primary template — not parseable by default.
template<typename _Type,
         typename = void>
struct is_parseable : std::false_type
{};

// is_parseable (SFINAE specialization)
//   trait: true when _Type exposes nested minor and major aliases.
template<typename _Type>
struct is_parseable<_Type, std::void_t<
    typename _Type::minor,
    typename _Type::major>
> : std::true_type
{};

// is_parseable<std::string>
//   trait: std::string is parseable (chars -> strings).
template<>
struct is_parseable<std::string, void> : std::true_type
{};

// is_parseable<const char*>
//   trait: const char* is parseable (chars -> C-strings).
template<>
struct is_parseable<const char*, void> : std::true_type
{};

// is_parseable<char*>
//   trait: char* is parseable (chars -> C-strings).
template<>
struct is_parseable<char*, void> : std::true_type
{};

// parseable<char, std::string>
//   trait: char is the minor type for std::string aggregation.
template<>
struct parseable<char, std::string>
{};

// parse_traits
//   trait: maps a parseable type to its minor/major pair.
template<typename _Type>
struct parse_traits;

// parse_traits<std::string>
//   trait: std::string parses from char to std::string.
template<>
struct parse_traits<std::string> : parseable<char, std::string>
{};

// parse_traits<const char*>
//   trait: const char* parses from char to const char*.
template<>
struct parse_traits<const char*> : parseable<char, const char*>
{};

// parse_traits<char*>
//   trait: char* parses from char to char*.
template<>
struct parse_traits<char*> : parseable<char, char*>
{};


// ================================================================
//  parse_status
// ================================================================

// parse_status
//   typedef: classifies the outcome of a parse operation.
typedef std::int32_t parse_status;


// ================================================================
//  parse_error
// ================================================================

// parse_error
//   struct: lightweight descriptor carrying information about a
// parse failure, including the offset at which it occurred, a
// status code, and an optional human-readable message.
struct parse_error
{
public:
    parse_error();

    parse_error
    (
        parse_status _status,
        std::size_t  _offset,
        const char*  _message = nullptr
    );

    parse_error(const parse_error& _other);

    parse_error& operator=(const parse_error& _other);

    parse_status    status()  const;
    std::size_t     offset()  const;
    const char*     message() const;

private:
    parse_status    m_status;
    std::size_t     m_offset;
    const char*     m_message;
};


// ================================================================
//  parse_message
// ================================================================

// parse_message
//   struct: variadic, tuple-like container for composing
// heterogeneous parse diagnostic values.
template<typename... _Types>
struct parse_message
{};

// parse_message (recursive case)
//   struct: stores the head value and inherits the remaining
// fields from the tail pack.
template<typename    _Type,
         typename... _Types>
struct parse_message<_Type, _Types...> : parse_message<_Types...>
{
    _Type m_value;

    parse_message
    (
        _Type     _value,
        _Types... _rest
    ) : parse_message<_Types...>(_rest...),
        m_value(_value)
    {
    }
};

NS_INTERNAL

    // parse_message_field
    //   trait: recursive accessor for parse_message elements by
    // index.
    template<size_t      _Index,
             typename    _Type,
             typename... _Types>
    struct parse_message_field
    {
        static auto& get(parse_message<_Type, _Types...>& _value)
        {
            return parse_message_field<
                _Index - 1, _Types...
            >::get(
                static_cast<parse_message<_Types...>&>(_value)
            );
        }
    };

    // parse_message_field<0, ...>
    //   trait: base case — returns the head element.
    template<typename    _Type,
             typename... _Types>
    struct parse_message_field<0, _Type, _Types...>
    {
        static _Type& get(parse_message<_Type, _Types...>& _msg)
        {
            return _msg.m_value;
        }
    };

NS_END  // internal

// get
//   extracts the element at _Index from a parse_message.
template<size_t      _Index,
         typename... _Types>
auto&
get(parse_message<_Types...>& _t)
{
    return internal::parse_message_field<
        _Index, _Types...
    >::get(_t);
}


// ================================================================
//  parse_result
// ================================================================

NS_INTERNAL

    // parse_result_storage
    //   trait: internal storage helper for parse_result.  Holds
    // the value in an aligned buffer to avoid requiring default
    // constructibility from _ValueType.
    template<typename _ValueType>
    struct parse_result_storage
    {
    private:
        using storage_type = typename std::aligned_storage<
            sizeof(_ValueType),
            alignof(_ValueType)
        >::type;

    protected:
        storage_type    m_storage;
        bool            m_has_value;

        parse_result_storage()
            : m_has_value(false)
        {
        }

        // value_ptr
        //   returns a pointer to the stored value.
        _ValueType* value_ptr()
        {
            return reinterpret_cast<_ValueType*>(
                &m_storage
            );
        }

        // value_ptr (const)
        //   returns a const pointer to the stored value.
        const _ValueType* value_ptr() const
        {
            return reinterpret_cast<const _ValueType*>(
                &m_storage
            );
        }
    };

NS_END  // internal

// parse_result
//   struct: a discriminated result type carrying either a parsed
// value of type _ValueType on success, or a parse_error on
// failure.  Does not require _ValueType to be default-
// constructible.
template<typename _ValueType>
struct parse_result : private internal::parse_result_storage<_ValueType>
{
private:
    using base_type = internal::parse_result_storage<_ValueType>;

public:
    using value_type = _ValueType;
    using error_type = parse_error;

    // parse_result (success)
    //   constructs a successful result holding a copy of the
    // value.
    explicit parse_result
    (
        const _ValueType& _value
    ) : base_type(),
        m_error()
    {
        new (this->value_ptr()) _ValueType(_value);

        this->m_has_value = true;
    }

    // parse_result (error)
    //   constructs a failed result holding the given error.
    explicit parse_result
    (
        const parse_error& _error
    ) : base_type(),
        m_error(_error)
    {
    }

    // parse_result (move — value)
    //   constructs a successful result by moving the value.
    explicit parse_result
    (
        _ValueType&& _value
    ) : base_type(),
        m_error()
    {
        new (this->value_ptr()) _ValueType(
            static_cast<_ValueType&&>(_value)
        );

        this->m_has_value = true;
    }

    // ~parse_result
    //   destructor: destroys the held value if present.
    ~parse_result()
    {
        if (this->m_has_value)
        {
            this->value_ptr()->~_ValueType();
        }
    }

    // parse_result (copy)
    //   copy constructor.
    parse_result
    (
        const parse_result& _other
    ) : base_type(),
        m_error(_other.m_error)
    {
        if (_other.m_has_value)
        {
            new (this->value_ptr()) _ValueType(
                *_other.value_ptr()
            );

            this->m_has_value = true;
        }
    }

    // parse_result (move)
    //   move constructor.
    parse_result
    (
        parse_result&& _other
    ) : base_type(),
        m_error(_other.m_error)
    {
        if (_other.m_has_value)
        {
            new (this->value_ptr()) _ValueType(
                static_cast<_ValueType&&>(*_other.value_ptr())
            );

            this->m_has_value = true;
        }
    }

    // ok
    //   returns true if the result holds a value.
    bool
    ok() const
    {
        return this->m_has_value;
    }

    // value
    //   returns a reference to the held value.
    //   Precondition: ok() == true.
    _ValueType&
    value()
    {
        return *this->value_ptr();
    }

    // value (const)
    //   returns a const reference to the held value.
    //   Precondition: ok() == true.
    const _ValueType&
    value() const
    {
        return *this->value_ptr();
    }

    // error
    //   returns the held error descriptor.
    const parse_error&
    error() const
    {
        return m_error;
    }

    // make_ok
    //   factory: creates a successful parse_result.
    D_STATIC parse_result
    make_ok(const _ValueType& _value)
    {
        return parse_result(_value);
    }

    // make_error
    //   factory: creates a failed parse_result.
    D_STATIC parse_result
    make_error
    (
        parse_status _status,
        std::size_t  _offset,
        const char*  _message = nullptr
    )
    {
        return parse_result(
            parse_error(_status, _offset, _message)
        );
    }

private:
    error_type m_error;
};


// ================================================================
//  parse_state
// ================================================================

// parse_state
//   struct: tracks the position and remaining extent of a parse
// operation over an input of element type _ElementType.  Agnostic
// to the nature of the underlying data — works equally for
// character streams, byte buffers, token sequences, etc.
template<typename _ElementType>
struct parse_state
{
    using element_type = _ElementType;

    const _ElementType* data;
    std::size_t         length;
    std::size_t         offset;

    parse_state
    () : data   (nullptr),
         length (0),
         offset (0)
    {
    }

    parse_state
    (
        const _ElementType* _data,
        std::size_t         _length,
        std::size_t         _offset = 0
    ) : data   (_data),
        length (_length),
        offset (_offset)
    {
    }

    // remaining
    //   returns the number of elements still available.
    std::size_t
    remaining() const
    {
        return (offset < length)
                    ? (length - offset)
                    : 0;
    }

    // at_end
    //   returns true when no input remains.
    bool
    at_end() const
    {
        return (offset >= length);
    }

    // current
    //   returns a pointer to the current element, or nullptr
    // if at the end.
    const _ElementType*
    current() const
    {
        return at_end()
                    ? nullptr
                    : (data + offset);
    }

    // advance
    //   moves the offset forward by _count elements, clamped
    // to the end of the input.
    void
    advance
    (
        std::size_t _count = 1
    )
    {
        offset += _count;

        if (offset > length)
        {
            offset = length;
        }

        return;
    }
};


NS_END  // parse


#endif  // DJINTERP_PARSE_