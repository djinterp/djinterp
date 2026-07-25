#include <climits>

#include "parse_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_parse_error_default_construction
  Verifies the default parse_error is the neutral failure the module
documents, so a descriptor is never in an indeterminate state.
  Tests the following:
  - status defaults to DParseStatusFailure, not to Success,
  - offset defaults to zero,
  - the message defaults to empty,
  - a default error is default-constructible and equal to another one.
*/
bool
tests_parse_error_default_construction()
{
    dp::parse_error error;

    // a default descriptor describes a failure, so it can never be
    // mistaken for a successful parse
    D_PA_CHECK(error.status() == dp::DParseStatusFailure);
    D_PA_CHECK(error.status() != dp::DParseStatusSuccess);

    // and it points at the start of the input with nothing to say
    D_PA_CHECK(error.offset() == 0u);
    D_PA_CHECK(error.message().empty());
    D_PA_CHECK(error.message().size() == 0u);

    // the type is default-constructible and defaults deterministically
    D_PA_CHECK(std::is_default_constructible<dp::parse_error>::value);

    dp::parse_error other;

    D_PA_CHECK(error == other);

    return true;
}

/*
tests_parse_error_string_construction
  Verifies the std::string constructor stores all three fields verbatim.
  Tests the following:
  - status, offset, and message are carried unchanged,
  - a message containing an embedded NUL keeps its full length,
  - a message containing whitespace and punctuation is untouched,
  - an explicitly empty message is distinguishable by size, not by content.
*/
bool
tests_parse_error_string_construction()
{
    const std::string message("unexpected token near column 4");

    dp::parse_error error(dp::DParseStatusMalformed, 4u, message);

    // every field is carried through unchanged
    D_PA_CHECK(error.status()  == dp::DParseStatusMalformed);
    D_PA_CHECK(error.offset()  == 4u);
    D_PA_CHECK(error.message() == message);
    D_PA_CHECK(error.message().size() == message.size());

    // a std::string may legitimately contain an embedded NUL; the
    // descriptor must not truncate at it the way a C string would
    std::string embedded("before");
    embedded.push_back('\0');
    embedded.append("after");

    dp::parse_error nul_error(dp::DParseStatusFailure, 0u, embedded);

    D_PA_CHECK(nul_error.message().size() == 12u);
    D_PA_CHECK(nul_error.message() == embedded);
    D_PA_CHECK(nul_error.message()[6] == '\0');

    // an explicitly empty message is a legitimate message
    dp::parse_error empty_error(dp::DParseStatusOverflow,
                                9u,
                                std::string());

    D_PA_CHECK(empty_error.message().empty());
    D_PA_CHECK(empty_error.offset() == 9u);

    return true;
}

/*
tests_parse_error_default_message
  Verifies the message argument really is optional on the std::string
overload, and that omitting it leaves the other two fields intact.
  Tests the following:
  - the two-argument form compiles and yields an empty message,
  - status and offset are unaffected by the omission,
  - it is equal to the explicit empty-string form.
*/
bool
tests_parse_error_default_message()
{
    dp::parse_error implicit(dp::DParseStatusEndOfInput, 17u);

    // the defaulted message is empty, and nothing else is disturbed
    D_PA_CHECK(implicit.message().empty());
    D_PA_CHECK(implicit.status() == dp::DParseStatusEndOfInput);
    D_PA_CHECK(implicit.offset() == 17u);

    // omitting the argument and passing an empty string are the same thing
    dp::parse_error explicit_empty(dp::DParseStatusEndOfInput,
                                   17u,
                                   std::string());

    D_PA_CHECK(implicit == explicit_empty);

    return true;
}

