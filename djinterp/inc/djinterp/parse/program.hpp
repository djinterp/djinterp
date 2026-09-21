/*******************************************************************************
* djinterp [parse]                                                  program.hpp
*
*   The C++ face of the instruction stream declared in program.h.
*   `instr` is an ALIAS, not a derived type.  That is deliberate: a program is
* an array of instructions, and forming a pointer to a derived type over an
* array of its base is the one thing a zero-overhead layer over C must not do.
* Everything an instruction needs is a field read or a free function, so there
* is nothing a wrapper would buy that would be worth that.
*
*   `program` does derive, adds no data member, and is asserted
* layout-identical -- so a C driver runs a C++-built program by taking its
* address.  It iterates as a range of instructions, so a backend walks it with
* a range-for and gets raw PODs.
*
*   The instruction is asserted a literal type here, which is the compile-time
* door: a static array of them is a constant expression, and a family whose
* operands are all immediates and branch targets can therefore have its program
* built at compile time and its pool left empty.
*
* path:      /inc/djinterp/parse/program.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE INSTRUCTION
    ---------------
    1.  Types
         1.  instr
         2.  operand
         3.  op_shape
    2.  Construction
         1.  make_instr
2.  THE PROGRAM
    -----------
    1.  The container
         1.  program
    2.  Self-contained storage
         1.  fixed_program
    3.  Layout guarantees
*/

#ifndef DJINTERP_PARSE_PROGRAM_HPP_
#define DJINTERP_PARSE_PROGRAM_HPP_ 1

// std
#include <cstddef>              // std::size_t
#include <cstdint>              // std::int32_t, std::uint16_t, std::uint32_t
#include <type_traits>          // std::is_standard_layout, std::is_trivial
// djinterp
#include "../djinterp.hpp"      // framework root
#include "./charset.hpp"        // parse::charset, and NS_PARSE
#include "./diagnostic.hpp"     // parse::diagnostics
#include "./machine.hpp"        // parse::op_set
#include "./pool.hpp"           // parse::pool
#include "./program.h"          // the C IR this layer faces


NS_DJINTERP
NS_PARSE


//==============================================================================
// 1.  THE INSTRUCTION
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// instr
//   type: one instruction, unchanged.  An alias rather than a derived type, so
// an array of them is an array of exactly the type the C side stores and a
// backend may walk it with a plain pointer.
using instr = ::d_parse_instr;

// 1.1.2
// operand
//   enum: what an operand of a given opcode means.  A scoped enum over the C
// kinds, with the same underlying width.
enum class operand : std::uint8_t
{
    none    = D_PARSE_OPERAND_NONE,
    imm     = D_PARSE_OPERAND_IMM,
    target  = D_PARSE_OPERAND_TARGET,
    charset = D_PARSE_OPERAND_CHARSET,
    name    = D_PARSE_OPERAND_NAME,
    blob    = D_PARSE_OPERAND_BLOB
};

// 1.1.3
// op_shape
//   type: what one opcode's two operands mean.  A family declares a static
// array of these beside its opcode enum; that declaration is what lets
// verification and disassembly be written once for every family.
using op_shape = ::d_parse_op_shape;

// shape
//   function: an op_shape for two operand kinds, so a family's table reads as
// intent rather than as integers.
constexpr op_shape
shape(
    operand _a = operand::none,
    operand _b = operand::none
) noexcept
{
    return op_shape{ static_cast<std::uint8_t>(_a),
                     static_cast<std::uint8_t>(_b) };
}


// 1.2    Construction
//------------------------------------------------------------------------------
// 1.2.1
// make_instr
//   function: one instruction as a constant expression.  The spelling a
// compile-time program is built from.
constexpr instr
make_instr(
    int          _op,
    std::int32_t _a = D_PARSE_NO_OPERAND,
    std::int32_t _b = D_PARSE_NO_OPERAND
) noexcept
{
    return instr{ static_cast<std::uint16_t>(_op), 0u, _a, _b };
}


//==============================================================================
// 2.  THE PROGRAM
//==============================================================================


// 2.1    The container
//------------------------------------------------------------------------------
// 2.1.1
// program
//   class: an instruction stream, its operand pool, and the identity of the
// opcode space it is written in, with lifetime.  Move-only, for the same
// reason a pool is: an operand index refers into exactly one pool.
class program : public d_parse_program
{
public:
    using value_type     = instr;
    using const_iterator = const instr*;

