/*******************************************************************************
* djinterp [core]                   constexpr_container_traits_tests_hazards.cpp
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests_hazards.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <memory>
#include <string>
#include <vector>
// djinterp
#include "constexpr_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_hazard_destructor
  HAZARD 1.  has_constexpr_size_expression can HARD-ERROR instead of reporting
  false, and the error is not catchable.

  THE MECHANISM.  The probe is

      std::integral_constant<std::size_t, clean_t<_Type>{}.size()>

  `clean_t<_Type>{}` creates a TEMPORARY, and a temporary in a constant
  expression must also be DESTROYED there.  So the destructor is constant-
  evaluated as part of the probe - which tests_size_expression_destructor already
  proves, by showing that marking a destructor constexpr flips the answer.

  The trouble is what happens when the destructor's BODY is not a constant
  expression.  A substitution failure must arise in the IMMEDIATE CONTEXT of the
  substitution; a failure inside a function body - and an implicitly-defined
  destructor has a body - is not in the immediate context.  It is a hard error.
  SFINAE cannot see it, D_TYPE_TRAIT_TRUE cannot trap it, and the trait's stated
  contract - "reports false otherwise, never errors" - is not kept.

  WHAT BREAKS.  std::array<std::string, N>, for N > 0.  Which is precisely the
  type the header's own comment holds up as the reason has_tuple_size exists:

      "an array<std::string, N> has a compile-time size of N even though string
       is not a literal type and array<string, N>{}.size() is therefore not a
       constant expression"

  The observation is right.  The consequence assumed from it - that the probe
  would therefore report false - is not.

  IT IS WORSE THAN A HARD ERROR: IT IS ORDER-DEPENDENT.  Whether the build breaks
  depends on where the specialization is first instantiated.  Reached from inside
  another template (as size_lifetime reaches it), GCC treats the failed constant
  evaluation as a substitution failure and yields false; named directly at
  namespace or block scope FIRST, it emits the error.  So

      size_lifetime<std::array<std::string, 3>>::value;              // fine
      has_constexpr_size_expression<std::array<std::string, 3>>::value;   // fine

  compiles, and the same two lines in the opposite order does not.  A program
  whose validity depends on instantiation order is not a program.

  WHY THE UMBRELLA USUALLY SURVIVES.  size_lifetime's disjunction is

      tag || extent || tuple_size || size_expression || iteration

  and has_tuple_size comes BEFORE has_constexpr_size_expression.  std::array has
  a std::tuple_size specialization, so the third disjunct is true and the fourth
  is never the first thing to be instantiated.  The disjunct ORDER is load-bearing
  for correctness, by accident.  Reorder that `||` chain - a change that looks
  like pure cosmetics - and std::array<std::string, N> stops compiling.

  THE FIX.  Do not materialize a temporary.  `std::declval<clean_t<_Type>&>()`
  never constructs and never destroys anything, so nothing outside the immediate
  context can fail:

      D_TYPE_TRAIT_TRUE(has_constexpr_size_expression,
          std::integral_constant<std::size_t,
              std::declval<const clean_t<_Type>&>().size()>)

  though note declval is not usable in a CONSTANT expression, so recovering "is
  size() constexpr?" without building a T needs a different formulation
  altogether - probably `clean_t<_Type>::size()` for a static size, or an opt-in.
  Either way, T{} inside a probe is a trap.

  Tests the following (safely):
  - a type whose destructor is constexpr passes the probe; the same type with a
    non-constexpr destructor does not.  This is the mechanism, isolated
  - std::array<std::string, 0> is FINE - no elements, nothing to destroy
  - std::array<std::unique_ptr<int>, 2> is FINE - it reports false, because its
    destructor happens to fail in a place SFINAE can see.  The hazard is not even
    consistent across element types
  - the destructive cases are behind D_CCT_HAZARD_TESTS
*/
bool
tests_hazard_destructor()
{
    bool ok = true;

    // the mechanism, isolated: the destructor IS part of the probe
    D_CCT_CHECK(!has_constexpr_size_expression<size_user_destructor>::value);

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    D_CCT_CHECK(has_constexpr_size_expression<
                    size_constexpr_destructor>::value);

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    // the safe neighbours of the mine

    // no elements -> nothing to destroy -> the probe fires
    D_CCT_CHECK(has_constexpr_size_expression<
                    std::array<std::string, 0>>::value);

    // a different non-literal element -> reports false, no diagnostic.  The
    // hazard is element-dependent, which is what makes it so easy to miss
    D_CCT_CHECK(!has_constexpr_size_expression<
                    std::array<std::unique_ptr<int>, 2>>::value);

    // the umbrella verdict for the mine IS reachable, because has_tuple_size
    // precedes has_constexpr_size_expression in the disjunction.  This is the
    // accident the whole finding turns on
    D_CCT_CHECK(has_tuple_size<std::array<std::string, 3>>::value);
    D_CCT_LIFE(lifetime::both,
               size_lifetime<std::array<std::string, 3>>::value);
    D_CCT_LIFE(lifetime::runtime,
               lifetime_of<std::array<std::string, 3>>::value);
    D_CCT_LIFE(lifetime::runtime,
               container_lifetime<std::array<std::string, 3>>::value);
    D_CCT_CHECK(!is_constexpr_container<std::array<std::string, 3>>::value);

#if D_CCT_HAZARD_TESTS

    // *** THE MINE ***
    //
    //   This line does not compile.  It is the whole finding, in one
    // instantiation:
    //
    //   error: '(((std::string*)(&<anonymous>.std::array<std::string, 3>::
    //           _M_elems)) != 0)' is not a constant expression
    //   note:  in 'constexpr' expansion of
    //          '<anonymous>.std::array<std::string, 3>::~array()'
    //   note:  in template argument for type 'long unsigned int'
    //            std::integral_constant<std::size_t, clean_t<_Type>{}.size()>
    //
    //   Note "in constexpr expansion of ~array()" - inside a destructor BODY,
    // which is not the immediate context, which is why SFINAE never sees it.
    D_CCT_CHECK(!has_constexpr_size_expression<
                    std::array<std::string, 3>>::value);

    //   And the same hazard in a type with NO tuple_size specialization, where
    // the disjunction cannot short-circuit past it - so size_lifetime,
    // container_lifetime, is_constexpr_container and constexpr_container_class
    // all inherit the break.
    struct hazard_no_tuple_size
    {
        std::array<std::string, 3> payload;

        constexpr std::size_t
        size() const
        {
            return 3;
        }
    };

    D_CCT_CHECK(!has_constexpr_size_expression<hazard_no_tuple_size>::value);

#endif  // D_CCT_HAZARD_TESTS

    return ok;
}

