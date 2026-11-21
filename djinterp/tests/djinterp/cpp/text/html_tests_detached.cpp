/*******************************************************************************
* djinterp [test]                                        html_tests_detached.cpp
*
*   The groups for html/html_template.hpp's factories and DHtmlElement::adopt,
* on the default backend. Each factory returns its element in a handle
* (DHtmlDetached), which frees it unless an element adopts it; adoption takes
* only a handle. A handle left unadopted here is freed when it goes, which
* the suite's runs under AddressSanitizer and LeakSanitizer confirm.
*
*
* path:      /tests/djinterp/cpp/text/html_tests_detached.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "html_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <string>  // std::string
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::false_type, std::true_type
    #include <utility>      // std::declval, std::move
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/text/html/html_template.hpp"  // tree


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// html_backend, html_element, html_new
//   the default backend, its element facade, and the factories' handle.
typedef djinterp::html::DHtmlDefaultBackend             html_backend;
typedef djinterp::html::DHtmlElement<html_backend>      html_element;
typedef djinterp::html::DHtmlDetached<html_backend>     html_new;

/*
d_tests_html_adopts
  True when Parent's adopt takes a Child: a compile-time check of what
adoption refuses.
*/
template<typename Parent,
         typename Child,
         typename = void>
struct d_tests_html_adopts
:   public std::false_type
{};

template<typename Parent,
         typename Child>
struct d_tests_html_adopts<Parent,
                           Child,
                           decltype((void)std::declval<Parent&>().adopt(
                                        std::declval<Child>()))>
:   public std::true_type
{};

/*
d_tests_html_attribute
  An element's attribute's value, or "(none)".
*/
static std::string
d_tests_html_attribute(
    const html_element& _element,
    const char*         _name
)
{
    const djinterp::xml::DXmlAttribute* found =
        _element.find_attribute(_name);

    return (found) ? found->value : std::string("(none)");
}


/*
d_tests_html_detached_factories
  Tests the following:
  - each of the seven factories returns a handle owning a parentless
    element of its tag, with the text, attributes, id and class it's given
  - make_heading clamps its level to h1 below 1 and above 6
  - a handle no element adopts frees its element
*/
void
d_tests_html_detached_factories(void)
{
    using namespace djinterp::html;

    html_new div   = make_div<html_backend>("main", "wide");
    html_new span  = make_span<html_backend>("s", "note");
    html_new link  = make_anchor<html_backend>("/a", "A");
    html_new image = make_image<html_backend>("p.png", "P");
    html_new para  = make_paragraph<html_backend>("p");
    html_new h3    = make_heading<html_backend>(3, "h");
    html_new h9    = make_heading<html_backend>(9);
    html_new by    = make_element_by_kind<html_backend>(
                         DHtmlElementKind::section);

    D_TESTS_TEXT_CHECK(div.valid() && (div->name() == "div") &&
                       (div->get_id() == "main") && div->has_class("wide") &&
                       (div.backend_handle()->parent == nullptr));
    D_TESTS_TEXT_CHECK((span->name() == "span") && (span->text() == "s") &&
                       span->has_class("note"));
    D_TESTS_TEXT_CHECK((link->name() == "a") && (link->text() == "A") &&
                       (d_tests_html_attribute(link.get(), "href") == "/a"));
    D_TESTS_TEXT_CHECK((image->name() == "img") &&
                       (d_tests_html_attribute(image.get(), "src") ==
                        "p.png") &&
                       (d_tests_html_attribute(image.get(), "alt") == "P"));
    D_TESTS_TEXT_CHECK((para->name() == "p") && (para->text() == "p"));
    D_TESTS_TEXT_CHECK((h3->name() == "h3") && (h3->text() == "h") &&
                       (h9->name() == "h1") &&
                       (make_heading<html_backend>(0)->name() == "h1"));
    D_TESTS_TEXT_CHECK((by->name() == "section") &&
                       (by->html_kind() == DHtmlElementKind::section));

    return;
}


/*
d_tests_html_detached_adopt
  Tests the following:
  - adopt makes the element the last child, sets its parent, empties the
    handle, and returns its facade; the facade get() gave before still
    reaches it
  - a subtree built under a handle's element goes with it
  - refused, with an empty facade and the handle keeping its element: an
    empty element, an empty handle, the handle's own element, and an
    element under it
  - a facade, or a handle not moved, doesn't compile as adopt's argument
*/
void
d_tests_html_detached_adopt(void)
{
    using namespace djinterp::html;

    DHtmlDocument<html_backend> doc =
        make_html5_document<html_backend>("T");
    html_element body = doc.body_element();
    html_new     div  = make_div<html_backend>("d");
    html_element seen = div.get();
    html_element span = div->adopt(make_span<html_backend>("x"));
    html_element placed;

    D_TESTS_TEXT_CHECK(span.valid() && (span.name() == "span") &&
                       (span.backend_handle()->parent ==
                        seen.backend_handle()));
    D_TESTS_TEXT_CHECK(!html_element().adopt(std::move(div)).valid() &&
                       div.valid());
    D_TESTS_TEXT_CHECK(!body.adopt(html_new()).valid());
    D_TESTS_TEXT_CHECK(!seen.adopt(std::move(div)).valid() && div.valid());
    D_TESTS_TEXT_CHECK(!span.adopt(std::move(div)).valid() && div.valid());

    placed = body.adopt(std::move(div));
    D_TESTS_TEXT_CHECK(!div.valid() && (placed.backend_handle() ==
                                        seen.backend_handle()));
    D_TESTS_TEXT_CHECK((body.child_count() == 1u) &&
                       (placed.backend_handle()->parent ==
                        body.backend_handle()) &&
                       (seen.child_count() == 1u) &&
                       (seen.first_child().name() == "span"));
    D_TESTS_TEXT_CHECK(body.adopt(make_paragraph<html_backend>("2")).valid()
                       && (body.child_count() == 2u));

    const bool refused =
        (!d_tests_html_adopts<html_element, html_element>::value) &&
        (!d_tests_html_adopts<html_element, html_new&>::value) &&
        (d_tests_html_adopts<html_element, html_new>::value);

    D_TESTS_TEXT_CHECK(refused);

    return;
}

#endif  // C++20
