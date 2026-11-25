/*******************************************************************************
* djinterp [cpp][math]                                           t_expr_face.cpp
*
* Tests for math/expr/, the run-time expression language, as steps 13 to 22 (2a
* to 3d) and step 23's first part (3e) made it: every header in place, its types
* whole; the arithmetic, the functions and the constants, the relations, logic
* and piecewise parsed, printed and evaluated; the handle, math::expr;
* substitution and simplification; and vectors and matrices, their kind, their
* literals, their arithmetic and indexing, and linear algebra's functions.
*   - every header read, at every level, and empty below C++11;
*   - the types: the error, its statuses and the answer of a step; the
*     syntax switches; the token; the operator and the operator table; the
*     atom, the term and its constructors and observers, and the term under
*     parse/expression's queries and transforms; the function and constant
*     tables; the bindings and the context; the shape of a matrix; the
*     handle;
*   - 2b's steps: the tokenizer; the operator table's arithmetic; the parser
*     layer, its grammar, implied multiplication and every error at its
*     place; the renderer, numbers with and without literals; both folds,
*     in each type and as a value, exact where they are; and the round trip,
*     random terms rendered and parsed back as themselves;
*   - 2c's: the comma; the factorial; the standard functions and constants,
*     in each type and as values, exact wherever a function's value is
*     rational; calls and bars in the parser and the renderer; and calls,
*     constants and the factorial in the round trip;
*   - 2d's: the relations and logic at their levels, logic below the
*     relations; chains; brackets as one operand; piecewise, its branches
*     and their errors; true, false, all_of and any_of; both folds, 1 and 0
*     in each type and booleans as values; and all of it in the round trip;
*   - 2e's: the handle, what it holds; str() and eval, in each type and as
*     a value; subs, simplify and diff, each its entry's; parse and
*     try_parse, by the syntax given; chains, roadmap Phase 2's examples
*     among them; an error passed through every member; and the handle
*     agreeing with the entries on random terms;
*   - 2f's: substitution, a variable replaced and nothing else, and on
*     random terms the value it gives; simplification, each rule, what it
*     leaves as it is, and on random terms no larger, a fixed point, its
*     text read back the same, and the value kept wherever each part of the
*     term is finite;
*   - 3a's: the square brackets and the semicolon as tokens; the matrix
*     kind, its storage keeping each element as given, its table and entries,
*     and its holders; expr_matrix, expr_vector, expr_is_matrix,
*     expr_shape_of and expr_element, and their errors; the literals in the
*     parser, each error at its place; the literals and the matrices a
*     fold makes in the renderer, read back; both folds, a literal's matrix
*     as a value and no value in a type; the operators, the rewrites and
*     the handle beside them; and random literals, read back, folded,
*     simplified and substituted in, as their elements are;
*   - 3b's: the kind's arithmetic, sums, scalar products and the product,
*     each result's storage, and what it refuses; the transpose, powers and
*     indexing as values, and two matrices compared; indexing and the
*     transpose in the parser, every error at its place, and in the
*     renderer, read back; both folds of matrix texts; simplification
*     keeping matrix factors in order and no error of matrices made a
*     value; and 1,000 random terms of matrices, each folded as the random
*     part's own arithmetic gives it, read back, and simplified to its
*     value;
*   - 3c's: linear algebra's functions, det, inv, trace, norm, eig, dot,
*     cross and solve, their entries and the table's, of matrices exact,
*     dense in each type, the C core's bit for bit, and of values with an
*     inexact element, and of numbers as scalars, in each type and as
*     values; a norm past its type's range scaled, not an infinity; the
*     eigenvalues exact where rational, repeated, beside irrational ones,
*     a dense symmetric matrix's by Jacobi where QR would find a complex
*     pair, a complex pair complex numbers, exact where they are Gaussian
*     rationals (S352); transpose, in the table; negative powers, and
*     every dense power the C core's; the empty matrix; literals of
*     blocks, in the parser, the folds, the renderer and simplification;
*     and 1,000 random exact matrices against the test's own cofactors and
*     arithmetic;
*   - 3d's: im, jm and km, constants of the table and never variables, as
*     values, in each type and through the handle;
*   - 3e's first part: im the exact imaginary unit as a value and a NaN in
*     each type, complex, real, imag, conj and arg in the table with their
*     four evaluators (t_expr_complex_face.cpp has the rest);
*   - 3f's second part: jm and km the exact units of the quaternions as
*     values, their product in order, and NaNs in each type; quat, parts and
*     rotate in the table with their seven evaluators, and its third part's
*     rotmat, rotquat and slerp, 58 functions (t_expr_quaternion_face.cpp
*     and t_expr_quaternion_matrix_face.cpp have the rest);
*   - every stub of a later step: the answer its contract names until its
*     step, the error not_yet with its message;
*   - each entry's type, by static assertion.
* Each later step of Phases 2 to 4 turns its stubs' checks into checks of
* what it fills.
*   Its text is in eight parts, t_expr_face_*.hpp beside it, included below in
* order (S438): the helpers, the types, parsing, the rewrites, three of
* matrices and the handle; this file holds the module's driver and main.
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/expr/t_expr_face.cpp -o t_expr_face
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.21
*******************************************************************************/


