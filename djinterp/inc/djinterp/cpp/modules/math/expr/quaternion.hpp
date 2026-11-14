/*******************************************************************************
* djinterp [cpp/math]                                             quaternion.hpp
*
* Quaternions as values of the run-time expression language; roadmap Phase
* 3, step 24 (3f), its second part.
*   THE KIND is a boxed kind of the value layer, as the complex number is
* (complex.hpp, S271), written here, in C++, from C++11. It is one kind,
* "quaternion", of the rank 950, past the complex number's 900, so that in an
* operation of a quaternion and a real or complex number the quaternion's
* table handles it, and below the matrix's 1000 (S275). A quaternion is a
* number, exact or not value by value, as its parts are (S325).
*   ITS PARTS are w + x im + y jm + z km, Hamilton's, im jm = km, of one real
* kind (S266): exact where all four are given exact, each kept as a value,
* so that (1 + 2jm) (3 - km) is 3 - 2im + 6jm - km exactly; and otherwise of
* one floating-point type, the widest of the four, each part its nearest
* there, kept densely as math::quaternion<T> (math/quaternion.hpp), the C
* core's array [x, y, z, w] (quat.h, S276). An exact quaternion whose jm and
* km parts come to 0 is the complex number of its other two, and real where
* its im part is 0 too (S275); an inexact one keeps its four parts, its zeros
* with their signs. A complex operand is taken in with its im the
* quaternion's first unit.
*   ARITHMETIC is the value's own, through the kind's table, with a quaternion
* and a number of any lower rank. Exact operands compute exactly on the
* parts, by value.h's operations: Hamilton's product of sixteen products,
* and a quotient p / q = p inv(q), the quotient on the right (S277), p q*
* over |q|^2, so that a part past the family becomes G2's long double,
* flagged. Otherwise an operation computes in the widest floating-point type
* of its operands, each exact part its nearest there, by quat.h's accurate
* forms through math::quaternion<T>: the product and the quotient each part
* rounded once from Dot2's compensated sums (S358, S359); a sum each part
* rounded once. A real operand is taken as itself, as C99's G.5 takes one
* with a complex number, so that a zero's sign survives: a sum or a
* difference changes w alone, a complex operand's w and x, and a product or
* a quotient by a real one scales each part. An exact division by an exact
* 0 is a pole, as a real one is. -q negates each part. |q| is the norm: of
* exact parts the root of the exact sum of their squares, exact where it is
* the square of a rational of the family, |1 + 2im + 2jm + 4km| being 5, and
* otherwise the long double nearest it; of floating-point parts quat.h's
* scaled norm, the root correctly rounded (S361).
*   COMPARISON is by the parts, in value.h's exact order: a quaternion equals
* a number whose parts are each equal to its own, a real number's or a
* complex number's missing parts being 0; and stands in no order otherwise,
* as no order of the quaternions is a field's, nor with a NaN part. The
* language's <, <=, > and >= of a quaternion are a domain error
* (function.hpp).
*   CONVERSION to a real type is of a number whose im, jm and km parts are 0,
* of either sign: its w part's nearest float, double or long double; no
* exact quaternion converts, as none is real.
*   TEXT is the value layer's: w, then each other part's sign, its magnitude
* and its unit: 1 + 2im - 3jm + 4km, 1/2 + 0im + 0jm - 3/4km; exact parts as
* integers or fractions, floating-point parts in max_digits10 significant
* digits, in the classic locale, and a part that is no finite number by its
* name, nan*jm. The language writes a quaternion by render.hpp, in brackets,
* (1 + 2 * im - 3 * jm + 4 * km), and reads it back as a term of its value.
*   HOLDERS are counted by value.h's d_math_boxed_hold and
* d_math_boxed_unhold, atomically wherever the compiler can (S134, S326): the
* language's constants jm and km are a box each, which every fold that reads
* one holds.
*   THE OPERANDS. expr_quaternion_operand is a number in a floating-point
* type as this arithmetic reads it, its parts and how many it holds, and its
* operators are the arithmetic above, a real or complex operand taken as
* itself, two complex ones as the complex kind computes them; with
* number_traits saying its products do not commute, the generic kernels
* compute a matrix of quaternions with it, each element as this kind makes
* it (matrix.hpp, S269, S387).
*   IN C++, expr_quaternion(w, x, y, z) makes the number of four parts, as the
* language's quat(w, x, y, z) does; expr_is_quaternion asks whether a value
* is one; expr_quaternion_real and expr_quaternion_conj give its real part
* and conjugate, expr_quaternion_inverse its inverse and
* expr_quaternion_sign q / |q|; expr_quaternion_of makes the value of a
* math::quaternion<T>, and expr_quaternion_to gives a number's nearest one;
* expr_quaternion_apply computes exp, log, sqrt or a real power of a
* quaternion in its parts' floating-point type, by quat.h's _c forms, and
* expr_quaternion_root an exact square root where the root is a quaternion
* of rationals. A quaternion's functions that give or take a vector or a
* matrix -- its parts, its rotations and slerp -- are rotation.hpp's.
*   STEPS. Step 24 (3f) comes in parts (S357): the C core's quat.h, grown and
* accurate by default, and the C++ faces, in chunk 44; this header, the kind
* in the language, its functions and the order of its factors (S275, S277),
* in chunk 45; and in chunk 46, matrices of quaternions (matrix.hpp), whose
* elements this header's operands compute, its parts and its rotation moved
* to rotation.hpp, which matrix.hpp's kind comes before (S387, S391).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/quaternion.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE KIND
    --------
    1.  Its storage
         1.  expr_quaternion_rank
         2.  expr_quaternion_storage
         3.  expr_quaternion_box
         4.  Reading a value
    2.  Making a quaternion
         1.  expr_quaternion_kind
         2.  expr_quaternion_adopt, expr_quaternion_dense
         3.  expr_quaternion_made
    3.  Its entries
         1.  expr_quaternion_retain, expr_quaternion_release
         2.  expr_quaternion_unary
         3.  expr_quaternion_binary
         4.  expr_quaternion_cmp
         5.  expr_quaternion_convert
         6.  expr_quaternion_text
         7.  expr_quaternion_exact
    4.  Its table
         1.  expr_quaternion_kind
2.  QUATERNION VALUES
    -----------------
    1.  Making and reading
         1.  expr_is_quaternion
         2.  expr_quaternion
         3.  expr_quaternion_of, expr_quaternion_to
    2.  The parts
         1.  expr_quaternion_real
         2.  expr_quaternion_conj
    3.  The inverse and the sign
         1.  expr_quaternion_inverse
         2.  expr_quaternion_sign
    4.  Functions
         1.  expr_quaternion_fn
         2.  expr_quaternion_apply
         3.  expr_quaternion_root
*/

#ifndef DJINTERP_MATH_EXPR_QUATERNION_HPP
#define DJINTERP_MATH_EXPR_QUATERNION_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cmath>    // std::isnan
#include <cstddef>  // std::size_t
#include <new>      // std::nothrow
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/value.h"  // dj_math_boxed_kind, holders
#include "../math_common.hpp"                  // NS_MATH
#include "../quaternion.hpp"                   // quaternion<T>, quat.h's face
#include "../value.hpp"                        // value, value_root_of
#include "./complex.hpp"                       // expr_complex_*
#include "./expr_common.hpp"                   // expr_result


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE KIND
//==============================================================================


// 1.1    Its storage
//------------------------------------------------------------------------------
// 1.1.1
// expr_quaternion_rank
//   constant: the kind's rank, its place in the tower and its own: past the
// complex number's 900, so that in an operation of a quaternion and a real or
// complex number the quaternion's table handles it; below the matrix's 1000
// (S238), whose table handles a quaternion operand (S275).
D_CONSTEXPR_VAR int expr_quaternion_rank = 950;

static_assert( (expr_quaternion_rank > expr_complex_rank),
               "a quaternion is past the complex numbers in the tower" );

