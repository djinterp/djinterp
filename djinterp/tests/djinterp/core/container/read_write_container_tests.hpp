/*******************************************************************************
* djinterp [tests]                                read_write_container_tests.hpp
*
*   Suite header for read_write_container.hpp - the full-capability member of
* the access trio, the unrestricted baseline its two siblings restrict.
* Declares every test body, supplies the fixtures the bodies need, and (in
* DTEST_SPEC_MODE) exposes the spec provider the runner hands to run_module.
*
*   BUILD PREREQUISITE - THE MODULE'S INCLUDE PATHS DO NOT RESOLVE AS WRITTEN.
*   read_write_container.hpp sits at core/container/access/ and includes:
*
*       #include "../djinterp.hpp"         -> core/container/djinterp.hpp
*       #include "../meta/read_write.hpp"
*                                   -> core/container/meta/read_write.hpp
*
*   Neither target is where the named header ships.  The capability tag
* `read_write` ships at core/container/access/read_write.hpp - a SIBLING of the
* wrapper, per its own banner - so the include should read "read_write.hpp".
* And djinterp.hpp ships at core/djinterp.hpp, which from access/ is
* "../../djinterp.hpp"; that is exactly what the sibling tag headers already
* use.  Both of this module's paths are one level short.  All three access
* wrappers share the defect.  The harness supplies forwarding shims at the
* as-written locations; the fix belongs in the module's include block.
*
*   WHAT MAKES THIS WRAPPER TESTABLE AS A UNIT.
*   read_write adds no constraint - it adds UNIFORMITY.  Its whole contract is
* therefore a single invariant: the surface of read_write_container<C> is
* exactly the surface of C, member for member, for every C.  That is what the
* rw_caps row and rw_probe exist to state.  A member the container lacks must
* not appear (a std::array wrapper has no push_back); a member it has must not
* go missing (a std::list wrapper keeps push_front).  The adaptivity block
* asserts the whole row at once against the raw container, so a forwarder that
* silently stopped resolving would fail there rather than passing unnoticed.
*
*   The const and non-const halves are tested separately, because they are
* separate overload sets: the const surface must answer on a const handle, and
* the non-const surface must hand back references through which the held
* container really can be written.
*
*   PORTABILITY:
*   The module targets C++11.  Its mutating forwarders are constexpr only
* where C++14 relaxed constexpr permits a non-const member to be one, which the
* module handles with an internal macro it undefines again at the end of the
* file; the suite therefore tests constexpr usability through the CONST
* surface,
* which is constexpr at every standard.  Built at C++20 by the DTest CMake
* helper.
*
*
* path:      /tests/djinterp/core/container/read_write_container_tests.hpp
*                read_write_container_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                              created: 2026.07.23
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONSTRUCTION    (read_write_container_tests_construction.cpp)
      -------------------------------------------------------------

II.   TRAITS          (read_write_container_tests_traits.cpp)
      -------------------------------------------------------

III.  OBSERVATION     (read_write_container_tests_observation.cpp)
      ------------------------------------------------------------

IV.   ELEMENT_ACCESS  (read_write_container_tests_element_access.cpp)
      ---------------------------------------------------------------

V.    MUTATION        (read_write_container_tests_mutation.cpp)
      ---------------------------------------------------------

VI.   ADAPTIVITY      (read_write_container_tests_adaptivity.cpp)
      -----------------------------------------------------------
*/

#ifndef DJINTERP_TESTS_READ_WRITE_CONTAINER_TESTS_
#define DJINTERP_TESTS_READ_WRITE_CONTAINER_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <type_traits>
// djinterp
// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <array>
#include <deque>
#include <list>
#include <string>
#include <vector>
#include "../../../../inc/djinterp/core/container/read_write_container.hpp"
// the sibling overlays, for the contrast tests that show read_write really is
// the unrestricted member of the trio
#include "../../../../inc/djinterp/core/container/read_only_container.hpp"
#include "../../../../inc/djinterp/core/container/write_only_container.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"
#endif


NS_DJINTERP
NS_TESTING

// dt
//   type: names the DTest entities.  Spec mode only - in normal mode the
// suite includes the module under test rather than the framework, and
// djinterp::test is not in scope.
#ifdef DTEST_SPEC_MODE
namespace dt = ::djinterp::test;
#endif


