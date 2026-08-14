/******************************************************************************
* re_std [ios]                                                  stream_types.hpp
*
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
* path:      /inc/djinterp/re_std/ios/stream_types.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_IOS_STREAM_TYPES_
#define RESTD_IOS_STREAM_TYPES_ 1

#include "../../djinterp.hpp"
#include "../type_traits/type_traits.hpp"

//   mbstate_t is a C library type; <cwchar> is the portable spelling and is
// available in C++98.
#include <cwchar>

NS_DJINTERP
NS_RESTD

// streamoff
//   typedef: a signed offset in a stream, wide enough for the largest file
// the platform supports.
#if D_ENV_HAS_LONG_LONG
    //   D_ENV_HAS_LONG_LONG is 1 on every mainstream compiler even at C++98,
    // where `long long` is an extension rather than a standard type - so
    // -Wpedantic emits "ISO C++ 1998 does not support 'long long'" here.
    // Suppressed locally, as in numeric/saturation_arith.hpp.
    //
    //   TODO(djinterp): this is now the THIRD file carrying its own copy of
    // this block (saturation_arith.hpp, the numeric test stub, and here), and
    // every header that names long long will need it.  It belongs in
    // djinterp.hpp as a shared D_LONG_LONG_DIAG_PUSH / D_LONG_LONG_DIAG_POP
    // pair - see patches/ for the NS_END precedent.
    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wlong-long"
    #endif

    typedef long long streamoff;

    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic pop
    #endif
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
template<typename _StateT>
class fpos
{
    streamoff m_offset;
    _StateT   m_state;

public:
    fpos() : m_offset(0), m_state() {}
    fpos(streamoff off) : m_offset(off), m_state() {}

    operator streamoff() const { return m_offset; }

    _StateT state() const        { return m_state; }
    void    state(_StateT value) { m_state = value; return; }

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
    template<typename _Int>
    typename enable_if<is_integral<_Int>::value, fpos>::type
    operator+(_Int off) const
    { fpos tmp(*this); tmp += static_cast<streamoff>(off); return tmp; }

    template<typename _Int>
    typename enable_if<is_integral<_Int>::value, fpos>::type
    operator-(_Int off) const
    { fpos tmp(*this); tmp -= static_cast<streamoff>(off); return tmp; }

    //   Position minus position is a DISTANCE, not a position.  There is
    // deliberately no operator+ between two fpos values.
    streamoff operator-(const fpos& other) const
    { return m_offset - other.m_offset; }
};

template<typename _StateT>
bool operator==(const fpos<_StateT>& a, const fpos<_StateT>& b)
{ return static_cast<streamoff>(a) == static_cast<streamoff>(b); }

template<typename _StateT>
bool operator!=(const fpos<_StateT>& a, const fpos<_StateT>& b)
{ return !(a == b); }

typedef fpos<std::mbstate_t> streampos;
typedef fpos<std::mbstate_t> wstreampos;
typedef fpos<std::mbstate_t> u16streampos;
typedef fpos<std::mbstate_t> u32streampos;

NS_END
NS_END

#endif  // RESTD_IOS_STREAM_TYPES_
