/*******************************************************************************
* djinterp [cpp]              read_write_container_traits_tests_read_surface.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/read_write_container_traits_tests_read_surface.cpp
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
bool tests_has_const_index_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_index_helper<DRwReadIndex>::value == true);
    D_RW_CHECK(internal::has_const_index_helper<DRwVector>::value  == true);
    D_RW_CHECK(internal::has_const_index_helper<DRwArray>::value   == true);
    D_RW_CHECK(internal::has_const_index_helper<DRwDeque>::value   == true);
    D_RW_CHECK(internal::has_const_index_helper<DRwString>::value  == true);
    D_RW_CHECK(internal::has_const_index_helper<DRwBoolVector>::value
                   == true);

    // const-ness is required
    D_RW_CHECK(internal::has_const_index_helper<DRwReadNonconstOnly>::value
                   == false);

    // the argument type is required
    D_RW_CHECK(internal::has_const_index_helper<
                   DRwReadWrongIndexArg>::value == false);

    // absent
    D_RW_CHECK(internal::has_const_index_helper<DRwList>::value  == false);
    D_RW_CHECK(internal::has_const_index_helper<DRwFlist>::value == false);
    D_RW_CHECK(internal::has_const_index_helper<DRwSet>::value   == false);
    D_RW_CHECK(internal::has_const_index_helper<DRwMap>::value   == false);
    D_RW_CHECK(internal::has_const_index_helper<DRwInert>::value == false);
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
bool tests_has_const_front_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_front_helper<DRwReadFront>::value == true);
    D_RW_CHECK(internal::has_const_front_helper<DRwVector>::value == true);
    D_RW_CHECK(internal::has_const_front_helper<DRwList>::value   == true);
    D_RW_CHECK(internal::has_const_front_helper<DRwFlist>::value  == true);
    D_RW_CHECK(internal::has_const_front_helper<DRwDeque>::value  == true);
    D_RW_CHECK(internal::has_const_front_helper<DRwString>::value == true);

    // const-ness is required
    D_RW_CHECK(internal::has_const_front_helper<
                   DRwReadNonconstOnly>::value == false);

    // absent
    D_RW_CHECK(internal::has_const_front_helper<DRwSet>::value == false);
    D_RW_CHECK(internal::has_const_front_helper<DRwMap>::value == false);
    D_RW_CHECK(internal::has_const_front_helper<DRwReadIndex>::value
                   == false);
    D_RW_CHECK(internal::has_const_front_helper<DRwInert>::value == false);
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
bool tests_has_const_data_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_data_helper<DRwReadData>::value == true);
    D_RW_CHECK(internal::has_const_data_helper<DRwVector>::value == true);
    D_RW_CHECK(internal::has_const_data_helper<DRwArray>::value  == true);
    D_RW_CHECK(internal::has_const_data_helper<DRwString>::value == true);

    // the bit-packed vector has none
    D_RW_CHECK(internal::has_const_data_helper<DRwBoolVector>::value
                   == false);

    // absent
    D_RW_CHECK(internal::has_const_data_helper<DRwDeque>::value == false);
    D_RW_CHECK(internal::has_const_data_helper<DRwList>::value  == false);
    D_RW_CHECK(internal::has_const_data_helper<DRwFlist>::value == false);
    D_RW_CHECK(internal::has_const_data_helper<DRwSet>::value   == false);
    D_RW_CHECK(internal::has_const_data_helper<DRwMap>::value   == false);
    D_RW_CHECK(internal::has_const_data_helper<
                   DRwReadNonconstOnly>::value == false);
    D_RW_CHECK(internal::has_const_data_helper<DRwInert>::value == false);
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
bool tests_has_const_begin_helper()
{
    bool ok = true;

    // present
    D_RW_CHECK(internal::has_const_begin_helper<DRwReadBegin>::value == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwVector>::value == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwArray>::value  == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwDeque>::value  == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwList>::value   == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwFlist>::value  == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwString>::value == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwSet>::value    == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwMap>::value    == true);
    D_RW_CHECK(internal::has_const_begin_helper<DRwUmap>::value   == true);

    // a void return still counts
    D_RW_CHECK(internal::has_const_begin_helper<DRwReadVoidBegin>::value
                   == true);

    // const-ness and access control are required
    D_RW_CHECK(internal::has_const_begin_helper<
                   DRwReadNonconstOnly>::value == false);
    D_RW_CHECK(internal::has_const_begin_helper<DRwReadPrivate>::value
                   == false);

    // absent
    D_RW_CHECK(internal::has_const_begin_helper<DRwReadIndex>::value
                   == false);
    D_RW_CHECK(internal::has_const_begin_helper<DRwInert>::value == false);
    D_RW_CHECK(internal::has_const_begin_helper<DRwPair>::value  == false);
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
bool tests_has_read_surface()
{
    bool ok = true;

    // each observer alone suffices
    D_RW_CHECK(internal::has_read_surface<DRwReadIndex>::value == true);
    D_RW_CHECK(internal::has_read_surface<DRwReadFront>::value == true);
    D_RW_CHECK(internal::has_read_surface<DRwReadData>::value  == true);
    D_RW_CHECK(internal::has_read_surface<DRwReadBegin>::value == true);

    // none of the four
    D_RW_CHECK(internal::has_read_surface<DRwInert>::value == false);
    D_RW_CHECK(internal::has_read_surface<DRwMetadataOnly>::value == false);
    D_RW_CHECK(internal::has_read_surface<DRwReadNonconstOnly>::value
                   == false);
    D_RW_CHECK(internal::has_read_surface<DRwReadPrivate>::value  == false);
    D_RW_CHECK(internal::has_read_surface<int>::value  == false);
    D_RW_CHECK(internal::has_read_surface<void>::value == false);

    // the disjunction, term by term
    D_RW_CHECK(internal::has_read_surface<DRwVector>::value
                   == (    internal::has_const_index_helper<DRwVector>::value
                        || internal::has_const_front_helper<DRwVector>::value
                        || internal::has_const_data_helper<DRwVector>::value
                        || internal::has_const_begin_helper<
                               DRwVector>::value ));
    D_RW_CHECK(internal::has_read_surface<DRwSet>::value
                   == (    internal::has_const_index_helper<DRwSet>::value
                        || internal::has_const_front_helper<DRwSet>::value
                        || internal::has_const_data_helper<DRwSet>::value
                        || internal::has_const_begin_helper<DRwSet>::value ));
    D_RW_CHECK(internal::has_read_surface<DRwInert>::value
                   == (    internal::has_const_index_helper<DRwInert>::value
                        || internal::has_const_front_helper<DRwInert>::value
                        || internal::has_const_data_helper<DRwInert>::value
                        || internal::has_const_begin_helper<
                               DRwInert>::value ));

    // a disjunction, not a conjunction: one term is enough
    D_RW_CHECK(   ( internal::has_const_index_helper<DRwReadIndex>::value
                        == true )
               && ( internal::has_const_front_helper<DRwReadIndex>::value
                        == false )
               && ( internal::has_const_data_helper<DRwReadIndex>::value
                        == false )
               && ( internal::has_const_begin_helper<DRwReadIndex>::value
                        == false )
               && ( internal::has_read_surface<DRwReadIndex>::value
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
bool tests_read_surface_excludes_metadata()
{
    bool ok = true;

    // metadata alone is not observation
    D_RW_CHECK(internal::has_read_surface<DRwMetadataOnly>::value == false);
    D_RW_CHECK(access_capability_of<DRwMetadataOnly>::value
                   == access_capability::none);

    // no probe is satisfied by it
    D_RW_CHECK(internal::has_const_index_helper<DRwMetadataOnly>::value
                   == false);
    D_RW_CHECK(internal::has_const_front_helper<DRwMetadataOnly>::value
                   == false);
    D_RW_CHECK(internal::has_const_data_helper<DRwMetadataOnly>::value
                   == false);
    D_RW_CHECK(internal::has_const_begin_helper<DRwMetadataOnly>::value
                   == false);

    // ... which is what makes a sink write_only rather than read_write
    D_RW_CHECK(internal::has_read_surface<DRwSink>::value  == false);
    D_RW_CHECK(internal::has_write_surface<DRwSink>::value == true);
    D_RW_CHECK(access_capability_of<DRwSink>::value
                   == access_capability::write_only);
    D_RW_CHECK(access_capability_of<DRwSink>::value
                   != access_capability::read_write);

    // and the exclusion is invisible on a standard container, which has both
    D_RW_CHECK(   ( internal::has_read_surface<DRwVector>::value == true )
               && ( internal::has_const_begin_helper<DRwVector>::value
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
bool tests_read_surface_std_types()
{
    bool ok = true;

    // contiguous: all four
    D_RW_CHECK(   ( internal::has_const_index_helper<DRwVector>::value == true )
               && ( internal::has_const_front_helper<DRwVector>::value == true )
               && ( internal::has_const_data_helper<DRwVector>::value  == true )
               && ( internal::has_const_begin_helper<DRwVector>::value
                        == true ));
    D_RW_CHECK(internal::has_read_surface<DRwArray>::value  == true);
    D_RW_CHECK(internal::has_read_surface<DRwString>::value == true);

    // deque: no data()
    D_RW_CHECK(   ( internal::has_const_index_helper<DRwDeque>::value == true )
               && ( internal::has_const_data_helper<DRwDeque>::value == false )
               && ( internal::has_read_surface<DRwDeque>::value      == true ));

    // list and forward_list: no indexing
    D_RW_CHECK(   ( internal::has_const_index_helper<DRwList>::value == false )
               && ( internal::has_const_front_helper<DRwList>::value == true )
               && ( internal::has_read_surface<DRwList>::value       == true ));
    D_RW_CHECK(   ( internal::has_const_index_helper<DRwFlist>::value == false )
               && ( internal::has_const_front_helper<DRwFlist>::value == true )
               && ( internal::has_read_surface<DRwFlist>::value      == true ));

    // associative: begin() alone
    D_RW_CHECK(   ( internal::has_const_index_helper<DRwSet>::value == false )
               && ( internal::has_const_front_helper<DRwSet>::value == false )
               && ( internal::has_const_data_helper<DRwSet>::value  == false )
               && ( internal::has_const_begin_helper<DRwSet>::value == true )
               && ( internal::has_read_surface<DRwSet>::value       == true ));
    D_RW_CHECK(   ( internal::has_const_index_helper<DRwMap>::value == false )
               && ( internal::has_const_begin_helper<DRwMap>::value == true )
               && ( internal::has_read_surface<DRwMap>::value       == true ));
    D_RW_CHECK(internal::has_read_surface<DRwUmap>::value == true);

    // the bit-packed vector: no data(), observable anyway
    D_RW_CHECK(   ( internal::has_const_data_helper<DRwBoolVector>::value
                        == false )
               && ( internal::has_read_surface<DRwBoolVector>::value
                        == true ));

    // a non-container satisfies none
    D_RW_CHECK(internal::has_read_surface<DRwPair>::value == false);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
