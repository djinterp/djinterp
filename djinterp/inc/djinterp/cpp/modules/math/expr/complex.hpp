/*******************************************************************************
* djinterp [cpp/math]                                                complex.hpp
*
* Complex numbers as values of the run-time expression language; roadmap
* Phase 3, step 23 (3e), its first part.
*   THE KIND is a boxed kind of the value layer, as the matrix is
* (matrix.hpp, S237), written here, in C++, from C++11, as the language that
* makes complex numbers is. It is one kind, "complex", of the rank 900, past
* every real kind's, so that in an operation of a complex number and a real
* one the complex number's table handles it, and below the quaternion's 950
* and the matrix's 1000 (S271). A complex number is a number, exact or not
* value by value, as its parts are, which its table's exact entry says
* (S325).
*   ITS PARTS are of one real kind (S266, S327): exact where both are given
* exact, an integer, a rational or a wide integer, each kept as a value, so
* that (1 + 2im) (3 - im) is 5 + 5im exactly; and otherwise of one
* floating-point type, the wider of the two, each part its nearest there,
* kept densely as math::complex<T>, the C core's struct (math/complex.hpp).
* An exact complex number whose imaginary part comes to 0 is its real part, a
* number of the real kinds, as a rational whose denominator comes to 1 is an
* integer; so no complex value has exact parts and an imaginary part of 0.
* An inexact one keeps its parts, its zeros with their signs.
*   ARITHMETIC is the value's own, through the kind's table, with a complex or
* a real operand of any lower rank (S328). Exact operands compute exactly, by
* value.h's operations on the parts: (a + bi) (c + di) is (ac - bd) +
* (ad + bc)i, and a quotient is over c^2 + d^2, so that a part past the
* family becomes G2's long double, flagged, and the number one of long double
* parts. Otherwise an operation computes in the widest floating-point type
* of its operands, each exact part its nearest there, as cplx.h's arithmetic,
* a product's parts by exact products and a quotient scaled, each part
* correctly rounded or within a step of it; a real operand is taken as
* itself, not as x + 0i (C99's G.5), so that a zero's sign survives:
* (1 + 0im) * -1 is -1 - 0im. An exact division by an exact 0 is a pole, as a
* real one is. -z negates both parts. |z| is the modulus: of exact parts the
* root of the exact c^2 + d^2, exact where it is the square of a rational of
* the family, |3 + 4im| being 5, and otherwise the long double nearest the
* root of the long double nearest it, as norm([c, d]) is (S257); of
* floating-point parts cplx.h's abs_c, within a step of the modulus
* correctly rounded, as math::complex<T>'s is (S313).
*   COMPARISON is by the parts, in value.h's exact order: a complex number
* equals a number whose parts are each equal to its own, a real number's
* imaginary part being 0, so that 3 + 0.0im equals 3 and 1/2 + im equals
* 0.5 + 1.0im; and stands in no order otherwise, as no order of the complex
* numbers is a field's, nor with a NaN part (S329). The language's <, <=, >
* and >= of a complex number are a domain error (function.hpp, S271).
*   CONVERSION to a real type is of a number whose imaginary part is 0: an
* inexact complex number's, ±0, its real part's nearest float, double or
* long double; no exact complex number converts, as none is real, and none
* converts to an integer or a rational.
*   TEXT is the value layer's: the real part, then + or -, then the imaginary
* part's magnitude and im: 3 + 4im, 1/2 - 3/4im, 1.5 + 0im, -0.25 - 0im;
* exact parts as integers or fractions, floating-point parts in
* max_digits10 significant digits, in the classic locale, as a matrix's
* elements are, and a part that is no finite number by its name, nan + inf*im
* (S330). The language writes a complex number by render.hpp, in brackets,
* (3 + 4im), and reads it back as a term of its value.
*   HOLDERS are counted by value.h's d_math_boxed_hold and
* d_math_boxed_unhold, atomically wherever the compiler can (S134, S326): the
* language's constant im is one box, which every fold that reads it holds.
*   IN C++, expr_complex(re, im) makes the number of two parts, as the
* language's complex(x, y) does; expr_is_complex asks whether a value is
* one; expr_real, expr_imag, expr_conj and expr_arg are the language's
* real, imag, conj and arg of a number; expr_complex_of makes the value of
* a math::complex<T>, and expr_complex_to gives a number's nearest one; and
* expr_complex_apply computes an operation of numbers, real or complex, in
* the widest floating-point type of theirs, as the language's functions of
* a complex number take their principal values there (function.hpp).
*   STEPS. Step 23 (3e) comes in parts (S324, S335): this header is its
* first's, with the language's im, complex, real, imag, conj, arg, the
* modulus, the relations and the exact powers to a whole exponent
* (function.hpp, evaluate.hpp); its second, in two chunks, gives each
* function of the table its principal complex value, expr_complex_apply
* its dense one, the first those cplx.h has and the second exp2, log2,
* log10, log1p and expm1 with cplx.h's own; and its third, in chunk 43,
* matrices of complex numbers, matrix.hpp's, which hold this kind's numbers
* as elements, and densely math::complex<T>, and take real, imag, conj and
* arg of a matrix element by element (S348, S350).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/complex.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.31
*                                                            revised: 2026.11.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE KIND
    --------
    1.  Its storage
         1.  expr_complex_rank
         2.  expr_complex_storage
         3.  expr_complex_box
         4.  Reading a value
    2.  Making a complex number
         1.  expr_complex_kind
         2.  expr_complex_adopt, expr_complex_dense
         3.  expr_complex_made
    3.  Its entries
         1.  expr_complex_retain, expr_complex_release
         2.  expr_complex_unary
         3.  expr_complex_binary
         4.  expr_complex_cmp
         5.  expr_complex_convert
         6.  expr_complex_text
         7.  expr_complex_exact
    4.  Its table
         1.  expr_complex_kind
2.  COMPLEX VALUES
    --------------
    1.  Making and reading
         1.  expr_is_complex
         2.  expr_complex
         3.  expr_complex_of, expr_complex_to
    2.  The parts
         1.  expr_real, expr_imag
         2.  expr_conj
         3.  expr_arg
    3.  Functions
         1.  expr_complex_apply_as
         2.  expr_complex_apply
*/

