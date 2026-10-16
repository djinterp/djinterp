/*******************************************************************************
* djinterp [core]                  constexpr_container_traits_tests_lifetime.cpp
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests_lifetime.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "constexpr_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// p_nested_type
//   probe: does the instantiated trait carry a nested `type`?  Probed rather
// than named, because naming a member that is not there is a hard error, not a
// false - which is the whole reason the size_lifetime / container_lifetime
// asymmetry below is checkable at all.
D_TEST_TYPE_PROBE(p_nested_type, typename _Type::type)


/*
tests_size_lifetime_disjuncts
  size_lifetime - the FIVE promoting signals, each in isolation.
  Tests the following:
  - the opt-in tag alone promotes
  - a static extent alone promotes
  - a std::tuple_size specialization alone promotes
  - a constexpr size() expression alone promotes
  - constexpr iteration alone promotes

  Each fixture carries exactly one signal, so each check exercises exactly one
  disjunct.  The fifth is the odd one: a type with constexpr_begin/constexpr_end
  and no size at all is declared to have a "compile-time-expressible SIZE".  That
  is a defensible inference (you can count a compile-time range at compile time)
  but it is an inference, not a signal, and it is pinned here as such.
*/
bool
tests_size_lifetime_disjuncts()
{
    bool ok = true;

    D_CCT_LIFE(lifetime::both, size_lifetime<tag_true>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<extent_static>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<tuple_conformer>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<size_constexpr>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<iter_alias>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<iter_pair>::value);

    // each of those fixtures really does carry ONE signal and no other
    D_CCT_CHECK(has_constexpr_container_tag<tag_true>::value &&
                !has_constexpr_extent<tag_true>::value       &&
                !has_tuple_size<tag_true>::value             &&
                !has_constexpr_iteration<tag_true>::value);

    D_CCT_CHECK(has_constexpr_extent<extent_static>::value              &&
                !has_constexpr_container_tag<extent_static>::value      &&
                !has_tuple_size<extent_static>::value                   &&
                !has_constexpr_size_expression<extent_static>::value    &&
                !has_constexpr_iteration<extent_static>::value);

    D_CCT_CHECK(has_tuple_size<tuple_conformer>::value                    &&
                !has_constexpr_container_tag<tuple_conformer>::value      &&
                !has_constexpr_extent<tuple_conformer>::value             &&
                !has_constexpr_size_expression<tuple_conformer>::value    &&
                !has_constexpr_iteration<tuple_conformer>::value);

    D_CCT_CHECK(has_constexpr_size_expression<size_constexpr>::value    &&
                !has_constexpr_container_tag<size_constexpr>::value     &&
                !has_constexpr_extent<size_constexpr>::value            &&
                !has_tuple_size<size_constexpr>::value                  &&
                !has_constexpr_iteration<size_constexpr>::value);

    D_CCT_CHECK(has_constexpr_iteration<iter_pair>::value                 &&
                !has_constexpr_container_tag<iter_pair>::value            &&
                !has_constexpr_extent<iter_pair>::value                   &&
                !has_tuple_size<iter_pair>::value                         &&
                !has_constexpr_size_expression<iter_pair>::value);

    return ok;
}

/*
tests_size_lifetime_negative
  No signal, no promotion.
  Tests the following:
  - a type with none of the five signals has a RUNTIME size lifetime
  - that is true whether its contents are literal or not: size_lifetime is
    about the SIZE only, and it never consults lifetime_of
  - half an iteration interface is not a signal
*/
bool
tests_size_lifetime_negative()
{
    bool ok = true;

    D_CCT_LIFE(lifetime::runtime, size_lifetime<no_signal>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<no_signal_nonliteral>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<size_runtime>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<iter_begin_only>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<iter_end_only>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<int>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<life_compile_unsized>::value);

    return ok;
}

