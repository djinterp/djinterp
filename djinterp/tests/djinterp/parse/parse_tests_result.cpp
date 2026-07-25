#include <stdexcept>
#include <utility>

#include "parse_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_parse_result_value_construction
  Verifies implicit construction from a value produces the success branch,
so a parser body can simply `return value;`.
  Tests the following:
  - an lvalue source yields a success carrying that value,
  - an rvalue source yields the same,
  - the branch predicates agree with one another,
  - a non-trivial payload type behaves identically to a scalar one.
*/
bool
tests_parse_result_value_construction()
{
    const int source = 42;

    // an lvalue binds the const-reference constructor
    dp::parse_result<int> from_lvalue(source);

    D_PA_CHECK(from_lvalue.ok());
    D_PA_CHECK(from_lvalue.value() == 42);
    D_PA_CHECK(from_lvalue.is_ok());
    D_PA_CHECK(!from_lvalue.is_err());

    // an rvalue binds the rvalue-reference constructor
    dp::parse_result<int> from_rvalue(7 + 1);

    D_PA_CHECK(from_rvalue.ok());
    D_PA_CHECK(from_rvalue.value() == 8);

    // the construction is implicit, so a bare return statement works
    dp::parse_result<int> implicit = 99;

    D_PA_CHECK(implicit.ok());
    D_PA_CHECK(implicit.value() == 99);

    // a payload with a non-trivial destructor takes the same path
    dp::parse_result<std::string> text(std::string("parsed"));

    D_PA_CHECK(text.ok());
    D_PA_CHECK(text.value() == std::string("parsed"));

    return true;
}

/*
tests_parse_result_error_construction
  Verifies implicit construction from a parse_error produces the failure
branch, so a parser body can simply `return error;`.
  Tests the following:
  - an lvalue error yields a failure carrying it unchanged,
  - an rvalue error yields the same,
  - all three error fields survive,
  - the branch predicates agree with one another.
*/
bool
tests_parse_result_error_construction()
{
    const dp::parse_error source(dp::DParseStatusMalformed, 3u, "bad");

    // an lvalue error binds the const-reference constructor
    dp::parse_result<int> from_lvalue(source);

    D_PA_CHECK(!from_lvalue.ok());
    D_PA_CHECK(from_lvalue.is_err());
    D_PA_CHECK(!from_lvalue.is_ok());
    D_PA_CHECK(from_lvalue.error() == source);

    // every field of the descriptor is carried, not just the status
    D_PA_CHECK(from_lvalue.error().status() == dp::DParseStatusMalformed);
    D_PA_CHECK(from_lvalue.error().offset() == 3u);
    D_PA_CHECK(from_lvalue.error().message() == std::string("bad"));

    // an rvalue error binds the rvalue-reference constructor
    dp::parse_result<int> from_rvalue(
        dp::parse_error(dp::DParseStatusOverflow, 8u, "too big"));

    D_PA_CHECK(!from_rvalue.ok());
    D_PA_CHECK(from_rvalue.error().status() == dp::DParseStatusOverflow);
    D_PA_CHECK(from_rvalue.error().offset() == 8u);

    // the construction is implicit here too
    dp::parse_result<std::string> implicit = source;

    D_PA_CHECK(!implicit.ok());
    D_PA_CHECK(implicit.error() == source);

    return true;
}

/*
tests_parse_result_base_construction
  Verifies the converting constructors from the base result, which are what
let a combinator's base-typed output flow back into the parse vocabulary.
  Tests the following:
  - a base success converts and keeps its value,
  - a base failure converts and keeps its error,
  - both the copy and the move overload are usable,
  - the converted result answers the derived predicates.
*/
bool
tests_parse_result_base_construction()
{
    using base_type = pa_fn::result<int, dp::parse_error>;

    // a base success converts through the const-reference overload
    const base_type base_ok = pa_fn::ok<int, dp::parse_error>(21);
    dp::parse_result<int> converted_ok(base_ok);

    D_PA_CHECK(converted_ok.ok());
    D_PA_CHECK(converted_ok.value() == 21);

    // a base failure converts and keeps its descriptor intact
    const base_type base_err =
        pa_fn::err<int, dp::parse_error>(
            dp::parse_error(dp::DParseStatusEndOfInput, 4u, "eoi"));
    dp::parse_result<int> converted_err(base_err);

    D_PA_CHECK(!converted_err.ok());
    D_PA_CHECK(converted_err.error().status() == dp::DParseStatusEndOfInput);
    D_PA_CHECK(converted_err.error().offset() == 4u);

    // the rvalue overload is reachable from a temporary
    dp::parse_result<int> moved_in(
        pa_fn::ok<int, dp::parse_error>(55));

    D_PA_CHECK(moved_in.ok());
    D_PA_CHECK(moved_in.value() == 55);

    // and from an explicitly moved named base
    base_type movable = pa_fn::ok<int, dp::parse_error>(66);
    dp::parse_result<int> moved_named(std::move(movable));

    D_PA_CHECK(moved_named.ok());
    D_PA_CHECK(moved_named.value() == 66);

    return true;
}