#ifndef DJINTERP_MATH_EXPR_COMPLEX_HPP
#define DJINTERP_MATH_EXPR_COMPLEX_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cmath>      // std::isnan, std::isinf, std::signbit
#include <cstddef>    // std::size_t
#include <iomanip>    // std::setprecision
#include <limits>     // std::numeric_limits
#include <locale>     // std::locale
#include <new>        // std::nothrow
#include <sstream>    // std::ostringstream
#include <string>     // std::string
// djinterp
#include "../../../../c/modules/math/cplx.h"   // struct d_cplx_*, abs_c and arg_c
#include "../../../../c/modules/math/value.h"  // d_math_boxed_kind, the holders
#include "../complex.hpp"                      // math::complex<T>, cplx.h's face
#include "../math_common.hpp"                  // NS_MATH
#include "../value.hpp"                        // value, value_root_of


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE KIND
//==============================================================================


// 1.1    Its storage
//------------------------------------------------------------------------------
// 1.1.1
// expr_complex_rank
//   constant: the kind's rank, its place in the tower and its own: past
// every real kind's, the long double's 500 among them, so that in an
// operation of a complex number and a real one the complex number's table
// handles it; below the quaternion's 950 (S275) and the matrix's 1000
// (S238), whose tables handle a complex operand (S271).
D_CONSTEXPR_VAR int expr_complex_rank = 900;

NS_INTERNAL

    // 1.1.2
    // expr_complex_storage
    //   enum: how a complex number holds its parts: as values, both exact,
    // or both of one boxed kind that is not; or dense, as the C core's
    // struct of floats, doubles or long doubles (S266, S327).
    enum class expr_complex_storage
    {
        values       = 1,   // two math::values, re and im
        floats       = 2,   // a math::complex<float>
        doubles      = 3,   // a math::complex<double>
        long_doubles = 4    // a math::complex<long double>
    };

    // 1.1.3
    // expr_complex_box
    //   struct: a complex number's storage: its holders, how it holds its
    // parts, and the parts, in the members its storage names, the others 0.
    // A box is made with one holder, and never changes after.
    struct expr_complex_box
    {
        std::size_t                holders;
        expr_complex_storage       storage;
        value                      re;
        value                      im;
        math::complex<float>       as_float;
        math::complex<double>      as_double;
        math::complex<long double> as_long_double;

        explicit expr_complex_box(
            expr_complex_storage _storage
        )
            : holders(1),
              storage(_storage),
              re(),
              im(),
              as_float(),
              as_double(),
              as_long_double()
        {}
    };

    // 1.1.4
    // Reading a value
    //   functions: whether a C value is a complex number, by its kind's rank,
    // as value.h's kinds know their values; a complex number's box; and a
    // number's two parts as values, a dense part as a value of its type and a
    // real number's imaginary part an exact 0, with whether it is complex.

    inline bool
    expr_complex_held(
        const d_math_value& _x
    )
    {
        return ( (_x.kind == D_MATH_VALUE_BOXED) &&
                 (_x.type != D_NULLPTR) &&
                 (_x.type->rank == expr_complex_rank) );
    }

    inline const expr_complex_box&
    expr_complex_box_of(
        const d_math_value& _x
    )
    {
        return *static_cast<const expr_complex_box*>(_x.box);
    }

    // expr_complex_parts
    //   struct: a number's parts, as an operation reads them.
    struct expr_complex_parts
    {
        value re;
        value im;
        bool  complex;
    };

    inline expr_complex_parts
    expr_complex_parts_of(
        const d_math_value& _x
    )
    {
        expr_complex_parts parts = { value(_x), value(0), false };

        // a real number: itself, and an exact 0
        if (!expr_complex_held(_x))
        {
            return parts;
        }

        const expr_complex_box& box = expr_complex_box_of(_x);

        parts.complex = true;

        // by storage
        switch (box.storage)
        {
            case expr_complex_storage::floats:
                parts.re = value(box.as_float.real());
                parts.im = value(box.as_float.imag());
                break;

            case expr_complex_storage::doubles:
                parts.re = value(box.as_double.real());
                parts.im = value(box.as_double.imag());
                break;

            case expr_complex_storage::long_doubles:
                parts.re = value(box.as_long_double.real());
                parts.im = value(box.as_long_double.imag());
                break;

            case expr_complex_storage::values:
            default:
                parts.re = box.re;
                parts.im = box.im;
                break;
        }

        return parts;
    }

    // expr_complex_type_of
    //   function: the kind an operation with _x among its operands computes
    // in, as far as _x asks: none for an exact number; a float, a double or
    // a long double for a number of one of those or of such parts; and a
    // boxed kind's for any other inexact number, which computes on its
    // parts as values. The widest asked of the operands is the operation's,
    // by these kinds' order.
    inline d_math_value_kind
    expr_complex_type_of(
        const d_math_value& _x
    )
    {
        // a complex number: its storage's type
        if (expr_complex_held(_x))
        {
            switch (expr_complex_box_of(_x).storage)
            {
                case expr_complex_storage::floats:
                    return D_MATH_VALUE_FLOAT;

                case expr_complex_storage::doubles:
                    return D_MATH_VALUE_DOUBLE;

                case expr_complex_storage::long_doubles:
                    return D_MATH_VALUE_LONG_DOUBLE;

                case expr_complex_storage::values:
                default:
                    break;
            }
        }
        else if ( (_x.kind == D_MATH_VALUE_FLOAT)  ||
                  (_x.kind == D_MATH_VALUE_DOUBLE) ||
                  (_x.kind == D_MATH_VALUE_LONG_DOUBLE) )
        {
            return _x.kind;
        }

        return ( (d_math_value_is_exact(_x)) ? D_MATH_VALUE_NONE
                                             : D_MATH_VALUE_BOXED );
    }

    // expr_complex_widest
    //   function: the wider of two kinds expr_complex_type_of gives, in the
    // order none, float, double, long double, boxed.
    inline d_math_value_kind
    expr_complex_widest(
        d_math_value_kind _a,
        d_math_value_kind _b
    )
    {
        // a boxed kind is past every inline one
        if ( (_a == D_MATH_VALUE_BOXED) ||
             (_b == D_MATH_VALUE_BOXED) )
        {
            return D_MATH_VALUE_BOXED;
        }

        return ( (_a > _b) ? _a : _b );
    }

    // expr_complex_flagged
    //   function: _x with _flags added to its own.
    inline value
    expr_complex_flagged(
        const value& _x,
        unsigned     _flags
    )
    {
        d_math_value held = value(_x).detach();

        held.flags |= _flags;

        return value::adopt(held);
    }

