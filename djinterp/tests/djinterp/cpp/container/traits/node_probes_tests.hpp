/*******************************************************************************
* djinterp [cpp][container]                                node_probes_tests.hpp
*
*   The probe types for node_probes.hpp's suite, and the macros that
* state each answer.  The suite is all compile time: each answer is a
* static_assert, built at every standard from C++11.
*
*
* path:      /tests/djinterp/cpp/container/traits/node_probes_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#ifndef DJINTERP_TESTS_NODE_PROBES_TESTS_HPP
#define DJINTERP_TESTS_NODE_PROBES_TESTS_HPP 1

// std
#include <cstddef>      // std::nullptr_t
#include <string>       // std::string
#include <type_traits>  // std::integral_constant, std::is_base_of
// djinterp
#include "node_probes.hpp"  // via INCLUDES (traits dir)


NS_DJINTERP
NS_TESTING

namespace np {

// ===========================================================================
// I.   The probe types
// ===========================================================================
//   For each method a probe asks about, the same 35 shapes: the method
// const, mutable, ref-qualified, static, deleted, private, protected,
// templated, taking arguments, overloaded, inherited, ambiguous; a data
// member of that name, plain, a function pointer, or a function object; and
// a type with none of these.  Each is asked about as T, const T, volatile
// T, T&, const T&, T&&, const T&& and T*.

// np_functor, np_mut_functor
//   type: a function object, callable const and mutable only.
struct np_functor     { int operator()() const; };
struct np_mut_functor { int operator()(); };

// fx_parent: the shapes, with a method or member named parent
namespace fx_parent {
    struct const_m         { int parent() const; };
    struct mut_m           { int parent(); };
    struct lref_m          { int parent() &; };
    struct rref_m          { int parent() &&; };
    struct clref_m         { int parent() const &; };
    struct crref_m         { int parent() const &&; };
    struct cv_m            { int parent() const volatile; };
    struct volatile_m      { int parent() volatile; };
    struct static_m        { static int parent(); };
    struct void_m          { void parent() const; };
    struct field_m         { int parent; };
    struct fnptr_m         { int (*parent)(); };
    struct functor_m       { np_functor parent; };
    struct mut_functor_m   { np_mut_functor parent; };
    struct private_m       { private: int parent() const; };
    struct protected_m     { protected: int parent() const; };
    struct deleted_m       { int parent() const = delete; };
    struct arg_int_m       { int parent(int) const; };
    struct arg_str_m       { int parent(const std::string&); };
    struct arg_int_int_m   { int parent(int, int); };
    struct arg_ptr_m       { int parent(int*) const; };
    struct default_arg_m   { int parent(int = 0) const; };
    struct variadic_m
        { template<typename... A> int parent(A&&...) const; };
    struct tmpl_m          { template<typename T = int> int parent() const; };
    struct tmpl_nodef_m    { template<typename T> int parent() const; };
    struct overloaded_m    { int parent() const; int parent(int); };
    struct mut_and_const_m { int parent(); int parent() const; };
    struct abstract_m      { virtual int parent() const = 0; };
    struct virtual_m       { virtual int parent() const; };
    struct ref_ret_m       { int& parent() const; };
    struct noexcept_m      { int parent() const noexcept; };
    struct empty           {  };
    struct derived_m : const_m {};
    struct derived_mut_m : mut_m {};
    struct derived_private_m : private_m {};
    struct base_a { int parent() const; };
    struct base_b { int parent() const; };
    struct ambiguous_m : base_a, base_b {};
    union  union_m { int parent() const; int x; };
}  // fx_parent

// fx_first_child: the shapes, with a method or member named first_child
namespace fx_first_child {
    struct const_m         { int first_child() const; };
    struct mut_m           { int first_child(); };
    struct lref_m          { int first_child() &; };
    struct rref_m          { int first_child() &&; };
    struct clref_m         { int first_child() const &; };
    struct crref_m         { int first_child() const &&; };
    struct cv_m            { int first_child() const volatile; };
    struct volatile_m      { int first_child() volatile; };
    struct static_m        { static int first_child(); };
    struct void_m          { void first_child() const; };
    struct field_m         { int first_child; };
    struct fnptr_m         { int (*first_child)(); };
    struct functor_m       { np_functor first_child; };
    struct mut_functor_m   { np_mut_functor first_child; };
    struct private_m       { private: int first_child() const; };
    struct protected_m     { protected: int first_child() const; };
    struct deleted_m       { int first_child() const = delete; };
    struct arg_int_m       { int first_child(int) const; };
    struct arg_str_m       { int first_child(const std::string&); };
    struct arg_int_int_m   { int first_child(int, int); };
    struct arg_ptr_m       { int first_child(int*) const; };
    struct default_arg_m   { int first_child(int = 0) const; };
    struct variadic_m
        { template<typename... A> int first_child(A&&...) const; };
    struct tmpl_m
        { template<typename T = int> int first_child() const; };
    struct tmpl_nodef_m    { template<typename T> int first_child() const; };
    struct overloaded_m    { int first_child() const; int first_child(int); };
    struct mut_and_const_m { int first_child(); int first_child() const; };
    struct abstract_m      { virtual int first_child() const = 0; };
    struct virtual_m       { virtual int first_child() const; };
    struct ref_ret_m       { int& first_child() const; };
    struct noexcept_m      { int first_child() const noexcept; };
    struct empty           {  };
    struct derived_m : const_m {};
    struct derived_mut_m : mut_m {};
    struct derived_private_m : private_m {};
    struct base_a { int first_child() const; };
    struct base_b { int first_child() const; };
    struct ambiguous_m : base_a, base_b {};
    union  union_m { int first_child() const; int x; };
}  // fx_first_child

// fx_children: the shapes, with a method or member named children
namespace fx_children {
    struct const_m         { int children() const; };
    struct mut_m           { int children(); };
    struct lref_m          { int children() &; };
    struct rref_m          { int children() &&; };
    struct clref_m         { int children() const &; };
    struct crref_m         { int children() const &&; };
    struct cv_m            { int children() const volatile; };
    struct volatile_m      { int children() volatile; };
    struct static_m        { static int children(); };
    struct void_m          { void children() const; };
    struct field_m         { int children; };
    struct fnptr_m         { int (*children)(); };
    struct functor_m       { np_functor children; };
    struct mut_functor_m   { np_mut_functor children; };
    struct private_m       { private: int children() const; };
    struct protected_m     { protected: int children() const; };
    struct deleted_m       { int children() const = delete; };
    struct arg_int_m       { int children(int) const; };
    struct arg_str_m       { int children(const std::string&); };
    struct arg_int_int_m   { int children(int, int); };
    struct arg_ptr_m       { int children(int*) const; };
    struct default_arg_m   { int children(int = 0) const; };
    struct variadic_m
        { template<typename... A> int children(A&&...) const; };
    struct tmpl_m          { template<typename T = int> int children() const; };
    struct tmpl_nodef_m    { template<typename T> int children() const; };
    struct overloaded_m    { int children() const; int children(int); };
    struct mut_and_const_m { int children(); int children() const; };
    struct abstract_m      { virtual int children() const = 0; };
    struct virtual_m       { virtual int children() const; };
    struct ref_ret_m       { int& children() const; };
    struct noexcept_m      { int children() const noexcept; };
    struct empty           {  };
    struct derived_m : const_m {};
    struct derived_mut_m : mut_m {};
    struct derived_private_m : private_m {};
    struct base_a { int children() const; };
    struct base_b { int children() const; };
    struct ambiguous_m : base_a, base_b {};
    union  union_m { int children() const; int x; };
}  // fx_children

// fx_child_count: the shapes, with a method or member named child_count
namespace fx_child_count {
    struct const_m         { int child_count() const; };
    struct mut_m           { int child_count(); };
    struct lref_m          { int child_count() &; };
    struct rref_m          { int child_count() &&; };
    struct clref_m         { int child_count() const &; };
    struct crref_m         { int child_count() const &&; };
    struct cv_m            { int child_count() const volatile; };
    struct volatile_m      { int child_count() volatile; };
    struct static_m        { static int child_count(); };
    struct void_m          { void child_count() const; };
    struct field_m         { int child_count; };
    struct fnptr_m         { int (*child_count)(); };
    struct functor_m       { np_functor child_count; };
    struct mut_functor_m   { np_mut_functor child_count; };
    struct private_m       { private: int child_count() const; };
    struct protected_m     { protected: int child_count() const; };
    struct deleted_m       { int child_count() const = delete; };
    struct arg_int_m       { int child_count(int) const; };
    struct arg_str_m       { int child_count(const std::string&); };
    struct arg_int_int_m   { int child_count(int, int); };
    struct arg_ptr_m       { int child_count(int*) const; };
    struct default_arg_m   { int child_count(int = 0) const; };
    struct variadic_m
        { template<typename... A> int child_count(A&&...) const; };
    struct tmpl_m
        { template<typename T = int> int child_count() const; };
    struct tmpl_nodef_m    { template<typename T> int child_count() const; };
    struct overloaded_m    { int child_count() const; int child_count(int); };
    struct mut_and_const_m { int child_count(); int child_count() const; };
    struct abstract_m      { virtual int child_count() const = 0; };
    struct virtual_m       { virtual int child_count() const; };
    struct ref_ret_m       { int& child_count() const; };
    struct noexcept_m      { int child_count() const noexcept; };
    struct empty           {  };
    struct derived_m : const_m {};
    struct derived_mut_m : mut_m {};
    struct derived_private_m : private_m {};
    struct base_a { int child_count() const; };
    struct base_b { int child_count() const; };
    struct ambiguous_m : base_a, base_b {};
    union  union_m { int child_count() const; int x; };
}  // fx_child_count

// fx_version: the shapes, with a method or member named version
namespace fx_version {
    struct const_m         { int version() const; };
    struct mut_m           { int version(); };
    struct lref_m          { int version() &; };
    struct rref_m          { int version() &&; };
    struct clref_m         { int version() const &; };
    struct crref_m         { int version() const &&; };
    struct cv_m            { int version() const volatile; };
    struct volatile_m      { int version() volatile; };
    struct static_m        { static int version(); };
    struct void_m          { void version() const; };
    struct field_m         { int version; };
    struct fnptr_m         { int (*version)(); };
    struct functor_m       { np_functor version; };
    struct mut_functor_m   { np_mut_functor version; };
    struct private_m       { private: int version() const; };
    struct protected_m     { protected: int version() const; };
    struct deleted_m       { int version() const = delete; };
    struct arg_int_m       { int version(int) const; };
    struct arg_str_m       { int version(const std::string&); };
    struct arg_int_int_m   { int version(int, int); };
    struct arg_ptr_m       { int version(int*) const; };
    struct default_arg_m   { int version(int = 0) const; };
    struct variadic_m
        { template<typename... A> int version(A&&...) const; };
    struct tmpl_m          { template<typename T = int> int version() const; };
    struct tmpl_nodef_m    { template<typename T> int version() const; };
    struct overloaded_m    { int version() const; int version(int); };
    struct mut_and_const_m { int version(); int version() const; };
    struct abstract_m      { virtual int version() const = 0; };
    struct virtual_m       { virtual int version() const; };
    struct ref_ret_m       { int& version() const; };
    struct noexcept_m      { int version() const noexcept; };
    struct empty           {  };
    struct derived_m : const_m {};
    struct derived_mut_m : mut_m {};
    struct derived_private_m : private_m {};
    struct base_a { int version() const; };
    struct base_b { int version() const; };
    struct ambiguous_m : base_a, base_b {};
    union  union_m { int version() const; int x; };
}  // fx_version

// fx_add_child: the shapes, with a method or member named add_child
namespace fx_add_child {
    struct const_m         { int add_child() const; };
    struct mut_m           { int add_child(); };
    struct lref_m          { int add_child() &; };
    struct rref_m          { int add_child() &&; };
    struct clref_m         { int add_child() const &; };
    struct crref_m         { int add_child() const &&; };
    struct cv_m            { int add_child() const volatile; };
    struct volatile_m      { int add_child() volatile; };
    struct static_m        { static int add_child(); };
    struct void_m          { void add_child() const; };
    struct field_m         { int add_child; };
    struct fnptr_m         { int (*add_child)(); };
    struct functor_m       { np_functor add_child; };
    struct mut_functor_m   { np_mut_functor add_child; };
    struct private_m       { private: int add_child() const; };
    struct protected_m     { protected: int add_child() const; };
    struct deleted_m       { int add_child() const = delete; };
    struct arg_int_m       { int add_child(int) const; };
    struct arg_str_m       { int add_child(const std::string&); };
    struct arg_int_int_m   { int add_child(int, int); };
    struct arg_ptr_m       { int add_child(int*) const; };
    struct default_arg_m   { int add_child(int = 0) const; };
    struct variadic_m
        { template<typename... A> int add_child(A&&...) const; };
    struct tmpl_m
        { template<typename T = int> int add_child() const; };
    struct tmpl_nodef_m    { template<typename T> int add_child() const; };
    struct overloaded_m    { int add_child() const; int add_child(int); };
    struct mut_and_const_m { int add_child(); int add_child() const; };
    struct abstract_m      { virtual int add_child() const = 0; };
    struct virtual_m       { virtual int add_child() const; };
    struct ref_ret_m       { int& add_child() const; };
    struct noexcept_m      { int add_child() const noexcept; };
    struct empty           {  };
    struct derived_m : const_m {};
    struct derived_mut_m : mut_m {};
    struct derived_private_m : private_m {};
    struct base_a { int add_child() const; };
    struct base_b { int add_child() const; };
    struct ambiguous_m : base_a, base_b {};
    union  union_m { int add_child() const; int x; };
}  // fx_add_child

// fx_name: the shapes, with a method or member named name
namespace fx_name {
    struct const_m         { int name() const; };
    struct mut_m           { int name(); };
    struct lref_m          { int name() &; };
    struct rref_m          { int name() &&; };
    struct clref_m         { int name() const &; };
    struct crref_m         { int name() const &&; };
    struct cv_m            { int name() const volatile; };
    struct volatile_m      { int name() volatile; };
    struct static_m        { static int name(); };
    struct void_m          { void name() const; };
    struct field_m         { int name; };
    struct fnptr_m         { int (*name)(); };
    struct functor_m       { np_functor name; };
    struct mut_functor_m   { np_mut_functor name; };
    struct private_m       { private: int name() const; };
    struct protected_m     { protected: int name() const; };
    struct deleted_m       { int name() const = delete; };
    struct arg_int_m       { int name(int) const; };
    struct arg_str_m       { int name(const std::string&); };
    struct arg_int_int_m   { int name(int, int); };
    struct arg_ptr_m       { int name(int*) const; };
    struct default_arg_m   { int name(int = 0) const; };
    struct variadic_m      { template<typename... A> int name(A&&...) const; };
    struct tmpl_m          { template<typename T = int> int name() const; };
    struct tmpl_nodef_m    { template<typename T> int name() const; };
    struct overloaded_m    { int name() const; int name(int); };
    struct mut_and_const_m { int name(); int name() const; };
    struct abstract_m      { virtual int name() const = 0; };
    struct virtual_m       { virtual int name() const; };
    struct ref_ret_m       { int& name() const; };
    struct noexcept_m      { int name() const noexcept; };
    struct empty           {  };
    struct derived_m : const_m {};
    struct derived_mut_m : mut_m {};
    struct derived_private_m : private_m {};
    struct base_a { int name() const; };
    struct base_b { int name() const; };
    struct ambiguous_m : base_a, base_b {};
    union  union_m { int name() const; int x; };
}  // fx_name

// fx_node_type: the shapes, with and without a member named node_type
namespace fx_node_type {
    struct pub        { typedef int node_type; };
    struct priv       { private: typedef int node_type; };
    struct prot       { protected: typedef int node_type; };
    struct nested     { struct node_type {}; };
    struct value      { static const int node_type = 0; };
    struct fn         { int node_type(); };
    struct tmpl_alias { template<typename> using node_type = int; };
    struct enum_t     { enum node_type { a }; };
    struct void_t     { typedef void node_type; };
    struct ref_t      { typedef int& node_type; };
    struct empty      {  };
    struct derived : pub {};
    struct derived_priv : priv {};
    struct base_a { typedef int  node_type; };
    struct base_b { typedef long node_type; };
    struct ambiguous : base_a, base_b {};
}  // fx_node_type

}  // np


// ===========================================================================
// II.  The answers
// ===========================================================================
//   A block of rows sets D_NP_PROBE (the probe), D_NP_FAMILY (the shapes
// it is asked of) and D_NP_ARGS (the argument types after Type, with a
// leading comma, or nothing).  Each answer asserts three things: the
// probe's value; that the probe is a std::integral_constant<bool, value>
// (std::true_type or std::false_type); and, where the language has variable
// templates, that its _v companion agrees.

// D_NP_CAT
//   macro: pastes its arguments after expanding them.
#define D_NP_CAT_(A, B)  A##B
#define D_NP_CAT(A, B)   D_NP_CAT_(A, B)

// D_NP_PROBE_OF
//   macro: the probe asked of TYPE, with the block's arguments.
#define D_NP_PROBE_OF(TYPE)                                                  \
    ::djinterp::D_NP_PROBE<TYPE D_NP_ARGS>

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    #define D_NP_ANSWER_V(TYPE)                                              \
        static_assert(::djinterp::D_NP_CAT(D_NP_PROBE, _v)<TYPE D_NP_ARGS> ==  \
                          D_NP_PROBE_OF(TYPE)::value,                       \
                      "the _v companion agrees with the probe");
#else
    #define D_NP_ANSWER_V(TYPE)  /* no variable templates */
