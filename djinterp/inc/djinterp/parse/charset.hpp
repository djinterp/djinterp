/*******************************************************************************
* djinterp [parse]                                                  charset.hpp
*
*   The C++ face of the character class declared in charset.h.
*   `charset` derives from d_parse_charset, adds no data member, and is
* asserted layout-identical and trivially copyable -- so it is passed to a C
* entry point by slicing to its base, stored in a pool as its own 32 bytes, and
* compared with memcmp, all at no cost.
*
*   Membership and the two single-value mutators are constexpr, which is what
* lets a class be built and queried at compile time -- the first piece of the
* compile-time-parser door that the POD instruction opens the rest of.
*
* path:      /inc/djinterp/parse/charset.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

#ifndef DJINTERP_PARSE_CHARSET_HPP_
#define DJINTERP_PARSE_CHARSET_HPP_ 1

// std
#include <cstddef>              // std::size_t
#include <cstdint>              // std::uint8_t, std::uint32_t
#include <type_traits>          // std::is_standard_layout
// djinterp
#include "../djinterp.hpp"      // framework root
#include "./charset.h"          // the C type this layer faces


// D_KEYWORD_PARSE
//   keyword: resolves to `parse`.  Guarded rather than owned -- parse.hpp is
// its canonical home, and this spelling only fires when the substrate is built
// without it.
#ifndef D_KEYWORD_PARSE
    #define D_KEYWORD_PARSE             parse
#endif

// NS_PARSE
//   namespace: the parse subsystem namespace.  Guarded for the same reason.
#ifndef NS_PARSE
    #define NS_PARSE                    D_NAMESPACE(D_KEYWORD_PARSE)
#endif


NS_DJINTERP
NS_PARSE


// charset
//   struct: a set of byte values, as 256 bits.  Adds constructors and named
// operations to d_parse_charset and nothing else.
struct charset : d_parse_charset
{
    // charset
    //   constructor: the empty set.
    constexpr charset() noexcept
        : d_parse_charset{}
    {}

    // charset
    //   constructor: adopts a C set unchanged.
    constexpr charset(
        const d_parse_charset& _set
    ) noexcept
        : d_parse_charset(_set)
    {}

    // of
    //   function: the set a class notation body denotes -- the text between
    // the brackets, with `^` negating and `a-z` forming a range.  Returns the
    // empty set on a malformed spec, which a frontend should have diagnosed
    // before reaching here.
    static charset
    of(
        const char* _spec
    ) noexcept
    {
        charset built;

        if (d_parse_charset_parse(&built, _spec) != 0)
        {
            d_parse_charset_clear(&built);
        }

        return built;
    }

    // range
    //   function: the set of one inclusive range of byte values.
    static charset
    range(
        unsigned char _low,
        unsigned char _high
    ) noexcept
    {
        charset built;

        d_parse_charset_add_range(&built, _low, _high);

        return built;
    }

    // test
    //   accessor: whether a byte value is in the set.
    constexpr bool
    test(
        unsigned char _value
    ) const noexcept
    {
        return ((bits[_value >> 3] >> (_value & 7u)) & 1u) != 0u;
    }

    // add
    //   function: adds a byte value.
    constexpr charset&
    add(
        unsigned char _value
    ) noexcept
    {
        bits[_value >> 3] |= static_cast<std::uint8_t>(1u << (_value & 7u));

        return *this;
    }

    // remove
    //   function: removes a byte value.
    constexpr charset&
    remove(
        unsigned char _value
    ) noexcept
    {
        bits[_value >> 3] &= static_cast<std::uint8_t>(~(1u << (_value & 7u)));

        return *this;
    }

    // add_range
    //   function: adds an inclusive range of byte values.
    charset&
    add_range(
        unsigned char _low,
        unsigned char _high
    ) noexcept
    {
        d_parse_charset_add_range(this, _low, _high);

        return *this;
    }

    // negate
    //   function: replaces the set with its complement.
    charset&
    negate() noexcept
    {
        d_parse_charset_negate(this);

        return *this;
    }

    // unite
    //   function: adds every member of another set.
    charset&
    unite(
        const d_parse_charset& _other
    ) noexcept
    {
        d_parse_charset_unite(this, &_other);

        return *this;
    }

    // intersect
    //   function: removes every value not also in another set.
    charset&
    intersect(
        const d_parse_charset& _other
    ) noexcept
    {
        d_parse_charset_intersect(this, &_other);

        return *this;
    }

    // subtract
    //   function: removes every value that is in another set.
    charset&
    subtract(
        const d_parse_charset& _other
    ) noexcept
    {
        d_parse_charset_subtract(this, &_other);

        return *this;
    }

    // count
    //   accessor: how many byte values are members.  Density, for choosing an
    // encoding.
    std::uint32_t
    count() const noexcept
    {
        return d_parse_charset_count(this);
    }

    // contiguous
    //   accessor: whether the set is exactly one non-empty run, and which.
    // A set that passes compiles to two compares instead of a table load.
    bool
    contiguous(
        unsigned char* _low  = nullptr,
        unsigned char* _high = nullptr
    ) const noexcept
    {
        return (d_parse_charset_range(this, _low, _high) != 0);
    }

    // disjoint
    //   accessor: whether this set shares no member with another.  The cheap
    // sufficient condition for compiling an ordered choice as a dispatch.
    bool
    disjoint(
        const d_parse_charset& _other
    ) const noexcept
    {
        return (d_parse_charset_disjoint(this, &_other) != 0);
    }

    // render
    //   function: writes the canonical class notation into a caller buffer.
    std::size_t
    render(
        char*       _out,
        std::size_t _size
    ) const noexcept
    {
        return d_parse_charset_render(this, _out, _size);
    }

    // operator==
    //   function: whether two sets have the same members.
    bool
    operator==(
        const d_parse_charset& _other
    ) const noexcept
    {
        return (d_parse_charset_equal(this, &_other) != 0);
    }

    // operator!=
    //   function: the negation of operator==.
    bool
    operator!=(
        const d_parse_charset& _other
    ) const noexcept
    {
        return !(*this == _other);
    }
};


//   The claim this header makes is that its type costs nothing over the C one.
// These assertions are that claim, checked.
static_assert(sizeof(charset) == sizeof(d_parse_charset),
              "parse::charset must add no data member");
static_assert(alignof(charset) == alignof(d_parse_charset),
              "parse::charset must add no data member");
static_assert(std::is_standard_layout<charset>::value,
              "parse::charset must remain standard-layout");
static_assert(std::is_trivially_copyable<charset>::value,
              "parse::charset must remain trivially copyable, since a pool "
              "stores it as its own bytes");


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_CHARSET_HPP_
