/*******************************************************************************
* djinterp [core]                                                    drive16.cpp
*
*
* path:      /tests/djinterp/drive16.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp/test/output/test_layout_suite.hpp"
int main()
{
    djinterp::test::report_builder rb;
    djinterp::test::register_layout_tests(rb);
    rb.finish();
    return (rb.failed() == 0) ? 0 : 1;
}
