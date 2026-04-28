/******************************************************************************
* djinterp [core]                                                  parser.hpp
*
* Generic parser base:
*   This header defines the CRTP (Curiously Recurring Template Pattern) base
* from which all concrete parsers derive. It establishes the minimum
* structural interface that every parser must expose, regardless of input
* kind (text, binary, or otherwise).
*
*   The base enforces its contract purely through SFINAE and static
* assertions - no virtual functions, no tag types. A conforming derived
* parser must provide:
*   - `input_type`    typedef  - the element type of the input stream
*   - `result_type`   typedef  - the type produced on a successful parse
*   - `do_parse(parse_state<input_type>&) -> parse_result<result_type>`
*
*
* path:      /inc/cpp/parse/parser.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.15
******************************************************************************/

#ifndef DJINTERP_PARSER_
#define DJINTERP_PARSER_ 1

#include <cstddef>
#include <type_traits>
#include "../core/djinterp.hpp"
#include "./parse.hpp"
#include "./parser_traits.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  parser_base
// ================================================================

// parser_base
//   class: CRTP base for all parsers.  Provides the public
// `parse` entry point which delegates to the derived class's
// `do_parse`.  Performs compile-time validation of the derived
// type's structural conformance.
//
//   _Derived must expose:
//     - `using input_type  = ...;`
//     - `using result_type = ...;`
//     - `parse_result<result_type> do_parse(parse_state<input_type>&);`
template<typename _Derived>
class parser_base
{
private:
    using derived_type = _Derived;

    // self
    //   returns a reference to the derived instance.
    derived_type& self()
    {
        return static_cast<derived_type&>(*this);
    }

    // self (const)
    //   returns a const reference to the derived instance.
    const derived_type& self() const
    {
        return static_cast<const derived_type&>(*this);
    }

protected:
    parser_base()
    {};


    ~parser_base()
    {};


public:
    // --------------------------------------------------------
    //  Structural contract assertions
    // --------------------------------------------------------
    // Deferred to first use of parse() so that incomplete types
    // at class-definition time do not trigger false failures.

    // parse
    //   invokes the derived parser's do_parse on the given state.
    // Performs a one-time static check that _Derived satisfies
    // the parser structural contract.
    auto parse(parse_state<typename derived_type::input_type>& _state)
        -> parse_result<typename derived_type::result_type>
    {
        static_assert(
            traits::has_input_type<derived_type>::value,
            "Parser must define a public `input_type` typedef.");

        static_assert(
            traits::has_result_type<derived_type>::value,
            "Parser must define a public `result_type` typedef.");

        static_assert(
            traits::has_do_parse_method<derived_type>::value,
            "Parser must define a public `do_parse` member function "
            "accepting parse_state<input_type>& and returning "
            "parse_result<result_type>.");

        return self().do_parse(_state);
    }

    // parse (convenience - raw pointer + length)
    //   constructs a parse_state and delegates.
    auto parse(const typename derived_type::input_type* _data,
               std::size_t                              _length)
        -> parse_result<typename derived_type::result_type>
    {
        parse_state<typename derived_type::input_type> state(_data,
                                                             _length);

        return parse(state);
    }

    // parse (convenience - initializer_list)
    //   constructs a parse_state from the list's range and
    // delegates.
    auto parse(
        std::initializer_list<typename derived_type::input_type> _list
    )   -> parse_result<typename derived_type::result_type>
    {
        parse_state<typename derived_type::input_type> state(
            _list.begin(),
            _list.size()
        );

        return parse(state);
    }
};


// ================================================================
//  parser combinators (free functions)
// ================================================================
// Minimal combinator vocabulary operating on any parser_base-
// derived type.  Extended combinators belong in a dedicated
// combinator header.

NS_INTERNAL

    // sequence_pair_result
    //   trait: helper producing the result type for a two-parser
    // sequence: a std::pair of the individual result types.
    template<typename _ParserA,
             typename _ParserB>
    struct sequence_pair_result
    {
        using type = std::pair<
            typename _ParserA::result_type,
            typename _ParserB::result_type
        >;
    };

NS_END  // internal


// sequence
//   function: runs _ParserA followed by _ParserB on the same
// state.  Succeeds only if both succeed, rolling back on
// failure of the second.
template<typename _ParserA,
         typename _ParserB>
auto sequence(_ParserA& _a,
              _ParserB& _b,
              parse_state<typename _ParserA::input_type>& _state)
    -> parse_result<
           typename internal::sequence_pair_result<_ParserA,
                                                   _ParserB>::type
       >
{
    using pair_type =
        typename internal::sequence_pair_result<_ParserA,
                                                _ParserB>::type;
    using result_type = parse_result<pair_type>;

    static_assert(
        std::is_same<
            typename _ParserA::input_type,
            typename _ParserB::input_type
        >::value,
        "Sequenced parsers must share the same input_type.");

    std::size_t saved_offset = _state.offset;

    auto first = _a.parse(_state);

    if (!first.ok())
    {
        return result_type(first.error());
    }

    auto second = _b.parse(_state);

    if (!second.ok())
    {
        // roll back
        _state.offset = saved_offset;

        return result_type(second.error());
    }

    return result_type(
        pair_type(first.value(), second.value())
    );
}

// alternative
//   function: tries _ParserA; on failure, resets state and tries
// _ParserB.  Both parsers must share the same input_type and
// result_type.
template<typename _ParserA,
         typename _ParserB>
auto alternative(_ParserA&                                    _a,
                 _ParserB&                                    _b,
                 parse_state<typename _ParserA::input_type>&  _state)
    -> parse_result<typename _ParserA::result_type>
{
    using result_type = parse_result<typename _ParserA::result_type>;

    static_assert(
        std::is_same<
            typename _ParserA::input_type,
            typename _ParserB::input_type
        >::value,
        "Alternative parsers must share the same input_type.");

    static_assert(
        std::is_same<
            typename _ParserA::result_type,
            typename _ParserB::result_type
        >::value,
        "Alternative parsers must share the same result_type.");

    std::size_t saved_offset = _state.offset;

    auto first = _a.parse(_state);

    if (first.ok())
    {
        return first;
    }

    // reset and try the alternative
    _state.offset = saved_offset;

    return _b.parse(_state);
}


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSER_
