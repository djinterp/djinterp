/*******************************************************************************
* djinterp [cpp][math]                                                  lazy.hpp
*
* The opt-in lazy type of the linear-algebra subframework (roadmap step 34,
* 5f; decision 2: eager by default, laziness an explicit opt-in from C++11).
*   lazy(x) wraps a vector<T, N>, a matrix<T, R, C> or a dynamic_matrix<T>,
* by reference; the operators on what it returns build an expression and
* compute nothing. eval(e), or e.eval(), computes it into a new value of the
* type eager arithmetic would give, and assign(dst, e) into dst, each
* element once, with no temporary for each step:
*     const dynamic_matrix<double> r = eval(lazy(a) * 2.0 + lazy(b)
*                                           - lazy(c));   // one pass, one
*                                                          // allocation
*   THE SAME BITS. An element is computed by the same operations, in the
* same order, as the eager operations compute it (principle 6): a sum
* l + r, a difference l - r, a negation -l, a scaled l * s, a divided l / s
* (a vector's), an entry-wise product l * r, element by element. Where the
* build may fuse a multiply and an add into one instruction, which the
* eager loops, each storing its result, never do, each product is fenced, so
* that the next sum takes it rounded, as eager arithmetic does (the C core's
* math_common.h, section 5). A matrix product, matrix by matrix or matrix
* by vector, and a transpose are computed by the eager operations
* themselves, once, before the elements: a product's entries are each a
* compensated dot product, which no element loop could share. Where float
* and double are evaluated wider than their types, a 32-bit x87, every
* element is fenced, rounded to its type as an eager loop's store rounds it.
*   WHAT IT HOLDS. The leaves hold their operands by reference, as lazy(x)
* takes them, so an expression lives no longer than they do; lazy of a
* temporary is refused at compile time. Operands are lazy on both sides: a
* concrete operand is wrapped explicitly, lazy(b), so that nothing is lazy
* unless asked. Shapes are checked as eager arithmetic checks them: at
* compile time for the fixed-size types, by assert for dynamic_matrix.
*   ASSIGNING. assign(dst, e) writes each element of e into dst in place
* where e reads dst, if at all, only at that element: sums, differences,
* scalings, negations, entry-wise products, and products and transposes,
* which are computed before the first element is written. dst takes e's
* shape, as an eager assignment would.
*   LEVELS. From C++11, and empty below it (N1). No operation is a constant
* expression: the fence is a run-time barrier, and laziness is a run-time
* economy.
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/lazy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PARTS
    -----
    1.  The fence
    2.  The operands' types
    3.  The base
2.  NODES
    -----
    1.  The leaf
    2.  Element-wise nodes
    3.  Products and transposes
3.  THE ENTRIES
    -----------
    1.  lazy
    2.  Operators
    3.  eval, assign
*/

#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_LAZY_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_LAZY_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cassert>      // assert
#include <cfloat>       // FLT_EVAL_METHOD
#include <cstddef>      // std::size_t
#include <type_traits>  // std::enable_if, std::is_arithmetic, std::declval
#include <utility>      // std::declval
// djinterp
#include "./dynamic_matrix.hpp"  // dynamic_matrix
#include "./linalg_common.hpp"   // linalg
#include "./matrix.hpp"          // matrix
#include "./vector.hpp"          // vector


NS_DJINTERP
NS_MATH
namespace linalg
{


//==============================================================================
// 1.  PARTS
//==============================================================================

// D_LINALG_LAZY_FENCED
//   constant: 1 where the compiler may fuse a multiply and an add of the
// text into one instruction, so that a lazy product is fenced: gcc and clang
// on a target with a fused multiply-add (the C core's math_common.h, section
// 5, says which fuse and when); 0 elsewhere, where the fence is nothing.
#if ( ( defined(__GNUC__) || defined(__clang__) ) &&                           \
      ( defined(__FP_FAST_FMA)  || defined(__FP_FAST_FMAF) ||                  \
        defined(__FMA__)        || defined(__ARM_FEATURE_FMA) ||               \
        defined(__aarch64__) ) )
    #define D_LINALG_LAZY_FENCED 1