NS_END  // internal

// 1.2    Making a complex number
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
    // expr_complex_kind
    //   function: the kind's table, section 1.4's; declared here, for the
    // values made before it.
    inline const d_math_boxed_kind*
    expr_complex_kind();

    // 1.2.2
    // expr_complex_adopt, expr_complex_dense
    //   functions: a value of the kind holding _box, with _flags, the box's
    // one holder becoming the value's; and a value of the kind of the dense
    // parts _z, with _flags. Storage that cannot be had is
    // D_MATH_VALUE_ERROR_MEMORY, as value.h asks, and nothing throws.
    inline value
    expr_complex_adopt(
        expr_complex_box* _box,
        unsigned          _flags
    )
    {
        d_math_value held = d_internal_math_value_blank(D_MATH_VALUE_BOXED,
                                                        _flags);

        held.type = expr_complex_kind();
        held.box  = _box;

        return value::adopt(held);
    }

    inline void
    expr_complex_store(
        expr_complex_box&           _box,
        const math::complex<float>& _z
    )
    {
        _box.as_float = _z;

        return;
    }

    inline void
    expr_complex_store(
        expr_complex_box&            _box,
        const math::complex<double>& _z
    )
    {
        _box.as_double = _z;

        return;
    }

    inline void
    expr_complex_store(
        expr_complex_box&                 _box,
        const math::complex<long double>& _z
    )
    {
        _box.as_long_double = _z;

        return;
    }

    // expr_complex_storage_for
    //   function: the dense storage of parts of Type.
    template<typename Type>
    expr_complex_storage
    expr_complex_storage_for();

    template<>
    inline expr_complex_storage
    expr_complex_storage_for<float>()
    {
        return expr_complex_storage::floats;
    }

    template<>
    inline expr_complex_storage
    expr_complex_storage_for<double>()
    {
        return expr_complex_storage::doubles;
    }

    template<>
    inline expr_complex_storage
    expr_complex_storage_for<long double>()
    {
        return expr_complex_storage::long_doubles;
    }

    template<typename Type>
    value
    expr_complex_dense(
        const math::complex<Type>& _z,
        unsigned                   _flags
    )
    {
        expr_complex_box* box =
            new (std::nothrow) expr_complex_box(
                expr_complex_storage_for<Type>());

        // no storage
        if (box == D_NULLPTR)
        {
            return value(D_MATH_VALUE_ERROR_MEMORY);
        }

        expr_complex_store(*box, _z);

        return expr_complex_adopt(box, _flags);
    }

    // expr_complex_nearest
    //   function: the dense number of Type nearest the parts _re and _im,
    // each its nearest Type, with _flags.
    template<typename Type>
    value
    expr_complex_nearest(
        const value& _re,
        const value& _im,
        unsigned     _flags
    )
    {
        Type re = Type(0);
        Type im = Type(0);

        static_cast<void>(_re.to(re));
        static_cast<void>(_im.to(im));

        return expr_complex_dense(math::complex<Type>(re, im), _flags);
    }

    // 1.2.3
    // expr_complex_made
    //   function: the complex number of the parts _re and _im, two real
    // numbers, with _flags and theirs, as S266 makes it: a part's error, the
    // first; the real part itself where both are exact and the imaginary is
    // 0; parts kept as values where both are exact, or where either is of a
    // boxed kind that is not; and otherwise dense, in the wider of their
    // floating-point types, each its nearest there.
    inline value
    expr_complex_made(
        const value& _re,
        const value& _im,
        unsigned     _flags
    )
    {
        const unsigned flags = (_flags | _re.flags() | _im.flags());

        // a part's error, the first
        if (_re.kind() == D_MATH_VALUE_ERROR)
        {
            return expr_complex_flagged(_re, flags);
        }

        if (_im.kind() == D_MATH_VALUE_ERROR)
        {
            return expr_complex_flagged(_im, flags);
        }

        const d_math_value_kind type =
            expr_complex_widest(expr_complex_type_of(_re.c_value()),
                                expr_complex_type_of(_im.c_value()));

        // exact, with no imaginary part: the real part
        if ( (type == D_MATH_VALUE_NONE) &&
             (d_math_value_cmp(_im.c_value(),
                               d_math_value_from_int(0)) ==
                  D_MATH_VALUE_EQUAL) )
        {
            return expr_complex_flagged(_re, flags);
        }

        // dense, in the wider floating-point type
        if (type == D_MATH_VALUE_FLOAT)
        {
            return expr_complex_nearest<float>(_re, _im, flags);
        }

        if (type == D_MATH_VALUE_DOUBLE)
        {
            return expr_complex_nearest<double>(_re, _im, flags);
        }

        if (type == D_MATH_VALUE_LONG_DOUBLE)
        {
            return expr_complex_nearest<long double>(_re, _im, flags);
        }

        // values: exact parts, or a boxed kind's
        expr_complex_box* box =
            new (std::nothrow) expr_complex_box(expr_complex_storage::values);

        if (box == D_NULLPTR)
        {
            return value(D_MATH_VALUE_ERROR_MEMORY);
        }

        box->re = _re;
        box->im = _im;

        return expr_complex_adopt(box, flags);
    }