/*
tests_parse_result_type_surface
  PINNED BEHAVIOUR.  Verifies the type-level shape of the refinement.
  Tests the following:
  - value_type and error_type are published as documented,
  - parse_result derives publicly from result<T, parse_error>,
  - PINNED: is_result<parse_result<T>> is FALSE, because is_result matches
    the result specialisation exactly and does not see through a derived
    class -- expected to invert if is_result ever decays through bases,
  - the deleted default constructor of the base is inherited in effect, so
    every parse_result is explicitly a success or a failure.
*/
bool
tests_parse_result_type_surface()
{
    using result_type = dp::parse_result<int>;
    using base_type   = pa_fn::result<int, dp::parse_error>;

    // the published aliases
    D_PA_CHECK((std::is_same<result_type::value_type, int>::value));
    D_PA_CHECK((std::is_same<result_type::error_type,
                             dp::parse_error>::value));

    // the refinement really is an IS-A relationship, publicly
    D_PA_CHECK((std::is_base_of<base_type, result_type>::value));
    D_PA_CHECK((std::is_convertible<result_type*, base_type*>::value));

    // PINNED: the module's own detection vocabulary does not recognise the
    // refinement, because is_result is keyed on the exact specialisation
    D_PA_CHECK(pa_fn::is_result<base_type>::value);
    D_PA_CHECK(!pa_fn::is_result<result_type>::value);

    // there is no neutral state: a parse_result is a success or a failure
    D_PA_CHECK(!std::is_default_constructible<result_type>::value);

    // but it remains an ordinary copyable value type
    D_PA_CHECK(std::is_copy_constructible<result_type>::value);
    D_PA_CHECK(std::is_move_constructible<result_type>::value);

    return true;
}

/*
tests_parse_result_ok_shadowing
  Verifies the deliberate shadowing of result::ok(), which the module
performs so that call sites read as the formal definition does.
  Tests the following:
  - the derived ok() returns bool, not maybe<T>,
  - the base's ok() has a different return type, so the shadowing is real,
  - ok() agrees with is_ok() and is the negation of is_err(),
  - the shadowing holds on both branches.
*/
bool
tests_parse_result_ok_shadowing()
{
    using result_type = dp::parse_result<int>;
    using base_type   = pa_fn::result<int, dp::parse_error>;

    // the derived ok() is the boolean predicate the parse vocabulary wants
    D_PA_CHECK((std::is_same<
        decltype(std::declval<const result_type&>().ok()), bool>::value));

    // the inherited ok() is something else entirely (a maybe), which is
    // exactly why it needed shadowing
    D_PA_CHECK(!(std::is_same<
        decltype(std::declval<const base_type&>().ok()), bool>::value));

    // and the predicate agrees with the inherited discriminator
    result_type success(1);
    result_type failure(dp::parse_error(dp::DParseStatusFailure, 0u));

    D_PA_CHECK(success.ok() == success.is_ok());
    D_PA_CHECK(success.ok() == (!success.is_err()));
    D_PA_CHECK(failure.ok() == failure.is_ok());
    D_PA_CHECK(failure.ok() == (!failure.is_err()));

    D_PA_CHECK(success.ok());
    D_PA_CHECK(!failure.ok());

    return true;
}

/*
tests_parse_result_value_accessors
  Verifies the compact legacy accessors, including the mutable value
overload a parser needs when it fixes up a partially built aggregate.
  Tests the following:
  - value() on a const result yields a const reference,
  - value() on a mutable result yields a mutable reference,
  - writing through the mutable reference is visible afterwards,
  - error() yields a const reference to the stored descriptor.
*/
bool
tests_parse_result_value_accessors()
{
    dp::parse_result<int> mutable_result(10);

    // the const overload hands back a reference to the stored object
    const dp::parse_result<int>& as_const = mutable_result;

    D_PA_CHECK((std::is_same<decltype(as_const.value()),
                             const int&>::value));
    D_PA_CHECK(as_const.value() == 10);

    // the non-const overload is mutable, and the write lands in place
    D_PA_CHECK((std::is_same<decltype(mutable_result.value()),
                             int&>::value));

    mutable_result.value() = 20;

    D_PA_CHECK(mutable_result.value() == 20);
    D_PA_CHECK(as_const.value() == 20);
    D_PA_CHECK(mutable_result.ok());

    // the reference really is to the stored object, not a copy
    D_PA_CHECK(&mutable_result.value() == &as_const.value());

    // error() exposes the descriptor by const reference on the other branch
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusMalformed, 2u, "nope"));

    D_PA_CHECK((std::is_same<decltype(failure.error()),
                             const dp::parse_error&>::value));
    D_PA_CHECK(failure.error().offset() == 2u);

    return true;
}