#else
    #define D_LINALG_LAZY_FENCED 0
#endif

// D_LINALG_LAZY_WIDE
//   constant: 1 where float and double are evaluated wider than their types
// (FLT_EVAL_METHOD not 0, a 32-bit x87) by gcc or clang, so that every lazy
// element is fenced, rounded to its type as an eager loop's store rounds
// it: clang keeps the wider value across an assignment there, as gcc does
// not where its excess precision is the standard's (S37); 0 elsewhere.
#if ( ( defined(__GNUC__) || defined(__clang__) ) &&                           \
      defined(FLT_EVAL_METHOD) && (FLT_EVAL_METHOD != 0) )
    #define D_LINALG_LAZY_WIDE 1
#else
    #define D_LINALG_LAZY_WIDE 0
#endif

namespace internal
{

// 1.1    The fence
//------------------------------------------------------------------------------
    // lazy_fence
    //   function: a product _v, rounded to its type and held in memory where
    // the build may fuse or evaluates wider, so that no add after it fuses
    // with it and it is of its type, as an eager loop's store makes it; _v
    // itself elsewhere.
    template<typename Type>
    inline Type
    lazy_fence(
        Type _v
    )
    {
    #if ( D_LINALG_LAZY_FENCED || D_LINALG_LAZY_WIDE )
        __asm__ __volatile__("" : : "r"(&_v) : "memory");
    #endif

        return _v;
    }

    // lazy_round
    //   function: any other element _v, rounded to its type and held in
    // memory where the build evaluates wider; _v itself elsewhere.
    template<typename Type>
    inline Type
    lazy_round(
        Type _v
    )
    {
    #if D_LINALG_LAZY_WIDE
        __asm__ __volatile__("" : : "r"(&_v) : "memory");
    #endif

        return _v;
    }


// 1.2    The operands' types
//------------------------------------------------------------------------------
    // lazy_shape
    //   trait: what lazy needs of each type it takes: its element type, its
    // shape, its elements in row-major order to read and write, and a value
    // of a shape to write into. Undefined for any other type, so that lazy
    // of one does not compile.
    template<typename Value>
    struct lazy_shape;

    template<typename    Type,
             std::size_t N>
    struct lazy_shape<vector<Type, N> >
    {
        typedef Type            value_type;
        typedef vector<Type, N> result_type;

        static const bool is_vector = true;

        static std::size_t rows(const result_type&) { return N; }
        static std::size_t cols(const result_type&) { return 1; }

        static const Type&
        at(const result_type& _x, std::size_t _i)
        {
            return _x[_i];
        }

        static void
        set(result_type& _x, std::size_t _r, std::size_t, const Type& _v)
        {
            _x[_r] = _v;
        }

        static result_type
        make(std::size_t, std::size_t)
        {
            return result_type();
        }
    };

    template<typename    Type,
             std::size_t Rows,
             std::size_t Cols>
    struct lazy_shape<matrix<Type, Rows, Cols> >
    {
        typedef Type                     value_type;
        typedef matrix<Type, Rows, Cols> result_type;

        static const bool is_vector = false;

        static std::size_t rows(const result_type&) { return Rows; }
        static std::size_t cols(const result_type&) { return Cols; }

        static const Type&
        at(const result_type& _x, std::size_t _i)
        {
            return _x.data()[_i];
        }

        static void
        set(result_type& _x, std::size_t _r, std::size_t _c, const Type& _v)
        {
            _x(_r, _c) = _v;
        }

        static result_type
        make(std::size_t, std::size_t)
        {
            return result_type();
        }
    };

    template<typename Type>
    struct lazy_shape<dynamic_matrix<Type> >
    {
        typedef Type                 value_type;
        typedef dynamic_matrix<Type> result_type;

        static const bool is_vector = false;

        static std::size_t rows(const result_type& _x) { return _x.rows(); }
        static std::size_t cols(const result_type& _x) { return _x.cols(); }

