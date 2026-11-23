/*******************************************************************************
* djinterp [cpp][math]                                               program.hpp
*
* A term compiled once, to be run many times: expr_evaluate<T>'s fold made a
* flat program, its names resolved, its variables in slots (S395).
*   WHY. The typed fold is one fold of parse/expression's over the term: at
* each node it builds a vector of its operands' answers, each answer a result
* that can hold an error's text, and it finds each function, each constant
* and each variable by its name. A term evaluated many times at other
* values - plotted, integrated, solved for, fitted - pays all of that each
* time: some 130 times the cost of the same arithmetic written in C++
* (2026.11.12). A program pays it once.
*   WHAT IT IS. expr_compile<T>(term) walks the term once, as the fold does,
* and writes its steps in the order the fold computes them, each operand
* before its operation, left to right: a literal rounded once to T, a
* constant's value in T, a variable's slot, an operator of the table's, a
* function's evaluator, indexing's NaN. A run takes the variables' values in
* their slots' order and does the steps on a stack whose greatest depth the
* compiler counted, in the caller's frame where it is within
* D_CFG_MATH_EXPR_PROGRAM_STACK (cfg_expr.h), so that a run of a program in
* float, double or long double allocates nothing, nor one of math::values
* or of bindings of up to eight variables. Each step is the fold's
* own operation on the same operands in the same order, so a run's answer
* is the fold's, bit for bit, in every type the fold takes.
*   ERRORS. Every error the fold can give in a type is known before it
* runs, but for an unbound variable: a literal (bad_shape), a name of no
* function or constant (unknown_name), a number of operands an operator or
* a function does not take (wrong_arity), a node of no kind the fold
* computes. expr_compile gives the error the fold would, the first in the
* fold's order; a program, once made, runs to a number. A variable the
* compiler is not given a slot for is unbound_variable, as the fold's is
* where no binding names it.
*   VARIABLES. expr_compile<T>(term) gives a slot to each of the term's
* variables, in the order they are first read, left to right
* (expr_variables); expr_compile<T>(term, names) to each of names, in its
* order, whether the term reads it or not. run(values) takes the values in
* that order, T's own in a real type, or math::values, each taken as the
* fold takes a bound value; run(bindings) finds them by name, as the fold
* does, and is unbound_variable for the first variable read, in the fold's
* order, that no binding names.
*   THE CONTEXT is given to each run, and from it to each function's
* evaluator, as the fold gives it (S340).
*   IN A COMPLEX TYPE a program computes as the fold does there (S339): a
* real number stays real, a real value bound real and a complex one
* complex, so a run takes math::values, or bindings, and no math::complex
* of its own, which could not say which of its numbers are real. Each
* operation takes its operands where they lie on the stack, as the fold's
* takes them from its vector (evaluate.hpp, expr_operands), so that a run in
* a complex type allocates nothing either, but for a call of more than eight
* arguments.
*   As a value, expr_evaluate with a context, a term is not compiled: a
* value's arithmetic allocates as it goes, matrices among its results, and
* its errors are values.
*
*   STEPS. 2026.11.13 S395: new.
*
*   Build: included, as the language is, through expr/expr.hpp, from C++11.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/program.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE PROGRAM
    -----------
    1.  Its steps
         1.  expr_program_code
         2.  expr_program_step
    2.  How it computes in each type
         1.  expr_program_ops
         2.  expr_program_ops<math::complex<Type> >
    3.  expr_program
2.  COMPILING
    ---------
    1.  A term's variables
         1.  expr_variables
    2.  The compiler
         1.  expr_program_builder
    3.  expr_compile
*/

#ifndef DJINTERP_MATH_EXPR_PROGRAM_HPP
#define DJINTERP_MATH_EXPR_PROGRAM_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../config/cpp/modules/math/expr/cfg_expr.h"  // D_CFG_MATH_EXPR_PROGRAM_STACK
#include "../../parse/expression/expression_ops.hpp"    // evaluate
#include "../math_common.hpp"                           // NS_MATH
#include "../value.hpp"                                 // value
#include "./evaluate.hpp"                               // the fold's steps
#include "./expr_common.hpp"                            // expr_result
#include "./function_table.hpp"                         // expr_function
#include "./term.hpp"                                   // expr_term


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE PROGRAM
//==============================================================================


