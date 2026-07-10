// djinterp [test] : adapter_tests_view.cpp
//   The non-owning view adapters (section IX): adapted_ref (mutable reference
// view), adapted_const_ref (read-only view), and adapted_view (a projecting
// view whose iterator applies a projection on dereference) — including the
// full iterator-operator surface (*, pre/post ++, ==, !=).

// std
#include <type_traits>
#include <vector>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // times_ten : the projection used by the adapted_view tests.
    struct times_ten
    {
        int
        operator()(
            int& _x
        ) const
        {
            return _x * 10;
        }
    };
}


/*
tests_adapted_ref_basic
  Verifies adapted_ref delegation and its type aliases.
  Tests the following:
  - size()/get() delegate through the policy
  - adaptee() aliases the referenced object
  - adaptee_type / adaptation_policy aliases resolve
*/
bool
tests_adapted_ref_basic()
{
    legacy_seq s;
    s.data = { 1, 2, 3 };

    adapted_ref<legacy_seq, legacy_policy> r(s);

    static_assert(std::is_same<decltype(r)::adaptee_type, legacy_seq>::value,
                  "adaptee_type");
    static_assert(std::is_same<decltype(r)::adaptation_policy,
                               legacy_policy>::value,
                  "adaptation_policy");

    bool ok = true;

    ok = ok && (r.size() == 3);
    ok = ok && (r.get(0) == 1);
    ok = ok && (r.get(2) == 3);
    ok = ok && (&r.adaptee() == &s);

    return ok;
}

/*
tests_adapted_ref_mutation
  Verifies write-through semantics of the mutable adapted_ref.
  Tests the following:
  - non-const get() writes reach the referenced object
  - mutation through adaptee() is visible via size()
*/
bool
tests_adapted_ref_mutation()
{
    legacy_seq s;
    s.data = { 1, 2, 3 };

    adapted_ref<legacy_seq, legacy_policy> r(s);

    bool ok = true;

    r.get(1) = 99;
    ok = ok && (s.data[1] == 99);

    r.adaptee().append(4);
    ok = ok && (r.size() == 4);

    return ok;
}

/*
tests_adapted_ref_const
  Verifies the const overloads of adapted_ref.
  Tests the following:
  - const size() delegates through the policy
  - const get() returns a const reference
  - const adaptee() returns a const reference
*/
bool
tests_adapted_ref_const()
{
    legacy_seq s;
    s.data = { 5, 6, 7 };

    const adapted_ref<legacy_seq, legacy_policy> r(s);

    bool ok = true;

    ok = ok && (r.size() == 3);

    const int& e = r.get(0);
    ok = ok && (e == 5);

    ok = ok && (r.adaptee().num_elements() == 3);

    return ok;
}

/*
tests_adapted_const_ref
  Verifies adapted_const_ref (read-only projection of an adaptee).
  Tests the following:
  - size()/get() delegate through the policy on a const view
  - adaptee() aliases the referenced object
  - adaptee_type alias resolves
*/
bool
tests_adapted_const_ref()
{
    legacy_seq s;
    s.data = { 10, 20 };

    adapted_const_ref<legacy_seq, legacy_policy> r(s);

    static_assert(std::is_same<decltype(r)::adaptee_type, legacy_seq>::value,
                  "adaptee_type");

    bool ok = true;

    ok = ok && (r.size() == 2);
    ok = ok && (r.get(0) == 10);
    ok = ok && (r.get(1) == 20);
    ok = ok && (&r.adaptee() == &s);

    return ok;
}

/*
tests_adapted_view_iteration
  Verifies adapted_view traversal with a projection on dereference.
  Tests the following:
  - each element is transformed by the projection when iterated
  - adaptee_type / projection_type aliases resolve
*/
bool
tests_adapted_view_iteration()
{
    std::vector<int> v = { 1, 2, 3 };

    adapted_view<std::vector<int>, times_ten> view(v, times_ten{});

    static_assert(std::is_same<decltype(view)::adaptee_type,
                               std::vector<int>>::value,
                  "adaptee_type");
    static_assert(std::is_same<decltype(view)::projection_type,
                               times_ten>::value,
                  "projection_type");

    bool ok = true;

    std::vector<int> got;
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        got.push_back(*it);
    }

    ok = ok && (got.size() == 3);
    ok = ok && (got[0] == 10);
    ok = ok && (got[1] == 20);
    ok = ok && (got[2] == 30);

    return ok;
}

/*
tests_adapted_view_iterator_ops
  Verifies the adapted_view::iterator operator surface.
  Tests the following:
  - operator* applies the projection
  - pre-increment advances and returns *this
  - post-increment returns the pre-advance position, then advances
  - operator== / operator!= compare underlying positions
  - advancing to end() compares equal to end()
*/
bool
tests_adapted_view_iterator_ops()
{
    std::vector<int> v = { 1, 2, 3 };

    adapted_view<std::vector<int>, times_ten> view(v, times_ten{});

    bool ok = true;

    auto it = view.begin();

    // operator*
    ok = ok && (*it == 10);

    // pre-increment returns *this
    auto& ref = ++it;
    ok = ok && (*it == 20);
    ok = ok && (&ref == &it);

    // post-increment returns the pre-advance copy, then advances
    auto old = it++;
    ok = ok && (*old == 20);
    ok = ok && (*it == 30);

    // equality / inequality
    ok = ok && (view.begin() == view.begin());
    ok = ok && (view.begin() != view.end());

    // advancing off the last element reaches end()
    ++it;
    ok = ok && (it == view.end());

    return ok;
}

/*
tests_adapted_view_size
  Verifies adapted_view::size() passthrough to the underlying adaptee.
  Tests the following:
  - size() reports the adaptee's element count
  - it reflects changes to the referenced adaptee
*/
bool
tests_adapted_view_size()
{
    std::vector<int> v = { 1, 2, 3, 4 };

    adapted_view<std::vector<int>, times_ten> view(v, times_ten{});

    bool ok = true;

    ok = ok && (view.size() == 4);

    v.push_back(5);
    ok = ok && (view.size() == 5);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
