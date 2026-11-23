/*******************************************************************************
* djinterp [cpp][meta]                               memory_discipline_tests.hpp
*
*   Declarations, fixtures, the per-suite assertion macro and the spec for the
* memory_discipline.hpp unit tests.  Two faces, selected by DTEST_SPEC_MODE:
* normal mode (the section TUs) pulls in the header under test and the
* fixtures; spec mode (the runner) pulls in test_defaults.hpp and exposes
* memory_discipline_spec().  memory_discipline.hpp owns the discipline
* taxonomy (the memory_discipline enum), its algebra of constexpr predicates and
* the name spelling, a type-level carrier and its five aliases, the cross-axis
* bridge to storage (discipline_implies_siting / discipline_siting_consistent),
* the strategy-contract detection signals (and the two constant readers), and the
* strategy-level classifier discipline_of.  Every tests_* predicate here is
* authored flat in djinterp::testing and referenced by address from the spec.
*
*   The section .cpp files that DEFINE these predicates mirror the header:
*     memory_discipline_tests_taxonomy.cpp    - I, II.  enum + algebra + name
*     memory_discipline_tests_carrier.cpp     - III.    carrier + five aliases
*     memory_discipline_tests_bridge.cpp      - IV.     the storage bridge
*     memory_discipline_tests_detection.cpp   - V.      contract detection signals
*     memory_discipline_tests_classifier.cpp  - VI.     discipline_of (+ _t + _v)
*
*   The strategy fixtures below are fabricated to select each classifier branch,
* and to isolate each detection surface.  They advertise only the surfaces the
* classifier reads (strategy_storage_kind, an allocate() shape, and the optional
* supports_individual_release / pointer_stable constants); nothing else.
*
*
* path:      /tests/djinterp/cpp/meta/memory_discipline_tests.hpp
* link(s):   TBA
* author(s): unit-test generator                             created: 2026.07.17
*                                                            revised: 2026.11.14
*******************************************************************************/

#ifndef DJINTERP_META_MEMORY_DISCIPLINE_TESTS_
#define DJINTERP_META_MEMORY_DISCIPLINE_TESTS_ 1

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/meta/memory_discipline.hpp>  // the module under test
#endif
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec (spec)
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).
namespace dt = ::djinterp::test;


// memory_discipline_check
//   function: routes one D_MD_CHECK evaluation.  Prints the failing
// expression and its location and hands the condition back.
inline
bool memory_discipline_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_MD_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds, printing the expression that failed.  Variadic
// so a condition may contain top-level commas (template argument lists,
// brace-init).  Unique letters (MD) so co-compiled suites never collide on the
// macro name.
#define D_MD_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::memory_discipline_check(                    \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// str_eq
//   helper: byte-wise C-string equality, so a test may compare a returned
// spelling to a literal without pulling <cstring> semantics into the suite.
inline
bool str_eq(
    const char* _a,
    const char* _b
)
{
    while ((*_a != '\0') && (*_a == *_b))
    {
        ++_a;
        ++_b;
    }

    return ( *_a == *_b );
}

// ===========================================================================
// Fixtures - fabricated strategy contracts
// ===========================================================================

// md_unknown
//   type: no strategy_storage_kind at all - not a recognisable strategy, so the
// classifier's first gate reports unknown.  Also the negative fixture for every
// detection signal.
struct md_unknown
{
    int unrelated;
};

// DMdNone
//   type: advertises the strategy marker but exposes NO allocate surface -
// descriptive-only, so the classifier reports none.
struct DMdNone
{
    static constexpr int strategy_storage_kind = 0;
};

// md_arena_absent
//   type: marker + an element allocate(size_t), and NO supports_individual_release
// constant.  Absent individual-release defaults false -> arena.  Isolates the
// element-allocate signal (it has no byte allocate).
struct md_arena_absent
{
    static constexpr int strategy_storage_kind = 0;

    void* allocate(
        std::size_t
    )
    {
        return nullptr;
    }
};