/*
tests_size_lifetime_is_binary
  size_lifetime is a two-valued function into a four-valued lattice.
  Tests the following:
  - it yields `both` or `runtime`, and never `compile_time` or `none`

  Worth stating outright, because it is what makes the meet in container_lifetime
  well-behaved: meet(both, X) = X, and meet(runtime, X) is X with the
  compile-time bit cleared.  A size_lifetime that could return `compile_time`
  would be claiming a container whose size exists at compile time and NOT at
  runtime, which is not a thing; the trait's shape rules it out by construction,
  and this check keeps it ruled out.
*/
bool
tests_size_lifetime_is_binary()
{
    bool ok = true;

    #define D_CCT_BINARY(...)                                                 \
        D_CCT_CHECK((size_lifetime<__VA_ARGS__>::value == lifetime::both) ||  \
                    (size_lifetime<__VA_ARGS__>::value == lifetime::runtime))

    D_CCT_BINARY(tag_true);
    D_CCT_BINARY(tag_false);
    D_CCT_BINARY(extent_static);
    D_CCT_BINARY(tuple_conformer);
    D_CCT_BINARY(size_constexpr);
    D_CCT_BINARY(iter_pair);
    D_CCT_BINARY(no_signal);
    D_CCT_BINARY(no_signal_nonliteral);
    D_CCT_BINARY(life_compile_sized);
    D_CCT_BINARY(life_none_sized);
    D_CCT_BINARY(life_compile_unsized);
    D_CCT_BINARY(extent_nonliteral);
    D_CCT_BINARY(std::array<int, 3>);
    D_CCT_BINARY(std::vector<int>);
    D_CCT_BINARY(int);

    #undef D_CCT_BINARY

    return ok;
}

/*
tests_size_lifetime_opt_out_ignored
  The explicit opt-OUT does not opt out.
  Tests the following:
  - `using is_constexpr_container = std::false_type` makes the TAG disjunct
    false, and nothing else.  A type that also carries a static extent is
    promoted anyway - its size_lifetime is `both` and, being literal, it comes
    out a constexpr container.

  The tag is documented as "an explicit claim of static-lifetime capability".
  A false_type is naturally read as an explicit DENIAL of that capability, and
  it has no such effect: the four structural disjuncts are ORed alongside it,
  not gated by it.  If the denial is meant to be authoritative, the tag needs to
  be consulted BEFORE the disjunction (as container_lifetime already consults it
  for the positive case) rather than inside it.  Pinned as it stands.
*/
bool
tests_size_lifetime_opt_out_ignored()
{
    bool ok = true;

    // the tag disjunct is false...
    D_CCT_CHECK(!has_constexpr_container_tag<tag_false_with_extent>::value);

    // ...and the extent disjunct promotes anyway
    D_CCT_CHECK(has_constexpr_extent<tag_false_with_extent>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<tag_false_with_extent>::value);
    D_CCT_LIFE(lifetime::both,
               container_lifetime<tag_false_with_extent>::value);
    D_CCT_CHECK(is_constexpr_container<tag_false_with_extent>::value);

    // a bare opt-out with no other signal is, of course, not promoted
    D_CCT_LIFE(lifetime::runtime, size_lifetime<tag_false>::value);

    return ok;
}

/*
tests_container_lifetime_meet
  container_lifetime = meet( size_lifetime, lifetime_of ) - the composition the
  header's opening formula names.
  Tests the following:
  - the four fixtures below all have size_lifetime = both (they carry an
    extent), and steer lifetime_of through the opt-in `lifetime_category`.  Since
    meet(both, X) = X, each lands on a different lattice value, and the four
    together are the meet's second row
  - the meet really is the lattice AND: it is recomputed here from
    lifetime_meet directly and compared
*/
bool
tests_container_lifetime_meet()
{
    bool ok = true;

    // meet(both, X) = X
    D_CCT_LIFE(lifetime::both,         container_lifetime<life_both_sized>::value);
    D_CCT_LIFE(lifetime::compile_time, container_lifetime<life_compile_sized>::value);
    D_CCT_LIFE(lifetime::runtime,      container_lifetime<life_runtime_sized>::value);
    D_CCT_LIFE(lifetime::none,         container_lifetime<life_none_sized>::value);

    // and each fixture really does have size_lifetime = both
    D_CCT_LIFE(lifetime::both, size_lifetime<life_both_sized>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<life_compile_sized>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<life_runtime_sized>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<life_none_sized>::value);

    // the verdict IS the meet - recomputed from the two operands
    #define D_CCT_IS_MEET(...)                                                \
        D_CCT_CHECK(container_lifetime<__VA_ARGS__>::value ==                 \
                    lifetime_meet(size_lifetime<__VA_ARGS__>::value,          \
                                  lifetime_of<__VA_ARGS__>::value))

    D_CCT_IS_MEET(life_both_sized);
    D_CCT_IS_MEET(life_compile_sized);
    D_CCT_IS_MEET(life_runtime_sized);
    D_CCT_IS_MEET(life_none_sized);
    D_CCT_IS_MEET(life_compile_unsized);
    D_CCT_IS_MEET(extent_static);
    D_CCT_IS_MEET(extent_nonliteral);
    D_CCT_IS_MEET(size_constexpr);
    D_CCT_IS_MEET(tuple_conformer);
    D_CCT_IS_MEET(tuple_conformer_nonliteral);
    D_CCT_IS_MEET(no_signal);
    D_CCT_IS_MEET(no_signal_nonliteral);
    D_CCT_IS_MEET(std::array<int, 3>);
    D_CCT_IS_MEET(std::vector<int>);
    D_CCT_IS_MEET(int);

    #undef D_CCT_IS_MEET

    // ...for every type EXCEPT one that carries the opt-in tag, which
    // short-circuits the meet entirely (see tests_container_lifetime_tag_bypass)
    D_CCT_CHECK(container_lifetime<tag_true_nonliteral>::value !=
                lifetime_meet(size_lifetime<tag_true_nonliteral>::value,
                              lifetime_of<tag_true_nonliteral>::value));

    return ok;
}

