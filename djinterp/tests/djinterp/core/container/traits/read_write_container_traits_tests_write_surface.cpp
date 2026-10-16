/*******************************************************************************
* djinterp [core]            read_write_container_traits_tests_write_surface.cpp
*
*
* path:      /tests/djinterp/core/container/traits/read_write_container_traits_tests_write_surface.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I-b. The write surface
// ===========================================================================
//   Five probes, joined by a disjunction.  Each asks whether some mutator is
// reachable on a NON-CONST instance.  Four of the five name value_type to
// build their call; the clear probe does not, and that asymmetry is worth
// knowing about - see tests_write_surface_value_type_asymmetry.


/*
tests_has_push_back_helper
  Verifies the append-at-the-end probe.
  Tests the following:
  - a push_back callable with a value_type is detected
  - the back-insertable standard containers carry it
  - a forward_list does NOT: it appends at the front only
  - a std::array does not: it is fixed-size
  - the associative containers do not: they have insert(), not push_back()
  - a push_back on a type with no value_type alias is NOT detected, the probe
    naming value_type to build its argument
*/
bool
tests_has_push_back_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_push_back_helper<rw_write_push_back>::value
                   == true);
    D_RW_CHECK(internal::has_push_back_helper<rw_vector>::value == true);
    D_RW_CHECK(internal::has_push_back_helper<rw_deque>::value  == true);
    D_RW_CHECK(internal::has_push_back_helper<rw_list>::value   == true);
    D_RW_CHECK(internal::has_push_back_helper<rw_string>::value == true);

    // absent
    D_RW_CHECK(internal::has_push_back_helper<rw_flist>::value == false);
    D_RW_CHECK(internal::has_push_back_helper<rw_array>::value == false);
    D_RW_CHECK(internal::has_push_back_helper<rw_set>::value   == false);
    D_RW_CHECK(internal::has_push_back_helper<rw_map>::value   == false);
    D_RW_CHECK(internal::has_push_back_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_push_back_helper<int>::value      == false);
    D_RW_CHECK(internal::has_push_back_helper<void>::value     == false);

    // value_type is required to build the argument
    D_RW_CHECK(internal::has_push_back_helper<
                   rw_push_back_no_value_type>::value == false);

    return ok;
}

/*
tests_has_emplace_back_helper
  Verifies the emplace-at-the-end probe.
  Tests the following:
  - an emplace_back callable with a value_type is detected
  - the back-emplaceable standard containers carry it
  - std::string does NOT: basic_string offers push_back but no emplace_back,
    so the two append probes are not redundant with one another
  - value_type is required here too
*/
bool
tests_has_emplace_back_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_emplace_back_helper<
                   rw_write_emplace_back>::value == true);
    D_RW_CHECK(internal::has_emplace_back_helper<rw_vector>::value == true);
    D_RW_CHECK(internal::has_emplace_back_helper<rw_deque>::value  == true);
    D_RW_CHECK(internal::has_emplace_back_helper<rw_list>::value   == true);

    // std::string has push_back but not emplace_back
    D_RW_CHECK(   ( internal::has_push_back_helper<rw_string>::value == true )
               && ( internal::has_emplace_back_helper<rw_string>::value
                        == false ));

    // absent
    D_RW_CHECK(internal::has_emplace_back_helper<rw_flist>::value == false);
    D_RW_CHECK(internal::has_emplace_back_helper<rw_array>::value == false);
    D_RW_CHECK(internal::has_emplace_back_helper<rw_set>::value   == false);
    D_RW_CHECK(internal::has_emplace_back_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_emplace_back_helper<int>::value      == false);
    D_RW_CHECK(internal::has_emplace_back_helper<void>::value     == false);

    // value_type is required
    D_RW_CHECK(internal::has_emplace_back_helper<
                   rw_push_back_no_value_type>::value == false);

    return ok;
}

