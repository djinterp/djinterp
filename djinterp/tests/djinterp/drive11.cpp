/*******************************************************************************
* djinterp [core]                                                    drive11.cpp
*
*
* path:      /tests/djinterp/drive11.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/core/functional/maybe.hpp"
#include "djinterp/core/functional/cofree.hpp"

using namespace djinterp;

struct head_of { int operator()(int s) const { return s; } };
struct next_of { maybe<int> operator()(int s) const {
    return s > 1 ? just(s - 1) : nothing<int>(); } };

int main()
{
    // the header's own usage example, run verbatim
    cofree<maybe, int> chain = unfold_cofree<maybe>(3, head_of{}, next_of{});

    std::cout << "extract(chain)      = " << extract(chain) << "   (expect 3)\n";

    auto doubled = functor_map(chain, [](int x){ return x * 2; });
    std::cout << "extract(doubled)    = " << extract(doubled) << "   (expect 6)\n";

    int total = annotated_cata<int>(
        [](int _head, const maybe<int>& _folded) -> int
        { return _head + (_folded.has_value() ? _folded.value() : 0); },
        chain);
    std::cout << "annotated_cata sum  = " << total << "   (expect 6)\n";

    int depth = annotated_cata<int>(
        [](int, const maybe<int>& _folded) -> int
        { return 1 + (_folded.has_value() ? _folded.value() : 0); },
        chain);
    std::cout << "annotated_cata depth= " << depth << "   (expect 3)\n";
    return 0;
}
