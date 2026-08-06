/******************************************************************************
* djinterp [math]                                                   linalg.hpp
*
* Umbrella header for the linear-algebra subframework.
*   Includes every public linalg header. Users may also include the sub-headers
* directly when they want to keep compile times tight or depend only on a slice
* of the framework.
*
* DIRECTORY MAP (built):
*   linalg_common.hpp - nested namespace, vector/matrix forward decls,
*                       structural traits (is_vector / is_matrix /
*                       is_square_matrix), the constexpr scalar kernel
*                       (sqrt / abs / acos), and the default tolerance
*   vector.hpp        - fixed-size vector<_T, _N>: arithmetic, dot, cross,
*                       norms, normalize, projection/rejection, angle, lerp,
*                       map/reduce, and the procedural free-function spellings
*   matrix.hpp        - fixed-size matrix<_T, _Rows, _Cols>: arithmetic,
*                       matrix*matrix and matrix*vector products, transpose,
*                       trace, Frobenius norm, integer powers, the linear-map
*                       operator() bridge, and the procedural spellings
*   square.hpp        - square-matrix operations: determinant (fraction-free),
*                       inverse (Gauss-Jordan), invertibility tests, submatrix /
*                       minor / cofactor / cofactor_matrix, adjugate,
*                       orthogonality test
*   decomposition.hpp - matrix factorizations as factor-once objects: LU
*                       (partial pivot, P A = L U), QR (Householder, A = Q R),
*                       Cholesky (A = L L^T); factories lu / qr / cholesky
*   solve.hpp         - linear-system solvers: solve (square, LU), triangular
*                       forward/back substitution, solve_spd (Cholesky),
*                       least_squares (overdetermined, QR)
*   eigen.hpp         - eigenproblems: symmetric Jacobi (values + orthonormal
*                       vectors), power iteration (dominant pair), shifted QR
*                       algorithm (real eigenvalues), general eigenvectors via
*                       inverse iteration; eigen_symmetric / power_iteration /
*                       eigenvalues_qr / eigen_general
*   transform.hpp     - transformation builders (scaling, rotation, translation),
*                       homogeneous coordinates (to/from, transform_point /
*                       transform_direction), and the std::array geometry bridge
*                       (to_array / to_vector)
*   svd.hpp           - singular value decomposition (one-sided Jacobi) and the
*                       Moore-Penrose pseudoinverse; svd / pseudoinverse / pinv
*   dynamic_matrix.hpp - runtime-sized, heap-backed matrix companion (NOT
*                       constexpr) mirroring the fixed-size API, with a
*                       from_fixed / to_fixed bridge to matrix<_T,_Rows,_Cols>
*
* NAMESPACE:
*   All types and functions live in djinterp::math::linalg (one level deeper
* than the geometry subframework -- see linalg_common.hpp for the rationale).
*
* WIRING INTO math.hpp:
*   add  #include "./linalg/linalg.hpp"  alongside the geometry/calculus
* includes in /inc/math/math.hpp to pull the subframework into the math
* umbrella.
*
* path:      /inc/djinterp/math/linear_algebra/linalg.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_MATH_LINALG_
#define DJINTERP_MATH_LINALG_ 1

#include "./linalg_common.hpp"

// core value types
#include "./vector.hpp"
#include "./matrix.hpp"

// operations
#include "./square.hpp"
#include "./decomposition.hpp"
#include "./solve.hpp"
#include "./eigen.hpp"
#include "./svd.hpp"

// transforms
#include "./transform.hpp"

// runtime-sized companion
#include "./dynamic_matrix.hpp"


#endif  // DJINTERP_MATH_LINALG_