/*
tests_parse_error_cstring_construction
  Verifies the const char* overload, which is what a string literal at a
call site actually selects.
  Tests the following:
  - a string literal is copied into the message,
  - the empty literal yields an empty message,
  - the result is equal to the std::string form with the same text,
  - the overload does not truncate an ordinary message.
*/
bool
tests_parse_error_cstring_construction()
{
    // a literal argument binds to const char*, which is an exact match and
    // therefore preferred over the user conversion to std::string
    dp::parse_error error(dp::DParseStatusMalformed, 2u, "bad escape");

    D_PA_CHECK(error.status()  == dp::DParseStatusMalformed);
    D_PA_CHECK(error.offset()  == 2u);
    D_PA_CHECK(error.message() == std::string("bad escape"));
    D_PA_CHECK(error.message().size() == 10u);

    // the empty literal is not the null pointer, and must be carried as an
    // ordinary empty message
    dp::parse_error empty(dp::DParseStatusFailure, 0u, "");

    D_PA_CHECK(empty.message().empty());

    // both overloads agree when given the same text
    dp::parse_error via_string(dp::DParseStatusMalformed,
                               2u,
                               std::string("bad escape"));

    D_PA_CHECK(error == via_string);

    return true;
}

/*
tests_parse_error_cstring_null_message
  Verifies the null guard on the const char* overload, which is the one
place the class can be handed a pointer it must not dereference.
  Tests the following:
  - a null message pointer yields an empty message rather than undefined
    behaviour,
  - status and offset are still stored,
  - the null form equals the empty-literal form,
  - the guard also holds for a null typed through a variable.
*/
bool
tests_parse_error_cstring_null_message()
{
    const char* no_message = nullptr;

    dp::parse_error error(dp::DParseStatusOverflow, 33u, no_message);

    // the guard converts null to empty instead of constructing a
    // std::string from a null pointer
    D_PA_CHECK(error.message().empty());
    D_PA_CHECK(error.message().size() == 0u);

    // the other fields are unaffected by taking the guarded path
    D_PA_CHECK(error.status() == dp::DParseStatusOverflow);
    D_PA_CHECK(error.offset() == 33u);

    // null and "" are indistinguishable once stored
    dp::parse_error from_empty(dp::DParseStatusOverflow, 33u, "");

    D_PA_CHECK(error == from_empty);

    // and the same holds for a literal null cast to the pointer type
    dp::parse_error cast_null(dp::DParseStatusFailure,
                              0u,
                              static_cast<const char*>(nullptr));

    D_PA_CHECK(cast_null.message().empty());

    return true;
}

/*
tests_parse_error_message_is_owning
  Verifies the message is an owning std::string, which is the property that
lets a parse_error be copied without lifetime caveats -- the module's stated
reason for not storing a const char*.
  Tests the following:
  - overwriting the source buffer does not change the stored message,
  - a message built from a buffer that later leaves scope stays valid,
  - the stored message does not alias the source storage.
*/
bool
tests_parse_error_message_is_owning()
{
    char buffer[16] = "original";

    dp::parse_error error(dp::DParseStatusMalformed, 1u, buffer);

    D_PA_CHECK(error.message() == std::string("original"));

    // mutate the source buffer under the descriptor's feet
    buffer[0] = 'X';
    buffer[1] = '\0';

    // an owning copy is unaffected; a stored pointer would now read "X"
    D_PA_CHECK(error.message() == std::string("original"));
    D_PA_CHECK(error.message().size() == 8u);

    // the stored message must not alias the source storage at all
    D_PA_CHECK(error.message().data() !=
               static_cast<const char*>(buffer));

    // a message whose source has left scope entirely is still readable
    dp::parse_error outlives(dp::DParseStatusFailure, 0u, "");

    {
        std::string temporary("temporary text");
        outlives = dp::parse_error(dp::DParseStatusFailure, 5u, temporary);
    }

    D_PA_CHECK(outlives.message() == std::string("temporary text"));

    return true;
}