/*
tests_has_push_front_helper
  Verifies the append-at-the-front probe - the one that catches a forward_list,
  which no other append probe reaches.
  Tests the following:
  - a push_front callable with a value_type is detected
  - forward_list, list and deque carry it
  - a vector does NOT, so this probe is not redundant with push_back
  - the associative containers do not
*/
bool
tests_has_push_front_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_push_front_helper<
                   rw_write_push_front>::value == true);
    D_RW_CHECK(internal::has_push_front_helper<rw_flist>::value == true);
    D_RW_CHECK(internal::has_push_front_helper<rw_list>::value  == true);
    D_RW_CHECK(internal::has_push_front_helper<rw_deque>::value == true);

    // a forward_list is reached by this probe and no other append probe
    D_RW_CHECK(   ( internal::has_push_back_helper<rw_flist>::value == false )
               && ( internal::has_emplace_back_helper<rw_flist>::value
                        == false )
               && ( internal::has_push_front_helper<rw_flist>::value == true ));

    // absent
    D_RW_CHECK(internal::has_push_front_helper<rw_vector>::value == false);
    D_RW_CHECK(internal::has_push_front_helper<rw_string>::value == false);
    D_RW_CHECK(internal::has_push_front_helper<rw_array>::value  == false);
    D_RW_CHECK(internal::has_push_front_helper<rw_set>::value    == false);
    D_RW_CHECK(internal::has_push_front_helper<rw_inert>::value  == false);
    D_RW_CHECK(internal::has_push_front_helper<int>::value       == false);
    D_RW_CHECK(internal::has_push_front_helper<void>::value      == false);

    return ok;
}

/*
tests_has_clear_helper
  Verifies the no-argument mutator probe - present on every growable standard
  container, sequence and associative alike, and absent on a fixed array.
  Tests the following:
  - clear() is detected on every growable standard container
  - a std::array does not carry it, being fixed-size
  - the probe calls on a NON-const lvalue, so a const-qualified clear() is
    accepted too - a const member function may be called on a non-const object
  - a private clear() is not detected
  - unlike the three append probes, this one needs no value_type
*/
bool
tests_has_clear_helper()
{
    bool ok = true;

    // present on every growable standard container
    D_RW_CHECK(internal::has_clear_helper<rw_write_clear>::value == true);
    D_RW_CHECK(internal::has_clear_helper<rw_vector>::value == true);
    D_RW_CHECK(internal::has_clear_helper<rw_deque>::value  == true);
    D_RW_CHECK(internal::has_clear_helper<rw_list>::value   == true);
    D_RW_CHECK(internal::has_clear_helper<rw_flist>::value  == true);
    D_RW_CHECK(internal::has_clear_helper<rw_string>::value == true);
    D_RW_CHECK(internal::has_clear_helper<rw_set>::value    == true);
    D_RW_CHECK(internal::has_clear_helper<rw_map>::value    == true);
    D_RW_CHECK(internal::has_clear_helper<rw_umap>::value   == true);

    // absent on a fixed array
    D_RW_CHECK(internal::has_clear_helper<rw_array>::value == false);

    // a const clear() is accepted, being callable on a non-const lvalue
    D_RW_CHECK(internal::has_clear_helper<rw_write_const_clear>::value
                   == true);

    // access control participates
    D_RW_CHECK(internal::has_clear_helper<rw_write_private>::value == false);

    // no value_type needed
    D_RW_CHECK(internal::has_clear_helper<rw_clear_no_value_type>::value
                   == true);

    // absent
    D_RW_CHECK(internal::has_clear_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_clear_helper<rw_pair>::value  == false);
    D_RW_CHECK(internal::has_clear_helper<int>::value      == false);
    D_RW_CHECK(internal::has_clear_helper<void>::value     == false);

    return ok;
}

/*
tests_has_assignable_index_helper
  Verifies the in-place mutation probe - what marks a fixed-size std::array as
  writable, and what a read_only handle must fail.
  Tests the following:
  - a non-const operator[] yielding an assignable lvalue is detected
  - a std::array is detected by this probe ALONE, having no append surface at
    all - so dropping this term would misclassify every fixed array
  - a type whose operator[] yields a CONST reference is not detected, which is
    exactly the read_only handle shape the header names
  - a PROXY return is detected, the probe asking whether the assignment
    expression is well-formed rather than whether a reference came back - the
    std::vector<bool> shape
  - the associative containers are not detected: a set has no operator[], and a
    map's is non-const but takes a key, which a size_t reaches only by
    conversion
*/
bool
tests_has_assignable_index_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_assignable_index_helper<
                   rw_write_assignable_index>::value == true);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_vector>::value
                   == true);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_array>::value
                   == true);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_deque>::value
                   == true);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_string>::value
                   == true);

    // a fixed array is reached by this probe and no other
    D_RW_CHECK(   ( internal::has_push_back_helper<rw_array>::value == false )
               && ( internal::has_emplace_back_helper<rw_array>::value
                        == false )
               && ( internal::has_push_front_helper<rw_array>::value == false )
               && ( internal::has_clear_helper<rw_array>::value == false )
               && ( internal::has_assignable_index_helper<rw_array>::value
                        == true ));

    // a const-yielding operator[] is not assignable
    D_RW_CHECK(internal::has_assignable_index_helper<
                   rw_write_const_index>::value == false);

    // a proxy return is
    D_RW_CHECK(internal::has_assignable_index_helper<
                   rw_write_proxy_index>::value == true);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_bool_vector>::value
                   == true);

    // absent
    D_RW_CHECK(internal::has_assignable_index_helper<rw_list>::value  == false);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_flist>::value == false);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_set>::value   == false);
    D_RW_CHECK(internal::has_assignable_index_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_assignable_index_helper<int>::value      == false);
    D_RW_CHECK(internal::has_assignable_index_helper<void>::value     == false);

    return ok;
}

