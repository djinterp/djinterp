// djinterp
#include "test_traits_tests.hpp"

// std
#include <type_traits>


NS_DJINTERP
NS_TESTING


/*
tests_is_bool_trait_positive
  is_bool_trait - the shape check.  The types that pass.
  Tests the following:
  - std::true_type and std::false_type
  - anything derived from them, which is what trait_detect.hpp emits
  - a standard trait
  - a trait with a FALSE value: is_bool_trait is about the SHAPE, not the answer,
    and a well-formed trait that says no is still well-formed.  Easy to get
    backwards, and a check that only ever tried true traits would not notice
*/
bool
tests_is_bool_trait_positive()
{
    bool ok = true;

    D_TT_CHECK(is_bool_trait<std::true_type>::value);
    D_TT_CHECK(is_bool_trait<std::false_type>::value);
    D_TT_CHECK(is_bool_trait<::djinterp::bool_constant<true>>::value);
    D_TT_CHECK(is_bool_trait<::djinterp::bool_constant<false>>::value);

    D_TT_CHECK(is_bool_trait<std::is_same<int, int>>::value);
    D_TT_CHECK(is_bool_trait<std::is_same<int, char>>::value);
    D_TT_CHECK(is_bool_trait<std::is_void<void>>::value);

    // the framework's own emitted traits
    D_TT_CHECK(is_bool_trait<has_member_value_type<sized>>::value);
    D_TT_CHECK(is_bool_trait<has_member_value_type<plain>>::value);

    // the suite's own well-shaped subjects, true and false alike
    D_TT_CHECK(is_bool_trait<always_true<int>>::value);
    D_TT_CHECK(is_bool_trait<always_false<int>>::value);
    D_TT_CHECK(is_bool_trait<is_int<int>>::value);
    D_TT_CHECK(is_bool_trait<is_int<char>>::value);
    D_TT_CHECK(is_bool_trait<half_bad<int>>::value);

    return ok;
}

/*
tests_is_bool_trait_sink
  is_bool_trait is a void_t sink over five members, and then three predicates
  over what the sink found.  This test is the SINK: the ways a trait can fail
  before any predicate is reached.

  Tests the following, one subject per failure, so that a false is attributable:
  - `value_only`      : no value_type, no ::type, no conversion, no call operator.
                        The sink fails at its first name.  This is the exact shape
                        of is_bounded_container and is_unbounded_container in
                        bounded_container_traits.hpp - the defect this trait was
                        written to find
  - `private_base`    : derives from true_type PRIVATELY.  std::is_base_of does
                        not check accessibility and would say yes; the sink's
                        static_cast<bool> cannot reach the inherited conversion,
                        and THAT is what rejects it.  The sink is doing work the
                        predicates cannot
  - `ambiguous_base`  : derives from bool_constant<true> twice.  is_base_of again
                        says yes; the inherited conversion is ambiguous, and the
                        sink again is what rejects it
  - `nonstatic_value` : shadows the static `value` with a non-static member, so
                        `_Trait::value` is an id-expression naming a non-static
                        member outside a member function - ill-formed, and the
                        sink's integral_constant<bool, value> cell fails
  - `wide_valued`     : value is int 3, which does not fit in a bool.
                        `integral_constant<bool, 3>` is a NARROWING conversion in
                        a converted constant expression - ill-formed - so the sink
                        rejects it before value_type is ever looked at

  Note the last two against tests_is_bool_trait_predicates: `int_valued` (value
  is int 1, which DOES fit) gets past the sink and is rejected by a predicate
  instead.  Same defect, two rejection paths, and only a subject on each proves
  both are live.
*/
bool
tests_is_bool_trait_sink()
{
    bool ok = true;

    D_TT_CHECK(!is_bool_trait<value_only<int>>::value);
    D_TT_CHECK(!is_bool_trait<private_base<int>>::value);
    D_TT_CHECK(!is_bool_trait<ambiguous_base<int>>::value);
    D_TT_CHECK(!is_bool_trait<nonstatic_value<int>>::value);
    D_TT_CHECK(!is_bool_trait<wide_valued<int>>::value);

    // and the same, one type over
    D_TT_CHECK(!is_bool_trait<value_only<char>>::value);
    D_TT_CHECK(!is_bool_trait<half_bad<char>>::value);   // the degraded spec

    // is_base_of ALONE would have accepted two of them - which is why the sink
    // exists.  Stated outright, so that a "simplification" of is_bool_trait down
    // to its predicates is caught here
    D_TT_CHECK((std::is_base_of<std::true_type, private_base<int>>::value));
    D_TT_CHECK((std::is_base_of<true_base_a, ambiguous_base<int>>::value));

    return ok;
}