NS_INTERNAL

    // 1.1.2
    // expr_quaternion_storage
    //   enum: how a quaternion holds its parts: as values, all four exact,
    // or of one boxed kind that is not; or dense, as the C core's array of
    // floats, doubles or long doubles (S266, S275).
    enum class expr_quaternion_storage
    {
        values       = 1,   // four math::values, w, x, y and z
        floats       = 2,   // a math::quaternion<float>
        doubles      = 3,   // a math::quaternion<double>
        long_doubles = 4    // a math::quaternion<long double>
    };

    // 1.1.3
    // expr_quaternion_box
    //   struct: a quaternion's storage: its holders, how it holds its parts,
    // and the parts, in the members its storage names, the others 0. A box
    // is made with one holder, and never changes after.
    struct expr_quaternion_box
    {
        std::size_t                   holders;
        expr_quaternion_storage       storage;
        value                         w;
        value                         x;
        value                         y;
        value                         z;
        math::quaternion<float>       as_float;
        math::quaternion<double>      as_double;
        math::quaternion<long double> as_long_double;

        explicit expr_quaternion_box(
            expr_quaternion_storage _storage
        )
            : holders(1),
              storage(_storage),
              w(),
              x(),
              y(),
              z(),
              as_float(),
              as_double(),
              as_long_double()
        {}
    };

    // 1.1.4
    // Reading a value
    //   functions: whether a C value is a quaternion, by its kind's rank, as
    // value.h's kinds know their values; a quaternion's box; and a number's
    // four parts as values, with how many of them it holds: a real number's
    // w alone, the others an exact 0; a complex number's w and x, its im
    // part the quaternion's first unit; and a quaternion's four, a dense
    // part a value of its type.

    inline bool
    expr_quaternion_held(
        const dj_math_value& _x
    )
    {
        return ( (_x.kind == D_MATH_VALUE_BOXED) &&
                 (_x.type != D_NULLPTR) &&
                 (_x.type->rank == expr_quaternion_rank) );
    }

    inline const expr_quaternion_box&
    expr_quaternion_box_of(
        const dj_math_value& _x
    )
    {
        return *static_cast<const expr_quaternion_box*>(_x.box);
    }

    // expr_quaternion_parts_held
    //   struct: a number's parts, as an operation reads them, and how many
    // of them the number holds: 1 for a real number, 2 for a complex one, 4
    // for a quaternion.
    struct expr_quaternion_parts_held
    {
        value       w;
        value       x;
        value       y;
        value       z;
        std::size_t held;
    };

    // expr_quaternion_dense_parts
    //   function: a dense quaternion's parts as values of its type.
    template<typename Type>
    void
    expr_quaternion_dense_parts(
        const math::quaternion<Type>& _q,
        expr_quaternion_parts_held&   _parts
    )
    {
        _parts.w = value(_q.w);
        _parts.x = value(_q.x);
        _parts.y = value(_q.y);
        _parts.z = value(_q.z);

        return;
    }

    inline expr_quaternion_parts_held
    expr_quaternion_parts_of(
        const dj_math_value& _x
    )
    {
        expr_quaternion_parts_held parts = { value(_x), value(0), value(0),
                                             value(0), 1 };

        // a complex number: its two parts, im the first unit
        if (expr_complex_held(_x))
        {
            const expr_complex_parts two = expr_complex_parts_of(_x);

            parts.w    = two.re;
            parts.x    = two.im;
            parts.held = 2;

            return parts;
        }

        // a real number: itself, and exact 0s
        if (!expr_quaternion_held(_x))
        {
            return parts;
        }

        const expr_quaternion_box& box = expr_quaternion_box_of(_x);

        parts.held = 4;

        // by storage
        switch (box.storage)
        {
            case expr_quaternion_storage::floats:
                expr_quaternion_dense_parts(box.as_float, parts);
                break;

            case expr_quaternion_storage::doubles:
                expr_quaternion_dense_parts(box.as_double, parts);
                break;

            case expr_quaternion_storage::long_doubles:
                expr_quaternion_dense_parts(box.as_long_double, parts);
                break;

            case expr_quaternion_storage::values:
            default:
                parts.w = box.w;
                parts.x = box.x;
                parts.y = box.y;
                parts.z = box.z;
                break;
        }

        return parts;
    }

    // expr_quaternion_type_of
    //   function: the kind an operation with _x among its operands computes
    // in, as far as _x asks, as complex.hpp's expr_complex_type_of says: a
    // quaternion's storage's type, a boxed kind's for parts of a boxed kind
    // that is not exact, none for exact ones; any other number's as that
    // function gives it.
    inline dj_math_value_kind
    expr_quaternion_type_of(
        const dj_math_value& _x
    )
    {
        // a quaternion: its storage's type
        if (expr_quaternion_held(_x))
        {
            const expr_quaternion_box& box = expr_quaternion_box_of(_x);

            switch (box.storage)
            {
                case expr_quaternion_storage::floats:
                    return D_MATH_VALUE_FLOAT;

                case expr_quaternion_storage::doubles:
                    return D_MATH_VALUE_DOUBLE;

                case expr_quaternion_storage::long_doubles:
                    return D_MATH_VALUE_LONG_DOUBLE;

                case expr_quaternion_storage::values:
                default:
                    break;
            }

            return ( ( (box.w.is_exact()) &&
                       (box.x.is_exact()) &&
                       (box.y.is_exact()) &&
                       (box.z.is_exact()) ) ? D_MATH_VALUE_NONE
                                            : D_MATH_VALUE_BOXED );
        }

        return expr_complex_type_of(_x);
    }

NS_END  // internal

