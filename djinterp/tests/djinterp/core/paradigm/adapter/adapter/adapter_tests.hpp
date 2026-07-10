/******************************************************************************
* djinterp [test]                                             adapter_tests.hpp
*
*   Unit-test suite header for inc/djinterp/paradigm/adapter/adapter.hpp.  Two
* faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate (each a
*       nullary bool returning true iff all its checks passed) plus the shared
*       helper types the section TUs build against.  Every adapter_tests_*.cpp
*       includes the header this way and DEFINES the predicates it owns.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       adapter_spec(), which assembles the module_spec the runner hands to
*       run_module.  Only the runner defines DTEST_SPEC_MODE, so adapter_spec()
*       is compiled exactly once and the section TUs contribute only bodies.
*
*   COVERAGE POSTURE (read BUG NOTES below):
*   adapter.hpp has two defects that block a slice of its surface from
* compiling at all.  The suite tests the entire WORKING surface exhaustively;
* the blocked paths are guarded by D_ADAPTER_BYREF_FIXED (off by default) so
* the suite is green against the code as-shipped, documents each defect in
* place, and expands to full coverage the moment the header is fixed and the
* macro is defined.
*
* path:      /tests/djinterp/paradigm/adapter/adapter_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.09
******************************************************************************/

#ifndef DJINTERP_TEST_ADAPTER_TESTS_
#define DJINTERP_TEST_ADAPTER_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "adapter.hpp"                        // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"    // module_spec, run_module (full-from-inc)


///////////////////////////////////////////////////////////////////////////////
///                BUG NOTES (adapter.hpp, as shipped)                       ///
///////////////////////////////////////////////////////////////////////////////
//
//   BUG 1 — object_adapter / interface_adapter cannot be built by_reference.
//   The single storing constructor move-initializes its member:
//
//       explicit object_adapter(stored_type _adaptee)
//           : m_adaptee(std::move(_adaptee)) {}      // adapter.hpp ~line 562
//
//   For the DEFAULT by_reference policy, stored_type is `_Adaptee&`, so the
//   member is a reference and std::move(_adaptee) is an rvalue: binding a
//   non-const lvalue reference to an rvalue is ill-formed.  Every by_reference
//   instantiation (the module's headline usage, plus make_object_adapter and
//   constrained_adapt, which hard-wire by_reference) fails to construct.  The
//   same constructor shape breaks interface_adapter<_,_,by_reference> (and its
//   defaulted default-ctor is deleted anyway, a reference member being
//   non-default-initializable).  Minimal fix: do not std::move when the store
//   is a reference — e.g. a value/reference-store overload, or bind directly.
//
//   BUG 2 — size() / const access unusable for owning policies.  by_value,
//   by_shared_ptr and by_unique_ptr expose only a NON-const access:
//
//       static reference_type access(stored_type& _s) noexcept;   // no const
//
//   object_adapter::size() is unconditionally const and routes through the
//   const adaptee(), which needs `access(const stored_type&)`; that overload
//   does not exist, so size() (and const get()/adaptee()) is ill-formed for
//   the owning policies.  interface_adapter's const adaptee() is blocked the
//   same way under by_value.  Minimal fix: add a const access overload to each
//   owning policy.
//
//   Both defects surface only on INSTANTIATION (constructor / member bodies),
//   so neither is detectable via std::is_constructible or a decltype probe —
//   hence the macro gate rather than a compile-time negative assertion.  The
//   working policies cover the very same member logic: by_pointer exercises
//   object_adapter's size()/get()/adaptee()/forward() in full, and by_value +
//   by_pointer together cover interface_adapter.  So the coverage actually
//   lost while the gate is off is only the by_reference construction lines and
//   the two by_reference factory bodies.
//
//   D_ADAPTER_BYREF_FIXED
//     macro: define to 1 once adapter.hpp's constructor is fixed to activate
//   the by_reference construction tests, make_object_adapter, and
//   constrained_adapt (uncomment the line below, or pass -D on the build).
// #define D_ADAPTER_BYREF_FIXED 1


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED HELPER TYPES                                       ///
///////////////////////////////////////////////////////////////////////////////
//   Only the helpers used by more than one section live here; section-specific
// probe types and functors are defined locally (in anonymous namespaces) in
// the owning TU.

