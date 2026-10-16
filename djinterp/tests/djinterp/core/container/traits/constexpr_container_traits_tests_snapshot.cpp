/*******************************************************************************
* djinterp [core]                  constexpr_container_traits_tests_snapshot.cpp
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests_snapshot.cpp
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


/*
tests_class_members
  constexpr_container_class - the six members it exposes.
  Tests the following:
  - the four SIGNAL members (has_tag, has_extent, has_size_expr, has_iteration)
    each mirror their standalone trait
  - the LIFETIME member mirrors container_lifetime
  - the VERDICT member mirrors is_constexpr_container
  - the all-ones and all-zeros rows
*/
bool
tests_class_members()
{
    bool ok = true;

    using tagged  = constexpr_container_class<tag_true>;
    using sized   = constexpr_container_class<extent_static>;
    using expr    = constexpr_container_class<size_constexpr>;
    using iterate = constexpr_container_class<iter_pair>;
    using none    = constexpr_container_class<no_signal>;
    using arr     = constexpr_container_class<std::array<int, 3>>;

    // each signal member, isolated on the fixture that carries only it
    D_CCT_CHECK(tagged::has_tag);
    D_CCT_CHECK(!tagged::has_extent);
    D_CCT_CHECK(!tagged::has_size_expr);
    D_CCT_CHECK(!tagged::has_iteration);

    D_CCT_CHECK(!sized::has_tag);
    D_CCT_CHECK(sized::has_extent);
    D_CCT_CHECK(!sized::has_size_expr);
    D_CCT_CHECK(!sized::has_iteration);

    D_CCT_CHECK(!expr::has_tag);
    D_CCT_CHECK(!expr::has_extent);
    D_CCT_CHECK(expr::has_size_expr);
    D_CCT_CHECK(!expr::has_iteration);

    D_CCT_CHECK(!iterate::has_tag);
    D_CCT_CHECK(!iterate::has_extent);
    D_CCT_CHECK(!iterate::has_size_expr);
    D_CCT_CHECK(iterate::has_iteration);

    // the all-zeros row
    D_CCT_CHECK(!none::has_tag);
    D_CCT_CHECK(!none::has_extent);
    D_CCT_CHECK(!none::has_size_expr);
    D_CCT_CHECK(!none::has_iteration);
    D_CCT_CHECK(none::life == lifetime::runtime);
    D_CCT_CHECK(!none::is_constexpr);

    // a real fixed array: promoted by tuple_size (which the snapshot does not
    // report) and by the size expression (which it does)
    D_CCT_CHECK(!arr::has_tag);
    D_CCT_CHECK(!arr::has_extent);
    D_CCT_CHECK(arr::has_size_expr);
    D_CCT_CHECK(!arr::has_iteration);
    D_CCT_CHECK(arr::life == lifetime::both);
    D_CCT_CHECK(arr::is_constexpr);

    return ok;
}

/*
tests_class_agrees_with_traits
  The snapshot must be a faithful restatement of its sources - a convenience
  that can disagree with what it summarizes is worse than no convenience.
  Tests the following:
  - all six members agree with their standalone traits, on every fixture
  - including the cv-ref forms, since the snapshot cleans its argument too
*/
bool
tests_class_agrees_with_traits()
{
    bool ok = true;

    #define D_CCT_SNAPSHOT_AGREES(...)                                        \
        D_CCT_CHECK(constexpr_container_class<__VA_ARGS__>::has_tag ==        \
                    has_constexpr_container_tag<__VA_ARGS__>::value);         \
        D_CCT_CHECK(constexpr_container_class<__VA_ARGS__>::has_extent ==     \
                    has_constexpr_extent<__VA_ARGS__>::value);                \
        D_CCT_CHECK(constexpr_container_class<__VA_ARGS__>::has_size_expr ==  \
                    has_constexpr_size_expression<__VA_ARGS__>::value);       \
        D_CCT_CHECK(constexpr_container_class<__VA_ARGS__>::has_iteration ==  \
                    has_constexpr_iteration<__VA_ARGS__>::value);             \
        D_CCT_CHECK(constexpr_container_class<__VA_ARGS__>::life ==           \
                    container_lifetime<__VA_ARGS__>::value);                  \
        D_CCT_CHECK(constexpr_container_class<__VA_ARGS__>::is_constexpr ==   \
                    is_constexpr_container<__VA_ARGS__>::value)

    D_CCT_SNAPSHOT_AGREES(std::array<int, 3>);
    D_CCT_SNAPSHOT_AGREES(std::vector<int>);
    D_CCT_SNAPSHOT_AGREES(tag_true);
    D_CCT_SNAPSHOT_AGREES(tag_true_nonliteral);
    D_CCT_SNAPSHOT_AGREES(tag_false_with_extent);
    D_CCT_SNAPSHOT_AGREES(extent_static);
    D_CCT_SNAPSHOT_AGREES(extent_nonliteral);
    D_CCT_SNAPSHOT_AGREES(size_constexpr);
    D_CCT_SNAPSHOT_AGREES(size_runtime);
    D_CCT_SNAPSHOT_AGREES(tuple_conformer);
    D_CCT_SNAPSHOT_AGREES(tuple_conformer_nonliteral);
    D_CCT_SNAPSHOT_AGREES(iter_alias);
    D_CCT_SNAPSHOT_AGREES(iter_pair);
    D_CCT_SNAPSHOT_AGREES(life_both_sized);
    D_CCT_SNAPSHOT_AGREES(life_compile_sized);
    D_CCT_SNAPSHOT_AGREES(life_runtime_sized);
    D_CCT_SNAPSHOT_AGREES(life_none_sized);
    D_CCT_SNAPSHOT_AGREES(life_compile_unsized);
    D_CCT_SNAPSHOT_AGREES(no_signal);
    D_CCT_SNAPSHOT_AGREES(int);

    // it cleans its argument, so the cv-ref forms agree with the bare one
    D_CCT_SNAPSHOT_AGREES(const std::array<int, 3>&);
    D_CCT_SNAPSHOT_AGREES(std::vector<int>&&);

    #undef D_CCT_SNAPSHOT_AGREES

    return ok;
}

