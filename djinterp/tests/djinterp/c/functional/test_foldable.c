/*******************************************************************************
* djinterp [functional]                                          test_foldable.c
*
*
* path:      /tests/djinterp/c/functional/test_foldable.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdint.h>
#include <stdio.h>
// djinterp
#include "../../../../inc/djinterp/c/functional/foldable.h"

D_MAYBE_DECLARE(tf_maybe, intmax_t);
D_RESULT_DECLARE(tf_result, intmax_t, intmax_t);

static size_t g_ck=0,g_ok=0;
static void ck(bool c,const char*l){g_ck++;if(c)g_ok++;else printf("    FAIL  %s\n",l);}
static bool even(const void*e,void*x){(void)x;return (*(const intmax_t*)e%2)==0;}
static bool pos(const void*e,void*x){(void)x;return *(const intmax_t*)e>0;}

int main(void)
{
    struct tf_maybe               mc;
    struct tf_result              rc;
    struct d_maybe                m, m2, mo;
    struct tf_maybe               mc2, mco;
    struct d_result               r;
    struct d_array_carrier        ac;
    struct d_producer_carrier     pc;
    struct d_producer_array_state ps;
    struct d_producer             prod;
    struct d_foldable             fa, fm, fr, fp;
    struct d_alternative          alt;
    struct d_monoid               sum;
    intmax_t arr[4] = {1,2,3,4};
    intmax_t v, out, scratch;

    printf("djinterp [functional] foldable/alternative conformance (tier 3)\n");

    fa = d_foldable_array(sizeof(intmax_t));
    fm = d_foldable_maybe(sizeof(intmax_t));
    fr = d_foldable_result(sizeof(intmax_t));
    fp = d_foldable_producer(sizeof(intmax_t));
    sum = d_monoid_sum_intmax();

    ac.elements = arr; ac.count = 4; ac.element_size = sizeof(intmax_t);
    m = D_MAYBE_VIEW(mc); r = D_RESULT_VIEW(rc);
    v = 10; d_maybe_just(&m, &v); d_result_ok(&r, &v);
    prod = d_producer_from_array(&ps, arr, 4, sizeof(intmax_t));
    pc.producer = &prod; pc.scratch = &scratch;

    /* length, through four different carriers, one implementation */
    ck(d_foldable_length(&fa,&ac)==4,"length over an array");
    ck(d_foldable_length(&fm,&m)==1,"length over a just");
    ck(d_foldable_length(&fr,&r)==1,"length over an ok");
    ck(d_foldable_length(&fp,&pc)==4,"length over a producer");

    d_maybe_nothing(&m);
    ck(d_foldable_length(&fm,&m)==0,"length over a nothing");
    ck(d_foldable_is_empty(&fm,&m),"is_empty over a nothing");
    ck(!d_foldable_is_empty(&fa,&ac),"is_empty over a non-empty array");
    /* the latch is the point: is_empty must consume ONE element, not all of
       them, or an unbounded carrier could never be asked */
    { intmax_t rest[8]; size_t left;
      prod = d_producer_from_array(&ps, arr, 4, sizeof(intmax_t));
      d_foldable_is_empty(&fp,&pc);
      left = d_producer_collect(&prod, rest, 8);
      ck(left==3,"is_empty latches after one element"); }

    /* THE PAYOFF: one monoid folds every carrier, neither knowing the other */
    ck(d_foldable_mconcat(&fa,&ac,&sum,&out)&&out==10,"mconcat over an array");
    v = 10; d_maybe_just(&m,&v);
    ck(d_foldable_mconcat(&fm,&m,&sum,&out)&&out==10,"mconcat over a just");
    d_maybe_nothing(&m);
    ck(d_foldable_mconcat(&fm,&m,&sum,&out)&&out==0,"mconcat over a nothing is mempty");
    ck(d_foldable_mconcat(&fr,&r,&sum,&out)&&out==10,"mconcat over an ok");
    v = -1; d_result_err(&r,&v);
    ck(d_foldable_mconcat(&fr,&r,&sum,&out)&&out==0,"mconcat over an err is mempty");
    prod = d_producer_from_array(&ps, arr, 4, sizeof(intmax_t));
    ck(d_foldable_mconcat(&fp,&pc,&sum,&out)&&out==10,"mconcat over a producer");

    /* a width mismatch between carrier and monoid must be refused */
    { struct d_foldable narrow = d_foldable_array(1);
      ck(!d_foldable_mconcat(&narrow,&ac,&sum,&out),
         "mconcat refuses a width mismatch"); }

    /* quantifiers, short-circuit intact, over any carrier */
    ck(d_foldable_any(&fa,&ac,even,NULL),"any over an array");
    ck(!d_foldable_all(&fa,&ac,even,NULL),"all over an array");
    ck(d_foldable_all(&fa,&ac,pos,NULL),"all holds over an array");
    d_maybe_nothing(&m);
    ck(d_foldable_all(&fm,&m,even,NULL),"all over an empty carrier is vacuous");
    ck(!d_foldable_any(&fm,&m,even,NULL),"any over an empty carrier is false");

    /* Alternative through a dictionary */
    alt = d_alternative_maybe();
    m2 = D_MAYBE_VIEW(mc2); mo = D_MAYBE_VIEW(mco);
    v = 1; d_maybe_just(&m,&v);
    v = 2; d_maybe_just(&m2,&v);
    ck(d_alternative_alt(&alt,&m,&m2,&mo)&&mco.value==1,"alt is left-biased");
    d_maybe_nothing(&m);
    ck(d_alternative_alt(&alt,&m,&m2,&mo)&&mco.value==2,"alt falls through");
    ck(d_alternative_aempty(&alt,&mo)&&d_maybe_is_nothing(&mo),"aempty is empty");
    v = 5; d_maybe_just(&m2,&v);
    d_alternative_aempty(&alt,&m);
    ck(d_alternative_alt(&alt,&m,&m2,&mo)&&mco.value==5,"aempty is a left identity");
    ck(d_alternative_alt(&alt,&m2,&m,&mo)&&mco.value==5,"aempty is a right identity");

    printf("\n  %lu / %lu assertions passed\n",(unsigned long)g_ok,(unsigned long)g_ck);
    return (g_ok==g_ck)?0:1;
}