#include "../../../../../../inc/djinterp/cpp/modules/math/expr/antiderivative.hpp"   // expr_antiderivative
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/arithmetic.hpp"       // ^, mod, the functions' evaluators
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/complex_kind.hpp"     // the complex kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/derivative.hpp"       // expr_derivative
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/evaluate.hpp"         // expr_evaluate, expr_bindings
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/expr.hpp"             // math::expr, parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/expr_common.hpp"      // expr_error, expr_result, expr_context
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/function_table.hpp"   // the function and constant tables
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/logic.hpp"            // the relations, logic, piecewise
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix_function.hpp"  // det, inv, trace, norm, eig, dot, ...
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix_kind.hpp"      // the matrix kind, expr_matrix
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix_kind_storage.hpp"    // its part: storage, entries, table
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix_kind_arithmetic.hpp" // its part: arithmetic
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/matrix_kind_transpose.hpp"  // its part: transposes, blocks
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/numeric.hpp"          // expr_integrate, expr_derivative_at
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/operator.hpp"         // expr_op, expr_signature
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/parse.hpp"            // expr_parse
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/program.hpp"          // expr_compile, expr_program
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/quaternion_kind.hpp"  // the quaternion kind
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/render.hpp"           // expr_render
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/rotation.hpp"         // parts, rotate, rotmat, rotquat, slerp
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/simplify.hpp"         // expr_simplify
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/substitute.hpp"       // expr_substitute
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/summation.hpp"        // expr_sum, expr_limit, expr_taylor
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/term.hpp"             // expr_term, expr_number
#include "../../../../../../inc/djinterp/cpp/modules/math/expr/token.hpp"            // expr_tokenize, expr_read_decimal

#include "./t_expr_face_helpers.hpp"          // sections 1 and 2
#include "./t_expr_face_types.hpp"            // sections 3 to 7
#include "./t_expr_face_parsing.hpp"          // sections 8 and 9
#include "./t_expr_face_rewrites.hpp"         // section 10
#include "./t_expr_face_matrices.hpp"         // section 11's first third
#include "./t_expr_face_arithmetic.hpp"       // section 11's second third
#include "./t_expr_face_algebra.hpp"          // section 11's last third
#include "./t_expr_face_handle.hpp"           // sections 12 to 14