/*
tests_has_write_surface
  Verifies the disjunction of the five probes.
  Tests the following:
  - each of the five mutators ALONE is enough, on a fixture isolating it
  - a type with none of the five has no write surface
  - the disjunction re-derived term by term, on carriers where different
    subsets hold
  - a type with only NON-CONST observers reads as WRITABLE rather than
    readable: its operator[] yields an assignable lvalue, so the assignable-
    index term fires while every const-instance read probe misses.  Observation
    through a non-const accessor is not observation as this axis defines it
*/
bool
tests_has_write_surface()
{
    bool ok = true;

    // each mutator alone suffices
    D_RW_CHECK(internal::has_write_surface<rw_write_push_back>::value
                   == true);
    D_RW_CHECK(internal::has_write_surface<rw_write_emplace_back>::value
                   == true);
    D_RW_CHECK(internal::has_write_surface<rw_write_push_front>::value
                   == true);
    D_RW_CHECK(internal::has_write_surface<rw_write_clear>::value == true);
    D_RW_CHECK(internal::has_write_surface<rw_write_assignable_index>::value
                   == true);

    // none of the five
    D_RW_CHECK(internal::has_write_surface<rw_inert>::value == false);
    D_RW_CHECK(internal::has_write_surface<rw_observer>::value == false);
    D_RW_CHECK(internal::has_write_surface<rw_metadata_only>::value == false);
    D_RW_CHECK(internal::has_write_surface<rw_write_private>::value == false);
    D_RW_CHECK(internal::has_write_surface<rw_write_const_index>::value
                   == false);
    D_RW_CHECK(internal::has_write_surface<int>::value  == false);
    D_RW_CHECK(internal::has_write_surface<void>::value == false);

    // the disjunction, term by term
    D_RW_CHECK(internal::has_write_surface<rw_array>::value
                   == (    internal::has_push_back_helper<rw_array>::value
                        || internal::has_emplace_back_helper<rw_array>::value
                        || internal::has_push_front_helper<rw_array>::value
                        || internal::has_clear_helper<rw_array>::value
                        || internal::has_assignable_index_helper<
                               rw_array>::value ));
    D_RW_CHECK(internal::has_write_surface<rw_set>::value
                   == (    internal::has_push_back_helper<rw_set>::value
                        || internal::has_emplace_back_helper<rw_set>::value
                        || internal::has_push_front_helper<rw_set>::value
                        || internal::has_clear_helper<rw_set>::value
                        || internal::has_assignable_index_helper<
                               rw_set>::value ));
    D_RW_CHECK(internal::has_write_surface<rw_inert>::value
                   == (    internal::has_push_back_helper<rw_inert>::value
                        || internal::has_emplace_back_helper<rw_inert>::value
                        || internal::has_push_front_helper<rw_inert>::value
                        || internal::has_clear_helper<rw_inert>::value
                        || internal::has_assignable_index_helper<
                               rw_inert>::value ));

    // non-const observers read as WRITABLE, not readable
    D_RW_CHECK(   ( internal::has_read_surface<rw_read_nonconst_only>::value
                        == false )
               && ( internal::has_assignable_index_helper<
                        rw_read_nonconst_only>::value == true )
               && ( internal::has_write_surface<rw_read_nonconst_only>::value
                        == true )
               && ( access_capability_of<rw_read_nonconst_only>::value
                        == access_capability::write_only ));

    return ok;
}

