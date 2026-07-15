// djinterp
#include "test_traits_tests.hpp"

// std
#include <type_traits>


NS_DJINTERP
NS_TESTING


// the cv-ref cells live in test_traits.hpp's own `internal` namespace.  They are
// tested here anyway: they are what makes trait_across_cvref TOTAL, the totality
// is the property most easily lost, and a change to a cell would show up nowhere
// else until some suite handed the matrix a function type and the build stopped.
namespace tt_cells = ::djinterp::test::internal;


/*
tests_cvref_cells
  The seven cell aliases - and the property the header claims for them: each is
  TOTAL.  add_const, add_lvalue_reference and their siblings are no-ops on the
  types that cannot take the qualifier, so every cell is well-formed for every
  type a trait might be handed, and trait_across_cvref never has to special-case
  anything.

  That is a strong claim, and it is the one that lets the matrix be run over
  void, over function types and over references without a guard.  It is checked
  here directly on the types that would break a naive spelling.

  Tests the following:
  - the cells produce the types they say they do, for an ordinary object type
  - const on a FUNCTION type is a no-op (a const-qualified function type is not a
    thing; std::add_const is specified to return the type unchanged)
  - a reference to VOID is not a thing either, and add_lvalue_reference returns
    void unchanged
  - cv-qualifying a REFERENCE is a no-op (the qualifier would apply to the
    reference, which cannot be qualified)
  - REFERENCE COLLAPSING: adding an lvalue reference to `int&` is `int&`, and
    adding an rvalue reference to `int&` is ALSO `int&` - so the lvalue_ref and
    rvalue_ref cells of a report about a reference type are the SAME type, and
    the matrix necessarily agrees on them.  Worth knowing before reading a report
    about a reference and concluding the trait is reference-agnostic
*/
bool
tests_cvref_cells()
{
    bool ok = true;

    // the ordinary case
    D_TT_CHECK((std::is_same<tt_cells::cell_const<int>, const int>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_volatile<int>,
                             volatile int>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_cv<int>,
                             const volatile int>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_lvalue_ref<int>, int&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_const_lvalue_ref<int>,
                             const int&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_rvalue_ref<int>, int&&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_const_rvalue_ref<int>,
                             const int&&>::value));

    // TOTALITY.  a function type cannot be const-qualified
    D_TT_CHECK((std::is_same<tt_cells::cell_const<int(int)>,
                             int(int)>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_cv<int(int)>, int(int)>::value));

    // void cannot be referenced
    D_TT_CHECK((std::is_same<tt_cells::cell_lvalue_ref<void>, void>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_rvalue_ref<void>, void>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_const<void>,
                             const void>::value));

    // a reference cannot be cv-qualified
    D_TT_CHECK((std::is_same<tt_cells::cell_const<int&>, int&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_volatile<int&>, int&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_cv<int&>, int&>::value));

    // REFERENCE COLLAPSING - the lvalue and rvalue cells of a reference type
    // are the same type, so a report about one necessarily agrees on both
    D_TT_CHECK((std::is_same<tt_cells::cell_lvalue_ref<int&>, int&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_rvalue_ref<int&>, int&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_lvalue_ref<int&&>, int&>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_rvalue_ref<int&&>, int&&>::value));

    // and an array, which takes every qualifier and every reference
    D_TT_CHECK((std::is_same<tt_cells::cell_const<int[3]>,
                             const int[3]>::value));
    D_TT_CHECK((std::is_same<tt_cells::cell_lvalue_ref<int[3]>,
                             int (&)[3]>::value));

    return ok;
}

