/*******************************************************************************
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
* path:      /inc/re_std/chrono/duration_fwd.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_DURATION_FWD_HPP
#define RE_STD_CHRONO_DURATION_FWD_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../ratio/ratio.hpp"
#include "../type_traits/true_type.hpp"
#include "../type_traits/false_type.hpp"


namespace re_std
{

// std places the whole facility in a NESTED namespace -- they are
// std::chrono::duration, not std::duration -- so re_std mirrors it as
// re_std::chrono::duration. There is no NS_ macro for a nested
// project namespace, so it is opened literally, as numbers.hpp does.
namespace chrono
{

    // duration
    //   class: forward declaration only. Defined in duration.hpp.
    template<typename Rep,
             typename Period = ratio<1> >
    class duration;


namespace internal
{

    // is_duration
    //   trait: true if Type is a chrono::duration. Exposition-only in the
    // standard, so it is not surfaced outside internal::.
    template<typename Type>
    struct is_duration
        : false_type
    {};

    template<typename Rep,
             typename Period>
    struct is_duration< duration<Rep, Period> >
        : true_type
    {};

    // The cv-qualified forms matter: duration_cast's target is written by
    // the caller, and `duration_cast<const milliseconds>(d)` should be
    // constrained in, not rejected on a technicality.
    template<typename Rep,
             typename Period>
    struct is_duration< const duration<Rep, Period> >
        : true_type
    {};

    template<typename Rep,
             typename Period>
    struct is_duration< volatile duration<Rep, Period> >
        : true_type
    {};

    template<typename Rep,
             typename Period>
    struct is_duration< const volatile duration<Rep, Period> >
        : true_type
    {};

}  // internal


}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_DURATION_FWD_HPP