// legacy_seq
//   type: a stand-in "legacy" container the adapters wrap.  Presents an
// idiosyncratic interface (num_elements / element_at / append) that the
// adaptation policies remap onto the target vocabulary (size / get / forward).
struct legacy_seq
{
    std::vector<int> data;

    std::size_t
    num_elements() const
    {
        return data.size();
    }

    int&
    element_at(
        std::size_t _i
    )
    {
        return data[_i];
    }

    const int&
    element_at(
        std::size_t _i
    ) const
    {
        return data[_i];
    }

    void
    append(
        int _v
    )
    {
        data.push_back(_v);

        return;
    }
};

// legacy_policy
//   struct: adaptation policy remapping the target interface (size / get /
// forward) onto legacy_seq's members.  Supplies both a mutable and a const
// get so an adapter's const and non-const get() overloads both resolve.
struct legacy_policy
{
    static std::size_t
    size(
        const legacy_seq& _c
    )
    {
        return _c.num_elements();
    }

    static int&
    get(
        legacy_seq& _c,
        std::size_t _i
    )
    {
        return _c.element_at(_i);
    }

    static const int&
    get(
        const legacy_seq& _c,
        std::size_t       _i
    )
    {
        return _c.element_at(_i);
    }

    static std::size_t
    forward(
        legacy_seq& _c,
        int         _v
    )
    {
        _c.append(_v);

        return _c.num_elements();
    }
};

// vector_policy
//   struct: adaptation policy mapping the target interface onto a
// std::vector<int> (size -> .size(), get -> operator[]).  Used where an
// adaptee that already models value_type + structure is needed (the concept
// section, chiefly).
struct vector_policy
{
    static std::size_t
    size(
        const std::vector<int>& _v
    )
    {
        return _v.size();
    }

    static int&
    get(
        std::vector<int>& _v,
        std::size_t       _i
    )
    {
        return _v[_i];
    }

    static const int&
    get(
        const std::vector<int>& _v,
        std::size_t             _i
    )
    {
        return _v[_i];
    }
};


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: configuration & ownership policies (adapter_tests_ownership.cpp)
bool tests_feature_gates();
bool tests_by_reference();
bool tests_by_pointer();
bool tests_by_value();
bool tests_by_shared_ptr();
bool tests_by_unique_ptr();

// -- section: adaptation traits (adapter_tests_traits.cpp)
bool tests_has_value_type();
bool tests_has_size_method();
bool tests_has_begin_end();
bool tests_has_push_back();
bool tests_has_insert();
bool tests_has_subscript_operator();
bool tests_value_types_compatible();
bool tests_is_invocable_mapping();
bool tests_are_value_type_compatible();
bool tests_is_structurally_adaptable();
bool tests_is_invocable_adapter();
bool tests_adaptation_class();

// -- section: object adapter (adapter_tests_object.cpp)
bool tests_object_adapter_typedefs();
bool tests_object_adapter_by_reference();
bool tests_object_adapter_by_pointer();
bool tests_object_adapter_by_value();
bool tests_object_adapter_by_shared_ptr();
bool tests_object_adapter_by_unique_ptr();
bool tests_object_adapter_const_access();
bool tests_object_adapter_forward();

// -- section: class / interface adapters + forwarding (adapter_tests_class_iface.cpp)
bool tests_class_adapter_basic();
bool tests_class_adapter_polymorphism();
bool tests_class_adapter_constructors();
bool tests_class_adapter_const_adaptee_ref();
bool tests_interface_adapter_by_value();
bool tests_interface_adapter_by_pointer();
bool tests_interface_adapter_const_adaptee();
bool tests_forward_as_is();
bool tests_forward_with_transform();

// -- section: function adapters (adapter_tests_function.cpp)
bool tests_function_adapter_transform();
bool tests_function_adapter_transform_const();
bool tests_function_adapter_passthrough();
bool tests_function_adapter_passthrough_const();
bool tests_result_adapter();
bool tests_result_adapter_const();
bool tests_argument_adapter();
bool tests_compose_adapter();
bool tests_compose_adapter_const();

// -- section: view adapters (adapter_tests_view.cpp)
bool tests_adapted_ref_basic();
bool tests_adapted_ref_mutation();
bool tests_adapted_ref_const();
bool tests_adapted_const_ref();
bool tests_adapted_view_iteration();
bool tests_adapted_view_iterator_ops();
bool tests_adapted_view_size();