// 1.2    Making a quaternion
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
    // expr_quaternion_kind
    //   function: the kind's table, section 1.4's; declared here, for the
    // values made before it.
    inline const dj_math_boxed_kind*
    expr_quaternion_kind();

    // 1.2.2
    // expr_quaternion_adopt, expr_quaternion_dense
    //   functions: a value of the kind holding _box, with _flags, the box's
    // one holder becoming the value's; and a value of the kind of the dense
    // parts _q, with _flags. Storage that cannot be had is
    // D_MATH_VALUE_ERROR_MEMORY, as value.h asks, and nothing throws.
    inline value
    expr_quaternion_adopt(
        expr_quaternion_box* _box,
        unsigned             _flags
    )
    {
        dj_math_value held = d_internal_math_value_blank(D_MATH_VALUE_BOXED,
                                                         _flags);

        held.type = expr_quaternion_kind();
        held.box  = _box;

        return value::adopt(held);
    }

    inline void
    expr_quaternion_store(
        expr_quaternion_box&           _box,
        const math::quaternion<float>& _q
    )
    {
        _box.as_float = _q;

        return;
    }

    inline void
    expr_quaternion_store(
        expr_quaternion_box&            _box,
        const math::quaternion<double>& _q
    )
    {
        _box.as_double = _q;

        return;
    }

    inline void
    expr_quaternion_store(
        expr_quaternion_box&                 _box,
        const math::quaternion<long double>& _q
    )
    {
        _box.as_long_double = _q;

        return;
    }

    // expr_quaternion_storage_for
    //   function: the dense storage of parts of Type.
    template<typename Type>
    expr_quaternion_storage
    expr_quaternion_storage_for();

    template<>
    inline expr_quaternion_storage
    expr_quaternion_storage_for<float>()
    {
        return expr_quaternion_storage::floats;
    }

    template<>
    inline expr_quaternion_storage
    expr_quaternion_storage_for<double>()
    {
        return expr_quaternion_storage::doubles;
    }

    template<>
    inline expr_quaternion_storage
    expr_quaternion_storage_for<long double>()
    {
        return expr_quaternion_storage::long_doubles;
    }

    template<typename Type>
    value
    expr_quaternion_dense(
        const math::quaternion<Type>& _q,
        unsigned                      _flags
    )
    {
        expr_quaternion_box* box =
            new (std::nothrow) expr_quaternion_box(
                expr_quaternion_storage_for<Type>());

        // no storage
        if (box == D_NULLPTR)
        {
            return value(D_MATH_VALUE_ERROR_MEMORY);
        }

        expr_quaternion_store(*box, _q);

        return expr_quaternion_adopt(box, _flags);
    }

    // expr_quaternion_nearest
    //   function: the dense quaternion of Type nearest the parts _w, _x, _y
    // and _z, each its nearest Type, with _flags.
    template<typename Type>
    value
    expr_quaternion_nearest(
        const value& _w,
        const value& _x,
        const value& _y,
        const value& _z,
        unsigned     _flags
    )
    {
        Type w = Type(0);
        Type x = Type(0);
        Type y = Type(0);
        Type z = Type(0);

        static_cast<void>(_w.to(w));
        static_cast<void>(_x.to(x));
        static_cast<void>(_y.to(y));
        static_cast<void>(_z.to(z));

        return expr_quaternion_dense(math::quaternion<Type>(x, y, z, w),
                                     _flags);
    }

    // 1.2.3
    // expr_quaternion_made
    //   function: the quaternion of the parts _w, _x, _y and _z, four real
    // numbers, with _flags and theirs, as S266 and S275 make it: a part's
    // error, the first; the complex number of _w and _x, or the real _w,
    // where all four are exact and _y and _z are 0; parts kept as values
    // where all four are exact, or where any is of a boxed kind that is not;
    // and otherwise dense, in the widest of their floating-point types, each
    // its nearest there.
    inline value
    expr_quaternion_made(
        const value& _w,
        const value& _x,
        const value& _y,
        const value& _z,
        unsigned     _flags
    )
    {
        const value* const parts[4] = { &_w, &_x, &_y, &_z };
        unsigned           flags    = _flags;
        dj_math_value_kind type     = D_MATH_VALUE_NONE;

        // a part's error, the first; every part's flags and type
        for (std::size_t i = 0; i < 4; ++i)
        {
            flags |= parts[i]->flags();
            type   = expr_complex_widest(
                         type,
                         expr_complex_type_of(parts[i]->c_value()));
        }

        for (std::size_t i = 0; i < 4; ++i)
        {
            if (parts[i]->kind() == D_MATH_VALUE_ERROR)
            {
                return expr_complex_flagged(*parts[i], flags);
            }
        }

        // exact, with no jm or km part: the complex number, or the real one
        if ( (type == D_MATH_VALUE_NONE) &&
             (d_math_value_cmp(_y.c_value(),
                               d_math_value_from_int(0)) ==
                  D_MATH_VALUE_EQUAL) &&
             (d_math_value_cmp(_z.c_value(),
                               d_math_value_from_int(0)) ==
                  D_MATH_VALUE_EQUAL) )
        {
            return expr_complex_made(_w, _x, flags);
        }

        // dense, in the widest floating-point type
        if (type == D_MATH_VALUE_FLOAT)
        {
            return expr_quaternion_nearest<float>(_w, _x, _y, _z, flags);
        }

        if (type == D_MATH_VALUE_DOUBLE)
        {
            return expr_quaternion_nearest<double>(_w, _x, _y, _z, flags);
        }

        if (type == D_MATH_VALUE_LONG_DOUBLE)
        {
            return expr_quaternion_nearest<long double>(_w,
                                                        _x,
                                                        _y,
                                                        _z,
                                                        flags);
        }

        // values: exact parts, or a boxed kind's
        expr_quaternion_box* box =
            new (std::nothrow) expr_quaternion_box(
                expr_quaternion_storage::values);

        if (box == D_NULLPTR)
        {
            return value(D_MATH_VALUE_ERROR_MEMORY);
        }

        box->w = _w;
        box->x = _x;
        box->y = _y;
        box->z = _z;

        return expr_quaternion_adopt(box, flags);
    }

NS_END  // internal