NS_END  // internal

// 1.3    Its entries
//------------------------------------------------------------------------------
// value.h's d_math_boxed_kind entries, each given values as that header says:
// one operand a complex number, the other a number of a rank no higher,
// neither an error, a boolean or none.
NS_INTERNAL

    // 1.3.1
    // expr_complex_retain, expr_complex_release
    //   functions: one holder more of _box, and one fewer, by value.h's
    // count (S134); the last release gives the box back.
    inline void
    expr_complex_retain(
        void* _box
    )
    {
        d_math_boxed_hold(&static_cast<expr_complex_box*>(_box)->holders);

        return;
    }

    inline void
    expr_complex_release(
        void* _box
    )
    {
        expr_complex_box* box = static_cast<expr_complex_box*>(_box);

        // the last holder: the box given back
        if (d_math_boxed_unhold(&box->holders))
        {
            delete box;
        }

        return;
    }

    // 1.3.2
    // expr_complex_unary
    //   function: -_x, both parts negated, and |_x|, the modulus, as the
    // banner says.
    template<typename Type>
    value
    expr_complex_dense_unary(
        d_math_value_op            _op,
        const math::complex<Type>& _z,
        unsigned                   _flags
    )
    {
        // the modulus: cplx.h's abs_c, in Type
        if (_op == D_MATH_VALUE_OP_ABS)
        {
            return expr_complex_flagged(value(abs(_z)), _flags);
        }

        return expr_complex_dense(-_z, _flags);
    }

    inline value
    expr_complex_modulus(
        const value& _re,
        const value& _im
    )
    {
        // the root of the sum of the squares: exact where it is a square
        const value sum  = ( (_re * _re) + (_im * _im) );
        const value root = value_root_of(sum);

        return expr_complex_flagged(root, sum.flags());
    }

    inline d_math_value
    expr_complex_unary(
        d_math_value_op _op,
        d_math_value    _x
    )
    {
        const expr_complex_box& box   = expr_complex_box_of(_x);
        const unsigned          flags = _x.flags;

        // by storage: dense parts, in their type
        switch (box.storage)
        {
            case expr_complex_storage::floats:
                return expr_complex_dense_unary(_op,
                                                box.as_float,
                                                flags).detach();

            case expr_complex_storage::doubles:
                return expr_complex_dense_unary(_op,
                                                box.as_double,
                                                flags).detach();

            case expr_complex_storage::long_doubles:
                return expr_complex_dense_unary(_op,
                                                box.as_long_double,
                                                flags).detach();

            case expr_complex_storage::values:
            default:
                break;
        }

        // parts as values: the modulus, or both negated
        if (_op == D_MATH_VALUE_OP_ABS)
        {
            return expr_complex_flagged(expr_complex_modulus(box.re,
                                                             box.im),
                                        flags).detach();
        }

        return expr_complex_made(-box.re, -box.im, flags).detach();
    }

    // 1.3.3
    // expr_complex_binary
    //   function: _a _op _b, one of them a complex number and the other a
    // number, as the banner says: exactly where both are exact; in the
    // widest floating-point type of the two, by cplx.h, where that is float,
    // double or long double; and on the parts as values where it is a boxed
    // kind's. A real operand is taken as itself.
    template<typename Type>
    value
    expr_complex_dense_binary(
        d_math_value_op           _op,
        const expr_complex_parts& _a,
        const expr_complex_parts& _b,
        unsigned                  _flags
    )
    {
        math::complex<Type> z;
        math::complex<Type> w;
        Type                x = Type(0);
        math::complex<Type> r;

        // each operand its nearest in Type: a complex one's parts, a real
        // one itself
        if (_a.complex)
        {
            Type re = Type(0);
            Type im = Type(0);

            static_cast<void>(_a.re.to(re));
            static_cast<void>(_a.im.to(im));
            z = math::complex<Type>(re, im);
        }
        else
        {
            static_cast<void>(_a.re.to(x));
        }

        if (_b.complex)
        {
            Type re = Type(0);
            Type im = Type(0);

            static_cast<void>(_b.re.to(re));
            static_cast<void>(_b.im.to(im));
            w = math::complex<Type>(re, im);
        }
        else
        {
            static_cast<void>(_b.re.to(x));
        }

        // by operation, and which operand is real
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
                r = ( (!_a.complex) ? (x + w)
                    : (!_b.complex) ? (z + x)
                                    : (z + w) );
                break;

            case D_MATH_VALUE_OP_SUB:
                r = ( (!_a.complex) ? (x - w)
                    : (!_b.complex) ? (z - x)
                                    : (z - w) );
                break;

            case D_MATH_VALUE_OP_MUL:
                r = ( (!_a.complex) ? (x * w)
                    : (!_b.complex) ? (z * x)
                                    : (z * w) );
                break;

            case D_MATH_VALUE_OP_DIV:
            default:
                r = ( (!_a.complex) ? (x / w)
                    : (!_b.complex) ? (z / x)
                                    : (z / w) );
                break;
        }

        return expr_complex_dense(r, _flags);
    }

    inline value
    expr_complex_value_binary(
        d_math_value_op           _op,
        const expr_complex_parts& _a,
        const expr_complex_parts& _b,
        unsigned                  _flags
    )
    {
        const value& a = _a.re;
        const value& b = _a.im;
        const value& c = _b.re;
        const value& d = _b.im;

        // by operation, and which operand is real, which keeps its part
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
                return ( (!_a.complex) ? expr_complex_made(a + c, d, _flags)
                       : (!_b.complex) ? expr_complex_made(a + c, b, _flags)
                       : expr_complex_made(a + c, b + d, _flags) );

            case D_MATH_VALUE_OP_SUB:
                return ( (!_a.complex) ? expr_complex_made(a - c, -d, _flags)
                       : (!_b.complex) ? expr_complex_made(a - c, b, _flags)
                       : expr_complex_made(a - c, b - d, _flags) );

            case D_MATH_VALUE_OP_MUL:
                return ( (!_a.complex) ? expr_complex_made(a * c,
                                                           a * d,
                                                           _flags)
                       : (!_b.complex) ? expr_complex_made(a * c,
                                                           b * c,
                                                           _flags)
                       : expr_complex_made( (a * c) - (b * d),
                                            (a * d) + (b * c),
                                            _flags) );

            case D_MATH_VALUE_OP_DIV:
            default:
                break;
        }

        // a quotient by a real number: each part by it; an exact one by an
        // exact 0 a pole, as the dividend is no 0
        if (!_b.complex)
        {
            if ( (_a.re.is_exact()) &&
                 (_a.im.is_exact()) &&
                 (c.is_exact()) &&
                 (d_math_value_cmp(c.c_value(),
                                   d_math_value_from_int(0)) ==
                      D_MATH_VALUE_EQUAL) )
            {
                return expr_complex_flagged(value(D_MATH_VALUE_ERROR_POLE),
                                            _flags);
            }

            return expr_complex_made(a / c, b / c, _flags);
        }

        // by a complex number: over the sum of its parts' squares, no 0
        const value s = ( (c * c) + (d * d) );

        if (!_a.complex)
        {
            return expr_complex_made( (a * c) / s,
                                      (-(a * d)) / s,
                                      _flags);
        }

        return expr_complex_made( ( (a * c) + (b * d) ) / s,
                                  ( (b * c) - (a * d) ) / s,
                                  _flags);
    }

    inline d_math_value
    expr_complex_binary(
        d_math_value_op _op,
        d_math_value    _a,
        d_math_value    _b
    )
    {
        const unsigned flags = (_a.flags | _b.flags);

        // an operand that is no number: outside the domain
        if ( (!d_math_value_is_number(_a)) ||
             (!d_math_value_is_number(_b)) )
        {
            return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const expr_complex_parts a = expr_complex_parts_of(_a);
        const expr_complex_parts b = expr_complex_parts_of(_b);
        const d_math_value_kind  type =
            expr_complex_widest(expr_complex_type_of(_a),
                                expr_complex_type_of(_b));

        // by the type the operation computes in
        switch (type)
        {
            case D_MATH_VALUE_FLOAT:
                return expr_complex_dense_binary<float>(_op,
                                                        a,
                                                        b,
                                                        flags).detach();

            case D_MATH_VALUE_DOUBLE:
                return expr_complex_dense_binary<double>(_op,
                                                         a,
                                                         b,
                                                         flags).detach();

            case D_MATH_VALUE_LONG_DOUBLE:
                return expr_complex_dense_binary<long double>(_op,
                                                              a,
                                                              b,
                                                              flags)
                    .detach();

            default:
                break;
        }

        return expr_complex_value_binary(_op, a, b, flags).detach();
    }

    // 1.3.4
    // expr_complex_cmp
    //   function: how _a stands to _b, one of them a complex number: equal
    // where each part of one is equal to the other's, a real number's
    // imaginary part being 0, by value.h's exact order; unordered otherwise
    // (S329).
    inline d_math_value_order
    expr_complex_cmp(
        d_math_value _a,
        d_math_value _b
    )
    {
        // an operand that is no number: no order
        if ( (!d_math_value_is_number(_a)) ||
             (!d_math_value_is_number(_b)) )
        {
            return D_MATH_VALUE_UNORDERED;
        }

        const expr_complex_parts a = expr_complex_parts_of(_a);
        const expr_complex_parts b = expr_complex_parts_of(_b);

        // part by part
        if ( (d_math_value_cmp(a.re.c_value(),
                               b.re.c_value()) == D_MATH_VALUE_EQUAL) &&
             (d_math_value_cmp(a.im.c_value(),
                               b.im.c_value()) == D_MATH_VALUE_EQUAL) )
        {
            return D_MATH_VALUE_EQUAL;
        }

        return D_MATH_VALUE_UNORDERED;
    }

    // 1.3.5
    // expr_complex_convert
    //   function: a complex number as an inline number of the kind _to: a
    // float, a double or a long double, its real part's nearest, where its
    // imaginary part is 0, with the rounding's direction found exactly;
    // false otherwise, and for an integer or a rational, as no exact complex
    // number is real; *_out as it was where false.
    inline bool
    expr_complex_convert(
        d_math_value*     _out,
        int*              _direction,
        d_math_value      _x,
        d_math_value_kind _to
    )
    {
        const expr_complex_parts parts = expr_complex_parts_of(_x);
        float                    f     = 0.0f;
        double                   d     = 0.0;
        long double              ld    = 0.0L;
        value                    nearest;

        // an imaginary part, or a NaN for one: no real number
        if (d_math_value_cmp(parts.im.c_value(),
                             d_math_value_from_int(0)) != D_MATH_VALUE_EQUAL)
        {
            return false;
        }

        // the real part's nearest, by kind
        if (_to == D_MATH_VALUE_FLOAT)
        {
            if (!parts.re.to(f))
            {
                return false;
            }

            nearest = value(f);
        }
        else if (_to == D_MATH_VALUE_DOUBLE)
        {
            if (!parts.re.to(d))
            {
                return false;
            }

            nearest = value(d);
        }
        else if (_to == D_MATH_VALUE_LONG_DOUBLE)
        {
            if (!parts.re.to(ld))
            {
                return false;
            }

            nearest = value(ld);
        }
        else
        {
            return false;
        }

        // the direction, by the exact order; 0 for a NaN
        const d_math_value_order order =
            d_math_value_cmp(nearest.c_value(), parts.re.c_value());

        *_direction = ( (order == D_MATH_VALUE_LESS)    ? -1
                      : (order == D_MATH_VALUE_GREATER) ? 1
                                                        : 0 );
        *_out       = nearest.detach();

        return true;
    }

    // 1.3.6
    // expr_complex_text
    //   function: a complex number as the value layer's text, as the banner
    // says. Into _buffer, at most _size characters, the closing NUL among
    // them; the whole text's length returned, as value.h asks.

    // expr_complex_floating_text
    //   function: a float, a double or a long double in max_digits10
    // significant digits, in the classic locale; nan, inf and -inf by name.
    template<typename Type>
    std::string
    expr_complex_floating_text(
        Type _x
    )
    {
        std::ostringstream out;

        // what has no digits
        if (std::isnan(_x))
        {
            return "nan";
        }

        if (std::isinf(_x))
        {
            return ( (_x < Type(0)) ? "-inf" : "inf" );
        }

        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<Type>::max_digits10)
            << _x;

        return out.str();
    }

    // expr_complex_part_text
    //   function: a part as text: an exact one's integer or fraction, a
    // boxed one's its kind's text, a floating-point one by
    // expr_complex_floating_text.
    inline std::string
    expr_complex_part_text(
        const value& _x
    )
    {
        std::ostringstream out;
        d_rational         exact = { 0, 1 };
        float              f     = 0.0f;
        double             d     = 0.0;
        long double        ld    = 0.0L;

        out.imbue(std::locale::classic());

        // an exact number of the family: its integer or its fraction
        if (_x.to(exact))
        {
            out << exact.num;

            if (exact.den != 1)
            {
                out << '/' << exact.den;
            }

            return out.str();
        }

        // a boxed number: its kind's text
        if (_x.kind() == D_MATH_VALUE_BOXED)
        {
            const d_math_value& held   = _x.c_value();
            const std::size_t   length = held.type->text(D_NULLPTR,
                                                         0,
                                                         held);
            std::string         text(length + 1, '\0');

            static_cast<void>(held.type->text(&text[0],
                                              length + 1,
                                              held));
            text.resize(length);

            return text;
        }

        // a float, a double or a long double, by kind
        if (_x.kind() == D_MATH_VALUE_FLOAT)
        {
            static_cast<void>(_x.to(f));

            return expr_complex_floating_text(f);
        }

        if (_x.kind() == D_MATH_VALUE_DOUBLE)
        {
            static_cast<void>(_x.to(d));

            return expr_complex_floating_text(d);
        }

        static_cast<void>(_x.to(ld));

        return expr_complex_floating_text(ld);
    }

    inline std::string
    expr_complex_string(
        const d_math_value& _x
    )
    {
        const expr_complex_parts parts = expr_complex_parts_of(_x);
        const std::string        re    = expr_complex_part_text(parts.re);
        std::string              im    = expr_complex_part_text(parts.im);
        std::string              sign  = " + ";

        // the imaginary part's sign before it, its magnitude after
        if ( (!im.empty()) &&
             (im[0] == '-') )
        {
            sign = " - ";
            im   = im.substr(1);
        }

        // a name multiplies im; digits stand before it
        return re + sign + im + ( ( (im == "nan") ||
                                    (im == "inf") ) ? "*im" : "im" );
    }

    inline std::size_t
    expr_complex_text(
        char*        _buffer,
        std::size_t  _size,
        d_math_value _x
    )
    {
        const std::string text = expr_complex_string(_x);

        // as much as _size holds, the closing NUL among it
        if (_size > 0)
        {
            const std::size_t kept = ( (text.size() < _size)
                                           ? text.size()
                                           : (_size - 1) );

            static_cast<void>(text.copy(_buffer, kept));
            _buffer[kept] = '\0';
        }

        return text.size();
    }

    // 1.3.7
    // expr_complex_exact
    //   function: whether a complex number is exact: where it holds its
    // parts as values, each exact (S325).
    inline bool
    expr_complex_exact(
        d_math_value _x
    )
    {
        const expr_complex_box& box = expr_complex_box_of(_x);

        return ( (box.storage == expr_complex_storage::values) &&
                 (box.re.is_exact()) &&
                 (box.im.is_exact()) );
    }

