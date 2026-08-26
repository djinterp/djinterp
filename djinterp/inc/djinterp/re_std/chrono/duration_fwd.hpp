/******************************************************************************
* djinterp [re_std]                                             duration_fwd.hpp
*
* the duration forward declaration and the is_duration trait:
*   This header exists to break a dependency cycle, and holds nothing the
* standard names.
*
*   THE CYCLE:
*   duration's converting constructor is specified in terms of
* duration_cast -- constructing a milliseconds from a seconds performs
* exactly that conversion. But duration_cast is constrained on its target
* being a duration, and is written against duration's own rep and period
* members. Each therefore needs the other, and neither can be first.
*
*   The cut is made here: a forward declaration of the class template,
* plus the exposition-only is_duration trait that duration_cast's
* constraint needs. Both are usable before duration is defined, because
* neither requires a complete type. duration.hpp then defines the class
* and reaches the conversion arithmetic through internal::
* duration_cast_helper directly, while duration_cast.hpp supplies the
* public spelling on top.
*
*   is_duration IS INTERNAL AND STAYS INTERNAL:
*   [time.traits] leaves it exposition-only, so there is no std::
* is_duration and there must be no re_std::is_duration either. It lives
* in re_std::chrono::internal, where user code has no business finding
* it. A caller who wants this question answered should ask
* treat_as_floating_point or match on the type directly.
*
*
* path:      /inc/djinterp/re_std/chrono/duration_fwd.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_FWD_
#define DJINTERP_RE_STD_CHRONO_DURATION_FWD_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../ratio/ratio.hpp"
#include "../type_traits/true_type.hpp"
#include "../type_traits/false_type.hpp"


NS_RESTD

// std places the whole facility in a NESTED namespace -- they are
// std::chrono::duration, not std::duration -- so re_std mirrors it as
// re_std::chrono::duration. There is no NS_ macro for a nested
// project namespace, so it is opened literally, as numbers.hpp does.
namespace chrono
{

    // duration
    //   class: forward declaration only. Defined in duration.hpp.
    template<typename _Rep,
             typename _Period = ratio<1> >
    class duration;


NS_INTERNAL

    // is_duration
    //   trait: true if _Type is a chrono::duration. Exposition-only in the
    // standard, so it is not surfaced outside internal::.
    template<typename _Type>
    struct is_duration
        : false_type
    {};

    template<typename _Rep,
             typename _Period>
    struct is_duration< duration<_Rep, _Period> >
        : true_type
    {};

    // The cv-qualified forms matter: duration_cast's target is written by
    // the caller, and `duration_cast<const milliseconds>(d)` should be
    // constrained in, not rejected on a technicality.
    template<typename _Rep,
             typename _Period>
    struct is_duration< const duration<_Rep, _Period> >
        : true_type
    {};

    template<typename _Rep,
             typename _Period>
    struct is_duration< volatile duration<_Rep, _Period> >
        : true_type
    {};

    template<typename _Rep,
             typename _Period>
    struct is_duration< const volatile duration<_Rep, _Period> >
        : true_type
    {};

NS_END  // internal


}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_FWD_
