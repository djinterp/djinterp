#include "contravariant_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_contramap_documented_usage_example
  The header's USAGE block, run verbatim.
  Tests the following:
  - a to_string_of<long> contramapped with a std::string -> long adapter
    yields a to_string_of<std::string>
  - running it on "hello" produces "5"
  - the adapted context answers for other inputs too
  - the original show_long is untouched and still renders longs
*/
bool
tests_contramap_documented_usage_example()
{
    to_string_of<long> show_long = make_show_long();       // knows how to show a long

    // string -> long, pre-composed onto the long serializer
    auto show_size = contramap(
        [](const std::string& _text)
        {
            return static_cast<long>(_text.size());
        },
        show_long);                                        // to_string_of<std::string>

    // the documented result type
    D_CV_CHECK((std::is_same<decltype(show_size), to_string_of<std::string> >::value));

    // the documented value
    D_CV_CHECK(show_size.run("hello") == "5");

    // and it is a serializer of strings generally, not a one-off
    D_CV_CHECK(show_size.run("") == "0");
    D_CV_CHECK(show_size.run("abcdefghij") == "10");

    // the source context still does its own job
    D_CV_CHECK(show_long.run(42) == "42");

    return true;
}

/*
tests_contramap_result_type_is_rebound_context
  The deduced result type is F<B> for the adapter's domain B.
  Tests the following:
  - contramapping a B -> A adapter onto an F<A> yields exactly F<B>
  - the result equals the instance's own rebind<B>, where one is supplied
  - the result type is a prvalue F<B>, not a reference to the source
  - the result is itself a contravariant context, so contramap composes
*/
bool
tests_contramap_result_type_is_rebound_context()
{
    to_string_of<long> show_long = make_show_long();
    predicate_of<long> is_big{ [](const long& _value) { return _value > 10; } };

    auto shown  = contramap(&string_size, show_long);
    auto tested = contramap(&string_size, is_big);

    // exactly F<B>
    D_CV_CHECK((std::is_same<decltype(shown), to_string_of<std::string> >::value));
    D_CV_CHECK((std::is_same<decltype(tested), predicate_of<std::string> >::value));

    // which is what the instance's own rebind names
    D_CV_CHECK((std::is_same<
                    decltype(shown),
                    contravariant_traits< to_string_of<long> >::rebind<std::string>
                >::value));
    D_CV_CHECK((std::is_same<
                    decltype(tested),
                    contravariant_traits< predicate_of<long> >::rebind<std::string>
                >::value));

    // a value, not a reference into the source
    D_CV_CHECK(!std::is_reference<decltype(contramap(&string_size, show_long))>::value);

    // and still a context, so the operation composes
    D_CV_CHECK(is_contravariant<decltype(shown)>::value);
    D_CV_CHECK((std::is_same<contravariant_value_type_t<decltype(shown)>,
                             std::string>::value));

    return true;
}

/*
tests_contramap_adapts_predicate_domain
  A predicate on A becomes a predicate on B.
  Tests the following:
  - the adapted predicate agrees with the original applied to the mapped value
  - it answers true and false on inputs either side of the boundary
  - it is exact at the boundary itself
  - the original predicate keeps answering on its own domain
*/
bool
tests_contramap_adapts_predicate_domain()
{
    predicate_of<long> longer_than_four{
        [](const long& _value) { return _value > 4; } };

    auto longer_word = contramap(&string_size, longer_than_four);

    // above the boundary
    D_CV_CHECK(longer_word.run("abcde"));
    D_CV_CHECK(longer_word.run("abcdefghij"));

    // below it
    D_CV_CHECK(!longer_word.run(""));
    D_CV_CHECK(!longer_word.run("abc"));

    // exactly at it
    D_CV_CHECK(!longer_word.run("abcd"));

    // agreement with the original applied to the mapped value, term by term
    D_CV_CHECK(longer_word.run("abcde") ==
               longer_than_four.run(string_size("abcde")));
    D_CV_CHECK(longer_word.run("ab") == longer_than_four.run(string_size("ab")));

    // the source is unchanged
    D_CV_CHECK(longer_than_four.run(9));
    D_CV_CHECK(!longer_than_four.run(2));

    return true;
}

