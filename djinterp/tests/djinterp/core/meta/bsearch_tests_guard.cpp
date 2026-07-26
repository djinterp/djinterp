#include "bsearch_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_guard_true_branch_indexes
  pack_element_or with _Ok == true is ordinary indexed access.
  Tests the following:
  - index 0 yields the head of the pack
  - an interior index yields the corresponding element
  - the final index yields the last element
  - the result equals pack_element_t at the same index
*/
bool
tests_guard_true_branch_indexes()
{
    // head
    D_BS_CHECK((std::is_same<pack_element_or_t<true, 0, ikey<0>, ikey<1>, ikey<2>, ikey<3> >,
                             ikey<1> >::value));

    // interior
    D_BS_CHECK((std::is_same<pack_element_or_t<true, 2, ikey<0>, ikey<1>, ikey<2>, ikey<3> >,
                             ikey<3> >::value));

    // last (the pack after the fallback is 3 entries: indices 0..2)
    D_BS_CHECK((std::is_same<pack_element_or_t<true, 2, ikey<9>, ikey<1>, ikey<2>, ikey<7> >,
                             ikey<7> >::value));

    // agrees with the unguarded accessor over the SAME pack (the fallback
    // slot sits before the pack, so pack_element_t indexes {int, double})
    D_BS_CHECK((std::is_same<pack_element_or_t<true, 1, char, int, double>,
                             pack_element_t<1, int, double> >::value));

    return true;
}

/*
tests_guard_false_branch_returns_fallback
  pack_element_or with _Ok == false is the fallback, unconditionally.
  Tests the following:
  - the fallback type is returned verbatim
  - the index value is irrelevant when _Ok is false
  - the pack contents are irrelevant when _Ok is false
  - a fallback that is itself one of the pack types is still just the fallback
*/
bool
tests_guard_false_branch_returns_fallback()
{
    // the fallback comes back
    D_BS_CHECK((std::is_same<pack_element_or_t<false, 0, lookup_not_found, int, char>,
                             lookup_not_found>::value));

    // the index does not matter
    D_BS_CHECK((std::is_same<pack_element_or_t<false, 2, lookup_not_found, int, char>,
                             lookup_not_found>::value));
    D_BS_CHECK((std::is_same<pack_element_or_t<false, 99, lookup_not_found, int, char>,
                             lookup_not_found>::value));

    // an arbitrary fallback type is honoured
    D_BS_CHECK((std::is_same<pack_element_or_t<false, 1, double, int, char>,
                             double>::value));

    // even a fallback that also appears in the pack is returned as the fallback
    D_BS_CHECK((std::is_same<pack_element_or_t<false, 0, int, int, char>,
                             int>::value));

    return true;
}

/*
tests_guard_defers_instantiation_at_invalid_index
  The false branch never instantiates pack_element_t (the header's whole point).
  Tests the following:
  - the guarded form compiles at lookup_npos, an index whose DIRECT access is a
    hard error (this TU compiling at all is half the assertion)
  - it yields the fallback at that catastrophic index
  - it yields the fallback at another out-of-range index
  - the guard is what defers: a std::conditional_t here would form both branch
    types and hard-error, which is exactly why pack_element_or exists
*/
bool
tests_guard_defers_instantiation_at_invalid_index()
{
    // lookup_npos as an index would recurse ~2^64 deep if ever instantiated;
    // the guard must keep pack_element_t untouched.  That this compiles is the
    // constructive evidence.
    using guarded_npos =
        pack_element_or_t<false, lookup_npos, lookup_not_found, int, char>;

    D_BS_CHECK((std::is_same<guarded_npos, lookup_not_found>::value));

    // another plainly out-of-range index, also deferred
    using guarded_oob =
        pack_element_or_t<false, 1000, lookup_not_found, int, char>;

    D_BS_CHECK((std::is_same<guarded_oob, lookup_not_found>::value));

    // and the compile-time face of the same fact
    static_assert(
        std::is_same<
            pack_element_or_t<false, lookup_npos, lookup_not_found, int>,
            lookup_not_found>::value,
        "false-branch pack_element_or must not instantiate pack_element_t");

    return true;
}

/*
tests_guard_alias_matches_trait
  pack_element_or_t is a faithful shorthand for pack_element_or<...>::type.
  Tests the following:
  - the alias and the ::type member agree on the true branch
  - they agree on the false branch
  - they agree at an interior index
  - the alias resolves to the expected concrete type, not merely to itself
*/
bool
tests_guard_alias_matches_trait()
{
    // true branch
    D_BS_CHECK((std::is_same<
                    pack_element_or_t<true, 0, lookup_not_found, int, char>,
                    pack_element_or<true, 0, lookup_not_found, int, char>::type
                >::value));

    // false branch
    D_BS_CHECK((std::is_same<
                    pack_element_or_t<false, 0, lookup_not_found, int, char>,
                    pack_element_or<false, 0, lookup_not_found, int, char>::type
                >::value));

    // interior index
    D_BS_CHECK((std::is_same<
                    pack_element_or_t<true, 2, lookup_not_found, int, char, double>,
                    pack_element_or<true, 2, lookup_not_found, int, char, double>::type
                >::value));

    // resolves to the expected type
    D_BS_CHECK((std::is_same<
                    pack_element_or_t<true, 1, lookup_not_found, int, char, double>,
                    char>::value));

    return true;
}

/*
tests_guard_independent_of_pack_contents
  The fallback branch is independent of the pack; the access branch reads it.
  Tests the following:
  - a false-branch access with an empty pack gives the fallback
  - a false-branch access with a populated pack gives the same fallback
  - a true-branch access reads whichever pack is present
  - two different packs with a true-branch access at the same index differ
    accordingly
*/
bool
tests_guard_independent_of_pack_contents()
{
    // false branch, empty pack
    D_BS_CHECK((std::is_same<pack_element_or_t<false, 0, lookup_not_found>,
                             lookup_not_found>::value));

    // false branch, populated pack -- same answer
    D_BS_CHECK((std::is_same<pack_element_or_t<false, 0, lookup_not_found, int, char>,
                             lookup_not_found>::value));

    // true branch reads the pack in front of it
    D_BS_CHECK((std::is_same<pack_element_or_t<true, 0, lookup_not_found, int>,
                             int>::value));
    D_BS_CHECK((std::is_same<pack_element_or_t<true, 0, lookup_not_found, char>,
                             char>::value));

    // and distinct packs at one index give distinct results
    D_BS_CHECK(!(std::is_same<
                     pack_element_or_t<true, 0, lookup_not_found, int, char>,
                     pack_element_or_t<true, 0, lookup_not_found, double, char>
                 >::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
