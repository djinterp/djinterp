/*******************************************************************************
* djinterp [test]                                    interval_protocol_tests.cpp
*
* Tests for container/traits/interval_protocol.hpp: is_interval_protocol
* holds for a type with the interval protocol's shape (static lower_bound
* and upper_bound, static bool is_left_open and is_right_open) and for
* nothing else, closed_bounds has the shape, and container_traits.hpp's
* bounds detection, which reads the protocol, accepts closed_bounds.
*   Compile-only: every check is a static assertion, so compiling this unit at
* a level runs the test there, and the ladder compiles it at every level.
* Below C++11 the header is empty and so is this test; the container_traits
* checks need C++17, that header's floor.
*
*
* path:      /tests/djinterp/cpp/container/traits/interval_protocol_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/container/traits/interval_protocol.hpp"
#include "../../../../../inc/djinterp/cpp/container/traits/container_traits.hpp"
// std
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <cstddef>  // std::size_t
#endif


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_DJINTERP
NS_INTERNAL

    // the probe types: the shape, and each way of missing it
    struct ip_shape
    {
        static const int  lower_bound   = 0;
        static const int  upper_bound   = 9;
        static const bool is_left_open  = false;
        static const bool is_right_open = true;
    };

    struct ip_no_upper
    {
        static const int  lower_bound   = 0;
        static const bool is_left_open  = false;
        static const bool is_right_open = false;
    };

    struct ip_member_bounds
    {
        int               lower_bound;
        int               upper_bound;
        static const bool is_left_open  = false;
        static const bool is_right_open = false;
    };

    struct ip_int_flags
    {
        static const int lower_bound   = 0;
        static const int upper_bound   = 9;
        static const int is_left_open  = 0;
        static const int is_right_open = 0;
    };

    struct ip_function_bounds
    {
        static int        lower_bound();
        static int        upper_bound();
        static const bool is_left_open  = false;
        static const bool is_right_open = false;
    };

NS_END  // internal
NS_END  // djinterp


// D_INTERNAL_IP_CHECK
//   macro (internal): TYPE has the protocol's shape exactly when EXPECTED.
#define D_INTERNAL_IP_CHECK(TYPE, EXPECTED)                                   \
    D_STATIC_ASSERT(                                                          \
        (djinterp::is_interval_protocol<TYPE>::value == (EXPECTED)),          \
        "is_interval_protocol<" #TYPE "> should be " #EXPECTED)

// the shape, with cv and references removed first
D_INTERNAL_IP_CHECK(djinterp::internal::ip_shape, 1);
D_INTERNAL_IP_CHECK(const djinterp::internal::ip_shape, 1);
D_INTERNAL_IP_CHECK(djinterp::internal::ip_shape&, 1);

// a static function is a static member, as math's probe takes it
D_INTERNAL_IP_CHECK(djinterp::internal::ip_function_bounds, 1);

// a member missing, a bound that is not static, a flag that is not bool
D_INTERNAL_IP_CHECK(djinterp::internal::ip_no_upper, 0);
D_INTERNAL_IP_CHECK(djinterp::internal::ip_member_bounds, 0);
D_INTERNAL_IP_CHECK(djinterp::internal::ip_int_flags, 0);
D_INTERNAL_IP_CHECK(int, 0);

// closed_bounds has the shape, and states its range closed on both sides
typedef djinterp::closed_bounds<unsigned, 2u, 7u> ip_closed;

D_INTERNAL_IP_CHECK(ip_closed, 1);
D_STATIC_ASSERT((ip_closed::lower_bound == 2u),
                "closed_bounds: lower_bound is Lower");
D_STATIC_ASSERT((ip_closed::upper_bound == 7u),
                "closed_bounds: upper_bound is Upper");
D_STATIC_ASSERT((!ip_closed::is_left_open && !ip_closed::is_right_open),
                "closed_bounds: closed on both sides");

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

NS_DJINTERP
NS_INTERNAL

    // a container that states its size bound with closed_bounds, and one
    // whose size_interval does not have the shape
    struct ip_bounded_container
    {
        typedef int         value_type;
        typedef std::size_t size_type;
        typedef closed_bounds<std::size_t, 0, 16> size_interval;

        size_type size() const;
    };

    struct ip_misbounded_container
    {
        typedef int         value_type;
        typedef std::size_t size_type;
        typedef int         size_interval;

        size_type size() const;
    };

NS_END  // internal
NS_END  // djinterp

// container_traits reads the protocol, not math::is_interval
D_STATIC_ASSERT(
    (djinterp::has_valid_size_interval_v<
        djinterp::internal::ip_bounded_container>),
    "a closed_bounds size_interval is a valid size interval");
D_STATIC_ASSERT(
    (!djinterp::has_valid_size_interval_v<
        djinterp::internal::ip_misbounded_container>),
    "an int size_interval is not a valid size interval");

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
