// djinterp [test]  reduce_tests_type.cpp
//   III. reduce_ct over a std::tuple -- the TYPE domain. The recursion walks the
//   element TYPES, feeding each as a type_c carrier; no tuple value is built.

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "reduce_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_ct_tuple_type_c_entry
  The PREFERRED type-domain entry: the sequence is carried as a type,
  type_c<std::tuple<Ts...>>.
  Tests the following:
  - a sum over the element sizes, and a count of the elements
  - the seed participates as the leftmost accumulator
*/
bool
tests_ct_tuple_type_c_entry()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using tup = std::tuple<char, int, double>;      // sizeof 1, 4, 8

    static_assert(reduce_ct(sum_with<type_leaf>(), 0, type_c<tup>) == 13,
                  "1 + 4 + 8");
    static_assert(reduce_ct(count_all(), 0, type_c<tup>) == 3, "three elements");
    static_assert(reduce_ct(sum_with<type_leaf>(), 100, type_c<tup>) == 113,
                  "the seed counts");

    ok = ok && (reduce_ct(count_all(), 0, type_c<tup>) == 3);
#endif

    return ok;
}


/*
tests_ct_tuple_value_entry
  The CONVENIENCE entry, matching the value-passing style: a std::tuple value is
  passed and immediately discarded (the parameter is unnamed).
  Tests the following:
  - the same folds work through the value form
*/
bool
tests_ct_tuple_value_entry()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using tup = std::tuple<char, int, double>;

    static_assert(reduce_ct(sum_with<type_leaf>(), 0, tup{}) == 13, "1 + 4 + 8");
    static_assert(reduce_ct(count_all(), 0, tup{}) == 3, "three elements");
    static_assert(reduce_ct(digits_type(), 0, tup{}) == 148, "left fold");

    // a populated tuple value folds identically -- only its TYPE is consulted.
    const tup populated{ 'a', 7, 2.5 };
    ok = ok && (reduce_ct(count_all(), 0, populated) == 3);
    ok = ok && (reduce_ct(sum_with<type_leaf>(), 0, populated) == 13);
#endif

    return ok;
}


/*
tests_ct_tuple_both_entries_agree
  Both entries route to the same recursion, so they must agree exactly.
  Tests the following:
  - the two give the same value and the same result TYPE, for several reducers
  - they agree on the empty tuple too
*/
bool
tests_ct_tuple_both_entries_agree()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using tup = std::tuple<char, int, double>;

    static_assert(reduce_ct(sum_with<type_leaf>(), 0, type_c<tup>) ==
                  reduce_ct(sum_with<type_leaf>(), 0, tup{}), "sum");
    static_assert(reduce_ct(digits_type(), 0, type_c<tup>) ==
                  reduce_ct(digits_type(), 0, tup{}), "left fold");
    static_assert(reduce_ct(count_all(), 0, type_c<std::tuple<> >) ==
                  reduce_ct(count_all(), 0, std::tuple<>{}), "empty");

    static_assert(std::is_same<
        decltype(reduce_ct(sizes_into_list(), value_list<>{}, type_c<tup>)),
        decltype(reduce_ct(sizes_into_list(), value_list<>{}, tup{}))
        >::value, "the same result type");

    ok = ok && (reduce_ct(count_all(), 0, type_c<tup>) ==
                reduce_ct(count_all(), 0, tup{}));
#endif

    return ok;
}


/*
tests_ct_tuple_empty_returns_the_seed
  The base case: the empty tuple folds to the accumulator, untouched -- the
  reducer is never invoked.
  Tests the following:
  - the seed comes back, by value and by type, through both entries
  - the reducer is not called
*/
bool
tests_ct_tuple_empty_returns_the_seed()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(reduce_ct(sum_with<type_leaf>(), 42,
                            type_c<std::tuple<> >) == 42, "the seed");
    static_assert(reduce_ct(sum_with<type_leaf>(), 42, std::tuple<>{}) == 42,
                  "through the value form too");
    static_assert(std::is_same<
        decltype(reduce_ct(sum_with<type_leaf>(), 42, type_c<std::tuple<> >)),
        int>::value, "and its type");

    // a list-shaped accumulator comes back untouched.
    static_assert(std::is_same<
        decltype(reduce_ct(sizes_into_list(), value_list<>{},
                           type_c<std::tuple<> >)),
        value_list<> >::value, "list seed");

    // and the reducer is never called.
    int          calls = 0;
    counting_any counter{ &calls };
    reduce_ct(counter, 0, type_c<std::tuple<> >);
    ok = ok && (calls == 0);
#endif

    return ok;
}