/*
tests_is_bool_trait_predicates
  The three PREDICATES, reached only once the sink has found every member.  Each
  subject below has all five members and fails exactly one predicate.

  Tests the following:
  - `shape_mimic` : value_type is bool, ::type is bool_constant<value>, the
                    conversion and the call operator are both there - and it
                    derives from NOTHING.  Only the is_base_of predicate rejects
                    it.  The subtlest of the eight failures, and the one a
                    hand-rolled shape check written with void_t alone would miss
                    entirely
  - `lying_type`  : every member present, derivation correct, and `::type` is
                    false_type while `::value` is true.  Only the type/value
                    AGREEMENT predicate rejects it.  A trait shaped like this
                    breaks exactly the code that tag-dispatches on ::type while
                    branching on ::value - which is to say, the code that never
                    gets tested, because both spellings "work"
  - `int_valued`  : derives from integral_constant<int, 1>.  1 fits in a bool, so
                    the sink's narrowing cell FORMS; only the value_type predicate
                    rejects it.  The pair with wide_valued (which the sink
                    rejects) shows both paths are live
*/
bool
tests_is_bool_trait_predicates()
{
    bool ok = true;

    D_TT_CHECK(!is_bool_trait<shape_mimic<int>>::value);
    D_TT_CHECK(!is_bool_trait<lying_type<int>>::value);
    D_TT_CHECK(!is_bool_trait<int_valued<int>>::value);

    // shape_mimic really does have all five members - so the ONLY thing that
    // can have rejected it is the derivation predicate
    D_TT_CHECK((std::is_same<shape_mimic<int>::value_type, bool>::value));
    D_TT_CHECK((std::is_same<shape_mimic<int>::type,
                             ::djinterp::bool_constant<true>>::value));
    D_TT_CHECK(shape_mimic<int>{});               // the conversion
    D_TT_CHECK(shape_mimic<int>{}());             // the call operator
    D_TT_CHECK((!std::is_base_of<::djinterp::bool_constant<true>,
                                 shape_mimic<int>>::value));

    // lying_type's ::value and ::type genuinely disagree
    D_TT_CHECK(lying_type<int>::value);
    D_TT_CHECK((std::is_same<lying_type<int>::type, std::false_type>::value));
    D_TT_CHECK((!std::is_same<lying_type<int>::type,
                              ::djinterp::bool_constant<
                                  lying_type<int>::value>>::value));

    // int_valued's value_type is int, and its value is 1
    D_TT_CHECK((std::is_same<int_valued<int>::value_type, int>::value));
    D_TT_CHECK(int_valued<int>::value == 1);

    return ok;
}

/*
tests_is_bool_trait_non_traits
  is_bool_trait must report false for things that are not traits at all - and
  must NOT diagnose, since a trait test hands it whatever the suite is holding.
  Tests the following:
  - a fundamental type, a class with no members, void, an incomplete type, a
    function type, an array, a reference
  - `nonesuch` - the detection idiom's own failure marker, which a suite will
    hand it by accident the moment a detected_t goes wrong
  - it survives the whole hostile zoo, which is what makes it safe to compose
    with the quantifiers
*/
bool
tests_is_bool_trait_non_traits()
{
    bool ok = true;

    D_TT_CHECK(!is_bool_trait<int>::value);
    D_TT_CHECK(!is_bool_trait<bool>::value);
    D_TT_CHECK(!is_bool_trait<plain>::value);
    D_TT_CHECK(!is_bool_trait<sized>::value);
    D_TT_CHECK(!is_bool_trait<void>::value);
    D_TT_CHECK(!is_bool_trait<nonesuch>::value);

    D_TT_CHECK(!is_bool_trait<fixtures::empty>::value);
    D_TT_CHECK(!is_bool_trait<fixtures::incomplete>::value);
    D_TT_CHECK(!is_bool_trait<fixtures::abstract>::value);
    D_TT_CHECK(!is_bool_trait<fixtures::greedy>::value);
    D_TT_CHECK(!is_bool_trait<fixtures::function_type>::value);
    D_TT_CHECK(!is_bool_trait<fixtures::array_type>::value);
    D_TT_CHECK(!is_bool_trait<fixtures::lvalue_ref_type>::value);
    D_TT_CHECK(!is_bool_trait<fixtures::nullptr_type>::value);

    // greedy is the one that could have fooled it - it converts to ANYTHING,
    // including bool, so the sink's static_cast<bool> succeeds.  It has no
    // value_type, so the sink rejects it anyway
    D_TT_CHECK((std::is_convertible<fixtures::greedy, bool>::value));
    D_TT_CHECK(!is_bool_trait<fixtures::greedy>::value);

    return ok;
}

