/*******************************************************************************
* djinterp [test]                                         t_option_set_compact.c
*
*   The option core's removal and compaction (option_set_common.h): removing
* by key and by index keeps the other cells in their order and moves no
* value; compaction gives the holes back, keeps every value and every slot's
* alignment, and works whatever order the cells' offsets are in, canon's
* included. A model carries a set through random adds, removals and
* compactions, and every value is read back against it after each step.
*   A standalone program, as the tests not yet in DTest are: a line per group,
* one per failed check, "N checks, M failed" at the end, and exit 0 only when
* nothing failed.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                  \
*        src/djinterp/c/option/option_set_common.c                             \
*        src/djinterp/c/option/option_common.c                                 \
*        tests/djinterp/c/option/t_option_set_compact.c -o t_option_set_compact
*
*
* path:      /tests/djinterp/c/option/t_option_set_compact.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // printf
#include <string.h>   // memcmp, memset
// djinterp
#include "../../../../inc/djinterp/c/option/option_set_common.h"  // the set
#include "../../../../inc/djinterp/c/meta/type_info.h"            // D_TYPE_*
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t, uint64_t


// D_TESTS_OPTION_SET_CHECK
//   macro: one check: counts it, and prints the expression and line when it
// fails.
#define D_TESTS_OPTION_SET_CHECK(expr)                                         \
    d_tests_option_set_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_OPTION_SET_CELLS
//   constant: the cells the random set holds at most.
#define D_TESTS_OPTION_SET_CELLS  24u

// D_TESTS_OPTION_SET_BYTES
//   constant: its value block: room for every cell at its widest, so only
// the holes removal leaves can fill it.
#define D_TESTS_OPTION_SET_BYTES  512u

static unsigned long d_tests_option_set_checks;
static unsigned long d_tests_option_set_failures;
static uint64_t      d_tests_option_set_state = 0x9E3779B97F4A7C15u;

D_OPTION_SET_DECLARE(d_tests_option_set_storage,
                     D_TESTS_OPTION_SET_CELLS,
                     D_TESTS_OPTION_SET_BYTES);

/*
d_tests_option_set_check
  Counts one check, and reports it when it failed.
*/
static bool
d_tests_option_set_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    ++d_tests_option_set_checks;

    // a failure says what and where
    if (_ok == 0)
    {
        ++d_tests_option_set_failures;
        printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return (_ok != 0);
}

/*
d_tests_option_set_next
  xorshift64: the random walk's numbers, the same every run.
*/
static uint64_t
d_tests_option_set_next(void)
{
    d_tests_option_set_state ^= d_tests_option_set_state << 13;
    d_tests_option_set_state ^= d_tests_option_set_state >> 7;
    d_tests_option_set_state ^= d_tests_option_set_state << 17;

    return d_tests_option_set_state;
}

/*
d_tests_option_set_add
  Adds a cell of `_size` bytes at its natural alignment, filled with a
pattern of its key, and reports the index.
*/
static int
d_tests_option_set_add(
    struct dj_option_set* _set,
    uint64_t              _key,
    uint32_t              _size
)
{
    unsigned char  bytes[16];
    const uint32_t lowbit = _size & (~_size + 1u);
    const uint32_t align  = (lowbit < 8u) ? lowbit : 8u;

    const struct dj_option_result added =
        d_option_set_add(_set,
                         _key,
                         (dj_type_info16)D_TYPE_ULLONG_(),
                         (dj_type_info16)D_TYPE_UCHAR_(),
                         _size,
                         align);

    // a refusal is the caller's to judge
    if (!added.is_ok)
    {
        return -1;
    }

    for (uint32_t i = 0u; i < _size; ++i)
    {
        bytes[i] = (unsigned char)((_key * 31u) + i);
    }

    (void)d_option_set_set(_set, _key, bytes, (size_t)_size);

    return (int)added.payload.value;
}

/*
d_tests_option_set_holds
  Whether the cell with `_key` holds its pattern.
*/
static int
d_tests_option_set_holds(
    const struct dj_option_set* _set,
    uint64_t                    _key,
    uint32_t                    _size
)
{
    unsigned char expected[16];
    unsigned char read[16];

    for (uint32_t i = 0u; i < _size; ++i)
    {
        expected[i] = (unsigned char)((_key * 31u) + i);
    }

    return ( (d_option_set_get(_set, _key, read, (size_t)_size).is_ok) &&
             (memcmp(read, expected, (size_t)_size) == 0) ) ? 1 : 0;
}

