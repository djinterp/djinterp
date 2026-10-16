/*******************************************************************************
* djinterp [test]                                     test_pack_tests_naming.cpp
*
*   The pure string helpers of section I.  pack_doc_ext_helper maps a document
* format to its extension.  pack_entry_name_helper derives the name the report
* takes as the sole entry of an archive: drop the directory prefix (either
* separator), replace an empty basename with "report", strip one trailing
* container extension - and the residual ".tar" a ".tar.<comp>" leaves behind -
* then apply the document extension.  The edges covered: unix vs windows
* separators, ".tar.gz" double-strip, a name with no extension, an empty path, a
* trailing separator, a leading-dot dotfile (whose dot at index 0 is NOT a
* strippable extension), and a trailing-dot name.
*
*
* path:      /tests/djinterp/test/test_pack_tests_naming.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_pack_tests.hpp"

NS_DJINTERP
NS_TESTING

namespace
{
    // shorthands local to this TU
    std::string ename(const std::string& _p, dt::test_doc_type _d)
    {
        return dt::internal::pack_entry_name_helper(_p, _d);
    }
}


// tests_naming_doc_ext
//   each document format maps to its file extension.
bool
tests_naming_doc_ext()
{
    bool ok = true;
    using dt::internal::pack_doc_ext_helper;

    ok = D_TP_CHECK(pack_doc_ext_helper(dt::test_doc_type::txt)  == "txt")  && ok;
    ok = D_TP_CHECK(pack_doc_ext_helper(dt::test_doc_type::xml)  == "xml")  && ok;
    ok = D_TP_CHECK(pack_doc_ext_helper(dt::test_doc_type::html) == "html") && ok;
    ok = D_TP_CHECK(pack_doc_ext_helper(dt::test_doc_type::pdf)  == "pdf")  && ok;

    return ok;
}

// tests_naming_basename_unix
//   a unix directory prefix is dropped; the container extension is replaced by
// the document extension.
bool
tests_naming_basename_unix()
{
    bool ok = true;

    ok = D_TP_CHECK(ename("out/r.zip", dt::test_doc_type::html) == "r.html") && ok;
    ok = D_TP_CHECK(ename("a/b/c/file.zip", dt::test_doc_type::txt) == "file.txt") && ok;
    ok = D_TP_CHECK(ename("r.zip", dt::test_doc_type::html) == "r.html") && ok;   // no dir

    return ok;
}

// tests_naming_basename_windows
//   a backslash directory prefix is dropped too.
bool
tests_naming_basename_windows()
{
    bool ok = true;

    ok = D_TP_CHECK(ename("C:\\out\\r.zip", dt::test_doc_type::html) == "r.html") && ok;
    ok = D_TP_CHECK(ename("dir\\sub\\a.7z", dt::test_doc_type::xml) == "a.xml")   && ok;

    return ok;
}

// tests_naming_container_strip
//   exactly one container extension is stripped before the document extension
// is applied.
bool
tests_naming_container_strip()
{
    bool ok = true;

    ok = D_TP_CHECK(ename("report.zip", dt::test_doc_type::pdf)  == "report.pdf")  && ok;
    ok = D_TP_CHECK(ename("report.gz",  dt::test_doc_type::txt)  == "report.txt")  && ok;
    ok = D_TP_CHECK(ename("data.7z",    dt::test_doc_type::html) == "data.html")   && ok;

    return ok;
}

// tests_naming_tar_gz_double_strip
//   a ".tar.<comp>" name loses BOTH the compression extension and the residual
// ".tar".
bool
tests_naming_tar_gz_double_strip()
{
    bool ok = true;

    ok = D_TP_CHECK(ename("report.tar.gz", dt::test_doc_type::txt) == "report.txt") && ok;
    ok = D_TP_CHECK(ename("a.tar.gz", dt::test_doc_type::pdf)      == "a.pdf")      && ok;
    ok = D_TP_CHECK(ename("logs/run.tar.gz", dt::test_doc_type::html) == "run.html") && ok;
    // a lone ".tar" is itself a strippable single container extension
    ok = D_TP_CHECK(ename("bundle.tar", dt::test_doc_type::txt) == "bundle.txt") && ok;

    return ok;
}

// tests_naming_no_extension
//   a name with no extension keeps its stem and just gains the document ext.
bool
tests_naming_no_extension()
{
    bool ok = true;

    ok = D_TP_CHECK(ename("plain", dt::test_doc_type::xml) == "plain.xml") && ok;
    ok = D_TP_CHECK(ename("out/noext", dt::test_doc_type::txt) == "noext.txt") && ok;

    return ok;
}

// tests_naming_empty_and_dotfile
//   an empty basename becomes "report"; a leading-dot dotfile is not stripped
// (its dot is at index 0), and a trailing-dot name loses the trailing dot.
bool
tests_naming_empty_and_dotfile()
{
    bool ok = true;

    ok = D_TP_CHECK(ename("", dt::test_doc_type::pdf) == "report.pdf")     && ok;  // empty path
    ok = D_TP_CHECK(ename("out/", dt::test_doc_type::txt) == "report.txt") && ok;  // trailing sep
    ok = D_TP_CHECK(ename(".gitignore", dt::test_doc_type::txt) == ".gitignore.txt") && ok; // dot@0 kept
    ok = D_TP_CHECK(ename("x.", dt::test_doc_type::txt) == "x.txt") && ok;         // trailing dot

    return ok;
}


NS_END  // testing
NS_END  // djinterp