/*
tests_class_tuple_size_gap
  THE FINDING.  Five signals feed size_lifetime; the snapshot reports four.

  size_lifetime is promoted by:
      has_constexpr_container_tag     -> reported as has_tag
      has_constexpr_extent            -> reported as has_extent
      has_tuple_size                  -> NOT REPORTED
      has_constexpr_size_expression   -> reported as has_size_expr
      has_constexpr_iteration         -> reported as has_iteration

  tuple_size is the one signal with no member on constexpr_container_class, and
  it is precisely the signal that reaches where the others cannot - it is what
  lets a fixed-size aggregate of NON-literal elements be recognised as
  compile-time-sized, which is the case the header's own comment singles out.

  Tests the following:
  - tuple_conformer is promoted by tuple_size and by nothing else
  - its snapshot therefore reads has_tag=0, has_extent=0, has_size_expr=0,
    has_iteration=0 - "no compile-time signals at all"
  - and its verdict reads life=both, is_constexpr=true

  A diagnostic that reports no evidence and a positive verdict is not a
  diagnostic.  The fix is one member; this check is what would then need a line
  adding to it.
*/
bool
tests_class_tuple_size_gap()
{
    bool ok = true;

    using cc = constexpr_container_class<tuple_conformer>;

    // exactly one signal fires, and it is the unreported one
    D_CCT_CHECK(has_tuple_size<tuple_conformer>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<tuple_conformer>::value);
    D_CCT_CHECK(!has_constexpr_extent<tuple_conformer>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<tuple_conformer>::value);
    D_CCT_CHECK(!has_constexpr_iteration<tuple_conformer>::value);

    // the snapshot shows nothing...
    D_CCT_CHECK(!cc::has_tag);
    D_CCT_CHECK(!cc::has_extent);
    D_CCT_CHECK(!cc::has_size_expr);
    D_CCT_CHECK(!cc::has_iteration);

    // ...and reports a positive verdict anyway
    D_CCT_CHECK(cc::life == lifetime::both);
    D_CCT_CHECK(cc::is_constexpr);

    // the same contradiction, one step earlier
    D_CCT_LIFE(lifetime::both, size_lifetime<tuple_conformer>::value);

    // and it is not confined to the fixture: std::tuple and std::pair are
    // promoted by tuple_size alone as well
    D_CCT_CHECK(has_tuple_size<std::tuple<int, char>>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<std::tuple<int, char>>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<std::tuple<int, char>>::value);
    D_CCT_CHECK(
        !constexpr_container_class<std::tuple<int, char>>::has_size_expr);

    return ok;
}

/*
tests_class_is_constexpr_and_empty
  Every member is a static constexpr, so the whole snapshot is usable at compile
  time and costs nothing at run time.
  Tests the following:
  - the bools are constant expressions (each is used as a template argument,
    which is the only way to prove it)
  - the lifetime member is a constant expression
  - the class is empty - it carries answers, not storage
*/
bool
tests_class_is_constexpr_and_empty()
{
    bool ok = true;

    using arr = constexpr_container_class<std::array<int, 3>>;
    using vec = constexpr_container_class<std::vector<int>>;

    // bool constant expressions: usable as template arguments
    D_CCT_CHECK((std::integral_constant<bool, arr::is_constexpr>::value));
    D_CCT_CHECK((std::integral_constant<bool, arr::has_size_expr>::value));
    D_CCT_CHECK((!std::integral_constant<bool, vec::is_constexpr>::value));
    D_CCT_CHECK((!std::integral_constant<bool, vec::has_tag>::value));

    // a lifetime constant expression
    D_CCT_CHECK((std::integral_constant<lifetime, arr::life>::value ==
                 lifetime::both));
    D_CCT_CHECK((std::is_same<lifetime_constant<arr::life>,
                              both_lifetime>::value));

    // and it is a pure compile-time carrier
    D_CCT_CHECK(std::is_empty<arr>::value);
    D_CCT_CHECK(std::is_empty<vec>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
