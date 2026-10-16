/*******************************************************************************
* djinterp [test]                                             sequence_tests.hpp
*
*   Unit-test suite header for inc/djinterp/core/paradigm/sequence/sequence.hpp
* - the ordered, contiguous linear element sequence and its make_sequence
* factories.  Two faces, selected by DTEST_SPEC_MODE:
*
*     - default face: the flat declarations of every tests_* predicate plus the
*       shared fixtures the section TUs build against.  Each
*       sequence_tests_*.cpp includes the header this way and DEFINES the
*       predicates it owns.
*
*     - spec-provider face (DTEST_SPEC_MODE defined): the above PLUS
*       sequence_spec(), which the runner hands to run_module.  Only the runner
*       defines DTEST_SPEC_MODE, so the spec is compiled exactly once.
*
*   LANGUAGE FLOOR: the variadic constructor's SFINAE guard uses a fold
* expression, so C++17 is the floor (GCC accepts it at C++14 only as an
* extension, with -Wc++17-extensions).  The suite targets C++17 and up and has
* no dialect-conditional surface.
*
*   BUILD PREREQUISITE (see the accompanying report): as shipped, sequence's
* iterator-pair constructor is an UNCONSTRAINED two-parameter template.  Under
* partial ordering that makes it MORE SPECIALIZED than the variadic
* constructor, so every two-argument variadic call selects it and forwards the
* pair straight to std::vector:
*     sequence<int>(1, 2)      -> vector<int>(1, 2)    -> {2}      (silent!)
*     sequence<int>(3, 5)      -> vector<int>(3, 5)    -> {5,5,5}  (silent!)
*     sequence<string>(a, b)   -> no viable vector ctor -> HARD COMPILE ERROR
* The fix constrains the overload to genuine iterators, exactly as std::vector
* constrains its own range constructor.  A second fix guards append() against
* self-append, which otherwise inserts from a range aliasing its own storage
* (observably corrupting move-enabled element types).  This suite targets the
* fixed header; patch and fixed header accompany it.
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_SEQUENCE_TESTS_
#define DJINTERP_TEST_SEQUENCE_TESTS_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/paradigm/sequence/sequence.hpp"                       // the header under test (bare form)
#include "djinterp/test/test_defaults.hpp"    // module_spec, run_module


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                SHARED FIXTURES                                          ///
///////////////////////////////////////////////////////////////////////////////
//   Aliases, one element type, and one comparison helper.  Definitions only -
// no shared mutable state - so the header is safe to include from every
// section TU.

// int_sequence
//   alias: the workhorse instantiation.  An arithmetic element type is the one
// that hides the two-argument construction bug behind a silent wrong answer
// rather than a compile error, so most construction tests run through it.
using int_sequence = sequence<int>;

// string_sequence
//   alias: a class element type.  Exercises the move overloads meaningfully and
// is the instantiation for which the unfixed two-argument variadic call did not
// compile at all.
using string_sequence = sequence<std::string>;


// probe
//   type: an element that records how it was constructed, so the suite can
// prove the move overloads really move rather than copy.  Copy-constructible
// (sequence requires it), move-constructible, default-constructible (so
// resize() can value-initialize it), and equality-comparable (so sequence's
// operator== instantiates).
//
//   Tests assert on the SOURCE object's moved_from flag rather than on a stored
// element, since the underlying vector may freely move elements around during
// reallocation.
struct probe
{
    int  value;
    bool moved_from;      // this object was the source of a move

    probe()
        : value     (0),
          moved_from(false)
    {}

    explicit probe(int _value)
        : value     (_value),
          moved_from(false)
    {}

    probe(const probe& _other)
        : value     (_other.value),
          moved_from(false)
    {}

    probe(probe&& _other) noexcept
        : value     (_other.value),
          moved_from(false)
    {
        _other.moved_from = true;
    }

    probe&
    operator=(const probe& _other)
    {
        value      = _other.value;
        moved_from = false;

        return *this;
    }

    probe&
    operator=(probe&& _other) noexcept
    {
        value             = _other.value;
        moved_from        = false;
        _other.moved_from = true;

        return *this;
    }

    bool
    operator==(const probe& _other) const
    {
        return (value == _other.value);
    }
};


// matches
//   helper: true if _seq holds exactly the elements of _expected, in order.
// Compares through the public index operator and size(), so it exercises the
// same surface a caller would.
template<typename _Type>
bool
matches(
    const sequence<_Type>&       _seq,
    std::initializer_list<_Type> _expected
)
{
    if (_seq.size() != _expected.size())
    {
        return false;
    }

    std::size_t i = 0;
    for (const _Type& e : _expected)
    {
        if (!(_seq[i] == e))
        {
            return false;
        }

        ++i;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                TEST DECLARATIONS                                         ///
///////////////////////////////////////////////////////////////////////////////

// -- section: construction (sequence_tests_construction.cpp)
bool tests_ctor_default();
bool tests_ctor_variadic_single();
bool tests_ctor_variadic_two();
bool tests_ctor_variadic_many();
bool tests_ctor_variadic_class_type();
bool tests_ctor_variadic_two_class_type();
bool tests_ctor_variadic_is_explicit();
bool tests_ctor_variadic_sfinae_mixed_types();
bool tests_ctor_initializer_list();
bool tests_ctor_initializer_list_empty();
bool tests_ctor_iterator_pair_pointer();
bool tests_ctor_iterator_pair_vector();
bool tests_ctor_iterator_pair_non_contiguous();
bool tests_ctor_iterator_pair_empty_range();
bool tests_ctor_pointer_count();
bool tests_ctor_pointer_count_zero();
bool tests_ctor_vector_copy();
bool tests_ctor_vector_move();
bool tests_ctor_count_value_fill();
bool tests_ctor_count_value_zero();
bool tests_ctor_copy();
bool tests_ctor_move();

// -- section: element access (sequence_tests_element_access.cpp)
bool tests_index_operator();
bool tests_index_operator_const();
bool tests_index_operator_mutates();
bool tests_front();
bool tests_front_const();
bool tests_front_mutates();
bool tests_back();
bool tests_back_const();
bool tests_back_mutates();
bool tests_data_const();
bool tests_data_mutable();
bool tests_data_contiguous();

// -- section: capacity (sequence_tests_capacity.cpp)
bool tests_size();
bool tests_empty();
bool tests_capacity_tracks_modification();

// -- section: iteration (sequence_tests_iteration.cpp)
bool tests_begin_end();
bool tests_begin_end_const();
bool tests_cbegin_cend();
bool tests_range_for();
bool tests_iterator_mutates();
bool tests_iterator_std_algorithms();
bool tests_iteration_empty();

// -- section: modification (sequence_tests_modification.cpp)
bool tests_push_back_copy();
bool tests_push_back_move();
bool tests_append();
bool tests_append_empty_source();
bool tests_append_to_empty();
bool tests_append_self();
bool tests_clear();
bool tests_reserve();
bool tests_resize_grow();
bool tests_resize_shrink();
bool tests_resize_zero();

// -- section: subsequence (sequence_tests_subsequence.cpp)
bool tests_sub();
bool tests_sub_zero_count();
bool tests_sub_full();
bool tests_sub_at_end();
bool tests_head();
bool tests_head_zero();
bool tests_head_full();
bool tests_tail();
bool tests_tail_zero();
bool tests_tail_full();
bool tests_subsequence_is_a_copy();

// -- section: comparison (sequence_tests_comparison.cpp)
bool tests_equality();
bool tests_equality_self();
bool tests_equality_different_size();
bool tests_equality_different_elements();
bool tests_equality_empty();
bool tests_inequality();

// -- section: conversion (sequence_tests_conversion.cpp)
bool tests_vec();
bool tests_vec_reflects_contents();
bool tests_release();
bool tests_release_empties_source();

// -- section: factories (sequence_tests_factories.cpp)
bool tests_make_sequence_variadic();
bool tests_make_sequence_variadic_two();
bool tests_make_sequence_variadic_single();
bool tests_make_sequence_variadic_class_type();
bool tests_make_sequence_pointer_count();
bool tests_make_sequence_c_array();


///////////////////////////////////////////////////////////////////////////////
///                SPEC-PROVIDER FACE  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// sequence_spec
//   function: assembles the module_spec - one block per semantic section of
// sequence.hpp, one test per tests_* predicate.  Compiled only in the runner TU.
inline ::djinterp::test::module_spec
sequence_spec()
{
    namespace dt = ::djinterp::test;

    dt::module_spec m;

    m.name       = "sequence";
    m.descriptor =
        "Ordered linear element sequence: construction, element access, "
        "capacity, iteration, modification, subsequence extraction, comparison, "
        "vector conversion, and the make_sequence factories.";

    m.blocks = std::vector<dt::block_spec>
    {
        dt::block_spec
        {
            "construction",
            "every constructor overload, and the resolution between them.",
            std::vector<dt::test_spec>
            {
                { "default",              "an empty sequence.",                     &tests_ctor_default                  },
                { "variadic (one)",       "a single element.",                      &tests_ctor_variadic_single          },
                { "variadic (two)",       "two elements - not count+fill.",         &tests_ctor_variadic_two             },
                { "variadic (many)",      "the documented multi-element form.",     &tests_ctor_variadic_many            },
                { "variadic (class)",     "a class element type.",                  &tests_ctor_variadic_class_type      },
                { "variadic (two class)", "two class elements.",                    &tests_ctor_variadic_two_class_type  },
                { "variadic is explicit", "no implicit element conversion.",        &tests_ctor_variadic_is_explicit     },
                { "variadic SFINAE",      "mixed argument types are rejected.",     &tests_ctor_variadic_sfinae_mixed_types },
                { "initializer_list",     "brace-enclosed construction.",           &tests_ctor_initializer_list         },
                { "initializer_list ()",  "an empty brace list.",                   &tests_ctor_initializer_list_empty   },
                { "iterators (pointer)",  "a raw pointer range.",                   &tests_ctor_iterator_pair_pointer    },
                { "iterators (vector)",   "a vector's iterators.",                  &tests_ctor_iterator_pair_vector     },
                { "iterators (list)",     "a non-contiguous forward range.",        &tests_ctor_iterator_pair_non_contiguous },
                { "iterators (empty)",    "an empty range.",                        &tests_ctor_iterator_pair_empty_range },
                { "pointer + count",      "a contiguous array.",                    &tests_ctor_pointer_count            },
                { "pointer + count (0)",  "a zero-length array.",                   &tests_ctor_pointer_count_zero       },
                { "vector (copy)",        "copying a vector.",                      &tests_ctor_vector_copy              },
                { "vector (move)",        "taking ownership of a vector.",          &tests_ctor_vector_move              },
                { "count + fill",         "n copies of a value.",                   &tests_ctor_count_value_fill         },
                { "count + fill (0)",     "zero copies.",                           &tests_ctor_count_value_zero         },
                { "copy",                 "value-semantic copy.",                   &tests_ctor_copy                     },
                { "move",                 "value-semantic move.",                   &tests_ctor_move                     }
            }
        },
        dt::block_spec
        {
            "element access",
            "operator[], front, back, and data.",
            std::vector<dt::test_spec>
            {
                { "operator[]",           "indexed read.",                    &tests_index_operator         },
                { "operator[] (const)",   "indexed read through const.",      &tests_index_operator_const   },
                { "operator[] mutates",   "indexed write.",                   &tests_index_operator_mutates },
                { "front",                "the first element.",               &tests_front                  },
                { "front (const)",        "the first element through const.", &tests_front_const            },
                { "front mutates",        "writing through front.",           &tests_front_mutates          },
                { "back",                 "the last element.",                &tests_back                   },
                { "back (const)",         "the last element through const.",  &tests_back_const             },
                { "back mutates",         "writing through back.",            &tests_back_mutates           },
                { "data (const)",         "a const pointer to storage.",      &tests_data_const             },
                { "data (mutable)",       "a mutable pointer to storage.",    &tests_data_mutable           },
                { "data contiguous",      "storage really is contiguous.",    &tests_data_contiguous        }
            }
        },
        dt::block_spec
        {
            "capacity",
            "size and empty.",
            std::vector<dt::test_spec>
            {
                { "size",            "the element count.",                 &tests_size                         },
                { "empty",           "emptiness tracks the count.",        &tests_empty                        },
                { "tracks changes",  "both follow every modification.",    &tests_capacity_tracks_modification }
            }
        },
        dt::block_spec
        {
            "iteration",
            "begin / end / cbegin / cend and their uses.",
            std::vector<dt::test_spec>
            {
                { "begin + end",       "forward traversal.",                  &tests_begin_end             },
                { "begin + end const", "traversal through const.",            &tests_begin_end_const       },
                { "cbegin + cend",     "explicit const iterators.",           &tests_cbegin_cend           },
                { "range-for",         "range-based for works.",              &tests_range_for             },
                { "iterator mutates",  "writing through an iterator.",        &tests_iterator_mutates      },
                { "std algorithms",    "usable with the standard algorithms.", &tests_iterator_std_algorithms },
                { "empty range",       "begin == end when empty.",            &tests_iteration_empty       }
            }
        },
        dt::block_spec
        {
            "modification",
            "push_back, append, clear, reserve, resize.",
            std::vector<dt::test_spec>
            {
                { "push_back (copy)",  "appending a copy.",                  &tests_push_back_copy      },
                { "push_back (move)",  "appending by move.",                 &tests_push_back_move      },
                { "append",            "appending another sequence.",        &tests_append              },
                { "append (empty src)","appending an empty sequence.",       &tests_append_empty_source },
                { "append to empty",   "appending onto an empty sequence.",  &tests_append_to_empty     },
                { "append (self)",     "appending a sequence to itself.",    &tests_append_self         },
                { "clear",             "removing every element.",            &tests_clear               },
                { "reserve",           "pre-allocation leaves size alone.",  &tests_reserve             },
                { "resize (grow)",     "new elements are value-initialized.", &tests_resize_grow        },
                { "resize (shrink)",   "trailing elements are dropped.",     &tests_resize_shrink       },
                { "resize (zero)",     "resizing to nothing.",               &tests_resize_zero         }
            }
        },
        dt::block_spec
        {
            "subsequence",
            "sub, head, and tail.",
            std::vector<dt::test_spec>
            {
                { "sub",             "an interior range.",              &tests_sub                  },
                { "sub (zero)",      "a zero-length range.",            &tests_sub_zero_count       },
                { "sub (full)",      "the whole sequence.",             &tests_sub_full             },
                { "sub (at end)",    "a zero-length range at the end.", &tests_sub_at_end           },
                { "head",            "the leading elements.",           &tests_head                 },
                { "head (zero)",     "no leading elements.",            &tests_head_zero            },
                { "head (full)",     "every element.",                  &tests_head_full            },
                { "tail",            "the trailing elements.",          &tests_tail                 },
                { "tail (zero)",     "no trailing elements.",           &tests_tail_zero            },
                { "tail (full)",     "every element.",                  &tests_tail_full            },
                { "result is a copy","the source is left untouched.",   &tests_subsequence_is_a_copy }
            }
        },
        dt::block_spec
        {
            "comparison",
            "operator== and operator!=.",
            std::vector<dt::test_spec>
            {
                { "equality",           "same size, same elements.",        &tests_equality                    },
                { "equality (self)",    "a sequence equals itself.",        &tests_equality_self               },
                { "differing size",     "size mismatch compares unequal.",  &tests_equality_different_size      },
                { "differing elements", "element mismatch compares unequal.", &tests_equality_different_elements },
                { "empty",              "two empty sequences are equal.",   &tests_equality_empty              },
                { "inequality",         "!= is the negation of ==.",        &tests_inequality                  }
            }
        },
        dt::block_spec
        {
            "conversion",
            "vec and release.",
            std::vector<dt::test_spec>
            {
                { "vec",             "a const view of the storage.",     &tests_vec                    },
                { "vec reflects",     "the view follows modifications.", &tests_vec_reflects_contents  },
                { "release",         "the storage is handed out.",       &tests_release                },
                { "release empties", "the source is left empty.",        &tests_release_empties_source }
            }
        },
        dt::block_spec
        {
            "factories",
            "make_sequence in its three forms.",
            std::vector<dt::test_spec>
            {
                { "variadic",         "from a list of elements.",     &tests_make_sequence_variadic            },
                { "variadic (two)",   "two elements - not count+fill.", &tests_make_sequence_variadic_two      },
                { "variadic (one)",   "a single element.",            &tests_make_sequence_variadic_single     },
                { "variadic (class)", "a class element type.",        &tests_make_sequence_variadic_class_type },
                { "pointer + count",  "from a contiguous array.",     &tests_make_sequence_pointer_count       },
                { "C array",          "from a C array, size deduced.", &tests_make_sequence_c_array            }
            }
        }
    };

    return m;
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_SEQUENCE_TESTS_