    // program
    //   constructor: an empty program over one opcode space, owning nothing.
    explicit program(
        std::uint16_t _family = 0u
    ) noexcept
    {
        d_parse_program_init(this, _family, nullptr, 0u);
    }

    // program
    //   constructor: a program over a caller-supplied instruction array.  The
    // pool is left empty; bind one through strings() if the family's operands
    // need it.
    program(
        std::uint16_t _family,
        instr*        _code,
        std::uint32_t _capacity
    ) noexcept
    {
        d_parse_program_init(this, _family, _code, _capacity);
    }

    program(const program&)            = delete;
    program& operator=(const program&) = delete;

    // program
    //   constructor: takes over another program's storage and ownership.
    program(
        program&& _other
    ) noexcept
        : d_parse_program(_other)
    {
        d_parse_program_init(&_other, _other.family, nullptr, 0u);
    }

    // operator=
    //   function: releases this program, then takes over another's.
    program&
    operator=(
        program&& _other
    ) noexcept
    {
        // guard against self-move, which would release the storage being taken
        if (this != &_other)
        {
            d_parse_program_release(this);

            static_cast<d_parse_program&>(*this) = _other;

            d_parse_program_init(&_other, _other.family, nullptr, 0u);
        }

        return *this;
    }

    // ~program
    //   destructor: releases any storage this program owns, its pool included.
    ~program() noexcept
    {
        d_parse_program_release(this);
    }

#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
    // reserve
    //   function: replaces this program's storage with storage it allocates
    // and owns, pool included, which then grows on demand.
    D_NODISCARD bool
    reserve(
        std::uint32_t _capacity = 0u
    ) noexcept
    {
        const std::uint16_t space = family;

        d_parse_program_release(this);

        return (d_parse_program_init_heap(this, space, _capacity) == 0);
    }
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP

    // emit
    //   function: appends one instruction and returns where it landed, or -1.
    // The returned counter is what a forward branch is patched at later.
    std::int32_t
    emit(
        int          _op,
        std::int32_t _a = D_PARSE_NO_OPERAND,
        std::int32_t _b = D_PARSE_NO_OPERAND
    ) noexcept
    {
        return d_parse_program_emit(this, _op, _a, _b);
    }

    // patch
    //   function: replaces the operands of an instruction already emitted.
    // Pass D_PARSE_KEEP for an operand that should not change.
    D_NODISCARD bool
    patch(
        std::int32_t _pc,
        std::int32_t _a,
        std::int32_t _b = D_PARSE_KEEP
    ) noexcept
    {
        return (d_parse_program_patch(this, _pc, _a, _b) == 0);
    }

    // intern
    //   function: interns a character class and returns the operand index.
    std::uint32_t
    intern(
        const d_parse_charset& _set
    ) noexcept
    {
        return d_parse_program_intern_charset(this, &_set);
    }

    // intern
    //   function: interns a name and returns the operand index.
    std::uint32_t
    intern(
        const char* _name
    ) noexcept
    {
        return d_parse_program_intern_name(this, _name);
    }

    // at
    //   accessor: the instruction at a counter, or null.
    const instr*
    at(
        std::int32_t _pc
    ) const noexcept
    {
        return d_parse_program_at(this, _pc);
    }

    // set_at
    //   accessor: an interned class by operand index, or null.
    const d_parse_charset*
    set_at(
        std::uint32_t _index
    ) const noexcept
    {
        return d_parse_program_charset(this, _index);
    }

    // name_at
    //   accessor: an interned name by operand index, or "".  Never null.
    const char*
    name_at(
        std::uint32_t _index
    ) const noexcept
    {
        return d_parse_program_name(this, _index);
    }

    // verify
    //   function: checks this program against the registry that will run it
    // and the operand shapes its family declares, reporting every problem
    // found and annotating every branch target.
    D_NODISCARD bool
    verify(
        const d_parse_op_set&     _ops,
        const op_shape*           _shapes,
        std::uint32_t             _shapes_n,
        d_parse_diag_sink*        _diag = nullptr
    ) noexcept
    {
        return (d_parse_program_verify(this,
                                       &_ops,
                                       _shapes,
                                       _shapes_n,
                                       _diag) == 0);
    }

    // verified
    //   accessor: whether verify has passed since the last edit.
    constexpr bool
    verified() const noexcept
    {
        return ((flags & D_PARSE_PROGRAM_VERIFIED) != 0u);
    }

