/*******************************************************************************
* djinterp [test]                                    test_emit_tests_builder.cpp
*
*   Sections IV + V - the builder and the origins.  Both faces (pipe and
* fluent) accumulate into one emit_builder that lowers the plan into the real
* bundle/packaging engine, so these tests observe that engine at its edges: the
* write-success bool, files appearing under a scratch directory, and console
* bytes captured off std::cout - never the renderer's internal output.
*
*   Loose names are fully determined by test_emit (dir + stem|"report" +
* format_extension), so those are asserted exactly; per_module fan-out is
* checked by file COUNT and per-module names; archive / deflate-each dispatch by
* success + that something was written (the packaging layer owns the exact
* archive filename); console by non-empty capture.  The RAII default (write on
* destruction when no terminal was reached - loose for dest-ful targets, console
* otherwise), moved-from inertness (a moved-from builder must not also flush),
* stem_of (extension dropped, directory kept), the four operator| overloads,
* and the null-report producer path each get a unit.
*
*
* path:      /tests/djinterp/test/test_emit_tests_builder.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_emit_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_builder_loose_names
//   a loose write (to_dir) with a dest-less target lands report.<ext> in the
// target directory and reports success.
bool
tests_builder_loose_names()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});

    bool _w = (dt::emit(r) | dt::as_text | dt::to_dir(td.dir_slash()));
    ok = D_TE_CHECK(_w)                    && ok;
    ok = D_TE_CHECK(td.has("report.txt"))  && ok;   // stem "report" + ".txt"
    ok = D_TE_CHECK(td.file_count() == 1)  && ok;

#endif
    return ok;
}

// tests_builder_stem_of
//   the pinned destination is stemmed to the bundle item name: the extension is
// dropped (and re-added by format) and any directory is kept.
bool
tests_builder_stem_of()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    std::error_code _ec;
    std::filesystem::create_directories(td.root / "sub", _ec);   // for the dir-bearing dest
    dt::test_report r = make_report({"m"});

    bool _w = (dt::emit(r)
        | dt::to_html_file("myreport.html")   // ext dropped, re-added -> myreport.html
        | dt::to_pdf_file("myreport")         // no dot            -> myreport.pdf
        | dt::to_text_file("sub/r.txt")       // dir kept, ext re-added -> sub/r.txt
        | dt::to_dir(td.dir_slash()));

    ok = D_TE_CHECK(_w)                       && ok;
    ok = D_TE_CHECK(td.has("myreport.html"))  && ok;
    ok = D_TE_CHECK(td.has("myreport.pdf"))   && ok;
    ok = D_TE_CHECK(td.has("sub/r.txt"))      && ok;

#endif
    return ok;
}

// tests_builder_per_module_fanout
//   per_module emits one file per module, each named by its module.
bool
tests_builder_per_module_fanout()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"alpha", "beta", "gamma"});

    bool _w = (dt::emit(r) | dt::per_module(dt::as_text) | dt::to_dir(td.dir_slash()));
    ok = D_TE_CHECK(_w)                    && ok;
    ok = D_TE_CHECK(td.file_count() == 3)  && ok;   // one per module
    ok = D_TE_CHECK(td.has("alpha.txt"))   && ok;
    ok = D_TE_CHECK(td.has("beta.txt"))    && ok;
    ok = D_TE_CHECK(td.has("gamma.txt"))   && ok;

#endif
    return ok;
}

// tests_builder_multiple_targets
//   a two-format group (not per_module) emits both documents, both named
// "report".
bool
tests_builder_multiple_targets()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});

    bool _w = (dt::emit(r) | (dt::as_text + dt::as_html) | dt::to_dir(td.dir_slash()));
    ok = D_TE_CHECK(_w)                    && ok;
    ok = D_TE_CHECK(td.file_count() == 2)  && ok;
    ok = D_TE_CHECK(td.has("report.txt"))  && ok;
    ok = D_TE_CHECK(td.has("report.html")) && ok;

#endif
    return ok;
}

// tests_builder_console_default
//   a dest-less builder that reaches no terminal flushes to the console on
// destruction (the RAII default).
bool
tests_builder_console_default()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::test_report r = make_report({"m"});

    cout_capture _cap;
    {
        dt::emit_builder _b(&r, dt::default_test_options());
        _b.as_html_();          // dest-less target, no terminal
    }                           // _b destructs -> flush_default -> console
    ok = D_TE_CHECK(!_cap.str().empty()) && ok;

#endif
    return ok;
}

// tests_builder_explicit_console
//   to_console writes the report bytes to std::cout and reports success.
bool
tests_builder_explicit_console()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::test_report r = make_report({"m"});

    cout_capture _cap;
    bool _w = (dt::emit(r) | dt::as_text | dt::to_console());
    ok = D_TE_CHECK(_w)                  && ok;
    ok = D_TE_CHECK(!_cap.str().empty()) && ok;

#endif
    return ok;
}

// tests_builder_archive_dispatch
//   into_zip drives the archive sink and reports success (the archive file is
// written; its exact name belongs to the packaging layer).
bool
tests_builder_archive_dispatch()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});

    bool _w = dt::emit(r).to_pdf_file("out.pdf").into_zip(td.with("archive"));
    ok = D_TE_CHECK(_w)                    && ok;
    ok = D_TE_CHECK(td.file_count() >= 1)  && ok;

#endif
    return ok;
}

// tests_builder_compress_dispatch
//   compress drives the deflate-each sink and reports success.
bool
tests_builder_compress_dispatch()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});
    const dt::emit_sink _def;   // codec reference

    bool _w = dt::emit(r).to_text_file(td.with("z")).compress(_def.cod);
    ok = D_TE_CHECK(_w)                    && ok;
    ok = D_TE_CHECK(td.file_count() >= 1)  && ok;

#endif
    return ok;
}

// tests_builder_pipe_operators
//   the four operator| overloads wire an option tune (with), a target (add), a
// group (pipe_group), and a sink (run_sink) - here three targets -> three
// files.
bool
tests_builder_pipe_operators()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});

    bool _w = (dt::emit(r)
        | dt::with(dt::tune::width(100))      // | option_tune  -> with()
        | dt::as_text                          // | emit_target  -> add
        | (dt::as_html + dt::as_xml)           // | target_group -> pipe_group
        | dt::to_dir(td.dir_slash()));         // | emit_sink    -> run_sink (bool)

    ok = D_TE_CHECK(_w)                    && ok;
    ok = D_TE_CHECK(td.file_count() == 3)  && ok;
    ok = D_TE_CHECK(td.has("report.txt") && td.has("report.html") && td.has("report.xml")) && ok;

#endif
    return ok;
}

// tests_builder_fluent_dest_less
//   the dest-less fluent format methods each add a target (all named "report").
bool
tests_builder_fluent_dest_less()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});

    bool _w = dt::emit(r).as_markdown_().as_xml_().as_pdf_().to_dir(td.dir_slash());
    ok = D_TE_CHECK(_w)                    && ok;
    ok = D_TE_CHECK(td.file_count() == 3)  && ok;
    ok = D_TE_CHECK(td.has("report.md") && td.has("report.xml") && td.has("report.pdf")) && ok;

#endif
    return ok;
}

// tests_builder_fluent_dest_ful_and_style
//   the dest-ful fluent methods (and style()) accumulate; with an empty to_dir
// prefix the pinned absolute paths are used verbatim.
bool
tests_builder_fluent_dest_ful_and_style()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});
    dt::pdf_template_source _src;   // empty style source (exercises style())

    bool _w = dt::emit(r)
                  .style(_src)
                  .to_md_file(td.with("a"))
                  .to_xml_file(td.with("b"))
                  .to_html_file(td.with("c"))
                  .to_dir(std::string());   // empty prefix -> abs paths as-is
    ok = D_TE_CHECK(_w)               && ok;
    ok = D_TE_CHECK(td.has("a.md"))   && ok;
    ok = D_TE_CHECK(td.has("b.xml"))  && ok;
    ok = D_TE_CHECK(td.has("c.html")) && ok;

#endif
    return ok;
}

// tests_builder_fluent_per_module_method
//   the fluent per_module() method fans a single format across the modules.
bool
tests_builder_fluent_per_module_method()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"x", "y"});

    bool _w = dt::emit(r).as_text_().per_module().to_dir(td.dir_slash());
    ok = D_TE_CHECK(_w)                   && ok;
    ok = D_TE_CHECK(td.file_count() == 2) && ok;
    ok = D_TE_CHECK(td.has("x.txt") && td.has("y.txt")) && ok;

#endif
    return ok;
}

// tests_builder_raii_loose
//   a builder with a dest-ful target and no terminal writes loose files (to the
// pinned paths) on destruction.
bool
tests_builder_raii_loose()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;
    dt::test_report r = make_report({"m"});

    {
        dt::emit_builder _b(&r, dt::default_test_options());
        _b.to_text_file(td.with("raii"));   // dest-ful, no terminal
    }                                       // destructor -> flush_default -> loose
    ok = D_TE_CHECK(td.has("raii.txt")) && ok;

#endif
    return ok;
}

// tests_builder_moved_from_inert
//   a moved-from builder is inert: only the moved-to builder flushes, so the
// console output matches a single explicit console emit (a live moved-from
// would double it).
bool
tests_builder_moved_from_inert()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::test_report r = make_report({"m"});

    // baseline: exactly one console flush
    std::string _one;
    {
        cout_capture _cap;
        (void)(dt::emit(r) | dt::as_text | dt::to_console());
        _one = _cap.str();
    }

    // move, then give BOTH builders a dest-less target.  Because the move also
    // moves the target group out of the source, the source is repopulated AFTER
    // the move - so the only thing keeping it from a second console flush is its
    // moved-from inertness (m_done).
    std::string _moved;
    {
        cout_capture _cap;
        {
            dt::emit_builder _src(&r, dt::default_test_options());
            dt::emit_builder _dst(static_cast<dt::emit_builder&&>(_src));   // move -> src inert
            _src.as_text_();   // repopulate src after the move
            _dst.as_text_();   // dst carries its own target
        }   // _dst flushes to console; an inert _src must NOT flush again
        _moved = _cap.str();
    }

    ok = D_TE_CHECK(!_one.empty())    && ok;
    ok = D_TE_CHECK(_moved == _one)   && ok;   // one flush, not two -> src was inert

#endif
    return ok;
}

// tests_builder_null_report
//   a builder with no report still writes (empty content) and reports success -
// the build() producer's null-report branch.
bool
tests_builder_null_report()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    temp_dir td;

    dt::emit_builder _b(nullptr, dt::default_test_options());
    bool _w = _b.to_text_file(td.with("empty")).to_dir(std::string());
    ok = D_TE_CHECK(_w)                  && ok;
    ok = D_TE_CHECK(td.has("empty.txt")) && ok;   // created, even if empty

#endif
    return ok;
}


NS_END  // testing
NS_END  // djinterp