/*
tests_container_lifetime_lattice
  All four lattice values are reachable, and `none` by two distinct routes.
  Tests the following:
  - both         : compile-time size, compile-time contents
  - compile_time : compile-time size, contents that exist ONLY at compile time
  - runtime      : compile-time size, runtime contents (the fixed array of
                   std::string; the commonest real case)
  - none, route 1: an explicit `lifetime_category = none`
  - none, route 2: contents that are compile-time ONLY, in a container whose
                   size is not - meet(runtime, compile_time) = none, because the
                   two stage-sets do not intersect

  Route 2 is the one a boolean "is it constexpr?" could never have expressed, and
  it is the reason the axis is a lattice rather than a flag.  It is also easy to
  break: any change that makes size_lifetime return `both` by default would
  silently turn it into `compile_time`.
*/
bool
tests_container_lifetime_lattice()
{
    bool ok = true;

    D_CCT_LIFE(lifetime::both,         container_lifetime<life_both_sized>::value);
    D_CCT_LIFE(lifetime::compile_time, container_lifetime<life_compile_sized>::value);
    D_CCT_LIFE(lifetime::runtime,      container_lifetime<extent_nonliteral>::value);

    // `none`, route 1: declared outright
    D_CCT_LIFE(lifetime::none, container_lifetime<life_none_sized>::value);

    // `none`, route 2: meet(runtime, compile_time) - the stage-sets are disjoint
    D_CCT_LIFE(lifetime::runtime,      size_lifetime<life_compile_unsized>::value);
    D_CCT_LIFE(lifetime::compile_time, lifetime_of<life_compile_unsized>::value);
    D_CCT_LIFE(lifetime::none, container_lifetime<life_compile_unsized>::value);
    D_CCT_CHECK(lifetime_meet(lifetime::runtime, lifetime::compile_time) ==
                lifetime::none);

    // the predicates read the lattice correctly
    D_CCT_CHECK(is_both(container_lifetime<life_both_sized>::value));
    D_CCT_CHECK(is_compile_time_only(
                    container_lifetime<life_compile_sized>::value));
    D_CCT_CHECK(is_runtime_only(container_lifetime<extent_nonliteral>::value));
    D_CCT_CHECK(is_none(container_lifetime<life_none_sized>::value));
    D_CCT_CHECK(is_none(container_lifetime<life_compile_unsized>::value));

    return ok;
}

/*
tests_container_lifetime_tag_bypass
  The opt-in tag short-circuits the composition entirely.
  Tests the following:
  - a type tagged is_constexpr_container = true_type resolves to lifetime::both
    WITHOUT consulting lifetime_of
  - so a tagged type holding a std::string - manifestly not constexpr-
    constructible - is nevertheless reported as a constexpr container

  The header says this is intended ("a coarse assertion of static-lifetime
  capability"), and it is: the tag is an assertion, and an assertion is not
  checked.  What is worth being explicit about is that the tag is therefore the
  ONE way to make this trait lie about a type, and that nothing anywhere
  validates it.  A `static_assert(is_literal_type<T>::value)` inside the opt-in
  would cost nothing and close it.
*/
bool
tests_container_lifetime_tag_bypass()
{
    bool ok = true;

    // the contents are demonstrably not constexpr-constructible
    D_CCT_LIFE(lifetime::runtime, lifetime_of<tag_true_nonliteral>::value);

    // ...and the verdict is `both` regardless
    D_CCT_LIFE(lifetime::both, container_lifetime<tag_true_nonliteral>::value);
    D_CCT_CHECK(is_constexpr_container<tag_true_nonliteral>::value);

    // which is NOT what the meet would have said
    D_CCT_CHECK(lifetime_meet(size_lifetime<tag_true_nonliteral>::value,
                              lifetime_of<tag_true_nonliteral>::value) ==
                lifetime::runtime);

    // a tagged type with literal contents agrees with the meet, so the bypass
    // is only observable on the non-literal ones
    D_CCT_LIFE(lifetime::both, container_lifetime<tag_true>::value);
    D_CCT_CHECK(container_lifetime<tag_true>::value ==
                lifetime_meet(size_lifetime<tag_true>::value,
                              lifetime_of<tag_true>::value));

    return ok;
}

