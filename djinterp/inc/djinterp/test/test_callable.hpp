/******************************************************************************
* djinterp [test]                                            test_callable.hpp
*
*   The test_callable_table: the out-of-line store of deferred test work
* that test_common.hpp / test_object.hpp already reference but do not
* define.  A test_object stays a flat, trivially-copyable, constexpr-
* friendly value precisely BECAUSE its runtime work does not live inline:
* the node carries only a test_callable_id, and the actual closure lives
* here, in a table the handler (or the builder in test_builder.hpp) binds
* and invokes during the tree walk.
*
*   A TEST IS A THUNK:
*   The unit stored here is a deferred boolean computation - a
* `std::function<bool()>`.  This is the functional core of the whole
* framework: a leaf's pass/fail is the value produced by running its
* thunk, and richer leaves are built by COMPOSING thunks under boolean
* algebra (short-circuit AND for conjoined assertions, OR for
* alternatives).  The node remains data; the behaviour is a value in
* this table.
*
*   ID DISCIPLINE:
*   Ids are 1-based.  The value 0 (k_no_callable, from test_common.hpp)
* is the reserved "no deferred work" sentinel and is never handed out:
* a node carrying id 0 is fully evaluated by data already in its
* m_result / m_status.  add() returns the first free id (>= 1); the
* table grows by append, so an id, once issued, is stable for the life
* of the table.
*
*   COMPOSITION:
*   compose_and / compose_or fold a new clause into the thunk already
* bound to an id, under the obvious short-circuit semantics.  These are
* the table-level spelling of the predicate combinators in
* functional/predicate.hpp (predicate_and / all_of, predicate_or /
* any_of); the table erases the clause types to thunk_type so leaves of
* mixed assertion shapes share one storage row.
*
*   RUNTIME ONLY:
*   std::function is a runtime facility; binding and invocation are not
* constant expressions.  This matches the framework's existing split:
* test_object EVALUATION may be constexpr (D_TEST_CONSTEXPR), but
* deferred-callable dispatch - like event dispatch - is runtime-only.
*
*   PORTABILITY:
*   C++11 minimum (std::function, lambdas, <utility>).
*
*
* TABLE OF CONTENTS
* =================
* I.    PORTABILITY CHECKS
* II.   TEST CALLABLE TABLE
*
*
* path:      /inc/djinterp/test/test_callable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.17
******************************************************************************/

#ifndef DJINTERP_TEST_CALLABLE_
#define DJINTERP_TEST_CALLABLE_ 1

#ifndef __cplusplus
    #error "test_callable.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_callable.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST CALLABLE TABLE                                  ///
///////////////////////////////////////////////////////////////////////////////

// test_callable_table
//   class: the out-of-line store of deferred test work.  Maps each
// issued test_callable_id to a `std::function<bool()>` thunk and lets
// the owner invoke or further compose it.  A test_object references a
// row by id only, so it stays flat and trivially copyable while its
// runtime work lives here.
//
//   The reserved id 0 (k_no_callable) is never stored or returned;
// row storage is 1-based, so id `n` lives at index `n - 1`.
//
// Usage:
//   test_callable_table tbl;
//   test_callable_id id = tbl.add([]{ return 1 + 1 == 2; });
//   tbl.compose_and(id, []{ return true; });   // conjoin a clause
//   bool ok = tbl.invoke(id);                   // run the composed thunk
class test_callable_table
{
public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------

    // thunk_type
    //   type: one deferred boolean computation - the value a test leaf
    // reduces to when run.
    using thunk_type = std::function<bool()>;

    using size_type  = std::size_t;


    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // test_callable_table
    //   constructor: default.  Empty table; the first add() issues id 1.
    test_callable_table()
        : m_thunks()
    {}


    // -----------------------------------------------------------------
    //  binding
    // -----------------------------------------------------------------

    // add
    //   stores _thunk as a new row and returns its freshly-issued id
    // (>= 1).  The returned id is stable for the life of the table.
    test_callable_id
    add(
        thunk_type _thunk
    )
    {
        m_thunks.push_back(static_cast<thunk_type&&>(_thunk));

        // ids are 1-based, so the id is the new size, not size - 1.
        return static_cast<test_callable_id>(m_thunks.size());
    }


