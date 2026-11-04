/*******************************************************************************
* djinterp [cpp/math]                                        t_platonic_face.cpp
*
*   Tests for geometry/platonic.hpp, at every level from C++98, per family:
* the five solids of unit circumradius -- their known volumes, every vertex
* on the unit sphere, the centroid at the origin, the origin inside and a
* point beyond the sphere outside -- and the plain double overloads below
* C++11's default template arguments.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++98 -Wall -Wextra -Werror -pedantic-errors -Iinc              \
*         tests/djinterp/math/geometry/t_platonic_face.cpp -o t -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/geometry/t_platonic_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/math/geometry/platonic.hpp"  // tested
// std
#include <cstdio>   // std::printf


namespace
{
    namespace dm = djinterp::math;

    long g_checks   = 0;
    long g_failures = 0;

    void
    d_tests_expect(
        bool        _ok,
        const char* _what,
        int         _line
    )
    {
        ++g_checks;

        // report the first few failures with what was checked, and where
        if (!_ok)
        {
            ++g_failures;

            if (g_failures <= 40)
            {
                std::printf("  FAIL line %d: %s\n", _line, _what);
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what)                                          \
        d_tests_expect((ok), (what), __LINE__)

    template<typename Float>
    bool
    d_tests_near(Float _a, Float _b, Float _tol)
    {
        const Float d = _a - _b;

        return ( (d <= _tol) &&
                 (d >= -_tol) );
    }

    template<typename Mesh>
    void
    d_tests_solid(
        const Mesh&                _m,
        typename Mesh::value_type  _volume,
        typename Mesh::value_type  _tol,
        const char*                _what
    )
    {
        typedef typename Mesh::value_type T;
        typedef typename Mesh::point_type p3;

        bool on_sphere = true;

        for (std::size_t i = 0; i < Mesh::vertex_count; ++i)
        {
            on_sphere = on_sphere &&
                        d_tests_near(_m.m_vertices[i].norm(), T(1), _tol);
        }

        D_TESTS_EXPECT( (d_tests_near(_m.closed_form_volume(), _volume,
                                      _tol * 4)) &&
                        (on_sphere) &&
                        (_m.closed_form_centroid().equals(p3(0, 0, 0),
                                                          _tol * 4)) &&
                        (_m.contains(p3(T(0.01), T(0.02), T(0.03)))) &&
                        (!_m.contains(p3(T(1.1), 0, 0))),
                        _what);

        return;
    }

    template<typename T>
    void
    d_tests_family(T _tol)
    {
        d_tests_solid(dm::tetrahedron_mesh<T>(), T(0.5132002392796673), _tol,
                      "a tetrahedron");
        d_tests_solid(dm::hexahedron_mesh<T>(), T(1.5396007178390020), _tol,
                      "a cube");
        d_tests_solid(dm::octahedron_mesh<T>(), T(1.3333333333333333), _tol,
                      "an octahedron");
        d_tests_solid(dm::icosahedron_mesh<T>(), T(2.5361507101204650), _tol,
                      "an icosahedron");
        d_tests_solid(dm::dodecahedron_mesh<T>(), T(2.7851638631226920), _tol,
                      "a dodecahedron");

        return;
    }
}  // namespace


int
main()
{
    d_tests_family<float>(1e-5f);
    d_tests_family<double>(1e-13);
    d_tests_family<long double>(1e-13L);

    // the plain overloads: double, unit radius
    D_TESTS_EXPECT( (dm::tetrahedron_mesh().vertex_count == 4) &&
                    (d_tests_near(dm::dodecahedron_mesh().closed_form_volume(),
                                  2.7851638631226920, 1e-13)),
                    "the no-argument factories");

    std::printf("t_platonic_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
