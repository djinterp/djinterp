/*******************************************************************************
* djinterp [core]                                         test_section_edges.cpp
*
*
* path:      /tests/djinterp/core/text/test_section_edges.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// Edge-case smoke tests for the section kernel.

// std
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
// djinterp
#include "djinterp/core/text/section.hpp"


struct ctx
{
    int v = 0;
};


using djinterp::binding_env;
using djinterp::section;
using djinterp::make_section;


int
main()
{
    binding_env<ctx> _env;

    _env.bind("v",
              [](const ctx& _c) -> std::string
              {
                  return std::to_string(_c.v);
              });

    // 1. default-constructed section: count is 0 for every ctx; body never
    //    runs.
    {
        section<ctx> _empty;
        ctx          _here;
        int          _hits  = 0;
        int          _proxy = 0;

        assert(_empty.empty());
        assert(_empty.count(_here) == 0);

        _empty.for_each(_env, _here, _proxy,
            [&](const binding_env<ctx>&, const ctx&, int&)
            {
                ++_hits;
            });

        assert(_hits == 0);
    }

    // 2. count > 0 but refocus is null: every iteration sees the OUTER ctx
    //    unchanged.  Useful as the degenerate "non-iterating section over
    //    the same focus" case.
    {
        section<ctx> _no_refocus(
            [](const ctx&) -> std::size_t { return 3; },
            nullptr);

        ctx                       _here;
        _here.v = 42;
        std::vector<std::string>  _drawn;

        _no_refocus.for_each(_env, _here, _drawn,
            [](const binding_env<ctx>&  _e,
               const ctx&               _c,
               std::vector<std::string>& _acc)
            {
                _acc.push_back(_e.project("v", _c));
            });

        assert(_drawn.size() == 3);
        assert(_drawn[0] == "42");
        assert(_drawn[1] == "42");
        assert(_drawn[2] == "42");
    }

    // 3. count of zero (legitimate, not unbound): body is never invoked.
    {
        section<ctx> _zero(
            [](const ctx&) -> std::size_t { return 0; },
            [](const ctx& _c, std::size_t) -> ctx { return _c; });

        ctx _here;
        int _hits  = 0;
        int _proxy = 0;

        assert(!_zero.empty());     // count_fn IS bound
        assert(_zero.count(_here) == 0);

        _zero.for_each(_env, _here, _proxy,
            [&](const binding_env<ctx>&, const ctx&, int&)
            {
                ++_hits;
            });

        assert(_hits == 0);
    }

    // 4. refocus actually re-points: every iteration sees a DIFFERENT ctx.
    {
        section<ctx> _bump(
            [](const ctx&) -> std::size_t { return 4; },
            [](const ctx& _c, std::size_t _i) -> ctx
            {
                ctx _r = _c;
                _r.v   = _c.v + static_cast<int>(_i) * 10;

                return _r;
            });

        ctx                       _here;
        _here.v = 1;
        std::vector<std::string>  _drawn;

        _bump.for_each(_env, _here, _drawn,
            [](const binding_env<ctx>&   _e,
               const ctx&                _c,
               std::vector<std::string>& _acc)
            {
                _acc.push_back(_e.project("v", _c));
            });

        assert(_drawn.size() == 4);
        assert(_drawn[0] == "1");
        assert(_drawn[1] == "11");
        assert(_drawn[2] == "21");
        assert(_drawn[3] == "31");
    }

    // 5. env is not mutated by iteration.
    {
        const auto _size_before = _env.size();

        section<ctx> _s(
            [](const ctx&) -> std::size_t { return 5; },
            [](const ctx& _c, std::size_t) -> ctx { return _c; });

        ctx _here;
        int _proxy = 0;

        _s.for_each(_env, _here, _proxy,
            [](const binding_env<ctx>&, const ctx&, int&)
            {});

        assert(_env.size() == _size_before);
    }

    std::cout << "all section edge cases passed.\n";
    return 0;
}
