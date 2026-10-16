/*******************************************************************************
* djinterp [core]                                djinterp_tests_resolve_self.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_resolve_self.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_resolve_self_passthrough
  Tests the following:
  - a type containing no self marker is returned unchanged (primary template)
  - scalars, a user type, and pointers/references to NON-self types pass
    through untouched
*/
bool
tests_resolve_self_passthrough()
{
    bool _ok = true;
    using T = probe_target;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<int, T>, int >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<double, T>, double >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<probe_target, int>, probe_target >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<int*, T>, int* >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<int&, T>, int& >::value));

    return _ok;
}

/*
tests_resolve_self_marker
  Tests the following:
  - a bare self marker resolves to the target type
  - the target may be any type, including a templated one
*/
bool
tests_resolve_self_marker()
{
    bool _ok = true;
    using ::djinterp::self;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<self, probe_target>, probe_target >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<self, int>, int >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<self, std::tuple<int, char>>, std::tuple<int, char> >::value));

    static_assert(std::is_same< ::djinterp::resolve_self_t<self, probe_target>, probe_target >::value, "");
    return _ok;
}

/*
tests_resolve_self_smart_pointers
  Tests the following:
  - unique_ptr<self>, shared_ptr<self>, and weak_ptr<self> resolve to the
    smart pointer of the target type (dedicated specializations)
  - a smart pointer wrapping a NON-self type still resolves correctly through
    the variadic catch-all (unique_ptr<vector<self>> -> unique_ptr<vector<T>>)
*/
bool
tests_resolve_self_smart_pointers()
{
    bool _ok = true;
    using ::djinterp::self;
    using T = probe_target;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::unique_ptr<self>, T>, std::unique_ptr<T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::shared_ptr<self>, T>, std::shared_ptr<T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::weak_ptr<self>, T>, std::weak_ptr<T> >::value));

    // nested: the dedicated unique_ptr<self> spec does not match; the variadic
    // catch-all recurses through unique_ptr and vector instead
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::unique_ptr<std::vector<self>>, T>, std::unique_ptr<std::vector<T>> >::value));

    return _ok;
}

/*
tests_resolve_self_raw_pointer
  Tests the following:
  - a raw pointer to self (self*) resolves to a pointer to the target
  - only self* is rewritten; a pointer to a non-self type passes through
*/
bool
tests_resolve_self_raw_pointer()
{
    bool _ok = true;
    using ::djinterp::self;
    using T = probe_target;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<self*, T>, T* >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<self*, int>, int* >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<int*, T>, int* >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<probe_target*, int>, probe_target* >::value));

    static_assert(std::is_same< ::djinterp::resolve_self_t<self*, probe_target>, probe_target* >::value, "");
    return _ok;
}

/*
tests_resolve_self_variadic_templates
  Tests the following:
  - the typename-only catch-all rewrites self inside arbitrary class templates:
    std::vector, std::optional, std::pair, std::tuple, and a user template
  - self is rewritten in any argument position (pair<int,self>, pair<self,int>,
    tuple mixes)
  - nesting is handled recursively
  - templates that contain no self pass through unchanged
*/
bool
tests_resolve_self_variadic_templates()
{
    bool _ok = true;
    using ::djinterp::self;
    using T = probe_target;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::vector<self>, T>, std::vector<T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::optional<self>, T>, std::optional<T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::pair<int, self>, T>, std::pair<int, T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::pair<self, int>, T>, std::pair<T, int> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::tuple<self, int, self>, T>, std::tuple<T, int, T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<user_template<self>, T>, user_template<T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<user_template<int, self, char>, T>, user_template<int, T, char> >::value));

    // nesting
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::vector<std::pair<int, self>>, T>, std::vector<std::pair<int, T>> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::optional<std::vector<self>>, T>, std::optional<std::vector<T>> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::tuple<std::vector<self>, std::pair<self, self>>, T>, std::tuple<std::vector<T>, std::pair<T, T>> >::value));

    // no self -> unchanged
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::vector<int>, T>, std::vector<int> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::pair<int, char>, T>, std::pair<int, char> >::value));

    return _ok;
}

/*
tests_resolve_self_repeat
  Tests the following:
  - resolve_self over repeat<self,N> yields a tuple of N targets
  - the inner type is resolved BEFORE repetition
    (repeat<vector<self>,2> -> tuple<vector<T>, vector<T>>)
  - repeat with no self repeats unchanged
  - the N=0 repeat resolves to an empty tuple
*/
bool
tests_resolve_self_repeat()
{
    bool _ok = true;
    using ::djinterp::self;
    using ::djinterp::repeat;
    using T = probe_target;

    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<repeat<self, 3>, T>, std::tuple<T, T, T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<repeat<self, 1>, T>, std::tuple<T> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<repeat<self, 0>, T>, std::tuple<> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<repeat<std::vector<self>, 2>, T>, std::tuple<std::vector<T>, std::vector<T>> >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<repeat<int, 3>, T>, std::tuple<int, int, int> >::value));

    return _ok;
}

/*
tests_resolve_self_limitations
  Tests the following:
  - documented limitation: templates with NON-TYPE parameters (std::array) are
    NOT matched by the typename-only catch-all, so self is left UNRESOLVED
  - cv-qualified self and reference-to-self are not the exact self spec and so
    pass through unchanged (exact-match semantics)
  - resolve_self_t is exactly resolve_self<...>::type
*/
bool
tests_resolve_self_limitations()
{
    bool _ok = true;
    using ::djinterp::self;
    using T = probe_target;

    // std::array<T,N> has a non-type parameter -> catch-all does not apply
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::array<self, 3>, T>, std::array<self, 3> >::value));

    // cv / reference qualified self is not the exact self specialization
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<const self, T>, const self >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<self&, T>, self& >::value));

    // alias parity with the underlying trait
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<self, T>, typename ::djinterp::resolve_self<self, T>::type >::value));
    D_DT_CHECK(_ok, (std::is_same< ::djinterp::resolve_self_t<std::vector<self>, T>, typename ::djinterp::resolve_self<std::vector<self>, T>::type >::value));

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