// read_write_check
//   function: reports a failed assertion and returns the condition, so the
// D_RW_CHECK macro can early-return on the first failure.
//
// Parameter(s):
//   _cond: the asserted condition.
//   _expr: the stringized expression.
//   _file: the source file of the assertion.
//   _line: the source line of the assertion.
// Return:
//   _cond, unchanged.
inline bool
read_write_check(
    bool        _cond,
    const char* _expr,
    const char* _file,
    int         _line
)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

// D_RW_CHECK
//   macro: assert _cond, printing the expression and its location and
// returning false from the enclosing test body on failure.  Variadic so a
// top-level comma inside the expression passes through whole.
#define D_RW_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::read_write_check(                           \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// dj
//   type: names the foundation namespace the wrapper lives in.
namespace dj = ::djinterp;


// rw_void_help
//   type: maps any well-formed type list to void, the substrate every
// detector below is built on.  Spelled locally rather than using
// std::void_t, which is C++17.
template<typename...>
struct rw_void_impl
{
    typedef void type;
};

template<typename... _T>
struct rw_void_help
{
    typedef typename rw_void_impl<_T...>::type type;
};

#define D_RW_VOID(...)                                                    \
    typename ::djinterp::testing::rw_void_help<__VA_ARGS__>::type

// D_RW_DETECT
//   macro: defines a trait _name<T> that is true exactly when _expr is
// well-formed for T.
#define D_RW_DETECT(_name, _expr)                                             \
    template<typename _T, typename = void>                                    \
    struct _name : std::false_type {};                                        \
    template<typename _T>                                                     \
    struct _name<_T, D_RW_VOID(decltype(_expr))> : std::true_type {}

// -- the observation surface -----------------------------------------------
D_RW_DETECT(rw_has_size,   std::declval<const _T&>().size());
D_RW_DETECT(rw_has_empty,  std::declval<const _T&>().empty());
D_RW_DETECT(rw_has_sub,    std::declval<_T&>()[0]);
D_RW_DETECT(rw_has_front,  std::declval<_T&>().front());
D_RW_DETECT(rw_has_back,   std::declval<_T&>().back());
D_RW_DETECT(rw_has_data,   std::declval<_T&>().data());
D_RW_DETECT(rw_has_begin,  std::declval<_T&>().begin());
D_RW_DETECT(rw_has_end,    std::declval<_T&>().end());
D_RW_DETECT(rw_has_cbegin, std::declval<const _T&>().cbegin());
D_RW_DETECT(rw_has_cend,   std::declval<const _T&>().cend());

// -- the mutating surface --------------------------------------------------
D_RW_DETECT(rw_has_push_back,
    std::declval<_T&>().push_back(std::declval<typename _T::value_type>()));
D_RW_DETECT(rw_has_push_front,
    std::declval<_T&>().push_front(std::declval<typename _T::value_type>()));
D_RW_DETECT(rw_has_emplace_back,
    std::declval<_T&>().emplace_back(std::declval<typename _T::value_type>()));
D_RW_DETECT(rw_has_pop_back,  std::declval<_T&>().pop_back());
D_RW_DETECT(rw_has_pop_front, std::declval<_T&>().pop_front());
D_RW_DETECT(rw_has_clear,     std::declval<_T&>().clear());
D_RW_DETECT(rw_has_resize,    std::declval<_T&>().resize(std::size_t(1)));
D_RW_DETECT(rw_has_reserve,   std::declval<_T&>().reserve(std::size_t(1)));
D_RW_DETECT(rw_has_insert,
    std::declval<_T&>().insert(std::declval<_T&>().cbegin(),
                               std::declval<typename _T::value_type>()));
D_RW_DETECT(rw_has_erase,
    std::declval<_T&>().erase(std::declval<_T&>().cbegin()));


// rw_caps
//   struct: one row of the detection matrix - every member the suite probes,
// for a single type.  The wrapper's contract is that its row equals the held
// container's row exactly.
struct rw_caps
{
    bool size;
    bool empty;
    bool subscript;
    bool front;
    bool back;
    bool data;
    bool begin;
    bool end;
    bool cbegin;
    bool cend;

    bool push_back;
    bool push_front;
    bool emplace_back;
    bool pop_back;
    bool pop_front;
    bool clear;
    bool resize;
    bool reserve;
    bool insert;
    bool erase;
};

