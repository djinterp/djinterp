/*******************************************************************************
* djinterp [cpp/parsegen]                                                peg.cpp
*
* Definitions for `peg.hpp`: the operators, the driver, and the profile and
* the memo a run can keep.
*   The family's state is the run's, reached through machine::ext; the
* operators read and write it, and nothing else does. A profile is counted
* in the driver before each dispatch, as the trace is written, and where the
* call stack changes -- CALL and MEMO, RETURN, and a backtrack -- so each
* call's steps are known when it closes. The memo is the state's too: a MEMO
* that enters its rule leaves a pending result at its call's depth, a RETURN
* at that depth keeps the success, and a backtrack below it the failure.
*
*
* path:      /src/djinterp/cpp/modules/parsegen/vparse/peg.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.17
*******************************************************************************/
// djinterp
#include "djinterp/cpp/modules/parsegen/vparse/peg.hpp"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <algorithm>      // std::binary_search, std::max, std::min,
                          // std::sort, std::unique
#include <cstddef>        // std::ptrdiff_t, std::size_t
#include <string>         // std::string, std::to_string
#include <vector>         // std::vector
// re_std
#include "re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint64_t

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace peg {

namespace {

// counter
//   struct: a profile while it is counted: the profile, whose code each
// address is, which cells and which calls the run has come to -- a bit for
// each position, in each rule, made when the rule first needs them -- and
// the calls open now, one for each return address on the call stack.
struct counter
{
    // DOpenCall
    //   struct: a call on the stack: its rule (-1 for none), the steps run
    // when it was made, and whether the rule was called at its position
    // before.
    struct DOpenCall
    {
        int         rule;
        std::size_t made;
        bool        repeat;
    };

    profile*                                   out       = nullptr;
    std::vector<int>                           owner;
    std::size_t                                positions = 0u;
    std::vector<std::vector<re_std::uint64_t>> stepped;
    std::vector<std::vector<re_std::uint64_t>> called;
    std::vector<std::size_t>                   touched_by;
    std::vector<std::size_t>                   arrivals_at;
    std::vector<std::size_t>                   touched_at;
    std::vector<DOpenCall>                     open;
    int                                        last_rule = -1;
    int                                        last_off  = -1;
};

// UNKNOWN
//   constant: the end of a result the memo does not keep.
const int UNKNOWN = -2;

// FAILED
//   constant: the end of a result that is the rule's failure.
const int FAILED = -1;

// MEMO_PAGE
//   constant: how many positions one page of a rule's results covers.
const std::size_t MEMO_PAGE = 256u;

// kept
//   struct: a rule's result at a position, as the memo keeps it: where it
// ended -- FAILED where it failed, UNKNOWN where nothing is kept -- and the
// captures it made, as a range of the memo's pool.
struct kept
{
    int         end;
    std::size_t from;
    std::size_t count;
};

// DMemoPage
//   type: MEMO_PAGE results of one rule, at consecutive positions.
using DMemoPage = std::vector<kept>;

// DMemoTable
//   struct: the memo's results, by rule and position: for each address a
// MEMO has called, a page for each MEMO_PAGE positions of the input, made
// when the first result in it is kept. A parse moves through the input, so
// the results it looks up are near the last it kept; and a rule called in
// one part of the input costs pages there alone.
struct DMemoTable
{
    std::vector<std::vector<DMemoPage>> pages;
    std::size_t                         used = 0u;
};

// pending
//   struct: a MEMO whose rule is running: the rule and the position its
// result is kept at, and the call and capture depths the MEMO was made at.
struct pending
{
    int         start;
    int         off;
    std::size_t depth;
    std::size_t capdepth;
};

// state
//   struct: this family's per-run state, reached through machine::ext.  The
// cursor (data / len / off) is seeded from the parse_state; the rest is the
// PEG machinery -- call/return stack, backtrack stack, and the mark/capture
// stacks -- with the memo, and the profile when one is counted.  TU-local.
struct state
{
    // bt
    //   struct: a backtrack point -- resume ip, cursor, and the call / mark /
    // capture depths to unwind to.
    struct bt
    {
        int         ip;
        int         off;
        std::size_t cdepth;
        std::size_t mdepth;
        std::size_t capdepth;
    };

    const program*       prog = nullptr;
    const char*          data = nullptr;   // the input, from the parse_state
    int                  len  = 0;
    int                  off  = 0;         // the cursor
    int                  ip   = 0;
    std::vector<int>     calls;
    std::vector<bt>      back;
    std::vector<int>     marks;
    std::vector<capture> caps;
    const DInstr*         cur  = nullptr;

    // the memo: results by rule and position, the captures they made, and
    // the MEMOs whose rules are running
    DMemoTable           memo;
    std::vector<capture> memo_caps;
    std::vector<pending> memo_open;
    std::size_t          memo_hits   = 0u;
    std::size_t          memo_misses = 0u;
    std::size_t          positions   = 0u;

    // the profile being counted, if any
    counter*             count       = nullptr;
};

// self
//   function: the family state of the run on a machine.
inline
state& self(
    machine& _m
)
{
    return *static_cast<state*>(_m.ext);
}

/**
 * @brief Gives the result the memo keeps for a rule at a position.
 *
 * @param[in] _table  the memo.
 * @param[in] _start  the rule's first address.
 * @param[in] _off    the position.
 * @return the result, or `nullptr` where none is kept.
 */
const kept* find_kept(
    const DMemoTable& _table,
    int               _start,
    int               _off
)
{
    const std::size_t rule = static_cast<std::size_t>(_start);
    const std::size_t page = static_cast<std::size_t>(_off) / MEMO_PAGE;

    // a rule with no page here keeps nothing here
    if ( (rule >= _table.pages.size())          ||
         (page >= _table.pages[rule].size())    ||
         (_table.pages[rule][page].empty()) )
    {
        return nullptr;
    }

    const kept& found =
        _table.pages[rule][page][static_cast<std::size_t>(_off) % MEMO_PAGE];

    return (found.end == UNKNOWN) ? nullptr
                                  : &found;
}

/**
 * @brief Keeps a rule's result at a position.
 *
 * @param[in,out] _table      the memo.
 * @param[in]     _start      the rule's first address.
 * @param[in]     _off        the position; below `_positions`.
 * @param[in]     _value      the result.
 * @param[in]     _positions  how many positions the input has.
 * @post   The rule has a page at the position, made if it had none.
 */
void put_kept(
    DMemoTable& _table,
    int         _start,
    int         _off,
    const kept& _value,
    std::size_t _positions
)
{
    const std::size_t rule = static_cast<std::size_t>(_start);
    const std::size_t page = static_cast<std::size_t>(_off) / MEMO_PAGE;

    // a rule the memo has kept nothing for: a row of pages, none made yet
    if (rule >= _table.pages.size())
    {
        _table.pages.resize(rule + 1u);
    }

    std::vector<DMemoPage>& row = _table.pages[rule];

    // the row covers every position
    if (row.empty())
    {
        row.resize((_positions + MEMO_PAGE - 1u) / MEMO_PAGE);
    }

    // the page's first result makes it
    if (row[page].empty())
    {
        row[page].assign(MEMO_PAGE,
                         kept{ UNKNOWN, 0u, 0u });
    }

    kept& slot = row[page][static_cast<std::size_t>(_off) % MEMO_PAGE];

    // a position kept for the first time
    if (slot.end == UNKNOWN)
    {
        _table.used++;
    }

    slot = _value;

    return;
}

/**
 * @brief Sets a position's bit in a rule's bits, and says whether it was set
 *        already.
 *
 * @param[in,out] _bits       the rule's bits; empty until the first is set,
 *                            when they are made for every position.
 * @param[in]     _at         the position; less than `_positions`.
 * @param[in]     _positions  how many positions there are.
 * @return whether the bit was set before.
 */
bool test_and_set(
    std::vector<re_std::uint64_t>& _bits,
    std::size_t                    _at,
    std::size_t                    _positions
)
{
    // a rule's first bit makes room for every position
    if (_bits.empty())
    {
        _bits.assign((_positions + 63u) / 64u,
                     0u);
    }

    const re_std::uint64_t bit  = static_cast<re_std::uint64_t>(1u)
                                      << (_at % 64u);
    re_std::uint64_t&      word = _bits[_at / 64u];
    const bool             was  = ((word & bit) != 0u);

    word |= bit;

    return was;
}

// rule_at
//   function: the rule whose code an address is, or -1 for none: the gate's,
// or an address outside the program.
int rule_at(
    const counter& _counter,
    int            _address
)
{
    const bool inside = ( (_address >= 0) &&
                          (static_cast<std::size_t>(_address) <
                           _counter.owner.size()) );

    return inside ? _counter.owner[static_cast<std::size_t>(_address)]
                  : -1;
}

/**
 * @brief Counts a call of a rule, and opens it where the rule runs.
 *
 * @param[in,out] _counter   the profile being counted.
 * @param[in]     _start     the address called.
 * @param[in]     _off       the position it is called at.
 * @param[in]     _answered  whether the memo answered the call, so that the
 *                           rule does not run.
 */
void count_call(
    counter& _counter,
    int      _start,
    int      _off,
    bool     _answered
)
{
    const int rule   = rule_at(_counter,
                               _start);
    bool      repeat = false;

    // a rule's call, and whether one came to this position before
    if (rule >= 0)
    {
        const std::size_t     index  = static_cast<std::size_t>(rule);
        profile::DRuleCounts& counts = _counter.out->rules[index];

        repeat = test_and_set(_counter.called[index],
                              static_cast<std::size_t>(_off),
                              _counter.positions);
        counts.calls++;

        // a call at a position the rule was called at before
        if (repeat)
        {
            counts.repeats++;
        }

        // the memo's answer, which runs nothing
        if (_answered)
        {
            counts.hits++;
        }
    }

    // a call the rule runs is open until it returns or is unwound
    if (!_answered)
    {
        _counter.open.push_back(counter::DOpenCall{ rule,
                                                    _counter.out->steps,
                                                    repeat });
    }

    return;
}

/**
 * @brief Closes the calls above a depth, adding the steps each repeat ran
 *        to its rule's.
 *
 * @param[in,out] _counter  the profile being counted.
 * @param[in]     _depth    the call stack's depth after: the calls above it
 *                          returned, were unwound, or saw the run end.
 */
void close_calls(
    counter&    _counter,
    std::size_t _depth
)
{
    // the innermost first
    while (_counter.open.size() > _depth)
    {
        const counter::DOpenCall done = _counter.open.back();

        _counter.open.pop_back();

        // the steps a repeat ran are the steps a memo would have saved
        if ( (done.repeat) &&
             (done.rule >= 0) )
        {
            _counter.out->rules[static_cast<std::size_t>(done.rule)]
                .repeat_steps += _counter.out->steps - done.made;
        }
    }

    return;
}

/**
 * @brief Keeps the result of the MEMO whose rule is returning: where it
 *        ended, and the captures it made.
 *
 * @param[in,out] _s  the run's state; its innermost pending MEMO is the one
 *                    whose rule is returning.
 * @post   The memo holds the result, its captures copied to the pool, and
 *         the MEMO is no longer pending.
 */
void keep_success(
    state& _s
)
{
    const pending       done  = _s.memo_open.back();
    const std::ptrdiff_t first = static_cast<std::ptrdiff_t>(done.capdepth);
    kept                result;

    _s.memo_open.pop_back();

    result.end   = _s.off;
    result.from  = _s.memo_caps.size();
    result.count = _s.caps.size() - done.capdepth;

    // the captures the rule made, if it made any
    if (result.count != 0u)
    {
        _s.memo_caps.insert(_s.memo_caps.end(),
                            _s.caps.begin() + first,
                            _s.caps.end());
    }

    put_kept(_s.memo,
             done.start,
             done.off,
             result,
             _s.positions);

    return;
}

/**
 * @brief Keeps the failure of the innermost pending MEMO's rule, which a
 *        backtrack is unwinding.
 *
 * @param[in,out] _s  the run's state; it has a pending MEMO.
 * @post   The memo holds the failure, and the MEMO is no longer pending.
 */
void keep_failure(
    state& _s
)
{
    const pending done = _s.memo_open.back();
    kept          result;

    _s.memo_open.pop_back();

    result.end   = FAILED;
    result.from  = 0u;
    result.count = 0u;

    put_kept(_s.memo,
             done.start,
             done.off,
             result,
             _s.positions);

    return;
}

// raise
//   function: a peak raised to a size it had not reached.
inline
void raise(
    std::size_t& _peak,
    std::size_t  _size
)
{
    // a size beyond the peak is the new peak
    if (_size > _peak)
    {
        _peak = _size;
    }

    return;
}

// fail
//   the PEG failure convention -- backtrack (restoring cursor, call depth, and
// the mark/capture stacks), or halt unsuccessfully. A backtrack that unwinds
// the call a MEMO made is that rule failing where the MEMO was made, and is
// kept; one that unwinds a counted call closes it.
inline
void fail(
    machine& _m
)
{
    state& s = self(_m);

    // nowhere to go back to: the run fails
    if (s.back.empty())
    {
        _m.ok     = false;
        _m.halted = true;

        return;
    }

    const state::bt b = s.back.back();

    s.back.pop_back();

    // every MEMO whose call this unwinds failed where it was made
    while ( (!s.memo_open.empty()) &&
            (s.memo_open.back().depth >= b.cdepth) )
    {
        keep_failure(s);
    }

    // and every counted call it unwinds closes
    if (s.count != nullptr)
    {
        close_calls(*s.count,
                    b.cdepth);
    }

    s.ip  = b.ip;
    s.off = b.off;
    s.calls.resize(b.cdepth);
    s.marks.resize(b.mdepth);
    s.caps.resize(b.capdepth);

    return;
}

// op_char
//   voperator: match the current literal char; advance or fail.
void op_char(
    machine& _m
)
{
    state& s = self(_m);

    // the literal is next
    if ( (s.off < s.len) &&
         (s.data[s.off] == s.cur->ch) )
    {
        s.off += 1;
        s.ip  += 1;

        return;
    }

    fail(_m);

    return;
}

// op_any
//   voperator: consume any one char; fail only at end of input.
void op_any(
    machine& _m
)
{
    state& s = self(_m);

    // anything is next
    if (s.off < s.len)
    {
        s.off += 1;
        s.ip  += 1;

        return;
    }

    fail(_m);

    return;
}

// op_choice
//   voperator: push a backtrack point (cursor + depths) and fall through.
void op_choice(
    machine& _m
)
{
    state& s = self(_m);

    s.back.push_back(state::bt{ s.cur->arg,
                                s.off,
                                s.calls.size(),
                                s.marks.size(),
                                s.caps.size() });
    s.ip += 1;

    // a counted run's deepest backtrack stack
    if (s.count != nullptr)
    {
        raise(s.count->out->peak_back,
              s.back.size());
    }

    return;
}

// op_jump
//   voperator: unconditional transfer of control.
void op_jump(
    machine& _m
)
{
    state& s = self(_m);

    s.ip = s.cur->arg;

    return;
}

// op_call
//   voperator: push the return address and enter a sub-rule.
void op_call(
    machine& _m
)
{
    state& s = self(_m);

    s.calls.push_back(s.ip + 1);

    // a counted call is open until it returns or is unwound
    if (s.count != nullptr)
    {
        count_call(*s.count,
                   s.cur->arg,
                   s.off,
                   false);
        raise(s.count->out->peak_calls,
              s.calls.size());
    }

    s.ip = s.cur->arg;

    return;
}

// op_return
//   voperator: pop the return address and resume the caller. The return of
// the call a MEMO made keeps its rule's success.
void op_return(
    machine& _m
)
{
    state& s = self(_m);

    // the call a MEMO made, returning
    if ( (!s.memo_open.empty()) &&
         ((s.memo_open.back().depth + 1u) == s.calls.size()) )
    {
        keep_success(s);
    }

    // a counted call closes
    if (s.count != nullptr)
    {
        close_calls(*s.count,
                    s.calls.size() - 1u);
    }

    s.ip = s.calls.back();
    s.calls.pop_back();

    return;
}

// op_commit
//   voperator: discard the most recent backtrack point and jump.
void op_commit(
    machine& _m
)
{
    state& s = self(_m);

    s.back.pop_back();
    s.ip = s.cur->arg;

    return;
}

// op_fail
//   voperator: force the failure convention.
void op_fail(
    machine& _m
)
{
    fail(_m);

    return;
}

// op_match
//   voperator: accept -- halt the machine successfully.
void op_match(
    machine& _m
)
{
    _m.ok     = true;
    _m.halted = true;

    return;
}

// op_set_class
//   voperator: match the current char against the class set; advance or fail.
void op_set_class(
    machine& _m
)
{
    state& s = self(_m);

    // a member of the class is next
    if ( (s.off < s.len) &&
         (s.cur->set.find(s.data[s.off]) != std::string::npos) )
    {
        s.off += 1;
        s.ip  += 1;

        return;
    }

    fail(_m);

    return;
}

// op_mark
//   voperator: record the cursor as the start of a capture.
void op_mark(
    machine& _m
)
{
    state& s = self(_m);

    s.marks.push_back(s.off);
    s.ip += 1;

    // a counted run's deepest mark stack
    if (s.count != nullptr)
    {
        raise(s.count->out->peak_marks,
              s.marks.size());
    }

    return;
}

// op_cap
//   voperator: pop the mark and record a tagged span [mark, cursor).
void op_cap(
    machine& _m
)
{
    state&    s     = self(_m);
    const int start = s.marks.back();

    s.marks.pop_back();
    s.caps.push_back(capture{ s.cur->arg,
                              start,
                              s.off });
    s.ip += 1;

    // a counted run's most captures
    if (s.count != nullptr)
    {
        raise(s.count->out->peak_caps,
              s.caps.size());
    }

    return;
}

// op_memo
//   voperator: a CALL through the memo. A result kept for the rule at this
// position is replayed -- its captures appended and the cursor moved to its
// end, or its failure taken -- and the rule does not run; otherwise the rule
// is entered as CALL enters it, its result pending until it returns or a
// backtrack unwinds it.
void op_memo(
    machine& _m
)
{
    state&            s     = self(_m);
    const kept* const found = find_kept(s.memo,
                                        s.cur->arg,
                                        s.off);

    // kept before: replayed, and the rule not run
    if (found != nullptr)
    {
        // a copy: a failure below keeps others, and may move this one
        const kept result = *found;

        s.memo_hits++;

        // a counted call, the memo's answer
        if (s.count != nullptr)
        {
            count_call(*s.count,
                       s.cur->arg,
                       s.off,
                       true);
        }

        // the rule failed here
        if (result.end == FAILED)
        {
            fail(_m);

            return;
        }

        const std::ptrdiff_t first = static_cast<std::ptrdiff_t>(result.from);
        const std::ptrdiff_t past  = first +
                                     static_cast<std::ptrdiff_t>(result.count);

        // the captures the rule made, if it made any
        if (result.count != 0u)
        {
            s.caps.insert(s.caps.end(),
                          s.memo_caps.begin() + first,
                          s.memo_caps.begin() + past);

            // a counted run's most captures
            if (s.count != nullptr)
            {
                raise(s.count->out->peak_caps,
                      s.caps.size());
            }
        }

        s.off = result.end;
        s.ip += 1;

        return;
    }

    s.memo_misses++;
    s.memo_open.push_back(pending{ s.cur->arg,
                                   s.off,
                                   s.calls.size(),
                                   s.caps.size() });

    s.calls.push_back(s.ip + 1);

    // a counted call is open until it returns or is unwound
    if (s.count != nullptr)
    {
        count_call(*s.count,
                   s.cur->arg,
                   s.off,
                   false);
        raise(s.count->out->peak_calls,
              s.calls.size());
    }

    s.ip = s.cur->arg;

    return;
}

// op_name
//   display mnemonic for an opcode.
const char* op_name(
    int _op
)
{
    switch (_op)
    {
        case CHAR:   return "CHAR";
        case ANY:    return "ANY";
        case CHOICE: return "CHOICE";
        case JUMP:   return "JUMP";
        case CALL:   return "CALL";
        case RETURN: return "RETURN";
        case COMMIT: return "COMMIT";
        case FAIL:   return "FAIL";
        case MATCH:  return "MATCH";
        case SET:    return "SET";
        case MARK:   return "MARK";
        case CAP:    return "CAP";
        case MEMO:   return "MEMO";
        default:     break;
    }

    return "?";
}

// has_addr
//   whether an opcode carries a jump/address operand.
bool has_addr(
    int _op
)
{
    return ( (_op == CHOICE) ||
             (_op == JUMP)   ||
             (_op == CALL)   ||
             (_op == COMMIT) ||
             (_op == MEMO) );
}

// describe
//   one trace line for the instruction about to run (format in peg.hpp).
std::string describe(
    const state&  _s,
    const DInstr& _ins
)
{
    return "ip="     + std::to_string(_s.ip)           +
           " off="   + std::to_string(_s.off)          +
           " calls=" + std::to_string(_s.calls.size()) +
           " back="  + std::to_string(_s.back.size())  +
           " marks=" + std::to_string(_s.marks.size()) +
           " caps="  + std::to_string(_s.caps.size())  +
           " | "     + fmt(_ins);
}

/**
 * @brief Makes ready to count a run's profile.
 *
 * @param[out] _counter    the counter, made for this run.
 * @param[out] _out        the profile, emptied, with a rule for each of the
 *                         program's.
 * @param[in]  _prog       the program.
 * @param[in]  _positions  how many positions the input has: its length and
 *                         one.
 */
void start_counting(
    counter&       _counter,
    profile&       _out,
    const program& _prog,
    std::size_t    _positions
)
{
    const layout      where = layout_of(_prog);
    const std::size_t rules = where.starts.size();

    _out = profile();
    _out.rules.resize(rules);
    _out.revisits_at.assign(_positions,
                            0u);

    // each rule named by its first address
    for (std::size_t index = 0u; index < rules; index++)
    {
        _out.rules[index].start = where.starts[index];
    }

    _counter.out       = &_out;
    _counter.owner     = where.owner;
    _counter.positions = _positions;
    _counter.stepped.assign(rules,
                            std::vector<re_std::uint64_t>());
    _counter.called.assign(rules,
                           std::vector<re_std::uint64_t>());
    _counter.touched_by.assign(rules,
                               0u);
    _counter.arrivals_at.assign(_positions,
                                0u);
    _counter.touched_at.assign(_positions,
                               0u);

    return;
}

/**
 * @brief Counts the step about to run: the run's, its rule's, and an
 *        arrival where its cell is not the last step's.
 *
 * @param[in,out] _counter  the profile being counted.
 * @param[in]     _s        the run's state; its ip and cursor are the
 *                          step's.
 */
void count_step(
    counter&     _counter,
    const state& _s
)
{
    profile&  out  = *_counter.out;
    const int rule = rule_at(_counter,
                             _s.ip);

    out.steps++;

    // the gate's steps are no rule's, and end any stay in a cell
    if (rule < 0)
    {
        out.gate_steps++;
        _counter.last_rule = -1;

        return;
    }

    const std::size_t index = static_cast<std::size_t>(rule);
    const std::size_t at    = static_cast<std::size_t>(_s.off);

    out.rules[index].steps++;

    // a cell other than the last step's is an arrival
    if ( (rule != _counter.last_rule) ||
         (_s.off != _counter.last_off) )
    {
        out.rules[index].arrivals++;
        _counter.arrivals_at[at]++;

        // the first arrival at a cell touches it
        if (!test_and_set(_counter.stepped[index],
                          at,
                          _counter.positions))
        {
            _counter.touched_by[index]++;
            _counter.touched_at[at]++;
        }
    }

    _counter.last_rule = rule;
    _counter.last_off  = _s.off;

    return;
}

/**
 * @brief Ends a run's counts: the calls still open close, and the revisits
 *        and the memo's counts are filled in.
 *
 * @param[in,out] _counter  the profile being counted.
 * @param[in]     _s        the run's state, as the run left it.
 */
void finish_counting(
    counter&     _counter,
    const state& _s
)
{
    profile& out = *_counter.out;

    // a call still open when the run stopped ran to its end
    close_calls(_counter,
                0u);

    // a rule's revisits are its arrivals at cells it had arrived at before
    for (std::size_t index = 0u; index < out.rules.size(); index++)
    {
        out.rules[index].revisits = out.rules[index].arrivals -
                                    _counter.touched_by[index];
        out.touched              += _counter.touched_by[index];
        out.revisits             += out.rules[index].revisits;
    }

    // and a position's, its arrivals at cells arrived at before
    for (std::size_t at = 0u; at < _counter.positions; at++)
    {
        out.revisits_at[at] = _counter.arrivals_at[at] -
                              _counter.touched_at[at];
    }

    out.memo_entries = _s.memo.used;
    out.memo_hits    = _s.memo_hits;
    out.memo_misses  = _s.memo_misses;

    return;
}

}  // anonymous namespace

/*
fmt
  One mnemonic, and the operand its kind carries: a literal quoted, a class
bracketed, a capture's tag after `#`, an address bare.
*/
std::string fmt(
    const DInstr& _ins
)
{
    std::string s = op_name(_ins.op);

    // the operand, as each kind carries it
    if (_ins.op == CHAR)
    {
        s += " '";
        s += _ins.ch;
        s += "'";
    }
    else if (_ins.op == SET)
    {
        s += " [" + _ins.set + "]";
    }
    else if (_ins.op == CAP)
    {
        s += " #" + std::to_string(_ins.arg);
    }
    else if (has_addr(_ins.op))
    {
        s += " " + std::to_string(_ins.arg);
    }

    return s;
}

/*
make_ops
  Every opcode of the family, MEMO among them, registered here and nowhere
else (invariant 1).
*/
op_set make_ops()
{
    op_set ops;

    ops.def(CHAR,
            "CHAR",
            &op_char);
    ops.def(ANY,
            "ANY",
            &op_any);
    ops.def(CHOICE,
            "CHOICE",
            &op_choice);
    ops.def(JUMP,
            "JUMP",
            &op_jump);
    ops.def(CALL,
            "CALL",
            &op_call);
    ops.def(RETURN,
            "RETURN",
            &op_return);
    ops.def(COMMIT,
            "COMMIT",
            &op_commit);
    ops.def(FAIL,
            "FAIL",
            &op_fail);
    ops.def(MATCH,
            "MATCH",
            &op_match);
    ops.def(SET,
            "SET",
            &op_set_class);
    ops.def(MARK,
            "MARK",
            &op_mark);
    ops.def(CAP,
            "CAP",
            &op_cap);
    ops.def(MEMO,
            "MEMO",
            &op_memo);

    return ops;
}

/*
run
  The rebased driver on a default machine.  Parses a parse_state<char> from
its offset; on success advances offset to the match end and hands back
captures, on failure leaves offset unchanged (match-or-restore).
*/
bool run(
    parse_state<char>&    _state,
    const program&        _prog,
    const op_set&         _ops,
    std::vector<capture>* _caps
)
{
    machine m;

    return run(m,
               _state,
               _prog,
               _ops,
               _caps);
}

/*
run (caller's machine)
  The driver with a capture sink and no profile.
*/
bool run(
    machine&              _machine,
    parse_state<char>&    _state,
    const program&        _prog,
    const op_set&         _ops,
    std::vector<capture>* _caps
)
{
    sinks out;

    out.captures = _caps;

    return run(_machine,
               _state,
               _prog,
               _ops,
               out);
}

/*
run (sinks)
  The driver loop. The machine is the caller's, so its step limit and trace
sink apply; the family state is local and is detached from the machine again
before returning. A profile is counted around each dispatch, as the trace is
written: the step before the operator runs, at the cursor it runs at, and
the stacks' sizes after.
*/
bool run(
    machine&           _machine,
    parse_state<char>& _state,
    const program&     _prog,
    const op_set&      _ops,
    const sinks&       _sinks
)
{
    state   s;
    counter counted;

    s.prog      = &_prog;
    s.data      = _state.data;
    s.len       = static_cast<int>(_state.length);
    s.off       = static_cast<int>(_state.offset);
    s.positions = std::max(_state.length,
                           _state.offset) + 1u;

    // a profile counts from the first step, at every position the cursor
    // can stand at
    if (_sinks.counts != nullptr)
    {
        start_counting(counted,
                       *_sinks.counts,
                       _prog,
                       s.positions);
        s.count = &counted;
    }

    _machine.ext    = &s;
    _machine.steps  = 0;
    _machine.halted = false;
    _machine.ok     = false;
    _machine.error.clear();

    while (true)
    {
        // the step limit stops the machine, the run unfinished
        if (_machine.steps >= _machine.step_limit)
        {
            _machine.ok    = false;
            _machine.error = "step limit";

            break;
        }

        // an ip with no instruction -- the first of an empty program, which
        // ebnf::compile returns for a grammar it cannot parse -- is an
        // ordinary failure, not a read past the end of the program
        if ( (s.ip < 0) ||
             (static_cast<std::size_t>(s.ip) >= _prog.size()) )
        {
            _machine.ok    = false;
            _machine.error = _prog.empty() ? "empty program"
                                           : "ip out of range";

            break;
        }

        const DInstr& ins = _prog[static_cast<std::size_t>(s.ip)];

        s.cur = &ins;

        // a line a dispatch, before the operator runs
        if (_machine.trace != nullptr)
        {
            _machine.trace->push_back(describe(s,
                                               ins));
        }

        // and a count, as the line would have it
        if (s.count != nullptr)
        {
            count_step(*s.count,
                       s);
        }

        const DVoperator* h = _ops.find(ins.op);

        // an opcode the operators do not have stops the machine
        if (h == nullptr)
        {
            _machine.halted = true;
            _machine.error  = "no operator for opcode " +
                              std::to_string(ins.op);

            break;
        }

        (*h)(_machine);

        // a MATCH, or a failure with nowhere to go back to
        if (_machine.halted)
        {
            break;
        }

        _machine.steps += 1;
    }

    _machine.ext = nullptr;

    // the profile is the run's, whatever its outcome
    if (s.count != nullptr)
    {
        finish_counting(*s.count,
                        s);
    }

    // match-or-restore: advance + publish captures on success, else no change
    if (_machine.ok)
    {
        _state.offset = static_cast<std::size_t>(s.off);

        // the captures, where they were asked for
        if (_sinks.captures != nullptr)
        {
            *_sinks.captures = s.caps;
        }
    }

    return _machine.ok;
}

/*
layout_of
  The same reading of a program the workbench labels its rules by: after the
gate, a rule ends at each RETURN. A program it does not fit -- one whose call
lands inside such a span -- is read by where its calls land instead, which is
every rule a run can enter.
*/
layout layout_of(
    const program& _prog
)
{
    layout      out;
    std::size_t at = 0u;

    // the gate ends one past the first MATCH
    while ( (at < _prog.size()) &&
            (_prog[at].op != MATCH) )
    {
        at++;
    }

    const std::size_t gate = at + 1u;

    // a rule a RETURN, each starting where the one before ended
    for (at = gate; at < _prog.size(); )
    {
        out.starts.push_back(static_cast<int>(at));

        // to the rule's RETURN
        while ( (at < _prog.size()) &&
                (_prog[at].op != RETURN) )
        {
            at++;
        }

        at++;
    }

    std::vector<int> targets;

    out.regular = true;

    // every call must land on a rule's start
    for (const DInstr& each : _prog)
    {
        // a call, and where it lands
        if ( (each.op == CALL) ||
             (each.op == MEMO) )
        {
            targets.push_back(each.arg);

            // inside a rule, or outside them all
            if (!std::binary_search(out.starts.begin(),
                                    out.starts.end(),
                                    each.arg))
            {
                out.regular = false;
            }
        }
    }

    // a program that is not laid out so has its rules where its calls land
    if (!out.regular)
    {
        std::sort(targets.begin(),
                  targets.end());
        targets.erase(std::unique(targets.begin(),
                                  targets.end()),
                      targets.end());
        out.starts.clear();

        // each place a call lands
        for (const int target : targets)
        {
            // a call into the gate, or past the program, starts no rule
            if ( (target >= static_cast<int>(gate)) &&
                 (static_cast<std::size_t>(target) < _prog.size()) )
            {
                out.starts.push_back(target);
            }
        }
    }

    std::size_t next = 0u;

    out.owner.assign(_prog.size(),
                     -1);

    // each address is the last rule to start at or before it
    for (at = 0u; at < _prog.size(); at++)
    {
        // the rules that have started by this address
        while ( (next < out.starts.size()) &&
                (static_cast<std::size_t>(out.starts[next]) <= at) )
        {
            next++;
        }

        out.owner[at] = static_cast<int>(next) - 1;
    }

    return out;
}

/*
memoized
  An opcode for an opcode: MEMO takes CALL's operand and its place, so the
rewrite moves no address and needs no relocation.
*/
program memoized(
    const program&          _prog,
    const std::vector<int>& _starts
)
{
    std::vector<int> chosen = _starts;
    program          out    = _prog;

    std::sort(chosen.begin(),
              chosen.end());

    // every instruction, for the calls among them
    for (DInstr& each : out)
    {
        // a call to a chosen rule goes through the memo
        if ( (each.op == CALL) &&
             (std::binary_search(chosen.begin(),
                                 chosen.end(),
                                 each.arg)) )
        {
            each.op = MEMO;
        }
    }

    return out;
}

/*
worth_memoizing
  A rule's repeated calls ran steps a memo would not have run; every call of
it, repeated or not, pays to keep its result or look it up. The rule is worth
it when the first is the greater. Each rule is judged by its own repeats, so
a rule whose repeats run inside another chosen rule's may be chosen with it;
the memo answering the outer leaves the inner fewer calls, never more.
*/
std::vector<int> worth_memoizing(
    const profile& _profile
)
{
    std::vector<int> out;

    // a rule at a time, in layout order
    for (const profile::DRuleCounts& each : _profile.rules)
    {
        // repeats that ran more than the memo would cost every call
        if ( (each.repeats > 0u) &&
             (each.repeat_steps > (MEMO_COST * each.calls)) )
        {
            out.push_back(each.start);
        }
    }

    return out;
}

/*
accumulate
  Counts add; peaks do not, since two runs' stacks are never on the machine
at once. The memo's size is a peak in the same way: each run has its own.
*/
void accumulate(
    profile&       _total,
    const profile& _run
)
{
    // an empty total takes the run's rules
    if (_total.rules.empty())
    {
        _total.rules.resize(_run.rules.size());

        // each named by its first address
        for (std::size_t index = 0u; index < _run.rules.size(); index++)
        {
            _total.rules[index].start = _run.rules[index].start;
        }
    }

    const std::size_t rules = std::min(_total.rules.size(),
                                       _run.rules.size());

    // rule by rule
    for (std::size_t index = 0u; index < rules; index++)
    {
        profile::DRuleCounts&       into = _total.rules[index];
        const profile::DRuleCounts& from = _run.rules[index];

        into.steps        += from.steps;
        into.arrivals     += from.arrivals;
        into.revisits     += from.revisits;
        into.calls        += from.calls;
        into.repeats      += from.repeats;
        into.repeat_steps += from.repeat_steps;
        into.hits         += from.hits;
    }

    // as many positions as the longer input has
    if (_total.revisits_at.size() < _run.revisits_at.size())
    {
        _total.revisits_at.resize(_run.revisits_at.size(),
                                  0u);
    }

    // position by position
    for (std::size_t at = 0u; at < _run.revisits_at.size(); at++)
    {
        _total.revisits_at[at] += _run.revisits_at[at];
    }

    _total.steps        += _run.steps;
    _total.gate_steps   += _run.gate_steps;
    _total.touched      += _run.touched;
    _total.revisits     += _run.revisits;
    _total.memo_hits    += _run.memo_hits;
    _total.memo_misses  += _run.memo_misses;
    _total.memo_entries  = std::max(_total.memo_entries,
                                    _run.memo_entries);
    _total.peak_calls    = std::max(_total.peak_calls,
                                    _run.peak_calls);
    _total.peak_back     = std::max(_total.peak_back,
                                    _run.peak_back);
    _total.peak_marks    = std::max(_total.peak_marks,
                                    _run.peak_marks);
    _total.peak_caps     = std::max(_total.peak_caps,
                                    _run.peak_caps);

    return;
}

}  // namespace peg

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
