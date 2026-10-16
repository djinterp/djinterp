/*******************************************************************************
* djinterp [core]             read_write_container_traits_tests_read_surface.cpp
*
*
* path:      /tests/djinterp/core/container/traits/read_write_container_traits_tests_read_surface.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I-a. The read surface
// ===========================================================================
//   Four probes, joined by a disjunction.  Each asks whether some VALUE
// observer is reachable on a CONST instance - const-ness being what makes the
// probe about observation rather than about the member merely existing.


/*
tests_has_const_index_helper
  Verifies the readable-random-access probe.
  Tests the following:
  - a const operator[] taking a size_t is detected
  - the standard indexable containers carry it
  - an operator[] that is not const-qualified is NOT detected, since the probe
    supplies a const instance
  - an operator[] taking something a size_t will not convert to is not
    detected, since the probe supplies a size_t
  - the associative containers are not detected: std::map's operator[] is
    non-const by design, being an inserting accessor
*/
bool
tests_has_const_index_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_index_helper<rw_read_index>::value == true);
    D_RW_CHECK(internal::has_const_index_helper<rw_vector>::value  == true);
    D_RW_CHECK(internal::has_const_index_helper<rw_array>::value   == true);
    D_RW_CHECK(internal::has_const_index_helper<rw_deque>::value   == true);
    D_RW_CHECK(internal::has_const_index_helper<rw_string>::value  == true);
    D_RW_CHECK(internal::has_const_index_helper<rw_bool_vector>::value
                   == true);

    // const-ness is required
    D_RW_CHECK(internal::has_const_index_helper<rw_read_nonconst_only>::value
                   == false);

    // the argument type is required
    D_RW_CHECK(internal::has_const_index_helper<
                   rw_read_wrong_index_arg>::value == false);

    // absent
    D_RW_CHECK(internal::has_const_index_helper<rw_list>::value  == false);
    D_RW_CHECK(internal::has_const_index_helper<rw_flist>::value == false);
    D_RW_CHECK(internal::has_const_index_helper<rw_set>::value   == false);
    D_RW_CHECK(internal::has_const_index_helper<rw_map>::value   == false);
    D_RW_CHECK(internal::has_const_index_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_const_index_helper<int>::value      == false);
    D_RW_CHECK(internal::has_const_index_helper<void>::value     == false);

    return ok;
}

/*
tests_has_const_front_helper
  Verifies the readable-first-element probe.
  Tests the following:
  - a const front() is detected
  - the sequence containers carry it, including forward_list, which has front()
    but no indexing
  - the associative containers do not: a set has no front()
  - a non-const front() is not detected
*/
bool
tests_has_const_front_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_front_helper<rw_read_front>::value == true);
    D_RW_CHECK(internal::has_const_front_helper<rw_vector>::value == true);
    D_RW_CHECK(internal::has_const_front_helper<rw_list>::value   == true);
    D_RW_CHECK(internal::has_const_front_helper<rw_flist>::value  == true);
    D_RW_CHECK(internal::has_const_front_helper<rw_deque>::value  == true);
    D_RW_CHECK(internal::has_const_front_helper<rw_string>::value == true);

    // const-ness is required
    D_RW_CHECK(internal::has_const_front_helper<
                   rw_read_nonconst_only>::value == false);

    // absent
    D_RW_CHECK(internal::has_const_front_helper<rw_set>::value == false);
    D_RW_CHECK(internal::has_const_front_helper<rw_map>::value == false);
    D_RW_CHECK(internal::has_const_front_helper<rw_read_index>::value
                   == false);
    D_RW_CHECK(internal::has_const_front_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_const_front_helper<int>::value      == false);
    D_RW_CHECK(internal::has_const_front_helper<void>::value     == false);

    return ok;
}

/*
tests_has_const_data_helper
  Verifies the readable-contiguous-block probe.
  Tests the following:
  - a const data() is detected
  - the contiguous containers carry it
  - std::vector<bool> does NOT: the bit-packed specialisation offers no data()
  - the node-based and associative containers do not
*/
bool
tests_has_const_data_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_data_helper<rw_read_data>::value == true);
    D_RW_CHECK(internal::has_const_data_helper<rw_vector>::value == true);
    D_RW_CHECK(internal::has_const_data_helper<rw_array>::value  == true);
    D_RW_CHECK(internal::has_const_data_helper<rw_string>::value == true);

    // the bit-packed vector has none
    D_RW_CHECK(internal::has_const_data_helper<rw_bool_vector>::value
                   == false);

    // absent
    D_RW_CHECK(internal::has_const_data_helper<rw_deque>::value == false);
    D_RW_CHECK(internal::has_const_data_helper<rw_list>::value  == false);
    D_RW_CHECK(internal::has_const_data_helper<rw_flist>::value == false);
    D_RW_CHECK(internal::has_const_data_helper<rw_set>::value   == false);
    D_RW_CHECK(internal::has_const_data_helper<rw_map>::value   == false);
    D_RW_CHECK(internal::has_const_data_helper<
                   rw_read_nonconst_only>::value == false);
    D_RW_CHECK(internal::has_const_data_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_const_data_helper<int>::value      == false);
    D_RW_CHECK(internal::has_const_data_helper<void>::value     == false);

    return ok;
}