/*
tests_parse_result_factories
  Verifies the named factories build exactly what the constructors do, so
either spelling may be used at a call site.
  Tests the following:
  - make_ok reproduces value construction,
  - make_error(status, offset, std::string) reproduces error construction,
  - make_error's message argument defaults to empty,
  - the const char* make_error overload, including its null guard.
*/
bool
tests_parse_result_factories()
{
    using result_type = dp::parse_result<int>;

    // make_ok and the value constructor agree
    result_type built  = result_type::make_ok(5);
    result_type direct = result_type(5);

    D_PA_CHECK(built.ok());
    D_PA_CHECK(built.value() == 5);
    D_PA_CHECK(built == direct);

    // the std::string make_error overload fills all three fields
    result_type failed = result_type::make_error(dp::DParseStatusMalformed,
                                                 11u,
                                                 std::string("malformed"));

    D_PA_CHECK(!failed.ok());
    D_PA_CHECK(failed.error().status()  == dp::DParseStatusMalformed);
    D_PA_CHECK(failed.error().offset()  == 11u);
    D_PA_CHECK(failed.error().message() == std::string("malformed"));

    // the message defaults to empty when omitted
    result_type terse = result_type::make_error(dp::DParseStatusOverflow,
                                                2u);

    D_PA_CHECK(!terse.ok());
    D_PA_CHECK(terse.error().message().empty());
    D_PA_CHECK(terse.error().offset() == 2u);

    // the const char* overload is selected by a literal, and guards null
    result_type from_literal =
        result_type::make_error(dp::DParseStatusEndOfInput, 1u, "eoi");
    result_type from_null =
        result_type::make_error(dp::DParseStatusEndOfInput,
                                1u,
                                static_cast<const char*>(nullptr));

    D_PA_CHECK(from_literal.error().message() == std::string("eoi"));
    D_PA_CHECK(from_null.error().message().empty());

    // and a factory-built failure equals a constructor-built one
    D_PA_CHECK(from_literal ==
               result_type(dp::parse_error(dp::DParseStatusEndOfInput,
                                           1u,
                                           "eoi")));

    return true;
}

/*
tests_parse_result_copy_and_move
  Verifies parse_result is a well-behaved value type on both branches, which
matters because the discriminated storage must be destroyed and rebuilt when
an assignment switches branch.
  Tests the following:
  - copy construction and copy assignment preserve the branch and payload,
  - move construction preserves the branch and payload,
  - assignment that switches branch rebuilds the storage correctly,
  - self-assignment is safe.
*/
bool
tests_parse_result_copy_and_move()
{
    dp::parse_result<std::string> original(std::string("value"));

    // copy construction
    dp::parse_result<std::string> copied(original);

    D_PA_CHECK(copied.ok());
    D_PA_CHECK(copied.value() == std::string("value"));
    D_PA_CHECK(original.value() == std::string("value"));

    // copy assignment within the same branch
    dp::parse_result<std::string> assigned(std::string("other"));
    assigned = original;

    D_PA_CHECK(assigned.ok());
    D_PA_CHECK(assigned.value() == std::string("value"));

    // assignment that switches branch: success becomes failure
    dp::parse_result<std::string> failure(
        dp::parse_error(dp::DParseStatusMalformed, 6u, "switch"));

    assigned = failure;

    D_PA_CHECK(!assigned.ok());
    D_PA_CHECK(assigned.error().offset() == 6u);
    D_PA_CHECK(assigned.error().message() == std::string("switch"));

    // and back again, so both directions of the branch switch are covered
    assigned = original;

    D_PA_CHECK(assigned.ok());
    D_PA_CHECK(assigned.value() == std::string("value"));

    // move construction carries the payload across
    dp::parse_result<std::string> source(std::string("moved"));
    dp::parse_result<std::string> moved(std::move(source));

    D_PA_CHECK(moved.ok());
    D_PA_CHECK(moved.value() == std::string("moved"));

    // move assignment, including across a branch switch
    dp::parse_result<std::string> target(
        dp::parse_error(dp::DParseStatusFailure, 0u));

    target = dp::parse_result<std::string>(std::string("assigned"));

    D_PA_CHECK(target.ok());
    D_PA_CHECK(target.value() == std::string("assigned"));

    // self-assignment must not destroy the payload; the pointer indirection
    // keeps the compiler's self-assignment diagnostic quiet
    const dp::parse_result<std::string>* self = &target;
    target = *self;

    D_PA_CHECK(target.ok());
    D_PA_CHECK(target.value() == std::string("assigned"));

    return true;
}

