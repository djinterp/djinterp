// djinterp [test] : adapter_tests_traits.cpp
//   The adaptation-trait layer (section III): the internal structural probes
// (has_value_type / has_size_method / has_begin_end / has_push_back /
// has_insert / has_subscript_operator), value-type compatibility and the
// invocable-mapping probe, the public traits (are_value_type_compatible,
// is_structurally_adaptable, is_invocable_adapter and their _v forms), and the
// adaptation_class aggregate.

// std
#include <set>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "adapter_tests.hpp"


NS_DJINTERP
NS_TESTING

// shorthand for the internal trait namespace exercised throughout this TU.
namespace di = ::djinterp::internal;

namespace
{
    // ---- structural probe types: each carries exactly one feature so a trait
    // ---- can be verified in isolation ----

    // probe_none
    //   type: no adaptable surface at all.
    struct probe_none
    {
    };

    // probe_value
    //   type: exposes value_type only.
    struct probe_value
    {
        using value_type = int;
    };

    // probe_sized
    //   type: exposes a const size() only.
    struct probe_sized
    {
        std::size_t
        size() const
        {
            return 3;
        }
    };

    // probe_iterable
    //   type: exposes begin()/end() only.
    struct probe_iterable
    {
        int m[3];

        int*
        begin()
        {
            return m;
        }

        int*
        end()
        {
            return m + 3;
        }
    };

    // probe_indexable
    //   type: exposes operator[](size_t) only.
    struct probe_indexable
    {
        int
        operator[](
            std::size_t
        )
        {
            return 0;
        }
    };

    // probe_pushable
    //   type: value_type + push_back(value_type).
    struct probe_pushable
    {
        using value_type = int;

        void
        push_back(
            value_type
        )
        {
        }
    };

    // probe_insertable
    //   type: value_type + insert(value_type).
    struct probe_insertable
    {
        using value_type = int;

        void
        insert(
            value_type
        )
        {
        }
    };

    // ---- invocable-mapping functors ----

    // int_to_long
    //   type: maps an int& to a long.
    struct int_to_long
    {
        long
        operator()(
            int&
        ) const
        {
            return 0;
        }
    };
}


/*
tests_has_value_type
  Verifies internal::has_value_type.
  Tests the following:
  - true for a type exposing value_type (probe_value, std::vector<int>)
  - false for a type without it (probe_none, int)
*/
bool
tests_has_value_type()
{
    static_assert( di::has_value_type<probe_value>::value,       "value_type present");
    static_assert(!di::has_value_type<probe_none>::value,        "value_type absent");

    bool ok = true;

    ok = ok && ( di::has_value_type<probe_value>::value);
    ok = ok && ( di::has_value_type<std::vector<int>>::value);
    ok = ok && (!di::has_value_type<probe_none>::value);
    ok = ok && (!di::has_value_type<int>::value);

    return ok;
}

/*
tests_has_size_method
  Verifies internal::has_size_method (probes a const size()).
  Tests the following:
  - true for a type with a const size() (probe_sized, std::vector<int>)
  - false for a type without one (probe_none, probe_iterable)
*/
bool
tests_has_size_method()
{
    static_assert( di::has_size_method<probe_sized>::value,      "size() present");
    static_assert(!di::has_size_method<probe_none>::value,       "size() absent");

    bool ok = true;

    ok = ok && ( di::has_size_method<probe_sized>::value);
    ok = ok && ( di::has_size_method<std::vector<int>>::value);
    ok = ok && (!di::has_size_method<probe_none>::value);
    ok = ok && (!di::has_size_method<probe_iterable>::value);

    return ok;
}

/*
tests_has_begin_end
  Verifies internal::has_begin_end.
  Tests the following:
  - true for a type with begin()/end() (probe_iterable, std::vector<int>)
  - false for a type without them (probe_none, probe_sized)
*/
bool
tests_has_begin_end()
{
    static_assert( di::has_begin_end<probe_iterable>::value,     "begin/end present");
    static_assert(!di::has_begin_end<probe_sized>::value,        "begin/end absent");

    bool ok = true;

    ok = ok && ( di::has_begin_end<probe_iterable>::value);
    ok = ok && ( di::has_begin_end<std::vector<int>>::value);
    ok = ok && (!di::has_begin_end<probe_none>::value);
    ok = ok && (!di::has_begin_end<probe_sized>::value);

    return ok;
}

/*
tests_has_push_back
  Verifies internal::has_push_back.
  Tests the following:
  - true for a type with push_back(value_type) (probe_pushable, vector)
  - false for value_type-only or insert-only types (probe_value, std::set)
*/
bool
tests_has_push_back()
{
    static_assert( di::has_push_back<probe_pushable>::value,     "push_back present");
    static_assert(!di::has_push_back<probe_value>::value,        "push_back absent");

    bool ok = true;

    ok = ok && ( di::has_push_back<probe_pushable>::value);
    ok = ok && ( di::has_push_back<std::vector<int>>::value);
    ok = ok && (!di::has_push_back<probe_value>::value);
    ok = ok && (!di::has_push_back<std::set<int>>::value);

    return ok;
}