#endif

// D_NP_ANSWER
//   macro: the probe's answer for TYPE is EXPECTED (1 or 0).
#define D_NP_ANSWER(TYPE, EXPECTED)                                          \
    static_assert(D_NP_PROBE_OF(TYPE)::value == ((EXPECTED) != 0),           \
                  "the probe answers as it did before it moved");           \
    static_assert(std::is_base_of<                                           \
                      std::integral_constant<bool, ((EXPECTED) != 0)>,       \
                      D_NP_PROBE_OF(TYPE) >::value,                          \
                  "the probe is an integral_constant of its answer");       \
    D_NP_ANSWER_V(TYPE)                                                      \
    ++count;

// D_NP_SHAPE
//   macro: the shape SHAPE of the block's family.
#define D_NP_SHAPE(SHAPE)  ::djinterp::testing::np::D_NP_FAMILY::SHAPE

// D_NP_ROW8
//   macro: the answers for SHAPE as T, const T, volatile T, T&, const T&,
// T&&, const T&& and T*, in that order.
#define D_NP_ROW8(SHAPE, V_T, V_CT, V_VT, V_LR, V_CLR, V_RR, V_CRR, V_PTR) \
    D_NP_ANSWER(D_NP_SHAPE(SHAPE),            V_T)                           \
    D_NP_ANSWER(const D_NP_SHAPE(SHAPE),      V_CT)                          \
    D_NP_ANSWER(volatile D_NP_SHAPE(SHAPE),   V_VT)                          \
    D_NP_ANSWER(D_NP_SHAPE(SHAPE)&,           V_LR)                          \
    D_NP_ANSWER(const D_NP_SHAPE(SHAPE)&,     V_CLR)                         \
    D_NP_ANSWER(D_NP_SHAPE(SHAPE)&&,          V_RR)                          \
    D_NP_ANSWER(const D_NP_SHAPE(SHAPE)&&,    V_CRR)                         \
    D_NP_ANSWER(D_NP_SHAPE(SHAPE)*,           V_PTR)

// D_NP_ROW1
//   macro: the answer for one TYPE, spelled in full.
#define D_NP_ROW1(TYPE, V)  D_NP_ANSWER(TYPE, V)


// node_probes_tests_answers
//   function: asserts every answer at compile time; returns how many.
int node_probes_tests_answers();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_NODE_PROBES_TESTS_HPP
