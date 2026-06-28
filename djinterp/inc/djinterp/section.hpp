/******************************************************************************
* djinterp [text]                                                  section.hpp
*
*   The iteration combinator of the document_template skeleton.  A section is
* a (count_fn + refocus_fn) pair over a caller-defined focus type _Ctx: at
* render time it asks the OUTER ctx for an iteration count, builds an INNER
* ctx for each iteration by re-pointing parts of the outer one, and invokes a
* caller-supplied BODY against (the binding_env, the inner ctx, a format-
* specific target).  The body places content; the section drives the loop.
*
*   THE INVARIANT THIS WHOLE LAYER RESTS ON:
*   The env never mutates during iteration.  Only the focus does.  Every body
* invocation receives the SAME env reference and a DIFFERENT _Ctx, so a body
* whose only reads of the env are env.source_for(_ctx) and env.project(_key,
* _ctx) draws iteration i's fragment from the SAME PROJECTION it would draw
* for any other i - only the ctx threaded into the projection differs.  In
* particular, the canonical tree and flow bodies:
*
*     tree body (document_writer cursor):
*         _cursor.open_child(_tag).text(_env.project(_key, _ctx));
*     flow body (pdf_template, format-string version):
*         _flow.add_text(_format_tpl.render(_env.source_for(_ctx)));
*
* differ ONLY in placement (a subtree vs a vertical band).  The same env, the
* same keys, the same projections, the same fragments.  That equality is the
* "only placement differs, not binding" property the document stack turns on.
*
*   FORMAT-AGNOSTIC BY CONSTRUCTION:
*   section<_Ctx> mentions no format type.  It is parameterized over _Ctx (the
* focus); the body and the target are template parameters of for_each.  A new
* format adds a new BODY shape, not a new section.  This is the iteration
* analogue of binding_env's "one env, two consumption methods" property.
*
*   COMPOSITION (RECURSION OF THE SKELETON):
*   A body is itself a sink-shaped operation, so a section's body may invoke
* ANOTHER section over a deeper focus part (units within a module, checks
* within a unit), and the binding-is-uniform proof carries through to every
* depth.  This is why the skeleton is recursive while the kernel here is
* a single combinator.
*
*   PDF DOES NOT FORCE A SECOND ENVIRONMENT:
*   It forces a second SKELETON KIND (flow vs tree), but it draws its values
* from the same env through env.source_for(ctx).  Per the binding_env design
* note, pdf_template resolves {key} through a borrowed text_template; the
* recommended seam is "hand pdf_template a literal that has already been
* resolved through env.source_for(ctx)", so the flow path depends on the
* lookup-source level, not on pdf_template's internal binding surface.
*
*   DTEST-AGNOSTIC:
*   section knows nothing of DTest (or of any document format).  It is generic
* over _Ctx and depends only on binding_env, the standard library, and the
* language-detection header.  Requires C++17 (parallel to binding_env, which
* it composes); self-suppresses below it.
*
*   RUNTIME COMBINATOR (LIKE binding_env):
*   The count and the refocus are run-dependent (n = run->modules.size()),
* so the section is necessarily a runtime value.  The static half of the
* skeleton (which keys / which sections exist, the structural shape) belongs
* to a higher layer (a compile-time document_template, future work); this
* header is the iteration kernel that layer composes.
*
*
* TABLE OF CONTENTS
* =================
* I.    SECTION                  (section<_Ctx>: count / refocus / for_each)
* II.   FACTORIES                (make_section)
*
*
* path:      /inc/djinterp/core/text/section.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_TEXT_SECTION_
#define DJINTERP_TEXT_SECTION_ 1

// std
#include <cstddef>
#include <functional>
#include <utility>
// djinterp
#include "../djinterp.hpp"      // NS_*, D_NODISCARD, D_NOEXCEPT, language gates
#include "./binding_env.hpp"    // binding_env<_Ctx> - the env this kernel drives


// section composes binding_env, which is C++17 (std::string_view); below the
// floor this module contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   SECTION                                              ///
///////////////////////////////////////////////////////////////////////////////

// section
//   class: an iteration node parameterized by a count_fn (n = count(outer_ctx))
// and a refocus_fn (inner = refocus(outer_ctx, i)).  for_each is the kernel:
// for i in [0, n), invoke body(env, refocus(outer_ctx, i), target).  Generic
// over _Ctx (the focus) and over the body/target through for_each's template
// parameters, so the same section drives a document_writer cursor, a
// pdf_template, or any other sink-shaped target without itself mentioning a
// format type.
//
// Usage (the canonical "per-module" case over a DTest run):
//   struct ctx { const test_report* run; const report_module* module; };
//
//   section<ctx> per_module(
//       [](const ctx& _c) -> std::size_t
//       {
//           return _c.run ? _c.run->modules.size() : 0;
//       },
//       [](const ctx& _c, std::size_t _i) -> ctx
//       {
//           ctx _r = _c;
//           _r.module = &_c.run->modules[_i];
//           return _r;
//       });
//
//   // tree face: append N module-subtrees under one head
//   per_module.for_each(env, here, cursor,
//       [](const binding_env<ctx>& _e, const ctx& _c, cursor& _cur)
//       {
//           _cur.open_child("module")
//               .text(_e.project("module_name", _c));
//       });
//
//   // flow face: append N module-bands (with an optional page break)
//   per_module.for_each(env, here, tpl,
//       [&](const binding_env<ctx>& _e, const ctx& _c, pdf_template& _t)
//       {
//           _t.add_text(name_tpl.render(_e.source_for(_c)));
//           _t.add_page_break();
//       });
template<typename _Ctx>
class section
{
public:
    // -- public type aliases -------------------------------------------------

    // context_type
    //   type: the focus a projection reads from (passed through from
    // binding_env; the same _Ctx the env's projections are written against).
    using context_type = _Ctx;

    // env_type
    //   type: the binding_env this section drives.  Borrowed by for_each;
    // never mutated by iteration.
    using env_type = binding_env<_Ctx>;

    // count_fn
    //   type: the extractor for the iteration count (n = count(outer_ctx)).
    // e.g. [](const ctx& _c) { return _c.run ? _c.run->modules.size() : 0; }.
    using count_fn = std::function<std::size_t(const _Ctx&)>;

    // refocus_fn
    //   type: builds the per-iteration inner ctx by re-pointing one or more
    // parts of the outer one - the "scope by focus" mechanism of the design.
    // e.g. [](const ctx& _c, std::size_t _i) { ctx _r = _c;
    //                                          _r.module = &_c.run->modules[_i];
    //                                          return _r; }.
    using refocus_fn = std::function<_Ctx(const _Ctx&, std::size_t)>;

    // size_type
    //   type: the iteration-count type.
    using size_type = std::size_t;

    // section
    //   constructor: an empty section (count is 0 for every ctx, refocus
    // would be identity if anyone reached it).
    section() = default;

    // section
    //   constructor: a section over _count + _refocus.  Either may be a null
    // std::function; the defaults are "no iterations" (empty count) and
    // "identity" (a null refocus returns the outer ctx unchanged).
    section(
        count_fn   _count,
        refocus_fn _refocus
    )
        : m_count(static_cast<count_fn&&>(_count)),
          m_refocus(static_cast<refocus_fn&&>(_refocus))
    {}

    // count
    //   the iteration count for _outer_ctx.  An unset count_fn yields 0 (a
    // render against this section does nothing - the "empty section" case).
    D_NODISCARD size_type
    count(
        const _Ctx& _outer_ctx
    ) const
    {
        if (!m_count)
        {
            return size_type(0);
        }

        return m_count(_outer_ctx);
    }

    // refocus
    //   the inner ctx for iteration _i over _outer_ctx.  An unset refocus_fn
    // returns _outer_ctx unchanged - the degenerate "non-iterating section
    // over the same focus" case (every body sees the outer focus).
    D_NODISCARD _Ctx
    refocus(
        const _Ctx& _outer_ctx,
        size_type   _i
    ) const
    {
        if (!m_refocus)
        {
            return _outer_ctx;
        }

        return m_refocus(_outer_ctx, _i);
    }

    // for_each
    //   THE KERNEL.  For i in [0, count(_outer_ctx)), invokes _body(_env,
    // refocus(_outer_ctx, i), _target).  The env is borrowed by reference and
    // is NEVER MUTATED by this kernel; the target is whatever sink the caller
    // supplies (a document_writer cursor, a pdf_template, ...).
    //
    //   The body signature is void(const env_type&, const _Ctx&, _Target&).
    // _Body and _Target are template parameters so the same section drives
    // every format without itself mentioning a format type.
    template<typename _Body,
             typename _Target>
    void
    for_each(
        const env_type& _env,
        const _Ctx&     _outer_ctx,
        _Target&        _target,
        _Body&&         _body
    ) const
    {
        const size_type _n = count(_outer_ctx);
        size_type       _i = 0;

        // iterate the refocused ctx through the caller's body; the env is
        // pass-through and the section never reads it itself.
        for (_i = 0; _i < _n; ++_i)
        {
            const _Ctx _inner = refocus(_outer_ctx, _i);

            _body(_env, _inner, _target);
        }

        return;
    }

    // empty
    //   true iff no count_fn is bound (count would be 0 for every ctx).
    D_NODISCARD bool
    empty() const D_NOEXCEPT
    {
        return !static_cast<bool>(m_count);
    }

private:
    count_fn   m_count;
    refocus_fn m_refocus;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  FACTORIES                                            ///
///////////////////////////////////////////////////////////////////////////////

// make_section
//   function: build a section<_Ctx> from a count callable and a refocus
// callable.  _Ctx is explicit (it is not deducible from the closures alone,
// because the count's argument and the refocus's argument are both _Ctx);
// the callables are forwarded into the std::function slots.  Equivalent to
// the two-arg constructor; useful when type-deduction at the call site is
// preferable to spelling section<_Ctx>::count_fn / refocus_fn.
//
// Usage:
//   auto per_module = make_section<ctx>(
//       [](const ctx& _c) -> std::size_t { ... },
//       [](const ctx& _c, std::size_t _i) -> ctx { ... });
template<typename _Ctx,
         typename _Count,
         typename _Refocus>
D_NODISCARD section<_Ctx>
make_section(
    _Count&&   _count,
    _Refocus&& _refocus
)
{
    return section<_Ctx>(
        typename section<_Ctx>::count_fn(static_cast<_Count&&>(_count)),
        typename section<_Ctx>::refocus_fn(static_cast<_Refocus&&>(_refocus)));
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEXT_SECTION_