/*
tests_cvref_report_accessors
  cvref_report - the aggregate, and its four accessors, exercised on HAND-BUILT
  reports so that the accessors are tested independently of the thing that fills
  them.

  Tests the following:
  - it is an aggregate and a LITERAL type: a `constexpr cvref_report` can be
    brace-initialized, and every accessor is usable during constant evaluation.
    That is what lets a suite hold the whole matrix as a constexpr and still
    print the failing cell's name at run time
  - all(), none() and agrees() on the all-true, all-false and mixed reports
  - THE IDENTITY: agrees() is equivalent to (all() || none()).  agrees() means
    every cell equals the bare cell; if bare is true they are all true, and if it
    is false they are all false.  A trait that is uniformly FALSE therefore
    "agrees" just as well as one that is uniformly true - which is why the header
    tells a caller to pair agrees() with all() or none() to pin the polarity, and
    why a suite that checked only agrees() would call a trait cv-ref agnostic on
    the strength of it answering no to everything
*/
bool
tests_cvref_report_accessors()
{
    bool ok = true;

    D_CONSTEXPR cvref_report all_true{
        true, true, true, true, true, true, true, true};
    D_CONSTEXPR cvref_report all_false{
        false, false, false, false, false, false, false, false};
    D_CONSTEXPR cvref_report mixed{
        true, true, false, false, true, true, true, true};

    // literal type: every accessor is a constant expression
    D_CONSTEXPR bool ct_all    = all_true.all();
    D_CONSTEXPR bool ct_none   = all_false.none();
    D_CONSTEXPR bool ct_agrees = mixed.agrees();

    D_TT_CHECK(ct_all);
    D_TT_CHECK(ct_none);
    D_TT_CHECK(!ct_agrees);

    // all
    D_TT_CHECK(all_true.all());
    D_TT_CHECK(!all_false.all());
    D_TT_CHECK(!mixed.all());

    // none
    D_TT_CHECK(!all_true.none());
    D_TT_CHECK(all_false.none());
    D_TT_CHECK(!mixed.none());

    // agrees
    D_TT_CHECK(all_true.agrees());
    D_TT_CHECK(all_false.agrees());     // uniformly FALSE also "agrees"
    D_TT_CHECK(!mixed.agrees());

    // THE IDENTITY: agrees() <=> (all() || none())
    D_TT_CHECK(all_true.agrees() == (all_true.all() || all_true.none()));
    D_TT_CHECK(all_false.agrees() == (all_false.all() || all_false.none()));
    D_TT_CHECK(mixed.agrees() == (mixed.all() || mixed.none()));

    // ...which is why agrees() alone is not enough to call a trait agnostic
    D_TT_CHECK(all_false.agrees() && !all_false.all());

    // a single flipped cell anywhere breaks agreement
    D_CONSTEXPR cvref_report last_only{
        true, true, true, true, true, true, true, false};

    D_TT_CHECK(!last_only.agrees());
    D_TT_CHECK(!last_only.all());
    D_TT_CHECK(!last_only.none());

    return ok;
}

/*
tests_cvref_report_first_disagreement
  first_disagreement() is a seven-way ternary chain with an eighth fall-through,
  and it is the single most valuable thing cvref_report does: it is what turns a
  failed cv-ref check from "something is wrong" into "`const volatile _Type` is
  wrong", which is the difference between a report line and a debugging session.

  Eight branches.  Here they are exercised on hand-built reports, one per branch,
  each with exactly ONE cell flipped, in the chain's own order - so a chain that
  tested the cells in the wrong order, or dropped one, is caught by name.

  Tests the following:
  - each of the seven named cells is reported when it is the one that differs
  - the FIRST disagreement wins when several differ - the chain is ordered, and
    the order is part of the contract
  - nullptr when every cell agrees, for a uniformly-true report AND for a
    uniformly-false one
*/
bool
tests_cvref_report_first_disagreement()
{
    bool ok = true;

    // one cell flipped, in chain order.  bare = true throughout.
    D_CONSTEXPR cvref_report c1{true, false, true, true, true, true, true, true};
    D_CONSTEXPR cvref_report c2{true, true, false, true, true, true, true, true};
    D_CONSTEXPR cvref_report c3{true, true, true, false, true, true, true, true};
    D_CONSTEXPR cvref_report c4{true, true, true, true, false, true, true, true};
    D_CONSTEXPR cvref_report c5{true, true, true, true, true, false, true, true};
    D_CONSTEXPR cvref_report c6{true, true, true, true, true, true, false, true};
    D_CONSTEXPR cvref_report c7{true, true, true, true, true, true, true, false};

    D_TT_CHECK(tt_names_match("const _Type", c1.first_disagreement()));
    D_TT_CHECK(tt_names_match("volatile _Type", c2.first_disagreement()));
    D_TT_CHECK(tt_names_match("const volatile _Type",
                              c3.first_disagreement()));
    D_TT_CHECK(tt_names_match("_Type&", c4.first_disagreement()));
    D_TT_CHECK(tt_names_match("const _Type&", c5.first_disagreement()));
    D_TT_CHECK(tt_names_match("_Type&&", c6.first_disagreement()));
    D_TT_CHECK(tt_names_match("const _Type&&", c7.first_disagreement()));

    // the eighth branch: nullptr, for both polarities
    D_CONSTEXPR cvref_report all_true{
        true, true, true, true, true, true, true, true};
    D_CONSTEXPR cvref_report all_false{
        false, false, false, false, false, false, false, false};

    D_TT_CHECK(all_true.first_disagreement() == nullptr);
    D_TT_CHECK(all_false.first_disagreement() == nullptr);
    D_TT_CHECK(tt_names_match(nullptr, all_true.first_disagreement()));

    // FIRST wins.  every cell after the second differs too, and it still names
    // the second
    D_CONSTEXPR cvref_report many{
        true, true, false, false, false, false, false, false};

    D_TT_CHECK(tt_names_match("volatile _Type", many.first_disagreement()));

    // ...and with bare = FALSE, the same chain runs against the opposite
    // baseline: here `const _Type` is the odd one out because it is TRUE
    D_CONSTEXPR cvref_report inverted{
        false, true, false, false, false, false, false, false};

    D_TT_CHECK(tt_names_match("const _Type", inverted.first_disagreement()));

    // it is constexpr
    D_CONSTEXPR const char* ct_name = c4.first_disagreement();

    D_TT_CHECK(tt_names_match("_Type&", ct_name));

    return ok;
}