// 1.3    Its entries
//------------------------------------------------------------------------------
// value.h's dj_math_boxed_kind entries, each given values as that header says:
// one operand a quaternion, the other a number of a rank no higher, neither
// an error, a boolean or none.
NS_INTERNAL

    // 1.3.1
    // expr_quaternion_retain, expr_quaternion_release
    //   functions: one holder more of _box, and one fewer, by value.h's count
    // (S134); the last release gives the box back.
    inline void
    expr_quaternion_retain(
        void* _box
    )
    {
        d_math_boxed_hold(
            &static_cast<expr_quaternion_box*>(_box)->holders);

        return;
    }

    inline void
    expr_quaternion_release(
        void* _box
    )
    {
        expr_quaternion_box* box = static_cast<expr_quaternion_box*>(_box);

        // the last holder: the box given back
        if (d_math_boxed_unhold(&box->holders))
        {
            delete box;
        }

        return;
    }

    // 1.3.2
    // expr_quaternion_unary
    //   function: -_x, each part negated, and |_x|, the norm, as the banner
    // says.
    template<typename Type>
    value
    expr_quaternion_dense_unary(
        dj_math_value_op              _op,
        const math::quaternion<Type>& _q,
        unsigned                      _flags
    )
    {
        // the norm: quat.h's, scaled, the root correctly rounded
        if (_op == D_MATH_VALUE_OP_ABS)
        {
            return expr_complex_flagged(value(norm(_q)), _flags);
        }

        return expr_quaternion_dense(math::quaternion<Type>(-_q.x,
                                                            -_q.y,
                                                            -_q.z,
                                                            -_q.w),
                                     _flags);
    }

    // expr_quaternion_norm_of
    //   function: the norm of four parts as values: the root of the sum of
    // their squares, exact where it is a square.
    inline value
    expr_quaternion_norm_of(
        const value& _w,
        const value& _x,
        const value& _y,
        const value& _z
    )
    {
        const value sum  = ( ( (_w * _w) + (_x * _x) ) +
                             ( (_y * _y) + (_z * _z) ) );
        const value root = value_root_of(sum);

        return expr_complex_flagged(root, sum.flags());
    }

    inline dj_math_value
    expr_quaternion_unary(
        dj_math_value_op _op,
        dj_math_value    _x
    )
    {
        const expr_quaternion_box& box   = expr_quaternion_box_of(_x);
        const unsigned             flags = _x.flags;

        // by storage: dense parts, in their type
        switch (box.storage)
        {
            case expr_quaternion_storage::floats:
                return expr_quaternion_dense_unary(_op,
                                                   box.as_float,
                                                   flags).detach();

            case expr_quaternion_storage::doubles:
                return expr_quaternion_dense_unary(_op,
                                                   box.as_double,
                                                   flags).detach();

            case expr_quaternion_storage::long_doubles:
                return expr_quaternion_dense_unary(_op,
                                                   box.as_long_double,
                                                   flags).detach();

            case expr_quaternion_storage::values:
            default:
                break;
        }

        // parts as values: the norm, or each negated
        if (_op == D_MATH_VALUE_OP_ABS)
        {
            return expr_complex_flagged(expr_quaternion_norm_of(box.w,
                                                                box.x,
                                                                box.y,
                                                                box.z),
                                        flags).detach();
        }

        return expr_quaternion_made(-box.w,
                                    -box.x,
                                    -box.y,
                                    -box.z,
                                    flags).detach();
    }

    // 1.3.3
    // expr_quaternion_binary
    //   function: _a _op _b, one of them a quaternion and the other a number,
    // as the banner says: exactly where all parts are exact; in the widest
    // floating-point type of the two, by quat.h, where that is float, double
    // or long double; and on the parts as values where it is a boxed kind's.
    // A real operand is taken as itself, and a complex one with its im the
    // first unit.

    // expr_quaternion_operand
    //   struct: a dense operand in Type: its parts, as a quaternion, and how
    // many of them it holds (expr_quaternion_parts_held), 1 for a real
    // number, 2 for a complex one and 4 for a quaternion, each part past those
    // +0. Its operators, below, are the kind's arithmetic in Type, and the
    // generic kernels compute a matrix of quaternions' with them, so that each
    // element is as the kind makes it (matrix.hpp, S269, S387).
    template<typename Type>
    struct expr_quaternion_operand
    {
        math::quaternion<Type> q;
        std::size_t            held;
    };

    // expr_quaternion_operand_made
    //   function: the operand of the parts _q holding _held of them, each part
    // past those +0.
    template<typename Type>
    expr_quaternion_operand<Type>
    expr_quaternion_operand_made(
        const math::quaternion<Type>& _q,
        std::size_t                   _held
    )
    {
        const expr_quaternion_operand<Type> operand =
            { math::quaternion<Type>( ( (_held >= 2) ? _q.x : Type(0) ),
                                      ( (_held == 4) ? _q.y : Type(0) ),
                                      ( (_held == 4) ? _q.z : Type(0) ),
                                      _q.w ),
              _held };

        return operand;
    }

    // expr_quaternion_operand_of
    //   function: a number's parts, as an operation reads them, in Type, each
    // its nearest there.
    template<typename Type>
    expr_quaternion_operand<Type>
    expr_quaternion_operand_of(
        const expr_quaternion_parts_held& _parts
    )
    {
        math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));

        static_cast<void>(_parts.w.to(q.w));
        static_cast<void>(_parts.x.to(q.x));
        static_cast<void>(_parts.y.to(q.y));
        static_cast<void>(_parts.z.to(q.z));

        return expr_quaternion_operand_made(q, _parts.held);
    }

    // expr_quaternion_sum_part
    //   function: one part of _a + _b, or of _a - _b where _minus, each part
    // an operand holds taken, the other's missing one not: a real operand
    // holds w alone, a complex one w and x (_index 0 to 3, w, x, y, z).
    template<typename Type>
    Type
    expr_quaternion_sum_part(
        Type        _a,
        Type        _b,
        std::size_t _index,
        std::size_t _held_a,
        std::size_t _held_b,
        bool        _minus
    )
    {
        const bool in_a = (_index < _held_a);
        const bool in_b = (_index < _held_b);

        // both hold it, one alone, or the other alone
        if ( (in_a) &&
             (in_b) )
        {
            return ( _minus ? (_a - _b) : (_a + _b) );
        }

        if (in_a)
        {
            return _a;
        }

        return ( _minus ? -_b : _b );
    }

    // expr_quaternion_scaled
    //   function: each part of _q times _r, or over it where _over.
    template<typename Type>
    math::quaternion<Type>
    expr_quaternion_scaled(
        const math::quaternion<Type>& _q,
        Type                          _r,
        bool                          _over
    )
    {
        // over _r, or times it
        if (_over)
        {
            return math::quaternion<Type>(_q.x / _r,
                                          _q.y / _r,
                                          _q.z / _r,
                                          _q.w / _r);
        }

        return math::quaternion<Type>(_q.x * _r,
                                      _q.y * _r,
                                      _q.z * _r,
                                      _q.w * _r);
    }

    // expr_quaternion_complex_part, expr_quaternion_of_complex
    //   functions: an operand of two parts at most as math::complex<Type>,
    // and the operand of a complex number.
    template<typename Type>
    math::complex<Type>
    expr_quaternion_complex_part(
        const expr_quaternion_operand<Type>& _a
    )
    {
        return math::complex<Type>(_a.q.w, _a.q.x);
    }

    template<typename Type>
    expr_quaternion_operand<Type>
    expr_quaternion_of_complex(
        const math::complex<Type>& _z
    )
    {
        return expr_quaternion_operand_made(
            math::quaternion<Type>(_z.imag(), Type(0), Type(0), _z.real()),
            2);
    }

    // expr_quaternion_operand_sum
    //   function: _a + _b, or _a - _b where _minus, part by part, a part one
    // operand lacks the other's as it is, holding the parts either holds.
    template<typename Type>
    expr_quaternion_operand<Type>
    expr_quaternion_operand_sum(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b,
        bool                                 _minus
    )
    {
        const std::size_t ha = _a.held;
        const std::size_t hb = _b.held;

        return expr_quaternion_operand_made(
            math::quaternion<Type>(
                expr_quaternion_sum_part(_a.q.x, _b.q.x, 1, ha, hb, _minus),
                expr_quaternion_sum_part(_a.q.y, _b.q.y, 2, ha, hb, _minus),
                expr_quaternion_sum_part(_a.q.z, _b.q.z, 3, ha, hb, _minus),
                expr_quaternion_sum_part(_a.q.w, _b.q.w, 0, ha, hb, _minus)),
            ( (ha > hb) ? ha : hb ));
    }

    // operator+, operator-, operator*, operator/ (expr_quaternion_operand)
    //   functions: the kind's arithmetic on two dense operands in Type, each
    // part an operand holds taken as it is (S372): a sum part by part; a
    // product or a quotient by a real operand each part scaled; of two
    // complex numbers, and a real or complex one over a complex one,
    // math::complex's, cplx.h's, as the complex kind computes them; any other
    // Hamilton's product, or the quotient on the right, a b^-1 (S277), by
    // quat.h's Dot2 forms (S358, S359). -a each part it holds negated; +=
    // and -= the sum and the difference; a == b where every part is equal, a
    // -0 to a 0 and a NaN to nothing.
    template<typename Type>
    expr_quaternion_operand<Type>
    operator+(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        return expr_quaternion_operand_sum(_a, _b, false);
    }

    template<typename Type>
    expr_quaternion_operand<Type>
    operator-(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        return expr_quaternion_operand_sum(_a, _b, true);
    }

    template<typename Type>
    expr_quaternion_operand<Type>
    operator-(
        const expr_quaternion_operand<Type>& _a
    )
    {
        return expr_quaternion_operand_made(
            math::quaternion<Type>(-_a.q.x, -_a.q.y, -_a.q.z, -_a.q.w),
            _a.held);
    }

    template<typename Type>
    expr_quaternion_operand<Type>
    operator*(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        // by a real operand: each part scaled
        if (_a.held == 1)
        {
            return expr_quaternion_operand_made(
                expr_quaternion_scaled(_b.q, _a.q.w, false),
                _b.held);
        }

        if (_b.held == 1)
        {
            return expr_quaternion_operand_made(
                expr_quaternion_scaled(_a.q, _b.q.w, false),
                _a.held);
        }

        // two complex numbers: cplx.h's product
        if ( (_a.held == 2) &&
             (_b.held == 2) )
        {
            return expr_quaternion_of_complex(
                expr_quaternion_complex_part(_a) *
                    expr_quaternion_complex_part(_b));
        }

        return expr_quaternion_operand_made(_a.q * _b.q, 4);
    }

    template<typename Type>
    expr_quaternion_operand<Type>
    operator/(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        // by a real operand: each part over it
        if (_b.held == 1)
        {
            return expr_quaternion_operand_made(
                expr_quaternion_scaled(_a.q, _b.q.w, true),
                _a.held);
        }

        // a real or complex number over a complex one: cplx.h's quotient
        if ( (_a.held <= 2) &&
             (_b.held == 2) )
        {
            return expr_quaternion_of_complex(
                ( (_a.held == 1)
                      ? (_a.q.w / expr_quaternion_complex_part(_b))
                      : (expr_quaternion_complex_part(_a) /
                         expr_quaternion_complex_part(_b)) ));
        }

        return expr_quaternion_operand_made(_a.q / _b.q, 4);
    }

    template<typename Type>
    expr_quaternion_operand<Type>&
    operator+=(
        expr_quaternion_operand<Type>&       _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        _a = (_a + _b);

        return _a;
    }

    template<typename Type>
    expr_quaternion_operand<Type>&
    operator-=(
        expr_quaternion_operand<Type>&       _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        _a = (_a - _b);

        return _a;
    }

    template<typename Type>
    bool
    operator==(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        return ( (_a.q.w == _b.q.w) &&
                 (_a.q.x == _b.q.x) &&
                 (_a.q.y == _b.q.y) &&
                 (_a.q.z == _b.q.z) );
    }

    template<typename Type>
    bool
    operator!=(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        return !(_a == _b);
    }

    // expr_quaternion_operand_conj
    //   function: an operand's conjugate, each part it holds past w negated.
    template<typename Type>
    expr_quaternion_operand<Type>
    expr_quaternion_operand_conj(
        const expr_quaternion_operand<Type>& _a
    )
    {
        return expr_quaternion_operand_made(
            math::quaternion<Type>(-_a.q.x, -_a.q.y, -_a.q.z, _a.q.w),
            _a.held);
    }

    // expr_quaternion_left_quotient
    //   function: _a^-1 _b, the quotient on the left, of two dense operands
    // in Type, as solve(a, b) asks (S390): of numbers whose product does not
    // hang on its order, a real a or two complex numbers, b / a, as above;
    // otherwise quat.h's left quotient, by Dot2, each part rounded once
    // (S358).
    template<typename Type>
    expr_quaternion_operand<Type>
    expr_quaternion_left_quotient(
        const expr_quaternion_operand<Type>& _a,
        const expr_quaternion_operand<Type>& _b
    )
    {
        // a real a, or two complex numbers: b / a
        if ( (_a.held == 1) ||
             ( (_a.held <= 2) &&
               (_b.held <= 2) ) )
        {
            return (_b / _a);
        }

        return expr_quaternion_operand_made(left_divide(_a.q, _b.q), 4);
    }

    // expr_quaternion_operand_value
    //   function: the value of an operand, with _flags: of a real one its
    // number, of a complex one the complex number, and of a quaternion the
    // quaternion, dense.
    template<typename Type>
    value
    expr_quaternion_operand_value(
        const expr_quaternion_operand<Type>& _a,
        unsigned                             _flags
    )
    {
        // by the parts it holds
        if (_a.held == 1)
        {
            return expr_complex_flagged(value(_a.q.w),
                                        _flags);
        }

        if (_a.held == 2)
        {
            return expr_complex_flagged(
                expr_complex_of(math::complex<Type>(_a.q.w, _a.q.x)),
                _flags);
        }

        return expr_quaternion_dense(_a.q,
                                     _flags);
    }

    template<typename Type>
    value
    expr_quaternion_dense_binary(
        dj_math_value_op                  _op,
        const expr_quaternion_parts_held& _a,
        const expr_quaternion_parts_held& _b,
        unsigned                          _flags
    )
    {
        const expr_quaternion_operand<Type> a =
            expr_quaternion_operand_of<Type>(_a);
        const expr_quaternion_operand<Type> b =
            expr_quaternion_operand_of<Type>(_b);
        expr_quaternion_operand<Type>       r = a;

        // by operation: the operands' arithmetic
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
                r = (a + b);
                break;

            case D_MATH_VALUE_OP_SUB:
                r = (a - b);
                break;

            case D_MATH_VALUE_OP_MUL:
                r = (a * b);
                break;

            case D_MATH_VALUE_OP_DIV:
            default:
                // a / b = a b^-1, on the right (S277)
                r = (a / b);
                break;
        }

        return expr_quaternion_dense(r.q, _flags);
    }

    // expr_quaternion_value_product
    //   function: Hamilton's product of two operands' parts as values,
    // exactly where they are exact: a real operand scales each part.
    inline value
    expr_quaternion_value_product(
        const expr_quaternion_parts_held& _a,
        const expr_quaternion_parts_held& _b,
        unsigned                          _flags
    )
    {
        // a real operand: each part scaled
        if (_a.held == 1)
        {
            return expr_quaternion_made(_a.w * _b.w,
                                        _a.w * _b.x,
                                        _a.w * _b.y,
                                        _a.w * _b.z,
                                        _flags);
        }

        if (_b.held == 1)
        {
            return expr_quaternion_made(_a.w * _b.w,
                                        _a.x * _b.w,
                                        _a.y * _b.w,
                                        _a.z * _b.w,
                                        _flags);
        }

        // Hamilton's, im jm = km, jm km = im, km im = jm
        return expr_quaternion_made(
            ( (_a.w * _b.w) - (_a.x * _b.x) ) -
                ( (_a.y * _b.y) + (_a.z * _b.z) ),
            ( (_a.w * _b.x) + (_a.x * _b.w) ) +
                ( (_a.y * _b.z) - (_a.z * _b.y) ),
            ( (_a.w * _b.y) - (_a.x * _b.z) ) +
                ( (_a.y * _b.w) + (_a.z * _b.x) ),
            ( (_a.w * _b.z) + (_a.x * _b.y) ) +
                ( (_a.z * _b.w) - (_a.y * _b.x) ),
            _flags);
    }

    // expr_quaternion_value_quotient
    //   function: _a / _b of two operands' parts as values, _a _b^-1, on the
    // right (S277): by a real _b each part over it, an exact 0 a pole; by any
    // other _a _b* over |_b|^2, the sum of its parts' squares.
    inline value
    expr_quaternion_value_quotient(
        const expr_quaternion_parts_held& _a,
        const expr_quaternion_parts_held& _b,
        unsigned                          _flags
    )
    {
        // by a real number: each part over it; an exact 0 a pole
        if (_b.held == 1)
        {
            if ( (_a.w.is_exact()) &&
                 (_a.x.is_exact()) &&
                 (_a.y.is_exact()) &&
                 (_a.z.is_exact()) &&
                 (_b.w.is_exact()) &&
                 (d_math_value_cmp(_b.w.c_value(),
                                   d_math_value_from_int(0)) ==
                      D_MATH_VALUE_EQUAL) )
            {
                return expr_complex_flagged(value(D_MATH_VALUE_ERROR_POLE),
                                            _flags);
            }

            return expr_quaternion_made(_a.w / _b.w,
                                        _a.x / _b.w,
                                        _a.y / _b.w,
                                        _a.z / _b.w,
                                        _flags);
        }

        // by a complex number or a quaternion: _a conj(_b) over |_b|^2
        const value n = ( ( (_b.w * _b.w) + (_b.x * _b.x) ) +
                          ( (_b.y * _b.y) + (_b.z * _b.z) ) );
        const expr_quaternion_parts_held conj = { _b.w, -_b.x, -_b.y, -_b.z,
                                                  4 };
        const value numerator = expr_quaternion_value_product(_a,
                                                              conj,
                                                              _flags);
        const expr_quaternion_parts_held top =
            expr_quaternion_parts_of(numerator.c_value());

        // the numerator's error passes
        if (numerator.kind() == D_MATH_VALUE_ERROR)
        {
            return numerator;
        }

        return expr_quaternion_made(top.w / n,
                                    top.x / n,
                                    top.y / n,
                                    top.z / n,
                                    _flags);
    }

    inline value
    expr_quaternion_value_binary(
        dj_math_value_op                  _op,
        const expr_quaternion_parts_held& _a,
        const expr_quaternion_parts_held& _b,
        unsigned                          _flags
    )
    {
        // by operation: a sum part by part, each operand's missing parts 0
        switch (_op)
        {
            case D_MATH_VALUE_OP_ADD:
                return expr_quaternion_made(_a.w + _b.w,
                                            _a.x + _b.x,
                                            _a.y + _b.y,
                                            _a.z + _b.z,
                                            _flags);

            case D_MATH_VALUE_OP_SUB:
                return expr_quaternion_made(_a.w - _b.w,
                                            _a.x - _b.x,
                                            _a.y - _b.y,
                                            _a.z - _b.z,
                                            _flags);

            case D_MATH_VALUE_OP_MUL:
                return expr_quaternion_value_product(_a, _b, _flags);

            case D_MATH_VALUE_OP_DIV:
            default:
                break;
        }

        return expr_quaternion_value_quotient(_a, _b, _flags);
    }

    inline dj_math_value
    expr_quaternion_binary(
        dj_math_value_op _op,
        dj_math_value    _a,
        dj_math_value    _b
    )
    {
        const unsigned flags = (_a.flags | _b.flags);

        // an operand that is no number: outside the domain
        if ( (!d_math_value_is_number(_a)) ||
             (!d_math_value_is_number(_b)) )
        {
            return d_math_value_from_error(D_MATH_VALUE_ERROR_DOMAIN);
        }

        const expr_quaternion_parts_held a = expr_quaternion_parts_of(_a);
        const expr_quaternion_parts_held b = expr_quaternion_parts_of(_b);
        const dj_math_value_kind         type =
            expr_complex_widest(expr_quaternion_type_of(_a),
                                expr_quaternion_type_of(_b));

        // by the type the operation computes in
        switch (type)
        {
            case D_MATH_VALUE_FLOAT:
                return expr_quaternion_dense_binary<float>(_op,
                                                           a,
                                                           b,
                                                           flags).detach();

            case D_MATH_VALUE_DOUBLE:
                return expr_quaternion_dense_binary<double>(_op,
                                                            a,
                                                            b,
                                                            flags).detach();

            case D_MATH_VALUE_LONG_DOUBLE:
                return expr_quaternion_dense_binary<long double>(_op,
                                                                 a,
                                                                 b,
                                                                 flags)
                    .detach();

            default:
                break;
        }

        return expr_quaternion_value_binary(_op, a, b, flags).detach();
    }

    // 1.3.4
    // expr_quaternion_cmp
    //   function: how _a stands to _b, one of them a quaternion: equal where
    // each part of one is equal to the other's, a missing part being 0, by
    // value.h's exact order; unordered otherwise.
    inline dj_math_value_order
    expr_quaternion_cmp(
        dj_math_value _a,
        dj_math_value _b
    )
    {
        // an operand that is no number: no order
        if ( (!d_math_value_is_number(_a)) ||
             (!d_math_value_is_number(_b)) )
        {
            return D_MATH_VALUE_UNORDERED;
        }

        const expr_quaternion_parts_held a = expr_quaternion_parts_of(_a);
        const expr_quaternion_parts_held b = expr_quaternion_parts_of(_b);

        // part by part
        if ( (d_math_value_cmp(a.w.c_value(),
                               b.w.c_value()) == D_MATH_VALUE_EQUAL) &&
             (d_math_value_cmp(a.x.c_value(),
                               b.x.c_value()) == D_MATH_VALUE_EQUAL) &&
             (d_math_value_cmp(a.y.c_value(),
                               b.y.c_value()) == D_MATH_VALUE_EQUAL) &&
             (d_math_value_cmp(a.z.c_value(),
                               b.z.c_value()) == D_MATH_VALUE_EQUAL) )
        {
            return D_MATH_VALUE_EQUAL;
        }

        return D_MATH_VALUE_UNORDERED;
    }

    // 1.3.5
    // expr_quaternion_convert
    //   function: a quaternion as an inline number of the kind _to: a float,
    // a double or a long double, its w part's nearest, where its im, jm and
    // km parts are 0, with the rounding's direction found exactly; false
    // otherwise, and for an integer or a rational, as no exact quaternion is
    // real; *_out as it was where false.
    inline bool
    expr_quaternion_convert(
        dj_math_value*     _out,
        int*               _direction,
        dj_math_value      _x,
        dj_math_value_kind _to
    )
    {
        const expr_quaternion_parts_held parts = expr_quaternion_parts_of(_x);
        const dj_math_value                zero  = d_math_value_from_int(0);

        // a part past w, or a NaN for one: no real number
        if ( (d_math_value_cmp(parts.x.c_value(), zero) !=
                  D_MATH_VALUE_EQUAL) ||
             (d_math_value_cmp(parts.y.c_value(), zero) !=
                  D_MATH_VALUE_EQUAL) ||
             (d_math_value_cmp(parts.z.c_value(), zero) !=
                  D_MATH_VALUE_EQUAL) )
        {
            return false;
        }

        const value       w = parts.w;
        float             f = 0.0f;
        double            d = 0.0;
        long double       l = 0.0L;
        value             nearest;

        // w's nearest, by kind
        if ( (_to == D_MATH_VALUE_FLOAT) &&
             (w.to(f)) )
        {
            nearest = value(f);
        }
        else if ( (_to == D_MATH_VALUE_DOUBLE) &&
                  (w.to(d)) )
        {
            nearest = value(d);
        }
        else if ( (_to == D_MATH_VALUE_LONG_DOUBLE) &&
                  (w.to(l)) )
        {
            nearest = value(l);
        }
        else
        {
            return false;
        }

        // the direction, by the exact order; 0 for a NaN
        const dj_math_value_order order =
            d_math_value_cmp(nearest.c_value(), w.c_value());

        *_direction = ( (order == D_MATH_VALUE_LESS)    ? -1
                      : (order == D_MATH_VALUE_GREATER) ? 1
                                                        : 0 );
        *_out       = nearest.detach();

        return true;
    }

    // 1.3.6
    // expr_quaternion_text
    //   function: a quaternion as the value layer's text, as the banner says.
    // Into _buffer, at most _size characters, the closing NUL among them;
    // the whole text's length returned, as value.h asks.

    // expr_quaternion_unit_text
    //   function: one part past w as text: its sign, its magnitude and its
    // unit, a name multiplying the unit.
    inline std::string
    expr_quaternion_unit_text(
        const value&       _part,
        const std::string& _unit
    )
    {
        std::string text = expr_complex_part_text(_part);
        std::string sign = " + ";

        // the sign before it, its magnitude after
        if ( (!text.empty()) &&
             (text[0] == '-') )
        {
            sign = " - ";
            text = text.substr(1);
        }

        // a name multiplies the unit; digits stand before it
        return sign + text + ( ( (text == "nan") ||
                                 (text == "inf") ) ? ("*" + _unit)
                                                   : _unit );
    }

    inline std::string
    expr_quaternion_string(
        const dj_math_value& _x
    )
    {
        const expr_quaternion_parts_held parts = expr_quaternion_parts_of(_x);

        return ( expr_complex_part_text(parts.w) +
                 expr_quaternion_unit_text(parts.x, "im") +
                 expr_quaternion_unit_text(parts.y, "jm") +
                 expr_quaternion_unit_text(parts.z, "km") );
    }

    inline std::size_t
    expr_quaternion_text(
        char*         _buffer,
        std::size_t   _size,
        dj_math_value _x
    )
    {
        const std::string text = expr_quaternion_string(_x);

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
    // expr_quaternion_exact
    //   function: whether a quaternion is exact: where it holds its parts as
    // values, each exact (S325).
    inline bool
    expr_quaternion_exact(
        dj_math_value _x
    )
    {
        const expr_quaternion_box& box = expr_quaternion_box_of(_x);

        return ( (box.storage == expr_quaternion_storage::values) &&
                 (box.w.is_exact()) &&
                 (box.x.is_exact()) &&
                 (box.y.is_exact()) &&
                 (box.z.is_exact()) );
    }

NS_END  // internal

// number_traits<internal::expr_quaternion_operand<Type> >
//   trait: a dense operand of the kind's arithmetic, as the generic kernels
// take it (S269, S387): a skew field, inexact, with no order, whose products
// do not commute, so that the kernels keep each in its order and divide on
// the side each row operation asks; its zero and one the quaternions 0 and
// 1, so that a sum from 0 adds each part; its magnitude, of Type, |w| + |x|
// + |y| + |z|, as cmat.h pivots a complex number by |re| + |im|, no larger
// than twice the norm and needing no root.
template<typename Type>
struct number_traits<internal::expr_quaternion_operand<Type> >
{
    typedef Type magnitude_type;

    static const bool is_specialized = true;
    static const bool is_exact       = false;
    static const bool is_integer     = false;
    static const bool is_field       = true;
    static const bool is_commutative = false;
    static const bool is_ordered     = false;

    // the additive and multiplicative identities: quaternions.
    static internal::expr_quaternion_operand<Type>
    zero()
    {
        return internal::expr_quaternion_operand_made(
            math::quaternion<Type>(Type(0), Type(0), Type(0), Type(0)),
            4);
    }

    static internal::expr_quaternion_operand<Type>
    one()
    {
        return internal::expr_quaternion_operand_made(
            math::quaternion<Type>(Type(0), Type(0), Type(0), Type(1)),
            4);
    }

    // what a pivot is chosen by: the sum of the parts' magnitudes.
    static Type
    magnitude(
        const internal::expr_quaternion_operand<Type>& _a
    )
    {
        return ( ( number_traits<Type>::abs(_a.q.w) +
                   number_traits<Type>::abs(_a.q.x) ) +
                 ( number_traits<Type>::abs(_a.q.y) +
                   number_traits<Type>::abs(_a.q.z) ) );
    }
};

// 1.4    Its table
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.4.1
    // expr_quaternion_kind
    //   function: the kind's table: its name, "quaternion", its rank, a
    // number but not exact in every value, and its entries, its exact entry
    // telling which values are; one for the program, as an inline function's
    // static is.
    inline const dj_math_boxed_kind*
    expr_quaternion_kind()
    {
        static const dj_math_boxed_kind table = {
            "quaternion",
            expr_quaternion_rank,
            D_MATH_BOXED_NUMBER,
            &expr_quaternion_retain,
            &expr_quaternion_release,
            &expr_quaternion_unary,
            &expr_quaternion_binary,
            &expr_quaternion_cmp,
            &expr_quaternion_convert,
            &expr_quaternion_text,
            &expr_quaternion_exact
        };

        return &table;
    }

NS_END  // internal


//==============================================================================
// 2.  QUATERNION VALUES
//==============================================================================


// 2.1    Making and reading
//------------------------------------------------------------------------------
// 2.1.1
// expr_is_quaternion
//   function: whether a value is a quaternion: a value of the kind, by its
// rank.
D_NODISCARD inline bool
expr_is_quaternion(
    const value& _value
)
{
    return internal::expr_quaternion_held(_value.c_value());
}

NS_INTERNAL

    // expr_quaternion_failed
    //   function: whether _x is an error, which passes, or no number, a
    // domain error; and if so that, in _out.
    inline bool
    expr_quaternion_failed(
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

    // expr_quaternion_widest
    //   function: the widest floating-point type an operation of the _count
    // numbers at _x computes in: theirs, a long double where every one is
    // exact or of a boxed kind that is not.
    inline dj_math_value_kind
    expr_quaternion_widest(
        const value* _x,
        std::size_t  _count
    )
    {
        dj_math_value_kind widest = D_MATH_VALUE_NONE;

        // the widest each asks
        for (std::size_t i = 0; i < _count; ++i)
        {
            widest = expr_complex_widest(
                         widest,
                         expr_quaternion_type_of(_x[i].c_value()));
        }

        return ( ( (widest == D_MATH_VALUE_FLOAT) ||
                   (widest == D_MATH_VALUE_DOUBLE) )
                     ? widest
                     : D_MATH_VALUE_LONG_DOUBLE );
    }

    // expr_quaternion_zero_like
    //   function: a 0 of _part's kind: an exact 0 for an exact part, a +0 of
    // its type for a float, a double or a long double, and a long double's
    // +0 for any other inexact kind, as a real number's imaginary part is
    // (complex.hpp's expr_imag).
    inline value
    expr_quaternion_zero_like(
        const value& _part
    )
    {
        // by kind
        switch (_part.kind())
        {
            case D_MATH_VALUE_FLOAT:       return value(0.0f);
            case D_MATH_VALUE_DOUBLE:      return value(0.0);
            case D_MATH_VALUE_LONG_DOUBLE: return value(0.0L);
            default:
                break;
        }

        return ( (_part.is_exact()) ? value(0) : value(0.0L) );
    }

NS_END  // internal

// 2.1.2
// expr_quaternion
/**
 * @brief The quaternion of four real parts, the language's quat(w, x, y, z),
 *        w + x im + y jm + z km, which makes the numbers a sum cannot, a
 *        part of -0 or an infinity (S275).
 *
 * @param[in] _w  the real part, a real number.
 * @param[in] _x  the im part, a real number.
 * @param[in] _y  the jm part, a real number.
 * @param[in] _z  the km part, a real number.
 * @return an argument's error, the first; a domain error where a part is no
 *         real number, a boolean, none, a matrix, a complex number or a
 *         quaternion; where all four are exact and _y and _z are 0, the
 *         complex number of _w and _x, or the real _w where _x is 0 too;
 *         otherwise the quaternion, its parts exact where all four are, and
 *         else each its nearest in the widest of their floating-point types
 *         (S266), carrying every part's flags.
 */
D_NODISCARD inline value
expr_quaternion(
    const value& _w,
    const value& _x,
    const value& _y,
    const value& _z
)
{
    const value parts[4] = { _w, _x, _y, _z };
    value       failed;

    // an error, or no real part
    if (internal::expr_complex_failed(parts, 4, failed))
    {
        return failed;
    }

    return internal::expr_quaternion_made(_w, _x, _y, _z, 0u);
}

// 2.1.3
// expr_quaternion_of, expr_quaternion_to
/**
 * @brief The value of a math::quaternion<T>: a quaternion of the kind, its
 *        parts dense in T, as given, never made complex or real.
 *
 * @param[in] _q  the quaternion, of float, double or long double parts.
 * @return the value; D_MATH_VALUE_ERROR_MEMORY where storage is not had.
 */
template<typename Type>
D_NODISCARD value
expr_quaternion_of(
    const math::quaternion<Type>& _q
)
{
    return internal::expr_quaternion_dense(_q, 0u);
}

/**
 * @brief A number's nearest math::quaternion<T>.
 *
 * @param[in]  _value  the value.
 * @param[out] _out    the nearest quaternion of T's parts: a quaternion's
 *                     parts each its nearest T; a complex number's two,
 *                     the others +0; and a real number with the others +0;
 *                     as it was where the value is no number.
 * @return whether the value is a number.
 */
template<typename Type>
bool
expr_quaternion_to(
    const value&            _value,
    math::quaternion<Type>& _out
)
{
    // no number
    if (!_value.is_number())
    {
        return false;
    }

    const internal::expr_quaternion_parts_held parts =
        internal::expr_quaternion_parts_of(_value.c_value());
    math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));

    static_cast<void>(parts.w.to(q.w));
    static_cast<void>(parts.x.to(q.x));
    static_cast<void>(parts.y.to(q.y));
    static_cast<void>(parts.z.to(q.z));
    _out = q;

    return true;
}