/*
d_tests_option_set_disjoint
  Whether every slot lies inside `value_used`, at its natural alignment, and
overlaps no other.
*/
static int
d_tests_option_set_disjoint(
    const struct dj_option_set* _set
)
{
    for (uint32_t i = 0u; i < _set->count; ++i)
    {
        const struct dj_option* const a      = &_set->options[i];
        const uint32_t                lowbit = a->value_size &
                                               (~a->value_size + 1u);
        const uint32_t                align  = (lowbit < 8u) ? lowbit : 8u;

        if ( (a->value_size != 0u) &&
             ( (a->value_offset + a->value_size > _set->value_used) ||
               ((a->value_offset % align) != 0u) ) )
        {
            return 0;
        }

        for (uint32_t j = i + 1u; j < _set->count; ++j)
        {
            const struct dj_option* const b = &_set->options[j];

            if ( (a->value_size != 0u)                          &&
                 (b->value_size != 0u)                          &&
                 (a->value_offset < b->value_offset + b->value_size) &&
                 (b->value_offset < a->value_offset + a->value_size) )
            {
                return 0;
            }
        }
    }

    return 1;
}

/*
d_tests_option_set_removal
  Tests the following:
  - removal by key reports the index, keeps the others in order, and moves
    no value; an absent key and a null set are refused
  - removal by index, at both ends, and past the end
  - a unary cell is removed like any other
*/
static void
d_tests_option_set_removal(void)
{
    struct d_tests_option_set_storage storage = D_OPTION_SET_STORAGE_INIT;
    struct dj_option_set              set     = D_OPTION_SET_VIEW(storage);

    printf("  removal\n");

    for (uint64_t key = 1u; key <= 5u; ++key)
    {
        (void)d_tests_option_set_add(&set, key, 4u);
    }

    const uint32_t                offset  = set.options[3].value_offset;
    const uint32_t                used    = set.value_used;
    const struct dj_option_result removed = d_option_set_remove(&set, 2u);

    D_TESTS_OPTION_SET_CHECK( (removed.is_ok) &&
                              (removed.payload.value == 1u) );
    D_TESTS_OPTION_SET_CHECK(set.count == 4u);
    D_TESTS_OPTION_SET_CHECK( (set.options[0].key == 1u) &&
                              (set.options[1].key == 3u) &&
                              (set.options[2].key == 4u) &&
                              (set.options[3].key == 5u) );
    D_TESTS_OPTION_SET_CHECK(set.options[2].value_offset == offset);
    D_TESTS_OPTION_SET_CHECK(set.value_used == used);
    D_TESTS_OPTION_SET_CHECK(!d_option_set_contains(&set, 2u));
    D_TESTS_OPTION_SET_CHECK(d_tests_option_set_holds(&set, 4u, 4u) != 0);
    D_TESTS_OPTION_SET_CHECK(d_option_set_is_valid(&set));

    // what is refused
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove(&set, 2u).payload.error ==
                             (int32_t)D_OPTION_STATUS_KEY_ABSENT);
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove(NULL, 1u).payload.error ==
                             (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove_at(&set, 4u).payload.error ==
                             (int32_t)D_OPTION_STATUS_KEY_ABSENT);
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove_at(NULL, 0u).payload.error ==
                             (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);

    // both ends
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove_at(&set, 3u).is_ok);
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove_at(&set, 0u).is_ok);
    D_TESTS_OPTION_SET_CHECK( (set.count == 2u) &&
                              (set.options[0].key == 3u) &&
                              (set.options[1].key == 4u) );

    // a unary cell, and the set emptied
    D_TESTS_OPTION_SET_CHECK(d_option_set_add_unary(&set,
                                                    9u,
                                                    (dj_type_info16)
                                                        D_TYPE_ULLONG_())
                                 .is_ok);
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove(&set, 9u).is_ok);
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove(&set, 3u).is_ok);
    D_TESTS_OPTION_SET_CHECK(d_option_set_remove(&set, 4u).is_ok);
    D_TESTS_OPTION_SET_CHECK(d_option_set_empty(&set));

    return;
}

