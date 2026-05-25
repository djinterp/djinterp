/******************************************************************************
* djinterp [options]                                option_override_policy.hpp
*
* Override policies for cascading option layers.
*   Provides compile-time configurable policies that govern when a
* descendant option layer is permitted to override a key inherited
* from its parent.
*
*   BUILT-IN POLICIES:
*
*     override_allow_all     - any descendant may override any key
*     override_deny_all      - no overrides permitted; inheritance is
*                              immutable (supremacy clause)
*     override_to_depth<N>   - overrides permitted only at depth <= N
*     override_if<_Predicate>     - overrides permitted when a user-supplied
*                              predicate returns true; integrates with
*                              the predicate combinators in predicate.hpp
*     override_unless<_Predicate> - complement of override_if
*     override_whitelist<>   - overrides permitted only for keys in a
*                              compile-time list (constexpr array)
*     override_blacklist<>   - overrides denied for keys in a list
*
*   STRUCTURAL CONTRACT:
*   A type is a valid override policy if it exposes:
*
*     bool allows(key, depth) const
*
*   All built-in policies satisfy this contract.  User-defined policies
* that expose allows(key, depth) are automatically detected by the
* trait system without tags.
*
*   COMPOSITION:
*   Policies compose via the predicate combinators:
*
*     auto policy = override_and(
*         override_to_depth<3>{},
*         override_if(my_predicate));
*
* DEPENDENCIES:
*   djinterp.hpp       - D_CONSTEXPR, namespaces
*   type_traits.hpp    - clean_t, void_t
*
* TABLE OF CONTENTS
* =================
* I.    Policy Structural Contract Detection
* II.   Built-in Policies
* III.  Predicate-Based Policies
* IV.   Policy Combinators
* V.    Policy Classification
*
*
* path:      /inc/djinterp/core/options/option_override_policy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.12
******************************************************************************/

#ifndef DJINTERP_OPTION_OVERRIDE_POLICY_
#define DJINTERP_OPTION_OVERRIDE_POLICY_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Policy Structural Contract Detection
// ===========================================================================
// A valid override policy exposes:
//   bool allows(const _Key& key, std::size_t depth) const
//
// Detection is tagless and purely structural.

NS_INTERNAL

    // has_allows_check
    //   helper: true if _Policy has allows(key, depth) const
    // returning something convertible to bool.
    // We use int as a stand-in key type for detection; the
    // actual key type is checked at the call site.
    template<typename _Policy, typename = void>
    struct has_allows_check : std::false_type
    {};

    template<typename _Policy>
    struct has_allows_check<_Policy,
        D_VOID_T<
            decltype(
                std::declval<const _Policy&>().allows(
                    std::declval<int>(),
                    std::declval<std::size_t>()))
        >> : std::true_type
    {};

    // has_allows_typed_check
    //   helper: true if _Policy has allows(_Key, depth) const
    // for a specific key type.
    template<typename _Policy,
             typename _Key,
             typename = void>
    struct has_allows_typed_check : std::false_type
    {};

    template<typename _Policy,
             typename _Key>
    struct has_allows_typed_check<_Policy, _Key,
        D_VOID_T<
            decltype(
                std::declval<const _Policy&>().allows(
                    std::declval<const _Key&>(),
                    std::declval<std::size_t>()))
        >> : std::true_type
    {};

NS_END  // internal

// is_override_policy
//   type trait: true if the type satisfies the override
// policy structural contract with a generic key type.
template<typename _Type>
struct is_override_policy
{
    static constexpr bool value =
        internal::has_allows_check<clean_t<_Type>>::value;
};

template<typename _Type>
inline constexpr bool is_override_policy_v =
    is_override_policy<_Type>::value;

// is_override_policy_for
//   type trait: true if the type satisfies the override
// policy contract for a specific key type.
template<typename _Type,
         typename _Key>
struct is_override_policy_for
{
    static constexpr bool value =
        internal::has_allows_typed_check<
            clean_t<_Type>, _Key>::value;
};

template<typename _Type,
         typename _Key>
inline constexpr bool is_override_policy_for_v =
    is_override_policy_for<_Type, _Key>::value;


// ===========================================================================
// II.  Built-in Policies
// ===========================================================================

// override_allow_all
//   policy: permits all overrides at any depth.  This is
// the default policy for option layers.
struct override_allow_all
{
    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key& /*_k*/,
           std::size_t /*_depth*/) const noexcept
    {
        return true;
    }
};

// override_deny_all
//   policy: denies all overrides.  Inheritance is immutable.
// This is the "supremacy clause" - the parent's values
// cannot be overridden by any descendant.
struct override_deny_all
{
    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key& /*_k*/,
           std::size_t /*_depth*/) const noexcept
    {
        return false;
    }
};