// 2.2    The parts
//------------------------------------------------------------------------------
// The language's real and conj of a number (function.hpp), and parts
// (rotation.hpp): an error passes, and what is no number, a boolean, none or
// a matrix, is a domain error; every result carries the argument's flags.
// 2.2.1
// expr_quaternion_real
/**
 * @brief A number's real part, w.
 *
 * @param[in] _value  the number.
 * @return a quaternion's w, as it holds it; any other number's real part,
 *         complex.hpp's expr_real; an error, or a domain error, as section
 *         2.2 says.
 */
D_NODISCARD inline value
expr_quaternion_real(
    const value& _value
)
{
    value failed;

    // an error, or no number
    if (internal::expr_quaternion_failed(_value, failed))
    {
        return failed;
    }

    // a real or complex number: complex.hpp's
    if (!expr_is_quaternion(_value))
    {
        return expr_real(_value);
    }

    return internal::expr_complex_flagged(
        internal::expr_quaternion_parts_of(_value.c_value()).w,
        _value.flags());
}

// 2.2.2
// expr_quaternion_conj
/**
 * @brief A number's conjugate.
 *
 * @param[in] _value  the number.
 * @return a quaternion's conjugate, w - x im - y jm - z km, a dense one's by
 *         quat.h's, each sign flipped whatever it holds; any other number's,
 *         complex.hpp's expr_conj; an error, or a domain error, as section
 *         2.2 says.
 */