/*
tests_ct_tuple_is_a_left_fold
  The recursion folds the HEAD type into the accumulator and recurses on the tail,
  so the element types arrive first-to-last. A non-commutative reducer over the
  sizes pins that: char, int, double (1, 4, 8) from a seed of 0 gives 148.
  Tests the following:
  - the digits reducer yields 148, and not the reverse
  - reordering the tuple's elements reorders the fold
*/
bool
tests_ct_tuple_is_a_left_fold()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using tup = std::tuple<char, int, double>;      // 1, 4, 8

    static_assert(reduce_ct(digits_type(), 0, type_c<tup>) == 148,
                  "((0*10 + 1)*10 + 4)*10 + 8");
    static_assert(reduce_ct(digits_type(), 0, type_c<tup>) != 841,
                  "not a right fold");

    // reversing the tuple reverses the fold.
    using rev = std::tuple<double, int, char>;      // 8, 4, 1
    static_assert(reduce_ct(digits_type(), 0, type_c<rev>) == 841,
                  "the other order");

    ok = ok && (reduce_ct(digits_type(), 0, type_c<tup>) == 148);
#endif

    return ok;
}


/*
tests_ct_tuple_walks_types_not_values
  The headline advantage of the preferred entry: the recursion walks element TYPES
  and never builds a tuple value, so the element types need NOT be
  default-constructible. The convenience entry, which takes a tuple by value,
  cannot even be called on such a tuple.
  Tests the following:
  - a tuple whose element cannot be default-constructed is folded by the type_c
    entry
  - that same tuple is genuinely not default-constructible
  - the VALUE entry is not callable for it, while it is for an ordinary tuple
*/
bool
tests_ct_tuple_walks_types_not_values()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using hard = std::tuple<no_default, int>;

    // the tuple really cannot be default-constructed.
    static_assert(!std::is_default_constructible<no_default>::value,
                  "the element cannot");
    static_assert(!std::is_default_constructible<hard>::value,
                  "so neither can the tuple");

    // ...and the type_c entry folds it anyway.
    static_assert(reduce_ct(count_all(), 0, type_c<hard>) == 2,
                  "two element TYPES walked");
    static_assert(reduce_ct(sum_with<type_leaf>(), 0, type_c<hard>) ==
                  static_cast<int>(sizeof(no_default) + sizeof(int)),
                  "and their sizes summed");

    // the VALUE entry cannot be called for it -- but can for an ordinary tuple.
    static_assert(!can_value_entry<count_all, int, hard>::value,
                  "the convenience form needs default-constructible elements");
    static_assert(can_value_entry<count_all, int, std::tuple<int, char> >::value,
                  "which an ordinary tuple has");

    ok = ok && (reduce_ct(count_all(), 0, type_c<hard>) == 2);
#endif

    return ok;
}


/*
tests_ct_tuple_feeds_each_type_as_a_carrier
  Each element type is handed to the reducer as a type_c carrier, so the reducer
  is a type-domain leaf: it recovers the type from the carrier, never from a value.
  Tests the following:
  - the reducer receives a type_t<T> for each element
  - the carried type is recoverable, and is the tuple's element type
*/
bool
tests_ct_tuple_feeds_each_type_as_a_carrier()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // the leaf's parameter really is a carrier, and it yields the type's size.
    static_assert(type_leaf()(type_c<double>) ==
                  static_cast<int>(sizeof(double)), "the leaf reads the carrier");

    // the carrier round-trips the type.
    static_assert(std::is_same<type_t<int>::type, int>::value, "carried type");
    static_assert(std::is_same<
        decltype(type_c<std::tuple<int> >), const type_t<std::tuple<int> > >::value,
        "type_c is a type_t carrier");

    // and the fold sees each element type in turn.
    static_assert(reduce_ct(sum_with<type_leaf>(), 0,
                            type_c<std::tuple<char> >) == 1, "char");
    static_assert(reduce_ct(sum_with<type_leaf>(), 0,
                            type_c<std::tuple<char, char> >) == 2, "char, char");

    ok = ok && (type_leaf()(type_c<char>) == 1);
#endif

    return ok;
}


/*
tests_ct_tuple_duplicate_types
  The recursion walks POSITIONS, not distinct types: a repeated element type is
  visited once per occurrence.
  Tests the following:
  - a tuple of three identical types has three elements folded
  - mixed duplicates are all counted
*/
bool
tests_ct_tuple_duplicate_types()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(reduce_ct(count_all(), 0,
                            type_c<std::tuple<int, int, int> >) == 3,
                  "three positions, one type");
    static_assert(reduce_ct(sum_with<type_leaf>(), 0,
                            type_c<std::tuple<char, char, char> >) == 3,
                  "each contributes its size");
    static_assert(reduce_ct(count_all(), 0,
                            type_c<std::tuple<int, char, int, char> >) == 4,
                  "mixed duplicates");

    ok = ok && (reduce_ct(count_all(), 0,
                          type_c<std::tuple<int, int, int> >) == 3);
