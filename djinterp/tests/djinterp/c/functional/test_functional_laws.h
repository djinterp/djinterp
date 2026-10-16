/*******************************************************************************
* djinterp [functional]                                   test_functional_laws.h
*
* Conformance declarations for the C functional dataflow and algebra modules.
*   One group per formal object. Each group asserts the claims the subframework
* makes AND the claims it deliberately does not make: that `all` over an empty
* source is vacuously true, that a semigroup fold over an empty source fails for
* want of an identity, that `first` admits no identity at all, and that
* floating-point addition is associative only up to rounding. A conformance test
* that only checks the positive claims lets a later change "fix" a stated
* non-property without anything noticing.
*   This is the conformance suite, not the differential suite. The parity law
* compares C against C++ over the same inputs, and the C++ sources for this
* subframework were not supplied, so that comparison cannot be written yet. The
* nearest available substitute is here in two forms: one reducer driven over an
* array and over a producer, and one transducer chain driven the same two ways,
* each required to agree byte for byte.
*
*
* path:      /tests/djinterp/c/functional/test_functional_laws.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_C_TEST_FUNCTIONAL_LAWS_
#define DJINTERP_C_TEST_FUNCTIONAL_LAWS_ 1

// std
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "../../../../inc/djinterp/c/functional/reducer.h"
#include "../../../../inc/djinterp/c/functional/producer.h"
#include "../../../../inc/djinterp/c/functional/transducer.h"
#include "../../../../inc/djinterp/c/functional/semigroup.h"
#include "../../../../inc/djinterp/c/functional/monoid.h"


// d_test_tally
//   struct: the running count of conformance assertions and their outcomes.
struct d_test_tally
{
    size_t checked;     // assertions attempted
    size_t passed;      // assertions that held
};

// d_test_probe
//   struct: observation state passed as a callable's context.
// `calls` counts invocations, which is how short-circuiting and latching are
// measured rather than assumed; `target` is the operand a comparing predicate
// tests against.
struct d_test_probe
{
    size_t   calls;     // how many times the callable ran
    intmax_t target;    // the value a comparing predicate tests against
};

// I.     conformance groups
bool     d_tests_semigroup_laws(struct d_test_tally* _tally);
bool     d_tests_fold_and_quantifiers(struct d_test_tally* _tally);
bool     d_tests_producer_protocol(struct d_test_tally* _tally);
bool     d_tests_transducer_composition(struct d_test_tally* _tally);
bool     d_tests_cost_and_layout(struct d_test_tally* _tally);


#endif  // DJINTERP_C_TEST_FUNCTIONAL_LAWS_
