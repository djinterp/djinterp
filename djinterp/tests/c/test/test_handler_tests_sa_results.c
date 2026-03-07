/******************************************************************************
* djinterp [test]                               test_handler_tests_sa_results.c
*
*   Unit tests for test_handler result queries:
*     d_test_handler_get_results, reset_results, print_results,
*     get_pass_rate, get_assertion_rate
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"

bool d_tests_sa_handler_get_results(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed; bool all_ok;
    printf("  --- Testing d_test_handler_get_results ---\n");
    initial_tests_passed = _test_info->tests_passed; all_ok = true;

    // NULL handler returns zeroed struct
    { struct d_test_result r = d_test_handler_get_results(NULL);
      if (!d_assert_standalone((r.tests_run==0&&r.assertions_run==0&&r.blocks_run==0&&r.max_depth==0),
          "get_results NULL", "NULL handler returns zeroed results", _test_info)) all_ok=false; }

    // Fresh handler returns zeros
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) { struct d_test_result r = d_test_handler_get_results(h);
        if (!d_assert_standalone((r.tests_run==0&&r.tests_passed==0&&r.tests_failed==0),
            "fresh handler zeros", "Fresh handler has zero results", _test_info)) all_ok=false;
        d_test_handler_free(h); } }

    // After execution returns correct values
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) { int i;
        for(i=0;i<3;i++){struct d_test*t=helper_make_passing_test();if(t){d_test_handler_run_test(h,t,NULL);d_test_free(t);}}
        for(i=0;i<2;i++){struct d_test*t=helper_make_failing_test();if(t){d_test_handler_run_test(h,t,NULL);d_test_free(t);}}
        struct d_test_result r = d_test_handler_get_results(h);
        if (!d_assert_standalone((r.tests_run==5&&r.tests_passed==3&&r.tests_failed==2),
            "results after execution", "Correct results after 5 tests", _test_info)) all_ok=false;
        d_test_handler_free(h); } }

    if (all_ok){_test_info->tests_passed++;printf("%s[PASS] get_results\n",D_INDENT);}
    else printf("%s[FAIL] get_results\n",D_INDENT);
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

bool d_tests_sa_handler_reset_results(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed; bool all_ok;
    printf("  --- Testing d_test_handler_reset_results ---\n");
    initial_tests_passed = _test_info->tests_passed; all_ok = true;

    // NULL handler does not crash
    d_test_handler_reset_results(NULL);
    if (!d_assert_standalone(true,"reset NULL","Reset NULL does not crash",_test_info)) all_ok=false;

    // Reset clears all fields
    { struct d_test_handler*h=d_test_handler_new(NULL);
      if(h){ struct d_test*t=helper_make_passing_test();
        if(t){d_test_handler_run_test(h,t,NULL);d_test_free(t);}
        d_test_handler_reset_results(h);
        struct d_test_result r=d_test_handler_get_results(h);
        if(!d_assert_standalone((r.tests_run==0&&r.tests_passed==0&&r.assertions_run==0&&r.blocks_run==0&&r.max_depth==0&&h->current_depth==0),
            "reset clears all","All fields zero after reset",_test_info))all_ok=false;
        d_test_handler_free(h);} }

    // Usable after reset
    { struct d_test_handler*h=d_test_handler_new(NULL);
      if(h){ struct d_test*t1=helper_make_passing_test();
        if(t1){d_test_handler_run_test(h,t1,NULL);d_test_free(t1);}
        d_test_handler_reset_results(h);
        struct d_test*t2=helper_make_passing_test();
        if(t2){bool r=d_test_handler_run_test(h,t2,NULL);
          struct d_test_result res=d_test_handler_get_results(h);
          if(!d_assert_standalone((r==true&&res.tests_run==1),
              "usable after reset","Handler works after reset",_test_info))all_ok=false;
          d_test_free(t2);}
        d_test_handler_free(h);} }

    if(all_ok){_test_info->tests_passed++;printf("%s[PASS] reset_results\n",D_INDENT);}
    else printf("%s[FAIL] reset_results\n",D_INDENT);
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

bool d_tests_sa_handler_print_results(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed; bool all_ok;
    printf("  --- Testing d_test_handler_print_results ---\n");
    initial_tests_passed = _test_info->tests_passed; all_ok = true;

    d_test_handler_print_results(NULL,"NULL Handler");
    if(!d_assert_standalone(true,"print NULL","Print NULL handler safe",_test_info))all_ok=false;

    {struct d_test_handler*h=d_test_handler_new(NULL);
     if(h){d_test_handler_print_results(h,NULL);
       if(!d_assert_standalone(true,"print NULL label","Print NULL label safe",_test_info))all_ok=false;
       d_test_handler_print_results(h,"Test");
       if(!d_assert_standalone(true,"print valid","Print valid succeeds",_test_info))all_ok=false;
       d_test_handler_free(h);}}

    if(all_ok){_test_info->tests_passed++;printf("%s[PASS] print_results\n",D_INDENT);}
    else printf("%s[FAIL] print_results\n",D_INDENT);
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

bool d_tests_sa_handler_get_pass_rate(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed; bool all_ok;
    printf("  --- Testing d_test_handler_get_pass_rate ---\n");
    initial_tests_passed = _test_info->tests_passed; all_ok = true;

    if(!d_assert_standalone(d_test_handler_get_pass_rate(NULL)==0.0,"pass_rate NULL","NULL=0.0",_test_info))all_ok=false;

    {struct d_test_handler*h=d_test_handler_new(NULL);
     if(h){if(!d_assert_standalone(d_test_handler_get_pass_rate(h)==0.0,"pass_rate no tests","No tests=0.0",_test_info))all_ok=false;
       int i;for(i=0;i<5;i++){struct d_test*t=helper_make_passing_test();if(t){d_test_handler_run_test(h,t,NULL);d_test_free(t);}}
       if(!d_assert_standalone(d_test_handler_get_pass_rate(h)==100.0,"pass_rate 100%","All pass=100.0",_test_info))all_ok=false;
       d_test_handler_reset_results(h);
       for(i=0;i<3;i++){struct d_test*t=helper_make_passing_test();if(t){d_test_handler_run_test(h,t,NULL);d_test_free(t);}}
       for(i=0;i<3;i++){struct d_test*t=helper_make_failing_test();if(t){d_test_handler_run_test(h,t,NULL);d_test_free(t);}}
       if(!d_assert_standalone(d_test_handler_get_pass_rate(h)==50.0,"pass_rate 50%","50% pass",_test_info))all_ok=false;
       d_test_handler_free(h);}}

    if(all_ok){_test_info->tests_passed++;printf("%s[PASS] get_pass_rate\n",D_INDENT);}
    else printf("%s[FAIL] get_pass_rate\n",D_INDENT);
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

bool d_tests_sa_handler_get_assertion_rate(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed; bool all_ok;
    printf("  --- Testing d_test_handler_get_assertion_rate ---\n");
    initial_tests_passed = _test_info->tests_passed; all_ok = true;

    if(!d_assert_standalone(d_test_handler_get_assertion_rate(NULL)==0.0,"assert_rate NULL","NULL=0.0",_test_info))all_ok=false;

    {struct d_test_handler*h=d_test_handler_new(NULL);
     if(h){int i;
       for(i=0;i<10;i++){struct d_assert*a=d_assert_new(true,"P","F");d_test_handler_record_assertion(h,a);d_assert_free(a);}
       if(!d_assert_standalone(d_test_handler_get_assertion_rate(h)==100.0,"assert_rate 100%","All pass=100.0",_test_info))all_ok=false;
       d_test_handler_reset_results(h);
       for(i=0;i<6;i++){struct d_assert*a=d_assert_new(true,"P","F");d_test_handler_record_assertion(h,a);d_assert_free(a);}
       for(i=0;i<4;i++){struct d_assert*a=d_assert_new(false,"P","F");d_test_handler_record_assertion(h,a);d_assert_free(a);}
       double rate=d_test_handler_get_assertion_rate(h);
       if(!d_assert_standalone((rate>=59.9&&rate<=60.1),"assert_rate 60%","60% assertion rate",_test_info))all_ok=false;
       d_test_handler_free(h);}}

    if(all_ok){_test_info->tests_passed++;printf("%s[PASS] get_assertion_rate\n",D_INDENT);}
    else printf("%s[FAIL] get_assertion_rate\n",D_INDENT);
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}