/*
tests_parse_result_move_forwards_value_category
  Verifies the two value constructors forward the value category they were
handed, so a large parsed aggregate is not copied when it need not be.
  Tests the following:
  - an rvalue source is moved exactly once and never copied,
  - an lvalue source is copied exactly once and never moved,
  - the carried value is correct in both cases.
*/
bool
tests_parse_result_move_forwards_value_category()
{
    // an rvalue takes the move constructor all the way to the storage
    pa_move_probe movable(7);
    dp::parse_result<pa_move_probe> moved(std::move(movable));

    D_PA_CHECK(moved.ok());
    D_PA_CHECK(moved.value().value  == 7);
    D_PA_CHECK(moved.value().moves  == 1);
    D_PA_CHECK(moved.value().copies == 0);

    // an lvalue takes the copy constructor
    pa_move_probe copyable(9);
    dp::parse_result<pa_move_probe> copied(copyable);

    D_PA_CHECK(copied.ok());
    D_PA_CHECK(copied.value().value  == 9);
    D_PA_CHECK(copied.value().copies == 1);
    D_PA_CHECK(copied.value().moves  == 0);

    // the lvalue source is untouched by having been copied from
    D_PA_CHECK(copyable.value  == 9);
    D_PA_CHECK(copyable.copies == 0);

    return true;
}

/*
tests_parse_result_equality
  Verifies equality reaches parse_result through derived-to-base template
argument deduction, so two outcomes can be compared directly.
  Tests the following:
  - two successes are equal exactly when their values are,
  - two failures are equal exactly when their errors are,
  - a success and a failure are never equal, in either ordering,
  - inequality is the negation of equality.
*/
bool
tests_parse_result_equality()
{
    dp::parse_result<int> ok_a(4);
    dp::parse_result<int> ok_b(4);
    dp::parse_result<int> ok_c(5);

    // the success branch compares values
    D_PA_CHECK(ok_a == ok_b);
    D_PA_CHECK(!(ok_a == ok_c));
    D_PA_CHECK(ok_a != ok_c);

    dp::parse_result<int> err_a(
        dp::parse_error(dp::DParseStatusMalformed, 1u, "m"));
    dp::parse_result<int> err_b(
        dp::parse_error(dp::DParseStatusMalformed, 1u, "m"));
    dp::parse_result<int> err_c(
        dp::parse_error(dp::DParseStatusMalformed, 2u, "m"));

    // the failure branch compares descriptors, field by field
    D_PA_CHECK(err_a == err_b);
    D_PA_CHECK(!(err_a == err_c));
    D_PA_CHECK(err_a != err_c);

    // a success is never equal to a failure, whichever way round
    D_PA_CHECK(!(ok_a == err_a));
    D_PA_CHECK(!(err_a == ok_a));
    D_PA_CHECK(ok_a != err_a);
    D_PA_CHECK(err_a != ok_a);

    // and a result equals itself on both branches
    D_PA_CHECK(ok_a == ok_a);
    D_PA_CHECK(err_a == err_a);

    return true;
}

/*
tests_parse_result_value_or_and_unwrap
  Verifies the two inherited extraction helpers, which are how a caller
leaves the result world with or without a failure path.
  Tests the following:
  - value_or returns the value on success and the default on failure,
  - the default is not substituted when a value is present,
  - unwrap returns the value on success,
  - unwrap throws std::runtime_error carrying the given message on failure.
*/
bool
tests_parse_result_value_or_and_unwrap()
{
    dp::parse_result<int> success(31);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusOverflow, 5u, "overflowed"));

    // value_or substitutes only on the failure branch
    D_PA_CHECK(success.value_or(-1) == 31);
    D_PA_CHECK(failure.value_or(-1) == -1);
    D_PA_CHECK(failure.value_or(0)  == 0);

    // unwrap is the throwing extraction
    D_PA_CHECK(success.unwrap("should not throw") == 31);

    bool        threw = false;
    std::string caught;

    // the failure branch must raise, carrying the caller's message
    try
    {
        (void)failure.unwrap("could not parse");
    }
    catch (const std::runtime_error& error)
    {
        threw  = true;
        caught = error.what();
    }

    D_PA_CHECK(threw);
    D_PA_CHECK(caught == std::string("could not parse"));

    // and the result is unchanged by the failed extraction attempt
    D_PA_CHECK(!failure.ok());
    D_PA_CHECK(failure.error().offset() == 5u);

    return true;
}

