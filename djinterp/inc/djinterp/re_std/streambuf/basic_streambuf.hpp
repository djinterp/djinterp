/******************************************************************************
* re_std [streambuf]                                          basic_streambuf.hpp
*
*   basic_streambuf - the buffer abstraction every stream sits on.
*
*   THE SHAPE IS SIX POINTERS, and understanding them is understanding the
* whole class:
*
*     get area:  eback() <= gptr() <= egptr()
*                 |          |         |
*                 |          |         end of readable data
*                 |          next character to read
*                 start of the putback area
*
*     put area:  pbase() <= pptr() <= epptr()
*
*   Everything public is defined in terms of those six.  A derived class
* implements underflow/overflow to refill or drain, and sets the pointers with
* setg/setp; it never touches the public interface.
*
*   THE PUBLIC/VIRTUAL SPLIT IS THE POINT OF THE DESIGN.
*   Every public operation is non-virtual and does the fast path inline -
*   sgetc() is a pointer compare and a dereference - and calls the
*   corresponding virtual ONLY when the buffer is exhausted.  That is why
*   reading a million buffered characters costs a million pointer bumps and a
*   handful of virtual calls, not a million virtual calls.  Collapsing the two
*   layers into virtuals would be simpler and roughly an order of magnitude
*   slower.
*
*   underflow VERSUS uflow, which is the one distinction people get wrong:
*     underflow()  peek  - makes a character available, does NOT consume it
*     uflow()      read  - makes one available AND consumes it
*   The default uflow() is underflow() followed by a gptr bump, so a derived
*   class normally overrides only underflow().  Overriding uflow() alone
*   leaves sgetc() broken; overriding underflow() alone is correct and
*   sufficient for any buffered source.
*
*   DEFAULT VIRTUALS ARE DELIBERATELY INERT, NOT ABSTRACT.  A default-
*   constructed basic_streambuf is a valid, permanently-empty buffer: underflow
*   returns eof, overflow returns eof, seekoff returns -1.  std specifies
*   exactly this, and it means a derived class overrides only what it actually
*   supports rather than being forced to stub out the rest.
*
*   DEFERRED, DELIBERATELY: pubimbue() and getloc() need <locale>, which
*   re_std does not have.  Tracked on the roadmap rather than faked.
*
*   STD IS C++98; re_std IS C++98.
*
*
* path:      /inc/djinterp/re_std/streambuf/basic_streambuf.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_STREAMBUF_BASIC_STREAMBUF_
#define DJINTERP_RE_STD_STREAMBUF_BASIC_STREAMBUF_ 1

// re_std
#include "../../core/djinterp.hpp"
#include "../type_traits/type_traits.hpp"
#include "../ios/ios_base.hpp"
#include "../ios/stream_types.hpp"
#include "../string_view/char_traits.hpp"

NS_RESTD

// basic_streambuf
//   class: abstract buffer over a character sequence.
template<typename _CharT, typename _Traits = char_traits<_CharT> >
class basic_streambuf
{
public:
    typedef _CharT                    char_type;
    typedef _Traits                   traits_type;
    typedef typename _Traits::int_type int_type;
    typedef typename _Traits::pos_type pos_type;
    typedef typename _Traits::off_type off_type;

    virtual ~basic_streambuf() {}

    // ---- buffer management -------------------------------------------
    basic_streambuf* pubsetbuf(char_type* s, streamsize n)
    { return setbuf(s, n); }

    pos_type pubseekoff(off_type off, ios_base::seekdir way,
                        ios_base::openmode which
                            = ios_base::in | ios_base::out)
    { return seekoff(off, way, which); }

    pos_type pubseekpos(pos_type sp,
                        ios_base::openmode which
                            = ios_base::in | ios_base::out)
    { return seekpos(sp, which); }

    int pubsync() { return sync(); }

    // ---- get area ----------------------------------------------------
    //   Characters immediately available without calling a virtual.
    streamsize in_avail()
    {
        if (gptr() < egptr()) { return egptr() - gptr(); }
        return showmanyc();
    }

    //   Advance, then peek. Note the ORDER - snextc consumes the current
    // character first, which is what makes it "next" rather than "get".
    int_type snextc()
    {
        if (traits_type::eq_int_type(sbumpc(), traits_type::eof()))
        {
            return traits_type::eof();
        }
        return sgetc();
    }

    //   Read and consume. Fast path is a pointer bump.
    int_type sbumpc()
    {
        if (gptr() < egptr())
        {
            const int_type c = traits_type::to_int_type(*gptr());
            gbump(1);
            return c;
        }
        return uflow();
    }

    //   Peek without consuming. Fast path is a dereference.
    int_type sgetc()
    {
        if (gptr() < egptr())
        {
            return traits_type::to_int_type(*gptr());
        }
        return underflow();
    }

    streamsize sgetn(char_type* s, streamsize n) { return xsgetn(s, n); }

    // ---- putback -----------------------------------------------------
    //   Fails rather than silently discarding when the character does not
    // match what was actually there - the putback area is only guaranteed to
    // hold what was read from it.
    int_type sputbackc(char_type c)
    {
        if (gptr() > eback() && traits_type::eq(c, gptr()[-1]))
        {
            gbump(-1);
            return traits_type::to_int_type(*gptr());
        }
        return pbackfail(traits_type::to_int_type(c));
    }

    int_type sungetc()
    {
        if (gptr() > eback())
        {
            gbump(-1);
            return traits_type::to_int_type(*gptr());
        }
        return pbackfail();
    }

    // ---- put area ----------------------------------------------------
    int_type sputc(char_type c)
    {
        if (pptr() < epptr())
        {
            *pptr() = c;
            pbump(1);
            return traits_type::to_int_type(c);
        }
        return overflow(traits_type::to_int_type(c));
    }

    streamsize sputn(const char_type* s, streamsize n) { return xsputn(s, n); }

protected:
    basic_streambuf()
        : m_gbeg(0), m_gnext(0), m_gend(0),
          m_pbeg(0), m_pnext(0), m_pend(0)
    {}

    // ---- get area access ---------------------------------------------
    char_type* eback() const { return m_gbeg; }
    char_type* gptr()  const { return m_gnext; }
    char_type* egptr() const { return m_gend; }

    void gbump(int n) { m_gnext += n; return; }

    void setg(char_type* gbeg, char_type* gnext, char_type* gend)
    {
        m_gbeg  = gbeg;
        m_gnext = gnext;
        m_gend  = gend;
        return;
    }

    // ---- put area access ---------------------------------------------
    char_type* pbase() const { return m_pbeg; }
    char_type* pptr()  const { return m_pnext; }
    char_type* epptr() const { return m_pend; }

    void pbump(int n) { m_pnext += n; return; }

    void setp(char_type* pbeg, char_type* pend)
    {
        m_pbeg  = pbeg;
        m_pnext = pbeg;
        m_pend  = pend;
        return;
    }

    // ---- virtuals: all inert by default ------------------------------
    //   A default-constructed basic_streambuf is a valid, permanently empty
    // buffer. See the header note.

    virtual basic_streambuf* setbuf(char_type*, streamsize) { return this; }

    virtual pos_type seekoff(off_type, ios_base::seekdir,
                             ios_base::openmode
                                 = ios_base::in | ios_base::out)
    { return pos_type(off_type(-1)); }

    virtual pos_type seekpos(pos_type,
                             ios_base::openmode
                                 = ios_base::in | ios_base::out)
    { return pos_type(off_type(-1)); }

    virtual int sync() { return 0; }

    //   "How many can I promise without blocking." 0 means "unknown, maybe
    // none"; -1 means "definitely none ever again".
    virtual streamsize showmanyc() { return 0; }

    //   Default loops over sbumpc. A derived class with a contiguous buffer
    // should override this to memcpy - that is the single biggest win
    // available to an implementor.
    virtual streamsize xsgetn(char_type* s, streamsize n)
    {
        streamsize count = 0;
        while (count < n)
        {
            const int_type c = sbumpc();
            if (traits_type::eq_int_type(c, traits_type::eof())) { break; }
            s[count] = traits_type::to_char_type(c);
            ++count;
        }
        return count;
    }

    //   Peek: make a character available WITHOUT consuming it.
    virtual int_type underflow() { return traits_type::eof(); }

    //   Read: make one available AND consume it. Defined in terms of
    // underflow so a derived class need only override that one.
    virtual int_type uflow()
    {
        const int_type c = underflow();
        if (traits_type::eq_int_type(c, traits_type::eof()))
        {
            return traits_type::eof();
        }
        gbump(1);
        return c;
    }

    virtual int_type pbackfail(int_type = _Traits::eof())
    { return traits_type::eof(); }

    virtual streamsize xsputn(const char_type* s, streamsize n)
    {
        streamsize count = 0;
        while (count < n)
        {
            if (traits_type::eq_int_type(sputc(s[count]),
                                         traits_type::eof()))
            {
                break;
            }
            ++count;
        }
        return count;
    }

    virtual int_type overflow(int_type = _Traits::eof())
    { return traits_type::eof(); }

private:
    //   Non-copyable, per std: a buffer's pointers describe storage it does
    // not own, so copying one produces two objects aliasing the same area.
    basic_streambuf(const basic_streambuf&);
    basic_streambuf& operator=(const basic_streambuf&);

    char_type* m_gbeg;
    char_type* m_gnext;
    char_type* m_gend;
    char_type* m_pbeg;
    char_type* m_pnext;
    char_type* m_pend;
};

typedef basic_streambuf<char>    streambuf;
typedef basic_streambuf<wchar_t> wstreambuf;

NS_END  // re_std
#endif  // DJINTERP_RE_STD_STREAMBUF_BASIC_STREAMBUF_