#endif

    return ok;
}


/*
tests_ct_tuple_accumulator_may_evolve
  The recursion recomputes the accumulator type at every step -- it is
  decltype(rf(acc, type_c<T0>)) -- so, as in the value domain and unlike reduce_rt,
  the accumulator's TYPE may change as the fold proceeds.
  Tests the following:
  - folding the element sizes into a growing value_list changes the accumulator
    type at each step, ending as the list of sizes
  - the empty tuple still yields the seed's type
*/
bool
tests_ct_tuple_accumulator_may_evolve()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using tup = std::tuple<char, int, double>;      // 1, 4, 8

    static_assert(std::is_same<
        decltype(reduce_ct(sizes_into_list(), value_list<>{}, type_c<tup>)),
        value_list<sizeof(char), sizeof(int), sizeof(double)> >::value,
        "the accumulator TYPE grew, element by element");

    // the same through the value entry.
    static_assert(std::is_same<
        decltype(reduce_ct(sizes_into_list(), value_list<>{}, tup{})),
        value_list<sizeof(char), sizeof(int), sizeof(double)> >::value,
        "and through the value form");

    // an empty tuple leaves the seed's type alone.
    static_assert(std::is_same<
        decltype(reduce_ct(sizes_into_list(), value_list<>{},
                           type_c<std::tuple<> >)),
        value_list<> >::value, "empty");

    ok = ok && (decltype(reduce_ct(sizes_into_list(), value_list<>{},
                                   type_c<tup>))::size() == 3u);
#endif

    return ok;
}


/*
tests_ct_tuple_single_element
  The one-element case: exactly one turn of the recursion before the base case.
  Tests the following:
  - the reducer is applied once, through both entries
*/
bool
tests_ct_tuple_single_element()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(reduce_ct(count_all(), 0, type_c<std::tuple<int> >) == 1,
                  "one element");
    static_assert(reduce_ct(count_all(), 0, std::tuple<int>{}) == 1,
                  "through the value form");
    static_assert(reduce_ct(sum_with<type_leaf>(), 0,
                            type_c<std::tuple<double> >) ==
                  static_cast<int>(sizeof(double)), "its size");
    static_assert(reduce_ct(digits_type(), 0, type_c<std::tuple<char> >) == 1,
                  "one step of the left fold");

    ok = ok && (reduce_ct(count_all(), 0, type_c<std::tuple<int> >) == 1);
#endif

    return ok;
}


/*
tests_ct_tuple_heterogeneous_sizes
  The tuple is a sequence of unrelated types, and the leaf reads a fact about each
  one in turn.
  Tests the following:
  - the sizes of several unrelated element types are summed correctly
  - a nested tuple is a single element, not flattened
*/
bool
tests_ct_tuple_heterogeneous_sizes()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // unrelated types.
    static_assert(reduce_ct(sum_with<type_leaf>(), 0,
                            type_c<std::tuple<char, short, int> >) ==
                  static_cast<int>(sizeof(char) + sizeof(short) + sizeof(int)),
                  "three unrelated types");

    // a nested tuple counts as ONE element -- the recursion does not flatten.
    static_assert(reduce_ct(count_all(), 0,
                            type_c<std::tuple<int, std::tuple<int, int> > >) == 2,
                  "two elements, one of them a tuple");

    ok = ok && (reduce_ct(count_all(), 0,
                          type_c<std::tuple<int, std::tuple<int, int> > >) == 2);
#endif

    return ok;
}


/*
tests_ct_tuple_visits_each_element_once
  The recursion peels exactly one head type per step -- none skipped, none folded
  twice. (reduce_ct is constexpr but runs perfectly well at runtime, so a
  recording reducer can observe the visits.)
  Tests the following:
  - the reducer is called once per element, for several tuple lengths
  - an empty tuple calls it not at all
*/
bool
tests_ct_tuple_visits_each_element_once()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    int          calls = 0;
    counting_any counter{ &calls };

    reduce_ct(counter, 0, type_c<std::tuple<char, int, double> >);
    ok = ok && (calls == 3);

    calls = 0;
    reduce_ct(counter, 0, type_c<std::tuple<int, int, int, int, int> >);
    ok = ok && (calls == 5);

    calls = 0;
    reduce_ct(counter, 0, type_c<std::tuple<> >);
    ok = ok && (calls == 0);

    // the value entry visits the same elements.
    calls = 0;
    reduce_ct(counter, 0, std::tuple<char, int>{});
    ok = ok && (calls == 2);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