/*
tests_has_const_begin_helper
  Verifies the readable-iteration probe - the one that lets an associative
  container, which has neither indexing nor front() nor data(), still count as
  observable.
  Tests the following:
  - a const begin() is detected
  - EVERY standard container carries it, which is what makes it the universal
    fallback observer
  - a begin() returning VOID still counts: the probe names the call
    expression's type and never constrains it, so this is a structural probe
    and not a semantic one
  - a non-const or private begin() is not detected
*/
bool
tests_has_const_begin_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_begin_helper<rw_read_begin>::value == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_vector>::value == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_array>::value  == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_deque>::value  == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_list>::value   == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_flist>::value  == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_string>::value == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_set>::value    == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_map>::value    == true);
    D_RW_CHECK(internal::has_const_begin_helper<rw_umap>::value   == true);

    // a void return still counts
    D_RW_CHECK(internal::has_const_begin_helper<rw_read_void_begin>::value
                   == true);

    // const-ness and access control are required
    D_RW_CHECK(internal::has_const_begin_helper<
                   rw_read_nonconst_only>::value == false);
    D_RW_CHECK(internal::has_const_begin_helper<rw_read_private>::value
                   == false);

    // absent
    D_RW_CHECK(internal::has_const_begin_helper<rw_read_index>::value
                   == false);
    D_RW_CHECK(internal::has_const_begin_helper<rw_inert>::value == false);
    D_RW_CHECK(internal::has_const_begin_helper<rw_pair>::value  == false);
    D_RW_CHECK(internal::has_const_begin_helper<int>::value      == false);
    D_RW_CHECK(internal::has_const_begin_helper<void>::value     == false);

    return ok;
}

/*
tests_has_read_surface
  Verifies the disjunction of the four probes.
  Tests the following:
  - each of the four observers ALONE is enough, on a fixture isolating it
  - a type with none of the four has no read surface
  - the disjunction re-derived term by term, on carriers where different
    subsets of the four hold - so a term cannot be dropped without a carrier
    noticing
  - it is a disjunction and not a conjunction, shown by a carrier satisfying
    exactly one term
*/
bool
tests_has_read_surface()
{
    bool ok = true;

    // each observer alone suffices
    D_RW_CHECK(internal::has_read_surface<rw_read_index>::value == true);
    D_RW_CHECK(internal::has_read_surface<rw_read_front>::value == true);
    D_RW_CHECK(internal::has_read_surface<rw_read_data>::value  == true);
    D_RW_CHECK(internal::has_read_surface<rw_read_begin>::value == true);

    // none of the four
    D_RW_CHECK(internal::has_read_surface<rw_inert>::value == false);
    D_RW_CHECK(internal::has_read_surface<rw_metadata_only>::value == false);
    D_RW_CHECK(internal::has_read_surface<rw_read_nonconst_only>::value
                   == false);
    D_RW_CHECK(internal::has_read_surface<rw_read_private>::value  == false);
    D_RW_CHECK(internal::has_read_surface<int>::value  == false);
    D_RW_CHECK(internal::has_read_surface<void>::value == false);

    // the disjunction, term by term
    D_RW_CHECK(internal::has_read_surface<rw_vector>::value
                   == (    internal::has_const_index_helper<rw_vector>::value
                        || internal::has_const_front_helper<rw_vector>::value
                        || internal::has_const_data_helper<rw_vector>::value
                        || internal::has_const_begin_helper<
                               rw_vector>::value ));
    D_RW_CHECK(internal::has_read_surface<rw_set>::value
                   == (    internal::has_const_index_helper<rw_set>::value
                        || internal::has_const_front_helper<rw_set>::value
                        || internal::has_const_data_helper<rw_set>::value
                        || internal::has_const_begin_helper<rw_set>::value ));
    D_RW_CHECK(internal::has_read_surface<rw_inert>::value
                   == (    internal::has_const_index_helper<rw_inert>::value
                        || internal::has_const_front_helper<rw_inert>::value
                        || internal::has_const_data_helper<rw_inert>::value
                        || internal::has_const_begin_helper<
                               rw_inert>::value ));

    // a disjunction, not a conjunction: one term is enough
    D_RW_CHECK(   ( internal::has_const_index_helper<rw_read_index>::value
                        == true )
               && ( internal::has_const_front_helper<rw_read_index>::value
                        == false )
               && ( internal::has_const_data_helper<rw_read_index>::value
                        == false )
               && ( internal::has_const_begin_helper<rw_read_index>::value
                        == false )
               && ( internal::has_read_surface<rw_read_index>::value
                        == true ));

    return ok;
}