// md_arena_false
//   type: marker + element allocate + supports_individual_release = FALSE
// explicitly.  Exercises read_individual_release's present-but-false path;
// still classifies arena.
struct md_arena_false
{
    static constexpr int  strategy_storage_kind        = 0;
    static constexpr bool supports_individual_release  = false;

    void* allocate(
        std::size_t
    )
    {
        return nullptr;
    }
};

// md_byte_alloc
//   type: marker + only a byte allocate(size_t, size_t) (pmr-shaped), NO element
// allocate, and no release constant.  Proves the byte-allocate signal alone
// satisfies "allocates" (so it is not none); classifies arena.  Isolates the
// byte-allocate signal.
struct md_byte_alloc
{
    static constexpr int strategy_storage_kind = 0;

    void* allocate(
        std::size_t,
        std::size_t
    )
    {
        return nullptr;
    }
};

// md_pooled
//   type: marker + allocate + supports_individual_release = true + pointer_stable
// = true -> per-object release AND pointer-stable -> pooled.
struct md_pooled
{
    static constexpr int  strategy_storage_kind       = 0;
    static constexpr bool supports_individual_release = true;
    static constexpr bool pointer_stable              = true;

    void* allocate(
        std::size_t
    )
    {
        return nullptr;
    }
};

// md_individual_absent
//   type: marker + allocate + supports_individual_release = true, and NO
// pointer_stable constant.  Absent pointer-stability defaults false -> individual.
struct md_individual_absent
{
    static constexpr int  strategy_storage_kind       = 0;
    static constexpr bool supports_individual_release = true;

    void* allocate(
        std::size_t
    )
    {
        return nullptr;
    }
};

// md_individual_false
//   type: marker + allocate + supports_individual_release = true + pointer_stable
// = FALSE explicitly.  Exercises read_pointer_stable's present-but-false path;
// classifies individual.
struct md_individual_false
{
    static constexpr int  strategy_storage_kind       = 0;
    static constexpr bool supports_individual_release = true;
    static constexpr bool pointer_stable              = false;

    void* allocate(
        std::size_t
    )
    {
        return nullptr;
    }
};

// md_function_constants
//   type: marker + allocate, with supports_individual_release and
// pointer_stable as static member FUNCTIONS.  A function is not a constant, so
// the type declares neither: both signals are false, both readers read the
// conservative false, and it classifies arena.  (It classified pooled until
// 2026.11.11: each function decayed to a pointer that is never null.)
struct md_function_constants
{
    static constexpr int strategy_storage_kind = 0;

    static
    bool supports_individual_release();

    static
    bool pointer_stable();

    void* allocate(
        std::size_t
    )
    {
        return nullptr;
    }
};

// md_member_constants
//   type: marker + allocate, with supports_individual_release and
// pointer_stable as NON-STATIC data members.  Not constants either: it
// declares neither and classifies arena.  (Until 2026.11.11 the readers did
// not compile for it.)
struct md_member_constants
{
    static constexpr int strategy_storage_kind = 0;

    bool supports_individual_release;
    bool pointer_stable;

