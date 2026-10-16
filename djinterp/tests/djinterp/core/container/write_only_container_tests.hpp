/*******************************************************************************
* djinterp [tests]                                write_only_container_tests.hpp
*
*   Suite header for write_only_container.hpp - the append-only sink, the
* strictest member of the access trio.  Declares every test body, supplies the
* fixtures the bodies need, and (in DTEST_SPEC_MODE) exposes the spec provider
* the runner hands to run_module.
*
*   BUILD PREREQUISITE - THE MODULE'S INCLUDE PATHS DO NOT RESOLVE AS WRITTEN.
*   write_only_container.hpp sits at core/container/access/ and includes:
*
*       #include "../djinterp.hpp"          -> core/container/djinterp.hpp
*       #include "../meta/write_only.hpp"
*                                  -> core/container/meta/write_only.hpp
*
*   Neither target is where the named header ships.  The capability tag
* `write_only` ships at core/container/access/write_only.hpp - a SIBLING of the
* wrapper, per its own banner - so the include should read "write_only.hpp".
* And djinterp.hpp ships at core/djinterp.hpp, which from access/ is
* "../../djinterp.hpp".  All three access wrappers share both defects.
*
*   Worth noting alongside it: the capability TAG headers are themselves
* inconsistent on the second path.  read_only.hpp and read_write.hpp use
* "../../djinterp.hpp", which resolves correctly; write_only.hpp uses
* "../djinterp.hpp", which does not.  The harness supplies forwarding shims at
* the as-written locations so every header compiles unchanged; the fix belongs
* in the include blocks, not here.
*
*   WHAT THIS OVERLAY CLAIMS, AND HOW IT IS TESTED.
*   write_only forwards push_back, emplace_back and clear, plus size and empty
* - and nothing else.  The claim is that a holder may put data in and reset,
* but never read it back out.  That is a claim about ABSENCE, so it needs
* compile-time detection rather than a runtime call, and the suite carries a
* battery of SFINAE detectors rolled into one wo_caps row per type.
*
*   The seal here is stronger than read_only's and is tested differently.
* read_only keeps the observers and makes them const; write_only removes them
* outright.  So the assertions are pure absence: no operator[], no front, no
* back, no data, no begin or end, no cbegin or cend - and no position-dependent
* edit either, since insert, erase, pop_back, pop_front and resize would all
* require observing the contents first.  size and empty survive because they
* reveal how MANY elements are present without revealing any element's value;
* the suite pins that distinction explicitly.
*
*   PORTABILITY:
*   The module targets C++11.  size and empty are D_CONSTEXPR; the append
* operations are constexpr only where C++14 relaxed constexpr permits a
* non-const member to be one.  Built at C++20 by the DTest CMake helper.
*
*
* path:      /tests/djinterp/core/container/write_only_container_tests.hpp
*                write_only_container_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                              created: 2026.07.23
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONSTRUCTION  (write_only_container_tests_construction.cpp)
      -----------------------------------------------------------

II.   TRAITS        (write_only_container_tests_traits.cpp)
      -----------------------------------------------------

III.  METADATA      (write_only_container_tests_metadata.cpp)
      -------------------------------------------------------

IV.   APPEND        (write_only_container_tests_append.cpp)
      -----------------------------------------------------

V.    SEAL          (write_only_container_tests_seal.cpp)
      ---------------------------------------------------
*/

#ifndef DJINTERP_CORE_CONTAINER_WRITE_ONLY_CONTAINER_TESTS_
#define DJINTERP_CORE_CONTAINER_WRITE_ONLY_CONTAINER_TESTS_ 1

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
#include "../../../../inc/djinterp/core/container/write_only_container.hpp"
// the sibling overlays, for the contrast tests that show what this one seals
#include "../../../../inc/djinterp/core/container/read_only_container.hpp"
#include "../../../../inc/djinterp/core/container/read_write_container.hpp"
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


