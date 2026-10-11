/* cpp98.cpp: the C++ floor -- nothing newer than C++98 unless guarded */
#include <array>
#include <string_view>
#include <span>
#include <vector>

#define D_LOG(...) d_log(__VA_ARGS__)

long long d_wide;
enum class d_colour { red };
struct d_plain
{
    d_plain() = default;
    virtual void run() override;
};
template<typename... Types> struct d_pack;
using d_alias = int;
auto d_deduced = 1;
constexpr int d_constant = 2;

void d_loop(std::vector<int>& _values)
{
    for (int value : _values)
    {
        (void)value;
    }

    d_callback([](int _a) { return _a; });
    std::move(_values);
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

D_CONSTEXPR_CPP14 int d_sum_from_14(int _count)
{
    int total = 0;

    for (int at = 0; at < _count; ++at)
    {
        total += at;
    }

    return total;
}

D_CONSTEXPR int d_twice(int _value)
{
    return _value * 2;
}
