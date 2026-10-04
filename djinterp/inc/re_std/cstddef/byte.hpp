/******************************************************************************
* djinterp [re_std]                                                     byte.hpp
*
* the byte type:
*   A distinct type for raw storage. It is neither a character type nor an
* arithmetic type: `byte` supports the bit operations and nothing else, so
* a buffer of bytes cannot silently take part in arithmetic or be printed
* as text the way an `unsigned char*` can.
*
*   RE_STD OWNS THIS TYPE -- IT IS NOT A RE-EXPORT:
*   Unlike size_t or nullptr_t, byte is an ordinary scoped enumeration with
* no compiler magic behind it, so re_std defines its own and back-ports it
* from C++17 to C++11 -- a six-year lead over std. The consequence is
* deliberate and worth stating plainly: on C++17 and later re_std::byte and
* std::byte are DIFFERENT TYPES. Neither converts implicitly to the other.
* A codebase should pick one spelling, as it would for re_std::any.
*
*   Re-exporting std::byte on C++17+ and defining an own type below it was
* the alternative, and it was rejected: it would give the same spelling two
* different identities depending on the tier, which is the exact failure
* mode the library exists to avoid.
*
*   WHY unsigned char AS THE UNDERLYING TYPE:
*   [cstddef.syn] fixes it. unsigned char is the one type guaranteed to
* have no padding bits and no trap representations, so every bit pattern
* is a valid value -- which is what makes byte usable for object
* representations at all.
*
*   C++11 FLOOR:
*   The interface IS a scoped enumeration; there is no C++98 spelling that
* keeps byte from converting to int. An unscoped enum would convert, a
* class wrapper would not be usable in a switch or as a non-type template
* argument, and either would be a different type from the one the standard
* describes. The header gates itself out below C++11 rather than ship a
* look-alike.
*
*   ONE SPELLING DOES NOT BACK-PORT -- byte{0x2A}:
*   Direct-list-initialising a scoped enum from an integer is P0138R2, a
* C++17 LANGUAGE change. The type back-ports to C++11 cleanly; the brace
* syntax cannot, because on C++11/14 the compiler rejects the narrowing
* from int to the enumeration before any library code is consulted. On
* those tiers construct with a cast, which is constexpr at every tier:
*
*       byte b = static_cast<byte>(0x2A);        // C++11 and up
*       byte b{0x2A};                            // C++17 and up
*       byte z{};                                // value-init: any tier
*
*   No to_byte() factory is provided to paper over this. The standard has
* no such function, and inventing surface std does not have would make
* re_std code non-portable in the other direction. The limitation is a
* language ceiling, recorded as such in the coverage entry.
*
*
* path:      /inc/djinterp/re_std/cstddef/byte.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDDEF_BYTE_
#define DJINTERP_RE_STD_CSTDDEF_BYTE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD

    // byte
    //   enum: distinct type for raw object storage. Scoped, so it never
    // converts implicitly to an integer -- to_integer<T> is the only way
    // out, and it must be asked for by name.
    enum class byte : unsigned char
    {};

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDDEF_BYTE_