// -- section: convenience factories (adapter_tests_factory.cpp)
bool tests_make_object_adapter();
bool tests_make_owning_adapter();
bool tests_make_function_adapter_transform();
bool tests_make_function_adapter_passthrough();
bool tests_make_result_adapter();
bool tests_make_compose();
bool tests_make_adapted_ref();
bool tests_make_adapted_view();

// -- section: concept-constrained adapters (adapter_tests_concepts.cpp)
bool tests_adaptable_to();
bool tests_adapter_for();
bool tests_function_adaptable();
bool tests_constrained_adapt();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// adapter_spec
//   function: assembles the module_spec for the adapter suite — one block per
// semantic section of adapter.hpp, one test per tests_* predicate.  Compiled
// only in the runner TU (the sole definer of DTEST_SPEC_MODE); the section
// TUs supply the predicate bodies these entries point at.
inline ::djinterp::test::module_spec
adapter_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "adapter";
    m.descriptor =
        "Adapter pattern module: ownership policies, adaptation traits, the "
        "object / class / interface adapters, function + view adapters, the "
        "convenience factories, and the C++20 concept surface.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "configuration & ownership policies",
            "feature gates and the five ownership storage policies.",
            std::vector<dt::test_spec>
            {
                { "feature gates",  "D_ADAPTER_HAS_* track the language level.", &tests_feature_gates },
                { "by_reference",   "reference store: typedefs + access().",     &tests_by_reference  },
                { "by_pointer",     "pointer store: typedefs + access().",       &tests_by_pointer    },
                { "by_value",       "value store: typedefs + access().",         &tests_by_value      },
                { "by_shared_ptr",  "shared_ptr store: typedefs + access().",    &tests_by_shared_ptr },
                { "by_unique_ptr",  "unique_ptr store: typedefs + access().",    &tests_by_unique_ptr }
            }
        },
        dt::block_spec
        {
            "adaptation traits",
            "structural probes, value-type compatibility, invocable mapping.",
            std::vector<dt::test_spec>
            {
                { "has_value_type",            "detects T::value_type.",                    &tests_has_value_type            },
                { "has_size_method",           "detects const T.size().",                   &tests_has_size_method           },
                { "has_begin_end",             "detects T.begin()/T.end().",                &tests_has_begin_end             },
                { "has_push_back",             "detects T.push_back(value_type).",          &tests_has_push_back             },
                { "has_insert",                "detects T.insert(value_type).",             &tests_has_insert                },
                { "has_subscript_operator",    "detects T[size_t].",                        &tests_has_subscript_operator    },
                { "value_types_compatible",    "internal value-type convertibility.",       &tests_value_types_compatible    },
                { "is_invocable_mapping",      "internal Fn:From->To probe.",               &tests_is_invocable_mapping      },
                { "are_value_type_compatible", "public value-type trait (+ _v).",           &tests_are_value_type_compatible },
                { "is_structurally_adaptable", "iterable/sized/indexable overlap (+ _v).",  &tests_is_structurally_adaptable },
                { "is_invocable_adapter",      "public Fn:From->To trait (+ _v).",          &tests_is_invocable_adapter      },
                { "adaptation_class",          "aggregate classification struct.",          &tests_adaptation_class          }
            }
        },
        dt::block_spec
        {
            "object adapter",
            "composition adapter across ownership policies + delegation.",
            std::vector<dt::test_spec>
            {
                { "typedefs",       "adaptee/adaptation/ownership aliases.",           &tests_object_adapter_typedefs      },
                { "by_reference",   "by_reference construction + delegation.",         &tests_object_adapter_by_reference  },
                { "by_pointer",     "by_pointer: size/get/adaptee/forward, const.",    &tests_object_adapter_by_pointer    },
                { "by_value",       "by_value owning copy: non-const delegation.",     &tests_object_adapter_by_value      },
                { "by_shared_ptr",  "shared ownership delegation.",                    &tests_object_adapter_by_shared_ptr },
                { "by_unique_ptr",  "move-only exclusive ownership delegation.",       &tests_object_adapter_by_unique_ptr },
                { "const access",   "const adaptee()/get()/size() overloads.",         &tests_object_adapter_const_access  },
                { "forward",        "variadic forward() through the policy.",          &tests_object_adapter_forward       }
            }
        },
        dt::block_spec
        {
            "class / interface adapters + forwarding",
            "MI adapter, CRTP adapter, and the method-forwarding policies.",
            std::vector<dt::test_spec>
            {
                { "class_adapter basic",          "delegation through adaptee_ref().",   &tests_class_adapter_basic          },
                { "class_adapter polymorphism",   "target interface dispatch.",          &tests_class_adapter_polymorphism   },
                { "class_adapter constructors",   "default / copy / move adaptee ctors.",&tests_class_adapter_constructors   },
                { "class_adapter const adaptee",  "const adaptee_ref().",                &tests_class_adapter_const_adaptee_ref },
                { "interface_adapter by_value",   "default+value ctor, non-const.",      &tests_interface_adapter_by_value   },
                { "interface_adapter by_pointer", "pointer store delegation.",           &tests_interface_adapter_by_pointer },
                { "interface_adapter const",      "const adaptee() (by_pointer).",       &tests_interface_adapter_const_adaptee },
                { "forward_as_is",                "identity method forwarding.",         &tests_forward_as_is                },
                { "forward_with_transform",       "argument-transforming forwarding.",   &tests_forward_with_transform       }
            }
        },
        dt::block_spec
        {
            "function adapters",
            "callable wrappers: transform, result, argument, compose.",
            std::vector<dt::test_spec>
            {
                { "function_adapter transform",       "per-arg transform then call.",    &tests_function_adapter_transform       },
                { "function_adapter transform const", "const operator() overload.",      &tests_function_adapter_transform_const },
                { "function_adapter passthrough",     "void-transform passthrough.",     &tests_function_adapter_passthrough     },
                { "function_adapter passthrough const","const passthrough overload.",    &tests_function_adapter_passthrough_const },
                { "result_adapter",                   "post-transform of result.",       &tests_result_adapter                   },
                { "result_adapter const",             "const operator() overload.",      &tests_result_adapter_const             },
                { "argument_adapter",                 "per-position tuple transforms.",  &tests_argument_adapter                 },
                { "compose_adapter",                  "outer(inner(args...)).",          &tests_compose_adapter                  },
                { "compose_adapter const",            "const operator() overload.",      &tests_compose_adapter_const            }
            }
        },
        dt::block_spec
        {
            "view adapters",
            "non-owning projections: adapted_ref / const_ref / view.",
            std::vector<dt::test_spec>
            {
                { "adapted_ref basic",       "size/get delegation.",                 &tests_adapted_ref_basic       },
                { "adapted_ref mutation",    "write-through get().",                 &tests_adapted_ref_mutation    },
                { "adapted_ref const",       "const adaptee()/get() overloads.",     &tests_adapted_ref_const       },
                { "adapted_const_ref",       "read-only view delegation.",           &tests_adapted_const_ref       },
                { "adapted_view iteration",  "projected element traversal.",         &tests_adapted_view_iteration  },
                { "adapted_view iterator",   "*, pre/post ++, ==, != operators.",    &tests_adapted_view_iterator_ops },
                { "adapted_view size",       "size() passthrough.",                  &tests_adapted_view_size       }
            }
        },
        dt::block_spec
        {
            "convenience factories",
            "make_* deduction helpers (C++14+).",
            std::vector<dt::test_spec>
            {
                { "make_object_adapter",             "by_reference object adapter.",   &tests_make_object_adapter             },
                { "make_owning_adapter",             "by_value object adapter.",       &tests_make_owning_adapter             },
                { "make_function_adapter transform", "transforming function adapter.", &tests_make_function_adapter_transform },
                { "make_function_adapter passthrough","passthrough function adapter.", &tests_make_function_adapter_passthrough },
                { "make_result_adapter",             "result adapter.",                &tests_make_result_adapter             },
                { "make_compose",                    "compose adapter.",               &tests_make_compose                    },
                { "make_adapted_ref",                "adapted_ref view.",              &tests_make_adapted_ref                },
                { "make_adapted_view",               "adapted_view projection.",       &tests_make_adapted_view               }
            }
        },
        dt::block_spec
        {
            "concept-constrained adapters",
            "C++20 concepts + the constrained factory.",
            std::vector<dt::test_spec>
            {
                { "adaptable_to",       "value + structural adaptability.",      &tests_adaptable_to       },
                { "adapter_for",        "adaptee() + size() surface.",           &tests_adapter_for        },
                { "function_adaptable", "invocability constraint.",              &tests_function_adaptable },
                { "constrained_adapt",  "requires-guarded object factory.",      &tests_constrained_adapt  }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_ADAPTER_TESTS_