/*
tests_parse_result_inherited_map
  PINNED BEHAVIOUR.  Verifies result::map over a parse_result.
  Tests the following:
  - map transforms the success value and keeps the branch,
  - map leaves a failure untouched and does not invoke the function,
  - the error survives the transformation unchanged,
  - PINNED: map returns the BASE result<U, parse_error>, not
    parse_result<U> -- expected to invert if parse_result ever overrides
    the combinators to preserve the refinement.
*/
bool
tests_parse_result_inherited_map()
{
    dp::parse_result<int> success(6);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusMalformed, 3u, "bad"));

    int calls = 0;

    // the success branch is transformed
    pa_fn::result<int, dp::parse_error> doubled =
        success.map([&calls](const int& _v) { ++calls; return _v * 2; });

    D_PA_CHECK(doubled.is_ok());
    D_PA_CHECK(doubled.value() == 12);
    D_PA_CHECK(calls == 1);

    // the failure branch propagates untouched, and the function is not run
    pa_fn::result<int, dp::parse_error> propagated =
        failure.map([&calls](const int& _v) { ++calls; return _v * 2; });

    D_PA_CHECK(propagated.is_err());
    D_PA_CHECK(calls == 1);
    D_PA_CHECK(propagated.error().status() == dp::DParseStatusMalformed);
    D_PA_CHECK(propagated.error().offset() == 3u);
    D_PA_CHECK(propagated.error().message() == std::string("bad"));

    // the mapped type may differ from the source type
    pa_fn::result<std::string, dp::parse_error> rendered =
        success.map(
            [](const int& _v)
            {
                return std::string(static_cast<std::size_t>(_v), 'x');
            });

    D_PA_CHECK(rendered.is_ok());
    D_PA_CHECK(rendered.value() == std::string("xxxxxx"));

    // PINNED: the refinement is lost across map -- the static type of the
    // expression is the base, not parse_result
    D_PA_CHECK((std::is_same<
        decltype(success.map(std::declval<int (*)(const int&)>())),
        pa_fn::result<int, dp::parse_error> >::value));
    D_PA_CHECK(!(std::is_same<
        decltype(success.map(std::declval<int (*)(const int&)>())),
        dp::parse_result<int> >::value));

    return true;
}

/*
tests_parse_result_inherited_map_err
  Verifies result::map_err over a parse_result, which is how a parse failure
is lifted into a caller's own error vocabulary.
  Tests the following:
  - map_err transforms the error side and keeps the failure branch,
  - map_err leaves a success untouched and does not invoke the function,
  - the success value survives with its type intact,
  - the error type of the output is the mapped type.
*/
bool
tests_parse_result_inherited_map_err()
{
    dp::parse_result<int> success(3);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusOverflow, 12u, "overflow"));

    int calls = 0;

    // the failure branch is transformed into a plain status code
    pa_fn::result<int, dp::parse_status> mapped =
        failure.map_err([&calls](const dp::parse_error& _e)
                        { ++calls; return _e.status(); });

    D_PA_CHECK(mapped.is_err());
    D_PA_CHECK(mapped.error() == dp::DParseStatusOverflow);
    D_PA_CHECK(calls == 1);

    // the success branch passes through untouched, function not invoked
    pa_fn::result<int, dp::parse_status> untouched =
        success.map_err([&calls](const dp::parse_error& _e)
                        { ++calls; return _e.status(); });

    D_PA_CHECK(untouched.is_ok());
    D_PA_CHECK(untouched.value() == 3);
    D_PA_CHECK(calls == 1);

    // the error side may become any type, including a string
    pa_fn::result<int, std::string> described =
        failure.map_err([](const dp::parse_error& _e)
                        { return _e.message(); });

    D_PA_CHECK(described.is_err());
    D_PA_CHECK(described.error() == std::string("overflow"));

    return true;
}

