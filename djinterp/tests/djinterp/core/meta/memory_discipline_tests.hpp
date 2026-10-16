/*******************************************************************************
* djinterp [meta/test]                               memory_discipline_tests.hpp
*
*   Declarations, fixtures, and the per-suite assertion macro for the
* memory_discipline.hpp unit tests.  memory_discipline.hpp owns the discipline
* taxonomy (the memory_discipline enum), its algebra of constexpr predicates and
* the name spelling, a type-level carrier and its five aliases, the cross-axis
* bridge to storage (discipline_implies_siting / discipline_siting_consistent),
* the strategy-contract detection signals (and the two constant readers), and the
* strategy-level classifier discipline_of.  Every tests_* predicate here is
* authored flat in djinterp::testing and referenced by address from the runner.
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
* path:      /tests/djinterp/core/meta/memory_discipline_tests.hpp
* link(s):   TBA
* author(s): unit-test generator                             created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_META_MEMORY_DISCIPLINE_TESTS_
#define DJINTERP_META_MEMORY_DISCIPLINE_TESTS_ 1

// std
#include <cstddef>
// djinterp
#include "../../../../inc/djinterp/core/meta/memory_discipline.hpp"   // the module under test (bare include)


// D_MD_CHECK
//   macro: fail the current test - return false from the enclosing predicate -
// unless the condition holds.  Variadic so a condition may contain top-level
// commas (template argument lists, brace-init).  Unique letters (MD) so
// co-compiled suites never collide on the macro name.
#define D_MD_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


NS_DJINTERP
NS_TESTING

// str_eq
//   helper: byte-wise C-string equality, so a test may compare a returned
// spelling to a literal without pulling <cstring> semantics into the suite.
inline bool
str_eq(const char* _a, const char* _b)
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

// md_none
//   type: advertises the strategy marker but exposes NO allocate surface -
// descriptive-only, so the classifier reports none.
struct md_none
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

    void* allocate(std::size_t)
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

    void* allocate(std::size_t)
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

    void* allocate(std::size_t, std::size_t)
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

    void* allocate(std::size_t)
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

    void* allocate(std::size_t)
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

    void* allocate(std::size_t)
    {
        return nullptr;
    }
};


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

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_META_MEMORY_DISCIPLINE_TESTS_