/*
tests_trait_across_cvref
  trait_across_cvref - the matrix, filled from a real trait rather than by hand.
  Eight branches again, and this time each is driven by a trait that is BLIND to
  exactly one qualification, so the whole path - the cells, the eight
  instantiations, the report, the chain - is exercised end to end.

  Tests the following:
  - a cv-ref agnostic trait agrees, and all() holds (it is uniformly TRUE for int)
  - a trait that is uniformly FALSE also agrees, and none() holds - the polarity
    check the header warns about
  - each of the seven named cells is reported when the trait is blind to exactly
    that one.  hates_const disagrees first at `const _Type`, hates_volatile at
    `volatile _Type`, hates_cv only when BOTH are present, and so on down the
    chain
  - the report is constexpr: the whole matrix is computed during constant
    evaluation and the failing cell's name survives to run time
*/
bool
tests_trait_across_cvref()
{
    bool ok = true;

    // agnostic: agrees, and uniformly TRUE
    D_TT_FIRST(nullptr, cvref_blind, int);
    D_TT_CHECK((trait_across_cvref<cvref_blind, int>().agrees()));
    D_TT_CHECK((trait_across_cvref<cvref_blind, int>().all()));
    D_TT_CHECK((!trait_across_cvref<cvref_blind, int>().none()));

    // uniformly FALSE: also agrees.  the polarity trap, live
    D_TT_FIRST(nullptr, cvref_blind, char);
    D_TT_CHECK((trait_across_cvref<cvref_blind, char>().agrees()));
    D_TT_CHECK((trait_across_cvref<cvref_blind, char>().none()));
    D_TT_CHECK((!trait_across_cvref<cvref_blind, char>().all()));

    // the seven cells, one blind trait each
    D_TT_FIRST("const _Type",           hates_const,            int);
    D_TT_FIRST("volatile _Type",        hates_volatile,         int);
    D_TT_FIRST("const volatile _Type",  hates_cv,               int);
    D_TT_FIRST("_Type&",                hates_lvalue_ref,       int);
    D_TT_FIRST("const _Type&",          hates_const_lvalue_ref, int);
    D_TT_FIRST("_Type&&",               hates_rvalue_ref,       int);
    D_TT_FIRST("const _Type&&",         hates_const_rvalue_ref, int);

    // none of them agrees, and none is all-or-nothing
    D_TT_CHECK((!trait_across_cvref<hates_const, int>().agrees()));
    D_TT_CHECK((!trait_across_cvref<hates_cv, int>().all()));
    D_TT_CHECK((!trait_across_cvref<hates_cv, int>().none()));

    // the cells really do say what the trait was built to say
    D_CONSTEXPR cvref_report cv = trait_across_cvref<hates_cv, int>();

    D_TT_CHECK(cv.bare);
    D_TT_CHECK(cv.with_const);       // const ALONE is tolerated
    D_TT_CHECK(cv.with_volatile);    // volatile ALONE is tolerated
    D_TT_CHECK(!cv.with_cv);         // ...both together are not
    D_TT_CHECK(cv.lvalue_ref);

    // the matrix is total: it runs over void, functions and arrays without a
    // guard, because the cells are total
    D_TT_CHECK((trait_across_cvref<always_true, void>().all()));
    D_TT_CHECK((trait_across_cvref<always_false,
                                   fixtures::function_type>().none()));
    D_TT_CHECK((trait_across_cvref<always_true,
                                   fixtures::array_type>().all()));
    D_TT_CHECK((trait_across_cvref<always_true,
                                   fixtures::incomplete>().all()));

    return ok;
}