/*
tests_parse_result_inherited_and_then
  Verifies monadic bind over a parse_result.  The callable must return the
BASE result<U, parse_error>: see the PINNED note in the suite preamble and
tests_parse_result_tagged_constructors_absent for why a parse_result-
returning callable does not compile.
  Tests the following:
  - a success is threaded through the callable,
  - a failure short-circuits and the callable is never invoked,
  - the original error propagates unchanged through the short circuit,
  - binds compose, including one that fails in the middle.
*/
bool
tests_parse_result_inherited_and_then()
{
    dp::parse_result<int> success(4);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusMalformed, 2u, "stop"));

    int calls = 0;

    // the success branch runs the callable and adopts its outcome
    pa_fn::result<int, dp::parse_error> chained =
        success.and_then([&calls](const int& _v)
                         {
                             ++calls;
                             return pa_fn::ok<int, dp::parse_error>(_v + 1);
                         });

    D_PA_CHECK(chained.is_ok());
    D_PA_CHECK(chained.value() == 5);
    D_PA_CHECK(calls == 1);

    // the failure branch short-circuits without invoking the callable
    pa_fn::result<int, dp::parse_error> short_circuit =
        failure.and_then([&calls](const int& _v)
                         {
                             ++calls;
                             return pa_fn::ok<int, dp::parse_error>(_v + 1);
                         });

    D_PA_CHECK(short_circuit.is_err());
    D_PA_CHECK(calls == 1);
    D_PA_CHECK(short_circuit.error().status() == dp::DParseStatusMalformed);
    D_PA_CHECK(short_circuit.error().offset() == 2u);
    D_PA_CHECK(short_circuit.error().message() == std::string("stop"));

    // a callable may also introduce a failure of its own
    pa_fn::result<int, dp::parse_error> rejected =
        success.and_then([](const int&)
                         {
                             return pa_fn::err<int, dp::parse_error>(
                                 dp::parse_error(dp::DParseStatusOverflow,
                                                 9u,
                                                 "rejected"));
                         });

    D_PA_CHECK(rejected.is_err());
    D_PA_CHECK(rejected.error().status() == dp::DParseStatusOverflow);
    D_PA_CHECK(rejected.error().offset() == 9u);

    // binds compose left to right
    pa_fn::result<int, dp::parse_error> composed =
        success
            .and_then([](const int& _v)
                      { return pa_fn::ok<int, dp::parse_error>(_v * 2); })
            .and_then([](const int& _v)
                      { return pa_fn::ok<int, dp::parse_error>(_v + 3); });

    D_PA_CHECK(composed.is_ok());
    D_PA_CHECK(composed.value() == 11);

    return true;
}

/*
tests_parse_result_inherited_or_else
  Verifies error-side recovery over a parse_result.  As with and_then, the
recovery callable must return the BASE result.
  Tests the following:
  - a failure is handed to the recovery callable,
  - a success passes through and the callable is never invoked,
  - recovery may substitute a value derived from the error,
  - recovery may also produce a different failure.
*/
bool
tests_parse_result_inherited_or_else()
{
    dp::parse_result<int> success(8);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusEndOfInput, 6u, "eoi"));

    int calls = 0;

    // the failure branch is recovered, using information from the error
    pa_fn::result<int, dp::parse_error> recovered =
        failure.or_else([&calls](const dp::parse_error& _e)
                        {
                            ++calls;
                            return pa_fn::ok<int, dp::parse_error>(
                                static_cast<int>(_e.offset()));
                        });

    D_PA_CHECK(recovered.is_ok());
    D_PA_CHECK(recovered.value() == 6);
    D_PA_CHECK(calls == 1);

    // the success branch passes through without invoking the callable
    pa_fn::result<int, dp::parse_error> untouched =
        success.or_else([&calls](const dp::parse_error& _e)
                        {
                            ++calls;
                            return pa_fn::ok<int, dp::parse_error>(
                                static_cast<int>(_e.offset()));
                        });

    D_PA_CHECK(untouched.is_ok());
    D_PA_CHECK(untouched.value() == 8);
    D_PA_CHECK(calls == 1);

    // recovery may decide the failure is not recoverable and re-fail
    pa_fn::result<int, dp::parse_error> re_failed =
        failure.or_else([](const dp::parse_error& _e)
                        {
                            return pa_fn::err<int, dp::parse_error>(
                                dp::parse_error(dp::DParseStatusFailure,
                                                _e.offset(),
                                                "unrecoverable"));
                        });

    D_PA_CHECK(re_failed.is_err());
    D_PA_CHECK(re_failed.error().status() == dp::DParseStatusFailure);
    D_PA_CHECK(re_failed.error().offset() == 6u);
    D_PA_CHECK(re_failed.error().message() == std::string("unrecoverable"));

    return true;
}

