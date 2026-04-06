/******************************************************************************
* djinterp [maths]                                       function_traits.hpp
*
* SFINAE type traits for mathematical function types.
*   This header provides compile-time detection of function categories and
* their properties using pure structural SFINAE (no tag types required).
* All detection uses the presence or absence of well-known static members,
* type aliases, and member functions.
*
* NAMING CONVENTIONS:
*   is_<pattern>             - primary trait (inherits from bool_constant)
*   is_<pattern>_v           - variable template helper
*   function_<property>      - type extraction metafunction
*   function_<property>_t    - type extraction alias
*
* DETECTED PATTERNS:
*   Category:
*     - Parametric function (curve, surface, N-dimensional)
*     - Piecewise function
*     - Implicit function
*     - Inequality / constraint expression
*     - Vector-valued function
*     - Polar-form function
*     - Inverse function marker
*     - Multivariable function
*     - Named/wrapped function (math_function)
*
*   Coordinate system:
*     - Cartesian, polar, cylindrical, spherical
*
*   Properties:
*     - Arity (nullary, unary, binary, ternary, n-ary)
*     - Output dimension (scalar vs vector)
*     - Parameter count (for parametric types)
*     - Domain attachment
*     - Differentiability
*     - Composition compatibility
*
*   Classification:
*     - function_class<T> aggregate struct
*
* STRUCTURAL REQUIREMENTS (detected members):
*   Functions:        static evaluate(...), static constexpr arity
*   Parametric:       static constexpr is_parametric,
*                     static constexpr parameter_count,
*                     components_type (tuple)
*   Piecewise:        static constexpr is_piecewise, pieces_type (tuple)
*   Implicit:         static constexpr is_implicit
*   Inequality:       static constexpr is_inequality
*   Vector-valued:    static constexpr is_vector_valued,
*                     static constexpr output_dimension
*   Inverse:          static constexpr is_inverse, original_type
*   Polar-form:       static constexpr is_polar_form, radius_type
*   Named function:   expression_type, domain_type, coordinate_system
*   Coordinate sys:   static constexpr dimension,
*                     static constexpr is_cartesian / is_polar /
*                     is_cylindrical / is_spherical
*
* path:      /inc/maths/function_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATHS_FUNCTION_TRAITS_
#define DJINTERP_MATHS_FUNCTION_TRAITS_ 1

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../env.h"
#include "../cpp_features.h"
#include "../djinterp.h"


NS_DJINTERP
NS_MATHS


// ============================================================================
// I.    STRUCTURAL DETECTION HELPERS
// ============================================================================

NS_INTERNAL

    // ---- evaluability -------------------------------------------------------

    // has_evaluate
    //   helper: detects static evaluate member function.
    template<typename _Type,
             typename = void>
    struct has_evaluate : std::false_type
    {
    };

    template<typename _Type>
    struct has_evaluate<_Type, void_t<
        decltype(_Type::evaluate(std::declval<int>()))
    >> : std::true_type
    {
    };

    // has_arity
    //   helper: detects static constexpr arity member.
    template<typename _Type,
             typename = void>
    struct has_arity : std::false_type
    {
    };

    template<typename _Type>
    struct has_arity<_Type, void_t<decltype(_Type::arity)>>
        : std::true_type
    {
    };

    // has_degree
    //   helper: detects static constexpr degree member.
    template<typename _Type,
             typename = void>
    struct has_degree : std::false_type
    {
    };

    template<typename _Type>
    struct has_degree<_Type, void_t<decltype(_Type::degree)>>
        : std::true_type
    {
    };

    // has_is_constant_expr
    //   helper: detects static constexpr is_constant_expr member.
    template<typename _Type,
             typename = void>
    struct has_is_constant_expr : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_constant_expr<_Type,
                               void_t<decltype(_Type::is_constant_expr)>>
        : std::true_type
    {
    };

    // ---- category markers ---------------------------------------------------

    // has_is_parametric
    //   helper: detects static constexpr is_parametric member.
    template<typename _Type,
             typename = void>
    struct has_is_parametric : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_parametric<_Type,
                            void_t<decltype(_Type::is_parametric)>>
        : std::true_type
    {
    };

    // has_is_implicit
    //   helper: detects static constexpr is_implicit member.
    template<typename _Type,
             typename = void>
    struct has_is_implicit : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_implicit<_Type,
                          void_t<decltype(_Type::is_implicit)>>
        : std::true_type
    {
    };

    // has_is_piecewise
    //   helper: detects static constexpr is_piecewise member.
    template<typename _Type,
             typename = void>
    struct has_is_piecewise : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_piecewise<_Type,
                           void_t<decltype(_Type::is_piecewise)>>
        : std::true_type
    {
    };

    // has_is_inequality
    //   helper: detects static constexpr is_inequality member.
    template<typename _Type,
             typename = void>
    struct has_is_inequality : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_inequality<_Type,
                            void_t<decltype(_Type::is_inequality)>>
        : std::true_type
    {
    };

    // has_is_vector_valued
    //   helper: detects static constexpr is_vector_valued member.
    template<typename _Type,
             typename = void>
    struct has_is_vector_valued : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_vector_valued<_Type,
                               void_t<decltype(_Type::is_vector_valued)>>
        : std::true_type
    {
    };

    // has_is_inverse
    //   helper: detects static constexpr is_inverse member.
    template<typename _Type,
             typename = void>
    struct has_is_inverse : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_inverse<_Type,
                         void_t<decltype(_Type::is_inverse)>>
        : std::true_type
    {
    };

    // has_is_polar_form
    //   helper: detects static constexpr is_polar_form member.
    template<typename _Type,
             typename = void>
    struct has_is_polar_form : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_polar_form<_Type,
                            void_t<decltype(_Type::is_polar_form)>>
        : std::true_type
    {
    };

    // ---- dimension and parameter members ------------------------------------

    // has_output_dimension
    //   helper: detects static constexpr output_dimension member.
    template<typename _Type,
             typename = void>
    struct has_output_dimension : std::false_type
    {
    };

    template<typename _Type>
    struct has_output_dimension<_Type,
                               void_t<decltype(_Type::output_dimension)>>
        : std::true_type
    {
    };

    // has_parameter_count
    //   helper: detects static constexpr parameter_count member.
    template<typename _Type,
             typename = void>
    struct has_parameter_count : std::false_type
    {
    };

    template<typename _Type>
    struct has_parameter_count<_Type,
                              void_t<decltype(_Type::parameter_count)>>
        : std::true_type
    {
    };

    // has_dimension
    //   helper: detects static constexpr dimension member.
    template<typename _Type,
             typename = void>
    struct has_dimension : std::false_type
    {
    };

    template<typename _Type>
    struct has_dimension<_Type,
                        void_t<decltype(_Type::dimension)>>
        : std::true_type
    {
    };

    // ---- type aliases -------------------------------------------------------

    // has_expression_type
    //   helper: detects expression_type member type.
    template<typename _Type,
             typename = void>
    struct has_expression_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_expression_type<_Type,
                              void_t<typename _Type::expression_type>>
        : std::true_type
    {
    };

    // has_domain_type
    //   helper: detects domain_type member type.
    template<typename _Type,
             typename = void>
    struct has_domain_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_domain_type<_Type,
                          void_t<typename _Type::domain_type>>
        : std::true_type
    {
    };

    // has_coordinate_system
    //   helper: detects coordinate_system member type.
    template<typename _Type,
             typename = void>
    struct has_coordinate_system : std::false_type
    {
    };

    template<typename _Type>
    struct has_coordinate_system<_Type,
                                void_t<typename _Type::coordinate_system>>
        : std::true_type
    {
    };

    // has_components_type
    //   helper: detects components_type member type (tuple of components).
    template<typename _Type,
             typename = void>
    struct has_components_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_components_type<_Type,
                              void_t<typename _Type::components_type>>
        : std::true_type
    {
    };

    // has_pieces_type
    //   helper: detects pieces_type member type (tuple of pieces).
    template<typename _Type,
             typename = void>
    struct has_pieces_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_pieces_type<_Type,
                          void_t<typename _Type::pieces_type>>
        : std::true_type
    {
    };

    // has_original_type
    //   helper: detects original_type member type (inverse functions).
    template<typename _Type,
             typename = void>
    struct has_original_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_original_type<_Type,
                            void_t<typename _Type::original_type>>
        : std::true_type
    {
    };

    // has_radius_type
    //   helper: detects radius_type member type (polar functions).
    template<typename _Type,
             typename = void>
    struct has_radius_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_radius_type<_Type,
                          void_t<typename _Type::radius_type>>
        : std::true_type
    {
    };

    // has_tangent_type
    //   helper: detects tangent member type (parametric curves).
    template<typename _Type,
             typename = void>
    struct has_tangent_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_tangent_type<_Type,
                           void_t<typename _Type::tangent>>
        : std::true_type
    {
    };

    // has_derivative_type
    //   helper: detects derivative or derivative_type member type.
    template<typename _Type,
             typename = void>
    struct has_derivative_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_derivative_type<_Type,
                              void_t<typename _Type::derivative>>
        : std::true_type
    {
    };

    // has_value_type
    //   helper: detects value_type member type.
    template<typename _Type,
             typename = void>
    struct has_value_type : std::false_type
    {
    };

    template<typename _Type>
    struct has_value_type<_Type,
                         void_t<typename _Type::value_type>>
        : std::true_type
    {
    };

    // ---- coordinate system booleans -----------------------------------------

    // has_is_cartesian
    //   helper: detects static constexpr is_cartesian member.
    template<typename _Type,
             typename = void>
    struct has_is_cartesian : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_cartesian<_Type,
                           void_t<decltype(_Type::is_cartesian)>>
        : std::true_type
    {
    };

    // has_is_polar
    //   helper: detects static constexpr is_polar member.
    template<typename _Type,
             typename = void>
    struct has_is_polar : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_polar<_Type,
                       void_t<decltype(_Type::is_polar)>>
        : std::true_type
    {
    };

    // has_is_cylindrical
    //   helper: detects static constexpr is_cylindrical member.
    template<typename _Type,
             typename = void>
    struct has_is_cylindrical : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_cylindrical<_Type,
                             void_t<decltype(_Type::is_cylindrical)>>
        : std::true_type
    {
    };

    // has_is_spherical
    //   helper: detects static constexpr is_spherical member.
    template<typename _Type,
             typename = void>
    struct has_is_spherical : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_spherical<_Type,
                           void_t<decltype(_Type::is_spherical)>>
        : std::true_type
    {
    };

NS_END  // internal


// ============================================================================
// II.   EVALUABLE FUNCTION DETECTION
// ============================================================================

NS_INTERNAL

    // evaluable_function_check
    //   helper: checks if type is an evaluable function (has evaluate
    // and arity).
    template<typename _Type,
             typename = void>
    struct evaluable_function_check : std::false_type
    {
    };

    template<typename _Type>
    struct evaluable_function_check<_Type, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_evaluable_function
//   trait: checks if _Type has both evaluate and arity.
// This is the broadest function detection: anything that can be
// called with arguments and reports its input count.
template<typename _Type>
struct is_evaluable_function : internal::evaluable_function_check<_Type>
{
};


// ============================================================================
// III.  FUNCTION CATEGORY DETECTION
// ============================================================================

// --- parametric --------------------------------------------------------------

NS_INTERNAL

    // parametric_check
    //   helper: structural check for parametric function.
    template<typename _Type,
             typename = void>
    struct parametric_check : std::false_type
    {
    };

    template<typename _Type>
    struct parametric_check<_Type, std::enable_if_t<
        ( has_is_parametric<_Type>::value   &&
          has_parameter_count<_Type>::value &&
          has_output_dimension<_Type>::value )
    >> : std::true_type
    {
    };

    // parametric_curve_check
    //   helper: parametric with parameter_count == 1.
    template<typename _Type,
             typename = void>
    struct parametric_curve_check : std::false_type
    {
    };

    template<typename _Type>
    struct parametric_curve_check<_Type, std::enable_if_t<
        ( parametric_check<_Type>::value &&
          (_Type::parameter_count == 1) )
    >> : std::true_type
    {
    };

    // parametric_surface_check
    //   helper: parametric with parameter_count == 2.
    template<typename _Type,
             typename = void>
    struct parametric_surface_check : std::false_type
    {
    };

    template<typename _Type>
    struct parametric_surface_check<_Type, std::enable_if_t<
        ( parametric_check<_Type>::value &&
          (_Type::parameter_count == 2) )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_parametric
//   trait: checks if _Type is a parametric function.
// A parametric function has is_parametric, parameter_count, and
// output_dimension static members.
template<typename _Type>
struct is_parametric : internal::parametric_check<_Type>
{
};

// is_parametric_curve
//   trait: checks if _Type is a parametric curve (1 parameter).
template<typename _Type>
struct is_parametric_curve : internal::parametric_curve_check<_Type>
{
};

// is_parametric_surface
//   trait: checks if _Type is a parametric surface (2 parameters).
template<typename _Type>
struct is_parametric_surface : internal::parametric_surface_check<_Type>
{
};

// --- piecewise ---------------------------------------------------------------

NS_INTERNAL

    // piecewise_check
    //   helper: structural check for piecewise function.
    template<typename _Type,
             typename = void>
    struct piecewise_check : std::false_type
    {
    };

    template<typename _Type>
    struct piecewise_check<_Type, std::enable_if_t<
        ( has_is_piecewise<_Type>::value &&
          has_pieces_type<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_piecewise
//   trait: checks if _Type is a piecewise-defined function.
// A piecewise function has is_piecewise and pieces_type members.
template<typename _Type>
struct is_piecewise : internal::piecewise_check<_Type>
{
};

// --- implicit ----------------------------------------------------------------

NS_INTERNAL

    // implicit_check
    //   helper: structural check for implicit function.
    template<typename _Type,
             typename = void>
    struct implicit_check : std::false_type
    {
    };

    template<typename _Type>
    struct implicit_check<_Type, std::enable_if_t<
        ( has_is_implicit<_Type>::value &&
          has_evaluate<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_implicit
//   trait: checks if _Type is an implicit function F(x,...) = 0.
// An implicit function has is_implicit and evaluate members.
template<typename _Type>
struct is_implicit : internal::implicit_check<_Type>
{
};

// --- inequality / constraint -------------------------------------------------

NS_INTERNAL

    // inequality_check
    //   helper: structural check for inequality expression.
    template<typename _Type,
             typename = void>
    struct inequality_check : std::false_type
    {
    };

    template<typename _Type>
    struct inequality_check<_Type, std::enable_if_t<
        ( has_is_inequality<_Type>::value &&
          has_evaluate<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_inequality
//   trait: checks if _Type is an inequality/constraint expression.
// An inequality has is_inequality and evaluate members.
template<typename _Type>
struct is_inequality : internal::inequality_check<_Type>
{
};

// --- vector-valued -----------------------------------------------------------

NS_INTERNAL

    // vector_valued_check
    //   helper: structural check for vector-valued function.
    template<typename _Type,
             typename = void>
    struct vector_valued_check : std::false_type
    {
    };

    template<typename _Type>
    struct vector_valued_check<_Type, std::enable_if_t<
        ( has_is_vector_valued<_Type>::value &&
          has_output_dimension<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_vector_valued
//   trait: checks if _Type is a vector-valued function (output_dimension > 1).
// A vector-valued function has is_vector_valued and output_dimension.
template<typename _Type>
struct is_vector_valued : internal::vector_valued_check<_Type>
{
};

// --- scalar-valued -----------------------------------------------------------

NS_INTERNAL

    // scalar_valued_check
    //   helper: evaluable function that is NOT vector-valued.
    template<typename _Type,
             typename = void>
    struct scalar_valued_check : std::false_type
    {
    };

    template<typename _Type>
    struct scalar_valued_check<_Type, std::enable_if_t<
        ( evaluable_function_check<_Type>::value &&
          !has_is_vector_valued<_Type>::value     &&
          !has_output_dimension<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_scalar_valued
//   trait: checks if _Type is a scalar-valued function.
// True when the type is evaluable but has no output_dimension or
// is_vector_valued marker (i.e. returns a single scalar).
template<typename _Type>
struct is_scalar_valued : internal::scalar_valued_check<_Type>
{
};

// --- inverse -----------------------------------------------------------------

NS_INTERNAL

    // inverse_check
    //   helper: structural check for inverse function marker.
    template<typename _Type,
             typename = void>
    struct inverse_check : std::false_type
    {
    };

    template<typename _Type>
    struct inverse_check<_Type, std::enable_if_t<
        ( has_is_inverse<_Type>::value   &&
          has_original_type<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_inverse
//   trait: checks if _Type is an inverse function marker.
// An inverse has is_inverse and original_type members.
template<typename _Type>
struct is_inverse : internal::inverse_check<_Type>
{
};

// --- polar-form --------------------------------------------------------------

NS_INTERNAL

    // polar_form_check
    //   helper: structural check for polar-form function.
    template<typename _Type,
             typename = void>
    struct polar_form_check : std::false_type
    {
    };

    template<typename _Type>
    struct polar_form_check<_Type, std::enable_if_t<
        ( has_is_polar_form<_Type>::value &&
          has_radius_type<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_polar_form
//   trait: checks if _Type is a polar-form function r = f(θ).
// A polar-form function has is_polar_form and radius_type members.
template<typename _Type>
struct is_polar_form : internal::polar_form_check<_Type>
{
};

// --- named / wrapped function ------------------------------------------------

NS_INTERNAL

    // named_function_check
    //   helper: structural check for math_function wrapper.
    template<typename _Type,
             typename = void>
    struct named_function_check : std::false_type
    {
    };

    template<typename _Type>
    struct named_function_check<_Type, std::enable_if_t<
        ( has_expression_type<_Type>::value &&
          has_domain_type<_Type>::value     &&
          has_coordinate_system<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_named_function
//   trait: checks if _Type is a math_function wrapper.
// A named function has expression_type, domain_type, and
// coordinate_system member types.
template<typename _Type>
struct is_named_function : internal::named_function_check<_Type>
{
};


// ============================================================================
// IV.   ARITY DETECTION
// ============================================================================

NS_INTERNAL

    // multivariable_check
    //   helper: checks if function has arity > 1.
    template<typename _Type,
             typename = void>
    struct multivariable_check : std::false_type
    {
    };

    template<typename _Type>
    struct multivariable_check<_Type, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value    &&
          (_Type::arity > 1) )
    >> : std::true_type
    {
    };

    // nullary_check
    //   helper: checks if function has arity == 0.
    template<typename _Type,
             typename = void>
    struct nullary_check : std::false_type
    {
    };

    template<typename _Type>
    struct nullary_check<_Type, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value    &&
          (_Type::arity == 0) )
    >> : std::true_type
    {
    };

    // unary_check
    //   helper: checks if function has arity == 1.
    template<typename _Type,
             typename = void>
    struct unary_check : std::false_type
    {
    };

    template<typename _Type>
    struct unary_check<_Type, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value    &&
          (_Type::arity == 1) )
    >> : std::true_type
    {
    };

    // binary_check
    //   helper: checks if function has arity == 2.
    template<typename _Type,
             typename = void>
    struct binary_check : std::false_type
    {
    };

    template<typename _Type>
    struct binary_check<_Type, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value    &&
          (_Type::arity == 2) )
    >> : std::true_type
    {
    };

    // ternary_check
    //   helper: checks if function has arity == 3.
    template<typename _Type,
             typename = void>
    struct ternary_check : std::false_type
    {
    };

    template<typename _Type>
    struct ternary_check<_Type, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value    &&
          (_Type::arity == 3) )
    >> : std::true_type
    {
    };

    // n_ary_check
    //   helper: checks if function has a specific arity.
    template<typename    _Type,
             std::size_t _N,
             typename = void>
    struct n_ary_check : std::false_type
    {
    };

    template<typename    _Type,
             std::size_t _N>
    struct n_ary_check<_Type, _N, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value    &&
          (_Type::arity == _N) )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_multivariable
//   trait: checks if _Type is a multivariable function (arity > 1).
template<typename _Type>
struct is_multivariable : internal::multivariable_check<_Type>
{
};

// is_nullary_function
//   trait: checks if _Type has arity 0 (constant function).
template<typename _Type>
struct is_nullary_function : internal::nullary_check<_Type>
{
};

// is_unary_function
//   trait: checks if _Type has arity 1.
template<typename _Type>
struct is_unary_function : internal::unary_check<_Type>
{
};

// is_binary_function
//   trait: checks if _Type has arity 2.
template<typename _Type>
struct is_binary_function : internal::binary_check<_Type>
{
};

// is_ternary_function
//   trait: checks if _Type has arity 3.
template<typename _Type>
struct is_ternary_function : internal::ternary_check<_Type>
{
};

// is_n_ary_function
//   trait: checks if _Type has a specific arity _N.
template<typename    _Type,
         std::size_t _N>
struct is_n_ary_function : internal::n_ary_check<_Type, _N>
{
};


// ============================================================================
// V.    COORDINATE SYSTEM DETECTION
// ============================================================================

NS_INTERNAL

    // coord_system_check
    //   helper: checks if type IS a coordinate system (has dimension
    // and at least one of the four coordinate boolean flags).
    template<typename _Type,
             typename = void>
    struct coord_system_check : std::false_type
    {
    };

    template<typename _Type>
    struct coord_system_check<_Type, std::enable_if_t<
        ( has_dimension<_Type>::value       &&
          has_is_cartesian<_Type>::value     &&
          has_is_polar<_Type>::value         &&
          has_is_cylindrical<_Type>::value   &&
          has_is_spherical<_Type>::value )
    >> : std::true_type
    {
    };

    // cartesian_check
    //   helper: checks if type is a Cartesian coordinate system.
    template<typename _Type,
             typename = void>
    struct cartesian_check : std::false_type
    {
    };

    template<typename _Type>
    struct cartesian_check<_Type, std::enable_if_t<
        ( coord_system_check<_Type>::value &&
          _Type::is_cartesian )
    >> : std::true_type
    {
    };

    // polar_check
    //   helper: checks if type is a polar coordinate system.
    template<typename _Type,
             typename = void>
    struct polar_check : std::false_type
    {
    };

    template<typename _Type>
    struct polar_check<_Type, std::enable_if_t<
        ( coord_system_check<_Type>::value &&
          _Type::is_polar )
    >> : std::true_type
    {
    };

    // cylindrical_check
    //   helper: checks if type is a cylindrical coordinate system.
    template<typename _Type,
             typename = void>
    struct cylindrical_check : std::false_type
    {
    };

    template<typename _Type>
    struct cylindrical_check<_Type, std::enable_if_t<
        ( coord_system_check<_Type>::value &&
          _Type::is_cylindrical )
    >> : std::true_type
    {
    };

    // spherical_check
    //   helper: checks if type is a spherical coordinate system.
    template<typename _Type,
             typename = void>
    struct spherical_check : std::false_type
    {
    };

    template<typename _Type>
    struct spherical_check<_Type, std::enable_if_t<
        ( coord_system_check<_Type>::value &&
          _Type::is_spherical )
    >> : std::true_type
    {
    };

    // --- coordinate system of a function (via coordinate_system alias) -------

    // function_uses_cartesian_check
    //   helper: checks if a function's coordinate_system is Cartesian.
    template<typename _Type,
             typename = void>
    struct function_uses_cartesian_check : std::false_type
    {
    };

    template<typename _Type>
    struct function_uses_cartesian_check<_Type, std::enable_if_t<
        ( has_coordinate_system<_Type>::value &&
          cartesian_check<typename _Type::coordinate_system>::value )
    >> : std::true_type
    {
    };

    // function_uses_polar_check
    //   helper: checks if a function's coordinate_system is polar.
    template<typename _Type,
             typename = void>
    struct function_uses_polar_check : std::false_type
    {
    };

    template<typename _Type>
    struct function_uses_polar_check<_Type, std::enable_if_t<
        ( has_coordinate_system<_Type>::value &&
          polar_check<typename _Type::coordinate_system>::value )
    >> : std::true_type
    {
    };

    // function_uses_cylindrical_check
    //   helper: checks if a function's coordinate_system is cylindrical.
    template<typename _Type,
             typename = void>
    struct function_uses_cylindrical_check : std::false_type
    {
    };

    template<typename _Type>
    struct function_uses_cylindrical_check<_Type, std::enable_if_t<
        ( has_coordinate_system<_Type>::value &&
          cylindrical_check<typename _Type::coordinate_system>::value )
    >> : std::true_type
    {
    };

    // function_uses_spherical_check
    //   helper: checks if a function's coordinate_system is spherical.
    template<typename _Type,
             typename = void>
    struct function_uses_spherical_check : std::false_type
    {
    };

    template<typename _Type>
    struct function_uses_spherical_check<_Type, std::enable_if_t<
        ( has_coordinate_system<_Type>::value &&
          spherical_check<typename _Type::coordinate_system>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// ---- coordinate system type traits ------------------------------------------

// is_coordinate_system
//   trait: checks if _Type is a coordinate system definition.
template<typename _Type>
struct is_coordinate_system : internal::coord_system_check<_Type>
{
};

// is_cartesian_system
//   trait: checks if _Type is a Cartesian coordinate system.
template<typename _Type>
struct is_cartesian_system : internal::cartesian_check<_Type>
{
};

// is_polar_system
//   trait: checks if _Type is a polar coordinate system.
template<typename _Type>
struct is_polar_system : internal::polar_check<_Type>
{
};

// is_cylindrical_system
//   trait: checks if _Type is a cylindrical coordinate system.
template<typename _Type>
struct is_cylindrical_system : internal::cylindrical_check<_Type>
{
};

// is_spherical_system
//   trait: checks if _Type is a spherical coordinate system.
template<typename _Type>
struct is_spherical_system : internal::spherical_check<_Type>
{
};

// ---- function coordinate usage traits ---------------------------------------

// uses_cartesian
//   trait: checks if function _Type uses Cartesian coordinates.
template<typename _Type>
struct uses_cartesian : internal::function_uses_cartesian_check<_Type>
{
};

// uses_polar
//   trait: checks if function _Type uses polar coordinates.
template<typename _Type>
struct uses_polar : internal::function_uses_polar_check<_Type>
{
};

// uses_cylindrical
//   trait: checks if function _Type uses cylindrical coordinates.
template<typename _Type>
struct uses_cylindrical : internal::function_uses_cylindrical_check<_Type>
{
};

// uses_spherical
//   trait: checks if function _Type uses spherical coordinates.
template<typename _Type>
struct uses_spherical : internal::function_uses_spherical_check<_Type>
{
};


// ============================================================================
// VI.   DOMAIN AND DIFFERENTIABILITY DETECTION
// ============================================================================

NS_INTERNAL

    // has_bounded_domain_check
    //   helper: checks if function has a non-void domain type.
    template<typename _Type,
             typename = void>
    struct has_bounded_domain_check : std::false_type
    {
    };

    template<typename _Type>
    struct has_bounded_domain_check<_Type, std::enable_if_t<
        ( has_domain_type<_Type>::value &&
          !std::is_void<typename _Type::domain_type>::value )
    >> : std::true_type
    {
    };

    // is_differentiable_check
    //   helper: checks if function provides a derivative type.
    template<typename _Type,
             typename = void>
    struct is_differentiable_check : std::false_type
    {
    };

    template<typename _Type>
    struct is_differentiable_check<_Type, std::enable_if_t<
        has_derivative_type<_Type>::value
    >> : std::true_type
    {
    };

    // is_constant_expression_check
    //   helper: checks if function is a constant expression.
    template<typename _Type,
             typename = void>
    struct is_constant_expression_check : std::false_type
    {
    };

    template<typename _Type>
    struct is_constant_expression_check<_Type, std::enable_if_t<
        ( has_is_constant_expr<_Type>::value &&
          _Type::is_constant_expr )
    >> : std::true_type
    {
    };

    // has_components_check
    //   helper: checks if type has extractable components.
    template<typename _Type,
             typename = void>
    struct has_components_check : std::false_type
    {
    };

    template<typename _Type>
    struct has_components_check<_Type, std::enable_if_t<
        ( has_components_type<_Type>::value &&
          has_output_dimension<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// has_bounded_domain
//   trait: checks if _Type has a non-void domain type.
template<typename _Type>
struct has_bounded_domain : internal::has_bounded_domain_check<_Type>
{
};

// is_differentiable
//   trait: checks if _Type provides its derivative as a nested type.
template<typename _Type>
struct is_differentiable : internal::is_differentiable_check<_Type>
{
};

// is_constant_expression
//   trait: checks if _Type is a compile-time constant expression.
template<typename _Type>
struct is_constant_expression
    : internal::is_constant_expression_check<_Type>
{
};

// has_components
//   trait: checks if _Type has extractable components (components_type
// and output_dimension).
template<typename _Type>
struct has_components : internal::has_components_check<_Type>
{
};


// ============================================================================
// VII.  OUTPUT DIMENSION DETECTION
// ============================================================================

NS_INTERNAL

    // scalar_output_check
    //   helper: output_dimension == 1 or no output_dimension (scalar).
    template<typename _Type,
             typename = void>
    struct scalar_output_check : std::false_type
    {
    };

    // evaluable but no output_dimension -> scalar
    template<typename _Type>
    struct scalar_output_check<_Type, std::enable_if_t<
        ( evaluable_function_check<_Type>::value &&
          !has_output_dimension<_Type>::value )
    >> : std::true_type
    {
    };

    // has output_dimension == 1 -> also scalar
    template<typename _Type,
             typename = void>
    struct explicit_scalar_check : std::false_type
    {
    };

    template<typename _Type>
    struct explicit_scalar_check<_Type, std::enable_if_t<
        ( has_output_dimension<_Type>::value &&
          (_Type::output_dimension == 1) )
    >> : std::true_type
    {
    };

    // 2d_output_check
    //   helper: output_dimension == 2.
    template<typename _Type,
             typename = void>
    struct output_2d_check : std::false_type
    {
    };

    template<typename _Type>
    struct output_2d_check<_Type, std::enable_if_t<
        ( has_output_dimension<_Type>::value &&
          (_Type::output_dimension == 2) )
    >> : std::true_type
    {
    };

    // 3d_output_check
    //   helper: output_dimension == 3.
    template<typename _Type,
             typename = void>
    struct output_3d_check : std::false_type
    {
    };

    template<typename _Type>
    struct output_3d_check<_Type, std::enable_if_t<
        ( has_output_dimension<_Type>::value &&
          (_Type::output_dimension == 3) )
    >> : std::true_type
    {
    };

    // n_dimensional_output_check
    //   helper: output_dimension == _N.
    template<typename    _Type,
             std::size_t _N,
             typename = void>
    struct n_dimensional_output_check : std::false_type
    {
    };

    template<typename    _Type,
             std::size_t _N>
    struct n_dimensional_output_check<_Type, _N, std::enable_if_t<
        ( has_output_dimension<_Type>::value &&
          (_Type::output_dimension == _N) )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_scalar_output
//   trait: checks if _Type produces a scalar output (no output_dimension
// member, or output_dimension == 1).
template<typename _Type>
struct is_scalar_output
    : std::bool_constant<
          internal::scalar_output_check<_Type>::value ||
          internal::explicit_scalar_check<_Type>::value
      >
{
};

// is_2d_output
//   trait: checks if _Type produces 2-dimensional output.
template<typename _Type>
struct is_2d_output : internal::output_2d_check<_Type>
{
};

// is_3d_output
//   trait: checks if _Type produces 3-dimensional output.
template<typename _Type>
struct is_3d_output : internal::output_3d_check<_Type>
{
};

// is_nd_output
//   trait: checks if _Type produces _N-dimensional output.
template<typename    _Type,
         std::size_t _N>
struct is_nd_output : internal::n_dimensional_output_check<_Type, _N>
{
};


// ============================================================================
// VIII. FUNCTION RELATIONSHIP TRAITS
// ============================================================================

NS_INTERNAL

    // same_arity_check
    //   helper: checks if two functions have the same arity.
    template<typename _F,
             typename _G,
             typename = void>
    struct same_arity_check : std::false_type
    {
    };

    template<typename _F,
             typename _G>
    struct same_arity_check<_F, _G, std::enable_if_t<
        ( has_arity<_F>::value &&
          has_arity<_G>::value &&
          (_F::arity == _G::arity) )
    >> : std::true_type
    {
    };

    // same_coordinate_system_check
    //   helper: checks if two functions use the same coordinate system.
    template<typename _F,
             typename _G,
             typename = void>
    struct same_coordinate_system_check : std::false_type
    {
    };

    template<typename _F,
             typename _G>
    struct same_coordinate_system_check<_F, _G, std::enable_if_t<
        ( has_coordinate_system<_F>::value                        &&
          has_coordinate_system<_G>::value                        &&
          std::is_same<typename _F::coordinate_system,
                       typename _G::coordinate_system>::value )
    >> : std::true_type
    {
    };

    // composable_check
    //   helper: checks if f(g(x)) is well-formed.
    // _F's arity must be 1 (takes one argument), OR _F must accept
    // the output dimension of _G. For scalar-output _G, _F must
    // accept a scalar. For vector-output _G, _F must accept a
    // vector of matching dimension.
    template<typename _F,
             typename _G,
             typename = void>
    struct composable_check : std::false_type
    {
    };

    // scalar g -> unary f: composable
    template<typename _F,
             typename _G>
    struct composable_check<_F, _G, std::enable_if_t<
        ( has_evaluate<_F>::value              &&
          has_evaluate<_G>::value              &&
          has_arity<_F>::value                 &&
          (_F::arity == 1)                     &&
          !has_output_dimension<_G>::value )
    >> : std::true_type
    {
    };

    // vector g with matching output_dimension -> f with matching arity
    template<typename _F,
             typename _G,
             typename = void>
    struct composable_vector_check : std::false_type
    {
    };

    template<typename _F,
             typename _G>
    struct composable_vector_check<_F, _G, std::enable_if_t<
        ( has_evaluate<_F>::value             &&
          has_evaluate<_G>::value             &&
          has_arity<_F>::value                &&
          has_output_dimension<_G>::value     &&
          (_F::arity == _G::output_dimension) )
    >> : std::true_type
    {
    };

    // same_value_type_check
    //   helper: checks if two functions share the same value_type.
    template<typename _F,
             typename _G,
             typename = void>
    struct same_value_type_check : std::false_type
    {
    };

    template<typename _F,
             typename _G>
    struct same_value_type_check<_F, _G, std::enable_if_t<
        ( has_value_type<_F>::value                           &&
          has_value_type<_G>::value                           &&
          std::is_same<typename _F::value_type,
                       typename _G::value_type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// functions_same_arity
//   trait: checks if _F and _G have the same arity.
template<typename _F,
         typename _G>
struct functions_same_arity : internal::same_arity_check<_F, _G>
{
};

// functions_same_coordinate_system
//   trait: checks if _F and _G use the same coordinate system.
template<typename _F,
         typename _G>
struct functions_same_coordinate_system
    : internal::same_coordinate_system_check<_F, _G>
{
};

// is_composable
//   trait: checks if f(g(x)) is structurally well-formed.
// True when _G produces scalar output and _F is unary, OR when
// _G's output_dimension matches _F's arity.
template<typename _F,
         typename _G>
struct is_composable
    : std::bool_constant<
          internal::composable_check<_F, _G>::value ||
          internal::composable_vector_check<_F, _G>::value
      >
{
};

// functions_same_value_type
//   trait: checks if _F and _G have the same value_type.
template<typename _F,
         typename _G>
struct functions_same_value_type
    : internal::same_value_type_check<_F, _G>
{
};


// ============================================================================
// IX.   TYPE EXTRACTION METAFUNCTIONS
// ============================================================================

NS_INTERNAL

    // function_value_type_helper
    //   helper: extracts value_type from a function type.
    template<typename _Type,
             typename = void>
    struct function_value_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_value_type_helper<_Type,
                                     void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    // function_expression_type_helper
    //   helper: extracts expression_type from a named function.
    template<typename _Type,
             typename = void>
    struct function_expression_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_expression_type_helper<_Type,
        void_t<typename _Type::expression_type>>
    {
        using type = typename _Type::expression_type;
    };

    // function_domain_type_helper
    //   helper: extracts domain_type from a named function.
    template<typename _Type,
             typename = void>
    struct function_domain_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_domain_type_helper<_Type,
                                      void_t<typename _Type::domain_type>>
    {
        using type = typename _Type::domain_type;
    };

    // function_coordinate_system_helper
    //   helper: extracts coordinate_system from a function.
    template<typename _Type,
             typename = void>
    struct function_coordinate_system_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_coordinate_system_helper<_Type,
        void_t<typename _Type::coordinate_system>>
    {
        using type = typename _Type::coordinate_system;
    };

    // function_components_type_helper
    //   helper: extracts components_type tuple.
    template<typename _Type,
             typename = void>
    struct function_components_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_components_type_helper<_Type,
        void_t<typename _Type::components_type>>
    {
        using type = typename _Type::components_type;
    };

    // function_pieces_type_helper
    //   helper: extracts pieces_type tuple.
    template<typename _Type,
             typename = void>
    struct function_pieces_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_pieces_type_helper<_Type,
                                      void_t<typename _Type::pieces_type>>
    {
        using type = typename _Type::pieces_type;
    };

    // function_original_type_helper
    //   helper: extracts original_type from an inverse function.
    template<typename _Type,
             typename = void>
    struct function_original_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_original_type_helper<_Type,
        void_t<typename _Type::original_type>>
    {
        using type = typename _Type::original_type;
    };

    // function_derivative_type_helper
    //   helper: extracts derivative type.
    template<typename _Type,
             typename = void>
    struct function_derivative_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct function_derivative_type_helper<_Type,
        void_t<typename _Type::derivative>>
    {
        using type = typename _Type::derivative;
    };

NS_END  // internal

// function_value_type
//   trait: extracts the value type from a function type.
template<typename _Type>
struct function_value_type
{
    using type = typename internal::function_value_type_helper<_Type>::type;
};

// function_value_type_t
//   type: shorthand for function_value_type<_Type>::type.
template<typename _Type>
using function_value_type_t =
    typename function_value_type<_Type>::type;

// function_expression_type
//   trait: extracts the expression type from a named function.
template<typename _Type>
struct function_expression_type
{
    using type =
        typename internal::function_expression_type_helper<_Type>::type;
};

// function_expression_type_t
//   type: shorthand for function_expression_type<_Type>::type.
template<typename _Type>
using function_expression_type_t =
    typename function_expression_type<_Type>::type;

// function_domain_type
//   trait: extracts the domain type from a named function.
template<typename _Type>
struct function_domain_type
{
    using type =
        typename internal::function_domain_type_helper<_Type>::type;
};

// function_domain_type_t
//   type: shorthand for function_domain_type<_Type>::type.
template<typename _Type>
using function_domain_type_t =
    typename function_domain_type<_Type>::type;

// function_coordinate_system
//   trait: extracts the coordinate system from a function.
template<typename _Type>
struct function_coordinate_system
{
    using type =
        typename internal::function_coordinate_system_helper<_Type>::type;
};

// function_coordinate_system_t
//   type: shorthand for function_coordinate_system<_Type>::type.
template<typename _Type>
using function_coordinate_system_t =
    typename function_coordinate_system<_Type>::type;

// function_components_type
//   trait: extracts the components tuple from a vector/parametric function.
template<typename _Type>
struct function_components_type
{
    using type =
        typename internal::function_components_type_helper<_Type>::type;
};

// function_components_type_t
//   type: shorthand for function_components_type<_Type>::type.
template<typename _Type>
using function_components_type_t =
    typename function_components_type<_Type>::type;

// function_pieces_type
//   trait: extracts the pieces tuple from a piecewise function.
template<typename _Type>
struct function_pieces_type
{
    using type =
        typename internal::function_pieces_type_helper<_Type>::type;
};

// function_pieces_type_t
//   type: shorthand for function_pieces_type<_Type>::type.
template<typename _Type>
using function_pieces_type_t =
    typename function_pieces_type<_Type>::type;

// function_original_type
//   trait: extracts the original type from an inverse function.
template<typename _Type>
struct function_original_type
{
    using type =
        typename internal::function_original_type_helper<_Type>::type;
};

// function_original_type_t
//   type: shorthand for function_original_type<_Type>::type.
template<typename _Type>
using function_original_type_t =
    typename function_original_type<_Type>::type;

// function_derivative_type
//   trait: extracts the derivative type from a function.
template<typename _Type>
struct function_derivative_type
{
    using type =
        typename internal::function_derivative_type_helper<_Type>::type;
};

// function_derivative_type_t
//   type: shorthand for function_derivative_type<_Type>::type.
template<typename _Type>
using function_derivative_type_t =
    typename function_derivative_type<_Type>::type;


// ============================================================================
// X.    FUNCTION CLASSIFICATION STRUCT
// ============================================================================

// function_class
//   struct: aggregate classification of a function type.
// Provides a single point of query for all function properties.
template<typename _Type>
struct function_class
{
    // ---- evaluability -------------------------------------------------------

    static constexpr bool is_evaluable =
        is_evaluable_function<_Type>::value;

    // ---- category -----------------------------------------------------------

    static constexpr bool parametric   = is_parametric<_Type>::value;
    static constexpr bool curve        = is_parametric_curve<_Type>::value;
    static constexpr bool surface      = is_parametric_surface<_Type>::value;
    static constexpr bool piecewise    = is_piecewise<_Type>::value;
    static constexpr bool implicit     = is_implicit<_Type>::value;
    static constexpr bool inequality   = is_inequality<_Type>::value;
    static constexpr bool vector_valued = is_vector_valued<_Type>::value;
    static constexpr bool scalar_valued = is_scalar_valued<_Type>::value;
    static constexpr bool inverse      = is_inverse<_Type>::value;
    static constexpr bool polar_form   = is_polar_form<_Type>::value;
    static constexpr bool named        = is_named_function<_Type>::value;

    // ---- arity --------------------------------------------------------------

    static constexpr bool multivariable = is_multivariable<_Type>::value;
    static constexpr bool nullary       = is_nullary_function<_Type>::value;
    static constexpr bool unary         = is_unary_function<_Type>::value;
    static constexpr bool binary        = is_binary_function<_Type>::value;
    static constexpr bool ternary       = is_ternary_function<_Type>::value;

    // ---- output -------------------------------------------------------------

    static constexpr bool scalar_output = is_scalar_output<_Type>::value;
    static constexpr bool output_2d     = is_2d_output<_Type>::value;
    static constexpr bool output_3d     = is_3d_output<_Type>::value;

    // ---- coordinate system --------------------------------------------------

    static constexpr bool has_coords    =
        internal::has_coordinate_system<_Type>::value;
    static constexpr bool cartesian     = uses_cartesian<_Type>::value;
    static constexpr bool polar         = uses_polar<_Type>::value;
    static constexpr bool cylindrical   = uses_cylindrical<_Type>::value;
    static constexpr bool spherical     = uses_spherical<_Type>::value;

    // ---- properties ---------------------------------------------------------

    static constexpr bool has_domain       =
        has_bounded_domain<_Type>::value;
    static constexpr bool differentiable   =
        is_differentiable<_Type>::value;
    static constexpr bool constant_expr    =
        is_constant_expression<_Type>::value;
    static constexpr bool has_comps        =
        has_components<_Type>::value;
    static constexpr bool has_tangent      =
        internal::has_tangent_type<_Type>::value;

    // ---- type extraction (void if not present) ------------------------------

    using value_type         = function_value_type_t<_Type>;
    using expression_type    = function_expression_type_t<_Type>;
    using domain_type        = function_domain_type_t<_Type>;
    using coordinate_system  = function_coordinate_system_t<_Type>;
    using components_type    = function_components_type_t<_Type>;
    using pieces_type        = function_pieces_type_t<_Type>;
    using derivative_type    = function_derivative_type_t<_Type>;
};


// ============================================================================
// XI.   VARIABLE TEMPLATES
// ============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ---- evaluability -------------------------------------------------------

    // is_evaluable_function_v
    //   variable template: value helper for is_evaluable_function.
    template<typename _Type>
    inline constexpr bool is_evaluable_function_v =
        is_evaluable_function<_Type>::value;

    // ---- category -----------------------------------------------------------

    // is_parametric_v
    //   variable template: value helper for is_parametric.
    template<typename _Type>
    inline constexpr bool is_parametric_v =
        is_parametric<_Type>::value;

    // is_parametric_curve_v
    //   variable template: value helper for is_parametric_curve.
    template<typename _Type>
    inline constexpr bool is_parametric_curve_v =
        is_parametric_curve<_Type>::value;

    // is_parametric_surface_v
    //   variable template: value helper for is_parametric_surface.
    template<typename _Type>
    inline constexpr bool is_parametric_surface_v =
        is_parametric_surface<_Type>::value;

    // is_piecewise_v
    //   variable template: value helper for is_piecewise.
    template<typename _Type>
    inline constexpr bool is_piecewise_v =
        is_piecewise<_Type>::value;

    // is_implicit_v
    //   variable template: value helper for is_implicit.
    template<typename _Type>
    inline constexpr bool is_implicit_v =
        is_implicit<_Type>::value;

    // is_inequality_v
    //   variable template: value helper for is_inequality.
    template<typename _Type>
    inline constexpr bool is_inequality_v =
        is_inequality<_Type>::value;

    // is_vector_valued_v
    //   variable template: value helper for is_vector_valued.
    template<typename _Type>
    inline constexpr bool is_vector_valued_v =
        is_vector_valued<_Type>::value;

    // is_scalar_valued_v
    //   variable template: value helper for is_scalar_valued.
    template<typename _Type>
    inline constexpr bool is_scalar_valued_v =
        is_scalar_valued<_Type>::value;

    // is_inverse_v
    //   variable template: value helper for is_inverse.
    template<typename _Type>
    inline constexpr bool is_inverse_v =
        is_inverse<_Type>::value;

    // is_polar_form_v
    //   variable template: value helper for is_polar_form.
    template<typename _Type>
    inline constexpr bool is_polar_form_v =
        is_polar_form<_Type>::value;

    // is_named_function_v
    //   variable template: value helper for is_named_function.
    template<typename _Type>
    inline constexpr bool is_named_function_v =
        is_named_function<_Type>::value;

    // ---- arity --------------------------------------------------------------

    // is_multivariable_v
    //   variable template: value helper for is_multivariable.
    template<typename _Type>
    inline constexpr bool is_multivariable_v =
        is_multivariable<_Type>::value;

    // is_nullary_function_v
    //   variable template: value helper for is_nullary_function.
    template<typename _Type>
    inline constexpr bool is_nullary_function_v =
        is_nullary_function<_Type>::value;

    // is_unary_function_v
    //   variable template: value helper for is_unary_function.
    template<typename _Type>
    inline constexpr bool is_unary_function_v =
        is_unary_function<_Type>::value;

    // is_binary_function_v
    //   variable template: value helper for is_binary_function.
    template<typename _Type>
    inline constexpr bool is_binary_function_v =
        is_binary_function<_Type>::value;

    // is_ternary_function_v
    //   variable template: value helper for is_ternary_function.
    template<typename _Type>
    inline constexpr bool is_ternary_function_v =
        is_ternary_function<_Type>::value;

    // is_n_ary_function_v
    //   variable template: value helper for is_n_ary_function.
    template<typename    _Type,
             std::size_t _N>
    inline constexpr bool is_n_ary_function_v =
        is_n_ary_function<_Type, _N>::value;

    // ---- coordinate system --------------------------------------------------

    // is_coordinate_system_v
    //   variable template: value helper for is_coordinate_system.
    template<typename _Type>
    inline constexpr bool is_coordinate_system_v =
        is_coordinate_system<_Type>::value;

    // is_cartesian_system_v
    //   variable template: value helper for is_cartesian_system.
    template<typename _Type>
    inline constexpr bool is_cartesian_system_v =
        is_cartesian_system<_Type>::value;

    // is_polar_system_v
    //   variable template: value helper for is_polar_system.
    template<typename _Type>
    inline constexpr bool is_polar_system_v =
        is_polar_system<_Type>::value;

    // is_cylindrical_system_v
    //   variable template: value helper for is_cylindrical_system.
    template<typename _Type>
    inline constexpr bool is_cylindrical_system_v =
        is_cylindrical_system<_Type>::value;

    // is_spherical_system_v
    //   variable template: value helper for is_spherical_system.
    template<typename _Type>
    inline constexpr bool is_spherical_system_v =
        is_spherical_system<_Type>::value;

    // uses_cartesian_v
    //   variable template: value helper for uses_cartesian.
    template<typename _Type>
    inline constexpr bool uses_cartesian_v =
        uses_cartesian<_Type>::value;

    // uses_polar_v
    //   variable template: value helper for uses_polar.
    template<typename _Type>
    inline constexpr bool uses_polar_v =
        uses_polar<_Type>::value;

    // uses_cylindrical_v
    //   variable template: value helper for uses_cylindrical.
    template<typename _Type>
    inline constexpr bool uses_cylindrical_v =
        uses_cylindrical<_Type>::value;

    // uses_spherical_v
    //   variable template: value helper for uses_spherical.
    template<typename _Type>
    inline constexpr bool uses_spherical_v =
        uses_spherical<_Type>::value;

    // ---- output dimension ---------------------------------------------------

    // is_scalar_output_v
    //   variable template: value helper for is_scalar_output.
    template<typename _Type>
    inline constexpr bool is_scalar_output_v =
        is_scalar_output<_Type>::value;

    // is_2d_output_v
    //   variable template: value helper for is_2d_output.
    template<typename _Type>
    inline constexpr bool is_2d_output_v =
        is_2d_output<_Type>::value;

    // is_3d_output_v
    //   variable template: value helper for is_3d_output.
    template<typename _Type>
    inline constexpr bool is_3d_output_v =
        is_3d_output<_Type>::value;

    // is_nd_output_v
    //   variable template: value helper for is_nd_output.
    template<typename    _Type,
             std::size_t _N>
    inline constexpr bool is_nd_output_v =
        is_nd_output<_Type, _N>::value;

    // ---- properties ---------------------------------------------------------

    // has_bounded_domain_v
    //   variable template: value helper for has_bounded_domain.
    template<typename _Type>
    inline constexpr bool has_bounded_domain_v =
        has_bounded_domain<_Type>::value;

    // is_differentiable_v
    //   variable template: value helper for is_differentiable.
    template<typename _Type>
    inline constexpr bool is_differentiable_v =
        is_differentiable<_Type>::value;

    // is_constant_expression_v
    //   variable template: value helper for is_constant_expression.
    template<typename _Type>
    inline constexpr bool is_constant_expression_v =
        is_constant_expression<_Type>::value;

    // has_components_v
    //   variable template: value helper for has_components.
    template<typename _Type>
    inline constexpr bool has_components_v =
        has_components<_Type>::value;

    // ---- relationships ------------------------------------------------------

    // functions_same_arity_v
    //   variable template: value helper for functions_same_arity.
    template<typename _F,
             typename _G>
    inline constexpr bool functions_same_arity_v =
        functions_same_arity<_F, _G>::value;

    // functions_same_coordinate_system_v
    //   variable template: value helper for functions_same_coordinate_system.
    template<typename _F,
             typename _G>
    inline constexpr bool functions_same_coordinate_system_v =
        functions_same_coordinate_system<_F, _G>::value;

    // is_composable_v
    //   variable template: value helper for is_composable.
    template<typename _F,
             typename _G>
    inline constexpr bool is_composable_v =
        is_composable<_F, _G>::value;

    // functions_same_value_type_v
    //   variable template: value helper for functions_same_value_type.
    template<typename _F,
             typename _G>
    inline constexpr bool functions_same_value_type_v =
        functions_same_value_type<_F, _G>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // maths
NS_END  // djinterp


#endif  // DJINTERP_MATHS_FUNCTION_TRAITS_