// write_only_check
//   function: reports a failed assertion and returns the condition, so the
// D_WO_CHECK macro can early-return on the first failure.
//
// Parameter(s):
//   _cond: the asserted condition.
//   _expr: the stringized expression.
//   _file: the source file of the assertion.
//   _line: the source line of the assertion.
// Return:
//   _cond, unchanged.
inline bool
write_only_check(
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

// D_WO_CHECK
//   macro: assert _cond, printing the expression and its location and
// returning false from the enclosing test body on failure.  Variadic so a
// top-level comma inside the expression passes through whole.
#define D_WO_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::write_only_check(                           \
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


// wo_void_help
//   type: maps any well-formed type list to void, the substrate every
// detector below is built on.  Spelled locally rather than using
// std::void_t, which is C++17.
template<typename...>
struct wo_void_impl
{
    typedef void type;
};

template<typename... _T>
struct wo_void_help
{
    typedef typename wo_void_impl<_T...>::type type;
};

#define D_WO_VOID(...)                                                    \
    typename ::djinterp::testing::wo_void_help<__VA_ARGS__>::type

// D_WO_DETECT
//   macro: defines a trait _name<T> that is true exactly when _expr is
// well-formed for T.
#define D_WO_DETECT(_name, _expr)                                             \
    template<typename _T, typename = void>                                    \
    struct _name : std::false_type {};                                        \
    template<typename _T>                                                     \
    struct _name<_T, D_WO_VOID(decltype(_expr))> : std::true_type {}

// -- value-free metadata, which must be PRESENT ----------------------------
D_WO_DETECT(wo_has_size,   std::declval<const _T&>().size());
D_WO_DETECT(wo_has_empty,  std::declval<const _T&>().empty());

// -- the reading surface, which must be ABSENT -----------------------------
D_WO_DETECT(wo_has_sub,    std::declval<_T&>()[0]);
D_WO_DETECT(wo_has_front,  std::declval<_T&>().front());
D_WO_DETECT(wo_has_back,   std::declval<_T&>().back());
D_WO_DETECT(wo_has_data,   std::declval<_T&>().data());
D_WO_DETECT(wo_has_begin,  std::declval<_T&>().begin());
D_WO_DETECT(wo_has_end,    std::declval<_T&>().end());
D_WO_DETECT(wo_has_cbegin, std::declval<const _T&>().cbegin());
D_WO_DETECT(wo_has_cend,   std::declval<const _T&>().cend());

// -- the append surface, which must be PRESENT where the container has it --
D_WO_DETECT(wo_has_push_back,
    std::declval<_T&>().push_back(std::declval<typename _T::value_type>()));
D_WO_DETECT(wo_has_emplace_back,
    std::declval<_T&>().emplace_back(std::declval<typename _T::value_type>()));
D_WO_DETECT(wo_has_clear,     std::declval<_T&>().clear());

// -- position-dependent edits, which must be ABSENT ------------------------
D_WO_DETECT(wo_has_push_front,
    std::declval<_T&>().push_front(std::declval<typename _T::value_type>()));
D_WO_DETECT(wo_has_pop_back,  std::declval<_T&>().pop_back());
D_WO_DETECT(wo_has_pop_front, std::declval<_T&>().pop_front());
D_WO_DETECT(wo_has_resize,    std::declval<_T&>().resize(std::size_t(1)));
D_WO_DETECT(wo_has_reserve,   std::declval<_T&>().reserve(std::size_t(1)));
D_WO_DETECT(wo_has_insert,
    std::declval<_T&>().insert(std::declval<_T&>().cbegin(),
                               std::declval<typename _T::value_type>()));
D_WO_DETECT(wo_has_erase,
    std::declval<_T&>().erase(std::declval<_T&>().cbegin()));


// wo_caps
//   struct: one row of the detection matrix - every member the suite probes,
// for a single type.
struct wo_caps
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
    bool emplace_back;
    bool clear;

    bool push_front;
    bool pop_back;
    bool pop_front;
    bool resize;
    bool reserve;
    bool insert;
    bool erase;
};

