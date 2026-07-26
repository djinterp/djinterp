#include "contravariant_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_contravariant_law_identity_serializer
  Identity law -- contramap(id) == id -- on the serializer.
  Tests the following:
  - contramapping the identity arrow preserves the context's type
  - it renders identically to the original over a battery of inputs
  - including boundary values (zero, negatives)
  - applying identity twice changes nothing further
*/
bool
tests_contravariant_law_identity_serializer()
{
    to_string_of<long> show_long = make_show_long();

    auto identity = [](const long& _value) { return _value; };

    auto once  = contramap(identity, show_long);
    auto twice = contramap(identity, once);

    // the type is preserved, which is what makes the law statable
    D_CV_CHECK((std::is_same<decltype(once), to_string_of<long> >::value));
    D_CV_CHECK((std::is_same<decltype(twice), to_string_of<long> >::value));

    // and the behaviour is unchanged
    const long probes[] = { 0, 1, -1, 42, -273, 1000000 };
    std::size_t index   = 0;

    for (index = 0; index < sizeof(probes) / sizeof(probes[0]); ++index)
    {
        D_CV_CHECK(once.run(probes[index]) == show_long.run(probes[index]));
        D_CV_CHECK(twice.run(probes[index]) == show_long.run(probes[index]));
    }

    return true;
}

/*
tests_contravariant_law_identity_predicate
  Identity law on the predicate instance.
  Tests the following:
  - the contramapped context has the source's type
  - it agrees with the source on every probe value
  - it agrees on both sides of the decision boundary
  - a second identity contramap still agrees
*/
bool
tests_contravariant_law_identity_predicate()
{
    predicate_of<long> is_big{ [](const long& _value) { return _value > 10; } };

    auto identity = [](const long& _value) { return _value; };

    auto once  = contramap(identity, is_big);
    auto twice = contramap(identity, once);

    D_CV_CHECK((std::is_same<decltype(once), predicate_of<long> >::value));

    const long  probes[] = { -5, 0, 9, 10, 11, 250 };
    std::size_t index    = 0;

    for (index = 0; index < sizeof(probes) / sizeof(probes[0]); ++index)
    {
        D_CV_CHECK(once.run(probes[index]) == is_big.run(probes[index]));
        D_CV_CHECK(twice.run(probes[index]) == is_big.run(probes[index]));
    }

    // and the boundary really is being crossed by the battery
    D_CV_CHECK(!once.run(10));
    D_CV_CHECK(once.run(11));

    return true;
}

/*
tests_contravariant_law_identity_hook_instance
  Identity law on the hook-registered sink.
  Tests the following:
  - the contramapped sink has the source's type
  - the lines it records match the lines the source records
  - the count of recorded lines matches
  - the two sinks remain interchangeable across several accepts
*/
bool
tests_contravariant_law_identity_hook_instance()
{
    std::vector<std::string> direct_log;
    std::vector<std::string> mapped_log;

    sink_of<long> direct{
        &direct_log,
        [](const long& _value) { return "<" + long_to_text(_value) + ">"; } };

    sink_of<long> source{
        &mapped_log,
        [](const long& _value) { return "<" + long_to_text(_value) + ">"; } };

    auto identity = [](const long& _value) { return _value; };
    auto mapped   = contramap(identity, source);

    D_CV_CHECK((std::is_same<decltype(mapped), sink_of<long> >::value));

    const long  probes[] = { 0, 7, -7, 99 };
    std::size_t index    = 0;

    for (index = 0; index < sizeof(probes) / sizeof(probes[0]); ++index)
    {
        direct.accept(probes[index]);
        mapped.accept(probes[index]);
    }

    // same number of lines, same lines
    D_CV_CHECK(direct_log.size() == mapped_log.size());
    D_CV_CHECK(direct_log.size() == 4u);

    for (index = 0; index < direct_log.size(); ++index)
    {
        D_CV_CHECK(direct_log[index] == mapped_log[index]);
    }

    D_CV_CHECK(direct_log[1] == "<7>");

    return true;
}