D_NODISCARD inline value
expr_quaternion_conj(
    const value& _value
)
{
    value failed;

    // an error, or no number
    if (internal::expr_quaternion_failed(_value, failed))
    {
        return failed;
    }

    // a real or complex number: complex.hpp's
    if (!expr_is_quaternion(_value))
    {
        return expr_conj(_value);
    }

    const internal::expr_quaternion_box& box =
        internal::expr_quaternion_box_of(_value.c_value());

    // by storage
    switch (box.storage)
    {
        case internal::expr_quaternion_storage::floats:
            return internal::expr_quaternion_dense(conjugate(box.as_float),
                                                   _value.flags());

        case internal::expr_quaternion_storage::doubles:
            return internal::expr_quaternion_dense(conjugate(box.as_double),
                                                   _value.flags());

        case internal::expr_quaternion_storage::long_doubles:
            return internal::expr_quaternion_dense(
                conjugate(box.as_long_double),
                _value.flags());

        case internal::expr_quaternion_storage::values:
        default:
            break;
    }

    return internal::expr_quaternion_made(box.w,
                                          -box.x,
                                          -box.y,
                                          -box.z,
                                          _value.flags());
}

// 2.3    The inverse and the sign
//------------------------------------------------------------------------------
// 2.3.1
// expr_quaternion_inverse
/**
 * @brief A number's inverse, 1 / q, the language's inv of a quaternion
 *        (S275).
 *
 * @param[in] _value  the number.
 * @return a quaternion's inverse, q* / |q|^2: exactly of exact parts, and of
 *         dense ones quat.h's, each part rounded once (S359); any other
 *         number's 1 / x, value.h's, an exact 0's a pole; an error, or a
 *         domain error, as section 2.2 says.
 */