/*
tests_contramap_pre_composes_rather_than_post_composes
  The adapter runs BEFORE the context -- the defining difference from a map.
  Tests the following:
  - running the adapted context marks the adapter first and the body second
  - the body receives the value the adapter produced, not the original input
  - nothing runs at all until the adapted context is invoked
  - a second invocation repeats the same order
*/
bool
tests_contramap_pre_composes_rather_than_post_composes()
{
    long seen_by_body = 0;

    to_string_of<long> body{
        [&seen_by_body](const long& _value)
        {
            order_log() += "f";
            seen_by_body = _value;

            return long_to_text(_value);
        } };

    order_log().clear();

    auto adapted = contramap(
        [](const std::string& _text)
        {
            order_log() += "g";

            return static_cast<long>(_text.size());
        },
        body);

    // building the adapted context runs neither step
    D_CV_CHECK(order_log().empty());

    // running it runs the adapter, then the body
    std::string rendered = adapted.run("abcd");

    D_CV_CHECK(order_log() == "gf");
    D_CV_CHECK(rendered == "4");

    // the body saw the ADAPTED value
    D_CV_CHECK(seen_by_body == 4);

    // and the order is stable across invocations
    order_log().clear();
    rendered = adapted.run("xyz");

    D_CV_CHECK(order_log() == "gf");
    D_CV_CHECK(seen_by_body == 3);

    return true;
}

/*
tests_contramap_forwards_value_categories
  Both arguments reach the instance's contramap with their value category
  intact.
  Tests the following:
  - a non-const lvalue context arrives as a non-const lvalue
  - a const lvalue context arrives as a const lvalue
  - a temporary context arrives as an rvalue
  - the adapter's category is tracked independently of the context's
*/
bool
tests_contramap_forwards_value_categories()
{
    predicate_of<long> is_big{ [](const long& _value) { return _value > 4; } };
    auto               size_of = [](const std::string& _text)
                                 {
                                     return static_cast<long>(_text.size());
                                 };

    // a non-const lvalue context
    reset_forward_log();
    predicate_of<std::string> from_lvalue = contramap(size_of, is_big);

    D_CV_CHECK(forward_log().saw_call);
    D_CV_CHECK(forward_log().context_is_lvalue);
    D_CV_CHECK(!forward_log().context_is_const);

    // a const lvalue context
    const predicate_of<long>& const_ref = is_big;

    reset_forward_log();
    predicate_of<std::string> from_const = contramap(size_of, const_ref);

    D_CV_CHECK(forward_log().context_is_lvalue);
    D_CV_CHECK(forward_log().context_is_const);

    // a temporary context
    reset_forward_log();
    predicate_of<std::string> from_rvalue = contramap(
        size_of,
        predicate_of<long>{ [](const long& _value) { return _value > 4; } });

    D_CV_CHECK(!forward_log().context_is_lvalue);
    D_CV_CHECK(!forward_log().context_is_const);

    // the adapter is tracked independently: named here, so an lvalue
    D_CV_CHECK(forward_log().adapter_is_lvalue);

    // and a moved-from adapter arrives as an rvalue while the context stays
    // an lvalue
    auto adapter_copy = size_of;

    reset_forward_log();
    predicate_of<std::string> mixed = contramap(std::move(adapter_copy), is_big);

    D_CV_CHECK(!forward_log().adapter_is_lvalue);
    D_CV_CHECK(forward_log().context_is_lvalue);

    // every one of them still computes the same answer
    D_CV_CHECK(from_lvalue.run("abcde"));
    D_CV_CHECK(from_const.run("abcde"));
    D_CV_CHECK(from_rvalue.run("abcde"));
    D_CV_CHECK(mixed.run("abcde"));
    D_CV_CHECK(!from_lvalue.run("ab"));

    return true;
}