/*
tests_hazard_tag_base
  HAZARD 2.  has_constexpr_container_tag INHERITS from the opt-in alias, so a
  malformed opt-in is a malformed BASE CLASS - and a base-clause is never in the
  immediate context.

  THE MECHANISM.  The trait is emitted by D_TYPE_TRAIT_TRUE_AS:

      template<typename _Type>
      struct has_constexpr_container_tag<_Type,
          D_VOID_T<typename clean_t<_Type>::is_constexpr_container>>
          : clean_t<_Type>::is_constexpr_container
      {};

  The void_t sink checks only that the member alias EXISTS.  Once it does, the
  specialization is selected and instantiated, and whatever the alias names
  becomes a base class.  If that is not a class type - or is a class with no
  ::value - the base-clause is ill-formed, and an ill-formed base-clause is a
  hard error, always, in every context.

  The inheritance is not incidental: it is how `= std::false_type` comes out
  false rather than merely "present" (tests_tag_access pins that).  So it cannot
  simply be removed.

  WHAT BREAKS.  Every one of these:

      using is_constexpr_container = int;                    // not a class
      using is_constexpr_container = void;                   // not a class
      using is_constexpr_container = struct { };             // no ::value

  and this one does not break, but degrades:

      using is_constexpr_container = std::integral_constant<int, 1>;

  - the trait inherits it, ::value is 1, everything downstream converts it to
  true, and has_constexpr_container_tag<T>::value_type is now `int`.  The trait
  has silently stopped being a bool trait.

  THE FIX.  Guard the base.  The success branch should inherit from
  `bool_constant<...>` built from the alias's value, not from the alias itself:

      D_TYPE_TRAIT_TRUE_AS(has_constexpr_container_tag,
          typename clean_t<_Type>::is_constexpr_container,
          ::djinterp::bool_constant<
              static_cast<bool>(clean_t<_Type>::is_constexpr_container::value)>)

  which keeps false_type meaning false, restores the bool-trait shape for the
  integral_constant<int, 1> case, and turns the three hard errors into ordinary
  ones with a name attached.  Tightening the DETECTION expression instead - to
  require the alias be a bool_constant - would turn them into a clean `false`,
  which is the friendlier option and one line more.

  Tests the following (safely):
  - the well-formed opt-ins behave: true_type, false_type, a derived
    bool_constant, and absent
  - the trait really does inherit, which is both the feature and the hazard
  - the malformed opt-ins are behind D_CCT_HAZARD_TESTS
*/
bool
tests_hazard_tag_base()
{
    bool ok = true;

    // the well-formed opt-ins
    D_CCT_CHECK(has_constexpr_container_tag<tag_true>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<tag_false>::value);
    D_CCT_CHECK(has_constexpr_container_tag<tag_derived>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<no_signal>::value);

    // the inheritance that makes false_type mean false - the feature...
    D_CCT_CHECK((std::is_base_of<std::false_type,
                                 has_constexpr_container_tag<
                                     tag_false>>::value));

    // ...and the shape it preserves, when the opt-in is well-formed
    D_CCT_CHECK(is_bool_trait<has_constexpr_container_tag<tag_true>>::value);
    D_CCT_CHECK(is_bool_trait<has_constexpr_container_tag<tag_false>>::value);
    D_CCT_CHECK((std::is_same<
                    has_constexpr_container_tag<tag_true>::value_type,
                    bool>::value));

#if D_CCT_HAZARD_TESTS

    // *** THE MINES ***
    //
    //   Each of the three is a hard error at the point the trait is
    // instantiated:
    //
    //   error: base type 'int' fails to be a struct or class type
    //   error: 'value' is not a member of '...::nope'
    //
    //   No amount of care in the DETECTION expression can trap these: the
    // failure is in the base-clause of the specialization, which is instantiated
    // only after the detection has already succeeded.
    D_CCT_CHECK(!has_constexpr_container_tag<tag_not_a_bool_constant>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<tag_valueless_base>::value);

    //   And the degradation, which compiles but is not a bool trait any more.
    // If the guarded fix above is applied, THIS check flips to true.
    struct tag_int_constant
    {
        using is_constexpr_container = std::integral_constant<int, 1>;
    };

    D_CCT_CHECK(has_constexpr_container_tag<tag_int_constant>::value);
    D_CCT_CHECK(!is_bool_trait<
                    has_constexpr_container_tag<tag_int_constant>>::value);
    D_CCT_CHECK((std::is_same<
                    has_constexpr_container_tag<tag_int_constant>::value_type,
                    int>::value));

#endif  // D_CCT_HAZARD_TESTS

    return ok;
}


NS_END  // testing
NS_END  // djinterp