/*
tests_has_insert
  Verifies internal::has_insert (single-argument insert(value_type)).
  Tests the following:
  - true for a type with insert(value_type) (probe_insertable, std::set)
  - false for probe_value and std::vector (whose insert needs an iterator)
*/
bool
tests_has_insert()
{
    static_assert( di::has_insert<probe_insertable>::value,      "insert present");
    static_assert(!di::has_insert<probe_value>::value,          "insert absent");

    bool ok = true;

    ok = ok && ( di::has_insert<probe_insertable>::value);
    ok = ok && ( di::has_insert<std::set<int>>::value);
    ok = ok && (!di::has_insert<probe_value>::value);
    ok = ok && (!di::has_insert<std::vector<int>>::value);

    return ok;
}

/*
tests_has_subscript_operator
  Verifies internal::has_subscript_operator.
  Tests the following:
  - true for a type with operator[](size_t) (probe_indexable, vector)
  - false for a type without one (probe_none, std::set)
*/
bool
tests_has_subscript_operator()
{
    static_assert( di::has_subscript_operator<probe_indexable>::value, "subscript present");
    static_assert(!di::has_subscript_operator<probe_none>::value,      "subscript absent");

    bool ok = true;

    ok = ok && ( di::has_subscript_operator<probe_indexable>::value);
    ok = ok && ( di::has_subscript_operator<std::vector<int>>::value);
    ok = ok && (!di::has_subscript_operator<probe_none>::value);
    ok = ok && (!di::has_subscript_operator<std::set<int>>::value);

    return ok;
}

/*
tests_value_types_compatible
  Verifies internal::value_types_compatible.
  Tests the following:
  - true when both expose value_type and From's is convertible to To's
  - false on an inconvertible value_type pair (int -> std::string)
  - false when either side lacks value_type
*/
bool
tests_value_types_compatible()
{
    static_assert( di::value_types_compatible<std::vector<int>,
                                              std::vector<long>>::value,
                  "int -> long compatible");
    static_assert(!di::value_types_compatible<std::vector<int>,
                                              std::vector<std::string>>::value,
                  "int -> string incompatible");

    bool ok = true;

    ok = ok && ( di::value_types_compatible<std::vector<int>,
                                            std::vector<long>>::value);
    ok = ok && ( di::value_types_compatible<std::vector<int>,
                                            std::vector<int>>::value);
    ok = ok && (!di::value_types_compatible<std::vector<int>,
                                            std::vector<std::string>>::value);
    ok = ok && (!di::value_types_compatible<probe_none,
                                            std::vector<int>>::value);
    ok = ok && (!di::value_types_compatible<std::vector<int>,
                                            probe_none>::value);

    return ok;
}

/*
tests_is_invocable_mapping
  Verifies internal::is_invocable_mapping.
  Tests the following:
  - true when Fn(From&) is valid and its result converts to To
  - false when Fn is not callable with From&
  - false when Fn's result is not convertible to To
*/
bool
tests_is_invocable_mapping()
{
    static_assert( di::is_invocable_mapping<int_to_long, int, long>::value,
                  "int& -> long, convertible to long");
    static_assert(!di::is_invocable_mapping<int_to_long, std::string, long>::value,
                  "not callable with string&");

    bool ok = true;

    ok = ok && ( di::is_invocable_mapping<int_to_long, int, long>::value);
    ok = ok && ( di::is_invocable_mapping<int_to_long, int, double>::value);
    ok = ok && (!di::is_invocable_mapping<int_to_long, std::string, long>::value);
    ok = ok && (!di::is_invocable_mapping<int_to_long, int, std::string>::value);

    return ok;
}

/*
tests_are_value_type_compatible
  Verifies the public are_value_type_compatible trait (and its _v form),
  including that clean_t strips cv/ref before probing.
  Tests the following:
  - true for convertible value types, even through references / const
  - false for inconvertible value types and for non-container types
  - the _v alias mirrors the trait's ::value
*/
bool
tests_are_value_type_compatible()
{
    static_assert( are_value_type_compatible<std::vector<int>,
                                             std::vector<long>>::value,
                  "int -> long");
    static_assert( are_value_type_compatible<std::vector<int>&,
                                             const std::vector<long>&>::value,
                  "clean_t strips ref/const");

    bool ok = true;

    ok = ok && ( are_value_type_compatible<std::vector<int>,
                                           std::vector<long>>::value);
    ok = ok && ( are_value_type_compatible<std::vector<int>&,
                                           const std::vector<long>&>::value);
    ok = ok && (!are_value_type_compatible<std::vector<int>,
                                           std::vector<std::string>>::value);
    ok = ok && (!are_value_type_compatible<int, int>::value);

    // _v mirrors ::value
    ok = ok && (are_value_type_compatible_v<std::vector<int>, std::vector<long>> ==
                are_value_type_compatible<std::vector<int>, std::vector<long>>::value);
    ok = ok && (are_value_type_compatible_v<int, int> == false);

    return ok;
}

