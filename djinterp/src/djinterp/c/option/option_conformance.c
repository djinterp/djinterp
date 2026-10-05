/*******************************************************************************
* djinterp [c]                                              option_conformance.c
*
*   The conformance suite for body-options.tex. Goal 1: "Every formal relation
* and operator ships with tests that transcribe the .tex's own claims --
* INCLUDING ITS STATED NON-PROPERTIES."
*
*   Two tests here exist for that second clause and are the reason the file is
* worth reading:
*
*     C05  agreement is NOT transitive
*     C13  (+) is NOT commutative
*
*   Both would still pass if someone "simplified" the relation they guard --
* which is exactly why they assert the failure rather than the success. A suite
* that only tested the positive properties would go green on a tolerance
* silently promoted to an equivalence, and that promotion is a real temptation:
* an equivalence can deduplicate, and a tolerance cannot.
*
*
* path:      /src/djinterp/c/option/option_conformance.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

// std
#include <stdio.h>
#include <string.h>
// djinterp
#include "../../../../inc/djinterp/c/option/option_override_common.h"

static int g_pass = 0;
static int g_fail = 0;

static void
check(
    const char* _id,
    const char* _claim,
    bool        _held
)
{
    if (_held)
    {
        ++g_pass;
        printf("  pass  %-5s %s\n", _id, _claim);
    }
    else
    {
        ++g_fail;
        printf("  FAIL  %-5s %s\n", _id, _claim);
    }

    return;
}


// ---------------------------------------------------------------------------
//   fixtures: small int32 sets over one enum key type.
// ---------------------------------------------------------------------------

enum k { k_foo = 1, k_bar = 2, k_baz = 3 };

#define CAP     8
#define VBYTES  128

struct fixture
{
    uint64_t            block[VBYTES / 8];
    struct d_option     cells[CAP];
    struct d_option_set set;
};

static void
fx_init(
    struct fixture* _f
)
{
    memset(_f, 0, sizeof(*_f));
    _f->set = d_option_set_view(_f->cells, (unsigned char*)_f->block,
                                CAP, VBYTES);
    return;
}

static void
fx_put(
    struct fixture* _f,
    uint64_t        _key,
    int32_t         _value
)
{
    (void)d_option_set_add(&_f->set, _key, (d_type_info16)7,
                           (d_type_info16)9, (uint32_t)sizeof(int32_t), 4u);
    (void)d_option_set_set(&_f->set, _key, &_value, sizeof(int32_t));
    return;
}

static bool
same(
    const struct d_option_set* _a,
    const struct d_option_set* _b
)
{
    return d_option_set_equal(_a, _b, NULL, NULL, NULL);
}


int
main(void)
{
    struct fixture a;
    struct fixture b;
    struct fixture c;
    struct fixture o1;
    struct fixture o2;
    struct fixture o3;
    struct fixture sc;

    printf("body-options.tex conformance\n");
    printf("============================\n\n");

    // -- Definitions -------------------------------------------------------
    printf("Definitions\n");

    fx_init(&a);
    fx_put(&a, k_foo, 1);
    {
        struct d_option_result dup =
            d_option_set_add(&a.set, k_foo, (d_type_info16)7,
                             (d_type_info16)9, 4u, 4u);
        check("C01", "k(.) is injective -- duplicate yields KEY_DUPLICATE",
              (!dup.is_ok) &&
              (dup.payload.error == (int32_t)D_OPTION_STATUS_KEY_DUPLICATE));
    }

    check("C02", "the set's own invariant predicate holds",
          d_option_set_is_valid(&a.set));

    // identity is key AND value -- the relation kv_pair::operator== gets wrong
    fx_init(&b);
    fx_put(&b, k_foo, 99);
    check("C03", "identity is key AND value: same key, different value "
                 "is NOT identical",
          !d_option_eq(&a.set.options[0], a.set.values,
                       &b.set.options[0], b.set.values, NULL, NULL, NULL));
    check("C04", "the key-only relation still reports them equal "
                 "(the two relations are distinct)",
          d_option_key_eq(&a.set.options[0], &b.set.options[0]));

    // -- Comparing option sets ---------------------------------------------
    printf("\nComparing option sets\n");

    //   the counterexample the .tex's table describes, built exactly:
    //       O1 = { (foo,1) }   O2 = { (bar,2) }   O3 = { (foo,99) }
    fx_init(&o1); fx_put(&o1, k_foo, 1);
    fx_init(&o2); fx_put(&o2, k_bar, 2);
    fx_init(&o3); fx_put(&o3, k_foo, 99);

    check("C05a", "agreement is reflexive",
          d_option_set_agrees(&o1.set, &o1.set, NULL, NULL, NULL));
    check("C05b", "agreement is symmetric",
          d_option_set_agrees(&o1.set, &o2.set, NULL, NULL, NULL) ==
          d_option_set_agrees(&o2.set, &o1.set, NULL, NULL, NULL));
    check("C05c", "disjoint key sets agree VACUOUSLY",
          d_option_set_agrees(&o1.set, &o2.set, NULL, NULL, NULL));

    //   THE STATED NON-PROPERTY. O1~O2 and O2~O3 but NOT O1~O3.
    check("C05", "*** agreement is NOT transitive ***",
          d_option_set_agrees(&o1.set, &o2.set, NULL, NULL, NULL) &&
          d_option_set_agrees(&o2.set, &o3.set, NULL, NULL, NULL) &&
          (!d_option_set_agrees(&o1.set, &o3.set, NULL, NULL, NULL)));

    check("C06", "agreement does not imply equality "
                 "(disjoint sets agree, are not equal)",
          d_option_set_agrees(&o1.set, &o2.set, NULL, NULL, NULL) &&
          (!same(&o1.set, &o2.set)));

    {
        uint64_t witness = 0u;

        check("C07", "conflict is exactly the negation of agreement",
              d_option_set_conflicts(&o1.set, &o3.set, NULL, NULL, NULL,
                                     &witness) ==
              (!d_option_set_agrees(&o1.set, &o3.set, NULL, NULL, NULL)));
        check("C08", "conflict reports its witness key",
              (witness == (uint64_t)k_foo));
    }

    // -- Union and intersection --------------------------------------------
    printf("\nUnion and intersection\n");

    fx_init(&c);
    check("C09", "union of CONFLICTING sets is undefined -> CONFLICT (formal)",
          (!d_option_set_union(&c.set, &o1.set, &o3.set,
                               NULL, NULL, NULL).is_ok));
    {
        struct d_option_result u =
            d_option_set_union(&c.set, &o1.set, &o3.set, NULL, NULL, NULL);

        check("C09b", "and that status is FORMAL, not mechanical",
              D_OPTION_STATUS_IS_FORMAL(u.payload.error) &&
              (u.payload.error == (int32_t)D_OPTION_STATUS_CONFLICT));
    }

    fx_init(&c);
    check("C10", "union of AGREEING sets is defined",
          d_option_set_union(&c.set, &o1.set, &o2.set,
                             NULL, NULL, NULL).is_ok);
    check("C10b", "and carries K(O1) u K(O2)",
          (d_option_set_size(&c.set) == 2u) &&
          d_option_set_contains(&c.set, k_foo) &&
          d_option_set_contains(&c.set, k_bar));

    //   intersection is IDENTITY-filtered, key intersection is not: they must
    // disagree on a conflicting key, or one of them is wrong.
    fx_init(&c);
    (void)d_option_set_intersection(&c.set, &o1.set, &o3.set,
                                    NULL, NULL, NULL);
    {
        struct d_option_result ki =
            d_option_set_key_intersection(&o1.set, &o3.set, NULL, 0u);

        check("C11", "intersection is identity-filtered: a shared key with "
                     "different values is in NEITHER",
              (d_option_set_size(&c.set) == 0u));
        check("C11b", "while the KEY intersection does contain it "
                      "(the two are different operations)",
              ki.is_ok && (ki.payload.value == 1u));
    }

    // -- Precedence --------------------------------------------------------
    printf("\nPrecedence\n");

    {
        struct d_option_policy replace = d_option_policy_replace();
        int32_t                v       = 0;

        fx_init(&c);
        (void)d_option_set_override(&c.set, &o1.set, &o3.set, &replace);
        (void)d_option_set_get(&c.set, k_foo, &v, sizeof(v));

        check("C12", "(+) case split: the DELTA wins on a shared key",
              (v == 99));

        //   THE SECOND STATED NON-PROPERTY.
        fx_init(&sc);
        (void)d_option_set_override(&sc.set, &o3.set, &o1.set, &replace);
        (void)d_option_set_get(&sc.set, k_foo, &v, sizeof(v));

        check("C13", "*** (+) is NOT commutative in general ***",
              (v == 1) && (!same(&c.set, &sc.set)));

        check("C14", "commutes(A,B) <=> A ~ B  [conflicting pair]",
              d_option_set_commutes(&o1.set, &o3.set, NULL, NULL, NULL) ==
              d_option_set_agrees(&o1.set, &o3.set, NULL, NULL, NULL));
        check("C14b", "commutes(A,B) <=> A ~ B  [agreeing pair]",
              d_option_set_commutes(&o1.set, &o2.set, NULL, NULL, NULL) &&
              d_option_set_agrees(&o1.set, &o2.set, NULL, NULL, NULL));

        //   the monoid: identity, then associativity.
        fx_init(&a);
        check("C15", "the empty set is the monoid identity element",
              d_option_set_is_identity(&a.set));

        fx_init(&c);
        (void)d_option_set_override(&c.set, &a.set, &o1.set, &replace);
        check("C16", "{} (+) A == A",  same(&c.set, &o1.set));

        fx_init(&c);
        (void)d_option_set_override(&c.set, &o1.set, &a.set, &replace);
        check("C17", "A (+) {} == A",  same(&c.set, &o1.set));

        {
            struct fixture ab;
            struct fixture ab_c;
            struct fixture bc;
            struct fixture a_bc;

            fx_init(&ab);   (void)d_option_set_override(&ab.set, &o1.set,
                                                        &o2.set, &replace);
            fx_init(&ab_c); (void)d_option_set_override(&ab_c.set, &ab.set,
                                                        &o3.set, &replace);
            fx_init(&bc);   (void)d_option_set_override(&bc.set, &o2.set,
                                                        &o3.set, &replace);
            fx_init(&a_bc); (void)d_option_set_override(&a_bc.set, &o1.set,
                                                        &bc.set, &replace);

            (void)d_option_set_canon(&ab_c.set);
            (void)d_option_set_canon(&a_bc.set);

            check("C18", "(A (+) B) (+) C == A (+) (B (+) C)  -- associative",
                  same(&ab_c.set, &a_bc.set));
        }

        //   u is the agreement-restricted total of (+): on agreeing sets the
        // two coincide, in either order.
        {
            struct fixture un;
            struct fixture ov;

            fx_init(&un); (void)d_option_set_union(&un.set, &o1.set, &o2.set,
                                                   NULL, NULL, NULL);
            fx_init(&ov); (void)d_option_set_override(&ov.set, &o1.set,
                                                      &o2.set, &replace);
            (void)d_option_set_canon(&un.set);
            (void)d_option_set_canon(&ov.set);

            check("C19", "on agreeing sets, u and (+) coincide",
                  same(&un.set, &ov.set));
        }
    }

    // -- Difference: carriers ----------------------------------------------
    printf("\nCarriers\n");

    {
        struct d_option_carrier absent = D_OPTION_CARRIER_ABSENT;
        int32_t                 datum  = 5;
        struct d_option_carrier present;

        present.data      = &datum;
        present.size      = (uint32_t)sizeof(datum);
        present.has_value = 1u;

        check("C20", "_|_ = _|_",
              d_option_carrier_eq(&absent, &absent, NULL, NULL));
        check("C21", "_|_ != v  for any value v",
              (!d_option_carrier_eq(&absent, &present, NULL, NULL)) &&
              (!d_option_carrier_eq(&present, &absent, NULL, NULL)));
        check("C22", "v = v structurally",
              d_option_carrier_eq(&present, &present, NULL, NULL));
    }

    // -- Order is not semantic ---------------------------------------------
    printf("\nRepresentation vs object\n");

    {
        struct fixture fwd;
        struct fixture rev;

        fx_init(&fwd); fx_put(&fwd, k_foo, 1); fx_put(&fwd, k_bar, 2);
        fx_init(&rev); fx_put(&rev, k_bar, 2); fx_put(&rev, k_foo, 1);

        check("C23", "a set is a SET: insertion order does not affect equality",
              same(&fwd.set, &rev.set));

        (void)d_option_set_canon(&rev.set);
        check("C24", "canon puts cells in ascending key order",
              d_option_set_is_canon(&rev.set));

        (void)d_option_set_canon(&rev.set);
        check("C25", "canon is idempotent",
              d_option_set_is_canon(&rev.set) && same(&fwd.set, &rev.set));
    }

    printf("\n----------------------------------------\n");
    printf("  %d passed, %d failed\n", g_pass, g_fail);

    return ((g_fail == 0) ? 0 : 1);
}