// override_to_depth
//   policy: permits overrides only at depth <= _MaxDepth.
// Depth 0 is the immediate child of the root.  At depth
// _MaxDepth + 1 and beyond, overrides are denied.
//
// Example:
//   override_to_depth<1> allows the root's child and
//   grandchild to override, but no deeper.
template<std::size_t _MaxDepth>
struct override_to_depth
{
    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key& /*_k*/,
           std::size_t _depth) const noexcept
    {
        return (_depth <= _MaxDepth);
    }
};

// override_at_depth
//   policy: permits overrides only at exactly depth _Depth.
template<std::size_t _Depth>
struct override_at_depth
{
    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key& /*_k*/,
           std::size_t _depth) const noexcept
    {
        return (_depth == _Depth);
    }
};


// ===========================================================================
// III. Predicate-Based Policies
// ===========================================================================
// These policies accept a user-supplied predicate to make
// override decisions.  The predicate is stored by value
// (decayed), matching the pattern used in predicate.hpp
// combinators.

// override_if
//   policy: permits overrides when a predicate over the key
// returns true.  The predicate signature is:
//   bool pred(const Key&)
// or equivalently any callable invocable with const Key&.
//
// Example:
//   auto policy = make_override_if(
//       [](const std::string& k) { return k != "locked"; });
template<typename _Predicate>
class override_if_policy
{
public:
    template<typename _PredFwd>
    D_CONSTEXPR explicit
    override_if_policy(_PredFwd&& _predicate)
        : m_pred(static_cast<_PredFwd&&>(_predicate))
    {}

    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key&  _k,
           std::size_t /*_depth*/) const
    {
        return static_cast<bool>(m_pred(_k));
    }

    D_CONSTEXPR const _Predicate&
    predicate() const noexcept
    {
        return m_pred;
    }

private:
    _Predicate m_pred;
};

// make_override_if
//   function: constructs an override_if_policy with a
// deduced predicate type.
template<typename _Predicate>
D_CONSTEXPR
override_if_policy<typename std::decay<_Predicate>::type>
make_override_if(_Predicate&& _predicate)
{
    return override_if_policy<
        typename std::decay<_Predicate>::type>(
            static_cast<_Predicate&&>(_predicate));
}

// override_unless
//   policy: permits overrides UNLESS a predicate returns true.
// Complement of override_if.
template<typename _Predicate>
class override_unless_policy
{
public:
    template<typename _PredFwd>
    D_CONSTEXPR explicit
    override_unless_policy(_PredFwd&& _predicate)
        : m_pred(static_cast<_PredFwd&&>(_predicate))
    {}

    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key&  _k,
           std::size_t /*_depth*/) const
    {
        return !static_cast<bool>(m_pred(_k));
    }

    D_CONSTEXPR const _Predicate&
    predicate() const noexcept
    {
        return m_pred;
    }

private:
    _Predicate m_pred;
};

// make_override_unless
//   function: constructs an override_unless_policy with a
// deduced predicate type.
template<typename _Predicate>
D_CONSTEXPR
override_unless_policy<typename std::decay<_Predicate>::type>
make_override_unless(_Predicate&& _predicate)
{
    return override_unless_policy<
        typename std::decay<_Predicate>::type>(
            static_cast<_Predicate&&>(_predicate));
}

// override_key_depth_if
//   policy: predicate receives both key AND depth for
// maximum flexibility.  The predicate signature is:
//   bool pred(const Key&, std::size_t depth)
template<typename _Predicate>
class override_key_depth_if_policy
{
public:
    template<typename _PredFwd>
    D_CONSTEXPR explicit
    override_key_depth_if_policy(_PredFwd&& _predicate)
        : m_pred(static_cast<_PredFwd&&>(_predicate))
    {}

    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key&  _k,
           std::size_t  _depth) const
    {
        return static_cast<bool>(m_pred(_k, _depth));
    }

private:
    _Predicate m_pred;
};

// make_override_key_depth_if
//   function: constructs an override_key_depth_if_policy.
template<typename _Predicate>
D_CONSTEXPR
override_key_depth_if_policy<
    typename std::decay<_Predicate>::type>
make_override_key_depth_if(_Predicate&& _predicate)
{
    return override_key_depth_if_policy<
        typename std::decay<_Predicate>::type>(
            static_cast<_Predicate&&>(_predicate));
}


// ===========================================================================
// IV.  Policy Combinators
// ===========================================================================
// Compose policies using logical AND/OR, mirroring the
// predicate combinator pattern from predicate.hpp.