// rw_probe
//   function: the detection row for _T.
template<typename _T>
inline rw_caps
rw_probe()
{
    rw_caps c;

    c.size      = rw_has_size<_T>::value;
    c.empty     = rw_has_empty<_T>::value;
    c.subscript = rw_has_sub<_T>::value;
    c.front     = rw_has_front<_T>::value;
    c.back      = rw_has_back<_T>::value;
    c.data      = rw_has_data<_T>::value;
    c.begin     = rw_has_begin<_T>::value;
    c.end       = rw_has_end<_T>::value;
    c.cbegin    = rw_has_cbegin<_T>::value;
    c.cend      = rw_has_cend<_T>::value;

    c.push_back    = rw_has_push_back<_T>::value;
    c.push_front   = rw_has_push_front<_T>::value;
    c.emplace_back = rw_has_emplace_back<_T>::value;
    c.pop_back     = rw_has_pop_back<_T>::value;
    c.pop_front    = rw_has_pop_front<_T>::value;
    c.clear        = rw_has_clear<_T>::value;
    c.resize       = rw_has_resize<_T>::value;
    c.reserve      = rw_has_reserve<_T>::value;
    c.insert       = rw_has_insert<_T>::value;
    c.erase        = rw_has_erase<_T>::value;

    return c;
}

// rw_rows_match
//   function: whether two detection rows agree on every probed member - the
// wrapper's entire contract, in one predicate.
inline bool
rw_rows_match(
    const rw_caps& _a,
    const rw_caps& _b
)
{
    return ( (_a.size         == _b.size)         &&
             (_a.empty        == _b.empty)        &&
             (_a.subscript    == _b.subscript)    &&
             (_a.front        == _b.front)        &&
             (_a.back         == _b.back)         &&
             (_a.data         == _b.data)         &&
             (_a.begin        == _b.begin)        &&
             (_a.end          == _b.end)          &&
             (_a.cbegin       == _b.cbegin)       &&
             (_a.cend         == _b.cend)         &&
             (_a.push_back    == _b.push_back)    &&
             (_a.push_front   == _b.push_front)   &&
             (_a.emplace_back == _b.emplace_back) &&
             (_a.pop_back     == _b.pop_back)     &&
             (_a.pop_front    == _b.pop_front)    &&
             (_a.clear        == _b.clear)        &&
             (_a.resize       == _b.resize)       &&
             (_a.reserve      == _b.reserve)      &&
             (_a.insert       == _b.insert)       &&
             (_a.erase        == _b.erase) );
}

// rw_report_mismatch
//   function: prints the first member on which two rows disagree, so a
// failing mirror assertion names the offending forwarder rather than merely
// reporting a false.
inline void
rw_report_mismatch(
    const char*    _what,
    const rw_caps& _wrapped,
    const rw_caps& _raw
)
{
    struct cell
    {
        const char* name;
        bool        wrapped;
        bool        raw;
    };

    const cell cells[20] =
    {
        { "size",         _wrapped.size,         _raw.size },
        { "empty",        _wrapped.empty,        _raw.empty },
        { "subscript",    _wrapped.subscript,    _raw.subscript },
        { "front",        _wrapped.front,        _raw.front },
        { "back",         _wrapped.back,         _raw.back },
        { "data",         _wrapped.data,         _raw.data },
        { "begin",        _wrapped.begin,        _raw.begin },
        { "end",          _wrapped.end,          _raw.end },
        { "cbegin",       _wrapped.cbegin,       _raw.cbegin },
        { "cend",         _wrapped.cend,         _raw.cend },
        { "push_back",    _wrapped.push_back,    _raw.push_back },
        { "push_front",   _wrapped.push_front,   _raw.push_front },
        { "emplace_back", _wrapped.emplace_back, _raw.emplace_back },
        { "pop_back",     _wrapped.pop_back,     _raw.pop_back },
        { "pop_front",    _wrapped.pop_front,    _raw.pop_front },
        { "clear",        _wrapped.clear,        _raw.clear },
        { "resize",       _wrapped.resize,       _raw.resize },
        { "reserve",      _wrapped.reserve,      _raw.reserve },
        { "insert",       _wrapped.insert,       _raw.insert },
        { "erase",        _wrapped.erase,        _raw.erase },
    };

    std::size_t i = 0;

    for (i = 0; i < 20u; ++i)
    {
        if (cells[i].wrapped != cells[i].raw)
        {
            std::printf("    [FAIL] %s: %s wrapped=%d raw=%d\n",
                        _what, cells[i].name,
                        static_cast<int>(cells[i].wrapped),
                        static_cast<int>(cells[i].raw));
        }
    }

    return;
}


