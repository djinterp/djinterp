/*******************************************************************************
* djinterp [test]                                      test_emit_tests_sinks.cpp
*
*   Section III - terminals.  A sink is a value type naming the packaging mode
* (loose / archive / deflate-each / console) plus a container format, a codec,
* and a name/prefix.  These checks read those public fields, comparing the
* format/codec against a default-constructed sink's fields rather than naming
* the engine's format_id_* / codec_id_* constants (which live in the packaging
* layer): into_7z and into_zip both select archive mode but differ in container
* (and into_zip matches the default zip); compress selects deflate-each and
* carries the codec it was handed; to_dir is loose with a prefix; to_console is
* console.
*
*
* path:      /tests/djinterp/test/test_emit_tests_sinks.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_emit_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_sink_defaults
//   a default-constructed sink is loose with an empty name; its format/codec
// are the zip/gzip fallbacks used as reference below.
bool
tests_sink_defaults()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::emit_sink _d;
    ok = D_TE_CHECK(_d.mode == dt::emit_sink::loose) && ok;
    ok = D_TE_CHECK(_d.name.empty())                 && ok;

    // the default format and codec are self-consistent references (compared
    // against, not spelled out, by the tests below)
    ok = D_TE_CHECK(_d.fmt == dt::emit_sink().fmt)   && ok;
    ok = D_TE_CHECK(_d.cod == dt::emit_sink().cod)   && ok;

#endif
    return ok;
}

// tests_sink_into_7z_zip
//   into_7z / into_zip select archive mode; they differ in container, and
// into_zip's container is the default (zip), while the codec stays at default.
bool
tests_sink_into_7z_zip()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    const dt::emit_sink _def;   // reference: zip container, gzip codec

    dt::emit_sink _7z = dt::into_7z("arc");
    ok = D_TE_CHECK(_7z.mode == dt::emit_sink::archive) && ok;
    ok = D_TE_CHECK(_7z.name == std::string("arc"))     && ok;
    ok = D_TE_CHECK(_7z.fmt != _def.fmt)                && ok;   // sevenzip != zip
    ok = D_TE_CHECK(_7z.cod == _def.cod)                && ok;   // codec left at default

    dt::emit_sink _zip = dt::into_zip("box");
    ok = D_TE_CHECK(_zip.mode == dt::emit_sink::archive) && ok;
    ok = D_TE_CHECK(_zip.name == std::string("box"))     && ok;
    ok = D_TE_CHECK(_zip.fmt == _def.fmt)                && ok;   // zip == default container
    ok = D_TE_CHECK(_7z.fmt != _zip.fmt)                 && ok;   // and 7z differs from zip

#endif
    return ok;
}

// tests_sink_compress
//   compress selects deflate-each and carries the codec it was handed, with no
// name.
bool
tests_sink_compress()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    const dt::emit_sink _def;                 // supplies a codec value to pass
    dt::emit_sink _c = dt::compress(_def.cod);

    ok = D_TE_CHECK(_c.mode == dt::emit_sink::deflate_each) && ok;
    ok = D_TE_CHECK(_c.cod == _def.cod)                     && ok;   // codec carried through
    ok = D_TE_CHECK(_c.name.empty())                        && ok;

#endif
    return ok;
}

// tests_sink_to_dir_console
//   to_dir is loose with the given prefix; to_console is console.
bool
tests_sink_to_dir_console()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::emit_sink _td = dt::to_dir("out/");
    ok = D_TE_CHECK(_td.mode == dt::emit_sink::loose)  && ok;
    ok = D_TE_CHECK(_td.name == std::string("out/"))   && ok;

    dt::emit_sink _tc = dt::to_console();
    ok = D_TE_CHECK(_tc.mode == dt::emit_sink::console) && ok;

#endif
    return ok;
}


NS_END  // testing
NS_END  // djinterp
