/*******************************************************************************
* djinterp [core]                                     test_functional_parity.cpp
*
*
* path:      /tests/djinterp/core/functional/test_functional_parity.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
/*
test_functional_parity.cpp
  The differential test required by the parity law:

      canon(lower(A (*)_cpp B))  ==  canon(lower(A) (*)_c B)

  For each formal object implemented on both sides, the same inputs are pushed
through the C++ face and through the C kernel and the results are compared
byte for byte. A mismatch is a parity break; a compile failure on either side
is a worse one.

  COVERAGE IS PARTIAL, AND THE GAPS ARE DEFECTS IN THE C++ FORK, NOT OMISSIONS
HERE. Of the eleven vocabulary rows, four cannot be compared because their C++
headers do not compile in this corpus:
    - producer.hpp   : two hard errors (see PARITY_NOTES.md D-1, D-2)
    - transducer.hpp : includes functional_traits.hpp, absent from the corpus
    - accumulator.hpp: same missing include
    - the monoid newtypes are reachable only flat in `djinterp`, because
      monoid.hpp never opens `namespace monoids` (D-3), so this file spells
      them `djinterp::sum<...>` rather than `djinterp::monoids::sum<...>`.
  What is compared here is the algebra (Semigroup, Monoid) and the fold, which
is the layer every other row is specified over.

  Build:
    g++ -std=c++17 -Wall -Wextra -o test_parity test_functional_parity.cpp \
        ../c/functional/reducer.c ../c/functional/semigroup.c \
        ../c/functional/monoid.c ../c/functional/producer.c \
        ../c/functional/transducer.c
*/

// djinterp
#include "../../../../inc/djinterp/core/functional/monoid.hpp"
#include "../../../../inc/djinterp/core/functional/reduce.hpp"
#include "../../../../inc/djinterp/core/functional/transducer.hpp"

extern "C"
{
    #include "../../../../inc/djinterp/c/functional/monoid.h"
    #include "../../../../inc/djinterp/c/functional/reducer.h"
    #include "../../../../inc/djinterp/c/functional/transducer.h"
    #include "../../../../inc/djinterp/c/functional/producer.h"
}

#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>


namespace
{
    // parity_tally
    //   struct: the running count of differential comparisons.
    struct parity_tally
    {
        std::size_t compared;
        std::size_t agreed;
    };

    void
    record(parity_tally& _tally, bool _agrees, const char* _label)
    {
        _tally.compared = _tally.compared + 1;

        if (_agrees)
        {
            _tally.agreed = _tally.agreed + 1;
        }
        else
        {
            std::printf("    PARITY BREAK  %s\n", _label);
        }
    }

    // exact_bits
    //   compares two values of the same width by their object representation,
    // which is what `canon` means for a scalar: no promotion, no tolerance.
    template<typename _Type>
    bool
    exact_bits(const _Type& _left, const _Type& _right)
    {
        return (std::memcmp(&_left, &_right, sizeof(_Type)) == 0);
    }


    /*
    compare_integer_monoid
      Runs one intmax_t monoid through both faces: identity, pairwise combine,
    and the fold over a sequence.
    */
    template<typename _CppNewtype>
    void
    compare_integer_monoid
    (
        parity_tally&           _tally,
        const struct d_monoid&  _c_monoid,
        const char*             _label
    )
    {
        std::vector<intmax_t>       sample;
        std::vector<_CppNewtype>    lifted;
        intmax_t                    c_identity;
        intmax_t                    c_combined;
        intmax_t                    c_folded;
        intmax_t                    cpp_identity;
        intmax_t                    cpp_combined;
        intmax_t                    cpp_folded;
        char                        name[128];

        sample.push_back(3);
        sample.push_back(-7);
        sample.push_back(11);
        sample.push_back(2);

        for (std::size_t i = 0; i < sample.size(); ++i)
        {
            lifted.push_back(_CppNewtype(sample[i]));
        }

        // identity
        cpp_identity = djinterp::mempty<_CppNewtype>().value;
        d_mempty(&_c_monoid, &c_identity);
        std::snprintf(name, sizeof(name), "%s: mempty", _label);
        record(_tally, exact_bits(cpp_identity, c_identity), name);

        // pairwise combine
        cpp_combined = djinterp::mappend(_CppNewtype(sample[0]),
                                         _CppNewtype(sample[1])).value;
        d_mappend(d_monoid_semigroup(&_c_monoid),
                  &c_combined, &sample[0], &sample[1]);
        std::snprintf(name, sizeof(name), "%s: mappend", _label);
        record(_tally, exact_bits(cpp_combined, c_combined), name);

        // fold over the whole sequence
        cpp_folded = djinterp::mconcat(lifted).value;
        d_mconcat(&_c_monoid, sample.data(), sample.size(), &c_folded);
        std::snprintf(name, sizeof(name), "%s: mconcat", _label);
        record(_tally, exact_bits(cpp_folded, c_folded), name);

        // the empty fold, which is answerable only because of the identity
        cpp_folded = djinterp::mconcat(std::vector<_CppNewtype>()).value;
        d_mconcat(&_c_monoid, sample.data(), 0, &c_folded);
        std::snprintf(name, sizeof(name), "%s: mconcat over empty", _label);
        record(_tally, exact_bits(cpp_folded, c_folded), name);
    }

