/*******************************************************************************
* djinterp [test]                                          css_tests_printer.cpp
*
*   The group for css/css_printer.hpp: a printer writes a stylesheet's
* rendering to its sink, in the mode it is given or set to; the string and
* stream helpers give the same text; and a file printer writes it to disk.
*
*
* path:      /tests/djinterp/cpp/text/css_tests_printer.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "css_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstddef>   // std::size_t
#include <cstdio>    // std::remove, std::tmpnam
#include <fstream>   // std::ifstream
#include <iterator>  // std::istreambuf_iterator
#include <sstream>   // std::ostringstream
#include <string>    // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/css/css_template.hpp"
                                                      // the stylesheet
#include "../../../../inc/djinterp/cpp/text/css/css_printer.hpp"
                                                      // the printer


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_css_printer
  Tests the following:
  - a printer's sink receives the rendering in the printer's mode, and the
    mode can be changed
  - css_to_string and css_to_stream give the same text as the stylesheet
  - save_css_to_file writes it, and says so
*/
void
d_tests_css_printer(void)
{
    using namespace djinterp::css;

    typedef DCssStylesheet<DCssDefaultBackend> sheet_type;

    sheet_type         sheet;
    std::string        sunk;
    std::ostringstream stream;

    sheet.add_style_rule("p").add_declaration("color", "red");

    auto printer = make_css_printer(
        [&sunk](const char* _data, std::size_t _size)
        {
            sunk.append(_data, _size);
        },
        DCssRenderMode::minified);

    printer.print(sheet);
    D_TESTS_TEXT_CHECK(sunk == sheet.render_to_minified_css());
    printer.set_mode(DCssRenderMode::pretty);
    sunk.clear();
    printer.print(sheet);
    D_TESTS_TEXT_CHECK((printer.mode() == DCssRenderMode::pretty) &&
                       (sunk == "p {\n  color: red;\n}\n"));
    D_TESTS_TEXT_CHECK(css_to_string(sheet) == sheet.render_to_css());
    D_TESTS_TEXT_CHECK(css_to_string(sheet, DCssRenderMode::scss) ==
                       sheet.render_to_scss());
    css_to_stream(stream, sheet, DCssRenderMode::minified);
    D_TESTS_TEXT_CHECK(stream.str() == "p{color:red;}");

    const std::string path = "css_tests_printer.css";

    D_TESTS_TEXT_CHECK(save_css_to_file(path, sheet));

    std::ifstream     in(path.c_str(), std::ios::binary);
    const std::string read((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    in.close();
    (void)std::remove(path.c_str());
    D_TESTS_TEXT_CHECK(read == sheet.render_to_css());

    return;
}

#endif  // C++17