    // -----------------------------------------------------------------
    //  composition (boolean algebra over thunks)
    // -----------------------------------------------------------------

    // compose_and
    //   conjoins _clause onto the thunk bound to _id under short-circuit
    // AND: the composed thunk passes iff the existing thunk AND _clause
    // both pass.  A no-op (other than binding _clause) if _id is unbound.
    void
    compose_and(
        test_callable_id _id,
        thunk_type       _clause
    )
    {
        compose_with(_id, static_cast<thunk_type&&>(_clause), true);

        return;
    }

    // compose_or
    //   disjoins _clause onto the thunk bound to _id under short-circuit
    // OR: the composed thunk passes iff the existing thunk OR _clause
    // passes.
    void
    compose_or(
        test_callable_id _id,
        thunk_type       _clause
    )
    {
        compose_with(_id, static_cast<thunk_type&&>(_clause), false);

        return;
    }


    // transform
    //   replaces the thunk bound to _id with _mapper applied to it.  The
    // mapper receives the current thunk and returns its replacement,
    // letting a caller WRAP a row - e.g. fold a side effect onto failure,
    // or memoize a result - without disturbing any other row.  No-op if
    // _id is unbound.
    void
    transform(
        test_callable_id                            _id,
        std::function<thunk_type(thunk_type)>       _mapper
    )
    {
        if (!has(_id))
        {
            return;
        }

        m_thunks[index_of(_id)] = _mapper(m_thunks[index_of(_id)]);

        return;
    }


    // -----------------------------------------------------------------
    //  query / invocation
    // -----------------------------------------------------------------

    // has
    //   true iff _id is a non-sentinel id with a bound, callable row.
    bool
    has(
        test_callable_id _id
    ) const D_NOEXCEPT
    {
        return ( (_id != k_no_callable)              &&
                 (index_of(_id) < m_thunks.size())   &&
                 (static_cast<bool>(m_thunks[index_of(_id)])) );
    }

    // invoke
    //   runs the thunk bound to _id and returns its result.  Returns
    // false if _id is unbound (so an absent row reads as a non-passing
    // leaf rather than throwing).
    bool
    invoke(
        test_callable_id _id
    ) const
    {
        if (!has(_id))
        {
            return false;
        }

        return m_thunks[index_of(_id)]();
    }

    // size
    //   number of rows issued so far (the highest live id).
    size_type
    size() const D_NOEXCEPT
    {
        return m_thunks.size();
    }

    // empty
    bool
    empty() const D_NOEXCEPT
    {
        return m_thunks.empty();
    }

    // clear
    //   drops every row.  Ids issued before a clear() must not be reused.
    void
    clear()
    {
        m_thunks.clear();

        return;
    }


private:
    // -----------------------------------------------------------------
    //  internal: id <-> index
    // -----------------------------------------------------------------

    // index_of
    //   maps a 1-based id to its 0-based storage index.  Only meaningful
    // for non-sentinel ids; callers guard with has().
    static size_type
    index_of(
        test_callable_id _id
    ) D_NOEXCEPT
    {
        return (static_cast<size_type>(_id) - 1);
    }


    // -----------------------------------------------------------------
    //  internal: composition core
    // -----------------------------------------------------------------

    // compose_with
    //   folds _clause into the row at _id.  When _conjoin is true the
    // fold is short-circuit AND, otherwise short-circuit OR.  If _id is
    // unbound the clause simply becomes the row's thunk.
    void
    compose_with(
        test_callable_id _id,
        thunk_type       _clause,
        bool             _conjoin
    )
    {
        // an unbound id cannot be folded into; bind the clause alone.
        if (!has(_id))
        {
            if ( (_id != k_no_callable) &&
                 (index_of(_id) < m_thunks.size()) )
            {
                m_thunks[index_of(_id)] = static_cast<thunk_type&&>(_clause);
            }

            return;
        }

        thunk_type existing = m_thunks[index_of(_id)];

        if (_conjoin)
        {
            m_thunks[index_of(_id)] =
                [existing, _clause]() -> bool
                {
                    return ( existing() && _clause() );
                };
        }
        else
        {
            m_thunks[index_of(_id)] =
                [existing, _clause]() -> bool
                {
                    return ( existing() || _clause() );
                };
        }

        return;
    }


    std::vector<thunk_type> m_thunks;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_CALLABLE_