/*
tests_trait_ignores_cvref
  trait_ignores_cvref - the report's agrees(), lifted to a trait so it can be
  pinned with D_TEST_STATIC or folded into a larger constant expression.
  Tests the following:
  - it agrees with trait_across_cvref(...).agrees(), for every subject
  - a blind trait ignores cv-ref; each of the seven one-cell-blind traits does
    not
  - it is itself a bool trait, so it composes
  - and the polarity trap it inherits: a uniformly-false trait "ignores cv-ref"
*/
bool
tests_trait_ignores_cvref()
{
    bool ok = true;

    #define D_TT_IGNORES_IS_AGREES(TRAIT, ...)                                \
        D_TT_CHECK((trait_ignores_cvref<TRAIT, __VA_ARGS__>::value ==         \
                    trait_across_cvref<TRAIT, __VA_ARGS__>().agrees()))

    D_TT_IGNORES_IS_AGREES(cvref_blind,            int);
    D_TT_IGNORES_IS_AGREES(cvref_blind,            char);
    D_TT_IGNORES_IS_AGREES(hates_const,            int);
    D_TT_IGNORES_IS_AGREES(hates_volatile,         int);
    D_TT_IGNORES_IS_AGREES(hates_cv,               int);
    D_TT_IGNORES_IS_AGREES(hates_lvalue_ref,       int);
    D_TT_IGNORES_IS_AGREES(hates_const_lvalue_ref, int);
    D_TT_IGNORES_IS_AGREES(hates_rvalue_ref,       int);
    D_TT_IGNORES_IS_AGREES(hates_const_rvalue_ref, int);
    D_TT_IGNORES_IS_AGREES(always_true,            void);

    #undef D_TT_IGNORES_IS_AGREES

    // the answers themselves
    D_TT_CHECK((trait_ignores_cvref<cvref_blind, int>::value));
    D_TT_CHECK((trait_ignores_cvref<always_true, int>::value));
    D_TT_CHECK((trait_ignores_cvref<always_false, int>::value));

    D_TT_CHECK((!trait_ignores_cvref<hates_const, int>::value));
    D_TT_CHECK((!trait_ignores_cvref<hates_volatile, int>::value));
    D_TT_CHECK((!trait_ignores_cvref<hates_cv, int>::value));
    D_TT_CHECK((!trait_ignores_cvref<hates_lvalue_ref, int>::value));
    D_TT_CHECK((!trait_ignores_cvref<hates_const_lvalue_ref, int>::value));
    D_TT_CHECK((!trait_ignores_cvref<hates_rvalue_ref, int>::value));
    D_TT_CHECK((!trait_ignores_cvref<hates_const_rvalue_ref, int>::value));

    // THE POLARITY TRAP.  always_false ignores cv-ref - it answers no to
    // everything, uniformly - and a suite that read that as "agnostic" would be
    // endorsing a trait that recognizes nothing
    D_TT_CHECK((trait_ignores_cvref<always_false, int>::value));
    D_TT_CHECK((trait_across_cvref<always_false, int>().none()));

    // it is a bool trait, so it composes
    D_TT_CHECK(is_bool_trait<trait_ignores_cvref<cvref_blind, int>>::value);
    D_TT_CHECK(is_bool_trait<trait_ignores_cvref<hates_const, int>>::value);

    return ok;
}

/*
tests_cvref_value_companion
  trait_ignores_cvref_v.
  Tests the following:
  - the shorthand carries the same value as the trait, on an agreeing and a
    disagreeing subject
  - it is typed const bool
*/
bool
tests_cvref_value_companion()
{
    bool ok = true;

    D_TT_CHECK((trait_ignores_cvref_v<cvref_blind, int>) ==
               (trait_ignores_cvref<cvref_blind, int>::value));
    D_TT_CHECK((trait_ignores_cvref_v<hates_const, int>) ==
               (trait_ignores_cvref<hates_const, int>::value));
    D_TT_CHECK((trait_ignores_cvref_v<always_false, int>) ==
               (trait_ignores_cvref<always_false, int>::value));

    D_TT_CHECK((trait_ignores_cvref_v<cvref_blind, int>));
    D_TT_CHECK((!trait_ignores_cvref_v<hates_const_rvalue_ref, int>));

    D_TT_CHECK((std::is_same<
                    decltype(trait_ignores_cvref_v<cvref_blind, int>),
                    const bool>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