/*
tests_trait_is_well_formed
  trait_is_well_formed - is_bool_trait, run over a battery.
  Tests the following:
  - a well-shaped trait is well-formed for every type in the zoo
  - a mis-shaped one is not, for any
  - it is a PER-TYPE property, not a property of the template.  `half_bad` is a
    well-formed bool trait for int and a broken one for char, so
    trait_is_well_formed<half_bad, int> is true and
    trait_is_well_formed<half_bad, int, char> is false.  A shape check that
    instantiated the trait once and generalized would get this exactly wrong, and
    this is the subject that says so
  - the two-parameter (trait_detect.hpp style) traits bind here as well
*/
bool
tests_trait_is_well_formed()
{
    bool ok = true;

    // well-shaped over the whole zoo
    D_TT_CHECK((trait_is_well_formed<always_true,
                                     D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((trait_is_well_formed<always_false,
                                     D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((trait_is_well_formed<is_int, D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((trait_is_well_formed<has_member_value_type,
                                     D_TEST_HOSTILE_TYPES>::value));

    // mis-shaped, for any type at all
    D_TT_CHECK((!trait_is_well_formed<value_only, int>::value));
    D_TT_CHECK((!trait_is_well_formed<shape_mimic, int>::value));
    D_TT_CHECK((!trait_is_well_formed<lying_type, int>::value));
    D_TT_CHECK((!trait_is_well_formed<value_only,
                                      D_TEST_HOSTILE_TYPES>::value));

    // PER-TYPE, not per-template
    D_TT_CHECK((trait_is_well_formed<half_bad, int>::value));
    D_TT_CHECK((trait_is_well_formed<half_bad, int, long, bool>::value));
    D_TT_CHECK((!trait_is_well_formed<half_bad, char>::value));
    D_TT_CHECK((!trait_is_well_formed<half_bad, int, char>::value));
    D_TT_CHECK((!trait_is_well_formed<half_bad, char, int>::value));

    // it IS holds_for_all over is_bool_trait, restated
    D_TT_CHECK((trait_is_well_formed<half_bad, int, char>::value ==
                (is_bool_trait<half_bad<int>>::value &&
                 is_bool_trait<half_bad<char>>::value)));

    return ok;
}

/*
tests_trait_v_agrees
  D_TEST_TRAIT_V_AGREES - does the `_v` companion still agree with its trait?

  A `_v` and its trait are emitted from different macros and are hand-written
  just as often.  Nothing but a test keeps them in step, and a stale `_v` - one
  left pointing at a renamed or re-based trait - is a silent, TOTAL INVERSION of
  the answer at every call site that prefers the shorthand.  The compiler is
  perfectly happy.

  Tests the following:
  - an agreeing pair -> true
  - a DISAGREEING pair -> false.  This is the check that proves the macro can
    fail: `stale_v` is deliberately the negation of `stale<T>::value`, which is
    exactly the shape a `_v` takes when its trait is re-based and the shorthand
    is not updated with it
  - it takes the trait's NAME, not an instantiation, and pastes `_v` onto its
    LAST token - so a fully qualified name works
  - it is variadic, so a multi-argument trait works
*/
bool
tests_trait_v_agrees()
{
    bool ok = true;

    // agreeing
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(honest, int));
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(honest, plain));
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(honest, void));

    // DISAGREEING - the macro must report false
    D_TT_CHECK(!D_TEST_TRAIT_V_AGREES(stale, int));
    D_TT_CHECK(!D_TEST_TRAIT_V_AGREES(stale, plain));

    // ...and the inversion is real, not an artefact of the macro
    D_TT_CHECK(stale<int>::value);
    D_TT_CHECK(!stale_v<int>);

    // a QUALIFIED name: only the last token is pasted onto _v
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(::djinterp::testing::honest, int));
    D_TT_CHECK(!D_TEST_TRAIT_V_AGREES(::djinterp::testing::stale, int));

    // a MULTI-ARGUMENT trait, from the standard library
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(std::is_same, int, int));
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(std::is_same, int, char));
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(std::is_constructible, sized));

    // and the toolkit's own
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(::djinterp::test::is_bool_trait,
                                     std::true_type));
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(::djinterp::test::is_bool_trait,
                                     value_only<int>));

    return ok;
}

