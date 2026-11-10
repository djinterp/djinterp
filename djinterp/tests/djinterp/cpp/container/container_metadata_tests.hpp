/*******************************************************************************
* djinterp [testing]                                container_metadata_tests.hpp
*
*   Unit-test suite for cpp/container/container_metadata.hpp: the
* DContainerMetadata key-value store, its is_container_metadata trait and its
* order-insensitive equality, and the DMetadataCarrier mixin that exposes a
* container's metadata through the metadata_traits protocol. Two-faced per
* the DTest guide: the section TUs include it in normal mode for the fixtures
* and the check macro, and the runner with DTEST_SPEC_MODE defined for the
* spec provider.
*
*   HISTORY:
*   The suite was written for an earlier design, a non-owning container
* pointer beside an owned payload, which did not take part in the metadata
* protocol. Its protocol test recorded that gap and said it would fail when
* the module was brought across. The module has since been rewritten as a
* key-value store with a carrier mixin that does take part, so the suite is
* rewritten against it (2026-11-09).
*
*   SECTIONS:
*   - store     (container_metadata_tests_store.cpp): construction, set,
*     find / at / contains, erase and clear, iteration, and a store other
*     than the default vector of pairs.
*   - detection (container_metadata_tests_detection.cpp): the trait, and
*     equality as a comparison of named entries, not of sequences.
*   - carrier   (container_metadata_tests_carrier.cpp): DMetadataCarrier's
*     accessors and aliases, and the metadata_traits protocol seeing them.
*
*
* path:      /tests/djinterp/cpp/container/container_metadata_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    check plumbing        (D_CM_CHECK)
      ----------------------------------

II.   fixtures
      --------

III.  test declarations
      -----------------

IV.   spec provider         (container_metadata_spec)
      -----------------------------------------------
*/

#ifndef DJINTERP_TESTS_CONTAINER_METADATA_TESTS_
#define DJINTERP_TESTS_CONTAINER_METADATA_TESTS_ 1

// std
#include <cstdio>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/container/container_metadata.hpp"     // under test
#include "../../../../inc/djinterp/cpp/util/metadata/metadata_traits.hpp"    // the protocol
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt
//   alias: the framework namespace.  Unconditional - the spec provider needs
// dt::module_spec in spec mode, and the fixtures need nothing from it.
namespace dt = ::djinterp::test;


// ===========================================================================
// I.   check plumbing
// ===========================================================================

// D_CM_CHECK
//   macro: states one claim; on failure names the expression and its
// location and returns false from the enclosing test.  Variadic so a
// template argument list may carry a top-level comma.
#define D_CM_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("    check failed: %s\n    at: %s:%d\n",              \
                        #__VA_ARGS__, __FILE__, __LINE__);                    \
            return false;                                                     \
        }                                                                     \
    } while (0)


// ===========================================================================
// II.  fixtures
// ===========================================================================

#ifndef DTEST_SPEC_MODE

// DCmStrings
//   alias: the default store, string keys to string values.
using DCmStrings = DContainerMetadata<>;

// DCmNumbers
//   alias: other key and value types, to show neither is fixed.
using DCmNumbers = DContainerMetadata<int, double>;

// DCmListed
//   alias: a store other than the default vector of pairs.
using DCmListed = DContainerMetadata<std::string,
                                     int,
                                     std::list<std::pair<std::string, int>>>;

// DCmCarried
//   class: a container that carries a DCmStrings through DMetadataCarrier,
// as the header's usage note shows.
class DCmCarried
    : public DMetadataCarrier<DCmStrings>
{
public:
    DCmCarried() = default;

    explicit DCmCarried(DCmStrings _metadata)
        : DMetadataCarrier<DCmStrings>(static_cast<DCmStrings&&>(_metadata))
    {}
};

// DCmNotCarried
//   struct: a plain type with no metadata surface.
struct DCmNotCarried
{
    int value;
};

#endif  // !DTEST_SPEC_MODE


// ===========================================================================
// III. test declarations
// ===========================================================================

// I.   store     (container_metadata_tests_store.cpp)
bool tests_container_metadata_construction();
bool tests_container_metadata_set();
bool tests_container_metadata_lookup();
bool tests_container_metadata_erase_clear();
bool tests_container_metadata_iteration();
bool tests_container_metadata_custom_store();

// II.  detection (container_metadata_tests_detection.cpp)
bool tests_container_metadata_detection();
bool tests_container_metadata_equality();

// III. carrier   (container_metadata_tests_carrier.cpp)
bool tests_container_metadata_carrier();
bool tests_container_metadata_protocol();


// ===========================================================================
// IV.  spec provider
// ===========================================================================

#ifdef DTEST_SPEC_MODE

// container_metadata_spec
//   function: the declarative description of this suite - one block per
// section TU, one row per test, each carrying the descriptor the report
// renders.
inline
dt::module_spec container_metadata_spec()
{
    return dt::module_spec{
        "container_metadata",
        "An ordered key-value store for a container's metadata over an open "
        "backing store, its detection trait and order-insensitive equality, "
        "and the mixin that carries it through the metadata protocol.",
        {
            dt::block_spec{
                "store",
                "DContainerMetadata as a collection of named entries.",
                {
                    { "tests_container_metadata_construction",
                      "Default construction is empty; an initializer list "
                      "is kept verbatim, duplicates included; the member "
                      "types name the key, value and store.",
                      &tests_container_metadata_construction },
                    { "tests_container_metadata_set",
                      "set appends a new key in first-seen order and "
                      "overwrites an existing one in place.",
                      &tests_container_metadata_set },
                    { "tests_container_metadata_lookup",
                      "find returns the bound value or nullptr, through "
                      "which writes land; at returns it or throws "
                      "std::out_of_range; contains agrees with find.",
                      &tests_container_metadata_lookup },
                    { "tests_container_metadata_erase_clear",
                      "erase removes one key and reports whether it did; "
                      "clear empties the store.",
                      &tests_container_metadata_erase_clear },
                    { "tests_container_metadata_iteration",
                      "Iteration and entries() follow insertion order, "
                      "const and not.",
                      &tests_container_metadata_iteration },
                    { "tests_container_metadata_custom_store",
                      "A std::list of pairs works as the store, and other "
                      "key and value types as the entries.",
                      &tests_container_metadata_custom_store },
                }
            },
            dt::block_spec{
                "detection",
                "is_container_metadata, and equality as a comparison of "
                "named entries.",
                {
                    { "tests_container_metadata_detection",
                      "The trait is true for every specialisation, through "
                      "cv and references, and false for look-alike stores "
                      "and for carriers.",
                      &tests_container_metadata_detection },
                    { "tests_container_metadata_equality",
                      "Equal means the same keys bound to equal values, in "
                      "any order; a size, a value or a key apart makes them "
                      "unequal, and != is its negation.",
                      &tests_container_metadata_equality },
                }
            },
            dt::block_spec{
                "carrier",
                "DMetadataCarrier and the metadata_traits protocol.",
                {
                    { "tests_container_metadata_carrier",
                      "metadata() reaches the one carried object, const and "
                      "not; a carrier constructed with metadata holds it; "
                      "both protocol aliases name it; the mixin cannot be "
                      "made alone.",
                      &tests_container_metadata_carrier },
                    { "tests_container_metadata_protocol",
                      "A carrier scores three of three on the protocol's "
                      "traits and its extractors name the metadata type; "
                      "the store itself and a plain type score none.",
                      &tests_container_metadata_protocol },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONTAINER_METADATA_TESTS_