NS_END  // internal

// 1.4    Its table
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.4.1
    // expr_complex_kind
    //   function: the kind's table: its name, "complex", its rank, a number
    // but not exact in every value, and its entries, its exact entry
    // telling which values are; one for the program, as an inline
    // function's static is.
    inline const d_math_boxed_kind*
    expr_complex_kind()
    {
        static const d_math_boxed_kind table = {
            "complex",
            expr_complex_rank,
            D_MATH_BOXED_NUMBER,
            &expr_complex_retain,
            &expr_complex_release,
            &expr_complex_unary,
            &expr_complex_binary,
            &expr_complex_cmp,
            &expr_complex_convert,
            &expr_complex_text,
            &expr_complex_exact
        };

        return &table;
    }

NS_END  // internal


//==============================================================================
// 2.  COMPLEX VALUES
//==============================================================================


// 2.1    Making and reading
//------------------------------------------------------------------------------
// 2.1.1
// expr_is_complex
//   function: whether a value is a complex number: a value of the kind, by
// its rank.
D_NODISCARD inline bool
expr_is_complex(
    const value& _value
)
{
    return internal::expr_complex_held(_value.c_value());
}

NS_INTERNAL

    // expr_complex_real_number
    //   function: whether _x is a real number: a number of no complex kind,
    // which also no matrix is, being no number.
    inline bool
    expr_complex_real_number(
        const value& _x
    )
    {
        return ( (_x.is_number()) &&
                 (!expr_is_complex(_x)) );
    }

    // expr_complex_failed
    //   function: whether an argument of _count at _x is an error, which
    // passes, the first, or no real number, a domain error; and if so that,
    // in _out.
    inline bool
    expr_complex_failed(
        const value* _x,
        std::size_t  _count,
        value&       _out
    )
    {
        // an error passes, the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (_x[i].kind() == D_MATH_VALUE_ERROR)
            {
                _out = _x[i];

                return true;
            }
        }

        // what is no real number has no part here
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (!expr_complex_real_number(_x[i]))
            {
                _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                return true;
            }
        }

        return false;
    }