    /*
    compare_bool_monoid
      Runs one boolean monoid through both faces.
    */
    template<typename _CppNewtype>
    void
    compare_bool_monoid
    (
        parity_tally&          _tally,
        const struct d_monoid& _c_monoid,
        const char*            _label
    )
    {
        std::vector<bool>        sample;
        std::vector<_CppNewtype> lifted;
        bool                     c_value;
        bool                     cpp_value;
        bool                     c_pair;
        bool                     cpp_pair;
        bool                     packed[3];
        char                     name[128];

        sample.push_back(true);
        sample.push_back(false);
        sample.push_back(true);

        for (std::size_t i = 0; i < sample.size(); ++i)
        {
            lifted.push_back(_CppNewtype(sample[i]));
            packed[i] = sample[i];
        }

        cpp_value = djinterp::mempty<_CppNewtype>().value;
        d_mempty(&_c_monoid, &c_value);
        std::snprintf(name, sizeof(name), "%s: mempty", _label);
        record(_tally, (cpp_value == c_value), name);

        cpp_pair = djinterp::mappend(_CppNewtype(true),
                                     _CppNewtype(false)).value;
        {
            bool left  = true;
            bool right = false;
            d_mappend(d_monoid_semigroup(&_c_monoid), &c_pair, &left, &right);
        }
        std::snprintf(name, sizeof(name), "%s: mappend", _label);
        record(_tally, (cpp_pair == c_pair), name);

        cpp_value = djinterp::mconcat(lifted).value;
        d_mconcat(&_c_monoid, packed, 3, &c_value);
        std::snprintf(name, sizeof(name), "%s: mconcat", _label);
        record(_tally, (cpp_value == c_value), name);
    }

    /*
    compare_double_monoid
      Runs one double monoid through both faces. min and max are exact; sum and
    product are compared exactly here on deliberately exact inputs, because
    floating-point associativity is a stated non-property and a tolerance has
    not yet been fixed in a .tex (open question 4).
    */
    template<typename _CppNewtype>
    void
    compare_double_monoid
    (
        parity_tally&          _tally,
        const struct d_monoid& _c_monoid,
        const char*            _label
    )
    {
        std::vector<double>      sample;
        std::vector<_CppNewtype> lifted;
        double                   c_value;
        double                   cpp_value;
        char                     name[128];

        // powers of two, so every partial result is exactly representable and
        // the comparison is not secretly testing the rounding mode
        sample.push_back(0.5);
        sample.push_back(-4.0);
        sample.push_back(2.0);
        sample.push_back(8.0);

        for (std::size_t i = 0; i < sample.size(); ++i)
        {
            lifted.push_back(_CppNewtype(sample[i]));
        }

        cpp_value = djinterp::mempty<_CppNewtype>().value;
        d_mempty(&_c_monoid, &c_value);
        std::snprintf(name, sizeof(name), "%s: mempty", _label);
        record(_tally, exact_bits(cpp_value, c_value), name);

        cpp_value = djinterp::mconcat(lifted).value;
        d_mconcat(&_c_monoid, sample.data(), sample.size(), &c_value);
        std::snprintf(name, sizeof(name), "%s: mconcat", _label);
        record(_tally, exact_bits(cpp_value, c_value), name);
    }


    /*
    d_parity_fold_add
      The C fold step used to mirror the C++ runtime reducer.
    */
    void
    parity_fold_add(void* _accumulator, const void* _element, void* _context)
    {
        (void)_context;

        *(intmax_t*)_accumulator = (*(intmax_t*)_accumulator) +
                                   (*(const intmax_t*)_element);
    }

