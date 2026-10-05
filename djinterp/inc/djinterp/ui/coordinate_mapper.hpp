/*******************************************************************************
* djinterp [djinterp]                                      coordinate_mapper.hpp
*
*   The runtime bridge between the configuration's coordinate_system enum and
* the compile-time maths coordinate systems.  The lens layer is chosen at
* runtime, but the maths systems (cartesian / cylindrical / spherical) are
* compile-time types; this header wraps each one behind a small polymorphic
* interface so a lens can place a point without knowing, at compile time, which
* system the user picked.  Each mapper forwards to its system's to_cartesian.
*
*   The maths `polar` system is two-dimensional, so the ui `polar` choice maps
* to the maths `cylindrical` system — polar in the xy-plane with a height z —
* which is its natural three-dimensional form for a scene.
*
*
* path:      /inc/djinterp/ui/coordinate_mapper.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_COORDINATE_MAPPER_HPP
#define DJINTERP_UI_COORDINATE_MAPPER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <memory>
// djinterp
//   the maths coordinate systems live in djinterp::math after the maths->math
//   namespace unification; adjust this path to your tree.
#include "../math/coordinate/coordinate.hpp"  // cylindrical, spherical
#include "./types.hpp"               // coordinate_system, scalar


NS_DJINTERP
NS_UI

// coordinate_mapper
//   class: the abstract bridge.  to_cartesian converts a point given in some
// coordinate system's coordinates into a Cartesian render position; the third
// component is the system's height / out-of-plane coordinate where one exists.
class coordinate_mapper
{
    public:

        virtual ~coordinate_mapper() = default;

        D_NODISCARD virtual vec<3, scalar>
        to_cartesian
        (
            const vec<3, scalar>& _coords
        ) const = 0;

        D_NODISCARD virtual coordinate_system
        system
        () const noexcept = 0;
};


// cartesian_mapper
//   class: the identity bridge; coordinates are already Cartesian.
class cartesian_mapper final : public coordinate_mapper
{
    public:

        D_NODISCARD vec<3, scalar>
        to_cartesian
        (
            const vec<3, scalar>& _coords
        ) const override
        {
            return _coords;
        }

        D_NODISCARD coordinate_system
        system
        () const noexcept override
        {
            return coordinate_system::cartesian;
        }
};


// polar_mapper
//   class: bridges (rho, phi, z) through the maths cylindrical system.
class polar_mapper final : public coordinate_mapper
{
    public:

        D_NODISCARD vec<3, scalar>
        to_cartesian
        (
            const vec<3, scalar>& _coords
        ) const override
        {
            return vec<3, scalar>::from_array(
                math::cylindrical<scalar>::to_cartesian(to_point(_coords)));
        }

        D_NODISCARD coordinate_system
        system
        () const noexcept override
        {
            return coordinate_system::polar;
        }
};


// spherical_mapper
//   class: bridges (r, theta, phi) through the maths spherical system.
class spherical_mapper final : public coordinate_mapper
{
    public:

        D_NODISCARD vec<3, scalar>
        to_cartesian
        (
            const vec<3, scalar>& _coords
        ) const override
        {
            return vec<3, scalar>::from_array(
                math::spherical<scalar>::to_cartesian(to_point(_coords)));
        }

        D_NODISCARD coordinate_system
        system
        () const noexcept override
        {
            return coordinate_system::spherical;
        }
};


// make_coordinate_mapper
//   constructs the mapper for a coordinate system selected at runtime.
D_NODISCARD inline std::unique_ptr<coordinate_mapper>
make_coordinate_mapper
(
    coordinate_system _system
)
{
    switch (_system)
    {
        case coordinate_system::polar:
            return std::unique_ptr<coordinate_mapper>(new polar_mapper());

        case coordinate_system::spherical:
            return std::unique_ptr<coordinate_mapper>(new spherical_mapper());

        case coordinate_system::cartesian:
        default:
            return std::unique_ptr<coordinate_mapper>(new cartesian_mapper());
    }
}

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_COORDINATE_MAPPER_HPP