/*
tests_parse_result_inherited_match
  Verifies pattern-matching dispatch, which is the total way to consume a
parse outcome without a branch predicate at the call site.
  Tests the following:
  - the ok arm runs on a success and the err arm does not,
  - the err arm runs on a failure and the ok arm does not,
  - both arms may read their payload,
  - the common return type is produced on either branch.
*/
bool
tests_parse_result_inherited_match()
{
    dp::parse_result<int> success(12);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusMalformed, 4u, "malformed"));

    int ok_calls  = 0;
    int err_calls = 0;

    // the success branch selects the ok arm alone
    std::string from_ok = success.match(
        [&ok_calls](const int& _v)
        {
            ++ok_calls;
            return std::string("ok:") + std::to_string(_v);
        },
        [&err_calls](const dp::parse_error& _e)
        {
            ++err_calls;
            return std::string("err:") + _e.message();
        });

    D_PA_CHECK(from_ok == std::string("ok:12"));
    D_PA_CHECK(ok_calls  == 1);
    D_PA_CHECK(err_calls == 0);

    // the failure branch selects the err arm alone
    std::string from_err = failure.match(
        [&ok_calls](const int& _v)
        {
            ++ok_calls;
            return std::string("ok:") + std::to_string(_v);
        },
        [&err_calls](const dp::parse_error& _e)
        {
            ++err_calls;
            return std::string("err:") + _e.message();
        });

    D_PA_CHECK(from_err == std::string("err:malformed"));
    D_PA_CHECK(ok_calls  == 1);
    D_PA_CHECK(err_calls == 1);

    // the err arm sees the whole descriptor, not just the message
    std::size_t reported = failure.match(
        [](const int&) { return static_cast<std::size_t>(0); },
        [](const dp::parse_error& _e) { return _e.offset(); });

    D_PA_CHECK(reported == 4u);

    return true;
}

/*
tests_parse_result_operator_bool
  Verifies the inherited explicit conversion to bool, which is what makes
`if (result)` read naturally without permitting accidental arithmetic.
  Tests the following:
  - the conversion is true on success and false on failure,
  - it agrees with ok(),
  - it is explicit, so implicit conversion to bool is not available,
  - it therefore does not permit implicit conversion to an integer.
*/
bool
tests_parse_result_operator_bool()
{
    dp::parse_result<int> success(1);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusFailure, 0u));

    // the conversion tracks the branch
    D_PA_CHECK(static_cast<bool>(success));
    D_PA_CHECK(!static_cast<bool>(failure));

    // and agrees with the shadowing predicate
    D_PA_CHECK(static_cast<bool>(success) == success.ok());
    D_PA_CHECK(static_cast<bool>(failure) == failure.ok());

    // it is usable in a condition, which is a contextual conversion
    if (success)
    {
        D_PA_CHECK(success.value() == 1);
    }
    else
    {
        D_PA_CHECK(false);
    }

    // but it is explicit: direct-initialisation works, copy-initialisation
    // and implicit widening to an arithmetic type do not
    D_PA_CHECK((std::is_constructible<bool,
                                      dp::parse_result<int> >::value));
    D_PA_CHECK(!(std::is_convertible<dp::parse_result<int>, bool>::value));
    D_PA_CHECK(!(std::is_convertible<dp::parse_result<int>, int>::value));

    return true;
}

/*
tests_parse_result_pipeline_combinators
  Verifies the operator| pipeline surface reaches parse_result through
derived-to-base template argument deduction, so the refinement does not cost
the caller the combinator vocabulary.
  Tests the following:
  - or_value_with extracts the value or substitutes the default,
  - map_err_with transforms the error side through the pipeline,
  - unwrap_with extracts the value on success,
  - unwrap_with throws on failure.
*/
bool
tests_parse_result_pipeline_combinators()
{
    dp::parse_result<int> success(17);
    dp::parse_result<int> failure(
        dp::parse_error(dp::DParseStatusOverflow, 3u, "over"));

    // or_value_with: extraction with a default, on both branches
    D_PA_CHECK((success | pa_fn::or_value_with(-1)) == 17);
    D_PA_CHECK((failure | pa_fn::or_value_with(-1)) == -1);

    // map_err_with: the error side is transformed, the branch preserved
    pa_fn::result<int, dp::parse_status> mapped =
        failure | pa_fn::map_err_with([](const dp::parse_error& _e)
                                      { return _e.status(); });

    D_PA_CHECK(mapped.is_err());
    D_PA_CHECK(mapped.error() == dp::DParseStatusOverflow);

    // a success passes through map_err_with untouched
    pa_fn::result<int, dp::parse_status> passed =
        success | pa_fn::map_err_with([](const dp::parse_error& _e)
                                      { return _e.status(); });

    D_PA_CHECK(passed.is_ok());
    D_PA_CHECK(passed.value() == 17);

    // unwrap_with: extraction or throw
    D_PA_CHECK((success | pa_fn::unwrap_with("no")) == 17);

    bool threw = false;

    try
    {
        (void)(failure | pa_fn::unwrap_with("pipeline failed"));
    }
    catch (const std::runtime_error& error)
    {
        threw = (std::string(error.what()) ==
                 std::string("pipeline failed"));
    }

    D_PA_CHECK(threw);

    return true;
}