// 1.1    Its steps
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // expr_program_code
    //   enum: what a step does: push a constant, push a variable's value, or
    // take its operands from the stack and push what an operator, a
    // function's evaluator, or indexing gives of them.
    enum class expr_program_code
    {
        constant,
        variable,
        apply,
        call,
        nan
    };

    // 1.1.2
    // expr_program_step
    //   struct: one step: its code; an operator's kind; the constant's or
    // the variable's index; the operands it takes from the stack; and a
    // call's function, the standard table's, which lives as long as the
    // program.
    struct expr_program_step
    {
        expr_program_code    code;
        expr_op_kind         op;
        std::size_t          index;
        std::size_t          arity;
        const expr_function* function;
    };

NS_END  // internal

// 1.2    How it computes in each type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
    // expr_program_ops
    //   struct: how a program computes in Type, a floating-point type, the
    // fold's operations on operands where they lie on the stack, with no
    // vector: an operator of the table's (expr_apply_in), a function's
    // evaluator, given the context (S340); and the run's answer, the
    // number carried.
    template<typename Type>
    struct expr_program_ops
    {
        typedef expr_typed<Type> typed;
        typedef Type             carrier;
        typedef Type             answer;

        static carrier
        apply(
            expr_op_kind        _kind,
            const carrier*      _x,
            std::size_t         _n,
            const expr_context& _context
        )
        {
            static_cast<void>(_n);
            static_cast<void>(_context);

            return expr_apply_in<Type>(_kind, _x);
        }

        static carrier
        call(
            const expr_function& _function,
            const carrier*       _x,
            std::size_t          _n,
            const expr_context&  _context
        )
        {
            return expr_evaluator_in<Type>(_function)(_x, _n, _context);
        }

        static answer
        answer_of(
            const carrier& _x
        )
        {
            return _x;
        }
    };

    // 1.2.2
    // expr_program_ops<math::complex<Type> >
    //   struct: the same in math::complex<Type>, the fold's carrier, a
    // complex number and whether it is real (S339): each operation the
    // fold's, on its operands where they lie; and the answer as the fold
    // gives it, a real number's imaginary part +0.
    template<typename Type>
    struct expr_program_ops<math::complex<Type> >
    {
        typedef expr_typed<math::complex<Type> > typed;
        typedef expr_complex_carrier<Type>       carrier;
        typedef math::complex<Type>              answer;

        static carrier
        apply(
            expr_op_kind        _kind,
            const carrier*      _x,
            std::size_t         _n,
            const expr_context& _context
        )
        {
            return typed::apply(_kind, _x, _n, _context);
        }

        static carrier
        call(
            const expr_function& _function,
            const carrier*       _x,
            std::size_t          _n,
            const expr_context&  _context
        )
        {
            return typed::call(_function, _x, _n, _context);
        }

        static answer
        answer_of(
            const carrier& _x
        )
        {
            return ( (_x.real) ? answer(_x.z.real())
                               : _x.z );
        }
    };

    template<typename Type>
    class expr_program_builder;

NS_END  // internal

// 1.3    expr_program
//------------------------------------------------------------------------------
// expr_program
//   class: a term compiled for Type, float, double or long double or the
// math::complex of one: its steps in the fold's order, its constants in
// Type, its variables' names in their slots' order, and the stack's
// greatest depth. A run is const and keeps nothing between runs, so one
// program may be run on many threads at once. A program made by default
// has no variable and runs to a NaN. Made by expr_compile.
template<typename Type>
class expr_program
{
public:
    typedef internal::expr_program_ops<Type> ops;
    typedef typename ops::carrier            carrier;
    typedef typename ops::answer             answer_type;

    // a NaN, of no variable.
    expr_program()
        : m_steps(),
          m_constants(),
          m_variables(),
          m_first(),
          m_depth(1)
    {
        const internal::expr_program_step nan = {
            internal::expr_program_code::nan, expr_op_kind::pos, 0, 0,
            D_NULLPTR };

        m_steps.push_back(nan);
    }

    // variables
    //   the variables' names, in the order run takes their values.
    D_NODISCARD const std::vector<std::string>&
    variables() const D_NOEXCEPT
    {
        return m_variables;
    }

    // size, depth
    //   the program's steps, and the greatest depth its stack reaches.
    D_NODISCARD std::size_t
    size() const D_NOEXCEPT
    {
        return m_steps.size();
    }

    D_NODISCARD std::size_t
    depth() const D_NOEXCEPT
    {
        return m_depth;
    }