    // digest
    //   accessor: a 64-bit key over the whole artifact -- family, entry,
    // instructions, and pool.  Stable across a rebuild and across verification,
    // which is what makes it usable as a cache key.
    std::uint64_t
    digest() const noexcept
    {
        return d_parse_program_hash(this);
    }

#if (D_INTERNAL_PARSE_PROGRAM_TRANSPORT == 1)
    // write
    //   function: writes this program in the transport format, or measures it
    // when _out is null.  A return greater than _size means nothing was
    // written and the caller should retry with that size.
    std::size_t
    write(
        void*       _out,
        std::size_t _size
    ) const noexcept
    {
        return d_parse_program_write(this, _out, _size);
    }

    // read
    //   function: replaces this program with one read from a buffer.  The
    // buffer is untrusted and fully checked.
    D_NODISCARD bool
    read(
        const void*        _in,
        std::size_t        _size,
        d_parse_diag_sink* _diag = nullptr
    ) noexcept
    {
        return (d_parse_program_read(this, _in, _size, _diag) == 0);
    }
#endif  // D_INTERNAL_PARSE_PROGRAM_TRANSPORT

    // size
    //   accessor: how many instructions the program holds.
    constexpr std::uint32_t
    size() const noexcept
    {
        return count;
    }

    // empty
    //   accessor: whether the program holds any instruction.
    constexpr bool
    empty() const noexcept
    {
        return (count == 0u);
    }

    // space
    //   accessor: which private opcode space this program is written in.
    constexpr std::uint16_t
    space() const noexcept
    {
        return family;
    }

    // begin
    //   accessor: a pointer to the first instruction.
    constexpr const_iterator
    begin() const noexcept
    {
        return code;
    }

    // end
    //   accessor: a pointer one past the last instruction.
    constexpr const_iterator
    end() const noexcept
    {
        return (code != nullptr) ? (code + count) : nullptr;
    }

    // strings
    //   accessor: the operand pool, for a caller interning directly.
    constexpr d_parse_pool&
    strings() noexcept
    {
        return pool;
    }
};


// 2.2    Self-contained storage
//------------------------------------------------------------------------------
// 2.2.1
// fixed_program
//   class: a program carrying its own instruction array and pool, for a stage
// that must not allocate -- a program whose size is known, or one built on a
// target with no allocator at all.
template<std::uint32_t _Capacity,
         std::uint32_t _PoolBytes   = 256u,
         std::uint32_t _PoolEntries = 16u>
class fixed_program : public program
{
public:
    // fixed_program
    //   constructor: binds the embedded storage as this program's.
    explicit fixed_program(
        std::uint16_t _family = 0u
    ) noexcept
    {
        d_parse_program_init(this, _family, m_code, _Capacity);
        d_parse_pool_init(&this->pool,
                          m_bytes,
                          _PoolBytes,
                          m_entries,
                          _PoolEntries);
    }

private:
    instr              m_code[_Capacity];
    char               m_bytes[_PoolBytes];
    d_parse_pool_entry m_entries[_PoolEntries];
};


// 2.3    Layout guarantees
//------------------------------------------------------------------------------
//   The instruction assertions are the load-bearing ones: everything the
// pipeline above expects of a program -- that it copies, hashes, serialises,
// crosses the language boundary, and can live in static storage -- follows
// from the instruction being a trivial, standard-layout, twelve-byte POD.
static_assert(sizeof(instr) == 12u,
              "an instruction is twelve bytes: opcode, flags, and two "
              "operands, with no padding");
static_assert(std::is_trivial<instr>::value,
              "an instruction must remain trivial, or a program stops being "
              "memcpy-able");
static_assert(std::is_standard_layout<instr>::value,
              "an instruction must remain standard-layout, or its written "
              "form stops matching its stored form");
static_assert(make_instr(0, 1, 2).a == 1,
              "an instruction must remain constructible in a constant "
              "expression, or a program cannot be built at compile time");
static_assert(std::is_trivially_destructible<instr>::value,
              "an instruction must remain trivially destructible, so a "
              "program may live in static storage");
static_assert(sizeof(program) == sizeof(d_parse_program),
              "parse::program must add no data member");
static_assert(alignof(program) == alignof(d_parse_program),
              "parse::program must add no data member");
static_assert(std::is_standard_layout<program>::value,
              "parse::program must remain standard-layout");


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PROGRAM_HPP_
