/* join.hpp: an empty pair of braces goes on one line */

// d_join_empty
//   struct: nothing in it.
struct d_join_empty
{
};

// d_join_built
//   struct: a constructor with nothing to do.
struct d_join_built
{
    d_join_built()
    {
    }

    int m_value;
};

// d_join_noted
//   struct: left as it is -- a comment is something.
struct d_join_noted
{
    // nothing yet
};