    // run
    //   the term's value at _values, one for each of variables(), in its
    // order, under _context: Type's own numbers, which only a program of a
    // floating-point type takes; or math::values, each taken as the fold
    // takes a bound value.
    D_NODISCARD answer_type
    run(
        const Type*         _values,
        const expr_context& _context = expr_context()
    ) const
    {
        D_STATIC_ASSERT(internal::expr_floating<Type>::value,
                        "expr_program<Type>::run: Type's own numbers are a "
                        "program's values in float, double or long double; "
                        "in a complex type, run takes math::values, which "
                        "say which numbers are real.");

        return run_carried(_values, _context);
    }

    D_NODISCARD answer_type
    run(
        const value*        _values,
        const expr_context& _context = expr_context()
    ) const
    {
        internal::expr_operands<carrier> carried(m_variables.size());

        // each value as the fold takes it
        for (std::size_t i = 0; i < m_variables.size(); ++i)
        {
            carried[i] = ops::typed::bound(_values[i]);
        }

        return run_carried(carried.data(), _context);
    }

    D_NODISCARD answer_type
    run(
        const std::vector<value>& _values,
        const expr_context&       _context = expr_context()
    ) const
    {
        return run(_values.data(), _context);
    }

    // run
    //   the term's value at _bindings, which give the variables theirs by
    // name, as the fold reads them; or unbound_variable for the first
    // variable the term reads, in the fold's order, that they do not name.
    D_NODISCARD expr_result<answer_type>
    run(
        const expr_bindings& _bindings,
        const expr_context&  _context = expr_context()
    ) const
    {
        internal::expr_operands<carrier> carried(m_variables.size());
        std::size_t                      missing = m_variables.size();

        // each variable's binding as the fold takes it, a NaN in a slot the
        // term does not read; the first read of those unbound
        for (std::size_t i = 0; i < m_variables.size(); ++i)
        {
            const value* bound = _bindings.find(m_variables[i]);

            if (bound != D_NULLPTR)
            {
                carried[i] = ops::typed::bound(*bound);
            }
            else
            {
                carried[i] = ops::typed::nan();

                if ( (m_first[i] < m_steps.size()) &&
                     ( (missing == m_variables.size()) ||
                       (m_first[i] < m_first[missing]) ) )
                {
                    missing = i;
                }
            }
        }

        // a variable read and not bound: the fold's error
        if (missing < m_variables.size())
        {
            return err<answer_type, expr_error>(
                internal::expr_unbound(m_variables[missing]));
        }

        return ok<answer_type, expr_error>(run_carried(carried.data(),
                                                       _context));
    }

private:
    friend class internal::expr_program_builder<Type>;

    // run_carried
    //   the steps, on the variables' carriers _x: the stack in the frame
    // where the depth allows, else in a vector.
    answer_type
    run_carried(
        const carrier*      _x,
        const expr_context& _context
    ) const
    {
        // the stack: in the frame, or on the heap past the knob
        if (m_depth <= D_CFG_MATH_EXPR_PROGRAM_STACK)
        {
            carrier frame[D_CFG_MATH_EXPR_PROGRAM_STACK];

            return steps_on(frame, _x, _context);
        }

        std::vector<carrier> heap(m_depth);

        return steps_on(heap.data(), _x, _context);
    }

    // steps_on
    //   each step in turn on _stack, from empty; the answer, its one entry.
    answer_type
    steps_on(
        carrier*            _stack,
        const carrier*      _x,
        const expr_context& _context
    ) const
    {
        typedef internal::expr_program_code code;

        std::size_t top = 0;

        // each step: push, or take its operands and push what they give
        for (std::size_t i = 0; i < m_steps.size(); ++i)
        {
            const internal::expr_program_step& step = m_steps[i];

            switch (step.code)
            {
                case code::constant:
                    _stack[top++] = m_constants[step.index];
                    break;
                case code::variable:
                    _stack[top++] = _x[step.index];
                    break;
                case code::apply:
                    top           -= step.arity;
                    _stack[top]    = ops::apply(step.op, &_stack[top],
                                                step.arity, _context);
                    ++top;
                    break;
                case code::call:
                    top           -= step.arity;
                    _stack[top]    = ops::call(*step.function, &_stack[top],
                                               step.arity, _context);
                    ++top;
                    break;
                case code::nan:
                default:
                    top           -= step.arity;
                    _stack[top++]  = ops::typed::nan();
                    break;
            }
        }

        return ops::answer_of(_stack[0]);
    }

    std::vector<internal::expr_program_step> m_steps;
    std::vector<carrier>                     m_constants;
    std::vector<std::string>                 m_variables;
    std::vector<std::size_t>                 m_first;
    std::size_t                              m_depth;
};



//==============================================================================
// 2.  COMPILING
//==============================================================================