// rw_const_result
//   trait: whether _T is a reference to const.
template<typename _T>
struct rw_const_result
    : std::is_const<typename std::remove_reference<_T>::type>
{};

// rw_const_pointee
//   trait: whether _T is a pointer to const.
template<typename _T>
struct rw_const_pointee
    : std::is_const<typename std::remove_pointer<_T>::type>
{};


// rw_probe_type
//   type: a value type with a two-argument constructor, so emplace_back can
// be shown to forward more than one argument.
struct rw_probe_type
{
    int  a;
    char b;

    rw_probe_type()
        : a(0),
          b('\0')
    {}

    rw_probe_type(
        int  _a,
        char _b
    )
        : a(_a),
          b(_b)
    {}
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   CONSTRUCTION   (read_write_container_tests_construction.cpp)
bool tests_read_write_default_construction();
bool tests_read_write_wraps_a_container_value();
bool tests_read_write_emplaces_from_arguments();
bool tests_read_write_copy_and_move_construction();
bool tests_read_write_variadic_ctor_excludes_self();
bool tests_read_write_factory_deduces_container();
bool tests_read_write_holds_an_independent_copy();

// II.  TRAITS   (read_write_container_tests_traits.cpp)
bool tests_read_write_exposes_container_type();
bool tests_read_write_exposes_value_type();
bool tests_read_write_capability_is_read_write();
bool tests_read_write_capability_tag_semantics();
bool tests_read_write_typedefs_track_container();

// III. OBSERVATION   (read_write_container_tests_observation.cpp)
bool tests_read_write_size_and_empty();
bool tests_read_write_const_subscript_reads();
bool tests_read_write_const_front_and_back();
bool tests_read_write_const_data_and_iteration();
bool tests_read_write_cbegin_cend_are_const();
bool tests_read_write_const_handle_keeps_observers();
bool tests_read_write_observers_track_mutations();

// IV.  ELEMENT_ACCESS   (read_write_container_tests_element_access.cpp)
bool tests_read_write_subscript_returns_mutable_ref();
bool tests_read_write_front_and_back_are_mutable();
bool tests_read_write_data_returns_mutable_pointer();
bool tests_read_write_iterators_are_mutable();
bool tests_read_write_writes_reach_the_container();
bool tests_read_write_const_handle_narrows_to_const();
bool tests_read_write_both_overload_sets_coexist();

// V.   MUTATION   (read_write_container_tests_mutation.cpp)
bool tests_read_write_push_back_appends();
bool tests_read_write_push_front_prepends();
bool tests_read_write_emplace_back_forwards_args();
bool tests_read_write_pop_back_removes_last();
bool tests_read_write_pop_front_removes_first();
bool tests_read_write_insert_places_at_position();
bool tests_read_write_erase_removes_at_position();
bool tests_read_write_clear_empties_the_container();
bool tests_read_write_resize_changes_the_count();
bool tests_read_write_reserve_affects_capacity_only();

// VI.  ADAPTIVITY   (read_write_container_tests_adaptivity.cpp)
bool tests_read_write_mirrors_the_container_exactly();
bool tests_read_write_array_gains_no_mutator();
bool tests_read_write_list_keeps_both_ends();
bool tests_read_write_deque_and_string_surfaces();
bool tests_read_write_is_the_unrestricted_sibling();
bool tests_read_write_const_surface_is_constexpr();
bool tests_read_write_detection_battery_is_sound();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// read_write_container_spec
//   function: the suite's authoritative description - one block per section
// TU, one row per test body, each carrying the descriptor the report renders.
inline dt::module_spec
read_write_container_spec()
{
    return dt::module_spec{
        "read_write_container",
        "The read_write access overlay: the full-capability baseline that "
        "holds its container privately and forwards whichever members that "
        "container actually has, adding uniformity rather than constraint.",
        {
            dt::block_spec{
                "construction",
                "Populating the handle: the default, wrapping and emplacing "
                "constructors, the guard that keeps the variadic form from "
                "hijacking a copy, and the deducing factory.",
                {
                    { "tests_read_write_default_construction",
                      "a default-constructed handle holds a "
                      "default-constructed container",
                      &tests_read_write_default_construction },
                    { "tests_read_write_wraps_a_container_value",
                      "a single container argument is copied or moved into "
                      "the handle intact",
                      &tests_read_write_wraps_a_container_value },
                    { "tests_read_write_emplaces_from_arguments",
                      "several arguments construct the held container in "
                      "place, forwarding each one",
                      &tests_read_write_emplaces_from_arguments },
                    { "tests_read_write_copy_and_move_construction",
                      "a handle can be copied and moved, and the copy is "
                      "independently mutable",
                      &tests_read_write_copy_and_move_construction },
                    { "tests_read_write_variadic_ctor_excludes_self",
                      "the enable_if guard keeps the variadic constructor "
                      "from being chosen over the copy constructor",
                      &tests_read_write_variadic_ctor_excludes_self },
                    { "tests_read_write_factory_deduces_container",
                      "make_read_write deduces the container type and strips "
                      "reference and cv qualification from it",
                      &tests_read_write_factory_deduces_container },
                    { "tests_read_write_holds_an_independent_copy",
                      "the handle owns its container by value, so writes "
                      "through it do not reach the source",
                      &tests_read_write_holds_an_independent_copy },
                }
            },
            dt::block_spec{
                "traits",
                "The three published typedefs and the capability tag that "
                "identifies this overlay to generic code.",
                {
                    { "tests_read_write_exposes_container_type",
                      "container_type names the held container exactly",
                      &tests_read_write_exposes_container_type },
                    { "tests_read_write_exposes_value_type",
                      "value_type is taken from the held container rather "
                      "than restated",
                      &tests_read_write_exposes_value_type },
                    { "tests_read_write_capability_is_read_write",
                      "the capability typedef names the read_write tag, not "
                      "a sibling overlay",
                      &tests_read_write_capability_is_read_write },
                    { "tests_read_write_capability_tag_semantics",
                      "the tag reports both readable and writable, the only "
                      "one of the three that does",
                      &tests_read_write_capability_tag_semantics },
                    { "tests_read_write_typedefs_track_container",
                      "the typedefs follow whatever container is wrapped "
                      "while the capability stays fixed",
                      &tests_read_write_typedefs_track_container },
                }
            },
            dt::block_spec{
                "observation",
                "The const half of the surface: the observers that must "
                "answer on a const handle and keep answering as the "
                "container is mutated.",
                {
                    { "tests_read_write_size_and_empty",
                      "size and empty report the held container's own state",
                      &tests_read_write_size_and_empty },
                    { "tests_read_write_const_subscript_reads",
                      "the const subscript reads every element through a "
                      "const handle",
                      &tests_read_write_const_subscript_reads },
                    { "tests_read_write_const_front_and_back",
                      "the const end-element accessors name the first and "
                      "last elements",
                      &tests_read_write_const_front_and_back },
                    { "tests_read_write_const_data_and_iteration",
                      "the const storage pointer and iteration range span "
                      "exactly the held elements",
                      &tests_read_write_const_data_and_iteration },
                    { "tests_read_write_cbegin_cend_are_const",
                      "cbegin and cend yield const iterators even from a "
                      "non-const handle",
                      &tests_read_write_cbegin_cend_are_const },
                    { "tests_read_write_const_handle_keeps_observers",
                      "a const handle retains the whole observing surface "
                      "and loses only the mutating one",
                      &tests_read_write_const_handle_keeps_observers },
                    { "tests_read_write_observers_track_mutations",
                      "the observers reflect each mutation immediately, so "
                      "the two halves address one container",
                      &tests_read_write_observers_track_mutations },
                }
            },
            dt::block_spec{
                "element_access",
                "The non-const half: the accessors that hand back references "
                "through which the held container really can be written.",
                {
                    { "tests_read_write_subscript_returns_mutable_ref",
                      "subscript on a non-const handle yields a reference to "
                      "non-const, and assigning through it takes effect",
                      &tests_read_write_subscript_returns_mutable_ref },
                    { "tests_read_write_front_and_back_are_mutable",
                      "the end-element accessors are writable in the same "
                      "way",
                      &tests_read_write_front_and_back_are_mutable },
                    { "tests_read_write_data_returns_mutable_pointer",
                      "data yields a pointer to non-const, and writing "
                      "through it reaches the container",
                      &tests_read_write_data_returns_mutable_pointer },
                    { "tests_read_write_iterators_are_mutable",
                      "begin and end yield ordinary iterators whose "
                      "referents can be assigned",
                      &tests_read_write_iterators_are_mutable },
                    { "tests_read_write_writes_reach_the_container",
                      "a write through any of the four routes is visible "
                      "through all of the others",
                      &tests_read_write_writes_reach_the_container },
                    { "tests_read_write_const_handle_narrows_to_const",
                      "the same accessors yield const references through a "
                      "const handle, so constness propagates",
                      &tests_read_write_const_handle_narrows_to_const },
                    { "tests_read_write_both_overload_sets_coexist",
                      "the const and non-const overloads select correctly by "
                      "the handle's own constness",
                      &tests_read_write_both_overload_sets_coexist },
                }
            },
            dt::block_spec{
                "mutation",
                "The forwarded mutating surface: each operation reaches the "
                "held container with its arguments intact and leaves the "
                "observers agreeing.",
                {
                    { "tests_read_write_push_back_appends",
                      "push_back adds one element at the end and grows the "
                      "reported size",
                      &tests_read_write_push_back_appends },
                    { "tests_read_write_push_front_prepends",
                      "push_front adds one element at the front, for the "
                      "containers that offer it",
                      &tests_read_write_push_front_prepends },
                    { "tests_read_write_emplace_back_forwards_args",
                      "emplace_back constructs in place, forwarding several "
                      "arguments to the element's constructor",
                      &tests_read_write_emplace_back_forwards_args },
                    { "tests_read_write_pop_back_removes_last",
                      "pop_back removes exactly the final element",
                      &tests_read_write_pop_back_removes_last },
                    { "tests_read_write_pop_front_removes_first",
                      "pop_front removes exactly the first element, for the "
                      "containers that offer it",
                      &tests_read_write_pop_front_removes_first },
                    { "tests_read_write_insert_places_at_position",
                      "insert places an element at the requested position "
                      "and returns the container's own result",
                      &tests_read_write_insert_places_at_position },
                    { "tests_read_write_erase_removes_at_position",
                      "erase removes the element at the requested position",
                      &tests_read_write_erase_removes_at_position },
                    { "tests_read_write_clear_empties_the_container",
                      "clear removes every element and leaves the handle "
                      "reporting empty",
                      &tests_read_write_clear_empties_the_container },
                    { "tests_read_write_resize_changes_the_count",
                      "resize grows and shrinks the element count, keeping "
                      "the surviving elements",
                      &tests_read_write_resize_changes_the_count },
                    { "tests_read_write_reserve_affects_capacity_only",
                      "reserve changes capacity without disturbing the "
                      "elements or the reported size",
                      &tests_read_write_reserve_affects_capacity_only },
                }
            },
            dt::block_spec{
                "adaptivity",
                "The wrapper's whole contract as one invariant: the surface "
                "of the handle equals the surface of the held container, for "
                "every container tried.",
                {
                    { "tests_read_write_mirrors_the_container_exactly",
                      "the handle's detection row equals the raw container's "
                      "on all twenty probed members",
                      &tests_read_write_mirrors_the_container_exactly },
                    { "tests_read_write_array_gains_no_mutator",
                      "wrapping a fixed-size container does not conjure "
                      "push_back, clear or resize",
                      &tests_read_write_array_gains_no_mutator },
                    { "tests_read_write_list_keeps_both_ends",
                      "wrapping a node container keeps push_front and "
                      "pop_front and still omits subscript and data",
                      &tests_read_write_list_keeps_both_ends },
                    { "tests_read_write_deque_and_string_surfaces",
                      "a deque keeps random access without data; a string "
                      "keeps the full contiguous surface",
                      &tests_read_write_deque_and_string_surfaces },
                    { "tests_read_write_is_the_unrestricted_sibling",
                      "against the same container, read_write keeps what "
                      "read_only and write_only each seal away",
                      &tests_read_write_is_the_unrestricted_sibling },
                    { "tests_read_write_const_surface_is_constexpr",
                      "the const forwarders really are usable at compile "
                      "time when the held container permits it",
                      &tests_read_write_const_surface_is_constexpr },
                    { "tests_read_write_detection_battery_is_sound",
                      "the suite's own detectors report correctly on raw "
                      "containers, so a mirror failure cannot be spurious",
                      &tests_read_write_detection_battery_is_sound },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_READ_WRITE_CONTAINER_TESTS_