        static const Type&
        at(const result_type& _x, std::size_t _i)
        {
            return _x.data()[_i];
        }

        static void
        set(result_type& _x, std::size_t _r, std::size_t _c, const Type& _v)
        {
            _x(_r, _c) = _v;
        }

        static result_type
        make(std::size_t _rows, std::size_t _cols)
        {
            return result_type(_rows, _cols);
        }
    };

    // lazy_transposed
    //   trait: the type of a transpose of a value: a fixed-size matrix's
    // columns its rows, a dynamic_matrix a dynamic_matrix.
    template<typename Value>
    struct lazy_transposed;

    template<typename    Type,
             std::size_t Rows,
             std::size_t Cols>
    struct lazy_transposed<matrix<Type, Rows, Cols> >
    {
        typedef matrix<Type, Cols, Rows> type;
    };

    template<typename Type>
    struct lazy_transposed<dynamic_matrix<Type> >
    {
        typedef dynamic_matrix<Type> type;
    };

}  // namespace internal


// 1.3    The base
//------------------------------------------------------------------------------
// lazy_base
//   struct: what every lazy node is (CRTP): detected by is_lazy, and
// evaluated or assigned by its members as by eval and assign.
template<typename Derived>
struct lazy_base
{
    const Derived&
    self() const D_NOEXCEPT
    {
        return static_cast<const Derived&>(*this);
    }

    // eval, assign_to: as the free functions (section 3.3); eval a template
    // only so that its type is read once Derived is complete.
    template<typename Node = Derived>
    typename Node::result_type
    eval() const;

    template<typename Value>
    void
    assign_to(Value& _dst) const;
};

// is_lazy
//   trait: whether a type is a lazy node.
template<typename Type>
struct is_lazy
    : std::is_base_of<lazy_base<typename std::decay<Type>::type>,
                      typename std::decay<Type>::type>
{
};


//==============================================================================
// 2.  NODES
//==============================================================================
// Each node gives its element type and the type it evaluates to; its shape,
// from its operands', before anything is computed; prepare(), which computes
// its products and transposes once, before the elements; and at(i), its i-th
// element in row-major order, once prepared.

// 2.1    The leaf
//------------------------------------------------------------------------------
// lazy_ref
//   struct: a value, by reference.
template<typename Value>
struct lazy_ref
    : lazy_base<lazy_ref<Value> >
{
    typedef internal::lazy_shape<Value>    shape;
    typedef typename shape::value_type     value_type;
    typedef typename shape::result_type    result_type;

    const Value* value;

    explicit lazy_ref(const Value& _value) D_NOEXCEPT
        : value(&_value)
    {
    }

    std::size_t rows() const { return shape::rows(*value); }
    std::size_t cols() const { return shape::cols(*value); }

    void prepare() {}

    value_type
    at(std::size_t _i) const
    {
        return shape::at(*value, _i);
    }
};


// 2.2    Element-wise nodes
//------------------------------------------------------------------------------
namespace internal
{
    // the element operations, as the eager kernels' loops write them
    struct lazy_add
    {
        template<typename Type>
        static Type
        apply(const Type& _a, const Type& _b)
        {
            return lazy_round<Type>(_a + _b);
        }
    };

    struct lazy_sub
    {
        template<typename Type>
        static Type
        apply(const Type& _a, const Type& _b)
        {
            return lazy_round<Type>(_a - _b);
        }
    };

    struct lazy_hadamard
    {
        template<typename Type>
        static Type
        apply(const Type& _a, const Type& _b)
        {
            return lazy_fence<Type>(_a * _b);
        }
    };

    // lazy_same_shape
    //   function: whether two nodes are of one shape: always, for the fixed
    // types, whose shapes the types check.
    template<typename Left, typename Right>
    bool
    lazy_same_shape(const Left& _l, const Right& _r)
    {
        return ( (_l.rows() == _r.rows()) && (_l.cols() == _r.cols()) );
    }
}  // namespace internal

// lazy_binary
//   struct: an element-wise operation of two nodes of one shape and type.
template<typename Op,
         typename Left,
         typename Right>