// D_TESTS_MODULE_READ
//   1 where each of math/expr/'s twenty-six headers was read: at every
// level, as each defines its guard before its floor.
#if ( defined(DJINTERP_MATH_EXPR_ANTIDERIVATIVE_HPP)  &&                       \
      defined(DJINTERP_MATH_EXPR_ARITHMETIC_HPP)      &&                       \
      defined(DJINTERP_MATH_EXPR_COMPLEX_KIND_HPP)    &&                       \
      defined(DJINTERP_MATH_EXPR_DERIVATIVE_HPP)      &&                       \
      defined(DJINTERP_MATH_EXPR_EVALUATE_HPP)        &&                       \
      defined(DJINTERP_MATH_EXPR_EXPR_HPP)            &&                       \
      defined(DJINTERP_MATH_EXPR_EXPR_COMMON_HPP)     &&                       \
      defined(DJINTERP_MATH_EXPR_FUNCTION_TABLE_HPP)  &&                       \
      defined(DJINTERP_MATH_EXPR_LOGIC_HPP)           &&                       \
      defined(DJINTERP_MATH_EXPR_MATRIX_FUNCTION_HPP) &&                       \
      defined(DJINTERP_MATH_EXPR_MATRIX_KIND_HPP)     &&                       \
      defined(DJINTERP_MATH_EXPR_MATRIX_KIND_STORAGE_HPP)    &&                \
      defined(DJINTERP_MATH_EXPR_MATRIX_KIND_ARITHMETIC_HPP) &&                \
      defined(DJINTERP_MATH_EXPR_MATRIX_KIND_TRANSPOSE_HPP)  &&                \
      defined(DJINTERP_MATH_EXPR_NUMERIC_HPP)         &&                       \
      defined(DJINTERP_MATH_EXPR_OPERATOR_HPP)        &&                       \
      defined(DJINTERP_MATH_EXPR_PARSE_HPP)           &&                       \
      defined(DJINTERP_MATH_EXPR_PROGRAM_HPP)         &&                       \
      defined(DJINTERP_MATH_EXPR_QUATERNION_KIND_HPP) &&                       \
      defined(DJINTERP_MATH_EXPR_RENDER_HPP)          &&                       \
      defined(DJINTERP_MATH_EXPR_ROTATION_HPP)        &&                       \
      defined(DJINTERP_MATH_EXPR_SIMPLIFY_HPP)        &&                       \
      defined(DJINTERP_MATH_EXPR_SUBSTITUTE_HPP)      &&                       \
      defined(DJINTERP_MATH_EXPR_SUMMATION_HPP)       &&                       \
      defined(DJINTERP_MATH_EXPR_TERM_HPP)            &&                       \
      defined(DJINTERP_MATH_EXPR_TOKEN_HPP) )
    #define D_TESTS_MODULE_READ 1
#else
    #define D_TESTS_MODULE_READ 0
#endif


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

namespace
{
    //==========================================================================
    // 15. THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part of the module, from its floor.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_MODULE_READ == 1),
                        "the module's twenty-three headers read");

        d_tests_errors();
        d_tests_tokens();
        d_tests_tokenize();
        d_tests_operators();
        d_tests_table();
        d_tests_atoms();
        d_tests_terms();
        d_tests_tables();
        d_tests_standard_functions();
        d_tests_standard_constants();
        d_tests_bindings();
        d_tests_fold_typed();
        d_tests_fold_value();
        d_tests_fold_passing();
        d_tests_fold_functions();
        d_tests_fold_functions_value();
        d_tests_fold_logic();
        d_tests_fold_errors();
        d_tests_parse_numbers();
        d_tests_parse_grammar();
        d_tests_parse_implied();
        d_tests_parse_calls();
        d_tests_parse_bars();
        d_tests_parse_logic();
        d_tests_parse_piecewise();
        d_tests_parse_tokens();
        d_tests_parse_errors();
        d_tests_render();
        d_tests_render_numbers();
        d_tests_round_trip();
        d_tests_folds_agree();
        d_tests_numbers_read_back();
        d_tests_substitute();
        d_tests_substitute_random();
        d_tests_simplify_constants();
        d_tests_simplify_powers();
        d_tests_simplify_sums();
        d_tests_simplify_products();
        d_tests_simplify_values();
        d_tests_simplify_random();
        d_tests_rewrites();
        d_tests_matrices();
        d_tests_parse_literals();
        d_tests_render_literals();
        d_tests_fold_literals();
        d_tests_literals_random();
        d_tests_matrix_arithmetic();
        d_tests_matrix_powers();
        d_tests_parse_indexing();
        d_tests_render_indexing();
        d_tests_fold_indexing();
        d_tests_simplify_matrices();
        d_tests_matrices_random();
        d_tests_matrix_algebra();
        d_tests_matrix_eigenvalues();
        d_tests_matrix_negative_powers();
        d_tests_empty_matrix();
        d_tests_blocks();
        d_tests_fold_algebra();
        d_tests_algebra_random();
        d_tests_units();
        d_tests_calculus();
        d_tests_handle();
        d_tests_handle_rewrites();
        d_tests_handle_entries();
        d_tests_handle_chains();
        d_tests_handle_agrees();
        d_tests_types();

        return;
    }
}  // namespace

#else

namespace
{
    //==========================================================================
    // 15. THE MODULE
    //==========================================================================

    // d_tests_module
    //   below the module's floor: each header read, and nothing more to
    // test, as each is empty here.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_MODULE_READ == 1),
                        "the module's twenty-three headers read");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    d_tests_module();

    std::printf("t_expr_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
