/******************************************************************************
* djinterp [testing]                                    contravariant_tests.hpp
*
* Unit test suite for contravariant.hpp -- the contravariant functor protocol.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_CV_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       contravariant_spec(), the module -> block -> test description.
*
*   THE MODULE SHIPS NO INSTANCES.  contravariant.hpp is a pure protocol
* header: a primary trait, a detector, a value_type extractor, one generic
* operation, and a concept.  Nothing in the tree registers itself with it yet,
* so this suite carries its own instance zoo and registers it against
* contravariant_traits.  Because a trait specialization must live in the
* namespace of its primary, the fixture region briefly closes `testing`, opens
* the specializations at `djinterp` scope, and reopens `testing`; the tests
* themselves stay flat in djinterp::testing throughout.
*
*   THE INSTANCE ZOO (fixtures, normal mode only)
*     to_string_of<A>   the documented serializer (A -> std::string); plain
*                       partial specialization; carries `rebind`.
*     predicate_of<A>   a predicate (A -> bool) whose traits record the value
*                       category of both arguments, so the suite can prove the
*                       generic contramap forwards rather than copies.
*     sink_of<A>        a consumer (A -> void, appends to a log) registered
*                       through the trait's SECOND parameter -- an enable_if
*                       hook -- rather than by naming the template; deliberately
*                       omits `rebind`, which the protocol makes optional.
*     ct_predicate<A,F> a literal-type predicate whose contramap is constexpr,
*                       used to pin D_CONSTEXPR on the generic operation.
*     ref_consumer      trait-shape fixture only: registers `value_type` as
*                       `const std::string&` to pin that value_type is NOT
*                       decayed (only the context type is).
*     marker_only / value_only / false_marked / hostile_marked / plain_probe /
*     never_defined     edge fixtures for the detector; see their comments.
*
*   MODULE NOTES (observed behaviour this suite pins, not defects)
*     1. contravariant_value_type<F> is documented "SFINAE-friendly", but the
*        soft-failure seam is the INTERNAL helper: the public wrapper declares
*        `using type = ...helper<void, decay<F>>::type;` in its class body, so
*        instantiating it for an unregistered F is a HARD error, not a
*        substitution failure.  Section `structural` therefore probes
*        internal::contravariant_value_type_helper directly and never
*        instantiates the wrapper on a non-instance.
*     2. is_contravariant keys on the PRESENCE of `is_specialized`, not on its
*        truth: a specialization declaring `is_specialized = std::false_type`
*        is still detected.  The marker must, however, be value-initializable
*        (`is_specialized{}` appears in the detector), so a marker type with a
*        deleted default constructor SFINAEs the detection back to false.
*     3. is_contravariant and contravariant_value_type are independent: the
*        first needs only `is_specialized`, the second only `value_type`.  Both
*        one-sided registrations are exercised.
*     4. `contramap` is constrained by expression validity, not by
*        is_contravariant, so a call on an unregistered type is a hard
*        resolution error by design.  The suite never makes such a call; the
*        negative direction is tested against a REGISTERED instance handed an
*        adapter its own contramap cannot accept.
*
*   BUILD PREREQUISITE: none.  contravariant.hpp compiles as shipped.
*
* CONTENTS
*   I.    PROTOCOL      (contravariant_tests_protocol.cpp)
*   II.   STRUCTURAL    (contravariant_tests_structural.cpp)
*   III.  CONTRAMAP     (contravariant_tests_contramap.cpp)
*   IV.   LAWS          (contravariant_tests_laws.cpp)
*   V.    CONCEPTS      (contravariant_tests_concepts.cpp)
*
* path:      /tests/djinterp/core/functional/contravariant_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TESTS_CONTRAVARIANT_TESTS_
#define DJINTERP_TESTS_CONTRAVARIANT_TESTS_ 1

// std
#include <cstdio>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// -- (part 1) mode-gated includes --------------------------------------------
// djinterp core is ALWAYS first and unconditional (NS_*, D_* qualifiers,
// language gates, void_t), so both faces have it.
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "contravariant.hpp"                    // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


// D_CV_CONSTEXPR_TESTS
//   macro: 1 when the constant-expression tests can be compiled -- C++14 or
// later (std::forward is not constexpr before C++14, and the generic contramap
// forwards) AND the qualifier layer is not stripping constexpr for test
// instrumentation.  0 otherwise, in which case the constexpr block is omitted
// from the declarations and from the spec in lockstep.
#if defined(D_INTERNAL_QUAL_STRIP_CONSTEXPR) && (D_INTERNAL_QUAL_STRIP_CONSTEXPR)
#   define D_CV_CONSTEXPR_TESTS 0
#elif D_ENV_LANG_IS_CPP14_OR_HIGHER
#   define D_CV_CONSTEXPR_TESTS 1
#else
#   define D_CV_CONSTEXPR_TESTS 0
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY,
// because the spec provider (spec mode) needs dt::module_spec too.
namespace dt = ::djinterp::test;


// contravariant_check
//   function: routes one D_CV_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition straight back.  Self-contained
// (<cstdio> only), so it lives above the fixture guard.
inline bool
contravariant_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_CV_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression (std::is_same<A, B>::value) passes through
// whole.  The `CV` suffix is this suite's unique two letters.
#define D_CV_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::contravariant_check(                        \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   ADAPTER-DOMAIN DEDUCTION                              ///
///////////////////////////////////////////////////////////////////////////////
//   contramap's result type is F<B>, where B is the DOMAIN of the adapter
// g : B -> A.  A bare lambda does not advertise its domain, so the instances
// below recover it from the callable's declared shape.  Every step is
// soft-failing (primary templates carry no `type`), which is what makes the
// negative expression-SFINAE test in section `contramap` possible.

// unary_domain_impl
//   trait: primary -- an unrecognized call signature yields no `type`.
template<typename _Signature>
struct unary_domain_impl
{};

// unary_domain_impl (const member call operator)
template<typename _Class,
         typename _Result,
         typename _Argument>
struct unary_domain_impl<_Result (_Class::*)(_Argument) const>
{
    using type = typename std::decay<_Argument>::type;
};

// unary_domain_impl (mutable member call operator)
template<typename _Class,
         typename _Result,
         typename _Argument>
struct unary_domain_impl<_Result (_Class::*)(_Argument)>
{
    using type = typename std::decay<_Argument>::type;
};

// unary_domain
//   trait: primary -- no `type` unless one of the specializations below
// applies.  Soft failure, never a hard error.
template<typename _Function,
         typename _Enable = void>
struct unary_domain
{};

// unary_domain (callable object with a non-generic operator())
template<typename _Function>
struct unary_domain<_Function, void_t<decltype(&_Function::operator())> >
    : unary_domain_impl<decltype(&_Function::operator())>
{};

// unary_domain (plain function pointer)
template<typename _Result,
         typename _Argument>
struct unary_domain<_Result (*)(_Argument), void>
{
    using type = typename std::decay<_Argument>::type;
};

// unary_domain_t
//   type: the decayed domain of a unary callable, after decaying the callable
// itself.  Substitution failure is in the immediate context, so an adapter of
// the wrong shape removes an instance's contramap from overload resolution
// rather than hard-erroring.
template<typename _Function>
using unary_domain_t =
    typename unary_domain<typename std::decay<_Function>::type>::type;


///////////////////////////////////////////////////////////////////////////////
///             F.2   OBSERVATION STATE                                     ///
///////////////////////////////////////////////////////////////////////////////

// forward_record
//   struct: what predicate_of's contramap saw about how the generic contramap
// handed its two arguments over -- the evidence for the forwarding test.
struct forward_record
{
    bool saw_call;
    bool adapter_is_lvalue;
    bool context_is_lvalue;
    bool context_is_const;

    forward_record()
        : saw_call(false),
          adapter_is_lvalue(false),
          context_is_lvalue(false),
          context_is_const(false)
    {}
};

// forward_log
//   function: the one process-wide forward_record.  A function-local static
// keeps a single instance across every section TU without an inline variable.
inline forward_record&
forward_log()
{
    static forward_record record;

    return record;
}

// reset_forward_log
//   function: clear the forwarding evidence before a new observation.
inline void
reset_forward_log()
{
    forward_log() = forward_record();

    return;
}

// order_log
//   function: the one process-wide call-order string.  Instances append a
// letter as they run, so a test can prove the adapter fires BEFORE the context.
inline std::string&
order_log()
{
    static std::string log;

    return log;
}


///////////////////////////////////////////////////////////////////////////////
///             F.3   THE INSTANCE ZOO                                      ///
///////////////////////////////////////////////////////////////////////////////

// to_string_of
//   struct: a serializer, A -> std::string.  The header's documented
// contravariant instance: knowing how to show an A plus a way to turn a B into
// an A is knowing how to show a B.
template<typename _Type>
struct to_string_of
{
    using element_type = _Type;
    using function_type = std::function<std::string(const _Type&)>;

    function_type fn;

    std::string
    run(
        const _Type& _value
    ) const
    {
        return fn(_value);
    }
};

// predicate_of
//   struct: a predicate, A -> bool.  Registered with a contramap that records
// how its arguments arrived (see forward_record).
template<typename _Type>
struct predicate_of
{
    using element_type = _Type;
    using function_type = std::function<bool(const _Type&)>;

    function_type fn;

    bool
    run(
        const _Type& _value
    ) const
    {
        return fn(_value);
    }
};

// sink_of
//   struct: a consumer, A -> void, appending a rendered line to a caller-owned
// log.  Registered through the trait's SFINAE hook rather than by name.
template<typename _Type>
struct sink_of
{
    using element_type = _Type;
    using function_type = std::function<std::string(const _Type&)>;

    std::vector<std::string>* log;
    function_type             render;

    void
    accept(
        const _Type& _value
    ) const
    {
        log->push_back(render(_value));

        return;
    }
};

// is_sink_context
//   trait: primary -- false for everything that is not a sink_of.
template<typename _Type>
struct is_sink_context
    : std::false_type
{};

// is_sink_context (sink_of family)
template<typename _Type>
struct is_sink_context< sink_of<_Type> >
    : std::true_type
{};

// ct_composed
//   struct: the literal-type composition an adapter and a predicate body fold
// into.  `inner` (the adapter) runs first, then `outer` (the original body) --
// pre-composition, spelled out as data so it stays a literal type.
template<typename _Outer,
         typename _Inner>
struct ct_composed
{
    _Inner inner;
    _Outer outer;

    template<typename _Value>
    D_CONSTEXPR bool
    operator()(
        const _Value& _value
    ) const
    {
        return outer(inner(_value));
    }
};

// ct_predicate
//   struct: an aggregate predicate over a literal callable, so contramap on it
// is usable inside a constant expression.
template<typename _Type,
         typename _Function>
struct ct_predicate
{
    using element_type = _Type;

    _Function fn;

    D_CONSTEXPR bool
    run(
        const _Type& _value
    ) const
    {
        return fn(_value);
    }
};

// ct_is_big
//   struct: literal predicate body, long -> bool.
struct ct_is_big
{
    D_CONSTEXPR bool
    operator()(
        long _value
    ) const
    {
        return _value > 10;
    }
};

// ct_doubled
//   struct: literal adapter, int -> long.  Its domain (int) differs from
// ct_is_big's domain (long), so the contramapped type visibly changes.
struct ct_doubled
{
    D_CONSTEXPR long
    operator()(
        int _value
    ) const
    {
        return static_cast<long>(_value) * 2;
    }
};

// ref_consumer
//   struct: trait-shape fixture only -- registered so that its `value_type` is
// a reference-to-const, pinning that contravariant_value_type reports what the
// instance declared and does not decay it.  Not a lawful instance (it has no
// contramap) and never handed to the generic operation.
struct ref_consumer
{
    std::string tag;
};


///////////////////////////////////////////////////////////////////////////////
///             F.4   DETECTOR EDGE FIXTURES                                ///
///////////////////////////////////////////////////////////////////////////////

// no_default
//   struct: a type that cannot be value-initialized.  Used as an
// `is_specialized` marker to show the detector requires `is_specialized{}` to
// be well-formed.
struct no_default
{
    no_default() = delete;
};

// marker_only
//   struct: registered with `is_specialized` but NO `value_type` -- detected as
// contravariant, yet carries no inner type.
struct marker_only
{};

// value_only
//   struct: registered with `value_type` but NO `is_specialized` -- NOT
// detected as contravariant, yet the value_type extractor still resolves.
struct value_only
{};

// false_marked
//   struct: registered with `is_specialized = std::false_type`.  The detector
// keys on presence, not truth, so this is still detected.
struct false_marked
{};

// hostile_marked
//   struct: registered with an `is_specialized` that has a deleted default
// constructor, so `is_specialized{}` is ill-formed and detection SFINAEs back
// to false.
struct hostile_marked
{};

// plain_probe
//   struct: a complete class type with no registration at all.
struct plain_probe
{};

// never_defined
//   struct: declared and never defined.  contravariant_traits<never_defined> is
// incomplete either way, so detection is a clean false.
struct never_defined;


///////////////////////////////////////////////////////////////////////////////
///             F.5   COMPILE-TIME PROBES                                   ///
///////////////////////////////////////////////////////////////////////////////

// has_inner_value_type
//   trait: does internal::contravariant_value_type_helper resolve a `type` for
// _Type?  This is the module's REAL soft-failure seam (see MODULE NOTES 1), so
// the negative direction is probed here rather than on the public wrapper.
template<typename _Type,
         typename _Enable = void>
struct has_inner_value_type
    : std::false_type
{};

// has_inner_value_type (well-formed specialization)
template<typename _Type>
struct has_inner_value_type<
    _Type,
    void_t<typename internal::contravariant_value_type_helper<void, _Type>::type> >
    : std::true_type
{};

// has_rebind
//   trait: does contravariant_traits<_Contravariant> expose rebind<_Other>?
// `rebind` is optional in the protocol, so both answers are legitimate.
template<typename _Contravariant,
         typename _Other,
         typename _Enable = void>
struct has_rebind
    : std::false_type
{};

// has_rebind (well-formed specialization)
template<typename _Contravariant,
         typename _Other>
struct has_rebind<
    _Contravariant,
    _Other,
    void_t<typename contravariant_traits<_Contravariant>::template rebind<_Other> > >
    : std::true_type
{};

// can_contramap
//   trait: is `contramap(_Function, _Contravariant)` a well-formed expression?
// SAFE ONLY FOR REGISTERED _Contravariant: on an unregistered context the
// generic operation's trailing return type reaches an incomplete
// contravariant_traits, which is a hard error by design (MODULE NOTES 4).
template<typename _Function,
         typename _Contravariant,
         typename _Enable = void>
struct can_contramap
    : std::false_type
{};

// can_contramap (well-formed specialization)
template<typename _Function,
         typename _Contravariant>
struct can_contramap<
    _Function,
    _Contravariant,
    void_t<decltype(::djinterp::contramap(std::declval<_Function>(),
                                          std::declval<_Contravariant>()))> >
    : std::true_type
{};


NS_END  // testing -- reopened below; trait specializations belong to djinterp


///////////////////////////////////////////////////////////////////////////////
///             F.6   REGISTRATIONS (namespace djinterp)                    ///
///////////////////////////////////////////////////////////////////////////////

// contravariant_traits (to_string_of)
//   trait: the documented serializer instance.  Plain partial specialization on
// the template, with the optional `rebind` supplied.
template<typename _Type>
struct contravariant_traits< ::djinterp::testing::to_string_of<_Type> >
{
    using value_type     = _Type;
    using is_specialized = std::true_type;

    template<typename _Other>
    using rebind = ::djinterp::testing::to_string_of<_Other>;

    template<typename _Function,
             typename _Show>
    static ::djinterp::testing::to_string_of<
               ::djinterp::testing::unary_domain_t<_Function> >
    contramap(
        _Function&& _g,
        _Show&&     _fa
    )
    {
        using domain_type  = ::djinterp::testing::unary_domain_t<_Function>;
        using adapter_type = typename std::decay<_Function>::type;

        ::djinterp::testing::to_string_of<_Type> inner(_fa);
        adapter_type                             adapter(std::forward<_Function>(_g));

        return ::djinterp::testing::to_string_of<domain_type>{
            [inner, adapter](const domain_type& _value)
            {
                return inner.run(adapter(_value));
            } };
    }
};

// contravariant_traits (predicate_of)
//   trait: a second, independent instance.  Its contramap records the value
// category and constness of both incoming arguments, which is how the suite
// observes that the generic operation forwards them.
template<typename _Type>
struct contravariant_traits< ::djinterp::testing::predicate_of<_Type> >
{
    using value_type     = _Type;
    using is_specialized = std::true_type;

    template<typename _Other>
    using rebind = ::djinterp::testing::predicate_of<_Other>;

    template<typename _Function,
             typename _Predicate>
    static ::djinterp::testing::predicate_of<
               ::djinterp::testing::unary_domain_t<_Function> >
    contramap(
        _Function&&  _g,
        _Predicate&& _fa
    )
    {
        using domain_type  = ::djinterp::testing::unary_domain_t<_Function>;
        using adapter_type = typename std::decay<_Function>::type;

        ::djinterp::testing::forward_record& record =
            ::djinterp::testing::forward_log();

        record.saw_call          = true;
        record.adapter_is_lvalue = std::is_lvalue_reference<_Function>::value;
        record.context_is_lvalue = std::is_lvalue_reference<_Predicate>::value;
        record.context_is_const  =
            std::is_const<typename std::remove_reference<_Predicate>::type>::value;

        ::djinterp::testing::predicate_of<_Type> inner(_fa);
        adapter_type                             adapter(std::forward<_Function>(_g));

        return ::djinterp::testing::predicate_of<domain_type>{
            [inner, adapter](const domain_type& _value)
            {
                return inner.run(adapter(_value));
            } };
    }
};

// contravariant_traits (sink_of, via the SFINAE hook)
//   trait: registration through the trait's SECOND parameter -- any type the
// is_sink_context predicate accepts is an instance, without naming its
// template.  Deliberately omits `rebind` (optional in the protocol).
template<typename _Contravariant>
struct contravariant_traits<
    _Contravariant,
    typename std::enable_if<
        ::djinterp::testing::is_sink_context<_Contravariant>::value >::type>
{
    using value_type     = typename _Contravariant::element_type;
    using is_specialized = std::true_type;

    template<typename _Function,
             typename _Sink>
    static ::djinterp::testing::sink_of<
               ::djinterp::testing::unary_domain_t<_Function> >
    contramap(
        _Function&& _g,
        _Sink&&     _fa
    )
    {
        using domain_type  = ::djinterp::testing::unary_domain_t<_Function>;
        using adapter_type = typename std::decay<_Function>::type;
        using sink_type    = typename std::decay<_Sink>::type;

        sink_type    inner(_fa);
        adapter_type adapter(std::forward<_Function>(_g));

        return ::djinterp::testing::sink_of<domain_type>{
            inner.log,
            [inner, adapter](const domain_type& _value)
            {
                return inner.render(adapter(_value));
            } };
    }
};

// contravariant_traits (ct_predicate)
//   trait: the literal-type instance.  contramap is a single aggregate
// initialization, so it survives constant evaluation.
template<typename _Type,
         typename _Function>
struct contravariant_traits< ::djinterp::testing::ct_predicate<_Type, _Function> >
{
    using value_type     = _Type;
    using is_specialized = std::true_type;

    template<typename _Adapter,
             typename _Predicate>
    static D_CONSTEXPR ::djinterp::testing::ct_predicate<
               ::djinterp::testing::unary_domain_t<_Adapter>,
               ::djinterp::testing::ct_composed<
                   _Function,
                   typename std::decay<_Adapter>::type> >
    contramap(
        _Adapter&&   _g,
        _Predicate&& _fa
    )
    {
        return { { std::forward<_Adapter>(_g), _fa.fn } };
    }
};

// contravariant_traits (ref_consumer)
//   trait: trait-shape fixture -- declares a reference-to-const value_type so
// the extractor can be shown to report it verbatim.
template<>
struct contravariant_traits< ::djinterp::testing::ref_consumer >
{
    using value_type     = const std::string&;
    using is_specialized = std::true_type;
};

// contravariant_traits (marker_only)
//   trait: marker present, value_type absent.
template<>
struct contravariant_traits< ::djinterp::testing::marker_only >
{
    using is_specialized = std::true_type;
};

// contravariant_traits (value_only)
//   trait: value_type present, marker absent.
template<>
struct contravariant_traits< ::djinterp::testing::value_only >
{
    using value_type = double;
};

// contravariant_traits (false_marked)
//   trait: the marker is present but spells false; detection keys on presence.
template<>
struct contravariant_traits< ::djinterp::testing::false_marked >
{
    using value_type     = char;
    using is_specialized = std::false_type;
};

// contravariant_traits (hostile_marked)
//   trait: the marker is present but cannot be value-initialized.
template<>
struct contravariant_traits< ::djinterp::testing::hostile_marked >
{
    using value_type     = int;
    using is_specialized = ::djinterp::testing::no_default;
};


NS_TESTING  // reopened -- everything below depends on the registrations above


///////////////////////////////////////////////////////////////////////////////
///             F.7   POST-REGISTRATION HELPERS                             ///
///////////////////////////////////////////////////////////////////////////////

// long_to_text
//   function: render a long in decimal without <sstream>, so instance bodies
// and the tests that read their output agree on one spelling.
inline std::string
long_to_text(
    long _value
)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%ld", _value);

    return std::string(buffer);
}

// make_show_long
//   function: the documented `show_long` -- a to_string_of<long> that renders a
// long in decimal without <sstream>.
inline to_string_of<long>
make_show_long()
{
    return to_string_of<long>{
        [](const long& _value)
        {
            return long_to_text(_value);
        } };
}

// string_size
//   function: a plain function (not a lambda) adapting std::string -> long, so
// the suite can hand contramap a function pointer as well as a closure.
inline long
string_size(
    const std::string& _text
)
{
    return static_cast<long>(_text.size());
}

// scaled_adapter
//   struct: a stateful function object, long -> long.  Third adapter shape
// accepted by the generic operation.
struct scaled_adapter
{
    long factor;

    long
    operator()(
        long _value
    ) const
    {
        return _value * factor;
    }
};

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // contramappable_with
    //   concept: _Type is a contravariant context that accepts _Function as an
    // adapter.  The conjunction short-circuits, so a non-instance is rejected
    // by the first operand and the contramap expression is never formed.
    template<typename _Function,
             typename _Type>
    concept contramappable_with =
        Contravariant<_Type> &&
        requires(_Function _f, _Type _t)
        {
            ::djinterp::contramap(_f, _t);
        };

    // overload_taken
    //   function: constrained overload -- chosen for contravariant contexts,
    // being the more constrained of the pair.
    template<Contravariant _Type>
    inline const char*
    overload_taken(
        const _Type&
    )
    {
        return "contravariant";
    }

    // overload_taken (unconstrained)
    //   function: the fallback, chosen for everything else.
    template<typename _Type>
    inline const char*
    overload_taken(
        const _Type&
    )
    {
        return "generic";
    }

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   PROTOCOL   (contravariant_tests_protocol.cpp)
bool tests_contravariant_traits_registration_members();
bool tests_contravariant_traits_rebind_shape();
bool tests_contravariant_detects_registered_instances();
bool tests_contravariant_rejects_unregistered_types();
bool tests_contravariant_detection_decays_cv_ref();
bool tests_contravariant_detection_decays_arrays_and_functions();
bool tests_contravariant_detection_bool_trait_shape();
bool tests_contravariant_enable_hook_specialization();
bool tests_contravariant_marker_presence_not_truth();
bool tests_contravariant_marker_must_be_value_initializable();
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
bool tests_contravariant_variable_template_agreement();
#endif

// II.  STRUCTURAL   (contravariant_tests_structural.cpp)
bool tests_contravariant_value_type_of_instances();
bool tests_contravariant_value_type_alias_agreement();
bool tests_contravariant_value_type_decays_context();
bool tests_contravariant_value_type_preserves_qualifiers();
bool tests_contravariant_value_type_helper_soft_failure();
bool tests_contravariant_value_type_orthogonal_to_marker();
bool tests_contravariant_value_type_nested_and_distinct();
bool tests_contravariant_value_type_of_hook_instance();

// III. CONTRAMAP   (contravariant_tests_contramap.cpp)
bool tests_contramap_documented_usage_example();
bool tests_contramap_result_type_is_rebound_context();
bool tests_contramap_adapts_predicate_domain();
bool tests_contramap_pre_composes_rather_than_post_composes();
bool tests_contramap_forwards_value_categories();
bool tests_contramap_dispatch_keyed_on_decayed_context();
bool tests_contramap_chains_across_three_domains();
bool tests_contramap_leaves_source_context_usable();
bool tests_contramap_through_enable_hook_instance();
bool tests_contramap_accepts_any_unary_callable();
bool tests_contramap_expression_sfinae_on_adapter();
#if D_CV_CONSTEXPR_TESTS
bool tests_contramap_is_usable_in_constant_expressions();
#endif

// IV.  LAWS   (contravariant_tests_laws.cpp)
bool tests_contravariant_law_identity_serializer();
bool tests_contravariant_law_identity_predicate();
bool tests_contravariant_law_identity_hook_instance();
bool tests_contravariant_law_composition_serializer();
bool tests_contravariant_law_composition_predicate();
bool tests_contravariant_law_composition_reverses_arrows();
bool tests_contravariant_law_identity_composition_interaction();
bool tests_contravariant_law_composition_associative_three_stage();
bool tests_contravariant_law_composition_result_types_agree();

// V.   CONCEPTS   (contravariant_tests_concepts.cpp)
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
bool tests_contravariant_concept_accepts_instances();
bool tests_contravariant_concept_rejects_non_instances();
bool tests_contravariant_concept_agrees_with_trait();
bool tests_contravariant_concept_constrains_overload_resolution();
bool tests_contravariant_concept_composes_in_requires_clause();
#endif


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// contravariant_spec
//   function: the authoritative description of this suite -- one block per
// section TU, one row per tests_* body, every node named and described.
inline dt::module_spec
contravariant_spec()
{
    return dt::module_spec{
        "contravariant",
        "The contravariant functor protocol: a type constructor F<A> that "
        "CONSUMES values of A, together with the single operation contramap, "
        "which pre-composes an arrow B -> A onto an F<A> to yield an F<B>. "
        "The header supplies the primary contravariant_traits<F> (undefined, so "
        "misuse is a clean resolution error), the is_contravariant detector, the "
        "contravariant_value_type extractor, the one generic operation, and the "
        "C++20 Contravariant concept. It ships no instances, so this suite "
        "registers its own zoo -- a serializer, a predicate, an enable_if-hook "
        "sink, and a literal-type predicate -- and exercises the protocol, the "
        "structural traits, dispatch and forwarding, the two contravariant laws, "
        "and the concept face against them.",
        {
            dt::block_spec{
                "protocol",
                "Registration through contravariant_traits and detection through "
                "is_contravariant: what a specialization must expose, what the "
                "detector keys on, and how it treats cv-qualification, "
                "references, decay, and marker shapes.",
                {
                    { "tests_contravariant_traits_registration_members",
                      "A registered specialization exposes value_type and the "
                      "is_specialized marker, and the marker is std::true_type "
                      "for each canonical instance.",
                      &tests_contravariant_traits_registration_members },
                    { "tests_contravariant_traits_rebind_shape",
                      "rebind<U> yields F<U> for the instances that supply it, "
                      "is the identity at U = A, and is absent -- legitimately, "
                      "since the protocol makes it optional -- on the hook "
                      "instance.",
                      &tests_contravariant_traits_rebind_shape },
                    { "tests_contravariant_detects_registered_instances",
                      "is_contravariant is true for every registered instance "
                      "across several element types, including nested contexts.",
                      &tests_contravariant_detects_registered_instances },
                    { "tests_contravariant_rejects_unregistered_types",
                      "is_contravariant is false for fundamentals, void, plain "
                      "classes, an incomplete class, std::string, pointers, and "
                      "the trait template's own instantiations.",
                      &tests_contravariant_rejects_unregistered_types },
                    { "tests_contravariant_detection_decays_cv_ref",
                      "Detection strips cv-qualification and references: const, "
                      "volatile, lvalue-ref, rvalue-ref and const-volatile-ref "
                      "spellings all agree with the bare type, in both the true "
                      "and the false direction.",
                      &tests_contravariant_detection_decays_cv_ref },
                    { "tests_contravariant_detection_decays_arrays_and_functions",
                      "std::decay is applied in full, so an array of instances "
                      "collapses to a pointer and a function type to a function "
                      "pointer -- neither of which is contravariant.",
                      &tests_contravariant_detection_decays_arrays_and_functions },
                    { "tests_contravariant_detection_bool_trait_shape",
                      "is_contravariant has the shape of a standard bool trait: "
                      "it derives from std::integral_constant, exposing value, "
                      "value_type, type, the conversion operator and operator(), "
                      "all usable in a constant expression.",
                      &tests_contravariant_detection_bool_trait_shape },
                    { "tests_contravariant_enable_hook_specialization",
                      "A specialization keyed on the trait's second (SFINAE) "
                      "parameter registers a whole family at once and is "
                      "detected exactly like a named one, while a sibling type "
                      "the hook rejects stays undetected.",
                      &tests_contravariant_enable_hook_specialization },
                    { "tests_contravariant_marker_presence_not_truth",
                      "The detector keys on the PRESENCE of is_specialized, not "
                      "its value: a specialization declaring "
                      "is_specialized = std::false_type is still detected.",
                      &tests_contravariant_marker_presence_not_truth },
                    { "tests_contravariant_marker_must_be_value_initializable",
                      "The detector value-initializes the marker, so a marker "
                      "type with a deleted default constructor SFINAEs "
                      "detection back to false while the specialization itself "
                      "remains perfectly visible.",
                      &tests_contravariant_marker_must_be_value_initializable },
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
                    { "tests_contravariant_variable_template_agreement",
                      "is_contravariant_v agrees with is_contravariant<T>::value "
                      "over the whole battery and is itself a constant "
                      "expression. [C++14+]",
                      &tests_contravariant_variable_template_agreement },
#endif
                }
            },
            dt::block_spec{
                "structural",
                "The Section 0 structural vocabulary: contravariant_value_type, "
                "its _t alias, and the internal helper that gives it its "
                "soft-failure behaviour -- what the inner consumed type is, and "
                "when there is none.",
                {
                    { "tests_contravariant_value_type_of_instances",
                      "contravariant_value_type<F>::type is the inner consumed "
                      "type A declared by the instance, for each registered "
                      "context.",
                      &tests_contravariant_value_type_of_instances },
                    { "tests_contravariant_value_type_alias_agreement",
                      "contravariant_value_type_t<F> and "
                      "contravariant_value_type<F>::type name the same type "
                      "everywhere.",
                      &tests_contravariant_value_type_alias_agreement },
                    { "tests_contravariant_value_type_decays_context",
                      "The CONTEXT argument is decayed before lookup, so const, "
                      "volatile, lvalue-ref and rvalue-ref spellings of the same "
                      "instance all report the same inner type.",
                      &tests_contravariant_value_type_decays_context },
                    { "tests_contravariant_value_type_preserves_qualifiers",
                      "The reported value_type is whatever the instance "
                      "declared: a reference-to-const value_type comes back "
                      "reference-to-const, undecayed.",
                      &tests_contravariant_value_type_preserves_qualifiers },
                    { "tests_contravariant_value_type_helper_soft_failure",
                      "The internal extractor helper resolves no `type` for "
                      "unregistered types -- the module's actual SFINAE seam -- "
                      "while resolving one for every registered instance.",
                      &tests_contravariant_value_type_helper_soft_failure },
                    { "tests_contravariant_value_type_orthogonal_to_marker",
                      "Detection and extraction are independent: a "
                      "marker-without-value_type registration is detected yet "
                      "yields no inner type, and a value_type-without-marker "
                      "registration yields one yet is not detected.",
                      &tests_contravariant_value_type_orthogonal_to_marker },
                    { "tests_contravariant_value_type_nested_and_distinct",
                      "Distinct instantiations report distinct inner types, and "
                      "a context nested over another context reports the inner "
                      "context itself.",
                      &tests_contravariant_value_type_nested_and_distinct },
                    { "tests_contravariant_value_type_of_hook_instance",
                      "An instance registered through the SFINAE hook exposes "
                      "its inner type through exactly the same extractor as a "
                      "named specialization.",
                      &tests_contravariant_value_type_of_hook_instance },
                }
            },
            dt::block_spec{
                "contramap",
                "The one generic operation: its documented usage, its deduced "
                "result type, its argument order, the pre-composition it "
                "performs, how it dispatches on the decayed context, how it "
                "forwards both arguments, and its constexpr face.",
                {
                    { "tests_contramap_documented_usage_example",
                      "The header's USAGE block runs verbatim: contramapping a "
                      "string -> long adapter onto a to_string_of<long> yields a "
                      "to_string_of<std::string> whose run(\"hello\") is \"5\".",
                      &tests_contramap_documented_usage_example },
                    { "tests_contramap_result_type_is_rebound_context",
                      "The deduced result type is exactly F<B> for the domain B "
                      "of the adapter, and equals the instance's own rebind<B>.",
                      &tests_contramap_result_type_is_rebound_context },
                    { "tests_contramap_adapts_predicate_domain",
                      "A predicate on long becomes a predicate on std::string "
                      "that agrees with the original on the mapped value, over "
                      "inputs on both sides of the threshold.",
                      &tests_contramap_adapts_predicate_domain },
                    { "tests_contramap_pre_composes_rather_than_post_composes",
                      "The adapter runs BEFORE the context: an instrumented "
                      "pair records the adapter's mark first and the context's "
                      "second, which is the whole difference from a covariant "
                      "map.",
                      &tests_contramap_pre_composes_rather_than_post_composes },
                    { "tests_contramap_forwards_value_categories",
                      "Both arguments are perfectly forwarded to the instance's "
                      "contramap: a non-const lvalue context arrives as a "
                      "non-const lvalue, a const lvalue as a const lvalue, and a "
                      "temporary as an rvalue, with the adapter tracked "
                      "independently.",
                      &tests_contramap_forwards_value_categories },
                    { "tests_contramap_dispatch_keyed_on_decayed_context",
                      "Dispatch is keyed on the decayed context type, so "
                      "lvalue, const-lvalue and rvalue calls all reach the same "
                      "specialization and produce the same result type and "
                      "value.",
                      &tests_contramap_dispatch_keyed_on_decayed_context },
                    { "tests_contramap_chains_across_three_domains",
                      "Two successive contramaps walk a context back across "
                      "three domains, each stage narrowing the consumed type "
                      "while the innermost body is unchanged.",
                      &tests_contramap_chains_across_three_domains },
                    { "tests_contramap_leaves_source_context_usable",
                      "contramap builds a new context rather than consuming the "
                      "old one: the source still answers on its own domain "
                      "afterwards, and the two are independent.",
                      &tests_contramap_leaves_source_context_usable },
                    { "tests_contramap_through_enable_hook_instance",
                      "The generic operation dispatches to a hook-registered "
                      "instance identically, and the value the sink records is "
                      "the pre-converted one.",
                      &tests_contramap_through_enable_hook_instance },
                    { "tests_contramap_accepts_any_unary_callable",
                      "The adapter may be a closure, a plain function pointer, a "
                      "stateful function object, or a std::function; all four "
                      "produce the same adapted context.",
                      &tests_contramap_accepts_any_unary_callable },
                    { "tests_contramap_expression_sfinae_on_adapter",
                      "The trailing return type is expression-SFINAE friendly "
                      "with respect to the instance's own constraints: an "
                      "adapter of the wrong arity removes the call from "
                      "overload resolution instead of hard-erroring.",
                      &tests_contramap_expression_sfinae_on_adapter },
#if D_CV_CONSTEXPR_TESTS
                    { "tests_contramap_is_usable_in_constant_expressions",
                      "D_CONSTEXPR is real: contramapping a literal-type "
                      "predicate is a constant expression, and the adapted "
                      "predicate answers inside static_assert. [C++14+]",
                      &tests_contramap_is_usable_in_constant_expressions },
#endif
                }
            },
            dt::block_spec{
                "laws",
                "The two contravariant functor laws -- contramap(id) == id and "
                "contramap(g) . contramap(f) == contramap(f . g) -- checked "
                "observationally on every lawful instance, including the arrow "
                "reversal that distinguishes a contravariant functor from a "
                "covariant one.",
                {
                    { "tests_contravariant_law_identity_serializer",
                      "Identity law on the serializer: contramapping the "
                      "identity arrow leaves the rendered output unchanged over "
                      "a battery of inputs.",
                      &tests_contravariant_law_identity_serializer },
                    { "tests_contravariant_law_identity_predicate",
                      "Identity law on the predicate: the contramapped context "
                      "agrees with the original on every probe value.",
                      &tests_contravariant_law_identity_predicate },
                    { "tests_contravariant_law_identity_hook_instance",
                      "Identity law on the hook-registered sink: the log "
                      "written through the contramapped sink matches the log "
                      "written through the original.",
                      &tests_contravariant_law_identity_hook_instance },
                    { "tests_contravariant_law_composition_serializer",
                      "Composition law on the serializer: contramap(g) after "
                      "contramap(f) renders identically to contramap(f . g) for "
                      "every input.",
                      &tests_contravariant_law_composition_serializer },
                    { "tests_contravariant_law_composition_predicate",
                      "Composition law on the predicate, over inputs that land "
                      "on both sides of the decision boundary.",
                      &tests_contravariant_law_composition_predicate },
                    { "tests_contravariant_law_composition_reverses_arrows",
                      "The reversal is real, not incidental: with both arrows "
                      "over one domain, contramap(g) after contramap(f) matches "
                      "contramap(f . g) and differs from contramap(g . f).",
                      &tests_contravariant_law_composition_reverses_arrows },
                    { "tests_contravariant_law_identity_composition_interaction",
                      "Identity is neutral on either side of a composition: "
                      "inserting the identity arrow before or after an adapter "
                      "changes nothing.",
                      &tests_contravariant_law_identity_composition_interaction },
                    { "tests_contravariant_law_composition_associative_three_stage",
                      "Three-stage composition agrees under both bracketings and "
                      "with the single fused arrow, so the law composes.",
                      &tests_contravariant_law_composition_associative_three_stage },
                    { "tests_contravariant_law_composition_result_types_agree",
                      "The two sides of the composition law have the same static "
                      "type, not merely the same behaviour.",
                      &tests_contravariant_law_composition_result_types_agree },
                }
            },
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
            dt::block_spec{
                "concepts",
                "The C++20 Contravariant concept: the PascalCase typeclass face "
                "of is_contravariant, its agreement with the trait, and its "
                "behaviour as a constraint in overload resolution and in "
                "composed requires-clauses. [C++20]",
                {
                    { "tests_contravariant_concept_accepts_instances",
                      "Contravariant is satisfied by every registered instance, "
                      "including the hook-registered family and cv-qualified "
                      "spellings.",
                      &tests_contravariant_concept_accepts_instances },
                    { "tests_contravariant_concept_rejects_non_instances",
                      "Contravariant is not satisfied by fundamentals, void, "
                      "plain classes, an incomplete class, or a "
                      "value_type-without-marker registration.",
                      &tests_contravariant_concept_rejects_non_instances },
                    { "tests_contravariant_concept_agrees_with_trait",
                      "Concept satisfaction and is_contravariant<T>::value agree "
                      "term by term over the whole battery.",
                      &tests_contravariant_concept_agrees_with_trait },
                    { "tests_contravariant_concept_constrains_overload_resolution",
                      "A Contravariant-constrained overload is preferred over an "
                      "unconstrained one for instances, and only the "
                      "unconstrained one is viable for everything else.",
                      &tests_contravariant_concept_constrains_overload_resolution },
                    { "tests_contravariant_concept_composes_in_requires_clause",
                      "Contravariant composes into a larger concept whose "
                      "conjunction short-circuits, so a non-instance is rejected "
                      "without ever forming the contramap expression.",
                      &tests_contravariant_concept_composes_in_requires_clause },
                }
            },
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONTRAVARIANT_TESTS_