D_NODISCARD inline value
expr_quaternion_inverse(
    const value& _value
)
{
    value failed;

    // an error, or no number
    if (internal::expr_quaternion_failed(_value, failed))
    {
        return failed;
    }

    // a real or complex number, or exact parts: the quotient of 1
    if ( (!expr_is_quaternion(_value)) ||
         (internal::expr_quaternion_box_of(_value.c_value()).storage ==
              internal::expr_quaternion_storage::values) )
    {
        return (value(1) / _value);
    }

    const internal::expr_quaternion_box& box =
        internal::expr_quaternion_box_of(_value.c_value());

    // by storage: quat.h's inverse in the parts' type
    switch (box.storage)
    {
        case internal::expr_quaternion_storage::floats:
            return internal::expr_quaternion_dense(inverse(box.as_float),
                                                   _value.flags());

        case internal::expr_quaternion_storage::doubles:
            return internal::expr_quaternion_dense(inverse(box.as_double),
                                                   _value.flags());

        case internal::expr_quaternion_storage::long_doubles:
        default:
            break;
    }

    return internal::expr_quaternion_dense(inverse(box.as_long_double),
                                           _value.flags());
}

NS_INTERNAL

    // expr_quaternion_normalized_as
    //   function: _q / |q| in Type by quat.h's scaled normalization (S361),
    // _q itself where its norm is 0, as sign(0) is 0 (S338).
    template<typename Type>
    value
    expr_quaternion_normalized_as(
        const value& _q
    )
    {
        math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));

        static_cast<void>(expr_quaternion_to(_q, q));

        // a norm of 0: itself
        if (norm(q) == Type(0))
        {
            return expr_quaternion_dense(q, _q.flags());
        }

        return expr_quaternion_dense(normalize(q), _q.flags());
    }