NS_END  // internal

// 2.1.2
// expr_complex
/**
 * @brief The complex number of two real parts, the language's complex(x, y),
 *        which makes the numbers a sum cannot, a part of -0 or an infinity,
 *        as C11's CMPLX does (S271).
 *
 * @param[in] _re  the real part, a real number.
 * @param[in] _im  the imaginary part, a real number.
 * @return an argument's error, the first; a domain error where a part is no
 *         real number, a boolean, none, a matrix or a complex number; the
 *         real part itself where both are exact and _im is 0; otherwise the
 *         complex number, its parts exact where both are, and else each its
 *         nearest in the wider of their floating-point types (S266),
 *         carrying both parts' flags.
 */
D_NODISCARD inline value
expr_complex(
    const value& _re,
    const value& _im
)
{
    const value parts[2] = { _re, _im };
    value       failed;

    // an error, or no real part
    if (internal::expr_complex_failed(parts, 2, failed))
    {
        return failed;
    }

    return internal::expr_complex_made(_re, _im, 0u);
}

// 2.1.3
// expr_complex_of, expr_complex_to
/**
 * @brief The value of a math::complex<T>: a complex number of the kind, its
 *        parts dense in T, as given, never made real.
 *
 * @param[in] _z  the complex number, of float, double or long double parts.
 * @return the value; D_MATH_VALUE_ERROR_MEMORY where storage is not had.
 */