/*
tests_contravariant_law_composition_serializer
  Composition law -- contramap(g) . contramap(f) == contramap(f . g) -- on the
  serializer.
  Tests the following:
  - the two-step and the fused form render identically
  - over inputs of several sizes, including the empty one
  - both sides end up consuming the same domain
  - the fused arrow really is f AFTER g, matching the reversed composition
*/
bool
tests_contravariant_law_composition_serializer()
{
    to_string_of<long> show_long = make_show_long();

    // f : std::string -> long
    auto f = [](const std::string& _text)
             {
                 return static_cast<long>(_text.size());
             };

    // g : std::vector<int> -> std::string
    auto g = [](const std::vector<int>& _items)
             {
                 return std::string(_items.size() * 4, 'x');
             };

    // contramap(g) . contramap(f)
    auto stepwise = contramap(g, contramap(f, show_long));

    // contramap(f . g)
    auto fused = contramap(
        [f, g](const std::vector<int>& _items) { return f(g(_items)); },
        show_long);

    // both consume the same domain
    D_CV_CHECK((std::is_same<decltype(stepwise),
                             to_string_of< std::vector<int> > >::value));
    D_CV_CHECK((std::is_same<decltype(stepwise), decltype(fused)>::value));

    // and render identically
    std::size_t size = 0;

    for (size = 0; size <= 5; ++size)
    {
        std::vector<int> items(size, 0);

        D_CV_CHECK(stepwise.run(items) == fused.run(items));
        D_CV_CHECK(stepwise.run(items) == show_long.run(f(g(items))));
    }

    // a concrete anchor: 3 items -> 12 characters -> "12"
    std::vector<int> three(3, 0);

    D_CV_CHECK(stepwise.run(three) == "12");

    return true;
}

/*
tests_contravariant_law_composition_predicate
  Composition law on the predicate instance.
  Tests the following:
  - the two-step and the fused form agree over the whole battery
  - the battery lands on both sides of the decision boundary
  - both sides agree with applying the arrows by hand
  - the two sides share one result type
*/
bool
tests_contravariant_law_composition_predicate()
{
    predicate_of<long> is_big{ [](const long& _value) { return _value > 10; } };

    auto f = [](const std::string& _text)
             {
                 return static_cast<long>(_text.size());
             };

    auto g = [](const std::vector<int>& _items)
             {
                 return std::string(_items.size() * 4, 'x');
             };

    auto stepwise = contramap(g, contramap(f, is_big));
    auto fused    = contramap(
        [f, g](const std::vector<int>& _items) { return f(g(_items)); },
        is_big);

    D_CV_CHECK((std::is_same<decltype(stepwise), decltype(fused)>::value));

    bool        saw_true  = false;
    bool        saw_false = false;
    std::size_t size      = 0;

    for (size = 0; size <= 5; ++size)
    {
        std::vector<int> items(size, 0);

        D_CV_CHECK(stepwise.run(items) == fused.run(items));
        D_CV_CHECK(stepwise.run(items) == is_big.run(f(g(items))));

        saw_true  = saw_true || stepwise.run(items);
        saw_false = saw_false || !stepwise.run(items);
    }

    // the battery genuinely straddles the boundary
    D_CV_CHECK(saw_true);
    D_CV_CHECK(saw_false);

    return true;
}

