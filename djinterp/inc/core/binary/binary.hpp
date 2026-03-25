/******************************************************************************
* djinterp [binary]                                                 binary.hpp
*
* Foundational binary I/O module for the djinterp framework.
*   Provides the common vocabulary types, header format, endianness
* detection, and low-level buffer primitives used by all binary
* encoding/decoding modules (containers, database blobs, network
* wire formats, file I/O, etc.).
*
*   This module is domain-agnostic: it has no knowledge of containers,
* database types, or any other higher-level abstraction.  Domain-
* specific modules (container_binary.hpp, database_binary.hpp, etc.)
* build on these primitives.
*
* BINARY HEADER FORMAT (40 bytes):
*   bytes  0- 3:  magic         (0x4E426A64 — "djNB")
*   bytes  4- 5:  version       (encoding format version)
*   bytes  6- 7:  flags         (endianness, options)
*   bytes  8-15:  type_info     (d_type_info64 descriptor)
*   bytes 16-19:  element_size  (0 if variable-length)
*   bytes 20-23:  reserved
*   bytes 24-31:  count         (uint64_t element count)
*   bytes 32-39:  payload_size  (uint64_t byte count)
*
* TABLE OF CONTENTS
* =================
* I.      Constants
* II.     Flags Enum and Operators
* III.    Header Struct
* IV.     Endianness Detection
* V.      Header Read / Write
* VI.     Buffer Write Primitives
* VII.    Buffer Read Primitives
* VIII.   Type Info Mapping
*
*
* path:      /inc/binary/binary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_BINARY_
#define DJINTERP_BINARY_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "../djinterp.hpp"
#include "../../c/type_info.h"


NS_DJINTERP

// D_KEYWORD_BINARY / NS_BINARY
#ifndef D_KEYWORD_BINARY
    #define D_KEYWORD_BINARY    binary
#endif

#ifndef NS_BINARY
    #define NS_BINARY           D_NAMESPACE(D_KEYWORD_BINARY)
#endif

NS_BINARY


// =============================================================================
// I.   Constants
// =============================================================================

static constexpr std::uint32_t D_BINARY_MAGIC =
    0x4E426A64u;

static constexpr std::uint16_t D_BINARY_VERSION =
    1u;

static constexpr std::size_t D_BINARY_HEADER_SIZE =
    40u;


// =============================================================================
// II.  Flags Enum and Operators
// =============================================================================

// binary_flags
//   enum: bit flags stored in the binary header.
enum class binary_flags : std::uint16_t
{
    none            = 0x0000,
    little_endian   = 0x0001,
    big_endian      = 0x0002,
    has_type_info   = 0x0010,
    fixed_elements  = 0x0020,
    compressed      = 0x0100,
    checksummed     = 0x0200
};

inline constexpr binary_flags
operator|(binary_flags _a,
          binary_flags _b) noexcept
{
    return static_cast<binary_flags>(
        static_cast<std::uint16_t>(_a) |
        static_cast<std::uint16_t>(_b));
}

inline constexpr binary_flags
operator&(binary_flags _a,
          binary_flags _b) noexcept
{
    return static_cast<binary_flags>(
        static_cast<std::uint16_t>(_a) &
        static_cast<std::uint16_t>(_b));
}

inline constexpr binary_flags&
operator|=(binary_flags& _a,
           binary_flags  _b) noexcept
{
    _a = _a | _b;

    return _a;
}

inline constexpr bool
has_flag(binary_flags _set,
         binary_flags _flag) noexcept
{
    return ((_set & _flag) == _flag);
}


// =============================================================================
// III. Header Struct
// =============================================================================

// binary_header
//   struct: 40-byte fixed-layout header prepended to
// binary blobs for self-description and validation.
struct binary_header
{
    std::uint32_t magic;
    std::uint16_t version;
    binary_flags  flags;
    d_type_info64 type_info;
    std::uint32_t element_size;
    std::uint32_t reserved;
    std::uint64_t count;
    std::uint64_t payload_size;
};

static_assert(sizeof(binary_header) ==
              D_BINARY_HEADER_SIZE,
    "binary_header size mismatch");


// =============================================================================
// IV.  Endianness Detection
// =============================================================================

// detect_endianness
//   function: returns the endianness flag for the current
// platform at runtime.
inline binary_flags
detect_endianness() noexcept
{
    const std::uint16_t probe = 0x0001;

    return (*reinterpret_cast<
                const unsigned char*>(&probe)
            == 0x01)
        ? binary_flags::little_endian
        : binary_flags::big_endian;
}

// is_little_endian
inline bool
is_little_endian() noexcept
{
    return has_flag(detect_endianness(),
                    binary_flags::little_endian);
}


// =============================================================================
// V.   Header Read / Write
// =============================================================================

// make_header
//   function: constructs a header with magic, version,
// endianness, and the given count/payload size.
// Type info and element size are left zeroed — callers
// fill those fields for their domain.
inline binary_header
make_header(std::uint64_t _count,
            std::uint64_t _payload_size) noexcept
{
    binary_header h{};

    h.magic        = D_BINARY_MAGIC;
    h.version      = D_BINARY_VERSION;
    h.flags        = detect_endianness();
    h.count        = _count;
    h.payload_size = _payload_size;

    return h;
}

// write_header
//   function: serializes a header into a buffer.
// Returns the number of bytes written
// (D_BINARY_HEADER_SIZE on success, 0 on failure).
inline std::size_t
write_header(char*                _buf,
             std::size_t          _capacity,
             const binary_header& _header) noexcept
{
    if ( (!_buf) ||
         (_capacity < D_BINARY_HEADER_SIZE) )
    {
        return 0;
    }

    std::memcpy(_buf, &_header,
                D_BINARY_HEADER_SIZE);

    return D_BINARY_HEADER_SIZE;
}

// read_header
//   function: deserializes a header from a buffer.
// Returns true if the magic and version are valid.
inline bool
read_header(const char*    _data,
            std::size_t    _size,
            binary_header& _out) noexcept
{
    if ( (!_data) ||
         (_size < D_BINARY_HEADER_SIZE) )
    {
        return false;
    }

    std::memcpy(&_out, _data, D_BINARY_HEADER_SIZE);

    return ( (_out.magic == D_BINARY_MAGIC) &&
             (_out.version <= D_BINARY_VERSION) );
}

// validate_header
//   function: checks magic and version of an already-
// read header.
inline bool
validate_header(
    const binary_header& _header) noexcept
{
    return ( (_header.magic == D_BINARY_MAGIC) &&
             (_header.version <=
              D_BINARY_VERSION) );
}


// =============================================================================
// VI.  Buffer Write Primitives
// =============================================================================
// Low-level helpers for writing typed values into a raw
// byte buffer.  Each returns the number of bytes written.

// write_bytes
//   function: copies _n bytes from _src into _buf.
inline std::size_t
write_bytes(char*       _buf,
            const void* _src,
            std::size_t _n) noexcept
{
    std::memcpy(_buf, _src, _n);

    return _n;
}

// write_value
//   function: writes a trivially copyable value into _buf.
template<typename _Type>
inline typename std::enable_if<
    std::is_trivially_copyable_v<_Type>,
    std::size_t
>::type
write_value(char*        _buf,
            const _Type& _val) noexcept
{
    std::memcpy(_buf, &_val, sizeof(_Type));

    return sizeof(_Type);
}

// write_u8 / write_u16 / write_u32 / write_u64
//   function: writes a fixed-width integer.
inline std::size_t
write_u8(char* _buf, std::uint8_t _val) noexcept
{
    *reinterpret_cast<std::uint8_t*>(_buf) = _val;

    return 1;
}

inline std::size_t
write_u16(char* _buf, std::uint16_t _val) noexcept
{
    std::memcpy(_buf, &_val, 2);

    return 2;
}

inline std::size_t
write_u32(char* _buf, std::uint32_t _val) noexcept
{
    std::memcpy(_buf, &_val, 4);

    return 4;
}

inline std::size_t
write_u64(char* _buf, std::uint64_t _val) noexcept
{
    std::memcpy(_buf, &_val, 8);

    return 8;
}


// =============================================================================
// VII. Buffer Read Primitives
// =============================================================================

// read_bytes
//   function: copies _n bytes from _buf into _dst.
inline std::size_t
read_bytes(const char* _buf,
           void*       _dst,
           std::size_t _n) noexcept
{
    std::memcpy(_dst, _buf, _n);

    return _n;
}

// read_value
//   function: reads a trivially copyable value from _buf.
template<typename _Type>
inline typename std::enable_if<
    std::is_trivially_copyable_v<_Type>,
    _Type
>::type
read_value(const char* _buf) noexcept
{
    _Type val;

    std::memcpy(&val, _buf, sizeof(_Type));

    return val;
}

// read_u8 / read_u16 / read_u32 / read_u64
inline std::uint8_t
read_u8(const char* _buf) noexcept
{
    return *reinterpret_cast<
        const std::uint8_t*>(_buf);
}

inline std::uint16_t
read_u16(const char* _buf) noexcept
{
    std::uint16_t val;

    std::memcpy(&val, _buf, 2);

    return val;
}

inline std::uint32_t
read_u32(const char* _buf) noexcept
{
    std::uint32_t val;

    std::memcpy(&val, _buf, 4);

    return val;
}

inline std::uint64_t
read_u64(const char* _buf) noexcept
{
    std::uint64_t val;

    std::memcpy(&val, _buf, 8);

    return val;
}


// =============================================================================
// VIII. Type Info Mapping
// =============================================================================
// Maps C++ fundamental types to d_type_info16 descriptors
// from type_info.h.  Domain modules use this to fill the
// header's type_info field.

NS_INTERNAL

    template<typename _Type>
    struct type_to_info
    {
        static constexpr d_type_info16 value = 0;
    };

    template<>
    struct type_to_info<bool>
    {
        static constexpr d_type_info16 value =
            D_TYPE_BOOL_();
    };

    template<>
    struct type_to_info<char>
    {
        static constexpr d_type_info16 value =
            D_TYPE_CHAR_();
    };

    template<>
    struct type_to_info<signed char>
    {
        static constexpr d_type_info16 value =
            D_TYPE_SCHAR_();
    };

    template<>
    struct type_to_info<unsigned char>
    {
        static constexpr d_type_info16 value =
            D_TYPE_UCHAR_();
    };

    template<>
    struct type_to_info<short>
    {
        static constexpr d_type_info16 value =
            D_TYPE_SHORT_();
    };

    template<>
    struct type_to_info<unsigned short>
    {
        static constexpr d_type_info16 value =
            D_TYPE_USHORT_();
    };

    template<>
    struct type_to_info<int>
    {
        static constexpr d_type_info16 value =
            D_TYPE_INT_();
    };

    template<>
    struct type_to_info<unsigned int>
    {
        static constexpr d_type_info16 value =
            D_TYPE_UINT_();
    };

    template<>
    struct type_to_info<long>
    {
        static constexpr d_type_info16 value =
            D_TYPE_LONG_();
    };

    template<>
    struct type_to_info<unsigned long>
    {
        static constexpr d_type_info16 value =
            D_TYPE_ULONG_();
    };

    template<>
    struct type_to_info<long long>
    {
        static constexpr d_type_info16 value =
            D_TYPE_LLONG_();
    };

    template<>
    struct type_to_info<unsigned long long>
    {
        static constexpr d_type_info16 value =
            D_TYPE_ULLONG_();
    };

    template<>
    struct type_to_info<float>
    {
        static constexpr d_type_info16 value =
            D_TYPE_FLOAT_();
    };

    template<>
    struct type_to_info<double>
    {
        static constexpr d_type_info16 value =
            D_TYPE_DOUBLE_();
    };

    template<>
    struct type_to_info<long double>
    {
        static constexpr d_type_info16 value =
            D_TYPE_LDOUBLE_();
    };

NS_END  // internal

// type_info_for
//   type trait: resolves the d_type_info16 for a given
// C++ type.  Returns 0 for unrecognized types.
template<typename _Type>
struct type_info_for
{
    static constexpr d_type_info16 value =
        internal::type_to_info<_Type>::value;
};

template<typename _Type>
inline constexpr d_type_info16 type_info_for_v =
    type_info_for<_Type>::value;


NS_END  // binary
NS_END  // djinterp


#endif  // DJINTERP_BINARY_