struct lazy_binary
    : lazy_base<lazy_binary<Op, Left, Right> >
{
    typedef typename Left::value_type  value_type;
    typedef typename Left::result_type result_type;

    static_assert(std::is_same<typename Left::result_type,
                               typename Right::result_type>::value,
                  "lazy: an element-wise operation of two operands of "
                  "different types or shapes.");

    Left  left;
    Right right;

    lazy_binary(const Left& _l, const Right& _r)
        : left(_l)
        , right(_r)
    {
        assert(internal::lazy_same_shape(_l, _r) &&
               "lazy: an element-wise operation of two operands of "
               "different shapes.");
    }

    std::size_t rows() const { return left.rows(); }
    std::size_t cols() const { return left.cols(); }

    void
    prepare()
    {
        left.prepare();
        right.prepare();
    }

    value_type
    at(std::size_t _i) const
    {
        return Op::template apply<value_type>(left.at(_i), right.at(_i));
    }
};

// lazy_scaled
//   struct: a node times a scalar, its product fenced.
template<typename Arg>
struct lazy_scaled
    : lazy_base<lazy_scaled<Arg> >
{
    typedef typename Arg::value_type  value_type;
    typedef typename Arg::result_type result_type;

    Arg        arg;
    value_type factor;

    lazy_scaled(const Arg& _a, const value_type& _s)
        : arg(_a)
        , factor(_s)
    {
    }

    std::size_t rows() const { return arg.rows(); }
    std::size_t cols() const { return arg.cols(); }

    void prepare() { arg.prepare(); }

    value_type
    at(std::size_t _i) const
    {
        return internal::lazy_fence<value_type>(arg.at(_i) * factor);
    }
};

// lazy_divided
//   struct: a vector node divided by a scalar.
template<typename Arg>
struct lazy_divided
    : lazy_base<lazy_divided<Arg> >
{
    typedef typename Arg::value_type  value_type;
    typedef typename Arg::result_type result_type;

    Arg        arg;
    value_type divisor;

    lazy_divided(const Arg& _a, const value_type& _s)
        : arg(_a)
        , divisor(_s)
    {
    }

    std::size_t rows() const { return arg.rows(); }
    std::size_t cols() const { return arg.cols(); }

    void prepare() { arg.prepare(); }

    value_type
    at(std::size_t _i) const
    {
        return internal::lazy_round<value_type>(arg.at(_i) / divisor);
    }
};

// lazy_negated
//   struct: a node negated.
template<typename Arg>
struct lazy_negated
    : lazy_base<lazy_negated<Arg> >
{
    typedef typename Arg::value_type  value_type;
    typedef typename Arg::result_type result_type;

    Arg arg;

    explicit lazy_negated(const Arg& _a)
        : arg(_a)
    {
    }

    std::size_t rows() const { return arg.rows(); }
    std::size_t cols() const { return arg.cols(); }

    void prepare() { arg.prepare(); }

    value_type
    at(std::size_t _i) const
    {
        return internal::lazy_round<value_type>(-arg.at(_i));
    }
};


// 2.3    Products and transposes
//------------------------------------------------------------------------------
namespace internal
{
    // lazy_operand
    //   function: a node's value for an eager operation: a leaf's own, by
    // reference, and any other node's, evaluated.
    template<typename Value>
    const Value&
    lazy_operand(const lazy_ref<Value>& _leaf)
    {
        return *_leaf.value;
    }

    template<typename Node>
    typename Node::result_type
    lazy_operand(const Node& _node)
    {
        return _node.eval();
    }
}  // namespace internal

// lazy_product
//   struct: the matrix product of two nodes, matrix by matrix or matrix by
// vector: the eager product of their values, computed once by prepare().
template<typename Left,
         typename Right>
