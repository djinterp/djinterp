/*******************************************************************************
* djinterp [re_std]                                             stream_types.hpp
*
* stream_types support header:
*   the stream position and size types: streamoff, streamsize, fpos<StateT>,
* and the streampos family.
*
*   WHY fpos IS A CLASS AND NOT AN INTEGER.
*   A byte offset is not enough to describe a position in a multibyte or
* stateful encoding: resuming mid-stream needs the CONVERSION STATE as well -
* which shift state a shift-JIS or ISO-2022 stream was in, how much of a
* multi-unit sequence had been consumed.  fpos carries an offset AND a
* mbstate_t, which is why seekpos takes an fpos and seekoff takes a bare
* streamoff.  Collapsing them would silently break every stateful encoding.
*
*   THE ARITHMETIC IS DELIBERATELY ASYMMETRIC, and it is not an oversight:
*     fpos +/- streamoff  ->  fpos          (move within the stream)
*     fpos  -  fpos       ->  streamoff     (distance between positions)
*     fpos  +  fpos       ->  does not exist
*   Adding two positions is meaningless, so std does not define it.  This is
* the same shape as pointer arithmetic and for the same reason.
*
*   streamsize IS SIGNED.  It looks like it should be unsigned - it is a count
* - but it must be able to express -1 as a failure return from xsgetn and
* friends, and it must be comparable against streamoff without a signedness
* conversion.  std makes it signed; re_std follows rather than "improving" it.
*
*   STD IS C++98; re_std IS C++98.
*
*
* path:      /inc/re_std/ios/stream_types.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_IOS_STREAM_TYPES_HPP
#define RE_STD_IOS_STREAM_TYPES_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
//   mbstate_t is a C library type; <cwchar> is the portable spelling and is
// available in C++98.
#include <cwchar>
// re_std
#include "../type_traits/type_traits.hpp"

namespace re_std
{

// streamoff
//   typedef: a signed offset in a stream, wide enough for the largest file
// the platform supports.
#if RE_STD_HAS_LONG_LONG
    //   RE_STD_HAS_LONG_LONG is 1 on every mainstream compiler even at C++98,
    // where `long long` is an extension rather than a standard type - so
    // -Wpedantic emits "ISO C++ 1998 does not support 'long long'" here.
    // config.hpp's shared pair suppresses it where it applies.
    RE_STD_LONG_LONG_DIAG_PUSH
    typedef long long streamoff;
    RE_STD_LONG_LONG_DIAG_POP
#else
    typedef long      streamoff;
#endif

// streamsize
//   typedef: a signed count of characters.  Signed on purpose - see the
// header note.
typedef streamoff streamsize;

// fpos
//   class: a stream position - an offset plus the conversion state needed to
// resume decoding there.
template<typename StateT>
class fpos
{
    streamoff m_offset;
    StateT   m_state;

public:
    fpos() : m_offset(0), m_state() {}
    fpos(streamoff off) : m_offset(off), m_state() {}

    operator streamoff() const { return m_offset; }

    StateT state() const        { return m_state; }
    void    state(StateT value) { m_state = value; return; }

    fpos& operator+=(streamoff off) { m_offset += off; return *this; }
    fpos& operator-=(streamoff off) { m_offset -= off; return *this; }

    //   THE OFFSET PARAMETER IS A TEMPLATE, and it has to be.  fpos converts
    // implicitly to streamoff, so a non-template `operator+(streamoff)` loses
    // to the BUILT-IN `operator+(long long, int)` for an expression as
    // ordinary as `pos + 50`: the member needs an integral conversion on the
    // argument while the built-in needs one on the object, so neither is
    // better and the call is AMBIGUOUS.  Taking the offset as a deduced
    // integral makes both arguments exact matches, and the member wins
    // outright.  Constrained to integral types so it cannot swallow anything
    // else.
    template<typename Int>
    typename enable_if<is_integral<Int>::value, fpos>::type
    operator+(Int off) const
    { fpos tmp(*this); tmp += static_cast<streamoff>(off); return tmp; }

    template<typename Int>
    typename enable_if<is_integral<Int>::value, fpos>::type
    operator-(Int off) const
    { fpos tmp(*this); tmp -= static_cast<streamoff>(off); return tmp; }

    //   Position minus position is a DISTANCE, not a position.  There is
    // deliberately no operator+ between two fpos values.
    streamoff operator-(const fpos& other) const
    { return m_offset - other.m_offset; }
};

template<typename StateT>
bool operator==(const fpos<StateT>& a, const fpos<StateT>& b)
{ return static_cast<streamoff>(a) == static_cast<streamoff>(b); }

template<typename StateT>
bool operator!=(const fpos<StateT>& a, const fpos<StateT>& b)
{ return !(a == b); }

typedef fpos<std::mbstate_t> streampos;
typedef fpos<std::mbstate_t> wstreampos;
typedef fpos<std::mbstate_t> u16streampos;
typedef fpos<std::mbstate_t> u32streampos;

}

#endif  // floor, for now


#endif  // RE_STD_IOS_STREAM_TYPES_HPP
