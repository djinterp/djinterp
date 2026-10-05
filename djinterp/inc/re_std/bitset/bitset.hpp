/*******************************************************************************
* djinterp [re_std]                                                   bitset.hpp
*
* bitset header:
*   A fixed-size sequence of N bits with the full standard operator
* surface: test / set / reset / flip, the bitwise operators, shifts,
* and the all / any / none / count observers.
*
*     bitset<8> b(0xA5ull);
*     b.count();        // 4
*     b[0];             // true
*     (b << 1).to_ulong();
*
*   WHAT IS DELIBERATELY MISSING, AND WHY:
*   std::bitset has three members that traffic in std::string --
* the string constructor, to_string, and the stream operators. re_std
* has no <string>, so those cannot be provided without either pulling
* in a dependency that does not exist or inventing a substitute API.
* Neither is acceptable, so they are ABSENT rather than approximated:
*
*     bitset(const char*)      -- PROVIDED (needs no string type)
*     bitset(const string&)    -- absent until <string> ships
*     to_string()              -- absent until <string> ships
*     operator<< / >>          -- absent until <ostream> ships
*
*   The char-pointer constructor covers the common case, so a caller
* can still build a bitset from a literal. Everything else waits.
*
*   STORAGE:
*   An array of unsigned long long words, ceil(N / 64) of them. A
* zero-length bitset still allocates one word, because a zero-length
* array is ill-formed and every observer then needs a special case;
* one wasted word is cheaper than that.
*
*   THE UNUSED HIGH BITS OF THE LAST WORD ARE ALWAYS ZERO.
*   Every mutating operation re-trims them. This is load-bearing, not
* tidiness: count(), any() and operator== all read whole words, so a
* stray bit above position N-1 would make a bitset compare unequal to
* itself after a flip, or report a count that is too high. trim_() is
* called at the end of flip, <<=, >>= and set().
*
*   reference:
*   operator[] on a non-const bitset returns a PROXY, not a bool&,
* because a bit is not addressable. The proxy holds the owning bitset
* and the index, and its operator= writes back.
*
*   PORTABILITY:
*   std::bitset is C++98; re_std requires C++11 for unsigned long long
* and the constexpr surface. Most const observers are constexpr from
* C++11; the mutators are constexpr from C++14, where a constexpr
* function may contain statements. std did not make bitset constexpr
* until C++23, so re_std is ahead here.
*
*
* path:      /inc/re_std/bitset/bitset.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_BITSET_BITSET_HPP
#define RE_STD_BITSET_BITSET_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#if !RE_STD_HAS_EXCEPTIONS
    #include <cstdlib>  // std::abort, a bad character without exceptions
#endif

// re_std
#include "../type_traits/integral_constant.hpp"
#include "../stdexception/out_of_range.hpp"
#include "../stdexception/invalid_argument.hpp"


// RE_STD_INTERNAL_BITSET_THROW
//   macro (internal): every bitset error, as an expression: throw EXC(MSG)
// where exceptions are on, as std's bitset does; std::abort() where they are
// off, as std's does then (the choice function.hpp makes). A macro, not a
// function, so the throw stays usable inside a C++11 constexpr expression.
#if RE_STD_HAS_EXCEPTIONS
    #define RE_STD_INTERNAL_BITSET_THROW(EXC, MSG)   throw EXC(MSG)
#else
    #define RE_STD_INTERNAL_BITSET_THROW(EXC, MSG)   std::abort()
#endif  // RE_STD_HAS_EXCEPTIONS


namespace re_std
{


// ===========================================================================
// 0.   COMPATIBILITY
// ===========================================================================


// ===========================================================================
// I.   BITSET
// ===========================================================================

// bitset
//   class: a fixed sequence of N bits packed into unsigned long long
// words. See the header note on the trimming invariant.
template<std::size_t N>
class bitset
{
private:
    typedef unsigned long long _word_t;

    static const std::size_t s_word_bits = 64;
    // ceil division; the max(...,1) keeps a zero-length bitset legal
    static const std::size_t s_words =
        (N + s_word_bits - 1) / s_word_bits > 0
            ? (N + s_word_bits - 1) / s_word_bits
            : 1;

    _word_t m_w[s_words];

    // trim_
    //   function: clears the bits above position N-1 in the top word.
    // Every mutator ends with this -- count(), any() and operator==
    // read whole words, so a stray high bit corrupts all three.
    RE_STD_CONSTEXPR_CPP14 void trim_()
    {
        const std::size_t _used = N % s_word_bits;
        if (_used != 0)
        {
            m_w[s_words - 1] &=
                static_cast<_word_t>( (static_cast<_word_t>(1) << _used)
                                      - static_cast<_word_t>(1) );
        }
    }

    static RE_STD_CONSTEXPR int popcount_(_word_t _v, int _acc = 0)
    {
        return (_v == 0)
            ? _acc
            : popcount_(static_cast<_word_t>(_v >> 1),
                        _acc + static_cast<int>(_v & 1ull));
    }

public:
    // -----------------------------------------------------------------
    // reference proxy
    // -----------------------------------------------------------------

    // reference
    //   class: proxy returned by non-const operator[]. A bit has no
    // address, so a bool& is impossible; the proxy stores the owner and
    // the index and writes back through operator=.
    class reference
    {
    private:
        bitset*     m_owner;
        std::size_t m_pos;

    public:
        RE_STD_CONSTEXPR_CPP14 reference(bitset& _b, std::size_t _p)
            : m_owner(&_b), m_pos(_p)
        {}

        RE_STD_CONSTEXPR_CPP14 reference& operator=(bool _v)
        {
            m_owner->set(m_pos, _v);
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 reference& operator=(const reference& _o)
        {
            m_owner->set(m_pos, static_cast<bool>(_o));
            return *this;
        }

        RE_STD_CONSTEXPR operator bool() const
        {
            return m_owner->test_unchecked_(m_pos);
        }

        RE_STD_CONSTEXPR bool operator~() const
        {
            return !m_owner->test_unchecked_(m_pos);
        }

        RE_STD_CONSTEXPR_CPP14 reference& flip()
        {
            m_owner->flip(m_pos);
            return *this;
        }
    };

    // test_unchecked_
    //   function: public only because `reference` needs it; performs no
    // bounds check, unlike test().
    RE_STD_CONSTEXPR bool test_unchecked_(std::size_t _pos) const
    {
        return ( (m_w[_pos / s_word_bits] >> (_pos % s_word_bits)) & 1ull )
               != 0ull;
    }

    // -----------------------------------------------------------------
    // construction
    // -----------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 bitset()
        : m_w()
    {}

    // bitset(unsigned long long)
    //   function: low-order bits of _v. Bits above N are discarded, and
    // bits above 64 are zero when N exceeds a word.
    RE_STD_CONSTEXPR_CPP14 bitset(unsigned long long _v)
        : m_w()
    {
        m_w[0] = static_cast<_word_t>(_v);
        trim_();
    }

    // bitset(const char*)
    //   function: parses '0'/'1', leftmost character is the HIGHEST index,
    // matching std's string constructor. Throws invalid_argument on any
    // other character. The std::string overload is absent -- see the
    // header note.
    RE_STD_CONSTEXPR_CPP14 explicit bitset(const char* _s)
        : m_w()
    {
        std::size_t _len = 0;
        while (_s[_len] != '\0') { ++_len; }
        for (std::size_t _i = 0; _i < _len; ++_i)
        {
            const char _c = _s[_len - 1 - _i];
            if (_c != '0' && _c != '1')
            {
                RE_STD_INTERNAL_BITSET_THROW(invalid_argument,
                                        "re_std::bitset: character is not '0' or '1'");
            }
            if (_i < N && _c == '1') { set(_i, true); }
        }
    }

    // -----------------------------------------------------------------
    // observers
    // -----------------------------------------------------------------

    RE_STD_CONSTEXPR std::size_t size() const { return N; }

    RE_STD_CONSTEXPR bool operator[](std::size_t _pos) const
    {
        return test_unchecked_(_pos);
    }

    RE_STD_CONSTEXPR_CPP14 reference operator[](std::size_t _pos)
    {
        return reference(*this, _pos);
    }

    // test
    //   function: bounds-checked read. Throws out_of_range, which is what
    // separates it from operator[].
    RE_STD_CONSTEXPR bool test(std::size_t _pos) const
    {
        return (_pos >= N)
            ? (RE_STD_INTERNAL_BITSET_THROW(out_of_range,
                                       "re_std::bitset::test: position out of range"),
               false)
            : test_unchecked_(_pos);
    }

    RE_STD_CONSTEXPR_CPP14 std::size_t count() const
    {
        std::size_t _n = 0;
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            _n += static_cast<std::size_t>(popcount_(m_w[_i]));
        }
        return _n;
    }

    RE_STD_CONSTEXPR_CPP14 bool any() const
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            if (m_w[_i] != 0) { return true; }
        }
        return false;
    }

    RE_STD_CONSTEXPR_CPP14 bool none() const { return !any(); }
    RE_STD_CONSTEXPR_CPP14 bool all()  const { return count() == N; }

    // -----------------------------------------------------------------
    // mutators
    // -----------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 bitset& set()
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            m_w[_i] = ~static_cast<_word_t>(0);
        }
        trim_();
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset& set(std::size_t _pos, bool _v = true)
    {
        if (_pos >= N)
        {
            RE_STD_INTERNAL_BITSET_THROW(out_of_range,
                                    "re_std::bitset::set: position out of range");
        }
        const _word_t _bit =
            static_cast<_word_t>(static_cast<_word_t>(1) << (_pos % s_word_bits));
        if (_v) { m_w[_pos / s_word_bits] |=  _bit; }
        else    { m_w[_pos / s_word_bits] &= static_cast<_word_t>(~_bit); }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset& reset()
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] = 0; }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset& reset(std::size_t _pos)
    {
        return set(_pos, false);
    }

    RE_STD_CONSTEXPR_CPP14 bitset& flip()
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            m_w[_i] = static_cast<_word_t>(~m_w[_i]);
        }
        trim_();
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset& flip(std::size_t _pos)
    {
        return set(_pos, !test(_pos));
    }

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    // to_ullong
    //   function: throws overflow_error if any bit at or above 64 is set,
    // per [bitset.members]. to_string is absent -- see the header note.
    RE_STD_CONSTEXPR_CPP14 unsigned long long to_ullong() const
    {
        for (std::size_t _i = 1; _i < s_words; ++_i)
        {
            if (m_w[_i] != 0)
            {
                RE_STD_INTERNAL_BITSET_THROW(out_of_range,
                                        "re_std::bitset::to_ullong: value does not fit");
            }
        }
        return m_w[0];
    }

    RE_STD_CONSTEXPR_CPP14 unsigned long to_ulong() const
    {
        const unsigned long long _v = to_ullong();
        if (_v > static_cast<unsigned long long>(
                     static_cast<unsigned long>(-1)))
        {
            RE_STD_INTERNAL_BITSET_THROW(out_of_range,
                                    "re_std::bitset::to_ulong: value does not fit");
        }
        return static_cast<unsigned long>(_v);
    }

    // -----------------------------------------------------------------
    // bitwise
    // -----------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 bitset& operator&=(const bitset& _o)
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] &= _o.m_w[_i]; }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset& operator|=(const bitset& _o)
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] |= _o.m_w[_i]; }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset& operator^=(const bitset& _o)
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] ^= _o.m_w[_i]; }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset operator~() const
    {
        bitset _r(*this);
        _r.flip();
        return _r;
    }

    // operator<<=
    //   function: shifts toward higher indices. Implemented bit-wise
    // rather than word-wise: correctness first, and a bitset is rarely
    // the hot path.
    RE_STD_CONSTEXPR_CPP14 bitset& operator<<=(std::size_t _s)
    {
        if (_s >= N) { return reset(); }
        for (std::size_t _i = N; _i-- > 0; )
        {
            set(_i, (_i >= _s) ? test_unchecked_(_i - _s) : false);
        }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset& operator>>=(std::size_t _s)
    {
        if (_s >= N) { return reset(); }
        for (std::size_t _i = 0; _i < N; ++_i)
        {
            set(_i, (_i + _s < N) ? test_unchecked_(_i + _s) : false);
        }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 bitset operator<<(std::size_t _s) const
    {
        bitset _r(*this); _r <<= _s; return _r;
    }

    RE_STD_CONSTEXPR_CPP14 bitset operator>>(std::size_t _s) const
    {
        bitset _r(*this); _r >>= _s; return _r;
    }

    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 bool operator==(const bitset& _o) const
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            if (m_w[_i] != _o.m_w[_i]) { return false; }
        }
        return true;
    }

    RE_STD_CONSTEXPR_CPP14 bool operator!=(const bitset& _o) const
    {
        return !(*this == _o);
    }
};


// ===========================================================================
// II.  FREE BITWISE OPERATORS
// ===========================================================================

template<std::size_t N>
RE_STD_CONSTEXPR_CPP14 bitset<N>
operator&(const bitset<N>& _a, const bitset<N>& _b)
{
    bitset<N> _r(_a); _r &= _b; return _r;
}

template<std::size_t N>
RE_STD_CONSTEXPR_CPP14 bitset<N>
operator|(const bitset<N>& _a, const bitset<N>& _b)
{
    bitset<N> _r(_a); _r |= _b; return _r;
}

template<std::size_t N>
RE_STD_CONSTEXPR_CPP14 bitset<N>
operator^(const bitset<N>& _a, const bitset<N>& _b)
{
    bitset<N> _r(_a); _r ^= _b; return _r;
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BITSET_BITSET_HPP