/*
tests_contramap_dispatch_keyed_on_decayed_context
  Dispatch keys on std::decay of the context argument.
  Tests the following:
  - lvalue, const-lvalue and rvalue calls reach the same specialization
  - all three produce the same result type
  - all three produce the same value
  - a const context is accepted without requiring a mutable one
*/
bool
tests_contramap_dispatch_keyed_on_decayed_context()
{
    to_string_of<long>        show_long  = make_show_long();
    const to_string_of<long>& const_show = show_long;

    auto from_lvalue = contramap(&string_size, show_long);
    auto from_const  = contramap(&string_size, const_show);
    auto from_rvalue = contramap(&string_size, make_show_long());

    // one specialization, one result type
    D_CV_CHECK((std::is_same<decltype(from_lvalue), decltype(from_const)>::value));
    D_CV_CHECK((std::is_same<decltype(from_lvalue), decltype(from_rvalue)>::value));
    D_CV_CHECK((std::is_same<decltype(from_lvalue), to_string_of<std::string> >::value));

    // and one answer
    D_CV_CHECK(from_lvalue.run("hello") == "5");
    D_CV_CHECK(from_const.run("hello") == "5");
    D_CV_CHECK(from_rvalue.run("hello") == "5");

    // an explicitly moved context dispatches the same way
    to_string_of<long> movable    = make_show_long();
    auto               from_moved = contramap(&string_size, std::move(movable));

    D_CV_CHECK((std::is_same<decltype(from_moved), to_string_of<std::string> >::value));
    D_CV_CHECK(from_moved.run("hello") == "5");

    return true;
}

/*
tests_contramap_chains_across_three_domains
  Successive contramaps walk a context back across several domains.
  Tests the following:
  - each stage yields F<B> for that stage's adapter domain
  - the composed context computes the whole chain in one call
  - each intermediate stage is still usable on its own domain
  - the chained result agrees with applying the adapters by hand
*/
bool
tests_contramap_chains_across_three_domains()
{
    // long -> bool, the innermost body
    predicate_of<long> is_big{ [](const long& _value) { return _value > 10; } };

    // std::string -> long
    predicate_of<std::string> on_text = contramap(&string_size, is_big);

    // std::vector<int> -> std::string
    auto widen = [](const std::vector<int>& _items)
                 {
                     return std::string(_items.size() * 4, 'x');
                 };

    predicate_of< std::vector<int> > on_items = contramap(widen, on_text);

    // each stage has the expected type
    D_CV_CHECK((std::is_same<decltype(on_text), predicate_of<std::string> >::value));
    D_CV_CHECK((std::is_same<decltype(on_items),
                             predicate_of< std::vector<int> > >::value));

    // the whole chain in one call: 3 items -> 12 characters -> 12 > 10
    std::vector<int> three(3, 0);
    std::vector<int> two(2, 0);

    D_CV_CHECK(on_items.run(three));
    D_CV_CHECK(!on_items.run(two));

    // agrees with applying the adapters by hand
    D_CV_CHECK(on_items.run(three) == is_big.run(string_size(widen(three))));
    D_CV_CHECK(on_items.run(two) == is_big.run(string_size(widen(two))));

    // and every intermediate stage still answers on its own domain
    D_CV_CHECK(is_big.run(11));
    D_CV_CHECK(!is_big.run(10));
    D_CV_CHECK(on_text.run("abcdefghijk"));
    D_CV_CHECK(!on_text.run("abc"));

    return true;
}

