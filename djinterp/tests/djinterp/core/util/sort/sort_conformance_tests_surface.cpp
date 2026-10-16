/*******************************************************************************
* djinterp [core]                             sort_conformance_tests_surface.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_conformance_tests_surface.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_conformance_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_conformance_ordered_surface_is_uniform
bool tests_sort_conformance_ordered_surface_is_uniform()
{
    verdict result;

    typedef std::vector<int>::iterator iterator_type;
    typedef less<int>                  compare_type;

    //   A caller swapping one algorithm for another depends on the six
    // *_sort_ordered entry points having the SAME shape.  They do not.
    //
    //   merge, quick and heap declare the fourth parameter sort_order.
    // bubble, selection and insertion declare it `bool` while their bodies
    // construct internal::order_comparator<C>(comp, sort_order) -- and
    // sort_order is a scoped enum, so no conversion exists in either
    // direction.  Those three templates cannot be instantiated at all: not
    // with a bool (the body is ill-formed) and not with a sort_order (the
    // call is).
    //
    //   The per-module *_entry sections already report their own algorithm's
    // half of this.  The claim HERE is the cross-cutting one -- that the
    // surface is uniform -- and it is what a caller writing generic code
    // actually leans on.  Fix: change the three signatures to sort_order.
    result.check(conf_merge_ordered_probe<iterator_type,
                                          compare_type>::value);
    result.check(conf_quick_ordered_probe<iterator_type,
                                          compare_type>::value);
    result.check(conf_heap_ordered_probe<iterator_type,
                                         compare_type>::value);

    //   expected FAIL x3 until the signatures are corrected
    result.check(conf_bubble_ordered_probe<iterator_type,
                                           compare_type>::value);
    result.check(conf_selection_ordered_probe<iterator_type,
                                              compare_type>::value);
    result.check(conf_insertion_ordered_probe<iterator_type,
                                              compare_type>::value);

    return result.ok();
}


// tests_sort_conformance_dialect_gating_is_consistent
bool tests_sort_conformance_dialect_gating_is_consistent()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;

    //   sort_common.hpp's banner says the header "is designed to compile
    // cleanly from C++98 through C++23 by gating modern features behind the
    // project's D_ENV_* detection macros".  Two constructs are NOT gated:
    //
    //     #include <cstdint>                            (C++11 header)
    //     enum class sort_order : std::uint8_t          (scoped enum)
    //
    //   Every algorithm header includes sort_common.hpp, so no part of the
    // subsystem compiles under -std=c++98.  That makes each `#else // C++98`
    // arm in insertion/merge/quick/heap unreachable -- roughly forty lines of
    // entry-point code that no build configuration can select, and that the
    // CMake project cannot even attempt (its CXX_STANDARD choices start at
    // 11).
    //
    //   The shape below is pinned so that gating the enum later does not
    // silently change the enumerators or the underlying type that callers,
    // and order_comparator, already depend on.
    result.check(std::is_enum<sort_order>::value);
    result.check(!std::is_convertible<sort_order, int>::value);
    result.check((std::is_same<
        typename std::underlying_type<sort_order>::type,
        std::uint8_t>::value));

    //   The *_apply_98 helpers are the part of the C++98 story that IS
    // reachable: they sit outside the dialect gate and are compiled into every
    // build.  Only four of the six algorithms have one -- bubble and selection
    // have no _98 variant and no C++98 entry point at all, which is a second,
    // quieter asymmetry in the same surface.
    data     = shape_random(64);
    expected = data;
    std::stable_sort(expected.begin(), expected.end(), less<int>());

    internal::insertion_sort_apply_98(data.begin(),
                                      data.end(),
                                      less<int>(),
                                      static_cast<int*>(0));
    result.check(ranges_equal(data, expected));

    data = shape_random(64);
    internal::merge_sort_apply_98(data.begin(),
                                  data.end(),
                                  less<int>(),
                                  static_cast<int*>(0),
                                  static_cast<std::ptrdiff_t*>(0));
    result.check(ranges_equal(data, expected));

    data = shape_random(64);
    internal::quick_sort_apply_98(data.begin(),
                                  data.end(),
                                  less<int>(),
                                  static_cast<int*>(0));
    result.check(ranges_equal(data, expected));

    data = shape_random(64);
    internal::heap_sort_apply_98(data.begin(),
                                 data.end(),
                                 less<int>(),
                                 static_cast<std::ptrdiff_t*>(0));
    result.check(ranges_equal(data, expected));

    return result.ok();
}


// tests_sort_conformance_bubble_shrink_boundary_invariant
bool tests_sort_conformance_bubble_shrink_boundary_invariant()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      length;
    std::ptrdiff_t   unsorted_end;
    std::ptrdiff_t   new_end;
    std::ptrdiff_t   current;
    bool             swapped;
    bool             guard_ever_fired;

    //   bubble_sort_apply ends each outer pass with
    //
    //       unsorted_end = new_end;
    //       if (_first == unsorted_end) { break; }
    //
    //   That guard is DEAD.  new_end starts every pass at _first and is only
    // ever reassigned to `next`, which starts at _first + 1 and only
    // increases; so whenever a swap happened new_end >= _first + 1, and when
    // none happened the earlier `if (!swapped) break` already left the loop.
    // _first == unsorted_end is therefore unreachable, which is exactly what
    // the coverage run reports: the taken-branch count for that line is zero
    // across every input in the battery.
    //
    //   This test re-implements the shrink bookkeeping and asserts the
    // invariant directly, so the dead branch is documented as removable
    // rather than left looking like an untested path.  Termination does not
    // depend on the guard: once unsorted_end reaches _first + 1 the inner
    // loop runs zero times, swapped stays false, and the pass breaks.
    guard_ever_fired = false;

    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            data = make_shape(shape_index, length);

            if (data.empty())
            {
                continue;
            }

            unsorted_end = static_cast<std::ptrdiff_t>(data.size());

            for (;;)
            {
                swapped = false;
                new_end = 0;

                for (current = 0; (current + 1) < unsorted_end; ++current)
                {
                    if (data[static_cast<std::size_t>(current + 1)] <
                        data[static_cast<std::size_t>(current)])
                    {
                        std::iter_swap(data.begin() + current,
                                       data.begin() + current + 1);
                        swapped = true;
                        new_end = current + 1;
                    }
                }

                if (!swapped)
                {
                    break;
                }

                //   the invariant: a pass that swapped always leaves the new
                // boundary strictly past _first
                result.check(new_end > 0);

                unsorted_end = new_end;

                if (unsorted_end == 0)
                {
                    guard_ever_fired = true;
                    break;
                }
            }

            //   and the bookkeeping still sorts, so the invariant is not
            // holding because the loop is broken
            result.check(is_sorted_range(data.begin(),
                                         data.end(),
                                         less<int>()));
        }
    }

    //   the guard never fires on any shape or size in the battery
    result.check(!guard_ever_fired);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