template<typename Type>
D_NODISCARD value
expr_complex_of(
    const math::complex<Type>& _z
)
{
    return internal::expr_complex_dense(_z, 0u);
}

/**
 * @brief A number's nearest math::complex<T>.
 *
 * @param[in]  _value  the value.
 * @param[out] _out    the nearest complex number of T's parts: a complex
 *                     number's parts each its nearest T, and a real number
 *                     with an imaginary part of +0; as it was where the
 *                     value is no number.
 * @return whether the value is a number.
 */
template<typename Type>
bool
expr_complex_to(
    const value&         _value,
    math::complex<Type>& _out
)
{
    // no number
    if (!_value.is_number())
    {
        return false;
    }

    const internal::expr_complex_parts parts =
        internal::expr_complex_parts_of(_value.c_value());
    Type re = Type(0);
    Type im = Type(0);

    static_cast<void>(parts.re.to(re));
    static_cast<void>(parts.im.to(im));
    _out = math::complex<Type>(re, im);

    return true;
}

// 2.2    The parts
//------------------------------------------------------------------------------
// The language's real, imag, conj and arg of a number (function.hpp): an
// error passes, and what is no number, a boolean, none or a matrix, is a
// domain error; every result carries the argument's flags. The language's
// of a matrix are each element's, by matrix.hpp's expr_matrix_each (S350).
NS_INTERNAL

    // expr_complex_number_failed
    //   function: whether _x is an error, which passes, or no number, a
    // domain error; and if so that, in _out.
    inline bool
    expr_complex_number_failed(
        const value& _x,
        value&       _out
    )
    {
        // an error passes
        if (_x.kind() == D_MATH_VALUE_ERROR)
        {
            _out = _x;

            return true;
        }

        // no number
        if (!_x.is_number())
        {
            _out = value(D_MATH_VALUE_ERROR_DOMAIN);

            return true;
        }

        return false;
    }

NS_END  // internal

// 2.2.1
// expr_real, expr_imag
/**
 * @brief A number's real part, and its imaginary part.
 *
 * @param[in] _value  the number.
 * @return a complex number's part, as it holds it, a dense part a value of
 *         its type; of a real number, the number itself, and an imaginary
 *         part of 0: an exact 0 for an exact number, a +0 of its type for a
 *         float, a double or a long double, and a long double's +0 for any
 *         other inexact kind; an error, or a domain error, as section 2.2
 *         says.
 */
D_NODISCARD inline value
expr_real(
    const value& _value
)
{
    value failed;

    // an error, or no number
    if (internal::expr_complex_number_failed(_value, failed))
    {
        return failed;
    }

    return internal::expr_complex_flagged(
        internal::expr_complex_parts_of(_value.c_value()).re,
        _value.flags());
}

D_NODISCARD inline value
expr_imag(
    const value& _value
)
{
    value failed;

    // an error, or no number
    if (internal::expr_complex_number_failed(_value, failed))
    {
        return failed;
    }

    // a complex number: its part
    if (expr_is_complex(_value))
    {
        return internal::expr_complex_flagged(
            internal::expr_complex_parts_of(_value.c_value()).im,
            _value.flags());
    }

    // a real number: a 0 of its kind
    switch (_value.kind())
    {
        case D_MATH_VALUE_FLOAT:
            return internal::expr_complex_flagged(value(0.0f),
                                                  _value.flags());

        case D_MATH_VALUE_DOUBLE:
            return internal::expr_complex_flagged(value(0.0),
                                                  _value.flags());

        case D_MATH_VALUE_LONG_DOUBLE:
            return internal::expr_complex_flagged(value(0.0L),
                                                  _value.flags());

        default:
            break;
    }

    return internal::expr_complex_flagged(
        ( (_value.is_exact()) ? value(0) : value(0.0L) ),
        _value.flags());
}

// 2.2.2
// expr_conj
/**
 * @brief A number's conjugate.
 *
 * @param[in] _value  the number.
 * @return a complex number's conjugate, its imaginary part negated, a
 *         dense one's by cplx.h's conj, its sign flipped whatever it holds;
 *         a real number itself; an error, or a domain error, as section 2.2
 *         says.
 */
