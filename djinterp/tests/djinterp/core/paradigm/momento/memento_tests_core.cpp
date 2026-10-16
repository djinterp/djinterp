/*******************************************************************************
* djinterp [core]                                         memento_tests_core.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_core.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_core.cpp
//   The memento core (section V): memento_metadata, the memento snapshot
// wrapper, and the CRTP memento_originator. The originator tests cover state
// capture/restore and pre-restore sequence monotonicity; they intentionally do
// not pin the sequence value after a restore, since a whole-object snapshot
// policy round-trips the counter through the snapshotted state (see the suite
// header's note and the accompanying report).

// std
#include <cstddef>
#include <string>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // doc_state : a minimal originator using the deep-copy policy.
    struct doc_state
        : memento_originator<doc_state, deep_copy_snapshot>
    {
        int value;

        doc_state() : value(0) {}
    };
}


/*
tests_memento_metadata_default
  Verifies the default-constructed metadata.
  Tests the following:
  - the default sequence number is zero
*/
bool
tests_memento_metadata_default()
{
    memento_metadata meta;

    return (meta.sequence == 0);
}

/*
tests_memento_metadata_sequence
  Verifies the explicit-sequence metadata constructor.
  Tests the following:
  - the supplied sequence number is stored
*/
bool
tests_memento_metadata_sequence()
{
    memento_metadata meta(11);

    return (meta.sequence == 11);
}

/*
tests_memento_metadata_tag
  Verifies the string_view-tagged metadata constructor.
  Tests the following:
  - the sequence and tag are both stored (C++17+)
*/
bool
tests_memento_metadata_tag()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    memento_metadata meta(4, std::string_view("checkpoint"));

    bool ok = true;

    ok = ok && (meta.sequence == 4);
    ok = ok && (meta.tag == std::string_view("checkpoint"));

    return ok;
#else
    return true;
#endif
}

/*
tests_memento_construction
  Verifies the memento wrapper holds state and metadata.
  Tests the following:
  - the stored state matches the constructor argument
  - the stored metadata sequence matches the constructor argument
*/
bool
tests_memento_construction()
{
    memento<int> m(42, memento_metadata(3));

    bool ok = true;

    ok = ok && (m.state == 42);
    ok = ok && (m.meta.sequence == 3);

    return ok;
}

/*
tests_memento_default
  Verifies the default-constructed memento.
  Tests the following:
  - the snapshot is value-initialized and the metadata sequence is zero
*/
bool
tests_memento_default()
{
    memento<int> m;

    bool ok = true;

    ok = ok && (m.state == 0);
    ok = ok && (m.meta.sequence == 0);

    return ok;
}

/*
tests_originator_create_restore
  Verifies the CRTP originator captures and restores its state.
  Tests the following:
  - create_memento captures the current state
  - restore_memento returns the originator to the captured state after mutation
*/
bool
tests_originator_create_restore()
{
    doc_state d;
    d.value = 3;

    doc_state::memento_type snap = d.create_memento();

    d.value = 99;
    d.restore_memento(snap);

    return (d.value == 3);
}

/*
tests_originator_sequence_monotonic
  Verifies the originator's sequence counter increments per capture.
  Tests the following:
  - current_sequence starts at zero
  - each create_memento stamps the next sequence number into the metadata
  - current_sequence advances by one per capture (measured without a restore,
    which - under a whole-object snapshot policy - would rewind the counter)
*/
bool
tests_originator_sequence_monotonic()
{
    doc_state d;

    bool ok = (d.current_sequence() == 0);

    doc_state::memento_type m0 = d.create_memento();
    doc_state::memento_type m1 = d.create_memento();
    doc_state::memento_type m2 = d.create_memento();

    ok = ok && (m0.meta.sequence == 0);
    ok = ok && (m1.meta.sequence == 1);
    ok = ok && (m2.meta.sequence == 2);
    ok = ok && (d.current_sequence() == 3);

    return ok;
}

/*
tests_originator_tagged
  Verifies the tagged create_memento overload.
  Tests the following:
  - a tagged capture stores both the sequence and the tag (C++17+)
*/
bool
tests_originator_tagged()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    doc_state d;
    d.value = 5;

    doc_state::memento_type m = d.create_memento(std::string_view("save-A"));

    bool ok = true;

    ok = ok && (m.meta.sequence == 0);
    ok = ok && (m.meta.tag == std::string_view("save-A"));
    ok = ok && (m.state.value == 5);

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
