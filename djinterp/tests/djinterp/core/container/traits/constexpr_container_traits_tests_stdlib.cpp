/*******************************************************************************
* djinterp [core]                    constexpr_container_traits_tests_stdlib.cpp
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests_stdlib.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
// djinterp
#include "constexpr_container_traits_tests.hpp"
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // std
    #include <version>
#endif


NS_DJINTERP
NS_TESTING


/*
tests_stdlib_array
  std::array - the case the whole header is aimed at.
  Tests the following:
  - promoted by TWO signals at once: std::tuple_size, and the size expression
    (std::array<int, N>{} is a constant expression and its size() is constexpr)
  - its contents are literal, so lifetime_of is `both` and the meet is `both`
  - so it IS a constexpr container - the one unambiguous success
  - the zero-length array too
  - it carries no `extent` member, so the extent disjunct is NOT what promotes
    it, despite the header's comment naming "our array<>::extent" alongside
    std::span - that convention belongs to the djinterp array, not to std::array
*/
bool
tests_stdlib_array()
{
    bool ok = true;

    D_CCT_CHECK(is_constexpr_container<std::array<int, 3>>::value);
    D_CCT_CHECK(is_constexpr_container<std::array<int, 0>>::value);
    D_CCT_CHECK(is_constexpr_container<std::array<char, 64>>::value);

    D_CCT_LIFE(lifetime::both, container_lifetime<std::array<int, 3>>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<std::array<int, 3>>::value);
    D_CCT_LIFE(lifetime::both, lifetime_of<std::array<int, 3>>::value);

    // two signals promote it, and `extent` is not one of them
    D_CCT_CHECK(has_tuple_size<std::array<int, 3>>::value);
    D_CCT_CHECK(has_constexpr_size_expression<std::array<int, 3>>::value);
    D_CCT_CHECK(!has_constexpr_extent<std::array<int, 3>>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<std::array<int, 3>>::value);
    D_CCT_CHECK(!has_constexpr_iteration<std::array<int, 3>>::value);

    return ok;
}

/*
tests_stdlib_vector_string
  THE FINDING.  On a C++20 library, has_constexpr_size_expression fires on
  std::vector and std::string.

  C++20 made both types constexpr: their default constructors and their size()
  are constexpr, and an EMPTY vector or a short string allocates nothing.  So
  `std::vector<int>{}.size()` really is a constant expression, the probe really
  does fire, and size_lifetime<std::vector<int>> is `both` - the trait asserts
  that a vector has a compile-time-expressible size.

  The final verdict survives, but only by accident of the composition:
  lifetime_of<std::vector<int>> is `runtime` (a vector is not trivially
  destructible), and meet(both, runtime) = runtime, so is_constexpr_container
  still says false.  The RIGHT answer, reached through a WRONG intermediate - and
  any caller who reads size_lifetime directly, as the Storage axis is documented
  to do ("the same fixed-extent evidence drives both axes"), gets the wrong one.

  The fix is not to weaken the probe but to recognise that "T{}.size() is
  constant" stopped meaning "the size is fixed by the type" the moment
  containers became constexpr.  It now means "an empty T can be built at compile
  time", which every constexpr container can do.  has_tuple_size and
  has_constexpr_extent are the signals that still mean what they say.

  Tests the following:
  - the expectations are gated on the library feature-test macros, so this suite
    states the right answer on whatever library it is built against
  - std::list and std::deque do NOT fire the probe (their default ctors are not
    constexpr in libstdc++), so the standard library is classified INCONSISTENTLY
    - which is the surest sign the signal is measuring the wrong thing
*/
bool
tests_stdlib_vector_string()
{
    bool ok = true;

#if defined(__cpp_lib_constexpr_vector) && (__cpp_lib_constexpr_vector >= 201907L)

    // C++20: vector{} is a constant expression, so the SIZE probe fires
    D_CCT_CHECK(has_constexpr_size_expression<std::vector<int>>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<std::vector<int>>::value);

#else

    D_CCT_CHECK(!has_constexpr_size_expression<std::vector<int>>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<std::vector<int>>::value);

#endif  // __cpp_lib_constexpr_vector

#if defined(__cpp_lib_constexpr_string) && (__cpp_lib_constexpr_string >= 201907L)

    D_CCT_CHECK(has_constexpr_size_expression<std::string>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<std::string>::value);

    // the same thing one level up: a plain struct holding a std::string is now
    // "compile-time sized" too
    D_CCT_CHECK(has_constexpr_size_expression<size_with_string_member>::value);

#else

    D_CCT_CHECK(!has_constexpr_size_expression<std::string>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<std::string>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<size_with_string_member>::value);

#endif  // __cpp_lib_constexpr_string

    // whatever the library, the CONTENTS are runtime and the meet rescues the
    // verdict - this is the invariant that actually holds
    D_CCT_LIFE(lifetime::runtime, lifetime_of<std::vector<int>>::value);
    D_CCT_LIFE(lifetime::runtime, lifetime_of<std::string>::value);
    D_CCT_LIFE(lifetime::runtime, container_lifetime<std::vector<int>>::value);
    D_CCT_LIFE(lifetime::runtime, container_lifetime<std::string>::value);
    D_CCT_CHECK(!is_constexpr_container<std::vector<int>>::value);
    D_CCT_CHECK(!is_constexpr_container<std::string>::value);
    D_CCT_CHECK(!is_constexpr_container<size_with_string_member>::value);

    // and no other signal is fooled
    D_CCT_CHECK(!has_tuple_size<std::vector<int>>::value);
    D_CCT_CHECK(!has_constexpr_extent<std::vector<int>>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<std::vector<int>>::value);

    return ok;
}

