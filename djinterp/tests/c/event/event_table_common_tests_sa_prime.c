#include "./event_table_common_tests_sa.h"


/******************************************************************************
 * III. PRIME NUMBER UTILITY TESTS
 *****************************************************************************/

// d_tests_sa_etc_is_prime
//   helper: verifies a value is prime by trial division.
static bool
d_tests_sa_etc_is_prime(size_t _n)
{
    size_t i;

    if (_n < 2)
    {
        return false;
    }

    if (_n == 2)
    {
        return true;
    }

    if (_n % 2 == 0)
    {
        return false;
    }

    for (i = 3; i * i <= _n; i += 2)
    {
        if (_n % i == 0)
        {
            return false;
        }
    }

    return true;
}


/*
d_tests_sa_event_hash_next_prime_basic
  Tests d_event_hash_next_prime with small inputs.
  Tests the following:
  - Input 1 returns 2
  - Input 2 returns 2
  - Input 3 returns 3
  - Input 4 returns 5
*/
bool
d_tests_sa_event_hash_next_prime_basic
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_hash_next_prime(1) == 2,
        "prime_after_1",
        "Next prime after 1 should be 2",
        _counter) && result;

    result = d_assert_standalone(
        d_event_hash_next_prime(2) == 2,
        "prime_after_2",
        "2 is prime, should return 2",
        _counter) && result;

    result = d_assert_standalone(
        d_event_hash_next_prime(3) == 3,
        "prime_after_3",
        "3 is prime, should return 3",
        _counter) && result;

    result = d_assert_standalone(
        d_event_hash_next_prime(4) == 5,
        "prime_after_4",
        "Next prime after 4 should be 5",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_next_prime_known
  Tests d_event_hash_next_prime with known values.
  Tests the following:
  - Input 10 returns 11
  - Input 100 returns 101
  - Input 1000 returns 1009
  - Returned values are actually prime
*/
bool
d_tests_sa_event_hash_next_prime_known
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t p;

    result = true;

    result = d_assert_standalone(
        d_event_hash_next_prime(10) == 11,
        "prime_after_10",
        "Next prime after 10 should be 11",
        _counter) && result;

    result = d_assert_standalone(
        d_event_hash_next_prime(100) == 101,
        "prime_after_100",
        "Next prime after 100 should be 101",
        _counter) && result;

    result = d_assert_standalone(
        d_event_hash_next_prime(1000) == 1009,
        "prime_after_1000",
        "Next prime after 1000 should be 1009",
        _counter) && result;

    // verify 50 -> prime >= 50 and is actually prime
    p = d_event_hash_next_prime(50);

    result = d_assert_standalone(
        p >= 50 && d_tests_sa_etc_is_prime(p),
        "prime_after_50_valid",
        "Result should be >= 50 and actually prime",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_next_prime_edge
  Tests d_event_hash_next_prime with edge-case inputs.
  Tests the following:
  - Input 0 returns 2
  - Even input returns odd prime >= input
*/
bool
d_tests_sa_event_hash_next_prime_edge
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t p;

    result = true;

    result = d_assert_standalone(
        d_event_hash_next_prime(0) == 2,
        "prime_after_0",
        "Next prime after 0 should be 2",
        _counter) && result;

    p = d_event_hash_next_prime(20);

    result = d_assert_standalone(
        p >= 20 && (p == 2 || p % 2 != 0),
        "prime_after_even",
        "Result after even input should be odd prime >= 20",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_next_prime_sequence
  Tests that d_event_hash_next_prime generates correct prime sequences.
  Tests the following:
  - Generated sequence is monotonically increasing
  - All values in sequence are prime
*/
bool
d_tests_sa_event_hash_next_prime_sequence
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t primes[10];
    size_t current;
    bool   is_increasing;
    bool   all_prime;
    int    i;

    result  = true;
    current = 2;

    for (i = 0; i < 10; i++)
    {
        primes[i] = d_event_hash_next_prime(current);
        current   = primes[i] + 1;
    }

    // verify monotonically increasing
    is_increasing = true;

    for (i = 1; i < 10; i++)
    {
        if (primes[i] <= primes[i - 1])
        {
            is_increasing = false;
            break;
        }
    }

    result = d_assert_standalone(
        is_increasing,
        "prime_sequence_increasing",
        "Prime sequence should be monotonically increasing",
        _counter) && result;

    // verify all prime
    all_prime = true;

    for (i = 0; i < 10; i++)
    {
        if (!d_tests_sa_etc_is_prime(primes[i]))
        {
            all_prime = false;
            break;
        }
    }

    result = d_assert_standalone(
        all_prime,
        "prime_sequence_all_prime",
        "All values in sequence should be prime",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_next_prime_large
  Tests d_event_hash_next_prime with large input.
  Tests the following:
  - Result is >= 10000 and is actually prime
*/
bool
d_tests_sa_event_hash_next_prime_large
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t p;

    result = true;
    p      = d_event_hash_next_prime(10000);

    result = d_assert_standalone(
        p >= 10000 && d_tests_sa_etc_is_prime(p),
        "prime_large",
        "Large input should find valid prime >= 10000",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_prime_all
  Runs all prime number utility tests.
*/
bool
d_tests_sa_event_hash_prime_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Prime Number Utilities\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_event_hash_next_prime_basic(_counter)    && result;
    result = d_tests_sa_event_hash_next_prime_known(_counter)    && result;
    result = d_tests_sa_event_hash_next_prime_edge(_counter)     && result;
    result = d_tests_sa_event_hash_next_prime_sequence(_counter) && result;
    result = d_tests_sa_event_hash_next_prime_large(_counter)    && result;

    return result;
}
