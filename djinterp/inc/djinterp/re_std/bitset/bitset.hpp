/******************************************************************************
* djinterp [re_std]                                                   bitset.hpp
*
* bitset header:
*   A fixed-size sequence of _N bits with the full standard operator
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
* path:      /inc/djinterp/re_std/bitset/bitset.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BITSET_BITSET_
#define DJINTERP_RE_STD_BITSET_BITSET_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>

// djinterp
#include "../type_traits/integral_constant.hpp"
#include "../stdexception/out_of_range.hpp"
#include "../stdexception/invalid_argument.hpp"


NS_RESTD


// ===========================================================================
// 0.   COMPATIBILITY
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


// ===========================================================================
// I.   BITSET
// ===========================================================================

// bitset
//   class: a fixed sequence of _N bits packed into unsigned long long
// words. See the header note on the trimming invariant.
template<std::size_t _N>
class bitset
{
private:
    typedef unsigned long long _word_t;

    static const std::size_t s_word_bits = 64;
    // ceil division; the max(...,1) keeps a zero-length bitset legal
    static const std::size_t s_words =
        (_N + s_word_bits - 1) / s_word_bits > 0
            ? (_N + s_word_bits - 1) / s_word_bits
            : 1;

    _word_t m_w[s_words];

    // trim_
    //   helper: clears the bits above position _N-1 in the top word.
    // Every mutator ends with this -- count(), any() and operator==
    // read whole words, so a stray high bit corrupts all three.
    D_CONSTEXPR_CPP14 void trim_()
    {
        const std::size_t _used = _N % s_word_bits;
        if (_used != 0)
        {
            m_w[s_words - 1] &=
                static_cast<_word_t>( (static_cast<_word_t>(1) << _used)
                                      - static_cast<_word_t>(1) );
        }
    }

    static D_CONSTEXPR int popcount_(_word_t _v, int _acc = 0)
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
        D_CONSTEXPR_CPP14 reference(bitset& _b, std::size_t _p)
            : m_owner(&_b), m_pos(_p)
        {}

        D_CONSTEXPR_CPP14 reference& operator=(bool _v)
        {
            m_owner->set(m_pos, _v);
            return *this;
        }

        D_CONSTEXPR_CPP14 reference& operator=(const reference& _o)
        {
            m_owner->set(m_pos, static_cast<bool>(_o));
            return *this;
        }

        D_CONSTEXPR operator bool() const
        {
            return m_owner->test_unchecked_(m_pos);
        }

        D_CONSTEXPR bool operator~() const
        {
            return !m_owner->test_unchecked_(m_pos);
        }

        D_CONSTEXPR_CPP14 reference& flip()
        {
            m_owner->flip(m_pos);
            return *this;
        }
    };

    // test_unchecked_
    //   helper: public only because `reference` needs it; performs no
    // bounds check, unlike test().
    D_CONSTEXPR bool test_unchecked_(std::size_t _pos) const
    {
        return ( (m_w[_pos / s_word_bits] >> (_pos % s_word_bits)) & 1ull )
               != 0ull;
    }

    // -----------------------------------------------------------------
    // construction
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 bitset()
        : m_w()
    {}

    // bitset(unsigned long long)
    //   ctor: low-order bits of _v. Bits above _N are discarded, and
    // bits above 64 are zero when _N exceeds a word.
    D_CONSTEXPR_CPP14 bitset(unsigned long long _v)
        : m_w()
    {
        m_w[0] = static_cast<_word_t>(_v);
        trim_();
    }

    // bitset(const char*)
    //   ctor: parses '0'/'1', leftmost character is the HIGHEST index,
    // matching std's string constructor. Throws invalid_argument on any
    // other character. The std::string overload is absent -- see the
    // header note.
    D_CONSTEXPR_CPP14 explicit bitset(const char* _s)
        : m_w()
    {
        std::size_t _len = 0;
        while (_s[_len] != '\0') { ++_len; }
        for (std::size_t _i = 0; _i < _len; ++_i)
        {
            const char _c = _s[_len - 1 - _i];
            if (_c != '0' && _c != '1')
            {
                throw invalid_argument(
                    "re_std::bitset: character is not '0' or '1'");
            }
            if (_i < _N && _c == '1') { set(_i, true); }
        }
    }

    // -----------------------------------------------------------------
    // observers
    // -----------------------------------------------------------------

    D_CONSTEXPR std::size_t size() const { return _N; }

    D_CONSTEXPR bool operator[](std::size_t _pos) const
    {
        return test_unchecked_(_pos);
    }

    D_CONSTEXPR_CPP14 reference operator[](std::size_t _pos)
    {
        return reference(*this, _pos);
    }

    // test
    //   function: bounds-checked read. Throws out_of_range, which is what
    // separates it from operator[].
    D_CONSTEXPR bool test(std::size_t _pos) const
    {
        return (_pos >= _N)
            ? (throw out_of_range("re_std::bitset::test: position out of range"),
               false)
            : test_unchecked_(_pos);
    }

    D_CONSTEXPR_CPP14 std::size_t count() const
    {
        std::size_t _n = 0;
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            _n += static_cast<std::size_t>(popcount_(m_w[_i]));
        }
        return _n;
    }

    D_CONSTEXPR_CPP14 bool any() const
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            if (m_w[_i] != 0) { return true; }
        }
        return false;
    }

    D_CONSTEXPR_CPP14 bool none() const { return !any(); }
    D_CONSTEXPR_CPP14 bool all()  const { return count() == _N; }

    // -----------------------------------------------------------------
    // mutators
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 bitset& set()
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            m_w[_i] = ~static_cast<_word_t>(0);
        }
        trim_();
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset& set(std::size_t _pos, bool _v = true)
    {
        if (_pos >= _N)
        {
            throw out_of_range("re_std::bitset::set: position out of range");
        }
        const _word_t _bit =
            static_cast<_word_t>(static_cast<_word_t>(1) << (_pos % s_word_bits));
        if (_v) { m_w[_pos / s_word_bits] |=  _bit; }
        else    { m_w[_pos / s_word_bits] &= static_cast<_word_t>(~_bit); }
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset& reset()
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] = 0; }
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset& reset(std::size_t _pos)
    {
        return set(_pos, false);
    }

    D_CONSTEXPR_CPP14 bitset& flip()
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            m_w[_i] = static_cast<_word_t>(~m_w[_i]);
        }
        trim_();
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset& flip(std::size_t _pos)
    {
        return set(_pos, !test(_pos));
    }

    // -----------------------------------------------------------------
    // conversion
    // -----------------------------------------------------------------

    // to_ullong
    //   function: throws overflow_error if any bit at or above 64 is set,
    // per [bitset.members]. to_string is absent -- see the header note.
    D_CONSTEXPR_CPP14 unsigned long long to_ullong() const
    {
        for (std::size_t _i = 1; _i < s_words; ++_i)
        {
            if (m_w[_i] != 0)
            {
                throw out_of_range(
                    "re_std::bitset::to_ullong: value does not fit");
            }
        }
        return m_w[0];
    }

    D_CONSTEXPR_CPP14 unsigned long to_ulong() const
    {
        const unsigned long long _v = to_ullong();
        if (_v > static_cast<unsigned long long>(
                     static_cast<unsigned long>(-1)))
        {
            throw out_of_range("re_std::bitset::to_ulong: value does not fit");
        }
        return static_cast<unsigned long>(_v);
    }

    // -----------------------------------------------------------------
    // bitwise
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 bitset& operator&=(const bitset& _o)
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] &= _o.m_w[_i]; }
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset& operator|=(const bitset& _o)
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] |= _o.m_w[_i]; }
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset& operator^=(const bitset& _o)
    {
        for (std::size_t _i = 0; _i < s_words; ++_i) { m_w[_i] ^= _o.m_w[_i]; }
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset operator~() const
    {
        bitset _r(*this);
        _r.flip();
        return _r;
    }

    // operator<<=
    //   function: shifts toward higher indices. Implemented bit-wise
    // rather than word-wise: correctness first, and a bitset is rarely
    // the hot path.
    D_CONSTEXPR_CPP14 bitset& operator<<=(std::size_t _s)
    {
        if (_s >= _N) { return reset(); }
        for (std::size_t _i = _N; _i-- > 0; )
        {
            set(_i, (_i >= _s) ? test_unchecked_(_i - _s) : false);
        }
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset& operator>>=(std::size_t _s)
    {
        if (_s >= _N) { return reset(); }
        for (std::size_t _i = 0; _i < _N; ++_i)
        {
            set(_i, (_i + _s < _N) ? test_unchecked_(_i + _s) : false);
        }
        return *this;
    }

    D_CONSTEXPR_CPP14 bitset operator<<(std::size_t _s) const
    {
        bitset _r(*this); _r <<= _s; return _r;
    }

    D_CONSTEXPR_CPP14 bitset operator>>(std::size_t _s) const
    {
        bitset _r(*this); _r >>= _s; return _r;
    }

    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    D_CONSTEXPR_CPP14 bool operator==(const bitset& _o) const
    {
        for (std::size_t _i = 0; _i < s_words; ++_i)
        {
            if (m_w[_i] != _o.m_w[_i]) { return false; }
        }
        return true;
    }

    D_CONSTEXPR_CPP14 bool operator!=(const bitset& _o) const
    {
        return !(*this == _o);
    }
};


// ===========================================================================
// II.  FREE BITWISE OPERATORS
// ===========================================================================

template<std::size_t _N>
D_CONSTEXPR_CPP14 bitset<_N>
operator&(const bitset<_N>& _a, const bitset<_N>& _b)
{
    bitset<_N> _r(_a); _r &= _b; return _r;
}

template<std::size_t _N>
D_CONSTEXPR_CPP14 bitset<_N>
operator|(const bitset<_N>& _a, const bitset<_N>& _b)
{
    bitset<_N> _r(_a); _r |= _b; return _r;
}

template<std::size_t _N>
D_CONSTEXPR_CPP14 bitset<_N>
operator^(const bitset<_N>& _a, const bitset<_N>& _b)
{
    bitset<_N> _r(_a); _r ^= _b; return _r;
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BITSET_BITSET_