/*
tests_parse_error_copy_semantics
  Verifies parse_error behaves as an ordinary value type, since parse_result
copies it freely on every error-side propagation.
  Tests the following:
  - copy construction reproduces all three fields,
  - copy assignment replaces all three fields,
  - self-assignment is safe and leaves the value intact,
  - the copy is independent of the original.
*/
bool
tests_parse_error_copy_semantics()
{
    dp::parse_error original(dp::DParseStatusMalformed, 7u, "original");

    // copy construction is field-for-field
    dp::parse_error copied(original);

    D_PA_CHECK(copied.status()  == dp::DParseStatusMalformed);
    D_PA_CHECK(copied.offset()  == 7u);
    D_PA_CHECK(copied.message() == std::string("original"));
    D_PA_CHECK(copied == original);

    // copy assignment replaces every field, including across statuses
    dp::parse_error assigned(dp::DParseStatusSuccess, 0u, "placeholder");
    assigned = original;

    D_PA_CHECK(assigned == original);
    D_PA_CHECK(assigned.message() == std::string("original"));

    // self-assignment must be a no-op rather than a corruption
    assigned = assigned;

    D_PA_CHECK(assigned == original);
    D_PA_CHECK(assigned.message() == std::string("original"));

    // the copy is genuinely independent: reassigning one leaves the other
    copied = dp::parse_error(dp::DParseStatusOverflow, 99u, "changed");

    D_PA_CHECK(original.status()  == dp::DParseStatusMalformed);
    D_PA_CHECK(original.offset()  == 7u);
    D_PA_CHECK(original.message() == std::string("original"));

    return true;
}

/*
tests_parse_error_equality_fields
  Verifies operator== compares all three fields and nothing else, which is
what makes parse_result equality well defined.
  Tests the following:
  - identical descriptors compare equal,
  - a difference in status alone breaks equality,
  - a difference in offset alone breaks equality,
  - a difference in message alone breaks equality.
*/
bool
tests_parse_error_equality_fields()
{
    dp::parse_error base(dp::DParseStatusMalformed, 5u, "message");

    // an identical descriptor is equal
    dp::parse_error same(dp::DParseStatusMalformed, 5u, "message");

    D_PA_CHECK(base == same);

    // status alone differing is enough to break equality
    dp::parse_error other_status(dp::DParseStatusOverflow, 5u, "message");

    D_PA_CHECK(!(base == other_status));

    // offset alone differing is enough
    dp::parse_error other_offset(dp::DParseStatusMalformed, 6u, "message");

    D_PA_CHECK(!(base == other_offset));

    // message alone differing is enough, including by case and by length
    dp::parse_error other_message(dp::DParseStatusMalformed, 5u, "Message");
    dp::parse_error longer(dp::DParseStatusMalformed, 5u, "message ");

    D_PA_CHECK(!(base == other_message));
    D_PA_CHECK(!(base == longer));

    // an empty message differs from a non-empty one
    dp::parse_error empty_message(dp::DParseStatusMalformed, 5u);

    D_PA_CHECK(!(base == empty_message));

    return true;
}

/*
tests_parse_error_equality_axioms
  Verifies equality is a genuine equivalence relation, so descriptors can be
compared, deduplicated, and used as expected values in assertions.
  Tests the following:
  - reflexivity over a sample set,
  - symmetry over every ordered pair,
  - transitivity across three equal descriptors.
*/
bool
tests_parse_error_equality_axioms()
{
    const dp::parse_error samples[4] =
    {
        dp::parse_error(),
        dp::parse_error(dp::DParseStatusMalformed, 5u, "a"),
        dp::parse_error(dp::DParseStatusMalformed, 6u, "a"),
        dp::parse_error(dp::DParseStatusOverflow,  5u, "b")
    };

    std::size_t i;
    std::size_t j;

    // reflexivity: every descriptor equals itself
    for (i = 0; i < 4u; ++i)
    {
        D_PA_CHECK(samples[i] == samples[i]);
    }

    // symmetry: the two orderings always agree
    for (i = 0; i < 4u; ++i)
    {
        for (j = 0; j < 4u; ++j)
        {
            D_PA_CHECK((samples[i] == samples[j]) ==
                       (samples[j] == samples[i]));
        }
    }

    // the sample set is pairwise distinct, so symmetry above was not
    // vacuously true
    for (i = 0; i < 4u; ++i)
    {
        for (j = (i + 1u); j < 4u; ++j)
        {
            D_PA_CHECK(!(samples[i] == samples[j]));
        }
    }

    // transitivity across three separately built but equal descriptors
    dp::parse_error first(dp::DParseStatusEndOfInput, 3u, "eoi");
    dp::parse_error second(dp::DParseStatusEndOfInput, 3u, "eoi");
    dp::parse_error third(dp::DParseStatusEndOfInput, 3u, "eoi");

    D_PA_CHECK(first == second);
    D_PA_CHECK(second == third);
    D_PA_CHECK(first == third);

    return true;
}