/*
tests_parse_result_tagged_constructors_absent
  PINNED BEHAVIOUR.  Verifies the root cause of the refinement's one sharp
edge: parse_result declares its own constructors and therefore does NOT
inherit result's tagged (ok_tag / err_tag) constructors.
  result::and_then and result::or_else build their short-circuit branch by
calling exactly those constructors on the callable's return type, so a
callable returning parse_result<U> makes them ill-formed.  That failure is a
hard error inside the function body rather than a substitution failure, so
it cannot be probed with SFINAE; this test pins the missing constructors
instead, which is the property that would have to change to fix it.
  Tests the following:
  - the base IS constructible from (ok_tag, T) and (err_tag, E),
  - PINNED: parse_result is NOT constructible from either,
  - the constructors parse_result does declare are all present,
  - so base-returning callables are the supported form.
*/
bool
tests_parse_result_tagged_constructors_absent()
{
    using base_type   = pa_fn::result<int, dp::parse_error>;
    using result_type = dp::parse_result<int>;
    using ok_tag_t    = pa_fn::internal::ok_tag;
    using err_tag_t   = pa_fn::internal::err_tag;

    // the base offers the tagged constructors the combinators rely on
    D_PA_CHECK((std::is_constructible<base_type, ok_tag_t, int>::value));
    D_PA_CHECK((std::is_constructible<base_type,
                                      err_tag_t,
                                      dp::parse_error>::value));

    // PINNED: the refinement does not inherit them, which is precisely why
    // and_then / or_else cannot round-trip through parse_result
    D_PA_CHECK(!(std::is_constructible<result_type, ok_tag_t, int>::value));
    D_PA_CHECK(!(std::is_constructible<result_type,
                                       err_tag_t,
                                       dp::parse_error>::value));

    // the constructors it does declare are all usable
    D_PA_CHECK((std::is_constructible<result_type, int>::value));
    D_PA_CHECK((std::is_constructible<result_type,
                                      dp::parse_error>::value));
    D_PA_CHECK((std::is_constructible<result_type, base_type>::value));

    // and they are implicit, so a parser body can return a bare value
    D_PA_CHECK((std::is_convertible<int, result_type>::value));
    D_PA_CHECK((std::is_convertible<dp::parse_error, result_type>::value));
    D_PA_CHECK((std::is_convertible<base_type, result_type>::value));

    return true;
}

/*
tests_parse_result_non_trivial_payload
  Verifies the discriminated storage manages a payload with a non-trivial
destructor correctly, which is where a tagged union most often goes wrong.
  Tests the following:
  - a string payload is constructed and read back intact,
  - a large payload is not truncated,
  - repeated branch switches leave no corruption behind,
  - the payload and the error can each be replaced many times over.
*/
bool
tests_parse_result_non_trivial_payload()
{
    const std::string large(2048u, 'q');

    // a large non-trivial payload survives construction
    dp::parse_result<std::string> held(large);

    D_PA_CHECK(held.ok());
    D_PA_CHECK(held.value().size() == 2048u);
    D_PA_CHECK(held.value() == large);

    std::size_t i;

    // repeated branch switching exercises destroy-then-reconstruct on both
    // sides of the discriminated storage
    for (i = 0; i < 32u; ++i)
    {
        held = dp::parse_result<std::string>(
            dp::parse_error(dp::DParseStatusMalformed,
                            i,
                            std::string(i + 1u, 'e')));

        D_PA_CHECK(!held.ok());
        D_PA_CHECK(held.error().offset() == i);
        D_PA_CHECK(held.error().message().size() == (i + 1u));

        held = dp::parse_result<std::string>(std::string(i + 1u, 'v'));

        D_PA_CHECK(held.ok());
        D_PA_CHECK(held.value().size() == (i + 1u));
    }

    // and the final state is exactly what the last iteration left
    D_PA_CHECK(held.ok());
    D_PA_CHECK(held.value() == std::string(32u, 'v'));

    // PINNED: parse_result<T> is UNINSTANTIABLE when T is parse_error
    // itself.  The value constructors take (const T&) / (T&&) and the error
    // constructors take (const parse_error&) / (parse_error&&); when
    // T == parse_error these are the same two signatures, so the class is
    // ill-formed on instantiation (a redeclared constructor), not merely
    // ambiguous at a call site.  We therefore assert the collision at the
    // TYPE level via a constructibility probe on a fresh identical shape,
    // rather than instantiating parse_result<parse_error>, which would not
    // compile.  Expected to invert if the error constructors are ever
    // disambiguated (e.g. via a tag) so a parse_error payload is allowed.
    D_PA_CHECK(pa_result_error_payload_collides<dp::parse_error>::value);
    D_PA_CHECK(!pa_result_error_payload_collides<int>::value);
    D_PA_CHECK(!pa_result_error_payload_collides<std::string>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