struct lazy_product
    : lazy_base<lazy_product<Left, Right> >
{
    typedef decltype(std::declval<const typename Left::result_type&>() *
                     std::declval<const typename Right::result_type&>())
        result_type;
    typedef internal::lazy_shape<result_type> shape;
    typedef typename shape::value_type        value_type;

    Left        left;
    Right       right;
    result_type product;

    lazy_product(const Left& _l, const Right& _r)
        : left(_l)
        , right(_r)
        , product()
    {
    }

    // the product's shape, known before it is computed
    std::size_t rows() const { return left.rows(); }
    std::size_t cols() const { return right.cols(); }

    void
    prepare()
    {
        product = internal::lazy_operand(left) *
                  internal::lazy_operand(right);
    }

    value_type
    at(std::size_t _i) const
    {
        return shape::at(product, _i);
    }
};

// lazy_transpose
//   struct: the transpose of a matrix node: the eager transpose of its
// value, computed once by prepare().
template<typename Arg>
struct lazy_transpose
    : lazy_base<lazy_transpose<Arg> >
{
    typedef typename internal::lazy_transposed<
        typename Arg::result_type>::type        result_type;
    typedef internal::lazy_shape<result_type>   shape;
    typedef typename shape::value_type          value_type;

    Arg         arg;
    result_type transposed;

    explicit lazy_transpose(const Arg& _a)
        : arg(_a)
        , transposed()
    {
    }

    // the transpose's shape, known before it is computed
    std::size_t rows() const { return arg.cols(); }
    std::size_t cols() const { return arg.rows(); }

    void
    prepare()
    {
        transposed = internal::lazy_operand(arg).transposed();
    }

    value_type
    at(std::size_t _i) const
    {
        return shape::at(transposed, _i);
    }
};


//==============================================================================
// 3.  THE ENTRIES
//==============================================================================

// 3.1    lazy
//------------------------------------------------------------------------------
// lazy
//   function: a vector, matrix or dynamic_matrix as the leaf of a lazy
// expression, by reference; of a temporary, refused.
template<typename Value>
lazy_ref<Value>
lazy(
    const Value& _value
)
{
    return lazy_ref<Value>(_value);
}

template<typename Value>
void lazy(const Value&& _value) = delete;


// 3.2    Operators
//------------------------------------------------------------------------------
// Both operands of + - and a product lazy nodes; a scalar any arithmetic
// type, converted to the element type, as eager arithmetic converts it.

template<typename Left,
         typename Right,
         typename std::enable_if<is_lazy<Left>::value &&
                                 is_lazy<Right>::value, int>::type = 0>
lazy_binary<internal::lazy_add, Left, Right>
operator+(
    const Left&  _l,
    const Right& _r
)
{
    return lazy_binary<internal::lazy_add, Left, Right>(_l, _r);
}

template<typename Left,
         typename Right,
         typename std::enable_if<is_lazy<Left>::value &&
                                 is_lazy<Right>::value, int>::type = 0>
lazy_binary<internal::lazy_sub, Left, Right>
operator-(
    const Left&  _l,
    const Right& _r
)
{
    return lazy_binary<internal::lazy_sub, Left, Right>(_l, _r);
}

template<typename Arg,
         typename std::enable_if<is_lazy<Arg>::value, int>::type = 0>
lazy_negated<Arg>
operator-(
    const Arg& _a
)
{
    return lazy_negated<Arg>(_a);
}

template<typename Arg,
         typename Scalar,
         typename std::enable_if<is_lazy<Arg>::value &&
                                 std::is_arithmetic<Scalar>::value,
                                 int>::type = 0>
lazy_scaled<Arg>
operator*(
    const Arg& _a,
    Scalar     _s
)
{
    return lazy_scaled<Arg>(_a,
                            static_cast<typename Arg::value_type>(_s));
}

template<typename Scalar,
         typename Arg,
         typename std::enable_if<std::is_arithmetic<Scalar>::value &&
                                 is_lazy<Arg>::value, int>::type = 0>
lazy_scaled<Arg>
operator*(
    Scalar     _s,
    const Arg& _a
)
{
    return lazy_scaled<Arg>(_a,
                            static_cast<typename Arg::value_type>(_s));
}

