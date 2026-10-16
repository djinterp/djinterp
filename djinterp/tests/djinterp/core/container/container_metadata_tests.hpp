/*******************************************************************************
* djinterp [testing]                                container_metadata_tests.hpp
*
*   Unit-test suite for container_metadata (core/container/
* container_metadata.hpp).  Two-faced per the DTest guide: the section TUs
* include it in normal mode for the fixtures and the check macro, and the
* runner with DTEST_SPEC_MODE defined for the spec provider.
*
*   BUILD PREREQUISITE:
*   container_metadata.hpp does not compile as shipped.  Line 73 reads
* `#include "./metadata.hpp"`, and that header no longer exists - it was
* superseded by metadata_traits.hpp, whose own preamble records the
* replacement.  Nothing in container_metadata's BODY refers to anything the
* deleted header declared, so removing the include is sufficient; the file
* then builds clean.  Its documentation is a separate matter, and still
* describes the superseded world at length: the three primitives
* metadata_value / metadata_entry / metadata_collection, and an `is_metadata`
* trait it advises callers to static_assert on.  None of those exist any
* more.
*
*   THE STANDING ARCHITECTURAL GAP:
*   container_metadata does not participate in the metadata protocol that
* replaced metadata.hpp.  It exposes container_type, payload_type and
* payload(); the protocol looks for metadata_container_type, metadata_type
* and metadata.  So a container_metadata scores zero of three on
* has_metadata / has_metadata_type / has_metadata_container_type, while a
* type written to the new names scores three of three.  The factory block
* records that as an executable assertion rather than a comment, so that
* whenever this module is brought onto the protocol the test will fail and
* say so.
*
*   WHAT THE TYPE ACTUALLY IS:
*   A non-owning `_Container*` beside an owned `_Payload`.  Two consequences
* drive most of the suite:
*
*   First, because the container is a borrowed pointer, equality compares
* PAYLOADS ONLY.  Two objects bound to different containers with equal
* payloads are equal, and an unbound object equals a bound one whose payload
* matches.  That is a deliberate choice and not an oversight - a non-owning
* reference has no value identity to compare - but it is surprising enough to
* be worth stating in its own tests.
*
*   Second, the class holds a raw pointer with no lifetime tie to the
* container.  The tests exercise binding and rebinding but never outlive a
* container, since the module offers no protection there to test.
*
*   A TRAP WORTH KNOWING ABOUT:
*   std::is_default_constructible reports TRUE for a specialisation whose
* payload has no default constructor, and the default construction then fails
* to compile.  The trait sees only the declaration; the failure is in the
* mem-initialiser of the definition, which is not part of the immediate
* context and so is not SFINAE-friendly.  The container-only constructor has
* the same shape.  cm_no_default is the fixture, and the construction block's
* hazards test states the trait's answer against what actually happens.
*
*   NOT TESTED:
*   The module advertises C++11 support, but reaching it through
* make_container_metadata instantiates clean_t, which djinterp.hpp spells
* without `typename` - C++20-only syntax.  The header does not build under
* c++11, c++14 or c++17, so the claim cannot be tested from inside the
* project.
*
*
* path:      /tests/djinterp/core/container/container_metadata_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    check plumbing        (D_CM_CHECK)
      ----------------------------------

II.   container / payload fixtures
      ----------------------------

III.  detection near-misses
      ---------------------

IV.   test declarations
      -----------------

V.    spec provider         (container_metadata_spec)
      -----------------------------------------------
*/

#ifndef DJINTERP_TESTS_CONTAINER_METADATA_TESTS_
#define DJINTERP_TESTS_CONTAINER_METADATA_TESTS_ 1

// std
#include <cstdio>
#include <type_traits>
#include <utility>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/container/container_metadata.hpp"           // the module under test
#include "../../../../inc/djinterp/core/util/metadata/metadata_traits.hpp"              // for the composition test
#include "djinterp/test/test_traits.hpp"    // hostile type zoo, quantifiers
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"  // module_spec + run_module
#endif


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


#ifndef DTEST_SPEC_MODE

// ===========================================================================
// II.  container / payload fixtures
// ===========================================================================

// cm_box
//   fixture: stands in for a container.  Carries mutable state so that a
// write made through get_container() can be observed on the ORIGINAL object,
// which is what "non-owning" has to mean if it means anything.
struct cm_box
{
    int n = 0;
};

// cm_note
//   fixture: an ordinary equality-comparable payload.
struct cm_note
{
    int v = 0;

    bool operator==(const cm_note& _other) const
    {
        return v == _other.v;
    }

    bool operator!=(const cm_note& _other) const
    {
        return !(*this == _other);
    }
};

// cm_counted
//   fixture: a payload that counts its own copies and moves, so the
// forwarding claims made by the factory and by set_payload can be measured
// rather than assumed.  Neither test_object nor test_counter in the shared
// fixtures counts these, so it is built here.
struct cm_counted
{
    inline static int copies = 0;
    inline static int moves  = 0;

    int v = 0;

    cm_counted() = default;