D_NODISCARD inline value
expr_conj(
    const value& _value
)
{
    value failed;

    // an error, or no number
    if (internal::expr_complex_number_failed(_value, failed))
    {
        return failed;
    }

    // a real number: itself
    if (!expr_is_complex(_value))
    {
        return _value;
    }

    const internal::expr_complex_box& box =
        internal::expr_complex_box_of(_value.c_value());

    // by storage
    switch (box.storage)
    {
        case internal::expr_complex_storage::floats:
            return internal::expr_complex_dense(conj(box.as_float),
                                                _value.flags());

        case internal::expr_complex_storage::doubles:
            return internal::expr_complex_dense(conj(box.as_double),
                                                _value.flags());

        case internal::expr_complex_storage::long_doubles:
            return internal::expr_complex_dense(conj(box.as_long_double),
                                                _value.flags());

        case internal::expr_complex_storage::values:
        default:
            break;
    }

    return internal::expr_complex_made(box.re, -box.im, _value.flags());
}

// 2.2.3
// expr_arg
/**
 * @brief A number's argument, the angle atan2(im, re), in (-pi, pi].
 *
 * @param[in] _value  the number.
 * @return of an exact real number, an exact 0 where it is 0 or more, 0
 *         among them, as C's carg(0) and every system's arg(0) are, and pi in
 *         long double where it is negative; of a float, a double or a long
 *         double x, carg(x + 0i) in its type: +0, or pi where x is negative
 *         or -0, a NaN for a NaN; of a complex number, cplx.h's arg_c of its
 *         parts in their type, exact ones' nearest long doubles, as a
 *         function's value is in the tower's kind (S271), never exact, as
 *         atan2(b, a) is rational only where b is 0 (Lindemann); an error,
 *         or a domain error, as section 2.2 says.
 */
D_NODISCARD inline value
expr_arg(
    const value& _value
)
{
    value                      failed;
    math::complex<long double> z;

    // an error, or no number
    if (internal::expr_complex_number_failed(_value, failed))
    {
        return failed;
    }

    // a real number: carg(x + 0i)
    if (!expr_is_complex(_value))
    {
        switch (_value.kind())
        {
            case D_MATH_VALUE_FLOAT:
            case D_MATH_VALUE_DOUBLE:
            case D_MATH_VALUE_LONG_DOUBLE:
                break;

            default:
                // an exact number: 0, exactly, where it is 0 or more
                if ( (_value.is_exact()) &&
                     (_value.compare(value(0)) != D_MATH_VALUE_LESS) )
                {
                    return internal::expr_complex_flagged(value(0),
                                                          _value.flags());
                }

                break;
        }
    }

    // by the type the parts are in: their own, or long double
    switch (internal::expr_complex_type_of(_value.c_value()))
    {
        case D_MATH_VALUE_FLOAT:
        {
            math::complex<float> in_float;

            static_cast<void>(expr_complex_to(_value, in_float));

            return internal::expr_complex_flagged(value(arg(in_float)),
                                                  _value.flags());
        }

        case D_MATH_VALUE_DOUBLE:
        {
            math::complex<double> in_double;

            static_cast<void>(expr_complex_to(_value, in_double));

            return internal::expr_complex_flagged(value(arg(in_double)),
                                                  _value.flags());
        }

        default:
            break;
    }

    // a long double's, of exact parts or a boxed kind's
    static_cast<void>(expr_complex_to(_value, z));

    return internal::expr_complex_flagged(value(arg(z)), _value.flags());
}

// 2.3    Functions
//------------------------------------------------------------------------------
// A function's principal complex value in the parts' floating-point type, as
// the language's functions of a complex number take it (function.hpp, S271,
// S336): the operation is math::complex<T>'s, so cplx.h's _c form, which a
// value and the face then give with the same bits (S313).
NS_INTERNAL

    // 2.3.1
    // expr_complex_apply_as
    //   function: _op of the _count numbers at _x, at most two, in Type, each
    // its nearest math::complex<Type>, a real one's imaginary part +0; the
    // result dense, never made real, with _flags.
    template<typename Type,
             typename Op>
    value
    expr_complex_apply_as(
        const value* _x,
        std::size_t  _count,
        const Op&    _op,
        unsigned     _flags
    )
    {
        math::complex<Type> z[2];

        // each operand its nearest complex number of Type
        for (std::size_t i = 0; i < _count; ++i)
        {
            static_cast<void>(expr_complex_to(_x[i], z[i]));
        }

        return expr_complex_dense(_op(z), _flags);
    }

NS_END  // internal

// 2.3.2
// expr_complex_apply
/**
 * @brief An operation of numbers, real or complex, as a complex number in the
 *        widest floating-point type of theirs.
 *
 * @param[in] _x      the operands, at most two, each a number.
 * @param[in] _count  their number.
 * @param[in] _op     the operation: a struct whose member template
 *                    operator()(const math::complex<T>* _z) gives it in T,
 *                    its operands at _z.
 * @return the operation in the widest of the operands' floating-point types,
 *         a long double where every operand is exact or of a boxed kind that
 *         is not, each operand its nearest complex number there and a real
 *         one's imaginary part +0; dense, never made real (S266); with every
 *         operand's flags. D_MATH_VALUE_ERROR_MEMORY where storage is not
 *         had.
 */
template<typename Op>
D_NODISCARD value
expr_complex_apply(
    const value* _x,
    std::size_t  _count,
    const Op&    _op
)
{
    d_math_value_kind widest = D_MATH_VALUE_NONE;
    unsigned          flags  = 0u;

    // the widest type the operands ask, and their flags
    for (std::size_t i = 0; i < _count; ++i)
    {
        widest = internal::expr_complex_widest(
                     widest,
                     internal::expr_complex_type_of(_x[i].c_value()));
        flags |= _x[i].flags();
    }

    // that type; a long double for exact operands and a boxed kind's
    if (widest == D_MATH_VALUE_FLOAT)
    {
        return internal::expr_complex_apply_as<float>(_x, _count, _op, flags);
    }

    if (widest == D_MATH_VALUE_DOUBLE)
    {
        return internal::expr_complex_apply_as<double>(_x, _count, _op, flags);
    }

    return internal::expr_complex_apply_as<long double>(_x,
                                                        _count,
                                                        _op,
                                                        flags);
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_COMPLEX_HPP
