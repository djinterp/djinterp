/******************************************************************************
* re_std [ios]                                                     ios_base.hpp
*
*   ios_base - the non-template base of every stream: format flags, stream
* state, and the open-mode and seek-direction enumerations.
*
*   THE FOUR TYPES ARE BITMASK TYPES, NOT ENUMS, and the difference is
* observable.  [bitmask.types] requires that |, &, ^, ~, |=, &=, ^= all work
* and all yield the bitmask type again.  A plain `enum` does not: `in | out`
* would decay to int and `mode & openmode::binary` would not compile against a
* parameter of the enum type.  A scoped `enum class` does not either, without
* the operators.  So each one is a distinct type with a full operator set -
* which is exactly what lets `ios_base::in | ios_base::binary` be passed where
* an openmode is expected.
*
*   WHY NOT enum class HERE.  std specifies these as unspecified bitmask types
* usable unqualified as `ios_base::in`, and legacy code passes them as ints in
* places.  A scoped enumeration would break that spelling.  re_std matches
* std's observable interface rather than modernising it, which is the whole
* premise of the library.
*
*   setf(flags, mask) IS NOT setf(flags).  The two-argument form CLEARS the
* mask first and then sets - it is how you switch between mutually exclusive
* alternatives like dec/oct/hex, which live in one basefield group.  The
* one-argument form only ORs in.  Calling the wrong one leaves two base flags
* set at once, and the resulting format is unspecified.  This trips people up
* often enough to be worth stating here.
*
*   DEFERRED, DELIBERATELY, three things - all tracked on the roadmap rather
* than stubbed out with something that merely looks like it works:
*     imbue() / getloc()             need <locale>
*     iword / pword / callbacks      need a growable per-stream array
*     ios_base::failure              needs re_std::string (and, from C++11,
*                                    system_error) - a const char*-only
*                                    stand-in would not be the type std
*                                    specifies, so catch clauses would still
*                                    not match a real implementation
*   Everything streambuf, istream and ostream actually need is present.
*
*   STD IS C++98; re_std IS C++98.
*
*
* path:      /inc/djinterp/re_std/ios/ios_base.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_IOS_IOS_BASE_
#define DJINTERP_RE_STD_IOS_IOS_BASE_ 1

// re_std
#include "../../core/djinterp.hpp"
#include "../type_traits/type_traits.hpp"
#include "./stream_types.hpp"

NS_RESTD

// ios_base
//   class: format state and the stream-wide enumerations.
class ios_base
{
public:
    // ---- bitmask types -----------------------------------------------
    //   Each is a distinct type with a full operator set, so that
    // `ios_base::in | ios_base::binary` yields an openmode rather than an int.

    typedef unsigned int fmtflags;
    typedef unsigned int iostate;
    typedef unsigned int openmode;
    typedef unsigned int seekdir;

    // format flags
    static const fmtflags boolalpha   = 0x0001;
    static const fmtflags dec         = 0x0002;
    static const fmtflags fixed       = 0x0004;
    static const fmtflags hex         = 0x0008;
    static const fmtflags internal    = 0x0010;
    static const fmtflags left        = 0x0020;
    static const fmtflags oct         = 0x0040;
    static const fmtflags right       = 0x0080;
    static const fmtflags scientific  = 0x0100;
    static const fmtflags showbase    = 0x0200;
    static const fmtflags showpoint   = 0x0400;
    static const fmtflags showpos     = 0x0800;
    static const fmtflags skipws      = 0x1000;
    static const fmtflags unitbuf     = 0x2000;
    static const fmtflags uppercase   = 0x4000;

    //   The three GROUPS. These exist so setf(flag, group) can clear the
    // whole group before setting, which is the only correct way to switch
    // between mutually exclusive alternatives - see the header note.
    static const fmtflags adjustfield = left | right | internal;
    static const fmtflags basefield   = dec | oct | hex;
    static const fmtflags floatfield  = scientific | fixed;

    // stream state
    static const iostate goodbit = 0x0;
    static const iostate badbit  = 0x1;
    static const iostate eofbit  = 0x2;
    static const iostate failbit = 0x4;

    // open mode
    static const openmode app    = 0x01;
    static const openmode ate    = 0x02;
    static const openmode binary = 0x04;
    static const openmode in     = 0x08;
    static const openmode out    = 0x10;
    static const openmode trunc  = 0x20;

    // seek direction
    static const seekdir beg = 0;
    static const seekdir cur = 1;
    static const seekdir end = 2;

    // ---- format state ------------------------------------------------
    fmtflags flags() const { return m_flags; }

    fmtflags flags(fmtflags value)
    {
        const fmtflags old = m_flags;
        m_flags = value;
        return old;
    }

    //   OR-only. Use the two-argument form to switch within a group.
    fmtflags setf(fmtflags value)
    {
        const fmtflags old = m_flags;
        m_flags |= value;
        return old;
    }

    //   Clears `mask` first, THEN sets the bits of `value` within it. This is
    // the form that makes dec/oct/hex mutually exclusive.
    fmtflags setf(fmtflags value, fmtflags mask)
    {
        const fmtflags old = m_flags;
        m_flags &= static_cast<fmtflags>(~mask);
        m_flags |= (value & mask);
        return old;
    }

    void unsetf(fmtflags mask)
    {
        m_flags &= static_cast<fmtflags>(~mask);
        return;
    }

    streamsize precision() const { return m_precision; }

    streamsize precision(streamsize value)
    {
        const streamsize old = m_precision;
        m_precision = value;
        return old;
    }

    streamsize width() const { return m_width; }

    streamsize width(streamsize value)
    {
        const streamsize old = m_width;
        m_width = value;
        return old;
    }

    //   ios_base::failure IS DEFERRED, not forgotten. Its constructors take
    // a `const string&`, and C++11 re-based it on system_error - so it needs
    // re_std::string, which does not exist yet (<string> is roadmap 49). A
    // `const char*`-only stand-in would compile but would not be the type std
    // specifies, and code catching ios_base::failure would still not catch
    // what a real implementation throws. Tracked rather than faked; nothing
    // in streambuf, istream or ostream needs it to exist.

protected:
    //   Protected per std: ios_base is a base class, never instantiated
    // directly. The defaults are std's: skipws | dec, precision 6, width 0.
    ios_base()
        : m_flags(skipws | dec), m_precision(6), m_width(0)
    {}

    ~ios_base() {}

private:
    //   Non-copyable, per std - a stream's state belongs to the stream.
    ios_base(const ios_base&);
    ios_base& operator=(const ios_base&);

    fmtflags   m_flags;
    streamsize m_precision;
    streamsize m_width;
};

NS_END  // re_std
#endif  // DJINTERP_RE_STD_IOS_IOS_BASE_