// wo_probe
//   function: the detection row for _T.
template<typename _T>
inline wo_caps
wo_probe()
{
    wo_caps c;

    c.size  = wo_has_size<_T>::value;
    c.empty = wo_has_empty<_T>::value;

    c.subscript = wo_has_sub<_T>::value;
    c.front     = wo_has_front<_T>::value;
    c.back      = wo_has_back<_T>::value;
    c.data      = wo_has_data<_T>::value;
    c.begin     = wo_has_begin<_T>::value;
    c.end       = wo_has_end<_T>::value;
    c.cbegin    = wo_has_cbegin<_T>::value;
    c.cend      = wo_has_cend<_T>::value;

    c.push_back    = wo_has_push_back<_T>::value;
    c.emplace_back = wo_has_emplace_back<_T>::value;
    c.clear        = wo_has_clear<_T>::value;

    c.push_front = wo_has_push_front<_T>::value;
    c.pop_back   = wo_has_pop_back<_T>::value;
    c.pop_front  = wo_has_pop_front<_T>::value;
    c.resize     = wo_has_resize<_T>::value;
    c.reserve    = wo_has_reserve<_T>::value;
    c.insert     = wo_has_insert<_T>::value;
    c.erase      = wo_has_erase<_T>::value;

    return c;
}

// wo_any_reader
//   function: whether _c reports ANY member through which an element's value
// could be observed.  The overlay's central claim reduces to this being
// false; size and empty are excluded because they reveal a count, not a
// value.
inline bool
wo_any_reader(
    const wo_caps& _c
)
{
    return ( _c.subscript || _c.front  || _c.back   || _c.data ||
             _c.begin     || _c.end    || _c.cbegin || _c.cend );
}

// wo_any_positional_edit
//   function: whether _c reports any edit that depends on a position within
// the container.  Each of these would require observing the contents first,
// so the append-only sink forwards none of them.
inline bool
wo_any_positional_edit(
    const wo_caps& _c
)
{
    return ( _c.push_front || _c.pop_back || _c.pop_front ||
             _c.resize     || _c.reserve  || _c.insert    ||
             _c.erase );
}


// wo_probe_type
//   type: a value type with a two-argument constructor, so emplace_back can
// be shown to forward more than one argument.  It counts its own live
// instances, which lets a test observe that an element really was stored
// without reading the element back.
struct wo_probe_type
{
    static int s_live;
    static int s_constructed;

    int  a;
    char b;

    wo_probe_type()
        : a(0),
          b('\0')
    {
        ++s_live;
        ++s_constructed;
    }

    wo_probe_type(
        int  _a,
        char _b
    )
        : a(_a),
          b(_b)
    {
        ++s_live;
        ++s_constructed;
    }

    wo_probe_type(
        const wo_probe_type& _other
    )
        : a(_other.a),
          b(_other.b)
    {
        ++s_live;
        ++s_constructed;
    }

    wo_probe_type&
    operator=(
        const wo_probe_type& _other
    )
    {
        a = _other.a;
        b = _other.b;

        return *this;
    }

