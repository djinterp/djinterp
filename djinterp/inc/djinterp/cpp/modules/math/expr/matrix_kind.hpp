/*******************************************************************************
* djinterp [cpp][math]                                           matrix_kind.hpp
*
* Vectors and matrices as values of the run-time expression language: the
* matrix kind, its values, its operators, transposes and blocks; roadmap
* Phase 3, 3a and 3b, and 3c's empty matrix, blocks and negative powers,
* matrices of complex numbers, 3e's third part, and matrices of quaternions,
* 3f's third part. Linear algebra's functions of them are
* matrix_function.hpp's.
*   THE KIND (3a) is a boxed kind of the value layer, as the wide integer is
* (c/math/value_wide.h), so that value.h meets it without change: its table is
* written here, in C++, from C++11, as the language that makes matrices is
* (S237). It is one kind, "matrix", of the rank 1000, past every number's, so
* that in an operation of a matrix and a number the matrix's table handles it;
* a matrix is no number (S238). It keeps each element as it is given: as a
* dense array of float, double or long double where every element is one of
* those, in the widest one's type, to which each widens exactly, or of
* math::complex of the widest type where any element is a complex number of
* such parts, a real element x then x + 0i (S348), or of math::quaternion of
* the widest type where any is a quaternion of such parts, a real element x
* then x + 0im + 0jm + 0km and a complex one x + yi x + yim + 0jm + 0km
* (S386); and as math::values, each of its own kind, where any element is
* exact or boxed, an exact complex number or quaternion among them. So no
* element is rounded by being put in a matrix, and a
* matrix of exact numbers is exact (S239). A vector is a matrix of one row or
* one column: expr_vector's, a vector literal's, is one row (S240). An element
* is a number: an element's error passes, the first, and an element that is no
* number, a boolean, none or a matrix, is a domain error, as in arithmetic; a
* matrix has a row and a column at least (S241), or is the empty matrix, of
* neither, 0 by 0 (S260). A matrix compares equal to a matrix of its shape
* whose elements are equal, place by place, and stands in no order otherwise
* (S242). A vector literal [1, 2, 3] and a matrix literal [1, 2; 3, 4] make
* one (parse.hpp, evaluate.hpp), [] the empty one; this header makes and reads
* them in C++.
*   THE OPERATORS (3b) are the value's own, through the kind's table: A + B
* and A - B of one shape, -A, A * s, s * A and A / s of a matrix and a number,
* and A * B where A's columns are B's rows; any other operation on a matrix is
* a domain error, A + s, s / A and A / B among them, and so is one of shapes
* that do not fit; a 1 by 1 matrix stays a matrix (S246). Each is computed by
* linear algebra's kernels, a dense matrix's by the C core's vec.h and mat.h
* in the wider type, one holding values by the generic path in the value's own
* arithmetic, exact wherever its elements are (S247); a dense complex one's in
* the wider type, complex where either operand is, element by element by
* math::complex's operators, a real operand's element or a real scalar taken
* as itself (C99's G.5), a product's each element's parts rounded once from
* Dot2's pair, as the face's kernels sum it, a real operand's products with
* the other's parts alone (S349, S412); and a dense one of quaternions' in the
* wider type, by the generic kernels on the quaternion kind's operands, each
* element as the kind computes its two, a real or complex operand taken as
* itself, each product in its order, a number's product with a matrix too,
* A / q being A inv(q) (S277, S387). Beside them, the conjugate transpose A'
* (S273); A ^ n of a square matrix and a whole n, by repeated squaring as the
* C core's d_mat_power squares (S248, S262), A ^ 0 the identity of A's
* storage and A ^ (-n) the inverse's power (S259); and indexing, from 1:
* A[i, j] an element, v[i] a vector's, A[i] another matrix's row i, an index
* any whole number, exact or not (S249).
*   BLOCKS. A literal's elements are blocks (S261): a row's numbers and
* matrices stand beside one another, each of the first's rows, and a
* matrix's rows one above another, each of the first's columns, so that
* [A, b] is A with the column b beside it and [A; B] A above B; the empty
* matrix is skipped, and blocks that do not fit are a domain error.
*   COMPLEX NUMBERS (complex_kind.hpp) are matrices' elements from step 23's
* third part (S273, S348). A' conjugates each, as a complex number's z' is
* its conjugate (S333), and transpose(A) does not; real, imag, conj and arg
* of a matrix are its elements', and complex(A, B) of two matrices of one
* shape the matrix of complex(a_ij, b_ij) (S350).
*   QUATERNIONS (quaternion_kind.hpp, S275) are matrices' elements from step
* 24's third part, chunk 46 (S386 to S390), whose expr_status::not_yet (S369) is
* gone: A' conjugates each, transpose(A) does not, real and conj of a matrix
* are its elements', and imag, arg and complex of one a domain error; a sum
* of a matrix and a quaternion stays a domain error, as a matrix plus a
* number is (S246). Their rotations' matrices are rotation.hpp's (S391).
*   ONE HEADER FOR IT. Until chunk 48 this header was matrix.hpp, of the
* name of linear algebra's class (linear_algebra/matrix.hpp), and held
* linear algebra's functions of matrix values too, 7,979 lines; they are
* matrix_function.hpp's now, which includes this header (S400, S401).
*   STEPS. Step 19 (3a) filled the kind, expr_matrix, expr_vector,
* expr_is_matrix, expr_shape_of and expr_element; step 20 (3b) filled
* expr_transpose and the kind's arithmetic, powers and indexing; step 21
* (3c) filled the empty matrix, blocks and negative powers; step 23 (3e) a
* complex number's transpose, and in its third part, chunk 43, matrices of
* complex numbers: their storage, arithmetic and transposes, the parts of a
* matrix and complex(A, B); step 24 (3f), its second part, in chunk 45, the
* refusals of quaternions (S369), and its third, chunk 46, matrices of
* them: their storage, arithmetic and transposes (S386, S387).
*   PARTS. Its text is in three headers that stand alone, included below in
* order (S437): matrix_kind_storage.hpp, sections 1 to 3.1;
* matrix_kind_arithmetic.hpp, section 3.2; and
* matrix_kind_transpose.hpp, sections 3.3 and 3.4.
*
* HOW TO USE
* ----------
*   Reached through expr.hpp. It includes its three parts in order (S437): the
* matrix kind of the language, its literals, arithmetic, transposes, blocks and
* indexing as values.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None of its own: it compiles from C++11, as the run-time language does, and
* is empty below it.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/matrix_kind.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SHAPES
    ------
    1.  The shape
         1.  expr_shape
2.  THE KIND
    --------
    1.  Its storage
         1.  expr_matrix_rank
         2.  expr_matrix_storage
         3.  expr_matrix_box
         4.  Reading a box
    2.  Its entries
         1.  expr_matrix_retain, expr_matrix_release
         2.  expr_matrix_unary, expr_matrix_binary
         3.  expr_matrix_cmp
         4.  expr_matrix_convert
         5.  expr_matrix_text
    3.  Its table
         1.  expr_matrix_kind
         2.  expr_matrix_adopt
3.  MATRIX VALUES
    -------------
    1.  Making and reading
         1.  expr_matrix
         2.  expr_vector
         3.  expr_is_matrix
         4.  expr_shape_of
         5.  expr_element
    2.  Arithmetic
         1.  Arrays
         2.  The dense type
         3.  Element by element
         4.  The product
         5.  expr_matrix_operation
         6.  The transpose
         7.  The square algebra
         8.  Powers
         9.  Indexing
    3.  The transpose
         1.  expr_transpose
         2.  expr_transpose_value
         3.  expr_matrix_each, expr_matrix_complex_of
    4.  Blocks
         1.  expr_matrix_beside, expr_matrix_stacked
*/

#ifndef DJINTERP_MATH_EXPR_MATRIX_KIND_HPP
#define DJINTERP_MATH_EXPR_MATRIX_KIND_HPP 1

#include "./matrix_kind_storage.hpp"     // sections 1 to 3.1
#include "./matrix_kind_arithmetic.hpp"  // section 3.2
#include "./matrix_kind_transpose.hpp"   // sections 3.3 and 3.4


#endif  // DJINTERP_MATH_EXPR_MATRIX_KIND_HPP