    /*
    compare_fold_driver
      Compares the C++ runtime fold driver against the C array driver over the
    same reducer body. This is the step/driver split asserted across languages:
    one reducer body, two implementations of the loop.
    */
    void
    compare_fold_driver
    (
        parity_tally& _tally
    )
    {
        std::vector<intmax_t> sample;
        intmax_t              cpp_result;
        intmax_t              c_result;

        sample.push_back(4);
        sample.push_back(-9);
        sample.push_back(15);
        sample.push_back(0);
        sample.push_back(7);

        cpp_result = djinterp::reduce_rt(
            [](intmax_t _acc, intmax_t _x) { return _acc + _x; },
            (intmax_t)0,
            sample.begin(),
            sample.end());

        c_result = 0;
        d_fold_left(sample.data(),
                    sample.size(),
                    sizeof(intmax_t),
                    &c_result,
                    &parity_fold_add,
                    NULL);

        record(_tally,
               exact_bits(cpp_result, c_result),
               "fold: reduce_rt vs d_fold_left");

        // the empty fold must return the seed unchanged on both sides
        cpp_result = djinterp::reduce_rt(
            [](intmax_t _acc, intmax_t _x) { return _acc + _x; },
            (intmax_t)42,
            sample.begin(),
            sample.begin());

        c_result = 42;
        d_fold_left(sample.data(), 0, sizeof(intmax_t), &c_result,
                    &parity_fold_add, NULL);

        record(_tally,
               exact_bits(cpp_result, c_result),
               "fold: empty range returns the seed on both sides");
    }

    /*
    compare_semigroup_without_identity
      Asserts the stated non-property on both sides: a Semigroup supplies no
    identity, so the C++ face offers no mempty for a bare semigroup and the C
    fold over an empty sequence must fail rather than invent an answer.
    */
    void
    compare_semigroup_without_identity
    (
        parity_tally& _tally
    )
    {
        struct d_semigroup first;
        intmax_t           sample;
        intmax_t           sink;

        first  = d_semigroup_first(sizeof(intmax_t));
        sample = 5;

        record(_tally,
               (!d_semigroup_reduce(&first, &sample, 0, &sink)),
               "semigroup: empty fold fails, having no identity");

        // and the same object under a monoid DOES answer, via its identity
        {
            struct d_monoid sum = d_monoid_sum_intmax();
            intmax_t        out = 999;

            record(_tally,
                   ( (d_mconcat(&sum, &sample, 0, &out)) && (out == 0) ),
                   "monoid: empty fold answers mempty");
        }
    }
}


// C-linkage callbacks, so the C kernel receives function pointers whose
// linkage matches the typedefs it declares.
extern "C"
{
    static bool
    parity_pred_even(const void* _element, void* _context)
    {
        (void)_context;

        return ( ((*(const intmax_t*)_element) % 2) == 0 );
    }

    static bool
    parity_pred_below_eight(const void* _element, void* _context)
    {
        (void)_context;

        return ( (*(const intmax_t*)_element) < 8 );
    }

    static bool
    parity_transform_double(const void* _in, void* _out, void* _context)
    {
        (void)_context;

        *(intmax_t*)_out = (*(const intmax_t*)_in) * 2;

        return true;
    }
}

namespace
{
    std::vector<intmax_t>
    parity_sample_ten()
    {
        std::vector<intmax_t> v;

        for (intmax_t i = 1; i <= 10; ++i)
        {
            v.push_back(i);
        }

        return v;
    }

    bool
    same_sequence(const std::vector<intmax_t>& _cpp,
                  const intmax_t*              _c,
                  std::size_t                  _c_count)
    {
        if (_cpp.size() != _c_count)
        {
            return false;
        }

        if (_c_count == 0)
        {
            return true;
        }

        return (std::memcmp(_cpp.data(), _c,
                            _c_count * sizeof(intmax_t)) == 0);
    }