    ~wo_probe_type()
    {
        --s_live;
    }
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   CONSTRUCTION   (write_only_container_tests_construction.cpp)
bool tests_write_only_default_construction();
bool tests_write_only_wraps_a_container_value();
bool tests_write_only_emplaces_from_arguments();
bool tests_write_only_copy_and_move_construction();
bool tests_write_only_variadic_ctor_excludes_self();
bool tests_write_only_factory_deduces_container();
bool tests_write_only_holds_an_independent_copy();

// II.  TRAITS   (write_only_container_tests_traits.cpp)
bool tests_write_only_exposes_container_type();
bool tests_write_only_exposes_value_type();
bool tests_write_only_capability_is_write_only();
bool tests_write_only_capability_tag_semantics();
bool tests_write_only_typedefs_track_container();

// III. METADATA   (write_only_container_tests_metadata.cpp)
bool tests_write_only_size_reports_the_count();
bool tests_write_only_empty_agrees_with_size();
bool tests_write_only_metadata_tracks_appends();
bool tests_write_only_metadata_is_value_free();
bool tests_write_only_metadata_is_constexpr();

// IV.  APPEND   (write_only_container_tests_append.cpp)
bool tests_write_only_push_back_appends();
bool tests_write_only_emplace_back_forwards_args();
bool tests_write_only_clear_empties_the_sink();
bool tests_write_only_append_and_reset_cycle();
bool tests_write_only_append_stores_the_element();
bool tests_write_only_append_surface_tracks_container();

// V.   SEAL   (write_only_container_tests_seal.cpp)
bool tests_write_only_exposes_no_reader();
bool tests_write_only_seal_holds_for_every_container();
bool tests_write_only_omits_positional_edits();
bool tests_write_only_never_yields_the_container();
bool tests_write_only_no_member_returns_an_element();
bool tests_write_only_is_stricter_than_read_write();
bool tests_write_only_is_disjoint_from_read_only();
bool tests_write_only_detection_battery_is_sound();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// write_only_container_spec
//   function: the suite's authoritative description - one block per section
// TU, one row per test body, each carrying the descriptor the report renders.
inline dt::module_spec
write_only_container_spec()
{
    return dt::module_spec{
        "write_only_container",
        "The write_only access overlay: an append-only sink that forwards "
        "push_back, emplace_back and clear plus the value-free metadata size "
        "and empty, and nothing else - so a holder may put data in and reset, "
        "but never read it back out.",
        {
            dt::block_spec{
                "construction",
                "Populating the sink: the default, wrapping and emplacing "
                "constructors, the guard that keeps the variadic form from "
                "hijacking a copy, and the deducing factory.",
                {
                    { "tests_write_only_default_construction",
                      "a default-constructed sink holds a "
                      "default-constructed container",
                      &tests_write_only_default_construction },
                    { "tests_write_only_wraps_a_container_value",
                      "a container handed over at construction is held, and "
                      "its element count is reported",
                      &tests_write_only_wraps_a_container_value },
                    { "tests_write_only_emplaces_from_arguments",
                      "several arguments construct the held container in "
                      "place, forwarding each one",
                      &tests_write_only_emplaces_from_arguments },
                    { "tests_write_only_copy_and_move_construction",
                      "a sink can be copied and moved, and the copy is "
                      "independently writable",
                      &tests_write_only_copy_and_move_construction },
                    { "tests_write_only_variadic_ctor_excludes_self",
                      "the enable_if guard keeps the variadic constructor "
                      "from being chosen over the copy constructor",
                      &tests_write_only_variadic_ctor_excludes_self },
                    { "tests_write_only_factory_deduces_container",
                      "make_write_only deduces the container type and strips "
                      "reference and cv qualification from it",
                      &tests_write_only_factory_deduces_container },
                    { "tests_write_only_holds_an_independent_copy",
                      "the sink owns its container by value, so appends do "
                      "not reach the source",
                      &tests_write_only_holds_an_independent_copy },
                }
            },
            dt::block_spec{
                "traits",
                "The three published typedefs and the capability tag that "
                "identifies this overlay to generic code.",
                {
                    { "tests_write_only_exposes_container_type",
                      "container_type names the held container exactly",
                      &tests_write_only_exposes_container_type },
                    { "tests_write_only_exposes_value_type",
                      "value_type is taken from the held container even "
                      "though no member returns one",
                      &tests_write_only_exposes_value_type },
                    { "tests_write_only_capability_is_write_only",
                      "the capability typedef names the write_only tag, not "
                      "a sibling overlay",
                      &tests_write_only_capability_is_write_only },
                    { "tests_write_only_capability_tag_semantics",
                      "the tag reports writable but not readable, the only "
                      "one of the three that does",
                      &tests_write_only_capability_tag_semantics },
                    { "tests_write_only_typedefs_track_container",
                      "the typedefs follow whatever container is wrapped "
                      "while the capability stays fixed",
                      &tests_write_only_typedefs_track_container },
                }
            },
            dt::block_spec{
                "metadata",
                "The two observers that survive: how many elements are "
                "present, without revealing what any of them is.",
                {
                    { "tests_write_only_size_reports_the_count",
                      "size reports the held container's element count",
                      &tests_write_only_size_reports_the_count },
                    { "tests_write_only_empty_agrees_with_size",
                      "empty is true exactly when size is zero",
                      &tests_write_only_empty_agrees_with_size },
                    { "tests_write_only_metadata_tracks_appends",
                      "both observers follow each append and each reset "
                      "immediately",
                      &tests_write_only_metadata_tracks_appends },
                    { "tests_write_only_metadata_is_value_free",
                      "neither observer returns an element or a reference to "
                      "one, which is what makes them safe to keep",
                      &tests_write_only_metadata_is_value_free },
                    { "tests_write_only_metadata_is_constexpr",
                      "size and empty answer in a constant expression when "
                      "the held container permits it",
                      &tests_write_only_metadata_is_constexpr },
                }
            },
            dt::block_spec{
                "append",
                "The only mutating surface: adding at the end and resetting, "
                "with the arguments forwarded intact.",
                {
                    { "tests_write_only_push_back_appends",
                      "push_back adds one element and the reported count "
                      "grows by one",
                      &tests_write_only_push_back_appends },
                    { "tests_write_only_emplace_back_forwards_args",
                      "emplace_back constructs in place, forwarding several "
                      "arguments to the element's constructor",
                      &tests_write_only_emplace_back_forwards_args },
                    { "tests_write_only_clear_empties_the_sink",
                      "clear removes every element and leaves the sink "
                      "reporting empty and reusable",
                      &tests_write_only_clear_empties_the_sink },
                    { "tests_write_only_append_and_reset_cycle",
                      "put-in and reset can be repeated, which is the whole "
                      "usage pattern the overlay exists for",
                      &tests_write_only_append_and_reset_cycle },
                    { "tests_write_only_append_stores_the_element",
                      "the appended element really is constructed and held, "
                      "shown by an element type that counts its instances",
                      &tests_write_only_append_stores_the_element },
                    { "tests_write_only_append_surface_tracks_container",
                      "each append member appears only when the held "
                      "container offers it",
                      &tests_write_only_append_surface_tracks_container },
                }
            },
            dt::block_spec{
                "seal",
                "The central negative claim: no member through which an "
                "element's value could be observed, and no edit that would "
                "require observing one first.",
                {
                    { "tests_write_only_exposes_no_reader",
                      "not one of the eight probed reading members is "
                      "present on the sink",
                      &tests_write_only_exposes_no_reader },
                    { "tests_write_only_seal_holds_for_every_container",
                      "the absence holds whatever container is wrapped, "
                      "including ones offering the full reading surface",
                      &tests_write_only_seal_holds_for_every_container },
                    { "tests_write_only_omits_positional_edits",
                      "insert, erase, pop_back, pop_front, resize and "
                      "reserve are all absent, since each needs a position",
                      &tests_write_only_omits_positional_edits },
                    { "tests_write_only_never_yields_the_container",
                      "no member hands back the held container itself, which "
                      "would return the whole reading surface at a stroke",
                      &tests_write_only_never_yields_the_container },
                    { "tests_write_only_no_member_returns_an_element",
                      "no member's return type is the element type or a "
                      "reference or pointer to it",
                      &tests_write_only_no_member_returns_an_element },
                    { "tests_write_only_is_stricter_than_read_write",
                      "against the same container it keeps strictly less "
                      "than the unrestricted sibling does",
                      &tests_write_only_is_stricter_than_read_write },
                    { "tests_write_only_is_disjoint_from_read_only",
                      "the two restricted overlays seal opposite halves, "
                      "sharing only the value-free metadata",
                      &tests_write_only_is_disjoint_from_read_only },
                    { "tests_write_only_detection_battery_is_sound",
                      "the suite's own detectors report correctly on raw "
                      "containers, so a false negative cannot mask a leak",
                      &tests_write_only_detection_battery_is_sound },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CORE_CONTAINER_WRITE_ONLY_CONTAINER_TESTS_
