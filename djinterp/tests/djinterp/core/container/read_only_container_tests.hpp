/*******************************************************************************
* djinterp [tests]                                 read_only_container_tests.hpp
*
*   Suite header for read_only_container.hpp - the access overlay that holds a
* container privately and forwards only its const observation surface.
* Declares every test body, supplies the fixtures the bodies need, and (in
* DTEST_SPEC_MODE) exposes the spec provider the runner hands to run_module.
*
*   BUILD PREREQUISITE - THE MODULE'S INCLUDE PATHS DO NOT RESOLVE AS WRITTEN.
*   read_only_container.hpp sits at core/container/access/ and includes:
*
*       #include "../djinterp.hpp"        -> core/container/djinterp.hpp
*       #include "../meta/read_only.hpp"  -> core/container/meta/read_only.hpp
*
*   Neither target is where the named header actually ships.  The capability
* tag `read_only` ships at core/container/access/read_only.hpp - a SIBLING of
* the wrapper, per its own banner - so the include should read "read_only.hpp"
* rather than reaching up and across into a meta/ directory.  And djinterp.hpp
* ships at core/djinterp.hpp, which from access/ is "../../djinterp.hpp"; that
* is exactly what the sibling tag headers read_only.hpp and read_write.hpp
* already use.  Both of this module's paths are one level short.
*
*   This suite does not patch the module.  The validation harness supplies
* forwarding shims at the two as-written locations so the header compiles
* unchanged, and the discrepancy is reported here per the authoring guide.
* The fix belongs in the module's include block.
*
*   NOTE ALSO - THE C++11 BASELINE IS BLOCKED BY A DEPENDENCY, NOT BY THIS
* MODULE.  The three access wrappers are genuinely C++11-clean: given a
* C++11-compatible clean_t they compile with zero errors at -std=c++11 through
* -std=c++20.  Their dependency does not follow: djinterp.hpp spells the alias
* `using clean_t = clean<_Type>::type;` with no `typename`, which is a C++20
* relaxation (P0634), and uses remove_cv_t / remove_reference_t, which are
* C++14.  So the documented "C++11 baseline" is unreachable in practice until
* that alias is written `typename clean<_Type>::type` over the C++11 traits.
* The claim is the module's to make and the obstruction is not the module's to
* fix, so it is recorded rather than asserted against.
*
*   HOW THE SEAL IS TESTED.
*   The module's central claim is negative - that there is NO path through this
* handle by which the held container can change.  A negative claim needs
* compile-time detection, not a runtime call, so the suite carries a battery of
* SFINAE detectors and rolls them into one ro_caps row per type.  A test then
* asserts the whole row at once.
*
*   Two distinct things make the seal, and the suite separates them:
*   - ABSENCE.  push_back, insert, erase, clear and the rest are not members at
*     all, so the detectors report false for every underlying container.
*   - CONSTNESS.  operator[], front, back and data DO exist, because observing
*     is the point; what makes them safe is that each returns a const-qualified
*     reference or pointer, so nothing can be written through the value they
*     hand back.  Testing only for absence would miss this half entirely.
*
*   PORTABILITY:
*   The module targets C++11 (see the note above); this suite is written to the
* same baseline apart from the fixtures.  Built at C++20 by the DTest CMake
* helper.
*
*
* path:      /tests/djinterp/core/container/read_only_container_tests.hpp
*                read_only_container_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                              created: 2026.07.23
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONSTRUCTION  (read_only_container_tests_construction.cpp)
      ----------------------------------------------------------

II.   TRAITS        (read_only_container_tests_traits.cpp)
      ----------------------------------------------------

III.  OBSERVATION   (read_only_container_tests_observation.cpp)
      ---------------------------------------------------------

IV.   SEAL          (read_only_container_tests_seal.cpp)
      --------------------------------------------------

V.    ADAPTIVITY    (read_only_container_tests_adaptivity.cpp)
      --------------------------------------------------------
*/

#ifndef DJINTERP_TESTS_READ_ONLY_CONTAINER_TESTS_
#define DJINTERP_TESTS_READ_ONLY_CONTAINER_TESTS_ 1

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
#include "../../../../inc/djinterp/core/container/read_only_container.hpp"
// the seal block contrasts this overlay against its unrestricted sibling -
// same held container, different handle - which is what shows the constness
// of the returned references to be the WRAPPER's doing rather than the
// container's.  Included here so every section TU sees one consistent set.
#include "../../../../inc/djinterp/core/container/read_write_container.hpp"
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
// djinterp::test is not in scope.  The module under test lives directly in
// djinterp, unlike the test/ modules other suites cover.
#ifdef DTEST_SPEC_MODE
namespace dt = ::djinterp::test;
#endif


