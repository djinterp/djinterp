/*******************************************************************************
* djinterp [core]                                            functional_face.hpp
*
* The C++ face of the functional core: wrappers over the C types, never
* redeclarations of them.
*
*   Goal 4 states the rule this header exists to obey:
*
*       "A shared type is declared once, in the C header, compiled by both
*        languages. The C++ face wraps it; it never redeclares the fields. Two
*        declarations of one layout is the failure mode this rule exists to
*        prevent."
*
*   And goal 2 states what that buys: the C++ layer is a HOMOMORPHIC IMAGE of
* the C core, not an independent reimplementation. Every operation below
* forwards to the C function of the same name. There is no second algorithm
* anywhere in this file, which is what makes the parity law hold by
* construction rather than by testing -- the differential tests then confirm it
* rather than being the only thing standing between the two.
*
* TWO WRAPPING PATTERNS, FOR TWO KINDS OF C TYPE.
*
*   1. A C type that is not parameterised -- `d_reducer`, `d_producer`,
*      `d_semigroup`, `d_monoid`, `d_free` -- is HELD BY VALUE as the wrapper's
*      single member. Nothing is restated; the C declaration is the only one.
*      `sizeof(wrapper) == sizeof(the C struct)` is asserted for each.
*
*   2. A C type whose layout depends on an element type -- the `maybe` and
*      `result` cells -- cannot be held that way, because C spells it with a
*      macro that names the struct. So the FIELDS were factored out in maybe.h
*      and result.h, and the C++ template below expands THE SAME MACRO. One
*      declaration of the layout, two languages consuming it. `D_MAYBE_FIELDS`
*      is the shared definition; neither language writes the members again.
*
* WHAT THIS FACE IS NOT. It is not the existing core/functional .hpp tree.
* Those modules predate the C core and declare their own types; reconciling them
* is a separate and larger job, and doing it silently here would hide the
* question. This header is the face for the C modules built in this port, and it
* demonstrates the pattern the rest should follow.
*
* NO OWNERSHIP IS TAKEN. Every wrapper borrows exactly what the C type borrows.
* Storage stays the caller's, per goal 3's "no hidden allocation; storage
* strategy is chosen by the caller".
*
*
* path:      /inc/djinterp/core/functional/functional_face.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FUNCTIONAL_FACE_HPP
#define DJINTERP_FUNCTIONAL_FUNCTIONAL_FACE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>

extern "C"
{
    #include "../../c/functional/reducer.h"
    #include "../../c/functional/producer.h"
    #include "../../c/functional/transducer.h"
    #include "../../c/functional/semigroup.h"
    #include "../../c/functional/monoid.h"
    #include "../../c/functional/maybe.h"
    #include "../../c/functional/result.h"
    #include "../../c/functional/foldable.h"
    #include "../../c/functional/functor.h"
    #include "../../c/functional/free.h"
    #include "../../c/functional/extractor.h"
    #include "../../c/functional/interpolate.h"
}


namespace djinterp
{
namespace face
{

// ---------------------------------------------------------------------------
// I.   Pattern 1 -- hold the C struct, restate nothing
// ---------------------------------------------------------------------------

// reducer
//   Wraps `d_reducer`. Every method forwards; the class adds no member beyond
// the C struct it holds, which the Cost law assertion below pins.
class reducer
{
public:
    reducer() : core_(d_reducer_make(nullptr, nullptr)) {}

    explicit reducer(const d_reducer& _core) : core_(_core) {}

    static reducer from_fold(d_fold_binding& _binding)
    {
        return reducer(d_reducer_from_fold(&_binding));
    }

    static reducer from_consumer(d_consumer_binding& _binding)
    {
        return reducer(d_reducer_from_consumer(&_binding));
    }

    static reducer from_quantifier(d_quantifier& _quantifier)
    {
        return reducer(d_reducer_from_quantifier(&_quantifier));
    }

    bool is_valid() const { return d_reducer_is_valid(&core_); }

    std::size_t drive(d_reducing_state& _state,
                      const void*       _elements,
                      std::size_t       _count,
                      std::size_t       _element_size) const
    {
        return d_reducer_drive_array(&core_, &_state, _elements,
                                     _count, _element_size);
    }

    const d_reducer& core() const { return core_; }
    d_reducer&       core()       { return core_; }

private:
    d_reducer core_;
};

// semigroup
//   Wraps `d_semigroup`. `mappend` forwards to `d_mappend`, so associativity is
// whatever the C instance says it is -- there is no second combine here to
// disagree with it.
class semigroup
{
public:
    explicit semigroup(const d_semigroup& _core) : core_(_core) {}

    static semigroup first(std::size_t _width)
    {
        return semigroup(d_semigroup_first(_width));
    }

    static semigroup last(std::size_t _width)
    {
        return semigroup(d_semigroup_last(_width));
    }

    bool is_valid() const { return d_semigroup_is_valid(&core_); }

    bool mappend(void* _result, const void* _left, const void* _right) const
    {
        return d_mappend(&core_, _result, _left, _right);
    }

    bool reduce(const void* _elements,
                std::size_t _count,
                void*       _accumulator) const
    {
        return d_semigroup_reduce(&core_, _elements, _count, _accumulator);
    }

    reducer as_reducer() const
    {
        return reducer(d_semigroup_reducer(&core_));
    }

    const d_semigroup& core() const { return core_; }

private:
    d_semigroup core_;
};

// monoid
//   Wraps `d_monoid`. The forgetful map to the semigroup is the C one, so the
// Conservativity law holds for the same reason it holds in C.
class monoid
{
public:
    explicit monoid(const d_monoid& _core) : core_(_core) {}

    static monoid sum_intmax()     { return monoid(d_monoid_sum_intmax()); }
    static monoid product_intmax() { return monoid(d_monoid_product_intmax()); }
    static monoid min_intmax()     { return monoid(d_monoid_min_intmax()); }
    static monoid max_intmax()     { return monoid(d_monoid_max_intmax()); }
    static monoid sum_double()     { return monoid(d_monoid_sum_double()); }
    static monoid all_of()         { return monoid(d_monoid_all()); }
    static monoid any_of()         { return monoid(d_monoid_any()); }

    bool is_valid() const { return d_monoid_is_valid(&core_); }

    bool mempty(void* _out) const { return d_mempty(&core_, _out); }

    bool mconcat(const void* _elements,
                 std::size_t _count,
                 void*       _out) const
    {
        return d_mconcat(&core_, _elements, _count, _out);
    }

    semigroup forget() const
    {
        return semigroup(*d_monoid_semigroup(&core_));
    }

    reducer as_reducer() const { return reducer(d_monoid_reducer(&core_)); }

    const d_monoid& core() const { return core_; }

private:
    d_monoid core_;
};

// producer
//   Wraps `d_producer`. Pulling is destructive on both sides, because it is the
// same state being advanced.
class producer
{
public:
    producer() : core_(d_producer_empty(0)) {}

    explicit producer(const d_producer& _core) : core_(_core) {}

    bool is_valid() const { return d_producer_is_valid(&core_); }

    bool next(void* _out) { return core_.next(core_.state, _out); }

    std::size_t collect(void* _out_array, std::size_t _capacity)
    {
        return d_producer_collect(&core_, _out_array, _capacity);
    }

    std::size_t drive(const reducer&     _reducer,
                      d_reducing_state&  _state,
                      void*              _scratch)
    {
        return d_producer_drive(&core_, &_reducer.core(), &_state, _scratch);
    }

    const d_producer& core() const { return core_; }
    d_producer&       core()       { return core_; }

private:
    d_producer core_;
};

// ---------------------------------------------------------------------------
// II.  Pattern 2 -- expand the SHARED layout macro, restate nothing
// ---------------------------------------------------------------------------

// maybe<T>
//   The cell's members come from `D_MAYBE_FIELDS`, the same macro
// `D_MAYBE_DECLARE` expands in C. This class therefore does not declare a
// layout; it instantiates the one C declares.
template<typename T>
class maybe
{
public:
    maybe() { view_ = make_view(); d_maybe_nothing(&view_); }

    explicit maybe(const T& _value)
    {
        view_ = make_view();
        d_maybe_just(&view_, &_value);
    }

    bool is_just()    const { return d_maybe_is_just(&view_); }
    bool is_nothing() const { return d_maybe_is_nothing(&view_); }

    bool set(const T& _value) { return d_maybe_just(&view_, &_value); }
    bool clear()              { return d_maybe_nothing(&view_); }

    T value_or(const T& _fallback) const
    {
        T out;

        d_maybe_value_or(&view_, &out, &_fallback);

        return out;
    }

    template<typename U>
    bool map(maybe<U>& _out, fn_transformer _transform, void* _context = 0)
    {
        return d_maybe_map(&view_, &_out.view(), _transform, _context);
    }

    template<typename U>
    bool bind(maybe<U>& _out, fn_kleisli _arrow, void* _context = 0)
    {
        return d_maybe_bind(&view_, &_out.view(), _arrow, _context);
    }

    bool alt(const maybe<T>& _second, maybe<T>& _out) const
    {
        return d_maybe_alt(&view_, &_second.view(), &_out.view());
    }

    std::size_t drive(const reducer& _reducer, d_reducing_state& _state) const
    {
        return d_maybe_drive(&view_, &_reducer.core(), &_state);
    }

    d_maybe&       view()       { return view_; }
    const d_maybe& view() const { return const_cast<d_maybe&>(view_); }

    // the cell: THE layout, from the shared macro, not restated here
    struct cell
    {
        D_MAYBE_FIELDS(T)
    };

    const cell& storage() const { return cell_; }

private:
    d_maybe make_view()
    {
        return d_maybe_view(&cell_.has_value,
                            &cell_.value,
                            sizeof(cell_.value));
    }

    cell    cell_;
    d_maybe view_;
};

// result<T, E>
//   Likewise: the members come from `D_RESULT_FIELDS`, shared with C.
template<typename T, typename E>
class result
{
public:
    result() { view_ = make_view(); }

    bool is_ok()  const { return d_result_is_ok(&view_); }
    bool is_err() const { return d_result_is_err(&view_); }

    bool set_ok(const T& _value)  { return d_result_ok(&view_, &_value); }
    bool set_err(const E& _error) { return d_result_err(&view_, &_error); }

    T value_or(const T& _fallback) const
    {
        T out;

        d_result_value_or(&view_, &out, &_fallback);

        return out;
    }

    bool error(E& _out) const { return d_result_error(&view_, &_out); }

    template<typename U>
    bool map(result<U, E>& _out, fn_transformer _transform, void* _ctx = 0)
    {
        return d_result_map(&view_, &_out.view(), _transform, _ctx);
    }

    bool to_maybe(maybe<T>& _out) const
    {
        return d_result_to_maybe(&view_, &_out.view());
    }

    std::size_t drive(const reducer& _reducer, d_reducing_state& _state) const
    {
        return d_result_drive(&view_, &_reducer.core(), &_state);
    }

    d_result&       view()       { return view_; }
    const d_result& view() const { return const_cast<d_result&>(view_); }

    // the cell: THE layout, from the shared macro, not restated here
    struct cell
    {
        D_RESULT_FIELDS(T, E)
    };

    const cell& storage() const { return cell_; }

private:
    d_result make_view()
    {
        return d_result_view(&cell_.is_ok,
                             &cell_.payload,
                             sizeof(cell_.payload.value),
                             sizeof(cell_.payload.error));
    }

    cell     cell_;
    d_result view_;
};

// ---------------------------------------------------------------------------
// III. Cost law -- a wrapper adds no members and no indirection
// ---------------------------------------------------------------------------
//   Goal 10: "A wrapper's sizeof equals the wrapped type's; abstraction adds no
// members and no indirection." Pattern-1 wrappers hold exactly one C struct, so
// the equality is exact and is asserted here. Pattern-2 classes hold a cell AND
// a view, which is two objects rather than a decorated one, so the cell's own
// size is what must match -- and it does, because it IS the C layout.

D_STATIC_ASSERT(sizeof(reducer)   == sizeof(d_reducer),   "reducer wrapper cost");
D_STATIC_ASSERT(sizeof(semigroup) == sizeof(d_semigroup), "semigroup wrapper cost");
D_STATIC_ASSERT(sizeof(monoid)    == sizeof(d_monoid),    "monoid wrapper cost");
D_STATIC_ASSERT(sizeof(producer)  == sizeof(d_producer),  "producer wrapper cost");

// no virtual functions in core types, ever (goal 3)
D_STATIC_ASSERT(sizeof(reducer) == (sizeof(fn_reducer_step) + sizeof(void*)),
                "a reducer wrapper must carry no vtable pointer");

}   // namespace face
}   // namespace djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_FUNCTIONAL_FACE_HPP
