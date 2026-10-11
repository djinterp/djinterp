/* cpp98_guarded.cpp: the same constructs under a conditional, all negative */
#include <vector>

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include <array>

#define D_LOG(...) d_log(__VA_ARGS__)

long long d_wide;
enum class d_colour { red };
template<typename... Types> struct d_pack;
using d_alias = int;
auto d_deduced = 1;
constexpr int d_constant = 2;

void d_loop(void)
{
    d_callback([](int _a) { return _a; });
}

D_CONSTEXPR int d_sum(int _count)
{
    int total = 0;

    for (int at = 0; at < _count; ++at)
    {
        total += at;
    }

    return total;
}
#endif