/*
tests_container_lifetime_type
  The nested ::type and the container_lifetime_t alias.
  Tests the following:
  - ::type is exactly lifetime_constant<value>, i.e.
    std::integral_constant<lifetime, value>
  - container_lifetime_t<T> names that same type
  - the carrier's ::value round-trips
  - the named carriers from lifetime.hpp (both_lifetime, runtime_lifetime, ...)
    are the same types, so tag dispatch on them works

  NOTE the asymmetry: container_lifetime exposes a ::type, and size_lifetime -
  its own input - does not.  A caller who wants to tag-dispatch on the size
  contribution alone has to build the carrier by hand.
*/
bool
tests_container_lifetime_type()
{
    bool ok = true;

    D_CCT_CHECK((std::is_same<container_lifetime<life_both_sized>::type,
                              lifetime_constant<lifetime::both>>::value));
    D_CCT_CHECK((std::is_same<container_lifetime<life_compile_sized>::type,
                              lifetime_constant<
                                  lifetime::compile_time>>::value));
    D_CCT_CHECK((std::is_same<container_lifetime<extent_nonliteral>::type,
                              lifetime_constant<lifetime::runtime>>::value));
    D_CCT_CHECK((std::is_same<container_lifetime<life_none_sized>::type,
                              lifetime_constant<lifetime::none>>::value));

    // the _t alias names the same type
    D_CCT_CHECK((std::is_same<container_lifetime_t<life_both_sized>,
                              container_lifetime<life_both_sized>::type>::value));
    D_CCT_CHECK((std::is_same<container_lifetime_t<std::vector<int>>,
                              container_lifetime<
                                  std::vector<int>>::type>::value));

    // and the named carriers are those types
    D_CCT_CHECK((std::is_same<container_lifetime_t<life_both_sized>,
                              both_lifetime>::value));
    D_CCT_CHECK((std::is_same<container_lifetime_t<life_compile_sized>,
                              compile_time_lifetime>::value));
    D_CCT_CHECK((std::is_same<container_lifetime_t<life_runtime_sized>,
                              runtime_lifetime>::value));
    D_CCT_CHECK((std::is_same<container_lifetime_t<life_none_sized>,
                              none_lifetime>::value));

    // the carrier round-trips
    D_CCT_CHECK(container_lifetime_t<life_both_sized>::value ==
                container_lifetime<life_both_sized>::value);
    D_CCT_CHECK(container_lifetime_t<std::vector<int>>::value ==
                container_lifetime<std::vector<int>>::value);

    // size_lifetime has no ::type - the asymmetry, pinned
    D_CCT_CHECK((!::djinterp::is_detected<p_nested_type,
                                          size_lifetime<
                                              extent_static>>::value));
    D_CCT_CHECK((::djinterp::is_detected<p_nested_type,
                                         container_lifetime<
                                             extent_static>>::value));

    return ok;
}

/*
tests_container_lifetime_value_companion
  container_lifetime_v - an inline variable on C++17+, a plain variable template
  on C++14, absent below.
  Tests the following:
  - the shorthand carries the same value as the trait
  - it is typed `lifetime`, not bool - so it cannot be folded into a boolean
    expression by accident
*/
bool
tests_container_lifetime_value_companion()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CCT_CHECK(container_lifetime_v<life_both_sized> ==
                container_lifetime<life_both_sized>::value);
    D_CCT_CHECK(container_lifetime_v<life_none_sized> ==
                container_lifetime<life_none_sized>::value);
    D_CCT_CHECK(container_lifetime_v<std::vector<int>> ==
                container_lifetime<std::vector<int>>::value);

    D_CCT_CHECK(container_lifetime_v<life_both_sized> == lifetime::both);
    D_CCT_CHECK(container_lifetime_v<life_compile_sized> ==
                lifetime::compile_time);
    D_CCT_CHECK(container_lifetime_v<life_none_sized> == lifetime::none);
    D_CCT_CHECK(container_lifetime_v<std::vector<int>> == lifetime::runtime);

    D_CCT_CHECK((std::is_same<decltype(container_lifetime_v<std::vector<int>>),
                              const lifetime>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
