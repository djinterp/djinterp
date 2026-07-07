/******************************************************************************
* djinterp [container]                                              decode.hpp
*
*   The foundational half of the SERIALIZATION externalisation axis on the READ
* side: the DECODER.  Where encode.hpp casts a value out to the flat medium B*
* (containers.tex, Serialization), this recovers a value from it.  This header
* owns the READER over a bit string, the PARTIAL-result type a decode yields, the
* LEAF element decoder dec_tau : B* -/-> tau, and the extension point through
* which a user type supplies its own dec_tau; the recursion that rebuilds a whole
* container lives in container_decode.hpp.
*
*   PARTIALITY:
*   A decoder is a PARTIAL function dec : B* -/-> F[tau] (containers.tex,
* Serialization) - "not every string need be a valid encoding".  Partiality is
* made explicit two ways that agree: a `byte_reader` carries a GOOD flag that a
* short read (underflow) clears, and every `decode` returns a `decode_result<T>`
* whose `ok` says whether a value was recovered.  A caller checks `ok`; a
* container decoder propagates the first failure outward.
*
*   THE ROUND TRIP - A COPY THROUGH THE MEDIUM:
*   dec_tau here is the exact inverse of encode.hpp's enc_tau: a fixed-width
* big-endian field is read back most-significant byte first, a signed integer
* through its two's-complement unsigned pattern, a float through its same-width
* unsigned bit pattern.  What the round trip does NOT restore - object identity,
* allocator, traversal machinery - is not this layer's concern; dec(enc(c)) is a
* value equal to c at the serialised level, its identity and realisation made
* anew (containers.tex, Serialization).
*
*   dec_tau REQUIRES A DEFAULT VALUE:
*   `decode_result<T>` holds a `T` by value, so a decodable `T` must be
* default-constructible - a failed result carries a default-constructed `T`, and
* a successful one is built into place.  The built-in leaves and the standard
* containers satisfy this; a custom leaf supplies a default constructor alongside
* its `decode` (below).
*
*   THE dec_tau CUSTOMIZATION SURFACE:
*   A non-built-in leaf becomes decodable by exposing a STATIC member
* `static decode_result<T> decode(byte_reader&)` on `T` - the inverse of the
* member `encode_into` of encode.hpp.  A member so found takes precedence over
* every built-in leaf reader.
*
*   STAGE:
*   Decoding an externally supplied stream is a runtime act (containers.tex,
* Lifetime): the reader walks a buffer and the result is built at runtime.  The
* leaf readers are ordinary function templates, not constexpr, matching the
* runtime nature of the act.
*
*   PORTABILITY:
*   C++11 baseline.  The `_v` companions degrade with the language as elsewhere.
*
*
* path:      /inc/djinterp/core/container/serial/decode.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_DECODE_
#define DJINTERP_DECODE_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"            // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"  // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL


NS_DJINTERP


// ===========================================================================
// I.   The medium (B*) - reader side
// ===========================================================================
//   `byte` and `byte_string` are the same medium encode.hpp names; declared
// here too so this header stands alone.  Identical `using` aliases are
// compatible, so the two coexist when both headers are included.

// byte
//   type: one octet of the binary medium.
using byte = unsigned char;

// byte_string
//   type: a finite run of bytes - the concrete realisation of B*.
using byte_string = std::vector<byte>;

// decode_length_type
//   type: the width-fixed unsigned integer a container decoder reads as the
// leading size (element count) - the inverse of encode's leading count.
using decode_length_type = std::uint64_t;


// ===========================================================================
// II.  byte_reader - a cursor over a bit string
// ===========================================================================

// byte_reader
//   class: a forward cursor over a run of bytes.  It advances as fields are
// taken and clears its GOOD flag on the first short read; once bad it stays
// bad, so a partial decode fails cleanly rather than reading past the end.
class byte_reader
{
public:
    using size_type = std::size_t;

    // --- construction ---

    // empty reader (immediately exhausted, still good).
    D_CONSTEXPR byte_reader()
        : m_cur(nullptr),
          m_end(nullptr),
          m_good(true)
    {}

    // over a raw byte range [ _data, _data + _size ).
    D_CONSTEXPR byte_reader(
        const byte* _data,
        size_type   _size
    )
        : m_cur(_data),
          m_end(_data + _size),
          m_good(true)
    {}

    // over a byte_string (the buffer must outlive the reader).
    explicit byte_reader(
        const byte_string& _bytes
    )
        : m_cur(_bytes.data()),
          m_end(_bytes.data() + _bytes.size()),
          m_good(true)
    {}

    // --- observation ---

    // good
    //   function: whether every read so far has succeeded.
    D_CONSTEXPR bool good() const
    {
        return m_good;
    }

    // remaining
    //   function: unread bytes left in the buffer.
    D_CONSTEXPR size_type remaining() const
    {
        return static_cast<size_type>(m_end - m_cur);
    }

    // exhausted
    //   function: whether the cursor has reached the end of the buffer.
    D_CONSTEXPR bool exhausted() const
    {
        return ( m_cur == m_end );
    }

    // --- consumption ---

    // take
    //   function: copy the next `_n` bytes into `_out` and advance.  On
    // underflow (fewer than `_n` remain) it takes nothing, clears the good flag,
    // and returns false; this is the sole way the flag is cleared.
    bool take(
        byte*     _out,
        size_type _n
    )
    {
        // a prior failure is sticky - never read past a cleared flag
        if (!m_good)
        {
            return false;
        }

        // underflow - not enough bytes remain to satisfy the request
        if (remaining() < _n)
        {
            m_good = false;

            return false;
        }

        // copy and advance
        for (size_type _i = 0; _i < _n; ++_i)
        {
            _out[_i] = m_cur[_i];
        }

        m_cur += _n;

        return true;
    }

private:
    const byte* m_cur;
    const byte* m_end;
    bool        m_good;
};


// ===========================================================================
// III. decode_result<T> - a recovered value, or failure
// ===========================================================================

// decode_result
//   struct: the outcome of a decode - `ok` records whether a value was
// recovered, `value` holds it (a default-constructed `T` when `ok` is false).
// The explicit partiality the model's dec : B* -/-> F[tau] calls for.
template<typename _Type>
struct decode_result
{
    bool  ok;
    _Type value;
};

// decode_success
//   function: a successful result carrying `_value`.
template<typename _Type>
decode_result<clean_t<_Type>>
decode_success(_Type&& _value)
{
    return decode_result<clean_t<_Type>>{
        true, static_cast<_Type&&>(_value)};
}

// decode_failure
//   function: a failed result of element type `_Type` (a default-constructed
// value paired with ok = false).
template<typename _Type>
decode_result<_Type>
decode_failure()
{
    return decode_result<_Type>{false, _Type()};
}


// ===========================================================================
// IV.  Member-decoder detection (the customization surface)
// ===========================================================================

// has_member_decode
//   trait: true iff `_Type` exposes a static `decode(byte_reader&)` yielding a
// `decode_result<_Type>` - the extension point by which a non-built-in leaf
// supplies its own dec_tau.  A member so found takes precedence over the
// built-in leaf readers.
template<typename _Type,
         typename = void>
struct has_member_decode : std::false_type
{};

template<typename _Type>
struct has_member_decode<_Type,
    D_VOID_T<decltype(clean_t<_Type>::decode(std::declval<byte_reader&>()))>>
    : std::is_same<
          decltype(clean_t<_Type>::decode(std::declval<byte_reader&>())),
          decode_result<clean_t<_Type>>>
{};


// ===========================================================================
// V.   Fixed-width byte readers (internal)
// ===========================================================================

NS_INTERNAL

    // get_uint_be
    //   helper: read `_width` (<= 8) bytes big-endian from `_reader` into
    // `_out`.  Returns false (and leaves the reader bad) on underflow.  The
    // exact inverse of encode's put_uint_be.
    inline bool
    get_uint_be(
        byte_reader&         _reader,
        std::size_t          _width,
        decode_length_type&  _out
    )
    {
        byte _tmp[8] = {0};

        // a width beyond the buffer of a decode_length_type cannot be read
        if (_width > 8)
        {
            return false;
        }

        // pull the raw field; underflow fails the whole decode
        if (!_reader.take(_tmp, _width))
        {
            return false;
        }

        // reassemble most-significant byte first
        decode_length_type _value = 0;
        for (std::size_t _i = 0; _i < _width; ++_i)
        {
            _value = ( _value << 8 )
                   | static_cast<decode_length_type>(_tmp[_i]);
        }

        _out = _value;

        return true;
    }

    // decode_integral_leaf
    //   helper: read an integral (non-bool) value written at its natural width -
    // reassemble the unsigned pattern, then cast back through the same-width
    // unsigned type to the target, reversing encode_integral_leaf exactly.
    template<typename _Integral>
    decode_result<_Integral>
    decode_integral_leaf(
        byte_reader& _reader
    )
    {
        using unsigned_type = typename std::make_unsigned<_Integral>::type;

        decode_length_type _raw = 0;

        if (!get_uint_be(_reader, sizeof(_Integral), _raw))
        {
            return decode_failure<_Integral>();
        }

        return decode_success(
            static_cast<_Integral>(static_cast<unsigned_type>(_raw)));
    }

    // decode_floating_leaf
    //   helper: read a 4- or 8-byte floating value - reassemble the same-width
    // unsigned pattern, then std::memcpy it back into the float, reversing
    // encode_floating_leaf exactly.
    template<typename _Float>
    decode_result<_Float>
    decode_floating_leaf(
        byte_reader& _reader
    )
    {
        decode_length_type _raw = 0;

        if (!get_uint_be(_reader, sizeof(_Float), _raw))
        {
            return decode_failure<_Float>();
        }

        _Float _value = _Float();

        // 4-byte <- uint32 pattern, 8-byte <- uint64 pattern; the enclosing
        // overload admits only these two widths.
        if (sizeof(_Float) == 4)
        {
            std::uint32_t _bits = static_cast<std::uint32_t>(_raw);
            std::memcpy(&_value, &_bits, 4);
        }
        else
        {
            std::uint64_t _bits = static_cast<std::uint64_t>(_raw);
            std::memcpy(&_value, &_bits, 8);
        }

        return decode_success(_value);
    }

    // leaf_decoder
    //   helper: dispatch a leaf `_Type` to its dec_tau by category.  The primary
    // is the "no dec_tau" case (a hard error if instantiated); the four built-in
    // specializations and the member surface cover every decodable leaf.  The
    // category booleans are mutually exclusive, so at most one specialization
    // is viable.
    template<typename _Type,
             bool _Member =
                 has_member_decode<_Type>::value,
             bool _Bool =
                 std::is_same<_Type, bool>::value,
             bool _Integral =
                 ( std::is_integral<_Type>::value
                && !std::is_same<_Type, bool>::value
                && ( sizeof(_Type) <= 8 ) ),
             bool _Enum =
                 std::is_enum<_Type>::value,
             bool _Float =
                 ( std::is_floating_point<_Type>::value
                && ( sizeof(_Type) == 4 || sizeof(_Type) == 8 ) )>
    struct leaf_decoder;

    // member surface (highest precedence)
    template<typename _Type,
             bool _B, bool _I, bool _E, bool _F>
    struct leaf_decoder<_Type, true, _B, _I, _E, _F>
    {
        static decode_result<_Type> read(byte_reader& _reader)
        {
            return clean_t<_Type>::decode(_reader);
        }
    };

    // bool
    template<typename _Type,
             bool _I, bool _E, bool _F>
    struct leaf_decoder<_Type, false, true, _I, _E, _F>
    {
        static decode_result<_Type> read(byte_reader& _reader)
        {
            byte _b = 0;

            if (!_reader.take(&_b, 1))
            {
                return decode_failure<_Type>();
            }

            return decode_success(static_cast<bool>(_b != 0));
        }
    };

    // integral (non-bool)
    template<typename _Type,
             bool _E, bool _F>
    struct leaf_decoder<_Type, false, false, true, _E, _F>
    {
        static decode_result<_Type> read(byte_reader& _reader)
        {
            return decode_integral_leaf<_Type>(_reader);
        }
    };

    // enum (through its underlying type)
    template<typename _Type,
             bool _F>
    struct leaf_decoder<_Type, false, false, false, true, _F>
    {
        static decode_result<_Type> read(byte_reader& _reader)
        {
            using underlying_type =
                typename std::underlying_type<_Type>::type;

            decode_result<underlying_type> _u =
                decode_integral_leaf<underlying_type>(_reader);

            if (!_u.ok)
            {
                return decode_failure<_Type>();
            }

            return decode_success(static_cast<_Type>(_u.value));
        }
    };

    // floating (4- or 8-byte)
    template<typename _Type>
    struct leaf_decoder<_Type, false, false, false, false, true>
    {
        static decode_result<_Type> read(byte_reader& _reader)
        {
            return decode_floating_leaf<_Type>(_reader);
        }
    };

NS_END  // internal


// ===========================================================================
// VI.  dec_tau - the leaf element decoder
// ===========================================================================

// decode
//   function: dec_tau for a leaf `_Type` - reads one value of `_Type` from the
// reader, returning a partial result.  `_Type` is explicit, since the return
// type depends on it and there is no value argument to deduce from.
template<typename _Type>
decode_result<clean_t<_Type>>
decode(byte_reader& _reader)
{
    return internal::leaf_decoder<clean_t<_Type>>::read(_reader);
}

// decode
//   function: convenience over a whole `byte_string` - wraps it in a reader and
// reads one leaf `_Type` from the front.
template<typename _Type>
decode_result<clean_t<_Type>>
decode(const byte_string& _bytes)
{
    byte_reader _reader(_bytes);

    return decode<_Type>(_reader);
}


// ===========================================================================
// VII. Leaf-decodability trait
// ===========================================================================

NS_INTERNAL

    // decode_builtin_leaf_ok
    //   helper: whether `_Type` is a built-in leaf of a fixed decodable width.
    // Gated on arithmetic-or-enum so `sizeof` is only ever applied to an object
    // type - a non-object leaf (in particular void, the "no element" type of a
    // non-container) selects the primary and reports false without a sizeof.
    template<typename _Type,
             bool = ( std::is_integral<_Type>::value
                   || std::is_floating_point<_Type>::value
                   || std::is_enum<_Type>::value )>
    struct decode_builtin_leaf_ok : std::false_type
    {};

    template<typename _Type>
    struct decode_builtin_leaf_ok<_Type, true>
        : std::integral_constant<bool,
              ( std::is_enum<_Type>::value
             || std::is_same<_Type, bool>::value
             || ( std::is_integral<_Type>::value
               && ( sizeof(_Type) <= 8 ) )
             || ( std::is_floating_point<_Type>::value
               && ( sizeof(_Type) == 4 || sizeof(_Type) == 8 ) ) )>
    {};

NS_END  // internal

// is_leaf_decodable
//   trait: true iff `_Type` has a leaf dec_tau in this header - a static member
// `decode`, or one of the built-in leaf families (bool, an <=8-byte integral,
// an enum, or a 4-/8-byte floating type).
template<typename _Type>
struct is_leaf_decodable
    : std::integral_constant<bool,
          ( has_member_decode<clean_t<_Type>>::value
         || internal::decode_builtin_leaf_ok<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_leaf_decodable)


NS_END  // djinterp


#endif  // DJINTERP_DECODE_