/*
tests_shape_self_application
  The toolkit turned on itself.

  Every trait test_traits.hpp exports as a BOOL trait must itself be a
  well-formed bool trait, and is_bool_trait is the thing that can say so.  If the
  toolkit's own traits are mis-shaped, every suite that composes them -
  `conjunction<is_bool_trait<X>, trait_ignores_cvref<Y, Z>>`, a concept face over
  either - breaks, and the toolkit is the last place anyone would look.

  Tests the following:
  - is_bool_trait applied to is_bool_trait
  - and to each of yields_lvalue, yields_xvalue, yields_prvalue,
    is_nothrow_probe, holds_for_all, holds_for_any, holds_for_none,
    trait_is_well_formed, trait_ignores_cvref
  - count_holds is NOT a bool trait, and correctly so: its value_type is
    std::size_t, because it is a COUNT.  A caller that folded it into a boolean
    would be reading "at least one" where it meant "exactly N", and the shape is
    what stops them
  - is_bool_trait is well-formed over the whole hostile zoo, so it is safe to
    hand to the quantifiers - which is exactly what trait_is_well_formed does
*/
bool
tests_shape_self_application()
{
    bool ok = true;

    // the shape checker checks itself
    D_TT_CHECK(is_bool_trait<is_bool_trait<int>>::value);
    D_TT_CHECK(is_bool_trait<is_bool_trait<std::true_type>>::value);
    D_TT_CHECK((trait_is_well_formed<is_bool_trait,
                                     D_TEST_HOSTILE_TYPES>::value));

    // section II
    D_TT_CHECK(is_bool_trait<yields_lvalue<p_front, sized>>::value);
    D_TT_CHECK(is_bool_trait<yields_xvalue<p_take, sized>>::value);
    D_TT_CHECK(is_bool_trait<yields_prvalue<p_size, sized>>::value);
    D_TT_CHECK(is_bool_trait<is_nothrow_probe<p_swap_noexcept, sized>>::value);

    // section III
    D_TT_CHECK(is_bool_trait<holds_for_all<is_int, int>>::value);
    D_TT_CHECK(is_bool_trait<holds_for_any<is_int, int>>::value);
    D_TT_CHECK(is_bool_trait<holds_for_none<is_int, int>>::value);

    // section IV
    D_TT_CHECK(is_bool_trait<trait_is_well_formed<always_true, int>>::value);

    // section V
    D_TT_CHECK(is_bool_trait<trait_ignores_cvref<always_true, int>>::value);
    D_TT_CHECK(is_bool_trait<trait_ignores_cvref<hates_const, int>>::value);

    // count_holds is a COUNT, not a predicate - and its shape says so
    D_TT_CHECK(!is_bool_trait<count_holds<is_int, int>>::value);
    D_TT_CHECK((std::is_same<count_holds<is_int, int>::value_type,
                             std::size_t>::value));

    return ok;
}

/*
tests_shape_value_companions
  The `_v` companions of section IV.
  Tests the following:
  - is_bool_trait_v and trait_is_well_formed_v agree with their traits
  - checked by the very macro this section exports, on itself
*/
bool
tests_shape_value_companions()
{
    bool ok = true;

    D_TT_CHECK((is_bool_trait_v<std::true_type>) ==
               (is_bool_trait<std::true_type>::value));
    D_TT_CHECK((is_bool_trait_v<value_only<int>>) ==
               (is_bool_trait<value_only<int>>::value));
    D_TT_CHECK((is_bool_trait_v<int>) == (is_bool_trait<int>::value));

    D_TT_CHECK((trait_is_well_formed_v<always_true, int>) ==
               (trait_is_well_formed<always_true, int>::value));
    D_TT_CHECK((trait_is_well_formed_v<value_only, int>) ==
               (trait_is_well_formed<value_only, int>::value));

    // the shorthands really are the values
    D_TT_CHECK(is_bool_trait_v<std::true_type>);
    D_TT_CHECK(!is_bool_trait_v<value_only<int>>);
    D_TT_CHECK((trait_is_well_formed_v<always_true, D_TEST_HOSTILE_TYPES>));
    D_TT_CHECK((!trait_is_well_formed_v<shape_mimic, int>));

    // the macro, on the traits it exists to guard
    D_TT_CHECK(D_TEST_TRAIT_V_AGREES(::djinterp::test::is_bool_trait,
                                     shape_mimic<int>));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
