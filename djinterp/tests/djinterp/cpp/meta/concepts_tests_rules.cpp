/*******************************************************************************
* djinterp [test]                                       concepts_tests_rules.cpp
*
*   Tests for concepts.hpp section II.5: class-definition rule concepts.
* Covers: follows_rule_of_zero_c, follows_rule_of_three_c,
* follows_rule_of_five_c.
*
*   Important semantic difference from the type_traits side:
*   The concepts in section II.5 are defined as conjunctions of
* `std::is_*_constructible` / `std::is_*_assignable` /
* `std::is_destructible` checks, which are *existence* predicates -
* they ask "is this operation a valid expression?", not "did the user
* author it?".  So:
*   - rule_of_zero_c: every special member is *trivially* implemented
*     (compiler-generated, no user prose).
*   - rule_of_three_c: copy ctor, copy assign, dtor all exist (any
*     accessibility / triviality).  rule_of_five_class also satisfies
*     this because the five-member shape is a superset.
*   - rule_of_five_c: all five exist.
*
*   The follows_rule_of_three_c concept is intentionally lenient about
* whether move operations are provided - that's what distinguishes
* three from five.  But because *the existence checks for copy
* members trivially imply existence checks for move members in many
* compiler-generated cases* (move falls back to copy when no move
* exists), a rule-of-three class will often also satisfy rule-of-five
* unless moves are explicitly deleted.
*
*
* path:      /tests/djinterp/cpp/meta/concepts_tests_rules.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


namespace {

    // DFiveDeleted
    //   helper: all five special members explicitly deleted.  Should
    // satisfy none of the rule-of-N concepts.
    class DFiveDeleted
    {
    public:
        DFiveDeleted() = delete;
        DFiveDeleted(
            const DFiveDeleted&
        ) = delete;
        DFiveDeleted(
            DFiveDeleted&&
        ) = delete;
        DFiveDeleted& operator=(
            const DFiveDeleted&
        ) = delete;
        DFiveDeleted& operator=(
            DFiveDeleted&&
        ) = delete;
        ~DFiveDeleted() = delete;
    };

    // DCopyDeleted
    //   helper: copy operations explicitly deleted, others defaulted.
    // Negative case for rule_of_three_c / rule_of_five_c.
    class DCopyDeleted
    {
    public:
        DCopyDeleted() = default;
        DCopyDeleted(
            const DCopyDeleted&
        ) = delete;
        DCopyDeleted(
            DCopyDeleted&&
        ) = default;
        DCopyDeleted& operator=(
            const DCopyDeleted&
        ) = delete;
        DCopyDeleted& operator=(
            DCopyDeleted&&
        ) = default;
        ~DCopyDeleted() = default;
    };

}  // unnamed namespace


NS_DJINTERP_TEST


void concepts_tests_rules(
    test_handler& _test_handler,
    test_option _options...
)
{
    using fixtures::rule_of_zero_class;
    using fixtures::rule_of_three_class;
    using fixtures::rule_of_five_class;
    using fixtures::nontrivial_class;
    using fixtures::trivial_class;
    using fixtures::polymorphic_class;

    // -------------------------------------------------------------------
    // follows_rule_of_zero_c
    // -------------------------------------------------------------------
    //   Defined as: every special member is *trivially* implemented.
    // The canonical positive case is a class with no user-provided
    // special members.

    DT_CONCEPT    (follows_rule_of_zero_c<rule_of_zero_class>);
    DT_CONCEPT    (follows_rule_of_zero_c<trivial_class>);
    DT_CONCEPT    (follows_rule_of_zero_c<int>);           // primitives qualify

    //   user-provided non-trivial special members defeat rule-of-zero
    DT_NOT_CONCEPT(follows_rule_of_zero_c<nontrivial_class>);
    DT_NOT_CONCEPT(follows_rule_of_zero_c<rule_of_three_class>);
    DT_NOT_CONCEPT(follows_rule_of_zero_c<rule_of_five_class>);

    //   polymorphic types: the implicit virtual dtor is NOT trivial,
    // so rule-of-zero fails
    DT_NOT_CONCEPT(follows_rule_of_zero_c<polymorphic_class>);

    //   five-deleted: no special members exist - the trivially_*
    // predicates fold to false because the operations themselves
    // don't exist
    DT_NOT_CONCEPT(follows_rule_of_zero_c<DFiveDeleted>);

    // -------------------------------------------------------------------
    // follows_rule_of_three_c
    // -------------------------------------------------------------------
    //   Defined as: copy ctor + copy assign + dtor exist.

    DT_CONCEPT    (follows_rule_of_three_c<rule_of_three_class>);
    DT_CONCEPT    (follows_rule_of_three_c<rule_of_five_class>);
    DT_CONCEPT    (follows_rule_of_three_c<rule_of_zero_class>);   // copies are implicit
    DT_CONCEPT    (follows_rule_of_three_c<trivial_class>);
    DT_CONCEPT    (follows_rule_of_three_c<nontrivial_class>);
    DT_CONCEPT    (follows_rule_of_three_c<int>);

    //   copy deleted: rule_of_three breaks
    DT_NOT_CONCEPT(follows_rule_of_three_c<DCopyDeleted>);
    DT_NOT_CONCEPT(follows_rule_of_three_c<DFiveDeleted>);

    // -------------------------------------------------------------------
    // follows_rule_of_five_c
    // -------------------------------------------------------------------
    //   Defined as: copy ctor + move ctor + copy assign + move assign +
    // dtor all exist.

    DT_CONCEPT    (follows_rule_of_five_c<rule_of_five_class>);
    DT_CONCEPT    (follows_rule_of_five_c<rule_of_zero_class>);
    DT_CONCEPT    (follows_rule_of_five_c<trivial_class>);
    DT_CONCEPT    (follows_rule_of_five_c<int>);

    //   note on rule_of_three_class: it has user-provided copy ctor,
    // copy assign, and dtor, which SUPPRESS the implicit move ctor /
    // move assign.  In that suppression scenario, the language falls
    // back: an rvalue is bound through the copy ctor.  So
    // `std::is_move_constructible<rule_of_three_class>` is true (via
    // copy fallback) and follows_rule_of_five_c is satisfied here too.
    DT_CONCEPT    (follows_rule_of_five_c<rule_of_three_class>);

    //   copy deleted - moves still work, but copy ctor / copy assign
    // don't, so rule_of_five fails
    DT_NOT_CONCEPT(follows_rule_of_five_c<DCopyDeleted>);
    DT_NOT_CONCEPT(follows_rule_of_five_c<DFiveDeleted>);

    // -------------------------------------------------------------------
    // Inclusion invariants
    // -------------------------------------------------------------------
    //   rule_of_zero => rule_of_five (a trivially-defined class still
    // has all five operations)
    DT_CONCEPT(!follows_rule_of_zero_c<rule_of_zero_class>
              || follows_rule_of_five_c<rule_of_zero_class>);
    DT_CONCEPT(!follows_rule_of_zero_c<trivial_class>
              || follows_rule_of_five_c<trivial_class>);

    //   rule_of_five => rule_of_three (three is a subset of five)
    DT_CONCEPT(!follows_rule_of_five_c<rule_of_five_class>
              || follows_rule_of_three_c<rule_of_five_class>);
    DT_CONCEPT(!follows_rule_of_five_c<rule_of_zero_class>
              || follows_rule_of_three_c<rule_of_zero_class>);

    return;
}


NS_END  // test
NS_END  // djinterp