/*
tests_contravariant_law_composition_reverses_arrows
  The composition law reverses the arrows, and that reversal is observable.
  Tests the following:
  - with both arrows over one domain, contramap(g) . contramap(f) matches
    contramap(f . g)
  - it does NOT match contramap(g . f), so the order is not incidental
  - the two fused arrows are themselves different functions
  - the same distinction shows up on a second instance
*/
bool
tests_contravariant_law_composition_reverses_arrows()
{
    to_string_of<long> show_long = make_show_long();

    auto f = [](const long& _value) { return _value + 1; };   // long -> long
    auto g = [](const long& _value) { return _value * 2; };   // long -> long

    auto stepwise   = contramap(g, contramap(f, show_long));
    auto fused_f_g  = contramap(
        [f, g](const long& _value) { return f(g(_value)); }, show_long);
    auto fused_g_f  = contramap(
        [f, g](const long& _value) { return g(f(_value)); }, show_long);

    // the arrows really do differ at the probe point
    D_CV_CHECK(f(g(3)) == 7);
    D_CV_CHECK(g(f(3)) == 8);

    // the law holds for the reversed composition ...
    D_CV_CHECK(stepwise.run(3) == fused_f_g.run(3));
    D_CV_CHECK(stepwise.run(3) == "7");

    // ... and fails for the un-reversed one, which is the point
    D_CV_CHECK(stepwise.run(3) != fused_g_f.run(3));
    D_CV_CHECK(fused_g_f.run(3) == "8");

    // the same distinction on the predicate instance
    predicate_of<long> is_big{ [](const long& _value) { return _value > 7; } };

    auto pred_stepwise = contramap(g, contramap(f, is_big));
    auto pred_fused_gf = contramap(
        [f, g](const long& _value) { return g(f(_value)); }, is_big);

    D_CV_CHECK(!pred_stepwise.run(3));      // 3 -> 6 -> 7, not > 7
    D_CV_CHECK(pred_fused_gf.run(3));       // 3 -> 4 -> 8, > 7

    return true;
}

/*
tests_contravariant_law_identity_composition_interaction
  Identity is neutral wherever it is inserted into a composition.
  Tests the following:
  - inserting identity after an adapter changes nothing
  - inserting it before an adapter changes nothing
  - inserting it on both sides changes nothing
  - every variant keeps the same result type
*/
bool
tests_contravariant_law_identity_composition_interaction()
{
    to_string_of<long> show_long = make_show_long();

    auto f          = [](const std::string& _text)
                      {
                          return static_cast<long>(_text.size());
                      };
    auto id_long    = [](const long& _value) { return _value; };
    auto id_string  = [](const std::string& _text) { return _text; };

    auto plain      = contramap(f, show_long);
    auto id_after   = contramap(id_string, contramap(f, show_long));
    auto id_before  = contramap(f, contramap(id_long, show_long));
    auto id_both    = contramap(id_string, contramap(f, contramap(id_long, show_long)));

    // one type throughout
    D_CV_CHECK((std::is_same<decltype(plain), to_string_of<std::string> >::value));
    D_CV_CHECK((std::is_same<decltype(plain), decltype(id_after)>::value));
    D_CV_CHECK((std::is_same<decltype(plain), decltype(id_before)>::value));
    D_CV_CHECK((std::is_same<decltype(plain), decltype(id_both)>::value));

    // one behaviour throughout
    const char* probes[] = { "", "a", "hello", "abcdefghij" };
    std::size_t index    = 0;

    for (index = 0; index < sizeof(probes) / sizeof(probes[0]); ++index)
    {
        const std::string text(probes[index]);

        D_CV_CHECK(id_after.run(text) == plain.run(text));
        D_CV_CHECK(id_before.run(text) == plain.run(text));
        D_CV_CHECK(id_both.run(text) == plain.run(text));
    }

    D_CV_CHECK(plain.run("hello") == "5");

    return true;
}

