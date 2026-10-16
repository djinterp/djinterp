/*******************************************************************************
* djinterp [core]                         test_functional_parity_quantifiers.cpp
*
*
* path:      /tests/djinterp/core/functional/test_functional_parity_quantifiers.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
/*
test_functional_parity_quantifiers.cpp
  The quantifier half of the differential test, in a SEPARATE translation unit
from test_functional_parity.cpp, and not by preference.

  `monoid.hpp` and `accumulator.hpp` cannot be compiled into one TU. Both
declare `sum`, `product`, `min` and `max` flat in namespace `djinterp` -- the
former as struct templates (monoid.hpp:99-199), the latter as function
templates (accumulator.hpp:1168-1247) -- because monoid.hpp never opens the
`namespace monoids` its own header comment, section banner and functional.md
all describe. Including both yields four `conflicting declaration of template`
errors. The split here is that defect, made visible rather than worked around.

  Two further notes, both recorded in PARITY_NOTES.md:
    - transducer.hpp must precede accumulator.hpp. accumulator.hpp uses
      D_CONSTEXPR14 forty times and never defines it; consumer.hpp and
      transducer.hpp each define it locally under an #ifndef guard, and
      djinterp.hpp does not define it at all. So accumulator.hpp compiles only
      when something else has already run.
    - the C-side `functional_common.h` must carry a guard distinct from the
      C++-side `functional_common.hpp`. Both shipped `DJINTERP_FUNCTIONAL_FUNCTIONAL_COMMON_HPP`,
      so whichever was included second was skipped entirely, which is what made
      a differential test impossible to write at all.

  Build:
    gcc -std=c99 -c ../c/functional/reducer.c -o reducer.o
    g++ -std=c++17 -c test_functional_parity_quantifiers.cpp -o q.o
    g++ -o test_parity_quantifiers q.o reducer.o
*/

// djinterp
#include "../../../../inc/djinterp/core/functional/transducer.hpp"
#include "../../../../inc/djinterp/core/functional/accumulator.hpp"

extern "C"
{
    #include "../../../../inc/djinterp/c/functional/reducer.h"
}

#include <vector>
#include <cstdio>
#include <cstdint>


extern "C"
{
    static bool
    parity_q_pred_even(const void* _element, void* _context)
    {
        (void)_context;

        return ( ((*(const intmax_t*)_element) % 2) == 0 );
    }

    static bool
    parity_q_pred_positive(const void* _element, void* _context)
    {
        (void)_context;

        return ( (*(const intmax_t*)_element) > 0 );
    }
}

namespace
{
    std::size_t g_compared = 0;
    std::size_t g_agreed   = 0;

    void
    record(bool _agrees, const char* _label)
    {
        g_compared = g_compared + 1;

        if (_agrees)
        {
            g_agreed = g_agreed + 1;
        }
        else
        {
            std::printf("    PARITY BREAK  %s\n", _label);
        }
    }
}


int
main
(
    void
)
{
    std::vector<intmax_t> input;
    std::vector<intmax_t> empty;
    bool                  cpp_value;
    bool                  c_value;

    for (intmax_t i = 1; i <= 10; ++i)
    {
        input.push_back(i);
    }

    std::printf("djinterp [functional] C / C++ differential parity\n");
    std::printf("  quantifiers (separate TU; see the header comment)\n");

    // any: satisfied at the second element, so both sides must short-circuit
    cpp_value = djinterp::any_match<intmax_t>(
                    [](intmax_t _x) { return (_x % 2) == 0; }).run(input);
    c_value   = d_reducer_any(input.data(), input.size(), sizeof(intmax_t),
                              &parity_q_pred_even, NULL);
    record((cpp_value == c_value), "quantifier: any");

    // all: fails at the first element
    cpp_value = djinterp::all_match<intmax_t>(
                    [](intmax_t _x) { return (_x % 2) == 0; }).run(input);
    c_value   = d_reducer_all(input.data(), input.size(), sizeof(intmax_t),
                              &parity_q_pred_even, NULL);
    record((cpp_value == c_value), "quantifier: all (counterexample)");

    // all: holds throughout, so neither side short-circuits
    cpp_value = djinterp::all_match<intmax_t>(
                    [](intmax_t _x) { return _x > 0; }).run(input);
    c_value   = d_reducer_all(input.data(), input.size(), sizeof(intmax_t),
                              &parity_q_pred_positive, NULL);
    record((cpp_value == c_value), "quantifier: all (holds)");

    cpp_value = djinterp::none_match<intmax_t>(
                    [](intmax_t _x) { return (_x % 2) == 0; }).run(input);
    c_value   = d_reducer_none(input.data(), input.size(), sizeof(intmax_t),
                               &parity_q_pred_even, NULL);
    record((cpp_value == c_value), "quantifier: none");

    // the vacuous cases, which are where quantifiers most often disagree
    cpp_value = djinterp::all_match<intmax_t>(
                    [](intmax_t _x) { return (_x % 2) == 0; }).run(empty);
    c_value   = d_reducer_all(input.data(), 0, sizeof(intmax_t),
                              &parity_q_pred_even, NULL);
    record((cpp_value == c_value),
           "quantifier: all over empty is vacuously true on both");

    cpp_value = djinterp::any_match<intmax_t>(
                    [](intmax_t _x) { return (_x % 2) == 0; }).run(empty);
    c_value   = d_reducer_any(input.data(), 0, sizeof(intmax_t),
                              &parity_q_pred_even, NULL);
    record((cpp_value == c_value),
           "quantifier: any over empty is false on both");

    cpp_value = djinterp::none_match<intmax_t>(
                    [](intmax_t _x) { return (_x % 2) == 0; }).run(empty);
    c_value   = d_reducer_none(input.data(), 0, sizeof(intmax_t),
                               &parity_q_pred_even, NULL);
    record((cpp_value == c_value),
           "quantifier: none over empty is vacuously true on both");

    std::printf("\n  %lu / %lu comparisons agreed\n",
                (unsigned long)g_agreed,
                (unsigned long)g_compared);

    if (g_agreed != g_compared)
    {
        return 1;
    }

    return 0;
}