/*
d_tests_option_set_compaction
  Tests the following:
  - compaction gives back exactly the bytes removal left, keeps every value
    and the cells' order, and reports how many bytes it gave back
  - a full value block takes a new cell after compaction
  - offsets out of order -- after canon -- compact as well, and so does a
    set with nothing to give back, or nothing in it, and a unary cell
*/
static void
d_tests_option_set_compaction(void)
{
    struct d_tests_option_set_storage storage = D_OPTION_SET_STORAGE_INIT;
    struct dj_option_set              set     = D_OPTION_SET_VIEW(storage);

    printf("  compaction\n");

    // 1, 8, 2, 8, 4: packed at 0, 8, 16, 24, 32; value_used 36
    (void)d_tests_option_set_add(&set, 10u, 1u);
    (void)d_tests_option_set_add(&set, 11u, 8u);
    (void)d_tests_option_set_add(&set, 12u, 2u);
    (void)d_tests_option_set_add(&set, 13u, 8u);
    (void)d_tests_option_set_add(&set, 14u, 4u);
    D_TESTS_OPTION_SET_CHECK(set.value_used == 36u);

    (void)d_option_set_remove(&set, 11u);

    const struct dj_option_result given = d_option_set_compact(&set);

    // 1 at 0, 2 at 2, 8 at 8, 4 at 16: 20 bytes
    D_TESTS_OPTION_SET_CHECK( (given.is_ok) &&
                              (given.payload.value == 16u) );
    D_TESTS_OPTION_SET_CHECK(set.value_used == 20u);
    D_TESTS_OPTION_SET_CHECK( (set.options[0].key == 10u) &&
                              (set.options[1].key == 12u) &&
                              (set.options[2].key == 13u) &&
                              (set.options[3].key == 14u) );
    D_TESTS_OPTION_SET_CHECK( (d_tests_option_set_holds(&set, 10u, 1u) != 0) &&
                              (d_tests_option_set_holds(&set, 12u, 2u) != 0) &&
                              (d_tests_option_set_holds(&set, 13u, 8u) != 0) &&
                              (d_tests_option_set_holds(&set, 14u, 4u) != 0) );
    D_TESTS_OPTION_SET_CHECK(d_tests_option_set_disjoint(&set) != 0);
    D_TESTS_OPTION_SET_CHECK(d_option_set_compact(&set).payload.value == 0u);

    // out of order: canon puts 14 (at 16) before... it is last already, so
    // reverse the keys by removing and re-adding in descending order
    (void)d_option_set_canon(&set);
    (void)d_option_set_remove(&set, 12u);
    (void)d_tests_option_set_add(&set, 5u, 8u);
    (void)d_option_set_canon(&set);
    D_TESTS_OPTION_SET_CHECK(set.options[0].key == 5u);
    D_TESTS_OPTION_SET_CHECK(set.options[0].value_offset >
                             set.options[1].value_offset);
    (void)d_option_set_compact(&set);
    D_TESTS_OPTION_SET_CHECK( (d_tests_option_set_holds(&set, 5u, 8u) != 0)  &&
                              (d_tests_option_set_holds(&set, 10u, 1u) != 0) &&
                              (d_tests_option_set_holds(&set, 13u, 8u) != 0) &&
                              (d_tests_option_set_holds(&set, 14u, 4u) != 0) );
    D_TESTS_OPTION_SET_CHECK(d_tests_option_set_disjoint(&set) != 0);
    D_TESTS_OPTION_SET_CHECK(set.value_used == 32u);

    // a unary cell, a null set, an empty one
    D_TESTS_OPTION_SET_CHECK(d_option_set_add_unary(&set,
                                                    77u,
                                                    (dj_type_info16)
                                                        D_TYPE_ULLONG_())
                                 .is_ok);
    D_TESTS_OPTION_SET_CHECK(d_option_set_compact(&set).is_ok);
    D_TESTS_OPTION_SET_CHECK(d_option_set_contains(&set, 77u));
    D_TESTS_OPTION_SET_CHECK(d_option_set_compact(NULL).payload.error ==
                             (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    d_option_set_clear(&set);
    D_TESTS_OPTION_SET_CHECK( (d_option_set_compact(&set).is_ok) &&
                              (set.value_used == 0u) );

    return;
}

/*
d_tests_option_set_full_block
  Tests the following:
  - a value block filled to its last byte refuses a new cell as
    VALUE_CAPACITY, and takes it once removal and compaction made room
*/
static void
d_tests_option_set_full_block(void)
{
    D_OPTION_SET_DECLARE(d_tests_small_storage, 8u, 24u);
    struct d_tests_small_storage storage = D_OPTION_SET_STORAGE_INIT;
    struct dj_option_set         set     = D_OPTION_SET_VIEW(storage);

    printf("  a full value block\n");

    (void)d_tests_option_set_add(&set, 1u, 8u);
    (void)d_tests_option_set_add(&set, 2u, 8u);
    (void)d_tests_option_set_add(&set, 3u, 8u);
    D_TESTS_OPTION_SET_CHECK(d_tests_option_set_add(&set, 4u, 8u) < 0);
    (void)d_option_set_remove(&set, 1u);
    D_TESTS_OPTION_SET_CHECK(d_tests_option_set_add(&set, 4u, 8u) < 0);
    (void)d_option_set_compact(&set);
    D_TESTS_OPTION_SET_CHECK(d_tests_option_set_add(&set, 4u, 8u) == 2);
    D_TESTS_OPTION_SET_CHECK( (d_tests_option_set_holds(&set, 2u, 8u) != 0) &&
                              (d_tests_option_set_holds(&set, 3u, 8u) != 0) &&
                              (d_tests_option_set_holds(&set, 4u, 8u) != 0) );

    return;
}

/*
d_tests_option_set_random
  Tests the following:
  - 20000 random adds, removals, canonicalisations and compactions; after
    each, every key the model holds reads its pattern, no other key is
    present, and the slots are disjoint and aligned; after a compaction no
    more than seven bytes of padding per slot remain
*/
static void
d_tests_option_set_random(void)
{
    static const uint32_t             SIZES[] = { 1u, 2u, 3u, 4u, 6u,
                                                  8u, 12u, 16u };
    struct d_tests_option_set_storage storage = D_OPTION_SET_STORAGE_INIT;
    struct dj_option_set              set     = D_OPTION_SET_VIEW(storage);
    uint32_t                          model[64];
    unsigned long                     wrong   = 0u;

    printf("  random changes against a model\n");

    memset(model, 0, sizeof(model));

    for (unsigned long step = 0u; step < 20000u; ++step)
    {
        const uint64_t roll = d_tests_option_set_next();
        const uint64_t key  = (roll >> 8) % 64u;

        // half adds, a third removals, the rest canon or compaction
        if ((roll % 6u) < 3u)
        {
            const uint32_t size = SIZES[(roll >> 16) % 8u];

            if ( (model[key] == 0u) &&
                 (d_tests_option_set_add(&set, key, size) >= 0) )
            {
                model[key] = size;
            }
            else if (model[key] == 0u)
            {
                (void)d_option_set_compact(&set);
            }
        }
        else if ((roll % 6u) < 5u)
        {
            const struct dj_option_result removed =
                d_option_set_remove(&set, key);

            wrong += (removed.is_ok != (model[key] != 0u)) ? 1u : 0u;
            model[key] = 0u;
        }
        else if (((roll >> 4) % 2u) == 0u)
        {
            (void)d_option_set_canon(&set);
        }
        else
        {
            uint32_t slots = 0u;
            uint32_t bytes = 0u;

            (void)d_option_set_compact(&set);

            for (uint32_t k = 0u; k < 64u; ++k)
            {
                slots += (model[k] != 0u) ? 1u : 0u;
                bytes += model[k];
            }

            wrong += (set.value_used > bytes + (7u * slots)) ? 1u : 0u;
        }

        for (uint32_t k = 0u; k < 64u; ++k)
        {
            const int present = d_option_set_contains(&set, k) ? 1 : 0;

            wrong += ( (present != (model[k] != 0u)) ||
                       ( (model[k] != 0u) &&
                         (d_tests_option_set_holds(&set, k, model[k]) == 0) ) )
                         ? 1u
                         : 0u;
        }

        wrong += (d_tests_option_set_disjoint(&set) == 0) ? 1u : 0u;
    }

    if (wrong != 0u)
    {
        printf("    %lu wrong reads\n", wrong);
    }

    D_TESTS_OPTION_SET_CHECK(wrong == 0u);
    D_TESTS_OPTION_SET_CHECK(d_option_set_is_valid(&set));

    return;
}

/*
main
  Every group, then the count.
*/
int
main(void)
{
    printf("t_option_set_compact\n");

    d_tests_option_set_removal();
    d_tests_option_set_compaction();
    d_tests_option_set_full_block();
    d_tests_option_set_random();

    printf("%lu checks, %lu failed\n",
           d_tests_option_set_checks,
           d_tests_option_set_failures);

    return (d_tests_option_set_failures == 0u) ? 0 : 1;
}