/*
tests_write_surface_value_type_asymmetry
  Verifies an asymmetry among the five write probes that decides whether a
  type is classifiable at all.

  The three append probes and the assignable-index probe each name
  `typename clean_t<T>::value_type` to build their call expression, so a type
  lacking that alias cannot satisfy any of them - however plainly mutable it
  is.  The clear probe names nothing of the sort.  The consequence is that
  `clear()` alone makes a type writable while `push_back(int)` alone does not:

    struct A { void clear(); };            -> write_only
    struct B { void push_back(int); };     -> none

  Neither reading is unreasonable on its own - value_type is a fair
  requirement for a container, and clear() is a fair mutator - but together
  they mean the axis admits any type with a clear() method, container or not,
  while rejecting an obvious appender that omits one alias.

  Tests the following:
  - the two shapes above, and the capabilities they reach
  - the four value_type-dependent probes all miss the appender
  - adding a value_type alias is enough to reclassify it, so the alias and not
    the mutator is what was missing
*/
bool
tests_write_surface_value_type_asymmetry()
{
    bool ok = true;

    // clear() alone: writable
    D_RW_CHECK(internal::has_clear_helper<rw_clear_no_value_type>::value
                   == true);
    D_RW_CHECK(internal::has_write_surface<rw_clear_no_value_type>::value
                   == true);
    D_RW_CHECK(access_capability_of<rw_clear_no_value_type>::value
                   == access_capability::write_only);

    // push_back() alone, no value_type: not writable
    D_RW_CHECK(internal::has_write_surface<
                   rw_push_back_no_value_type>::value == false);
    D_RW_CHECK(access_capability_of<rw_push_back_no_value_type>::value
                   == access_capability::none);

    // every value_type-dependent probe misses it
    D_RW_CHECK(internal::has_push_back_helper<
                   rw_push_back_no_value_type>::value == false);
    D_RW_CHECK(internal::has_emplace_back_helper<
                   rw_push_back_no_value_type>::value == false);
    D_RW_CHECK(internal::has_push_front_helper<
                   rw_push_back_no_value_type>::value == false);
    D_RW_CHECK(internal::has_assignable_index_helper<
                   rw_push_back_no_value_type>::value == false);

    // the alias, not the mutator, is what was missing
    D_RW_CHECK(   ( internal::has_push_back_helper<
                        rw_write_push_back>::value == true )
               && ( internal::has_push_back_helper<
                        rw_push_back_no_value_type>::value == false ));

    // the two shapes disagree though both plainly mutate
    D_RW_CHECK(access_capability_of<rw_clear_no_value_type>::value
                   != access_capability_of<
                          rw_push_back_no_value_type>::value);

    return ok;
}

/*
tests_write_surface_std_types
  Verifies HOW each standard container reaches writability.
  Tests the following:
  - a vector satisfies four of the five, all but push_front
  - a deque satisfies all five
  - a list satisfies the appends and clear but not indexing
  - a forward_list satisfies push_front and clear only
  - a fixed array satisfies the assignable-index term ALONE
  - an associative container satisfies clear() ALONE - it has no push_back and
    no assignable indexing, so dropping the clear term would make every set and
    map read read_only
*/
bool
tests_write_surface_std_types()
{
    bool ok = true;

    // vector: all but push_front
    D_RW_CHECK(   ( internal::has_push_back_helper<rw_vector>::value  == true )
               && ( internal::has_push_front_helper<rw_vector>::value == false )
               && ( internal::has_clear_helper<rw_vector>::value      == true )
               && ( internal::has_assignable_index_helper<rw_vector>::value
                        == true ));

    // deque: all five
    D_RW_CHECK(   ( internal::has_push_back_helper<rw_deque>::value  == true )
               && ( internal::has_push_front_helper<rw_deque>::value == true )
               && ( internal::has_clear_helper<rw_deque>::value      == true )
               && ( internal::has_assignable_index_helper<rw_deque>::value
                        == true ));

    // list: no indexing
    D_RW_CHECK(   ( internal::has_push_back_helper<rw_list>::value == true )
               && ( internal::has_assignable_index_helper<rw_list>::value
                        == false )
               && ( internal::has_write_surface<rw_list>::value == true ));

    // forward_list: push_front and clear only
    D_RW_CHECK(   ( internal::has_push_front_helper<rw_flist>::value == true )
               && ( internal::has_clear_helper<rw_flist>::value      == true )
               && ( internal::has_push_back_helper<rw_flist>::value  == false )
               && ( internal::has_write_surface<rw_flist>::value     == true ));

    // an associative container: clear() alone
    D_RW_CHECK(   ( internal::has_push_back_helper<rw_set>::value    == false )
               && ( internal::has_emplace_back_helper<rw_set>::value == false )
               && ( internal::has_push_front_helper<rw_set>::value   == false )
               && ( internal::has_assignable_index_helper<rw_set>::value
                        == false )
               && ( internal::has_clear_helper<rw_set>::value        == true )
               && ( internal::has_write_surface<rw_set>::value       == true ));
    D_RW_CHECK(internal::has_write_surface<rw_map>::value  == true);
    D_RW_CHECK(internal::has_write_surface<rw_umap>::value == true);

    // a non-container satisfies none
    D_RW_CHECK(internal::has_write_surface<rw_pair>::value == false);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