NS_END  // internal

// 2.3.2
// expr_quaternion_sign
/**
 * @brief A quaternion's sign, q / |q|, the language's sign of a quaternion
 *        (S275), as sign(z) is z / |z| of a complex z (S338).
 *
 * @param[in] _value  the quaternion.
 * @return q / |q|: exactly where |q| is, |q| being rational; and otherwise
 *         in the parts' floating-point type, a long double for exact ones,
 *         by quat.h's scaled normalization, dense; a 0 quaternion itself;
 *         an error passes; a domain error for any value but a quaternion,
 *         whose sign is function.hpp's.
 */
D_NODISCARD inline value
expr_quaternion_sign(
    const value& _value
)
{
    // an error passes; no quaternion: outside the domain
    if (_value.kind() == D_MATH_VALUE_ERROR)
    {
        return _value;
    }

    if (!expr_is_quaternion(_value))
    {
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    const value modulus = abs(_value);

    // an exact norm: an exact quotient
    if (modulus.is_exact())
    {
        return (_value / modulus);
    }

    // in the parts' type
    switch (internal::expr_quaternion_widest(&_value, 1))
    {
        case D_MATH_VALUE_FLOAT:
            return internal::expr_quaternion_normalized_as<float>(_value);

        case D_MATH_VALUE_DOUBLE:
            return internal::expr_quaternion_normalized_as<double>(_value);

        default:
            break;
    }

    return internal::expr_quaternion_normalized_as<long double>(_value);
}

// 2.4    Functions
//------------------------------------------------------------------------------
// A function's principal value of a quaternion, in its parts' floating-point
// type, as the language's exp, log, sqrt and ^ of a real exponent take it
// (function.hpp, S275): the complex function in q's plane, quat.h's _c form
// through math::quaternion<T>, the same bits as the face's.
NS_INTERNAL

    // 2.4.1
    // expr_quaternion_fn
    //   enum: the functions of a quaternion this section computes.
    enum class expr_quaternion_fn
    {
        exp  = 1,
        log  = 2,
        sqrt = 3,
        pow  = 4
    };

    // expr_quaternion_apply_as
    //   function: _fn of _q in Type, a real power by _p, each its nearest
    // there; the result dense, never made complex or real, with _flags.
    template<typename Type>
    value
    expr_quaternion_apply_as(
        expr_quaternion_fn _fn,
        const value&       _q,
        const value&       _p,
        unsigned           _flags
    )
    {
        math::quaternion<Type> q(Type(0), Type(0), Type(0), Type(0));
        Type                   p = Type(0);

        static_cast<void>(expr_quaternion_to(_q, q));
        static_cast<void>(_p.to(p));

        // by function
        switch (_fn)
        {
            case expr_quaternion_fn::exp:
                return expr_quaternion_dense(exp(q), _flags);

            case expr_quaternion_fn::log:
                return expr_quaternion_dense(log(q), _flags);

            case expr_quaternion_fn::sqrt:
                return expr_quaternion_dense(sqrt(q), _flags);

            case expr_quaternion_fn::pow:
            default:
                break;
        }

        return expr_quaternion_dense(pow(q, p), _flags);
    }

NS_END  // internal

// 2.4.2
// expr_quaternion_apply
/**
 * @brief exp, log, sqrt or a real power of a quaternion, in the widest
 *        floating-point type of its parts and the exponent's.
 *
 * @param[in] _fn  the function.
 * @param[in] _q   the quaternion, or any number, taken as one.
 * @param[in] _p   the real exponent of the power, and ignored otherwise.
 * @return the function in the widest of the operands' floating-point types,
 *         a long double where every one is exact or of a boxed kind that is
 *         not, by quat.h's _c form; dense, never made complex or real
 *         (S266); with every operand's flags. D_MATH_VALUE_ERROR_MEMORY
 *         where storage is not had.
 */
D_NODISCARD inline value
expr_quaternion_apply(
    internal::expr_quaternion_fn _fn,
    const value&                 _q,
    const value&                 _p
)
{
    const value       operands[2] = { _q, _p };
    const std::size_t count       =
        ( (_fn == internal::expr_quaternion_fn::pow) ? 2 : 1 );
    const unsigned    flags       =
        ( _q.flags() | ( (count == 2) ? _p.flags() : 0u ) );

    // in the widest type
    switch (internal::expr_quaternion_widest(operands, count))
    {
        case D_MATH_VALUE_FLOAT:
            return internal::expr_quaternion_apply_as<float>(_fn,
                                                             _q,
                                                             _p,
                                                             flags);

        case D_MATH_VALUE_DOUBLE:
            return internal::expr_quaternion_apply_as<double>(_fn,
                                                              _q,
                                                              _p,
                                                              flags);

        default:
            break;
    }

    return internal::expr_quaternion_apply_as<long double>(_fn,
                                                           _q,
                                                           _p,
                                                           flags);
}

// 2.4.3
// expr_quaternion_root
/**
 * @brief The principal square root of an exact quaternion, exactly, where it
 *        is a quaternion of rationals: (s, v / 2s), s^2 = (|q| + w) / 2.
 *
 * @param[in]  _value  the quaternion.
 * @param[out] _out    the root, where there is one, with the quaternion's
 *                     flags.
 * @return whether the root is exact: the quaternion's parts exact, |q|
 *         rational, and (|q| + w) / 2 the square of a rational, as every
 *         rational root's real part makes it; false for any other value.
 */
D_NODISCARD inline bool
expr_quaternion_root(
    const value& _value,
    value&       _out
)
{
    // no exact quaternion
    if ( (!expr_is_quaternion(_value)) ||
         (!_value.is_exact()) )
    {
        return false;
    }

    const internal::expr_quaternion_parts_held parts =
        internal::expr_quaternion_parts_of(_value.c_value());
    const value modulus = abs(_value);

    // an irrational norm: no rational root
    if (!modulus.is_exact())
    {
        return false;
    }

    const value s =
        internal::value_root_of( (modulus + parts.w) / value(2) );

    // s irrational: no rational root; s is no 0, as |v| is not
    if (!s.is_exact())
    {
        return false;
    }

    const value twice = (s * value(2));

    _out = internal::expr_complex_flagged(
        internal::expr_quaternion_made(s,
                                       parts.x / twice,
                                       parts.y / twice,
                                       parts.z / twice,
                                       0u),
        _value.flags());

    return true;
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_QUATERNION_HPP