/*
tests_parse_error_inequality
  Verifies operator!= is the exact negation of operator== rather than an
independent implementation that could drift.
  Tests the following:
  - the two operators disagree on every ordered pair of a sample set,
  - a descriptor is never unequal to itself,
  - differing descriptors are unequal in both orderings.
*/
bool
tests_parse_error_inequality()
{
    const dp::parse_error samples[4] =
    {
        dp::parse_error(),
        dp::parse_error(dp::DParseStatusFailure, 0u, "x"),
        dp::parse_error(dp::DParseStatusFailure, 1u, "x"),
        dp::parse_error(dp::DParseStatusUserBase, 1u, "x")
    };

    std::size_t i;
    std::size_t j;

    // != is exactly !(==) over the whole matrix
    for (i = 0; i < 4u; ++i)
    {
        for (j = 0; j < 4u; ++j)
        {
            D_PA_CHECK((samples[i] != samples[j]) ==
                       (!(samples[i] == samples[j])));
        }
    }

    // irreflexivity of !=
    for (i = 0; i < 4u; ++i)
    {
        D_PA_CHECK(!(samples[i] != samples[i]));
    }

    // and a spot check in both orderings
    D_PA_CHECK(samples[1] != samples[2]);
    D_PA_CHECK(samples[2] != samples[1]);

    return true;
}

/*
tests_parse_error_extreme_fields
  Verifies the descriptor carries the boundary values its field types allow,
so a parse over a very large input or with an unusual code is describable.
  Tests the following:
  - the maximum representable offset round-trips,
  - INT32 status extremes round-trip,
  - a long message is stored at full length,
  - extreme descriptors still compare correctly.
*/
bool
tests_parse_error_extreme_fields()
{
    const std::size_t max_offset =
        static_cast<std::size_t>(-1);

    // the largest offset the field can hold survives unchanged
    dp::parse_error huge(dp::DParseStatusEndOfInput, max_offset, "far");

    D_PA_CHECK(huge.offset() == max_offset);
    D_PA_CHECK(huge.offset() > 0u);

    // the status field spans the whole of int32_t
    dp::parse_error low(INT32_MIN, 0u, "low");
    dp::parse_error high(INT32_MAX, 0u, "high");

    D_PA_CHECK(low.status()  == INT32_MIN);
    D_PA_CHECK(high.status() == INT32_MAX);
    D_PA_CHECK(low != high);

    // a long message is stored whole, not truncated to a buffer size
    std::string long_message(4096u, 'z');

    dp::parse_error verbose(dp::DParseStatusMalformed, 1u, long_message);

    D_PA_CHECK(verbose.message().size() == 4096u);
    D_PA_CHECK(verbose.message() == long_message);
    D_PA_CHECK(verbose.message()[4095] == 'z');

    // extreme descriptors still compare on all three fields
    dp::parse_error huge_again(dp::DParseStatusEndOfInput,
                               max_offset,
                               "far");
    dp::parse_error huge_shifted(dp::DParseStatusEndOfInput,
                                 max_offset - 1u,
                                 "far");

    D_PA_CHECK(huge == huge_again);
    D_PA_CHECK(huge != huge_shifted);

    return true;
}

NS_END  // testing
NS_END  // djinterp
