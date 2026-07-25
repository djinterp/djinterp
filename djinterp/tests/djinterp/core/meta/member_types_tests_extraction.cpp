#include "member_types_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_extraction_present_yields_extracted
  pick_member_type<true, Extracted, Fallback> yields Extracted.
  Tests the following:
  - with the present flag, ::type is the extracted type
  - it is not the fallback type
  - the same holds for a fundamental extracted type
  - the same holds when extracted and fallback are both class types
*/
bool
tests_extraction_present_yields_extracted()
{
    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<true, probe_extracted, probe_fallback>::type,
                    probe_extracted>::value));

    D_MT_CHECK(!(std::is_same<
                     internal::pick_member_type<true, probe_extracted, probe_fallback>::type,
                     probe_fallback>::value));

    // fundamental extracted type
    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<true, int, char>::type,
                    int>::value));

    // both class types
    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<true, has_all, has_none>::type,
                    has_all>::value));

    return true;
}

/*
tests_extraction_absent_yields_fallback
  pick_member_type<false, Extracted, Fallback> yields Fallback.
  Tests the following:
  - with the absent flag, ::type is the fallback type
  - it is not the extracted type
  - the fallback may be void (the historical "produce void on absence" shape)
  - the fallback may be a class type
*/
bool
tests_extraction_absent_yields_fallback()
{
    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<false, probe_extracted, probe_fallback>::type,
                    probe_fallback>::value));

    D_MT_CHECK(!(std::is_same<
                     internal::pick_member_type<false, probe_extracted, probe_fallback>::type,
                     probe_extracted>::value));

    // void fallback
    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<false, int, void>::type,
                    void>::value));

    // class fallback
    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<false, has_all, has_none>::type,
                    has_none>::value));

    return true;
}

/*
tests_extraction_fallback_ignores_extracted
  The false branch ignores the extracted type entirely.
  Tests the following:
  - the fallback is chosen regardless of what the extracted type is
  - swapping the extracted type does not change the false-branch result
  - even an extracted tag the branch never names is ignored
  - the true branch, by contrast, does depend on the extracted type
*/
bool
tests_extraction_fallback_ignores_extracted()
{
    // two different extracted types, same fallback -> same result
    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<false, probe_extracted, probe_fallback>::type,
                    internal::pick_member_type<false, int, probe_fallback>::type>::value));

    D_MT_CHECK((std::is_same<
                    internal::pick_member_type<false, has_all, probe_fallback>::type,
                    probe_fallback>::value));

    // contrast: the true branch DOES depend on the extracted type
    D_MT_CHECK(!(std::is_same<
                     internal::pick_member_type<true, probe_extracted, probe_fallback>::type,
                     internal::pick_member_type<true, int, probe_fallback>::type>::value));

    return true;
}

/*
tests_extraction_distinct_types_are_distinguished
  With three distinct tags the chosen branch is provable by identity.
  Tests the following:
  - present picks the extracted tag, not the fallback tag
  - absent picks the fallback tag, not the extracted tag
  - the two branch results are different types from one another
  - the present result equals the extracted tag and the absent result equals
    the fallback tag
*/
bool
tests_extraction_distinct_types_are_distinguished()
{
    using present = internal::pick_member_type<true,  probe_extracted, probe_fallback>::type;
    using absent  = internal::pick_member_type<false, probe_extracted, probe_fallback>::type;

    D_MT_CHECK((std::is_same<present, probe_extracted>::value));
    D_MT_CHECK((std::is_same<absent, probe_fallback>::value));

    // the two branches disagree
    D_MT_CHECK(!(std::is_same<present, absent>::value));

    // and neither is the unrelated third tag
    D_MT_CHECK(!(std::is_same<present, probe_present>::value));
    D_MT_CHECK(!(std::is_same<absent, probe_present>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