/*
tests_is_structurally_adaptable
  Verifies is_structurally_adaptable (and its _v form): true when the adaptee
  and target share an iterable, sized, OR indexable interface.
  Tests the following:
  - true via a shared begin/end interface
  - true via a shared size() interface
  - true via a shared operator[] interface
  - false when there is no shared structural surface
  - the _v alias mirrors the trait's ::value
*/
bool
tests_is_structurally_adaptable()
{
    static_assert( is_structurally_adaptable<std::vector<int>,
                                             std::vector<long>>::value,
                  "both iterable");
    static_assert(!is_structurally_adaptable<probe_sized,
                                             probe_iterable>::value,
                  "no shared surface");

    bool ok = true;

    // shared begin/end
    ok = ok && ( is_structurally_adaptable<std::vector<int>,
                                           std::vector<long>>::value);
    // shared size()
    ok = ok && ( is_structurally_adaptable<probe_sized, probe_sized>::value);
    // shared operator[]
    ok = ok && ( is_structurally_adaptable<probe_indexable,
                                           probe_indexable>::value);
    // disjoint surfaces
    ok = ok && (!is_structurally_adaptable<probe_sized, probe_iterable>::value);
    ok = ok && (!is_structurally_adaptable<probe_none, probe_none>::value);

    // _v mirrors ::value
    ok = ok && (is_structurally_adaptable_v<probe_sized, probe_sized> ==
                is_structurally_adaptable<probe_sized, probe_sized>::value);

    return ok;
}

/*
tests_is_invocable_adapter
  Verifies the public is_invocable_adapter trait (and its _v form).
  Tests the following:
  - true when Fn maps From to something convertible to To
  - false when Fn cannot be called with From&
  - the _v alias mirrors the trait's ::value
*/
bool
tests_is_invocable_adapter()
{
    static_assert( is_invocable_adapter<int_to_long, int, long>::value,
                  "invocable mapping");
    static_assert(!is_invocable_adapter<int_to_long, std::string, long>::value,
                  "not invocable");

    bool ok = true;

    ok = ok && ( is_invocable_adapter<int_to_long, int, long>::value);
    ok = ok && (!is_invocable_adapter<int_to_long, std::string, long>::value);

    ok = ok && (is_invocable_adapter_v<int_to_long, int, long> ==
                is_invocable_adapter<int_to_long, int, long>::value);

    return ok;
}

/*
tests_adaptation_class
  Verifies the adaptation_class aggregate classification across three adaptees
  chosen to drive every member true and false at least once.
  Tests the following:
  - vector<int>: values compatible, structurally adaptable, iterable, sized,
    indexable, has push_back, NO single-arg insert, directly adaptable
  - set<int>: iterable + sized, has insert, NO push_back, NOT indexable
  - probe_none: every classification flag false
*/
bool
tests_adaptation_class()
{
    using vv = adaptation_class<std::vector<int>, std::vector<int>>;
    using ss = adaptation_class<std::set<int>,    std::set<int>>;
    using nn = adaptation_class<probe_none,       probe_none>;

    bool ok = true;

    // vector<int> / vector<int>
    ok = ok && ( vv::compatible_values);
    ok = ok && ( vv::structurally_adaptable);
    ok = ok && ( vv::adaptee_iterable);
    ok = ok && ( vv::target_iterable);
    ok = ok && ( vv::adaptee_sized);
    ok = ok && ( vv::target_sized);
    ok = ok && ( vv::adaptee_indexable);
    ok = ok && ( vv::target_indexable);
    ok = ok && ( vv::adaptee_has_push_back);
    ok = ok && (!vv::adaptee_has_insert);        // vector has no single-arg insert
    ok = ok && ( vv::is_directly_adaptable);

    // set<int> / set<int>
    ok = ok && ( ss::compatible_values);
    ok = ok && ( ss::structurally_adaptable);
    ok = ok && ( ss::adaptee_iterable);
    ok = ok && ( ss::adaptee_sized);
    ok = ok && (!ss::adaptee_indexable);         // set has no operator[]
    ok = ok && (!ss::adaptee_has_push_back);     // set has no push_back
    ok = ok && ( ss::adaptee_has_insert);        // set has single-arg insert
    ok = ok && ( ss::is_directly_adaptable);

    // probe_none / probe_none
    ok = ok && (!nn::compatible_values);
    ok = ok && (!nn::structurally_adaptable);
    ok = ok && (!nn::adaptee_iterable);
    ok = ok && (!nn::target_iterable);
    ok = ok && (!nn::adaptee_sized);
    ok = ok && (!nn::target_sized);
    ok = ok && (!nn::adaptee_indexable);
    ok = ok && (!nn::target_indexable);
    ok = ok && (!nn::adaptee_has_push_back);
    ok = ok && (!nn::adaptee_has_insert);
    ok = ok && (!nn::is_directly_adaptable);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