    void* allocate(
        std::size_t
    )
    {
        return nullptr;
    }
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// ===========================================================================
// Declarations  (grouped by the section of memory_discipline.hpp they exercise)
// ===========================================================================

// I.   the taxonomy enum
// II.  the discipline algebra + name
bool tests_memory_discipline_enum_distinct();
bool tests_memory_discipline_predicates();
bool tests_memory_discipline_family_and_allocates();
bool tests_memory_discipline_name();

// III. the type-level carrier + five aliases
bool tests_memory_discipline_carrier();

// IV.  the cross-axis bridge to storage
bool tests_memory_discipline_implies_siting();
bool tests_memory_discipline_siting_consistent();

// V.   strategy-contract detection signals + constant readers
bool tests_memory_discipline_signal_kind_and_allocate();
bool tests_memory_discipline_signal_constants();
bool tests_memory_discipline_read_constants();

// VI.  the strategy-level classifier
bool tests_memory_discipline_classify_unknown_none();
bool tests_memory_discipline_classify_arena();
bool tests_memory_discipline_classify_pooled_individual();
bool tests_memory_discipline_classify_allocate_isolation();
bool tests_memory_discipline_classify_aliases_and_v();


// ===========================================================================
// The spec  (spec mode only)
// ===========================================================================
#ifdef DTEST_SPEC_MODE

// memory_discipline_spec
//   function: the authoritative description of this suite, one block per
// section file.
inline
dt::module_spec memory_discipline_spec()
{
    return dt::module_spec{
        "memory_discipline",
        "meta/memory_discipline.hpp: the five-value discipline taxonomy, its "
        "predicates and names, the type-level carrier, the bridge to storage "
        "siting, the strategy-contract signals and readers, and the "
        "discipline_of classifier.",
        {
            dt::block_spec{
                "taxonomy",
                "The enum, its algebra of predicates, and its names.",
                {
                    { "tests_memory_discipline_enum_distinct",
                      "The five discipline values are mutually exclusive.",
                      &tests_memory_discipline_enum_distinct },
                    { "tests_memory_discipline_predicates",
                      "Each single-value predicate recognises exactly its "
                      "own discipline.",
                      &tests_memory_discipline_predicates },
                    { "tests_memory_discipline_family_and_allocates",
                      "The two grouping predicates partition the "
                      "disciplines.",
                      &tests_memory_discipline_family_and_allocates },
                    { "tests_memory_discipline_name",
                      "The name spelling is stable for every value.",
                      &tests_memory_discipline_name },
                }
            },
            dt::block_spec{
                "carrier",
                "memory_discipline_constant and its five aliases.",
                {
                    { "tests_memory_discipline_carrier",
                      "The carrier reifies a discipline value as a distinct "
                      "type.",
                      &tests_memory_discipline_carrier },
                }
            },
            dt::block_spec{
                "bridge",
                "The cross-axis bridge to storage siting.",
                {
                    { "tests_memory_discipline_implies_siting",
                      "Each discipline forces the siting the model "
                      "prescribes.",
                      &tests_memory_discipline_implies_siting },
                    { "tests_memory_discipline_siting_consistent",
                      "A (discipline, siting) pair is consistent exactly "
                      "when the bridge allows it.",
                      &tests_memory_discipline_siting_consistent },
                }
            },
            dt::block_spec{
                "detection",
                "The strategy-contract signals and the two constant readers.",
                {
                    { "tests_memory_discipline_signal_kind_and_allocate",
                      "The marker and the two allocate shapes are each "
                      "detected precisely.",
                      &tests_memory_discipline_signal_kind_and_allocate },
                    { "tests_memory_discipline_signal_constants",
                      "The two optional constants are detected by "
                      "declaration, not by value.",
                      &tests_memory_discipline_signal_constants },
                    { "tests_memory_discipline_read_constants",
                      "The readers yield the value when present and false "
                      "when absent.",
                      &tests_memory_discipline_read_constants },
                }
            },
            dt::block_spec{
                "classifier",
                "discipline_of, with its _t and _v forms.",
                {
                    { "tests_memory_discipline_classify_unknown_none",
                      "Not-a-strategy reads unknown; descriptive-only reads "
                      "none.",
                      &tests_memory_discipline_classify_unknown_none },
                    { "tests_memory_discipline_classify_arena",
                      "An allocating strategy with no individual release "
                      "reads arena.",
                      &tests_memory_discipline_classify_arena },
                    { "tests_memory_discipline_classify_pooled_individual",
                      "An individual-release strategy splits on pointer "
                      "stability.",
                      &tests_memory_discipline_classify_pooled_individual },
                    { "tests_memory_discipline_classify_allocate_isolation",
                      "Either allocate shape counts as allocating.",
                      &tests_memory_discipline_classify_allocate_isolation },
                    { "tests_memory_discipline_classify_aliases_and_v",
                      "The _t and _v forms track discipline_of, through cv "
                      "and references.",
                      &tests_memory_discipline_classify_aliases_and_v },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_META_MEMORY_DISCIPLINE_TESTS_