// override_and_policy
//   policy: permits override only when BOTH inner policies
// permit it.
template<typename _Policy1,
         typename _Policy2>
class override_and_policy
{
public:
    template<typename _P1Fwd,
             typename _P2Fwd>
    D_CONSTEXPR
    override_and_policy(_P1Fwd&& _p1, _P2Fwd&& _p2)
        : m_p1(static_cast<_P1Fwd&&>(_p1)),
          m_p2(static_cast<_P2Fwd&&>(_p2))
    {}

    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key&  _k,
           std::size_t  _depth) const
    {
        return ( m_p1.allows(_k, _depth) &&
                 m_p2.allows(_k, _depth) );
    }

    D_CONSTEXPR const _Policy1& first()  const { return m_p1; }
    D_CONSTEXPR const _Policy2& second() const { return m_p2; }

private:
    _Policy1 m_p1;
    _Policy2 m_p2;
};

// override_and
//   function: creates a combined AND policy.
template<typename _Policy1,
         typename _Policy2>
D_CONSTEXPR
override_and_policy<typename std::decay<_Policy1>::type,
                    typename std::decay<_Policy2>::type>
override_and(_Policy1&& _p1, _Policy2&& _p2)
{
    return override_and_policy<
        typename std::decay<_Policy1>::type,
        typename std::decay<_Policy2>::type>(
            static_cast<_Policy1&&>(_p1),
            static_cast<_Policy2&&>(_p2));
}

// override_or_policy
//   policy: permits override when EITHER inner policy
// permits it.
template<typename _Policy1,
         typename _Policy2>
class override_or_policy
{
public:
    template<typename _P1Fwd,
             typename _P2Fwd>
    D_CONSTEXPR
    override_or_policy(_P1Fwd&& _p1, _P2Fwd&& _p2)
        : m_p1(static_cast<_P1Fwd&&>(_p1)),
          m_p2(static_cast<_P2Fwd&&>(_p2))
    {}

    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key&  _k,
           std::size_t  _depth) const
    {
        return ( m_p1.allows(_k, _depth) ||
                 m_p2.allows(_k, _depth) );
    }

    D_CONSTEXPR const _Policy1& first()  const { return m_p1; }
    D_CONSTEXPR const _Policy2& second() const { return m_p2; }

private:
    _Policy1 m_p1;
    _Policy2 m_p2;
};

// override_or
//   function: creates a combined OR policy.
template<typename _Policy1,
         typename _Policy2>
D_CONSTEXPR
override_or_policy<typename std::decay<_Policy1>::type,
                   typename std::decay<_Policy2>::type>
override_or(_Policy1&& _p1, _Policy2&& _p2)
{
    return override_or_policy<
        typename std::decay<_Policy1>::type,
        typename std::decay<_Policy2>::type>(
            static_cast<_Policy1&&>(_p1),
            static_cast<_Policy2&&>(_p2));
}

// override_not_policy
//   policy: inverts another policy.
template<typename _Policy>
class override_not_policy
{
public:
    template<typename _PFwd>
    D_CONSTEXPR explicit
    override_not_policy(_PFwd&& _p)
        : m_p(static_cast<_PFwd&&>(_p))
    {}

    template<typename _Key>
    D_CONSTEXPR bool
    allows(const _Key&  _k,
           std::size_t  _depth) const
    {
        return !m_p.allows(_k, _depth);
    }

    D_CONSTEXPR const _Policy& inner() const { return m_p; }

private:
    _Policy m_p;
};

// override_not
//   function: creates an inverted policy.
template<typename _Policy>
D_CONSTEXPR
override_not_policy<typename std::decay<_Policy>::type>
override_not(_Policy&& _p)
{
    return override_not_policy<
        typename std::decay<_Policy>::type>(
            static_cast<_Policy&&>(_p));
}


// ===========================================================================
// V.   Policy Classification
// ===========================================================================


// override_policy_class
//   struct: compile-time classification of an override
// policy type.
template<typename _Type>
struct override_policy_class
{
    using clean_type = clean_t<_Type>;

    // structural contract
    static constexpr bool is_policy =
        is_override_policy_v<clean_type>;

    // built-in identification (by type)
    static constexpr bool is_allow_all =
        std::is_same<clean_type,
                     override_allow_all>::value;
    static constexpr bool is_deny_all =
        std::is_same<clean_type,
                     override_deny_all>::value;

    // whether the policy is trivially constant
    // (allows everything or denies everything)
    static constexpr bool is_trivial =
        ( is_allow_all || is_deny_all );
};



NS_END  // djinterp


#endif  // DJINTERP_OPTION_OVERRIDE_POLICY_