/*
tests_stdlib_node_containers
  std::list, std::deque, std::set, std::map.
  Tests the following:
  - none of them fires ANY signal - their default constructors are not constexpr
    in libstdc++, so even the size expression declines
  - all of them are runtime on both halves, and none is a constexpr container

  The contrast with std::vector is the point.  vector and list are both growable,
  both heap-backed, and both equally not-compile-time-sized; the probe separates
  them purely on whether their library authors got round to marking the default
  constructor constexpr.  That is a property of libstdc++, not of the container.
*/
bool
tests_stdlib_node_containers()
{
    bool ok = true;

    #define D_CCT_NO_SIGNAL(...)                                              \
        D_CCT_CHECK(!has_constexpr_container_tag<__VA_ARGS__>::value);        \
        D_CCT_CHECK(!has_constexpr_extent<__VA_ARGS__>::value);               \
        D_CCT_CHECK(!has_tuple_size<__VA_ARGS__>::value);                     \
        D_CCT_CHECK(!has_constexpr_size_expression<__VA_ARGS__>::value);      \
        D_CCT_CHECK(!has_constexpr_iteration<__VA_ARGS__>::value);            \
        D_CCT_CHECK(size_lifetime<__VA_ARGS__>::value == lifetime::runtime);  \
        D_CCT_CHECK(container_lifetime<__VA_ARGS__>::value ==                 \
                    lifetime::runtime);                                       \
        D_CCT_CHECK(!is_constexpr_container<__VA_ARGS__>::value);             \
        D_CCT_CHECK(is_not_constexpr_container<__VA_ARGS__>::value)

    D_CCT_NO_SIGNAL(std::list<int>);
    D_CCT_NO_SIGNAL(std::deque<int>);
    D_CCT_NO_SIGNAL(std::set<int>);
    D_CCT_NO_SIGNAL(std::map<int, int>);

    #undef D_CCT_NO_SIGNAL

    return ok;
}