    explicit cm_counted(int _v)
        : v(_v)
    {}

    cm_counted(const cm_counted& _other)
        : v(_other.v)
    {
        ++copies;
    }

    cm_counted(cm_counted&& _other) noexcept
        : v(_other.v)
    {
        ++moves;
    }

    cm_counted& operator=(const cm_counted& _other)
    {
        v = _other.v;
        ++copies;
        return *this;
    }

    cm_counted& operator=(cm_counted&& _other) noexcept
    {
        v = _other.v;
        ++moves;
        return *this;
    }

    ~cm_counted() = default;

    bool operator==(const cm_counted& _other) const
    {
        return v == _other.v;
    }

    // reset
    //   function: zeroes the counters before a measurement.
    static void reset()
    {
        copies = 0;
        moves  = 0;
    }
};

// cm_no_default
//   fixture: a payload with NO default constructor, used to state the
// is_default_constructible trap described in the header note.
struct cm_no_default
{
    int v;

    explicit cm_no_default(int _v)
        : v(_v)
    {}

    bool operator==(const cm_no_default& _other) const
    {
        return v == _other.v;
    }
};

// cm_throwing
//   fixture: a payload whose move constructor is NOT noexcept, so the
// conditional noexcept on the module's move-taking constructors has
// something to report false for.
struct cm_throwing
{
    int v = 0;

    cm_throwing() = default;
    cm_throwing(const cm_throwing&) = default;

    cm_throwing(cm_throwing&& _other)
        : v(_other.v)
    {}

    cm_throwing& operator=(const cm_throwing&) = default;
    cm_throwing& operator=(cm_throwing&&) = default;
    ~cm_throwing() = default;

    bool operator==(const cm_throwing& _other) const
    {
        return v == _other.v;
    }
};

// cm_meta
//   alias: the specialisation under test throughout most of the suite.
using cm_meta = container_metadata<cm_box, cm_note>;


// ===========================================================================
// III. detection near-misses
// ===========================================================================
//   is_container_metadata is a duck-typing trait over four requirements: the
// two member type aliases, and get_container() and payload() both callable
// on a CONST object.  Each fixture below removes exactly one of them, so
// every requirement can be shown to be load-bearing rather than merely
// present.

// cm_duck
//   fixture: satisfies all four requirements without being a
// container_metadata, which is what makes the trait structural.
struct cm_duck
{
    using container_type = cm_box;
    using payload_type   = cm_note;

    cm_box*  m_container = nullptr;
    cm_note  m_payload;

    cm_box* get_container() const
    {
        return m_container;
    }

    const cm_note& payload() const
    {
        return m_payload;
    }
};

// cm_no_container_type
//   fixture: as cm_duck, without the container_type alias.
struct cm_no_container_type
{
    using payload_type = cm_note;

    cm_box*  m_container = nullptr;
    cm_note  m_payload;

    cm_box* get_container() const
    {
        return m_container;
    }

    const cm_note& payload() const
    {
        return m_payload;
    }
};

// cm_no_payload_type
//   fixture: as cm_duck, without the payload_type alias.
struct cm_no_payload_type
{
    using container_type = cm_box;

    cm_box*  m_container = nullptr;
    cm_note  m_payload;

    cm_box* get_container() const
    {
        return m_container;
    }

    const cm_note& payload() const
    {
        return m_payload;
    }
};

// cm_no_get_container
//   fixture: as cm_duck, without get_container().
struct cm_no_get_container
{
    using container_type = cm_box;
    using payload_type   = cm_note;

    cm_note m_payload;

    const cm_note& payload() const
    {
        return m_payload;
    }
};

// cm_no_payload_fn
//   fixture: as cm_duck, without payload().
struct cm_no_payload_fn
{
    using container_type = cm_box;
    using payload_type   = cm_note;

    cm_box* m_container = nullptr;

    cm_box* get_container() const
    {
        return m_container;
    }
};

// cm_nonconst_accessors
//   fixture: has all four names, but the two accessors are not
// const-qualified.  The trait probes a const object, so this is the sharpest
// near-miss of the set - everything is spelled correctly and it is still
// rejected.
struct cm_nonconst_accessors
{
    using container_type = cm_box;
    using payload_type   = cm_note;

    cm_box*  m_container = nullptr;
    cm_note  m_payload;

    cm_box* get_container()
    {
        return m_container;
    }

    const cm_note& payload()
    {
        return m_payload;
    }
};