// 2.1    A term's variables
//------------------------------------------------------------------------------
// 2.1.1
// expr_variables
/**
 * @brief A term's variables, each once, in the order the fold first reads
 *        them, left to right: the slots expr_compile gives them.
 *
 * @param[in] _term  the term.
 * @return the names, none for a term made by default.
 */
D_NODISCARD inline std::vector<std::string>
expr_variables(
    const expr_term& _term
)
{
    std::vector<std::string> names;

    // no term: no variable
    if (_term.empty())
    {
        return names;
    }

    static_cast<void>(::djinterp::evaluate<int, expr_op, expr_atom>(
        _term,
        // a leaf: a variable's name, where it is new
        [&names](const expr_atom& _atom) -> int
        {
            if (_atom.kind == expr_atom_kind::variable)
            {
                bool seen = false;

                for (std::size_t i = 0; i < names.size(); ++i)
                {
                    seen = ( (seen) ||
                             (names[i] == _atom.text) );
                }

                if (!seen)
                {
                    names.push_back(_atom.text);
                }
            }

            return 0;
        },
        // an application: nothing of its own
        [](const expr_op&, const std::vector<int>&) -> int
        {
            return 0;
        }));

    return names;
}

// 2.2    The compiler
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.2.1
    // expr_program_builder
    //   class: a program being written, one step at a time, in the fold's
    // order, and the first error the fold would give, after which nothing is
    // written: leaf writes a leaf's step, application an operation's; each
    // returns the stack's depth its subterm reaches.
    template<typename Type>
    class expr_program_builder
    {
    public:
        typedef expr_program_ops<Type>     ops;
        typedef typename ops::typed        typed;
        typedef typename ops::carrier      carrier;

        explicit expr_program_builder(
            const std::vector<std::string>& _variables
        )
            : m_program(),
              m_failed(false),
              m_error()
        {
            m_program.m_steps.clear();
            m_program.m_variables = _variables;
            m_program.m_first.assign(_variables.size(), npos());
        }

        // leaf
        //   a number's or a constant's value in Type, or a variable's slot;
        // a name of no constant, or a variable without a slot, the error.
        std::size_t
        leaf(
            const expr_atom& _atom
        )
        {
            // a number: its literal rounded once, as the fold rounds it
            if (_atom.kind == expr_atom_kind::number)
            {
                push_constant(typed::number(_atom));

                return 1;
            }

            // a constant: its value in Type
            if (_atom.kind == expr_atom_kind::constant)
            {
                const expr_result<const expr_named_constant*> constant =
                    expr_constant_of(_atom.text);

                if (constant.is_err())
                {
                    fail(constant.error());

                    return 1;
                }

                push_constant(typed::constant(*constant.value()));

                return 1;
            }

            push_variable(_atom.text);

            return 1;
        }

        // application
        //   _op of operands whose subterms reach the depths _depths: the
        // fold's step, or its error.
        std::size_t
        application(
            const expr_op&                  _op,
            const std::vector<std::size_t>& _depths
        )
        {
            const std::size_t n     = _depths.size();
            std::size_t       depth = 1;

            // the deepest of the operands, each above those before it
            for (std::size_t i = 0; i < n; ++i)
            {
                depth = ( ((i + _depths[i]) > depth) ? (i + _depths[i])
                                                     : depth );
            }

            write_operation(_op, n);

            return depth;
        }

        // made
        //   the program, of the depth _depth; or the first error.
        expr_result<expr_program<Type> >
        made(
            std::size_t _depth
        )
        {
            if (m_failed)
            {
                return err<expr_program<Type>, expr_error>(m_error);
            }

            m_program.m_depth = _depth;

            return ok<expr_program<Type>, expr_error>(m_program);
        }

    private:
        static std::size_t
        npos()
        {
            return static_cast<std::size_t>(-1);
        }

        // fail
        //   the first error the fold would give; nothing written after it.
        void
        fail(
            const expr_error& _error
        )
        {
            if (!m_failed)
            {
                m_failed = true;
                m_error  = _error;
            }

            return;
        }

        void
        push(
            expr_program_code    _code,
            expr_op_kind         _op,
            std::size_t          _index,
            std::size_t          _arity,
            const expr_function* _function
        )
        {
            const expr_program_step step = { _code, _op, _index, _arity,
                                             _function };

            if (!m_failed)
            {
                m_program.m_steps.push_back(step);
            }

            return;
        }

        void
        push_constant(
            const carrier& _value
        )
        {
            push(expr_program_code::constant, expr_op_kind::pos,
                 m_program.m_constants.size(), 0, D_NULLPTR);

            if (!m_failed)
            {
                m_program.m_constants.push_back(_value);
            }

            return;
        }

        // push_variable
        //   a variable's slot, the first step that reads it noted; a name
        // without a slot unbound, as the fold's is without a binding.
        void
        push_variable(
            const std::string& _name
        )
        {
            const std::vector<std::string>& names = m_program.m_variables;

            for (std::size_t i = 0; i < names.size(); ++i)
            {
                if (names[i] == _name)
                {
                    if (m_program.m_first[i] == npos())
                    {
                        m_program.m_first[i] = m_program.m_steps.size();
                    }

                    push(expr_program_code::variable, expr_op_kind::pos, i,
                         0, D_NULLPTR);

                    return;
                }
            }

            fail(expr_unbound(_name));

            return;
        }

        // write_operation
        //   the fold's step of _op of _n operands, as
        // expr_typed_application decides it: a literal bad_shape; indexing
        // a NaN; a call its function's evaluator; an operator of the
        // table's; any other the fold's error.
        void
        write_operation(
            const expr_op& _op,
            std::size_t    _n
        )
        {
            // a literal: no value in a type
            if (expr_is_literal(_op.kind))
            {
                fail(expr_error(expr_status::bad_shape,
                                expr_no_offset,
                                "expr_evaluate<Type>: a vector or a matrix "
                                "has no value in a type"));

                return;
            }

            // indexing: in a type no operand is a matrix, so a NaN
            if (_op.kind == expr_op_kind::index)
            {
                if (!expr_index_takes(_n))
                {
                    fail(expr_index_error(_n));

                    return;
                }

                push(expr_program_code::nan, _op.kind, 0, _n, D_NULLPTR);

                return;
            }

            // a call: its function's evaluator
            if (_op.kind == expr_op_kind::call)
            {
                const expr_result<const expr_function*> function =
                    expr_function_of(_op, _n);

                if (function.is_err())
                {
                    fail(function.error());

                    return;
                }

                push(expr_program_code::call, _op.kind, 0, _n,
                     function.value());

                return;
            }

            // an operator of the table's, on the operands it takes
            if (expr_takes(_op.kind, _n))
            {
                push(expr_program_code::apply, _op.kind, 0, _n, D_NULLPTR);

                return;
            }

            fail(expr_fold_error(_op, _n));

            return;
        }

        expr_program<Type> m_program;
        bool               m_failed;
        expr_error         m_error;
    };