// read_only_check
//   function: reports a failed assertion and returns the condition, so the
// D_RO_CHECK macro can early-return on the first failure.
//
// Parameter(s):
//   _cond: the asserted condition.
//   _expr: the stringized expression.
//   _file: the source file of the assertion.
//   _line: the source line of the assertion.
// Return:
//   _cond, unchanged.
inline bool
read_only_check(
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

// D_RO_CHECK
//   macro: assert _cond, printing the expression and its location and
// returning false from the enclosing test body on failure.  Variadic so a
// top-level comma inside the expression passes through whole.
#define D_RO_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::read_only_check(                            \
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


// ro_void_t
//   type: maps any well-formed type list to void, the substrate every
// detector below is built on.  Spelled locally rather than using
// std::void_t, which is C++17.
template<typename...>
struct ro_void_impl
{
    typedef void type;
};

template<typename... _T>
struct ro_void_help
{
    typedef typename ro_void_impl<_T...>::type type;
};

#define D_RO_VOID(...)                                                    \
    typename ::djinterp::testing::ro_void_help<__VA_ARGS__>::type

// D_RO_DETECT
//   macro: defines a trait _name<T> that is true exactly when _expr is
// well-formed for T.  One line per member keeps the battery readable and
// makes an added member a one-line change.
#define D_RO_DETECT(_name, _expr)                                             \
    template<typename _T, typename = void>                                    \
    struct _name : std::false_type {};                                        \
    template<typename _T>                                                     \
    struct _name<_T, D_RO_VOID(decltype(_expr))> : std::true_type {}

// -- the observation surface -----------------------------------------------
D_RO_DETECT(ro_has_size,   std::declval<const _T&>().size());
D_RO_DETECT(ro_has_empty,  std::declval<const _T&>().empty());
D_RO_DETECT(ro_has_sub,    std::declval<_T&>()[0]);
D_RO_DETECT(ro_has_front,  std::declval<_T&>().front());
D_RO_DETECT(ro_has_back,   std::declval<_T&>().back());
D_RO_DETECT(ro_has_data,   std::declval<_T&>().data());
D_RO_DETECT(ro_has_begin,  std::declval<_T&>().begin());
D_RO_DETECT(ro_has_end,    std::declval<_T&>().end());
D_RO_DETECT(ro_has_cbegin, std::declval<const _T&>().cbegin());
D_RO_DETECT(ro_has_cend,   std::declval<const _T&>().cend());

// -- the mutating surface, which must be absent ----------------------------
D_RO_DETECT(ro_has_push_back,
    std::declval<_T&>().push_back(std::declval<typename _T::value_type>()));
D_RO_DETECT(ro_has_push_front,
    std::declval<_T&>().push_front(std::declval<typename _T::value_type>()));
D_RO_DETECT(ro_has_emplace_back,
    std::declval<_T&>().emplace_back(std::declval<typename _T::value_type>()));
D_RO_DETECT(ro_has_pop_back,  std::declval<_T&>().pop_back());
D_RO_DETECT(ro_has_pop_front, std::declval<_T&>().pop_front());
D_RO_DETECT(ro_has_clear,     std::declval<_T&>().clear());
D_RO_DETECT(ro_has_resize,    std::declval<_T&>().resize(std::size_t(1)));
D_RO_DETECT(ro_has_reserve,   std::declval<_T&>().reserve(std::size_t(1)));
D_RO_DETECT(ro_has_insert,
    std::declval<_T&>().insert(std::declval<_T&>().cbegin(),
                               std::declval<typename _T::value_type>()));
D_RO_DETECT(ro_has_erase,
    std::declval<_T&>().erase(std::declval<_T&>().cbegin()));


// ro_caps
//   struct: one row of the detection matrix - every member the suite probes,
// for a single type.  Asserting a whole row at once is what lets a single
// test body cover the entire surface of one instantiation.
struct ro_caps
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

// ro_probe
//   function: the detection row for _T.
template<typename _T>
inline ro_caps
ro_probe()
{
    ro_caps c;

    c.size      = ro_has_size<_T>::value;
    c.empty     = ro_has_empty<_T>::value;
    c.subscript = ro_has_sub<_T>::value;
    c.front     = ro_has_front<_T>::value;
    c.back      = ro_has_back<_T>::value;
    c.data      = ro_has_data<_T>::value;
    c.begin     = ro_has_begin<_T>::value;
    c.end       = ro_has_end<_T>::value;
    c.cbegin    = ro_has_cbegin<_T>::value;
    c.cend      = ro_has_cend<_T>::value;

    c.push_back    = ro_has_push_back<_T>::value;
    c.push_front   = ro_has_push_front<_T>::value;
    c.emplace_back = ro_has_emplace_back<_T>::value;
    c.pop_back     = ro_has_pop_back<_T>::value;
    c.pop_front    = ro_has_pop_front<_T>::value;
    c.clear        = ro_has_clear<_T>::value;
    c.resize       = ro_has_resize<_T>::value;
    c.reserve      = ro_has_reserve<_T>::value;
    c.insert       = ro_has_insert<_T>::value;
    c.erase        = ro_has_erase<_T>::value;

    return c;
}

// ro_any_mutator
//   function: whether _c reports ANY mutating member.  The seal's whole claim
// reduces to this being false.
inline bool
ro_any_mutator(
    const ro_caps& _c
)
{
    return ( _c.push_back    || _c.push_front || _c.emplace_back ||
             _c.pop_back     || _c.pop_front  || _c.clear        ||
             _c.resize       || _c.reserve    || _c.insert       ||
             _c.erase );
}

// ro_observers_match
//   function: whether two rows agree on every OBSERVING member.  Used to show
// that wrapping neither adds nor removes an observer relative to the held
// container.
inline bool
ro_observers_match(
    const ro_caps& _a,
    const ro_caps& _b
)
{
    return ( (_a.size      == _b.size)      &&
             (_a.empty     == _b.empty)     &&
             (_a.subscript == _b.subscript) &&
             (_a.front     == _b.front)     &&
             (_a.back      == _b.back)      &&
             (_a.data      == _b.data)      &&
             (_a.begin     == _b.begin)     &&
             (_a.end       == _b.end)       &&
             (_a.cbegin    == _b.cbegin)    &&
             (_a.cend      == _b.cend) );
}


// ro_const_result
//   trait: whether _T is a reference to const - the property that makes an
// observer safe to hand out.
template<typename _T>
struct ro_const_result
    : std::is_const<typename std::remove_reference<_T>::type>
{};

// ro_const_pointee
//   trait: whether _T is a pointer to const.
template<typename _T>
struct ro_const_pointee
    : std::is_const<typename std::remove_pointer<_T>::type>
{};


// ro_probe_type
//   type: a value type with a two-argument constructor, so the emplacing
// constructor can be shown to forward more than one argument.
struct ro_probe_type
{
    int  a;
    char b;

    ro_probe_type()
        : a(0),
          b('\0')
    {}

    ro_probe_type(
        int  _a,
        char _b
    )
        : a(_a),
          b(_b)
    {}
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   CONSTRUCTION   (read_only_container_tests_construction.cpp)
bool tests_read_only_default_construction();
bool tests_read_only_wraps_a_container_value();
bool tests_read_only_emplaces_from_arguments();
bool tests_read_only_copy_and_move_construction();
bool tests_read_only_variadic_ctor_excludes_self();
bool tests_read_only_factory_deduces_container();
bool tests_read_only_holds_an_independent_copy();

// II.  TRAITS   (read_only_container_tests_traits.cpp)
bool tests_read_only_exposes_container_type();
bool tests_read_only_exposes_value_type();
bool tests_read_only_capability_is_read_only();
bool tests_read_only_capability_tag_semantics();
bool tests_read_only_typedefs_track_container();

// III. OBSERVATION   (read_only_container_tests_observation.cpp)
bool tests_read_only_size_and_empty();
bool tests_read_only_subscript_reads_elements();
bool tests_read_only_front_and_back();
bool tests_read_only_data_addresses_storage();
bool tests_read_only_iteration_visits_elements();
bool tests_read_only_cbegin_cend_match_begin_end();
bool tests_read_only_observers_work_on_const_handle();
bool tests_read_only_observes_an_empty_container();
bool tests_read_only_forwards_element_values_faithfully();

// IV.  SEAL   (read_only_container_tests_seal.cpp)
bool tests_read_only_exposes_no_mutator();
bool tests_read_only_seal_holds_for_every_container();
bool tests_read_only_subscript_returns_const_reference();
bool tests_read_only_front_and_back_return_const();
bool tests_read_only_data_returns_pointer_to_const();
bool tests_read_only_iterators_are_const_iterators();
bool tests_read_only_never_yields_the_container();
bool tests_read_only_seal_is_independent_of_container();
bool tests_read_only_value_type_is_not_itself_const();

// V.   ADAPTIVITY   (read_only_container_tests_adaptivity.cpp)
bool tests_read_only_observers_mirror_the_container();
bool tests_read_only_omits_subscript_for_list();
bool tests_read_only_omits_data_for_node_containers();
bool tests_read_only_supports_array_and_string();
bool tests_read_only_is_usable_in_constant_expressions();
bool tests_read_only_detection_battery_is_sound();

// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// read_only_container_spec
//   function: the suite's authoritative description - one block per section
// TU, one row per test body, each carrying the descriptor the report renders.
inline dt::module_spec
read_only_container_spec()
{
    return dt::module_spec{
        "read_only_container",
        "The read_only access overlay: a handle that holds its container "
        "privately and forwards only the const observation surface, so there "
        "is no path through it by which the held container can change.",
        {
            dt::block_spec{
                "construction",
                "Populating the handle: the default, wrapping and emplacing "
                "constructors, the guard that keeps the variadic form from "
                "hijacking a copy, and the deducing factory.",
                {
                    { "tests_read_only_default_construction",
                      "a default-constructed handle holds a "
                      "default-constructed container",
                      &tests_read_only_default_construction },
                    { "tests_read_only_wraps_a_container_value",
                      "a single container argument is copied or moved into "
                      "the handle intact",
                      &tests_read_only_wraps_a_container_value },
                    { "tests_read_only_emplaces_from_arguments",
                      "several arguments construct the held container in "
                      "place, forwarding each one",
                      &tests_read_only_emplaces_from_arguments },
                    { "tests_read_only_copy_and_move_construction",
                      "a handle can be copied and moved, and the copy "
                      "observes the same elements",
                      &tests_read_only_copy_and_move_construction },
                    { "tests_read_only_variadic_ctor_excludes_self",
                      "the enable_if guard keeps the variadic constructor "
                      "from being chosen over the copy constructor",
                      &tests_read_only_variadic_ctor_excludes_self },
                    { "tests_read_only_factory_deduces_container",
                      "make_read_only deduces the container type and strips "
                      "reference and cv qualification from it",
                      &tests_read_only_factory_deduces_container },
                    { "tests_read_only_holds_an_independent_copy",
                      "the handle owns its container by value, so a later "
                      "change to the source is not observed through it",
                      &tests_read_only_holds_an_independent_copy },
                }
            },
            dt::block_spec{
                "traits",
                "The three published typedefs and the capability tag that "
                "identifies this overlay to generic code.",
                {
                    { "tests_read_only_exposes_container_type",
                      "container_type names the held container exactly",
                      &tests_read_only_exposes_container_type },
                    { "tests_read_only_exposes_value_type",
                      "value_type is taken from the held container rather "
                      "than restated",
                      &tests_read_only_exposes_value_type },
                    { "tests_read_only_capability_is_read_only",
                      "the capability typedef names the read_only tag, not a "
                      "sibling overlay",
                      &tests_read_only_capability_is_read_only },
                    { "tests_read_only_capability_tag_semantics",
                      "the tag reports readable, not writable, and names "
                      "itself for diagnostics",
                      &tests_read_only_capability_tag_semantics },
                    { "tests_read_only_typedefs_track_container",
                      "the typedefs follow whatever container is wrapped, "
                      "across several element and container types",
                      &tests_read_only_typedefs_track_container },
                }
            },
            dt::block_spec{
                "observation",
                "The const surface that is the whole point of the overlay: "
                "size, element access, storage address and iteration, each "
                "forwarding the held container faithfully.",
                {
                    { "tests_read_only_size_and_empty",
                      "size and empty report the held container's own state",
                      &tests_read_only_size_and_empty },
                    { "tests_read_only_subscript_reads_elements",
                      "subscript reaches every element, including the first "
                      "and the last",
                      &tests_read_only_subscript_reads_elements },
                    { "tests_read_only_front_and_back",
                      "front and back name the end elements and agree with "
                      "the corresponding subscripts",
                      &tests_read_only_front_and_back },
                    { "tests_read_only_data_addresses_storage",
                      "data addresses the held container's own storage, not "
                      "a copy of it",
                      &tests_read_only_data_addresses_storage },
                    { "tests_read_only_iteration_visits_elements",
                      "begin and end span exactly the held elements, in "
                      "order",
                      &tests_read_only_iteration_visits_elements },
                    { "tests_read_only_cbegin_cend_match_begin_end",
                      "the explicitly const iterators describe the same range "
                      "as the ordinary ones",
                      &tests_read_only_cbegin_cend_match_begin_end },
                    { "tests_read_only_observers_work_on_const_handle",
                      "every observer is callable on a const handle, since "
                      "all of them are const members",
                      &tests_read_only_observers_work_on_const_handle },
                    { "tests_read_only_observes_an_empty_container",
                      "an empty container reports empty and yields an empty "
                      "iteration range without a spurious failure",
                      &tests_read_only_observes_an_empty_container },
                    { "tests_read_only_forwards_element_values_faithfully",
                      "values pass through unchanged, including negative and "
                      "extreme ones",
                      &tests_read_only_forwards_element_values_faithfully },
                }
            },
            dt::block_spec{
                "seal",
                "The negative claim, tested in both of its halves: the "
                "mutating members are absent outright, and the observers that "
                "do exist hand back const-qualified references and pointers.",
                {
                    { "tests_read_only_exposes_no_mutator",
                      "not one of the ten probed mutating members is present "
                      "on the handle",
                      &tests_read_only_exposes_no_mutator },
                    { "tests_read_only_seal_holds_for_every_container",
                      "the absence holds whatever container is wrapped, "
                      "including ones that offer every mutator",
                      &tests_read_only_seal_holds_for_every_container },
                    { "tests_read_only_subscript_returns_const_reference",
                      "subscript yields a reference to const even on a "
                      "non-const handle, so nothing can be written through it",
                      &tests_read_only_subscript_returns_const_reference },
                    { "tests_read_only_front_and_back_return_const",
                      "the end-element accessors are const-qualified in the "
                      "same way",
                      &tests_read_only_front_and_back_return_const },
                    { "tests_read_only_data_returns_pointer_to_const",
                      "data yields a pointer to const, closing the direct "
                      "route into the storage",
                      &tests_read_only_data_returns_pointer_to_const },
                    { "tests_read_only_iterators_are_const_iterators",
                      "iteration yields const_iterator, closing the route "
                      "through a dereferenced iterator",
                      &tests_read_only_iterators_are_const_iterators },
                    { "tests_read_only_never_yields_the_container",
                      "no member hands back the held container itself, by "
                      "reference or by pointer",
                      &tests_read_only_never_yields_the_container },
                    { "tests_read_only_seal_is_independent_of_container",
                      "the overlay restricts the handle regardless of what "
                      "the underlying type would permit",
                      &tests_read_only_seal_is_independent_of_container },
                    { "tests_read_only_value_type_is_not_itself_const",
                      "the seal lives in the returned references, not in the "
                      "element type, which stays unqualified",
                      &tests_read_only_value_type_is_not_itself_const },
                }
            },
            dt::block_spec{
                "adaptivity",
                "The SFINAE-guarded forwarders: a member materialises only "
                "when the held container really has it, so the handle neither "
                "invents nor loses an observer.",
                {
                    { "tests_read_only_observers_mirror_the_container",
                      "the handle's observing surface matches the held "
                      "container's, member for member",
                      &tests_read_only_observers_mirror_the_container },
                    { "tests_read_only_omits_subscript_for_list",
                      "wrapping a container without random access does not "
                      "conjure a subscript operator",
                      &tests_read_only_omits_subscript_for_list },
                    { "tests_read_only_omits_data_for_node_containers",
                      "data is absent for containers with no contiguous "
                      "storage to address",
                      &tests_read_only_omits_data_for_node_containers },
                    { "tests_read_only_supports_array_and_string",
                      "fixed-size and character containers wrap as readily as "
                      "a vector does",
                      &tests_read_only_supports_array_and_string },
                    { "tests_read_only_is_usable_in_constant_expressions",
                      "the constexpr forwarders really are usable at compile "
                      "time when the held container permits it",
                      &tests_read_only_is_usable_in_constant_expressions },
                    { "tests_read_only_detection_battery_is_sound",
                      "the suite's own detectors report correctly on raw "
                      "containers, so a false negative cannot mask a leak",
                      &tests_read_only_detection_battery_is_sound },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_READ_ONLY_CONTAINER_TESTS_
