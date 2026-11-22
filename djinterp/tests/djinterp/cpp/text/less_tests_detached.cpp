/*******************************************************************************
* djinterp [cpp][text]                                   less_tests_detached.cpp
*
*   The groups for css/less/less_template.hpp's five factories and its adopt
* functions, on the default backend. Each factory returns its rule in a
* handle (DLessDetached), which frees it unless a rule or a stylesheet adopts
* it. A Less rule's handle converts to a CSS rule's, so CSS's adopt takes it
* too; Less's adopt takes a CSS rule's handle, and gives a Less facade. A
* handle left unadopted here is freed when it goes, which the suite's runs
* under AddressSanitizer and LeakSanitizer confirm.
*
*
* path:      /tests/djinterp/cpp/text/less_tests_detached.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "less_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::false_type, std::is_same, std::true_type
    #include <utility>      // std::declval, std::move
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/text/css/less/less_template.hpp"  // tree


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// less_backend, less_sheet, less_rule, less_new, less_css_new
//   the default backend, its stylesheet and rule facades, the factories'
// handle, and CSS's handle over the same backend.
typedef djinterp::less::DLessDefaultBackend           less_backend;
typedef djinterp::less::DLessStylesheet<less_backend> less_sheet;
typedef djinterp::less::DLessRule<less_backend>       less_rule;
typedef djinterp::less::DLessDetached<less_backend>   less_new;
typedef djinterp::css::DCssDetached<less_backend>     less_css_new;

/*
d_tests_less_adopts
  True when Parent's adopt takes a Child: a compile-time check of what
adoption refuses.
*/
template<typename Parent,
         typename Child,
         typename = void>
struct d_tests_less_adopts
:   public std::false_type
{};

template<typename Parent,
         typename Child>
struct d_tests_less_adopts<Parent,
                           Child,
                           decltype((void)std::declval<Parent&>().adopt(
                                        std::declval<Child>()))>
:   public std::true_type
{};


/*
d_tests_less_detached_factories
  Tests the following:
  - each of the five factories returns a handle owning a parentless rule
    of its Less kind: a variable (its '@' added once), a mixin with its
    parameters and guard, a mixin call, an @import, an @plugin
  - a mixin reads as a definition once it has a body, given through its
    handle before it's placed; with none, it reads as a call
  - a handle nothing adopts frees its rule
*/
void
d_tests_less_detached_factories(void)
{
    using namespace djinterp::less;

    less_new var    = make_variable<less_backend>("w", "1px");
    less_new at_var = make_variable<less_backend>("@h", "2px");
    less_new mixin  = make_mixin<less_backend>(".m", "@a", "@a > 0");
    less_new call   = make_mixin_call<less_backend>(".m", "1");
    less_new import = make_import<less_backend>("a.less");
    less_new plugin = make_plugin<less_backend>("p.js");

    D_TESTS_TEXT_CHECK(var.valid() && var->is_variable_declaration() &&
                       (var.backend_handle()->property == "@w") &&
                       (at_var.backend_handle()->property == "@h") &&
                       (var.backend_handle()->parent == nullptr));
    D_TESTS_TEXT_CHECK(mixin->is_mixin_call() && mixin->is_guarded() &&
                       (mixin->selector() == ".m(@a) when (@a > 0)"));
    D_TESTS_TEXT_CHECK(mixin->add_declaration("width", "@a").valid() &&
                       mixin->is_mixin_definition() &&
                       mixin->is_parametric_mixin());
    D_TESTS_TEXT_CHECK(call->is_mixin_call() &&
                       (call->selector() == ".m(1)"));
    D_TESTS_TEXT_CHECK((import->at_keyword() == "import") &&
                       plugin->is_plugin_import());

    return;
}


/*
d_tests_less_detached_adopt
  Tests the following:
  - a Less stylesheet's adopt and a Less rule's take a Less rule's handle
    or a CSS rule's, and give a Less facade
  - a CSS stylesheet over the same backend takes a Less rule's handle
  - refused, with an empty facade and the handle keeping its rule: an
    empty rule, an empty handle, the handle's own rule, a rule under it
  - a facade, or a handle not moved, doesn't compile as adopt's argument;
    a CSS rule's handle doesn't become a Less rule's
*/
void
d_tests_less_detached_adopt(void)
{
    using namespace djinterp::less;

    less_sheet sheet;
    less_new   mixin = make_mixin<less_backend>(".m");
    less_rule  held  = mixin.get();
    less_rule  call  = mixin->adopt(make_mixin_call<less_backend>(".n"));
    less_rule  plain = held.adopt(
                           djinterp::css::make_style_rule<less_backend>("a"));

    D_TESTS_TEXT_CHECK(call.valid() && call.is_mixin_call() &&
                       (call.backend_handle()->parent ==
                        held.backend_handle()) &&
                       plain.valid() && (held.rule_count() == 2u));
    D_TESTS_TEXT_CHECK(!less_rule().adopt(std::move(mixin)).valid() &&
                       !sheet.adopt(less_new()).valid() &&
                       mixin.valid());
    D_TESTS_TEXT_CHECK(!held.adopt(std::move(mixin)).valid() &&
                       !call.adopt(std::move(mixin)).valid() &&
                       mixin.valid());
    D_TESTS_TEXT_CHECK((sheet.adopt(std::move(mixin)).backend_handle() ==
                        held.backend_handle()) &&
                       !mixin.valid() && (sheet.rule_count() == 1u));
    D_TESTS_TEXT_CHECK(sheet.adopt(
                           djinterp::css::make_comment<less_backend>("c"))
                           .valid() &&
                       (sheet.rule_count() == 2u));

    djinterp::css::DCssStylesheet<less_backend> css_sheet;

    D_TESTS_TEXT_CHECK(css_sheet.adopt(make_variable<less_backend>("v", "1"))
                           .valid() &&
                       (css_sheet.rule_count() == 1u));

    const bool adopts =
        (d_tests_less_adopts<less_sheet, less_new>::value) &&
        (d_tests_less_adopts<less_sheet, less_css_new>::value) &&
        (d_tests_less_adopts<less_rule, less_css_new>::value) &&
        (std::is_same<decltype(std::declval<less_sheet&>().adopt(
                          std::declval<less_css_new>())),
                      less_rule>::value);
    const bool refused =
        (!d_tests_less_adopts<less_sheet, less_rule>::value) &&
        (!d_tests_less_adopts<less_rule, less_new&>::value) &&
        (!std::is_constructible<less_new, less_css_new&&>::value);

    D_TESTS_TEXT_CHECK(adopts);
    D_TESTS_TEXT_CHECK(refused);

    return;
}

#endif  // C++17