/*
tests_contramap_leaves_source_context_usable
  contramap builds a new context; it does not consume the old one.
  Tests the following:
  - the source still answers on its own domain after being contramapped
  - the source can be contramapped more than once
  - the two results are independent of one another
  - contramapping through a const reference to the source is enough
*/
bool
tests_contramap_leaves_source_context_usable()
{
    to_string_of<long> show_long = make_show_long();

    auto show_size = contramap(&string_size, show_long);

    // the source is intact
    D_CV_CHECK(show_long.run(7) == "7");
    D_CV_CHECK(show_long.run(-3) == "-3");

    // and can be adapted again, differently
    auto show_scaled = contramap(scaled_adapter{ 3 }, show_long);

    D_CV_CHECK((std::is_same<decltype(show_scaled), to_string_of<long> >::value));
    D_CV_CHECK(show_scaled.run(5) == "15");

    // the two adaptations do not interfere
    D_CV_CHECK(show_size.run("hello") == "5");
    D_CV_CHECK(show_scaled.run(5) == "15");
    D_CV_CHECK(show_long.run(5) == "5");

    // a const reference to the source is a sufficient input
    const to_string_of<long>& locked = show_long;
    auto                      from_locked = contramap(&string_size, locked);

    D_CV_CHECK(from_locked.run("abc") == "3");
    D_CV_CHECK(show_long.run(1) == "1");

    return true;
}

/*
tests_contramap_through_enable_hook_instance
  The generic operation reaches a hook-registered instance identically.
  Tests the following:
  - contramap on a sink yields sink_of<B> for the adapter's domain B
  - the sink records the rendering of the PRE-CONVERTED value
  - the original sink still accepts values of its own domain
  - both write into the same caller-owned log, in call order
*/
bool
tests_contramap_through_enable_hook_instance()
{
    std::vector<std::string> log;

    sink_of<long> raw{
        &log,
        [](const long& _value) { return "<" + long_to_text(_value) + ">"; } };

    auto adapted = contramap(&string_size, raw);

    // the hook instance rebinds like any other
    D_CV_CHECK((std::is_same<decltype(adapted), sink_of<std::string> >::value));
    D_CV_CHECK(is_contravariant<decltype(adapted)>::value);

    // the recorded line is the rendering of the converted value
    adapted.accept("hello");

    D_CV_CHECK(log.size() == 1u);
    D_CV_CHECK(log[0] == "<5>");

    // the source sink is untouched and shares the same log
    raw.accept(7);

    D_CV_CHECK(log.size() == 2u);
    D_CV_CHECK(log[1] == "<7>");

    // and the adapted sink keeps working afterwards
    adapted.accept("ab");

    D_CV_CHECK(log.size() == 3u);
    D_CV_CHECK(log[2] == "<2>");

    return true;
}

/*
tests_contramap_accepts_any_unary_callable
  The adapter may be any unary callable the instance can hold.
  Tests the following:
  - a closure works
  - a pointer to a free function works, whether taken by address or by name
  - a stateful function object works and carries its state through
  - a std::function works
  - all of them produce the same adapted answer for the same mapping
*/
bool
tests_contramap_accepts_any_unary_callable()
{
    to_string_of<long> show_long = make_show_long();

    // a closure
    auto from_closure = contramap(
        [](const std::string& _text)
        {
            return static_cast<long>(_text.size());
        },
        show_long);

    // a function pointer, by address and by name
    auto from_address = contramap(&string_size, show_long);
    auto from_name    = contramap(string_size, show_long);

    // a std::function
    std::function<long(const std::string&)> erased = &string_size;
    auto                                    from_erased = contramap(erased, show_long);

    // every shape lands on the same context type and the same answer
    D_CV_CHECK((std::is_same<decltype(from_closure), to_string_of<std::string> >::value));
    D_CV_CHECK((std::is_same<decltype(from_address), to_string_of<std::string> >::value));
    D_CV_CHECK((std::is_same<decltype(from_name), to_string_of<std::string> >::value));
    D_CV_CHECK((std::is_same<decltype(from_erased), to_string_of<std::string> >::value));

    D_CV_CHECK(from_closure.run("hello") == "5");
    D_CV_CHECK(from_address.run("hello") == "5");
    D_CV_CHECK(from_name.run("hello") == "5");
    D_CV_CHECK(from_erased.run("hello") == "5");

    // a stateful function object carries its state into the adapted context
    auto times_two   = contramap(scaled_adapter{ 2 }, show_long);
    auto times_three = contramap(scaled_adapter{ 3 }, show_long);

    D_CV_CHECK(times_two.run(5) == "10");
    D_CV_CHECK(times_three.run(5) == "15");

    return true;
}