// cm_protocol_type
//   fixture: a type written to the metadata protocol that REPLACED
// metadata.hpp, for the composition test to contrast against
// container_metadata itself.
struct cm_protocol_type
{
    using metadata_type           = cm_note;
    using metadata_container_type = cm_box;
    cm_note metadata;
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// ===========================================================================
// IV.  test declarations
// ===========================================================================

// I.   construction (container_metadata_tests_construction.cpp)
bool tests_container_metadata_construction();
bool tests_container_metadata_construction_hazards();

// II.  binding      (container_metadata_tests_binding.cpp)
bool tests_container_metadata_binding();
bool tests_container_metadata_non_owning();

// III. payload      (container_metadata_tests_payload.cpp)
bool tests_container_metadata_payload();
bool tests_container_metadata_set_payload();
bool tests_container_metadata_comparison();

// IV.  detection    (container_metadata_tests_detection.cpp)
bool tests_container_metadata_detection();
bool tests_container_metadata_detection_requirements();

// V.   factory      (container_metadata_tests_factory.cpp)
bool tests_container_metadata_factory();
bool tests_container_metadata_factory_forwarding();
bool tests_container_metadata_protocol_composition();


// ===========================================================================
// V.   spec provider
// ===========================================================================

#ifdef DTEST_SPEC_MODE

// container_metadata_spec
//   function: the declarative description of this suite - one block per
// section TU, one row per test, each carrying the descriptor the report
// renders.
inline dt::module_spec
container_metadata_spec()
{
    return dt::module_spec{
        "container_metadata",
        "A non-owning container pointer paired with an owned payload: six "
        "constructors, a bind/unbind lifecycle, payload accessors, "
        "payload-only equality, a structural detection trait and a "
        "deducing factory.",
        {
            dt::block_spec{
                "construction",
                "The six constructors and the two places where construction "
                "does not behave as it reads.",
                {
                    { "tests_container_metadata_construction",
                      "Each of the six constructors produces the state it "
                      "advertises; the single-argument forms are explicit "
                      "and the move-taking forms are noexcept exactly when "
                      "the payload's move is.",
                      &tests_container_metadata_construction },
                    { "tests_container_metadata_construction_hazards",
                      "is_default_constructible is a false positive for a "
                      "payload with no default constructor, and where "
                      "container and payload share a type the "
                      "single-argument overloads silently diverge on "
                      "const-ness rather than becoming ambiguous.",
                      &tests_container_metadata_construction_hazards },
                }
            },
            dt::block_spec{
                "binding",
                "The bind/unbind lifecycle and what it means for the "
                "container to be borrowed rather than owned.",
                {
                    { "tests_container_metadata_binding",
                      "bind attaches and rebinds, unbind clears the "
                      "association while preserving the payload, repeated "
                      "unbinding is harmless, and the whole surface is "
                      "noexcept.",
                      &tests_container_metadata_binding },
                    { "tests_container_metadata_non_owning",
                      "get_container() hands back the original object - "
                      "writes through it land on that object and are "
                      "visible through other handles - and wrapper "
                      "constness reaches the borrowed container.",
                      &tests_container_metadata_non_owning },
                }
            },
            dt::block_spec{
                "payload",
                "The owned half: accessing it, replacing it, and the "
                "equality that considers nothing else.",
                {
                    { "tests_container_metadata_payload",
                      "The two payload() overloads return references to the "
                      "stored object with constness tracking the wrapper, "
                      "and the payload survives changes to the binding.",
                      &tests_container_metadata_payload },
                    { "tests_container_metadata_set_payload",
                      "set_payload copies from an lvalue and moves from an "
                      "rvalue - measured with a counting payload - and "
                      "leaves the binding untouched.",
                      &tests_container_metadata_set_payload },
                    { "tests_container_metadata_comparison",
                      "Equality compares payloads only: objects bound to "
                      "different containers compare equal, an unbound "
                      "object equals a bound one, and only the payload "
                      "changes the answer.",
                      &tests_container_metadata_comparison },
                }
            },
            dt::block_spec{
                "detection",
                "is_container_metadata as a structural duck-typing trait, "
                "and the four requirements it rests on.",
                {
                    { "tests_container_metadata_detection",
                      "The trait accepts the template's specialisations and "
                      "any type of the same shape, rejects the hostile type "
                      "zoo, and is transparent to const but not to "
                      "references - unlike the protocol traits next door.",
                      &tests_container_metadata_detection },
                    { "tests_container_metadata_detection_requirements",
                      "Removing either type alias or either accessor drops "
                      "a type from the trait, and the accessors must be "
                      "const-callable - the sharpest near-miss, correct in "
                      "every name and still rejected.",
                      &tests_container_metadata_detection_requirements },
                }
            },
            dt::block_spec{
                "factory",
                "make_container_metadata, its forwarding, and the standing "
                "gap between this module and the metadata protocol.",
                {
                    { "tests_container_metadata_factory",
                      "The factory deduces both parameters, cleans its "
                      "arguments so no specialisation is over a reference "
                      "type, and binds the result to the container passed.",
                      &tests_container_metadata_factory },
                    { "tests_container_metadata_factory_forwarding",
                      "An rvalue payload is moved and an lvalue copied "
                      "exactly once, measured rather than inferred from the "
                      "signature.",
                      &tests_container_metadata_factory_forwarding },
                    { "tests_container_metadata_protocol_composition",
                      "container_metadata scores zero of three on the "
                      "metadata protocol while a type written to the new "
                      "names scores three of three - a recorded gap, "
                      "expected to fail when the module is brought across.",
                      &tests_container_metadata_protocol_composition },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONTAINER_METADATA_TESTS_