/*
tests_contravariant_law_composition_associative_three_stage
  The law composes: three arrows agree under every bracketing.
  Tests the following:
  - the fully stepwise form, the two partially fused forms, and the fully
    fused form all agree
  - they agree over a battery of inputs
  - they share one result type
  - the chain really is being exercised (the anchor value is non-trivial)
*/
bool
tests_contravariant_law_composition_associative_three_stage()
{
    to_string_of<long> show_long = make_show_long();

    // f : std::string -> long, g : std::vector<int> -> std::string,
    // h : int -> std::vector<int>
    auto f = [](const std::string& _text)
             {
                 return static_cast<long>(_text.size());
             };
    auto g = [](const std::vector<int>& _items)
             {
                 return std::string(_items.size() * 4, 'x');
             };
    auto h = [](const int& _count)
             {
                 return std::vector<int>(static_cast<std::size_t>(_count), 0);
             };

    auto stepwise   = contramap(h, contramap(g, contramap(f, show_long)));
    auto fuse_inner = contramap(
        h,
        contramap([f, g](const std::vector<int>& _items) { return f(g(_items)); },
                  show_long));
    auto fuse_outer = contramap(
        [g, h](const int& _count) { return g(h(_count)); },
        contramap(f, show_long));
    auto fuse_all   = contramap(
        [f, g, h](const int& _count) { return f(g(h(_count))); },
        show_long);

    // one type for every bracketing
    D_CV_CHECK((std::is_same<decltype(stepwise), to_string_of<int> >::value));
    D_CV_CHECK((std::is_same<decltype(stepwise), decltype(fuse_inner)>::value));
    D_CV_CHECK((std::is_same<decltype(stepwise), decltype(fuse_outer)>::value));
    D_CV_CHECK((std::is_same<decltype(stepwise), decltype(fuse_all)>::value));

    // one answer for every bracketing
    int count = 0;

    for (count = 0; count <= 4; ++count)
    {
        D_CV_CHECK(stepwise.run(count) == fuse_inner.run(count));
        D_CV_CHECK(stepwise.run(count) == fuse_outer.run(count));
        D_CV_CHECK(stepwise.run(count) == fuse_all.run(count));
    }

    // the anchor: 3 -> a 3-element vector -> 12 characters -> "12"
    D_CV_CHECK(stepwise.run(3) == "12");
    D_CV_CHECK(stepwise.run(0) == "0");

    return true;
}

/*
tests_contravariant_law_composition_result_types_agree
  The law is a type-level identity as well as a behavioural one.
  Tests the following:
  - the stepwise and fused forms have the same static type on the serializer
  - and on the predicate
  - and on the hook-registered sink
  - and that type is F<B> for the outermost adapter's domain
*/
bool
tests_contravariant_law_composition_result_types_agree()
{
    to_string_of<long> show_long = make_show_long();
    predicate_of<long> is_big{ [](const long& _value) { return _value > 10; } };

    std::vector<std::string> log;
    sink_of<long>            raw{
        &log,
        [](const long& _value) { return long_to_text(_value); } };

    auto f = [](const std::string& _text)
             {
                 return static_cast<long>(_text.size());
             };
    auto g = [](const std::vector<int>& _items)
             {
                 return std::string(_items.size() * 4, 'x');
             };
    auto fused = [f, g](const std::vector<int>& _items) { return f(g(_items)); };

    // the serializer
    D_CV_CHECK((std::is_same<decltype(contramap(g, contramap(f, show_long))),
                             decltype(contramap(fused, show_long))>::value));

    // the predicate
    D_CV_CHECK((std::is_same<decltype(contramap(g, contramap(f, is_big))),
                             decltype(contramap(fused, is_big))>::value));

    // the hook-registered sink
    D_CV_CHECK((std::is_same<decltype(contramap(g, contramap(f, raw))),
                             decltype(contramap(fused, raw))>::value));

    // and the shared type is F<B> for the outermost domain
    D_CV_CHECK((std::is_same<decltype(contramap(fused, show_long)),
                             to_string_of< std::vector<int> > >::value));
    D_CV_CHECK((std::is_same<decltype(contramap(fused, is_big)),
                             predicate_of< std::vector<int> > >::value));
    D_CV_CHECK((std::is_same<decltype(contramap(fused, raw)),
                             sink_of< std::vector<int> > >::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
