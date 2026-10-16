/*******************************************************************************
* djinterp [test]                                        sort_dispatch_tests.hpp
*
*   unit-test suite for sort_dispatch.hpp.
* Declares every tests_sort_dispatch_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     test_sort_dispatch_selector.cpp   sort_algorithm -- the enumerators and
*                                       the pinned underlying type
*     test_sort_dispatch_tags.cpp       the six algorithm tags, is_sort_tag,
*                                       and default_sort_tag
*     test_sort_dispatch_bridge.cpp     sort_algorithm_traits, algorithm_tag_t,
*                                       properties_of, name_of -- including the
*                                       unrecognised enumerator
*     test_sort_dispatch_runtime.cpp    internal::dispatch_runtime -- every
*                                       case of the switch plus its fall-through
*     test_sort_dispatch_sorter.cpp     sorter<tag> and the
*                                       sorter<dynamic_algorithm> specialization
*     test_sort_dispatch_entry.cpp      the six free sort() overloads
*
*   The tag surface is the subsystem's real single source of truth for the
* per-algorithm properties: only merge_sort.hpp and heap_sort.hpp additionally
* publish a *_sort_traits struct, so the cross-checks that keep the two in
* agreement live in those two suites.
*
*
* path:      /tests/djinterp/core/util/sort/sort_dispatch_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_DISPATCH_TESTS_
#define DJINTERP_TESTS_SORT_DISPATCH_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
// djinterp [test]
#include "sort_support.hpp"
#endif  // !DTEST_SPEC_MODE
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // module_spec + run_module
#endif  // DTEST_SPEC_MODE


NS_DJINTERP

//   djinterp::test has to be NAMEABLE in both faces: normal mode does not pull
// in test_defaults.hpp, so the namespace is declared here before it is aliased.
NS_TEST
NS_END  // test

NS_TESTING

//   dt names the entities under test (djinterp::test).  Declared
// UNCONDITIONALLY: the spec provider below needs dt::module_spec too.
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE   // fixtures -- normal mode only


///////////////////////////////////////////////////////////////////////////////
///                    I.   ADAPTERS                                        ///
///////////////////////////////////////////////////////////////////////////////

// names_equal
//   helper: string comparison for the name_of / tag::name checks.
inline
bool names_equal(
    const char* _a,
    const char* _b
)
{
    if ( (_a == nullptr) ||
         (_b == nullptr) )
    {
        return (_a == _b);
    }

    return (std::strcmp(_a, _b) == 0);
}

// unrecognised_algorithm
//   helper: a sort_algorithm value outside the six enumerators.  The switch
// statements in this header carry no default label, so this is the input that
// exercises their fall-through: dispatch_runtime must leave the range
// untouched, properties_of must report all-false, and name_of must answer
// "unknown".
inline
sort_algorithm unrecognised_algorithm()
{
    return static_cast<sort_algorithm>(99);
}

// tag_apply_ints
//   helper: drives one algorithm tag's uniform apply over vector<int>.
template<typename _Tag>
void tag_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    _Tag::apply(_first, _last, less<int>());

    return;
}

// runtime_dispatch_ints
//   helper: drives internal::dispatch_runtime for a compile-time-known
// enumerator, so a plain function pointer can be handed to the battery.
template<sort_algorithm _Algorithm>
void runtime_dispatch_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::dispatch_runtime(_Algorithm,
                                           _first,
                                           _last,
                                           less<int>());

    return;
}

// static_sorter_ints
//   helper: drives sorter<_Tag> through its two-argument call operator.
template<typename _Tag>
void static_sorter_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    sorter<_Tag> instance;

    instance(_first, _last);

    return;
}

// free_sort_ints
//   helper: drives the compile-time free sort<_Tag>(first, last) overload.
template<typename _Tag>
void free_sort_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    sort<_Tag>(_first, _last);

    return;
}

// free_sort_ints_raw
//   helper: the compile-time free overload over raw pointers.
template<typename _Tag>
void free_sort_ints_raw(
    int* _first,
    int* _last
)
{
    sort<_Tag>(_first, _last);

    return;
}

// dynamic_sorter_adapter
//   class: presents sorter<dynamic_algorithm> as a two-argument callable, so
// a run-time-selected algorithm can be driven over the standard battery.  A
// functor rather than a function template because the selector is a value.
class dynamic_sorter_adapter
{
public:
    // dynamic_sorter_adapter
    //   ctor: binds the wrapped sorter to _algorithm.
    explicit dynamic_sorter_adapter(
        sort_algorithm _algorithm
    )
        : m_sorter(_algorithm)
    {
    }

    // operator()
    //   function: sorts [_first, _last) through the wrapped sorter.
    template<typename _Iterator>
    void operator()(
        _Iterator _first,
        _Iterator _last
    ) const
    {
        m_sorter(_first, _last);

        return;
    }

private:
    sorter<dynamic_algorithm> m_sorter;
};

// free_runtime_sort_adapter
//   class: presents the run-time free sort(algorithm, first, last) overload
// as a two-argument callable.
class free_runtime_sort_adapter
{
public:
    // free_runtime_sort_adapter
    //   ctor: binds the adapter to _algorithm.
    explicit free_runtime_sort_adapter(
        sort_algorithm _algorithm
    )
        : m_algorithm(_algorithm)
    {
    }

    // operator()
    //   function: sorts [_first, _last) through the free overload.
    template<typename _Iterator>
    void operator()(
        _Iterator _first,
        _Iterator _last
    ) const
    {
        sort(m_algorithm, _first, _last);

        return;
    }

private:
    sort_algorithm m_algorithm;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// selector -- sort_algorithm
bool tests_sort_dispatch_algorithm_enumerators();
bool tests_sort_dispatch_algorithm_underlying_type();

// tags -- the six algorithm tags
bool tests_sort_dispatch_tag_properties();
bool tests_sort_dispatch_tag_names();
bool tests_sort_dispatch_tag_values();
bool tests_sort_dispatch_tag_apply();
bool tests_sort_dispatch_is_sort_tag();
bool tests_sort_dispatch_default_tag();

// bridge -- sort_algorithm_traits / properties_of / name_of
bool tests_sort_dispatch_algorithm_traits();
bool tests_sort_dispatch_algorithm_tag_alias();
bool tests_sort_dispatch_properties_of();
bool tests_sort_dispatch_name_of();
bool tests_sort_dispatch_unrecognised_enumerator();

// runtime -- internal::dispatch_runtime
bool tests_sort_dispatch_runtime_every_case();
bool tests_sort_dispatch_runtime_unrecognised_is_noop();
bool tests_sort_dispatch_runtime_battery();

// sorter -- sorter<tag> / sorter<dynamic_algorithm>
bool tests_sort_dispatch_sorter_static_surface();
bool tests_sort_dispatch_sorter_static_calls();
bool tests_sort_dispatch_sorter_dynamic_construction();
bool tests_sort_dispatch_sorter_dynamic_rebinding();
bool tests_sort_dispatch_sorter_dynamic_properties();
bool tests_sort_dispatch_sorter_ordered_overloads();

// entry -- the six free sort() overloads
bool tests_sort_dispatch_entry_compile_time_forms();
bool tests_sort_dispatch_entry_runtime_forms();
bool tests_sort_dispatch_entry_default_tag();
bool tests_sort_dispatch_entry_battery();
bool tests_sort_dispatch_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// sort_dispatch_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_dispatch_tests_runner.cpp.
inline dt::module_spec
sort_dispatch_spec()
{
    return dt::module_spec
    {
        "sort_dispatch.hpp",
        "the algorithm-agnostic dispatch facility: the sort_algorithm "
        "selector, the six tags, the traits bridge between the compile-time "
        "and run-time surfaces, the sorter functor in both its forms, and "
        "the six free sort() entry points.",
        {
            dt::block_spec
            {
                "selector",
                "sort_algorithm",
                {
                    {
                        "sort_dispatch_algorithm_enumerators",
                        "algorithm enumerators",
                        &tests_sort_dispatch_algorithm_enumerators
                    },
                    {
                        "sort_dispatch_algorithm_underlying_type",
                        "algorithm underlying type",
                        &tests_sort_dispatch_algorithm_underlying_type
                    }
                }
            },
            dt::block_spec
            {
                "tags",
                "the six algorithm tags",
                {
                    {
                        "sort_dispatch_tag_properties",
                        "tag properties",
                        &tests_sort_dispatch_tag_properties
                    },
                    {
                        "sort_dispatch_tag_names",
                        "tag names",
                        &tests_sort_dispatch_tag_names
                    },
                    {
                        "sort_dispatch_tag_values",
                        "tag values",
                        &tests_sort_dispatch_tag_values
                    },
                    {
                        "sort_dispatch_tag_apply",
                        "tag apply",
                        &tests_sort_dispatch_tag_apply
                    },
                    {
                        "sort_dispatch_is_sort_tag",
                        "is sort tag",
                        &tests_sort_dispatch_is_sort_tag
                    },
                    {
                        "sort_dispatch_default_tag",
                        "default tag",
                        &tests_sort_dispatch_default_tag
                    }
                }
            },
            dt::block_spec
            {
                "bridge",
                "sort_algorithm_traits / properties_of / name_of",
                {
                    {
                        "sort_dispatch_algorithm_traits",
                        "algorithm traits",
                        &tests_sort_dispatch_algorithm_traits
                    },
                    {
                        "sort_dispatch_algorithm_tag_alias",
                        "algorithm tag alias",
                        &tests_sort_dispatch_algorithm_tag_alias
                    },
                    {
                        "sort_dispatch_properties_of",
                        "properties of",
                        &tests_sort_dispatch_properties_of
                    },
                    {
                        "sort_dispatch_name_of",
                        "name of",
                        &tests_sort_dispatch_name_of
                    },
                    {
                        "sort_dispatch_unrecognised_enumerator",
                        "unrecognised enumerator",
                        &tests_sort_dispatch_unrecognised_enumerator
                    }
                }
            },
            dt::block_spec
            {
                "runtime",
                "internal::dispatch_runtime",
                {
                    {
                        "sort_dispatch_runtime_every_case",
                        "every case",
                        &tests_sort_dispatch_runtime_every_case
                    },
                    {
                        "sort_dispatch_runtime_unrecognised_is_noop",
                        "unrecognised is noop",
                        &tests_sort_dispatch_runtime_unrecognised_is_noop
                    },
                    {
                        "sort_dispatch_runtime_battery",
                        "battery",
                        &tests_sort_dispatch_runtime_battery
                    }
                }
            },
            dt::block_spec
            {
                "sorter",
                "sorter<tag> / sorter<dynamic_algorithm>",
                {
                    {
                        "sort_dispatch_sorter_static_surface",
                        "static surface",
                        &tests_sort_dispatch_sorter_static_surface
                    },
                    {
                        "sort_dispatch_sorter_static_calls",
                        "static calls",
                        &tests_sort_dispatch_sorter_static_calls
                    },
                    {
                        "sort_dispatch_sorter_dynamic_construction",
                        "dynamic construction",
                        &tests_sort_dispatch_sorter_dynamic_construction
                    },
                    {
                        "sort_dispatch_sorter_dynamic_rebinding",
                        "dynamic rebinding",
                        &tests_sort_dispatch_sorter_dynamic_rebinding
                    },
                    {
                        "sort_dispatch_sorter_dynamic_properties",
                        "dynamic properties",
                        &tests_sort_dispatch_sorter_dynamic_properties
                    },
                    {
                        "sort_dispatch_sorter_ordered_overloads",
                        "ordered overloads",
                        &tests_sort_dispatch_sorter_ordered_overloads
                    }
                }
            },
            dt::block_spec
            {
                "entry",
                "the six free sort() overloads",
                {
                    {
                        "sort_dispatch_entry_compile_time_forms",
                        "compile time forms",
                        &tests_sort_dispatch_entry_compile_time_forms
                    },
                    {
                        "sort_dispatch_entry_runtime_forms",
                        "runtime forms",
                        &tests_sort_dispatch_entry_runtime_forms
                    },
                    {
                        "sort_dispatch_entry_default_tag",
                        "default tag",
                        &tests_sort_dispatch_entry_default_tag
                    },
                    {
                        "sort_dispatch_entry_battery",
                        "battery",
                        &tests_sort_dispatch_entry_battery
                    },
                    {
                        "sort_dispatch_entry_pointer_iterators",
                        "pointer iterators",
                        &tests_sort_dispatch_entry_pointer_iterators
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_DISPATCH_TESTS_