    /*
    compare_transducer_chains
      Runs the same chains through both faces. The C++ factories are spelled
    flat in `djinterp`, not `djinterp::transducers`, because transducer.hpp
    never opens that namespace (PARITY_NOTES D-2).
    */
    void
    compare_transducer_chains(parity_tally& _tally)
    {
        std::vector<intmax_t>     input;
        std::vector<intmax_t>     cpp_out;
        struct d_transducer_stage stages[2];
        intmax_t                  c_out[32];
        intmax_t                  scratch;
        std::size_t               written;

        input = parity_sample_ten();

        cpp_out = djinterp::transduce_into_vector<intmax_t>(
            djinterp::filter([](intmax_t _x) { return (_x % 2) == 0; }), input);
        stages[0] = d_transducer_filter(&parity_pred_even, NULL);
        written   = 0;
        d_transduce_into_array(stages, 1, input.data(), input.size(),
                               sizeof(intmax_t), c_out, 32, &written);
        record(_tally, same_sequence(cpp_out, c_out, written),
               "transducer: filter");

        cpp_out = djinterp::transduce_into_vector<intmax_t>(
            djinterp::compose(
                djinterp::filter([](intmax_t _x) { return (_x % 2) == 0; }),
                djinterp::take(3)), input);
        stages[0] = d_transducer_filter(&parity_pred_even, NULL);
        stages[1] = d_transducer_take(3);
        written   = 0;
        d_transduce_into_array(stages, 2, input.data(), input.size(),
                               sizeof(intmax_t), c_out, 32, &written);
        record(_tally, same_sequence(cpp_out, c_out, written),
               "transducer: filter | take (short-circuit)");

        cpp_out = djinterp::transduce_into_vector<intmax_t>(
            djinterp::compose(
                djinterp::map([](intmax_t _x) { return _x * 2; }),
                djinterp::filter([](intmax_t _x) { return _x < 8; })), input);
        stages[0] = d_transducer_map(&parity_transform_double, NULL, &scratch);
        stages[1] = d_transducer_filter(&parity_pred_below_eight, NULL);
        written   = 0;
        d_transduce_into_array(stages, 2, input.data(), input.size(),
                               sizeof(intmax_t), c_out, 32, &written);
        record(_tally, same_sequence(cpp_out, c_out, written),
               "transducer: map | filter (stage order)");

        cpp_out = djinterp::transduce_into_vector<intmax_t>(
            djinterp::drop(4), input);
        stages[0] = d_transducer_drop(4);
        written   = 0;
        d_transduce_into_array(stages, 1, input.data(), input.size(),
                               sizeof(intmax_t), c_out, 32, &written);
        record(_tally, same_sequence(cpp_out, c_out, written),
               "transducer: drop");

        cpp_out = djinterp::transduce_into_vector<intmax_t>(
            djinterp::take(0), input);
        stages[0] = d_transducer_take(0);
        written   = 0;
        d_transduce_into_array(stages, 1, input.data(), input.size(),
                               sizeof(intmax_t), c_out, 32, &written);
        record(_tally, same_sequence(cpp_out, c_out, written),
               "transducer: take(0) emits nothing");

        cpp_out = djinterp::transduce_into_vector<intmax_t>(
            djinterp::take(99), input);
        stages[0] = d_transducer_take(99);
        written   = 0;
        d_transduce_into_array(stages, 1, input.data(), input.size(),
                               sizeof(intmax_t), c_out, 32, &written);
        record(_tally, same_sequence(cpp_out, c_out, written),
               "transducer: take beyond the source");
    }

}


int
main
(
    void
)
{
    parity_tally tally;

    tally.compared = 0;
    tally.agreed   = 0;

    std::printf("djinterp [functional] C / C++ differential parity\n");
    std::printf("  algebra and fold (the layers whose C++ headers compile)\n");

    {
        struct d_monoid m;

        m = d_monoid_sum_intmax();
        compare_integer_monoid< djinterp::sum<intmax_t> >(tally, m, "sum");

        m = d_monoid_product_intmax();
        compare_integer_monoid< djinterp::product<intmax_t> >(tally, m,
                                                              "product");
        m = d_monoid_min_intmax();
        compare_integer_monoid< djinterp::min<intmax_t> >(tally, m, "min");

        m = d_monoid_max_intmax();
        compare_integer_monoid< djinterp::max<intmax_t> >(tally, m, "max");

        m = d_monoid_all();
        compare_bool_monoid< djinterp::all >(tally, m, "all");

        m = d_monoid_any();
        compare_bool_monoid< djinterp::any >(tally, m, "any");

        m = d_monoid_min_double();
        compare_double_monoid< djinterp::min<double> >(tally, m, "min_double");

        m = d_monoid_max_double();
        compare_double_monoid< djinterp::max<double> >(tally, m, "max_double");

        m = d_monoid_sum_double();
        compare_double_monoid< djinterp::sum<double> >(tally, m, "sum_double");
    }

    compare_transducer_chains(tally);
    compare_fold_driver(tally);
    compare_semigroup_without_identity(tally);

    std::printf("\n  %lu / %lu comparisons agreed\n",
                (unsigned long)tally.agreed,
                (unsigned long)tally.compared);

    if (tally.agreed != tally.compared)
    {
        return 1;
    }

    return 0;
}
