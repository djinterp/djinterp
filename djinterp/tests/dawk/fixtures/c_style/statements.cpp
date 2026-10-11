/* statements.cpp: the statement rules, read as C++ */

namespace d_fixture
{

struct d_counter
{
    d_counter()
    {}

    void bump(int amount)
    {
        if (amount > 0) m_count += amount;
        m_count += 1;
    }

    int value() const
    {
        int result = m_count;
        return result;
    }

    void reset()
    {
        m_count = 0;

        return;
    }

    int m_count;
};

}