NS_END  // internal

// 2.3    expr_compile
//------------------------------------------------------------------------------
// expr_compile
/**
 * @brief A term compiled for Type, float, double or long double or the
 *        math::complex of one, to be run many times (S395).
 *
 * @param[in] _term       the term.
 * @param[in] _variables  the variables' names, in the order a run takes
 *                        their values; by default the term's own, in the
 *                        order the fold first reads them (expr_variables).
 * @return the program, whose runs give what expr_evaluate<Type> gives, bit
 *         for bit; or the error expr_evaluate<Type> would give with a
 *         binding for each name of _variables: the first, in the fold's
 *         order, of unbound_variable for a variable not among them,
 *         unknown_name, wrong_arity and bad_shape; or missing_operand for a
 *         term made by default.
 */
template<typename Type>
D_NODISCARD expr_result<expr_program<Type> >
expr_compile(
    const expr_term&                _term,
    const std::vector<std::string>& _variables
)
{
    D_STATIC_ASSERT(internal::expr_typed<Type>::is_type,
                    "expr_compile<Type>: Type is float, double or long "
                    "double, or the math::complex of one.");

    // no term: a term made by default
    if (_term.empty())
    {
        return err<expr_program<Type>, expr_error>(internal::expr_no_term());
    }

    internal::expr_program_builder<Type> builder(_variables);

    const std::size_t depth =
        ::djinterp::evaluate<std::size_t, expr_op, expr_atom>(
            _term,
            // a leaf: its value, or its slot
            [&builder](const expr_atom& _atom) -> std::size_t
            {
                return builder.leaf(_atom);
            },
            // an application: its step, after its operands'
            [&builder](const expr_op&                  _op,
                       const std::vector<std::size_t>& _depths)
                -> std::size_t
            {
                return builder.application(_op, _depths);
            });

    return builder.made(depth);
}

template<typename Type>
D_NODISCARD expr_result<expr_program<Type> >
expr_compile(
    const expr_term& _term
)
{
    return expr_compile<Type>(_term, expr_variables(_term));
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_PROGRAM_HPP
