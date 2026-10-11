/* params.hpp: parameter layout, read as C++ */

struct d_layout
{
    d_layout(int _a, int _b)
    {}

    int sum(int _a,
            int _b);
    int detached(
        int _a,
        int _b);
    int crooked(int _a,
                  int _b);
};