/*
tests_read_surface_excludes_metadata
  Verifies the design decision the whole write_only capability rests on.

  size() and empty() reveal no ELEMENT VALUE, so neither is a read surface.
  Without that exclusion an append-only sink - which necessarily reports how
  much it has taken - would read as read_write, and the write_only corner of
  the lattice would be unreachable for any realistic type.

  Tests the following:
  - a type exposing only size() and empty() has no read surface, and having no
    mutator either it is not access-classifiable at all
  - none of the four observer probes is satisfied by metadata
  - a sink that adds an append surface to that metadata reads write_only, not
    read_write - the capability the exclusion exists to make reachable
  - the standard containers all expose size() AND a value observer, so the
    exclusion is invisible on them; it only shows on a type that has the one
    without the other
*/
bool
tests_read_surface_excludes_metadata()
{
    bool ok = true;

    // metadata alone is not observation
    D_RW_CHECK(internal::has_read_surface<rw_metadata_only>::value == false);
    D_RW_CHECK(access_capability_of<rw_metadata_only>::value
                   == access_capability::none);

    // no probe is satisfied by it
    D_RW_CHECK(internal::has_const_index_helper<rw_metadata_only>::value
                   == false);
    D_RW_CHECK(internal::has_const_front_helper<rw_metadata_only>::value
                   == false);
    D_RW_CHECK(internal::has_const_data_helper<rw_metadata_only>::value
                   == false);
    D_RW_CHECK(internal::has_const_begin_helper<rw_metadata_only>::value
                   == false);

    // ... which is what makes a sink write_only rather than read_write
    D_RW_CHECK(internal::has_read_surface<rw_sink>::value  == false);
    D_RW_CHECK(internal::has_write_surface<rw_sink>::value == true);
    D_RW_CHECK(access_capability_of<rw_sink>::value
                   == access_capability::write_only);
    D_RW_CHECK(access_capability_of<rw_sink>::value
                   != access_capability::read_write);

    // and the exclusion is invisible on a standard container, which has both
    D_RW_CHECK(   ( internal::has_read_surface<rw_vector>::value == true )
               && ( internal::has_const_begin_helper<rw_vector>::value
                        == true ));

    return ok;
}

/*
tests_read_surface_std_types
  Verifies HOW each standard container reaches observability, which differs by
  container even though the verdict does not.
  Tests the following:
  - a contiguous container satisfies all four probes
  - a deque satisfies three, having no data()
  - a list satisfies front() and begin() only, having no indexing
  - a forward_list likewise
  - an associative container satisfies begin() ALONE - the case the fourth
    probe exists for, and the one that would break if it were dropped
  - std::vector<bool> misses data() but is observable regardless
*/
bool
tests_read_surface_std_types()
{
    bool ok = true;

    // contiguous: all four
    D_RW_CHECK(   ( internal::has_const_index_helper<rw_vector>::value == true )
               && ( internal::has_const_front_helper<rw_vector>::value == true )
               && ( internal::has_const_data_helper<rw_vector>::value  == true )
               && ( internal::has_const_begin_helper<rw_vector>::value
                        == true ));
    D_RW_CHECK(internal::has_read_surface<rw_array>::value  == true);
    D_RW_CHECK(internal::has_read_surface<rw_string>::value == true);

    // deque: no data()
    D_RW_CHECK(   ( internal::has_const_index_helper<rw_deque>::value == true )
               && ( internal::has_const_data_helper<rw_deque>::value == false )
               && ( internal::has_read_surface<rw_deque>::value      == true ));

    // list and forward_list: no indexing
    D_RW_CHECK(   ( internal::has_const_index_helper<rw_list>::value == false )
               && ( internal::has_const_front_helper<rw_list>::value == true )
               && ( internal::has_read_surface<rw_list>::value       == true ));
    D_RW_CHECK(   ( internal::has_const_index_helper<rw_flist>::value == false )
               && ( internal::has_const_front_helper<rw_flist>::value == true )
               && ( internal::has_read_surface<rw_flist>::value      == true ));

    // associative: begin() alone
    D_RW_CHECK(   ( internal::has_const_index_helper<rw_set>::value == false )
               && ( internal::has_const_front_helper<rw_set>::value == false )
               && ( internal::has_const_data_helper<rw_set>::value  == false )
               && ( internal::has_const_begin_helper<rw_set>::value == true )
               && ( internal::has_read_surface<rw_set>::value       == true ));
    D_RW_CHECK(   ( internal::has_const_index_helper<rw_map>::value == false )
               && ( internal::has_const_begin_helper<rw_map>::value == true )
               && ( internal::has_read_surface<rw_map>::value       == true ));
    D_RW_CHECK(internal::has_read_surface<rw_umap>::value == true);

    // the bit-packed vector: no data(), observable anyway
    D_RW_CHECK(   ( internal::has_const_data_helper<rw_bool_vector>::value
                        == false )
               && ( internal::has_read_surface<rw_bool_vector>::value
                        == true ));

    // a non-container satisfies none
    D_RW_CHECK(internal::has_read_surface<rw_pair>::value == false);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