/*
tests_stdlib_tuple_pair
  THE SECOND FINDING.  std::tuple and std::pair are NOT constexpr containers.

  Both are promoted on the SIZE axis - std::tuple_size sees them, as it must.
  Both fail on the CONTENTS axis, because lifetime_of routes through
  is_literal_type, which on C++17+ is the heuristic

      is_scalar<T> ||
      ( is_trivially_destructible<T> && is_trivially_default_constructible<T> )

  and libstdc++'s std::tuple and std::pair have USER-PROVIDED default
  constructors.  Those constructors are constexpr - so both types are perfectly
  usable in constant evaluation, and `constexpr std::pair<int, char> p{};`
  compiles - but they are not TRIVIALLY default-constructible, so the heuristic
  says "not literal", lifetime_of says `runtime`, and the meet says `runtime`.

  std::array escapes only because it is an aggregate.

  This is a false negative on two of the most obviously constexpr-usable types in
  the language, and it is not fixable in this header: the defect is
  is_literal_type in meta/lifetime.hpp.  `is_trivially_default_constructible` was
  never the right proxy for "literal" - the C++11 definition of a literal class
  requires a CONSTEXPR default constructor (or that it be an aggregate), not a
  trivial one.  There is no perfect replacement now that std::is_literal_type is
  gone, but `is_trivially_destructible<T> && is_constructible<T>` plus a
  constexpr-constructibility probe (the same T{} trick this header already uses,
  guarded) would recover tuple and pair.

  Tests the following:
  - the size half is promoted, the contents half is not, for both types
  - the verdict is therefore false
  - and the types really ARE constexpr-constructible, which is what makes it a
    false negative rather than a judgement call
  - the sharpest form of it: std::tuple<> IS a constexpr container and
    std::tuple<int> is NOT.  Adding one int to a tuple flips the verdict, because
    libstdc++ gives the empty specialization a trivial default constructor and
    the general template a user-provided one.  The trait is measuring the
    library, not the language
*/
bool
tests_stdlib_tuple_pair()
{
    bool ok = true;

    // the SIZE half is fine
    D_CCT_CHECK(has_tuple_size<std::tuple<int, char>>::value);
    D_CCT_CHECK(has_tuple_size<std::pair<int, char>>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<std::tuple<int, char>>::value);
    D_CCT_LIFE(lifetime::both, size_lifetime<std::pair<int, char>>::value);

    // the CONTENTS half is not
    D_CCT_LIFE(lifetime::runtime, lifetime_of<std::tuple<int, char>>::value);
    D_CCT_LIFE(lifetime::runtime, lifetime_of<std::pair<int, char>>::value);
    D_CCT_CHECK(!is_literal_type<std::tuple<int, char>>::value);
    D_CCT_CHECK(!is_literal_type<std::pair<int, char>>::value);

    // ...because of exactly one clause of the heuristic
    D_CCT_CHECK(std::is_trivially_destructible<std::pair<int, char>>::value);
    D_CCT_CHECK(!std::is_trivially_default_constructible<
                    std::pair<int, char>>::value);

    // so the verdict is false
    D_CCT_LIFE(lifetime::runtime,
               container_lifetime<std::tuple<int, char>>::value);
    D_CCT_CHECK(!is_constexpr_container<std::tuple<int, char>>::value);
    D_CCT_CHECK(!is_constexpr_container<std::pair<int, char>>::value);

    // AND THE SHARP EDGE.  std::tuple<> - the EMPTY tuple - IS trivially
    // default-constructible (libstdc++ specializes it), so it passes the
    // heuristic, and it IS a constexpr container.  std::tuple<int> is not.
    //
    //   Adding one int to a tuple stops it being a constexpr container.
    //
    // Nothing about the type's constexpr-usability changed; only whether
    // libstdc++ happened to give that specialization a trivial default
    // constructor.  A trait whose answer turns on that is measuring the library,
    // not the language
    D_CCT_CHECK(std::is_trivially_default_constructible<std::tuple<>>::value);
    D_CCT_CHECK(!std::is_trivially_default_constructible<
                    std::tuple<int>>::value);

    D_CCT_CHECK(is_constexpr_container<std::tuple<>>::value);
    D_CCT_CHECK(!is_constexpr_container<std::tuple<int>>::value);

    D_CCT_LIFE(lifetime::both,    container_lifetime<std::tuple<>>::value);
    D_CCT_LIFE(lifetime::runtime, container_lifetime<std::tuple<int>>::value);

    // ...and yet both types ARE constexpr-constructible.  If this line compiles,
    // the verdict above is wrong
    D_CONSTEXPR std::pair<int, char>      p{};
    D_CONSTEXPR std::tuple<int, char>     t{};

    D_CCT_CHECK(p.first == 0);
    D_CCT_CHECK(std::get<0>(t) == 0);

    // std::array escapes only because it is an aggregate
    D_CCT_CHECK(std::is_trivially_default_constructible<
                    std::array<int, 3>>::value);
    D_CCT_CHECK(is_constexpr_container<std::array<int, 3>>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
