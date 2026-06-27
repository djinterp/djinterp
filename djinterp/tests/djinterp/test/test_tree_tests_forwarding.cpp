// djinterp
#include "test_tree_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tree_size_and_empty
  Verifies the forwarded capacity surface tracks the backing forest, counting
  the implied conjunctive root once it exists.
  Tests the following:
  - size() / empty() on a fresh tree (0 / true)
  - empty() becomes false and size() == roots + 1 after the first add_root
  - size() grows by one per additional root
*/
bool
tests_tree_size_and_empty()
{
    test_tree<basic_test> t;

    const bool fresh_ok =
        ( (t.size()  == 0) &&
          (t.empty() == true) );

    // first add_root materialises the conjunctive root -> size 2
    t.add_root(make_test(1, true));

    const bool one_ok =
        ( (t.size()  == 2) &&
          (t.empty() == false) );

    t.add_root(make_test(2, true));

    const bool two_ok = (t.size() == 3);

    return ( fresh_ok &&
             one_ok   &&
             two_ok );
}

/*
tests_tree_root_accessor
  Verifies root() returns the implied conjunctive root once created, through
  both overloads.
  Tests the following:
  - root() (non-const) is nullptr before, non-null after add_root
  - the conjunctive root is the default element (type id 0, pending)
  - root() const observes the same node
*/
bool
tests_tree_root_accessor()
{
    test_tree<basic_test> t;

    const bool before = (t.root() == nullptr);

    t.add_root(make_test(5, true));

    test_tree<basic_test>::node_type* r = t.root();

    const bool after =
        ( (r != nullptr)            &&
          (r->data().type_id() == 0) &&
          status_is(r->data(), test_status::pending) );

    const test_tree<basic_test>& ct = t;

    const bool const_ok = (ct.root() != nullptr);

    return ( before   &&
             after    &&
             const_ok );
}

/*
tests_tree_begin_end_iteration
  Verifies the (non-const) sequential walk visits exactly the backing nodes -
  the conjunctive root and every root beneath it.
  Tests the following:
  - begin() / end() (non-const)
  - the walk length equals size()
  - element status is reachable through the iterator (one passed root seen)
*/
bool
tests_tree_begin_end_iteration()
{
    test_tree<basic_test> t;
    t.add_root(make_status_test(1, test_status::passed));
    t.add_root(make_status_test(2, test_status::failed));

    std::size_t walked = 0;
    std::size_t passed = 0;

    for (auto it = t.begin(); it != t.end(); ++it)
    {
        ++walked;

        if (status_is(*it, test_status::passed))
        {
            ++passed;
        }
    }

    return ( (walked == t.size()) &&
             (passed == 1) );
}

/*
tests_tree_const_begin_end_iteration
  Verifies the const sequential walk visits the same nodes via the const
  overloads.
  Tests the following:
  - begin() const / end() const
  - the const walk length equals size()
*/
bool
tests_tree_const_begin_end_iteration()
{
    test_tree<basic_test> t;
    t.add_root(make_status_test(1, test_status::passed));

    const test_tree<basic_test>& ct = t;

    std::size_t walked = 0;

    for (auto it = ct.begin(); it != ct.end(); ++it)
    {
        ++walked;
    }

    return (walked == ct.size());
}

/*
tests_tree_clear
  Verifies clear() empties the forest (including the conjunctive root) and
  that the tree is reusable afterwards.
  Tests the following:
  - clear() drops every node; size() == 0, empty() true, root() nullptr
  - a subsequent add_root re-creates the conjunctive root cleanly
*/
bool
tests_tree_clear()
{
    test_tree<basic_test> t;
    t.add_root(make_test(1, true));
    t.add_root(make_test(2, true));

    const bool grown = (t.size() >= 3);

    t.clear();

    const bool cleared =
        ( (t.size()  == 0)       &&
          (t.empty() == true)    &&
          (t.root()  == nullptr) );

    // reusable: the conjunctive root is created afresh on the next add_root
    t.add_root(make_test(3, true));

    const bool reusable =
        ( (t.size() == 2) &&
          (t.root() != nullptr) );

    return ( grown    &&
             cleared  &&
             reusable );
}


NS_END  // testing
NS_END  // djinterp
