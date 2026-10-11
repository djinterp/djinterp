/* templates.hpp: members of a class template are functions like any other */
#ifndef D_STD_TEMPLATES_FIXTURE
#define D_STD_TEMPLATES_FIXTURE 1

template<typename Type>
struct d_plain
{
    D_CONSTEXPR Type sum(int _count) const
    {
        Type total = Type();

        for (int at = 0; at < _count; ++at)
        {
            total += Type(at);
        }

        return total;
    }
};

template<typename Type = double>
struct d_defaulted
{
    D_CONSTEXPR Type sum(int _count) const
    {
        Type total = Type();

        for (int at = 0; at < _count; ++at)
        {
            total += Type(at);
        }

        return total;
    }
};

template<typename First, typename Second = d_plain<d_plain<int> > >
struct d_nested
{
    D_CONSTEXPR int count(int _limit) const
    {
        int found = 0;

        while (found < _limit)
        {
            ++found;
        }

        return found;
    }
};

template<>
struct d_plain<bool>
{
    D_CONSTEXPR int count(int _limit) const
    {
        int found = 0;

        while (found < _limit)
        {
            ++found;
        }

        return found;
    }
};

template<typename Type = int>
D_CONSTEXPR Type d_free_sum(int _count)
{
    Type total = Type();

    for (int at = 0; at < _count; ++at)
    {
        total += Type(at);
    }

    return total;
}

template<typename Type = int>
D_CONSTEXPR Type d_free_twice(Type _value)
{
    return _value + _value;
}

#endif  // D_STD_TEMPLATES_FIXTURE
