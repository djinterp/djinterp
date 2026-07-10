/******************************************************************************
* djinterp [test]                                   byte_size_tests_signals.cpp
*
*   Section II of byte_size.hpp: the internal storage-shape signal detectors
* that decide a container's dynamic footprint.  They are implementation detail
* (djinterp::internal), but each is a small SFINAE trait with a true and a
* false specialization, so pinning both branches of each -- and both operands
* of the two composite helpers -- is the most direct route to full coverage.
*
*   Detectors (true on a matching shape, false on has_nothing and on a library
* container that lacks the signal):
*     byte_has_capacity_helper / _size_helper / _allocator_helper /
*     _reserve_helper / _c_str_helper.
*   Composites:
*     byte_is_dynamically_sited_helper  -- the allocator OR reserve disjunction
*       (alloc-only, reserve-only, both, neither);
*     byte_recurse_into_element_helper  -- is_iterable_container AND NOT c_str
*       (nested container -> true; scalar / c_str leaf / non-iterable -> false).
*   Also: the detectors strip cv / ref via clean_t, so qualified inputs agree.
*
*
* path:      /inc/djinterp/test/byte_size_tests_signals.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

#include "byte_size_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace byte_size_test_types;

namespace bi = ::djinterp::internal;


// =========================================================================
// I.   byte_has_capacity_helper
// =========================================================================

static_assert(bi::byte_has_capacity_helper<has_capacity_only>::value,  "cap: custom yes");
static_assert(bi::byte_has_capacity_helper<std::vector<int>>::value,   "cap: vector yes");
static_assert(bi::byte_has_capacity_helper<std::string>::value,        "cap: string yes");
static_assert(!bi::byte_has_capacity_helper<has_nothing>::value,       "cap: none no");
static_assert(!bi::byte_has_capacity_helper<std::list<int>>::value,    "cap: list no");
static_assert(!bi::byte_has_capacity_helper<std::array<int, 4>>::value,"cap: array no");


// =========================================================================
// II.  byte_has_size_helper
// =========================================================================

static_assert(bi::byte_has_size_helper<has_size_only>::value,          "size: custom yes");
static_assert(bi::byte_has_size_helper<std::list<int>>::value,         "size: list yes");
static_assert(bi::byte_has_size_helper<std::vector<int>>::value,       "size: vector yes");
static_assert(bi::byte_has_size_helper<std::array<int, 4>>::value,     "size: array yes");
static_assert(!bi::byte_has_size_helper<has_nothing>::value,           "size: none no");
static_assert(!bi::byte_has_size_helper<std::forward_list<int>>::value,"size: forward_list NO");


// =========================================================================
// III. byte_has_allocator_helper
// =========================================================================

static_assert(bi::byte_has_allocator_helper<has_allocator_only>::value,"alloc: custom yes");
static_assert(bi::byte_has_allocator_helper<std::vector<int>>::value,  "alloc: vector yes");
static_assert(bi::byte_has_allocator_helper<std::list<int>>::value,    "alloc: list yes");
static_assert(bi::byte_has_allocator_helper<std::string>::value,       "alloc: string yes");
static_assert(!bi::byte_has_allocator_helper<has_nothing>::value,      "alloc: none no");
static_assert(!bi::byte_has_allocator_helper<std::array<int, 4>>::value,"alloc: array no");
static_assert(!bi::byte_has_allocator_helper<int>::value,              "alloc: scalar no");


// =========================================================================
// IV.  byte_has_reserve_helper
// =========================================================================

static_assert(bi::byte_has_reserve_helper<has_reserve_only>::value,    "reserve: custom yes");
static_assert(bi::byte_has_reserve_helper<std::vector<int>>::value,    "reserve: vector yes");
static_assert(bi::byte_has_reserve_helper<std::string>::value,         "reserve: string yes");
static_assert(!bi::byte_has_reserve_helper<has_nothing>::value,        "reserve: none no");
static_assert(!bi::byte_has_reserve_helper<std::list<int>>::value,     "reserve: list no");
static_assert(!bi::byte_has_reserve_helper<std::array<int, 4>>::value, "reserve: array no");


// =========================================================================
// V.   byte_has_c_str_helper
// =========================================================================

static_assert(bi::byte_has_c_str_helper<has_c_str_only>::value,        "c_str: custom yes");
static_assert(bi::byte_has_c_str_helper<std::string>::value,           "c_str: string yes");
static_assert(!bi::byte_has_c_str_helper<has_nothing>::value,          "c_str: none no");
static_assert(!bi::byte_has_c_str_helper<std::vector<char>>::value,    "c_str: vector<char> no");
static_assert(!bi::byte_has_c_str_helper<std::vector<int>>::value,     "c_str: vector<int> no");


// =========================================================================
// VI.  byte_is_dynamically_sited_helper  (allocator OR reserve)
// =========================================================================

static_assert(bi::byte_is_dynamically_sited_helper<sited_alloc_only>::value,
              "sited: allocator alone -> true");
static_assert(bi::byte_is_dynamically_sited_helper<sited_reserve_only>::value,
              "sited: reserve alone -> true");
static_assert(bi::byte_is_dynamically_sited_helper<sited_both>::value,
              "sited: both -> true");
static_assert(!bi::byte_is_dynamically_sited_helper<has_nothing>::value,
              "sited: neither -> false");
static_assert(bi::byte_is_dynamically_sited_helper<std::vector<int>>::value,
              "sited: vector -> true");
static_assert(bi::byte_is_dynamically_sited_helper<std::list<int>>::value,
              "sited: list (allocator) -> true");
static_assert(!bi::byte_is_dynamically_sited_helper<std::array<int, 4>>::value,
              "sited: array -> false");


// =========================================================================
// VII. byte_recurse_into_element_helper  (iterable AND NOT c_str)
// =========================================================================

static_assert(bi::byte_recurse_into_element_helper<std::vector<int>>::value,
              "recurse: nested container element -> true");
static_assert(bi::byte_recurse_into_element_helper<std::list<int>>::value,
              "recurse: list element -> true");
static_assert(bi::byte_recurse_into_element_helper<inline_bag<int, 3>>::value,
              "recurse: inline_bag element -> true");
static_assert(!bi::byte_recurse_into_element_helper<int>::value,
              "recurse: scalar -> false (not iterable)");
static_assert(!bi::byte_recurse_into_element_helper<std::string>::value,
              "recurse: string -> false (c_str leaf)");
static_assert(!bi::byte_recurse_into_element_helper<has_nothing>::value,
              "recurse: non-iterable struct -> false");


// =========================================================================
// VIII. clean_t stripping inside the detectors
// =========================================================================

static_assert(bi::byte_has_capacity_helper<const has_capacity_only&>::value,
              "clean: capacity through const-ref");
static_assert(bi::byte_has_c_str_helper<const std::string&>::value,
              "clean: c_str through const-ref");
static_assert(bi::byte_is_dynamically_sited_helper<volatile std::vector<int>&>::value,
              "clean: sited through volatile-ref");
static_assert(!bi::byte_recurse_into_element_helper<const std::string&>::value,
              "clean: recurse gate through const-ref (string still a leaf)");


// =========================================================================
// IX.  RUN-TIME MIRRORS
// =========================================================================

bool
tests_byte_size_signal_capacity()
{
    bool _ok = true;
    _ok = _ok && bi::byte_has_capacity_helper<has_capacity_only>::value;
    _ok = _ok && bi::byte_has_capacity_helper<std::vector<int>>::value;
    _ok = _ok && !bi::byte_has_capacity_helper<has_nothing>::value;
    _ok = _ok && !bi::byte_has_capacity_helper<std::list<int>>::value;
    return _ok;
}

bool
tests_byte_size_signal_size()
{
    bool _ok = true;
    _ok = _ok && bi::byte_has_size_helper<has_size_only>::value;
    _ok = _ok && bi::byte_has_size_helper<std::list<int>>::value;
    _ok = _ok && !bi::byte_has_size_helper<has_nothing>::value;
    _ok = _ok && !bi::byte_has_size_helper<std::forward_list<int>>::value;
    return _ok;
}

bool
tests_byte_size_signal_allocator()
{
    bool _ok = true;
    _ok = _ok && bi::byte_has_allocator_helper<has_allocator_only>::value;
    _ok = _ok && bi::byte_has_allocator_helper<std::vector<int>>::value;
    _ok = _ok && !bi::byte_has_allocator_helper<has_nothing>::value;
    _ok = _ok && !bi::byte_has_allocator_helper<std::array<int, 4>>::value;
    return _ok;
}

bool
tests_byte_size_signal_reserve()
{
    bool _ok = true;
    _ok = _ok && bi::byte_has_reserve_helper<has_reserve_only>::value;
    _ok = _ok && bi::byte_has_reserve_helper<std::vector<int>>::value;
    _ok = _ok && !bi::byte_has_reserve_helper<has_nothing>::value;
    _ok = _ok && !bi::byte_has_reserve_helper<std::list<int>>::value;
    return _ok;
}

bool
tests_byte_size_signal_c_str()
{
    bool _ok = true;
    _ok = _ok && bi::byte_has_c_str_helper<has_c_str_only>::value;
    _ok = _ok && bi::byte_has_c_str_helper<std::string>::value;
    _ok = _ok && !bi::byte_has_c_str_helper<has_nothing>::value;
    _ok = _ok && !bi::byte_has_c_str_helper<std::vector<char>>::value;
    return _ok;
}

bool
tests_byte_size_signal_dynamically_sited()
{
    bool _ok = true;
    _ok = _ok && bi::byte_is_dynamically_sited_helper<sited_alloc_only>::value;
    _ok = _ok && bi::byte_is_dynamically_sited_helper<sited_reserve_only>::value;
    _ok = _ok && bi::byte_is_dynamically_sited_helper<sited_both>::value;
    _ok = _ok && !bi::byte_is_dynamically_sited_helper<has_nothing>::value;
    _ok = _ok && bi::byte_is_dynamically_sited_helper<std::vector<int>>::value;
    _ok = _ok && !bi::byte_is_dynamically_sited_helper<std::array<int, 4>>::value;
    return _ok;
}

bool
tests_byte_size_signal_recurse_into_element()
{
    bool _ok = true;
    _ok = _ok && bi::byte_recurse_into_element_helper<std::vector<int>>::value;
    _ok = _ok && bi::byte_recurse_into_element_helper<inline_bag<int, 3>>::value;
    _ok = _ok && !bi::byte_recurse_into_element_helper<int>::value;
    _ok = _ok && !bi::byte_recurse_into_element_helper<std::string>::value;
    _ok = _ok && !bi::byte_recurse_into_element_helper<has_nothing>::value;
    return _ok;
}

bool
tests_byte_size_signal_cleans()
{
    bool _ok = true;
    _ok = _ok && bi::byte_has_capacity_helper<const has_capacity_only&>::value;
    _ok = _ok && bi::byte_has_c_str_helper<const std::string&>::value;
    _ok = _ok && bi::byte_is_dynamically_sited_helper<volatile std::vector<int>&>::value;
    _ok = _ok && !bi::byte_recurse_into_element_helper<const std::string&>::value;
    return _ok;
}


NS_END  // testing
NS_END  // djinterp