// a vector divided by a scalar, as vector's operator/ is; a matrix has none
template<typename Arg,
         typename Scalar,
         typename std::enable_if<is_lazy<Arg>::value &&
                                 internal::lazy_shape<
                                     typename Arg::result_type>::is_vector &&
                                 std::is_arithmetic<Scalar>::value,
                                 int>::type = 0>
lazy_divided<Arg>
operator/(
    const Arg& _a,
    Scalar     _s
)
{
    return lazy_divided<Arg>(_a,
                             static_cast<typename Arg::value_type>(_s));
}

// the matrix product, of two lazy nodes whose values eager operator* takes
template<typename Left,
         typename Right,
         typename std::enable_if<is_lazy<Left>::value &&
                                 is_lazy<Right>::value, int>::type = 0>
lazy_product<Left, Right>
operator*(
    const Left&  _l,
    const Right& _r
)
{
    return lazy_product<Left, Right>(_l, _r);
}

// hadamard: the entry-wise product of two nodes of one shape.
template<typename Left,
         typename Right,
         typename std::enable_if<is_lazy<Left>::value &&
                                 is_lazy<Right>::value, int>::type = 0>
lazy_binary<internal::lazy_hadamard, Left, Right>
hadamard(
    const Left&  _l,
    const Right& _r
)
{
    return lazy_binary<internal::lazy_hadamard, Left, Right>(_l, _r);
}

// transpose: the transpose of a matrix node.
template<typename Arg,
         typename std::enable_if<is_lazy<Arg>::value, int>::type = 0>
lazy_transpose<Arg>
transpose(
    const Arg& _a
)
{
    return lazy_transpose<Arg>(_a);
}


// 3.3    eval, assign
//------------------------------------------------------------------------------
namespace internal
{
    // lazy_write
    //   function: a prepared node's elements written into _out, of its
    // shape, each once, in row-major order.
    template<typename Node>
    void
    lazy_write(
        typename Node::result_type& _out,
        const Node&                 _work
    )
    {
        typedef lazy_shape<typename Node::result_type> shape;

        const std::size_t rows = _work.rows();
        const std::size_t cols = _work.cols();
        std::size_t       i    = 0;

        for (std::size_t r = 0; r < rows; ++r)
        {
            for (std::size_t c = 0; c < cols; ++c, ++i)
            {
                shape::set(_out, r, c, _work.at(i));
            }
        }

        return;
    }
}  // namespace internal

// eval
//   function: a lazy expression computed into a new value of the type eager
// arithmetic gives, each element once: its products and transposes first.
template<typename Node,
         typename std::enable_if<is_lazy<Node>::value, int>::type = 0>
typename Node::result_type
eval(
    const Node& _e
)
{
    typedef internal::lazy_shape<typename Node::result_type> shape;

    Node work(_e);

    work.prepare();

    typename Node::result_type out = shape::make(work.rows(), work.cols());

    internal::lazy_write(out, work);

    return out;
}

// assign
//   function: a lazy expression computed into _dst, in place, each element
// once, _dst taking its shape: an element of _dst the expression reads, if
// it reads one, is read only for the element written there, and its
// products and transposes are computed before the first is written.
template<typename Node,
         typename std::enable_if<is_lazy<Node>::value, int>::type = 0>
void
assign(
    typename Node::result_type& _dst,
    const Node&                 _e
)
{
    typedef internal::lazy_shape<typename Node::result_type> shape;

    Node work(_e);

    work.prepare();

    // a dynamic_matrix of another shape takes the expression's
    if ( (shape::rows(_dst) != work.rows()) ||
         (shape::cols(_dst) != work.cols()) )
    {
        _dst = shape::make(work.rows(), work.cols());
    }

    internal::lazy_write(_dst, work);

    return;
}

// the base's members
template<typename Derived>
template<typename Node>
typename Node::result_type
lazy_base<Derived>::eval() const
{
    return ::djinterp::math::linalg::eval(self());
}

template<typename Derived>
template<typename Value>
void
lazy_base<Derived>::assign_to(
    Value& _dst
) const
{
    ::djinterp::math::linalg::assign(_dst, self());

    return;
}


}  // linalg
NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_LAZY_HPP