/*
tests_contramap_expression_sfinae_on_adapter
  The trailing return type is expression-SFINAE friendly.
  Tests the following:
  - a well-shaped adapter makes the call well-formed on every instance
  - an adapter of the wrong arity removes the call from overload resolution
    instead of hard-erroring
  - a non-callable adapter does the same
  - the detection probe itself is a well-formed bool trait, which is the
    evidence the failure was soft
*/
bool
tests_contramap_expression_sfinae_on_adapter()
{
    using unary_type  = long (*)(const std::string&);
    using binary_type = long (*)(const std::string&, int);

    // the well-shaped adapter is accepted by every instance
    D_CV_CHECK((can_contramap<unary_type, to_string_of<long> >::value));
    D_CV_CHECK((can_contramap<unary_type, predicate_of<long> >::value));
    D_CV_CHECK((can_contramap<unary_type, sink_of<long> >::value));

    // an adapter of the wrong arity is rejected, softly
    D_CV_CHECK(!(can_contramap<binary_type, to_string_of<long> >::value));
    D_CV_CHECK(!(can_contramap<binary_type, predicate_of<long> >::value));
    D_CV_CHECK(!(can_contramap<binary_type, sink_of<long> >::value));

    // so is something that is not callable at all
    D_CV_CHECK(!(can_contramap<int, to_string_of<long> >::value));
    D_CV_CHECK(!(can_contramap<std::string, to_string_of<long> >::value));

    // the probe compiled, which is what "soft" means here
    D_CV_CHECK((std::is_base_of<std::true_type,
                                can_contramap<unary_type, to_string_of<long> >
                >::value));
    D_CV_CHECK((std::is_base_of<std::false_type,
                                can_contramap<binary_type, to_string_of<long> >
                >::value));

    return true;
}

#if D_CV_CONSTEXPR_TESTS
/*
tests_contramap_is_usable_in_constant_expressions
  D_CONSTEXPR on the generic operation is real.
  Tests the following:
  - contramapping a literal-type predicate is itself a constant expression
  - the adapted predicate answers inside static_assert
  - the adapted type is the expected rebound literal type
  - the operation chains within a single constant expression
*/
bool
tests_contramap_is_usable_in_constant_expressions()
{
    constexpr ct_predicate<long, ct_is_big> base{ ct_is_big() };

    // long -> bool becomes int -> bool, at compile time
    constexpr auto adapted = contramap(ct_doubled(), base);

    static_assert(adapted.run(6), "6 doubled is 12, which is big");
    static_assert(!adapted.run(4), "4 doubled is 8, which is not big");

    // the rebound literal type
    D_CV_CHECK((std::is_same<
                    decltype(adapted),
                    const ct_predicate<int, ct_composed<ct_is_big, ct_doubled> >
                >::value));
    D_CV_CHECK((std::is_same<contravariant_value_type_t<decltype(adapted)>,
                             int>::value));

    // chaining stays constant
    constexpr auto chained = contramap(ct_doubled(), adapted);

    static_assert(chained.run(3), "3 doubled twice is 12, which is big");
    static_assert(!chained.run(2), "2 doubled twice is 8, which is not big");

    // and the same objects answer at run time
    D_CV_CHECK(base.run(11));
    D_CV_CHECK(!base.run(10));
    D_CV_CHECK(adapted.run(6));
    D_CV_CHECK(!adapted.run(5));
    D_CV_CHECK(chained.run(3));

    return true;
}
#endif  // D_CV_CONSTEXPR_TESTS

NS_END  // testing
NS_END  // djinterp
