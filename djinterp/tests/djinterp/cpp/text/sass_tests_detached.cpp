/*******************************************************************************
* djinterp [test]                                        sass_tests_detached.cpp
*
*   The groups for css/sass/sass_template.hpp's eight factories and its adopt
* functions, on the default backend. Each factory returns its rule in a
* handle (DSassDetached), which frees it unless a rule or a stylesheet adopts
* it. A Sass rule's handle converts to a CSS rule's, so CSS's adopt takes it
* too; Sass's adopt takes a CSS rule's handle, and gives a Sass facade. A
* handle left unadopted here is freed when it goes, which the suite's runs
* under AddressSanitizer and LeakSanitizer confirm.
*
*
* path:      /tests/djinterp/cpp/text/sass_tests_detached.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "sass_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::false_type, std::is_same, std::true_type
    #include <utility>      // std::declval, std::move
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/text/css/sass/sass_template.hpp"  // tree


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// sass_backend, sass_sheet, sass_rule, sass_new, sass_css_new
//   the default backend, its stylesheet and rule facades, the factories'
// handle, and CSS's handle over the same backend.
typedef djinterp::sass::DSassDefaultBackend           sass_backend;
typedef djinterp::sass::DSassStylesheet<sass_backend> sass_sheet;
typedef djinterp::sass::DSassRule<sass_backend>       sass_rule;
typedef djinterp::sass::DSassDetached<sass_backend>   sass_new;
typedef djinterp::css::DCssDetached<sass_backend>     sass_css_new;

/*
d_tests_sass_adopts
  True when Parent's adopt takes a Child: a compile-time check of what
adoption refuses.
*/
template<typename Parent,
         typename Child,
         typename = void>
struct d_tests_sass_adopts
:   public std::false_type
{};

template<typename Parent,
         typename Child>
struct d_tests_sass_adopts<Parent,
                           Child,
                           decltype((void)std::declval<Parent&>().adopt(
                                        std::declval<Child>()))>
:   public std::true_type
{};


/*
d_tests_sass_detached_factories
  Tests the following:
  - each of the eight factories returns a handle owning a parentless rule
    of its Sass kind: a variable, @mixin, @include, @extend, @use, a
    placeholder selector, @each and @if
  - a handle nothing adopts frees its rule
*/
void
d_tests_sass_detached_factories(void)
{
    using namespace djinterp::sass;

    sass_new var     = make_variable<sass_backend>("w", "1px", true);
    sass_new mixin   = make_mixin<sass_backend>("m", "$a");
    sass_new include = make_include<sass_backend>("m", "1");
    sass_new extend  = make_extend<sass_backend>("%p", true);
    sass_new use     = make_use<sass_backend>("sass:math", "m");
    sass_new holder  = make_placeholder_rule<sass_backend>("p");
    sass_new each    = make_each<sass_backend>("$i", "1, 2");
    sass_new when    = make_if<sass_backend>("$a > 0");

    D_TESTS_TEXT_CHECK(var.valid() && var->is_variable_declaration() &&
                       (var.backend_handle()->parent == nullptr));
    D_TESTS_TEXT_CHECK(mixin->is_mixin_declaration() &&
                       include->is_include_statement() &&
                       extend->is_extend_statement());
    D_TESTS_TEXT_CHECK(use->is_use_rule() && holder->is_placeholder_rule() &&
                       each->is_each_statement() && when->is_if_statement());

    return;
}


/*
d_tests_sass_detached_adopt
  Tests the following:
  - a Sass stylesheet's adopt and a Sass rule's take a Sass rule's handle
    or a CSS rule's, and give a Sass facade
  - a CSS stylesheet over the same backend takes a Sass rule's handle
  - refused, with an empty facade and the handle keeping its rule: an
    empty rule, an empty handle, the handle's own rule, a rule under it
  - a facade, or a handle not moved, doesn't compile as adopt's argument;
    a CSS rule's handle doesn't become a Sass rule's
*/
void
d_tests_sass_detached_adopt(void)
{
    using namespace djinterp::sass;

    sass_sheet sheet;
    sass_new   mixin   = make_mixin<sass_backend>("m");
    sass_rule  held    = mixin.get();
    sass_rule  include = mixin->adopt(make_include<sass_backend>("n"));
    sass_rule  plain   = held.adopt(
                             djinterp::css::make_style_rule<sass_backend>("a"));

    D_TESTS_TEXT_CHECK(include.valid() && include.is_include_statement() &&
                       (include.backend_handle()->parent ==
                        held.backend_handle()) &&
                       plain.valid() && (held.rule_count() == 2u));
    D_TESTS_TEXT_CHECK(!sass_rule().adopt(std::move(mixin)).valid() &&
                       !sheet.adopt(sass_new()).valid() &&
                       mixin.valid());
    D_TESTS_TEXT_CHECK(!held.adopt(std::move(mixin)).valid() &&
                       !include.adopt(std::move(mixin)).valid() &&
                       mixin.valid());
    D_TESTS_TEXT_CHECK((sheet.adopt(std::move(mixin)).backend_handle() ==
                        held.backend_handle()) &&
                       !mixin.valid() && (sheet.rule_count() == 1u));
    D_TESTS_TEXT_CHECK(sheet.adopt(
                           djinterp::css::make_comment<sass_backend>("c"))
                           .valid() &&
                       (sheet.rule_count() == 2u));

    djinterp::css::DCssStylesheet<sass_backend> css_sheet;

    D_TESTS_TEXT_CHECK(css_sheet.adopt(make_variable<sass_backend>("v", "1"))
                           .valid() &&
                       (css_sheet.rule_count() == 1u));

    const bool adopts =
        (d_tests_sass_adopts<sass_sheet, sass_new>::value) &&
        (d_tests_sass_adopts<sass_sheet, sass_css_new>::value) &&
        (d_tests_sass_adopts<sass_rule, sass_css_new>::value) &&
        (std::is_same<decltype(std::declval<sass_sheet&>().adopt(
                          std::declval<sass_css_new>())),
                      sass_rule>::value);
    const bool refused =
        (!d_tests_sass_adopts<sass_sheet, sass_rule>::value) &&
        (!d_tests_sass_adopts<sass_rule, sass_new&>::value) &&
        (!std::is_constructible<sass_new, sass_css_new&&>::value);

    D_TESTS_TEXT_CHECK(adopts);
    D_TESTS_TEXT_CHECK(refused);

    return;
}

#endif  // C++17
