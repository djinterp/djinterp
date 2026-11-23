/*******************************************************************************
* djinterp [cpp][meta]                                   trait_detect_parity.cpp
*
* Parity test for trait_detect.hpp's engines (decision 4.8): the C++98 sizeof
* engine, the void_t engine (C++11 and up) and the requires engine (C++20 with
* concepts) must give the same `::value` for the same type, and the public
* D_TYPE_TRAIT_HAS_* must agree with them; from C++11 the same holds for the
* expression shapes, D_TYPE_TRAIT_TRUE (and DETECTED, its old spelling),
* TRUE_AS and TRUE_FROM, and for the opt-in shapes, D_TYPE_TRAIT_HAS_BOOL_
* CONSTANT and D_TYPE_TRAIT_OPT_IN, whose void_t and requires forms are both
* checked where the level has them. The opt-in rows cover every shape a member
* of the probed name can take: a bool, an int 0, 1 or 2, an enumerator, a
* constexpr object, a static function, a non-static member, a type, and none.
* D_TYPE_TRAIT_HAS_METHOD_WITH's rows give the argument types at the use: one
* method name with two signatures, a const and a non-const call, overloads,
* and the wrong arity.
*   Compile-only: every check is a static assertion, so compiling this unit at
* a level runs the test there, and the ladder compiles it at every level.
* Below C++11 it checks the sizeof engine against the expected values; above,
* every engine the level has against the same values and against each other.
*
*
* path:      /tests/djinterp/cpp/meta/trait_detect_parity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.01
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "../../../../inc/djinterp/cpp/meta/trait_detect.hpp"  // the engines
// std
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <cstddef>      // std::size_t
    #include <type_traits>  // std::is_integral, std::is_class
    #include <utility>      // std::declval
#endif


NS_DJINTERP
NS_INTERNAL

    // the probe types: one shape each, and one without any
    struct parity_none
    {};

    struct parity_type
    {
        typedef int  value_type;
        typedef int& ref_type;
    };

    struct parity_static
    {
        static int count;
        static
        int make();
    };

    struct parity_method
    {
        typedef int value_type;

        void push(
            const value_type& _value
        );
    };

    // push takes a non-const lvalue reference: a prvalue argument cannot
    // bind to it, so the trait is false at every level
    struct parity_ref_method
    {
        typedef int value_type;

        void push(
            value_type& _value
        );
    };

    struct parity_ops
    {};

    parity_ops operator+(
        const parity_ops& _left,
        const parity_ops& _right
    );

    struct parity_negatable
    {
        parity_negatable operator-() const;
    };

    // the C++98 engine, at every level
    D_INTERNAL_TYPE_TRAIT_98_HAS_TYPE(p98_has_value_type, value_type)
    D_INTERNAL_TYPE_TRAIT_98_HAS_TYPE(p98_has_ref_type, ref_type)
    D_INTERNAL_TYPE_TRAIT_98_HAS_STATIC_MEMBER(p98_has_count, count)
    D_INTERNAL_TYPE_TRAIT_98_HAS_STATIC_MEMBER(p98_has_make, make)
    D_INTERNAL_TYPE_TRAIT_98_HAS_METHOD(p98_has_push, push)
    D_INTERNAL_TYPE_TRAIT_98_HAS_BINARY_OP(p98_has_plus, +)
    D_INTERNAL_TYPE_TRAIT_98_HAS_UNARY_OP(p98_has_minus, -)

    // the public names, whichever engine the level selects
    D_TYPE_TRAIT_HAS_TYPE(pub_has_value_type, value_type)
    D_TYPE_TRAIT_HAS_TYPE(pub_has_ref_type, ref_type)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(pub_has_count, count)
    D_TYPE_TRAIT_HAS_STATIC_MEMBER(pub_has_make, make)
    D_TYPE_TRAIT_HAS_METHOD(pub_has_push, push)
    D_TYPE_TRAIT_HAS_BINARY_OP(pub_has_plus, +)
    D_TYPE_TRAIT_HAS_UNARY_OP(pub_has_minus, -)

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_INTERNAL_TYPE_TRAIT_11_HAS_TYPE(p11_has_value_type, value_type)
    D_INTERNAL_TYPE_TRAIT_11_HAS_TYPE(p11_has_ref_type, ref_type)
    D_INTERNAL_TYPE_TRAIT_11_HAS_STATIC_MEMBER(p11_has_count, count)
    D_INTERNAL_TYPE_TRAIT_11_HAS_STATIC_MEMBER(p11_has_make, make)
    D_INTERNAL_TYPE_TRAIT_11_HAS_METHOD(p11_has_push, push)
    D_INTERNAL_TYPE_TRAIT_11_HAS_BINARY_OP(p11_has_plus, +)
    D_INTERNAL_TYPE_TRAIT_11_HAS_UNARY_OP(p11_has_minus, -)
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // the expression shapes' probe types: a const size(), the same size()
    // private, and a value_type that is not integral
    struct parity_sized
    {
        std::size_t size() const;
    };

    class parity_private
    {
    private:
        std::size_t size() const;
    };

    struct parity_class_value
    {
        typedef parity_none value_type;
    };

    // the expression shapes from the void_t engine, at every level from
    // C++11; one expression, two, a nested type, and TRUE_AS's base
    D_INTERNAL_TYPE_TRAIT_11_TRUE(p11_true_size,
        decltype(std::declval<const Type&>().size()))
    D_INTERNAL_TYPE_TRAIT_11_TRUE(p11_true_value_push,
        typename Type::value_type,
        decltype(std::declval<Type&>().push(
            std::declval<typename Type::value_type>())))
    D_INTERNAL_TYPE_TRAIT_11_TRUE(p11_true_value_type,
        typename Type::value_type)
    D_INTERNAL_TYPE_TRAIT_11_TRUE_AS(p11_true_as_integral,
        typename Type::value_type,
        std::is_integral<typename Type::value_type>)

    // the public names, whichever engine the level selects; DETECTED is
    // TRUE's old spelling, and TRUE_FROM inherits a unary trait on success
    D_TYPE_TRAIT_TRUE(pub_true_size,
        decltype(std::declval<const Type&>().size()))
    D_TYPE_TRAIT_TRUE(pub_true_value_push,
        typename Type::value_type,
        decltype(std::declval<Type&>().push(
            std::declval<typename Type::value_type>())))
    D_TYPE_TRAIT_DETECTED(pub_true_value_type,
        typename Type::value_type)
    D_TYPE_TRAIT_TRUE_AS(pub_true_as_integral,
        typename Type::value_type,
        std::is_integral<typename Type::value_type>)
    D_TYPE_TRAIT_TRUE_FROM(pub_true_from_class,
        typename Type::value_type,
        std::is_class)

    // the opt-in shapes' probe types: one per shape a member named `flag`
    // can take, and a nested `capability::can` for the qualified name
    struct parity_flag_true
    {
        static constexpr bool flag = true;
    };

    struct parity_flag_false
    {
        static constexpr bool flag = false;
    };

    struct parity_flag_zero
    {
        static constexpr int flag = 0;
    };

    struct parity_flag_one
    {
        static constexpr int flag = 1;
    };

    // 2 does not convert to bool without narrowing
    struct parity_flag_two
    {
        static constexpr int flag = 2;
    };

    struct parity_flag_enum
    {
        enum { flag = 1 };
    };

    struct parity_flag_object
    {
        static constexpr std::true_type flag{};
    };

    // the three that are not constants: decltype and a cast to bool read the
    // function as true, and the member as a hard error
    struct parity_flag_function
    {
        static
        bool flag();
    };

    struct parity_flag_member
    {
        bool flag;
    };

    struct parity_flag_type
    {
        typedef bool flag;
    };

    struct parity_capability
    {
        struct capability
        {
            static constexpr bool can = true;
        };
    };

    struct parity_capability_off
    {
        struct capability
        {
            static constexpr bool can = false;
        };
    };

    D_INTERNAL_TYPE_TRAIT_11_HAS_BOOL_CONSTANT(p11_has_flag_constant, flag)
    D_INTERNAL_TYPE_TRAIT_11_OPT_IN(p11_opt_in_flag, flag)
    D_INTERNAL_TYPE_TRAIT_11_OPT_IN(p11_opt_in_can, capability::can)

    D_TYPE_TRAIT_HAS_BOOL_CONSTANT(pub_has_flag_constant, flag)
    D_TYPE_TRAIT_OPT_IN(pub_opt_in_flag, flag)
    D_TYPE_TRAIT_OPT_IN(pub_opt_in_can, capability::can)

    // HAS_METHOD_WITH's probe types: is_root with and without an argument,
    // an add callable only on a non-const object, and an overloaded add
    struct parity_is_root_id
    {
        bool is_root(
            int _id
        ) const;
    };

    struct parity_is_root_bare
    {
        bool is_root() const;
    };

    struct parity_add_mutable
    {
        void add(
            int _value
        );
    };

    struct parity_add_overloaded
    {
        void add(
            int _value
        );

        void add(
            int         _value,
            const char* _name
        );
    };
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) )
    // the expression shapes from the requires engine
    D_INTERNAL_TYPE_TRAIT_20_TRUE(p20_true_size,
        decltype(std::declval<const Type&>().size()))
    D_INTERNAL_TYPE_TRAIT_20_TRUE(p20_true_value_push,
        typename Type::value_type,
        decltype(std::declval<Type&>().push(
            std::declval<typename Type::value_type>())))
    D_INTERNAL_TYPE_TRAIT_20_TRUE(p20_true_value_type,
        typename Type::value_type)
    D_INTERNAL_TYPE_TRAIT_20_TRUE_AS(p20_true_as_integral,
        typename Type::value_type,
        std::is_integral<typename Type::value_type>)

    D_INTERNAL_TYPE_TRAIT_20_HAS_TYPE(p20_has_value_type, value_type)
    D_INTERNAL_TYPE_TRAIT_20_HAS_TYPE(p20_has_ref_type, ref_type)
    D_INTERNAL_TYPE_TRAIT_20_HAS_STATIC_MEMBER(p20_has_count, count)
    D_INTERNAL_TYPE_TRAIT_20_HAS_STATIC_MEMBER(p20_has_make, make)
    D_INTERNAL_TYPE_TRAIT_20_HAS_METHOD(p20_has_push, push)
    D_INTERNAL_TYPE_TRAIT_20_HAS_BINARY_OP(p20_has_plus, +)
    D_INTERNAL_TYPE_TRAIT_20_HAS_UNARY_OP(p20_has_minus, -)

    // the opt-in shapes from the requires engine
    D_INTERNAL_TYPE_TRAIT_20_HAS_BOOL_CONSTANT(p20_has_flag_constant, flag)
    D_INTERNAL_TYPE_TRAIT_20_OPT_IN(p20_opt_in_flag, flag)
    D_INTERNAL_TYPE_TRAIT_20_OPT_IN(p20_opt_in_can, capability::can)
#endif

NS_END  // internal

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // HAS_METHOD_WITH emits its detector into `internal`, so it is invoked at
    // djinterp's scope (trait_detect.hpp's INVOCATION SCOPE)
    D_INTERNAL_TYPE_TRAIT_11_HAS_METHOD_WITH(p11_has_is_root_with, is_root)
    D_INTERNAL_TYPE_TRAIT_11_HAS_METHOD_WITH(p11_has_add_with, add)

    D_TYPE_TRAIT_HAS_METHOD_WITH(pub_has_is_root_with, is_root)
    D_TYPE_TRAIT_HAS_METHOD_WITH(pub_has_add_with, add)
#endif

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) )
    D_INTERNAL_TYPE_TRAIT_20_HAS_METHOD_WITH(p20_has_is_root_with, is_root)
    D_INTERNAL_TYPE_TRAIT_20_HAS_METHOD_WITH(p20_has_add_with, add)
#endif

NS_END  // djinterp


// D_INTERNAL_PARITY_CHECK
//   macro (internal): one row of the table - TRAIT applied to TYPE gives
// EXPECTED from every engine this level has.
#define D_INTERNAL_PARITY_CHECK_ONE(PREFIX, TRAIT, TYPE, EXPECTED)            \
    D_STATIC_ASSERT(                                                          \
        (djinterp::internal::PREFIX##_##TRAIT<TYPE>::value == (EXPECTED)),    \
        #PREFIX "_" #TRAIT "<" #TYPE "> should be " #EXPECTED)

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) )
    #define D_INTERNAL_PARITY_CHECK_20(TRAIT, TYPE, EXPECTED)                 \
        D_INTERNAL_PARITY_CHECK_ONE(p20, TRAIT, TYPE, EXPECTED);
#else
    #define D_INTERNAL_PARITY_CHECK_20(TRAIT, TYPE, EXPECTED)
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_INTERNAL_PARITY_CHECK_11(TRAIT, TYPE, EXPECTED)                 \
        D_INTERNAL_PARITY_CHECK_ONE(p11, TRAIT, TYPE, EXPECTED);
#else
    #define D_INTERNAL_PARITY_CHECK_11(TRAIT, TYPE, EXPECTED)
#endif

#define D_INTERNAL_PARITY_CHECK(TRAIT, TYPE, EXPECTED)                        \
    D_INTERNAL_PARITY_CHECK_ONE(p98, TRAIT, TYPE, EXPECTED);                  \
    D_INTERNAL_PARITY_CHECK_ONE(pub, TRAIT, TYPE, EXPECTED);                  \
    D_INTERNAL_PARITY_CHECK_11(TRAIT, TYPE, EXPECTED)                         \
    D_INTERNAL_PARITY_CHECK_20(TRAIT, TYPE, EXPECTED)

// nested types, with cv and references removed first
D_INTERNAL_PARITY_CHECK(has_value_type, djinterp::internal::parity_type, 1)
D_INTERNAL_PARITY_CHECK(has_value_type, const djinterp::internal::parity_type, 1)
D_INTERNAL_PARITY_CHECK(has_value_type, djinterp::internal::parity_type&, 1)
D_INTERNAL_PARITY_CHECK(has_value_type, djinterp::internal::parity_none, 0)
D_INTERNAL_PARITY_CHECK(has_value_type, int, 0)
D_INTERNAL_PARITY_CHECK(has_ref_type, djinterp::internal::parity_type, 1)
D_INTERNAL_PARITY_CHECK(has_ref_type, djinterp::internal::parity_none, 0)

// static members, data and function
D_INTERNAL_PARITY_CHECK(has_count, djinterp::internal::parity_static, 1)
D_INTERNAL_PARITY_CHECK(has_make, djinterp::internal::parity_static, 1)
D_INTERNAL_PARITY_CHECK(has_count, djinterp::internal::parity_none, 0)
D_INTERNAL_PARITY_CHECK(has_make, int, 0)

// a method called with a prvalue value_type
D_INTERNAL_PARITY_CHECK(has_push, djinterp::internal::parity_method, 1)
D_INTERNAL_PARITY_CHECK(has_push, djinterp::internal::parity_ref_method, 0)
D_INTERNAL_PARITY_CHECK(has_push, djinterp::internal::parity_type, 0)
D_INTERNAL_PARITY_CHECK(has_push, int, 0)

// operators on the type itself, built-in and overloaded
D_INTERNAL_PARITY_CHECK(has_plus, djinterp::internal::parity_ops, 1)
D_INTERNAL_PARITY_CHECK(has_plus, int, 1)
D_INTERNAL_PARITY_CHECK(has_plus, djinterp::internal::parity_none, 0)
D_INTERNAL_PARITY_CHECK(has_minus, djinterp::internal::parity_negatable, 1)
D_INTERNAL_PARITY_CHECK(has_minus, int, 1)
D_INTERNAL_PARITY_CHECK(has_minus, djinterp::internal::parity_ops, 0)

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// D_INTERNAL_PARITY_CHECK_EXPR
//   macro (internal): one row for an expression shape - TRAIT applied to
// TYPE gives EXPECTED from the void_t engine, the public name, and the
// requires engine where the level has it. No C++98 column: the shape takes
// a type expression, which needs decltype.
#define D_INTERNAL_PARITY_CHECK_EXPR(TRAIT, TYPE, EXPECTED)                   \
    D_INTERNAL_PARITY_CHECK_ONE(p11, TRAIT, TYPE, EXPECTED);                  \
    D_INTERNAL_PARITY_CHECK_ONE(pub, TRAIT, TYPE, EXPECTED);                  \
    D_INTERNAL_PARITY_CHECK_20(TRAIT, TYPE, EXPECTED)

// one expression: a const member call, public, private, absent, and on a
// non-class type
D_INTERNAL_PARITY_CHECK_EXPR(true_size, djinterp::internal::parity_sized, 1)
D_INTERNAL_PARITY_CHECK_EXPR(true_size,
                             const djinterp::internal::parity_sized,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(true_size, djinterp::internal::parity_private, 0)
D_INTERNAL_PARITY_CHECK_EXPR(true_size, djinterp::internal::parity_none, 0)
D_INTERNAL_PARITY_CHECK_EXPR(true_size, int, 0)

// two expressions, both required: the nested type and a call that uses it
D_INTERNAL_PARITY_CHECK_EXPR(true_value_push,
                             djinterp::internal::parity_method,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(true_value_push,
                             djinterp::internal::parity_ref_method,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(true_value_push,
                             djinterp::internal::parity_type,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(true_value_push,
                             djinterp::internal::parity_none,
                             0)

// a nested type, through TRUE (p11, p20) and through DETECTED (pub)
D_INTERNAL_PARITY_CHECK_EXPR(true_value_type,
                             djinterp::internal::parity_type,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(true_value_type,
                             djinterp::internal::parity_none,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(true_value_type, int, 0)

// TRUE_AS: the base on success (true for an integral value_type, false for
// a class one), std::false_type where the detection fails, and the base is
// formed only where it can be
D_INTERNAL_PARITY_CHECK_EXPR(true_as_integral,
                             djinterp::internal::parity_type,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(true_as_integral,
                             djinterp::internal::parity_class_value,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(true_as_integral,
                             djinterp::internal::parity_none,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(true_as_integral, int, 0)

// TRUE_FROM, from the public name only: BASE_TRAIT<Type> on success
D_INTERNAL_PARITY_CHECK_ONE(pub, true_from_class,
                            djinterp::internal::parity_type, 1);
D_INTERNAL_PARITY_CHECK_ONE(pub, true_from_class,
                            djinterp::internal::parity_none, 0);
D_INTERNAL_PARITY_CHECK_ONE(pub, true_from_class, int, 0);

// HAS_BOOL_CONSTANT: a constant usable as a bool template argument - bool,
// 0 or 1, an enumerator, a constexpr object - and nothing else: not 2 (a
// narrowing conversion), a static function, a non-static member, or a type
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_true,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             const djinterp::internal::parity_flag_true&,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_false,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_zero,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_one,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_two,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_enum,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_object,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_function,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_member,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_flag_type,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant,
                             djinterp::internal::parity_none,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(has_flag_constant, int, 0)

// OPT_IN: the constant's value where HAS_BOOL_CONSTANT holds, else false
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_true,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             const djinterp::internal::parity_flag_true&,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_false,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_zero,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_one,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_two,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_enum,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_object,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_function,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_member,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag,
                             djinterp::internal::parity_flag_type,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag, djinterp::internal::parity_none, 0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_flag, int, 0)

// a qualified name: the nested struct's constant, or false without one
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_can,
                             djinterp::internal::parity_capability,
                             1)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_can,
                             djinterp::internal::parity_capability_off,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_can,
                             djinterp::internal::parity_flag_true,
                             0)
D_INTERNAL_PARITY_CHECK_EXPR(opt_in_can, int, 0)

// the _v companions, where the level has variable templates
#if ( (D_ENV_LANG_IS_CPP17_OR_HIGHER) ||                                      \
      (D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES) )
D_STATIC_ASSERT(
    (djinterp::internal::pub_opt_in_flag_v<
        djinterp::internal::parity_flag_one>),
    "pub_opt_in_flag_v<parity_flag_one> should be 1");
D_STATIC_ASSERT(
    (!djinterp::internal::pub_has_flag_constant_v<
        djinterp::internal::parity_flag_function>),
    "pub_has_flag_constant_v<parity_flag_function> should be 0");
#endif

// D_INTERNAL_PARITY_CHECK_WITH
//   macro (internal): one row for HAS_METHOD_WITH - TRAIT applied to the
// type and argument types after EXPECTED gives EXPECTED from the void_t
// engine, the public name, and the requires engine where the level has it.
#define D_INTERNAL_PARITY_CHECK_WITH_ONE(PREFIX, TRAIT, EXPECTED, ...)        \
    D_STATIC_ASSERT(                                                          \
        (djinterp::PREFIX##_##TRAIT<__VA_ARGS__>::value == (EXPECTED)),      \
        #PREFIX "_" #TRAIT "<" #__VA_ARGS__ "> should be " #EXPECTED)

#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                      \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) )
    #define D_INTERNAL_PARITY_CHECK_WITH_20(TRAIT, EXPECTED, ...)            \
        D_INTERNAL_PARITY_CHECK_WITH_ONE(p20, TRAIT, EXPECTED, __VA_ARGS__);
#else
    #define D_INTERNAL_PARITY_CHECK_WITH_20(TRAIT, EXPECTED, ...)
#endif

#define D_INTERNAL_PARITY_CHECK_WITH(TRAIT, EXPECTED, ...)                    \
    D_INTERNAL_PARITY_CHECK_WITH_ONE(p11, TRAIT, EXPECTED, __VA_ARGS__);      \
    D_INTERNAL_PARITY_CHECK_WITH_ONE(pub, TRAIT, EXPECTED, __VA_ARGS__);      \
    D_INTERNAL_PARITY_CHECK_WITH_20(TRAIT, EXPECTED, __VA_ARGS__)

// HAS_METHOD_WITH: one name, the arguments given at the use - is_root with
// an id and without, a call on a const object and on a non-const one, an
// overload chosen by its arguments, and the wrong arity
D_INTERNAL_PARITY_CHECK_WITH(has_is_root_with,
                             1,
                             const djinterp::internal::parity_is_root_id,
                             int)
D_INTERNAL_PARITY_CHECK_WITH(has_is_root_with,
                             0,
                             const djinterp::internal::parity_is_root_id)
D_INTERNAL_PARITY_CHECK_WITH(has_is_root_with,
                             1,
                             const djinterp::internal::parity_is_root_bare)
D_INTERNAL_PARITY_CHECK_WITH(has_is_root_with,
                             0,
                             const djinterp::internal::parity_is_root_bare,
                             int)
D_INTERNAL_PARITY_CHECK_WITH(has_is_root_with,
                             1,
                             djinterp::internal::parity_is_root_id&,
                             long)
D_INTERNAL_PARITY_CHECK_WITH(has_is_root_with,
                             0,
                             djinterp::internal::parity_none)
D_INTERNAL_PARITY_CHECK_WITH(has_is_root_with, 0, int, int)
D_INTERNAL_PARITY_CHECK_WITH(has_add_with,
                             1,
                             djinterp::internal::parity_add_mutable,
                             int)
D_INTERNAL_PARITY_CHECK_WITH(has_add_with,
                             0,
                             const djinterp::internal::parity_add_mutable,
                             int)
D_INTERNAL_PARITY_CHECK_WITH(has_add_with,
                             0,
                             djinterp::internal::parity_add_mutable)
D_INTERNAL_PARITY_CHECK_WITH(has_add_with,
                             1,
                             djinterp::internal::parity_add_overloaded,
                             int)
D_INTERNAL_PARITY_CHECK_WITH(has_add_with,
                             1,
                             djinterp::internal::parity_add_overloaded,
                             int,
                             const char*)
D_INTERNAL_PARITY_CHECK_WITH(has_add_with,
                             0,
                             djinterp::internal::parity_add_overloaded,
                             int,
                             int)
D_INTERNAL_PARITY_CHECK_WITH(has_add_with,
                             0,
                             djinterp::internal::parity_add_overloaded,
                             int,
                             const char*,
                             int)

// the _v companion takes the same arguments
#if ( (D_ENV_LANG_IS_CPP17_OR_HIGHER) ||                                      \
      (D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES) )
D_STATIC_ASSERT(
    (djinterp::pub_has_add_with_v<djinterp::internal::parity_add_mutable,
                                  int>),
    "pub_has_add_with_v<parity_add_mutable, int> should be 1");
#endif
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
