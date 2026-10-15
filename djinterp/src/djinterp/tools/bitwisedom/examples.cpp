/*******************************************************************************
* bitwisedom [bitwisedom]                                           examples.cpp
*
* Twenty-one sheets that show what a sheet can hold.
*   uart_cr is a control register: flags, ranges with named values,
* expressions over both, a hand-written macro, and collapsed reserved bits.
* rv32i_itype is an instruction encoding, with a signed immediate whose
* values run negative. posix_mode is a permission word written in octal,
* with its grid's major lines every three bits to match.
*   The rest are real words, from one byte to sixty-four. HARDWARE: a USB
* setup packet's first byte, every bit a field; an LM75's temperature, a
* signed field in a 16-bit register; x86's EFLAGS, flags scattered through
* it; an x86-64 page-table entry, a 40-bit frame between flags at both ends;
* and a PCI function's configuration header, 512 bits, wider than any
* integer: its header keeps it in an array of its sixteen registers.
* FORMATS: an IEEE 754 float, its exponent offset binary with a bias of 127;
* an RGB565 pixel, coloured by its channels. And the flag words and codes of
* WINDOWS (HRESULT, window styles), MACOS (Mach-O's header flags and CPU
* types, a signed int), LINUX (ioctl numbers, epoll events, capabilities)
* and FREEBSD (file flags; kevent's flags, and its filters, negative
* shorts), each with the values of the header it names, which the tests
* compare against that header wherever the build has it.
*   LARGE: every HRESULT MinGW-w64's winerror.h defines, 1,376 of them in a
* group for each facility; and ten of CPUID's feature registers, 320 bits,
* with x86-64's levels as sets of them. These two are JSON, in
* examples/large, which the build embeds (cmake/EmbedExamples.cmake) and
* this file reads as it would any bitfield; tools/ holds the scripts that
* made them.
*
* path:      /src/djinterp/tools/bitwisedom/examples.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/examples.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <string_view>  // std::string_view
#include <utility>      // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/exchange.hpp>  // read_json_sheet


namespace bitwisedom
{

namespace
{

// LARGE_WIN32_HRESULTS, LARGE_X86_CPUID
//   constant: the large examples' bitfields, byte by byte, as the build
// embedded them (cmake/EmbedExamples.cmake).
#include "large_examples.inc"

// make
//   struct: a sheet under construction, and the group rows go into.
struct make
{
    sheet s;
    int   group = 0;

    int add(const row& _row)
    {
        return insert_row(s, group, -1, _row);
    }

    int open(
        const std::string& _name,
        const std::string& _prefix,
        const std::string& _note
    )
    {
        row made;
        made.kind   = row_kind::group;
        made.name   = _name;
        made.prefix = _prefix;
        made.note   = _note;

        group = insert_row(s, 0, -1, made);

        return group;
    }

    int bit(
        const std::string& _name,
        int                _bit,
        const std::string& _note
    )
    {
        row made;
        made.name = _name;
        made.def  = def_kind::bit;
        made.bit  = _bit;
        made.note = _note;

        return add(made);
    }

    int range(
        const std::string& _name,
        int                _lsb,
        int                _width,
        const std::string& _note
    )
    {
        row made;
        made.name  = _name;
        made.def   = def_kind::range;
        made.lsb   = _lsb;
        made.width = _width;
        made.note  = _note;

        return add(made);
    }

    int value(
        const std::string& _name,
        int                _field,
        i64                _number,
        encoding           _encoding,
        const std::string& _note,
        i64                _bias = 0
    )
    {
        row made;
        made.name   = _name;
        made.def    = def_kind::field_value;
        made.field  = _field;
        made.number = _number;
        made.enc    = _encoding;
        made.bias   = _bias;
        made.note   = _note;

        return add(made);
    }

    // a value in a range, as nothing but a number: the common case
    int number(
        const std::string& _name,
        int                _field,
        i64                _number,
        const std::string& _note
    )
    {
        return value(_name, _field, _number, encoding::unsigned_binary,
                     _note);
    }

    int expr(
        const std::string& _name,
        const std::string& _expr,
        const std::string& _note
    )
    {
        row made;
        made.name = _name;
        made.def  = def_kind::expression;
        made.expr = _expr;
        made.note = _note;

        return add(made);
    }

    int literal(
        const std::string& _name,
        u64                _value,
        const std::string& _note
    )
    {
        row made;
        made.name    = _name;
        made.def     = def_kind::literal;
        made.literal = _value;
        made.note    = _note;

        return add(made);
    }

    // a literal written as the negative number it is
    int negative(
        const std::string& _name,
        i64                _value,
        const std::string& _note
    )
    {
        row made;
        made.name     = _name;
        made.def      = def_kind::literal;
        made.literal  = static_cast<u64>(_value);
        made.negative = (_value < 0);
        made.note     = _note;

        return add(made);
    }

    int macro(
        const std::string& _name,
        const std::string& _params,
        const std::string& _body,
        const std::string& _note
    )
    {
        row made;
        made.name   = _name;
        made.def    = def_kind::macro;
        made.params = _params;
        made.body   = _body;
        made.note   = _note;

        return add(made);
    }

    // the group being filled may hold symbols that share bits: aliases,
    // or a mask over the flags it names
    void shared()
    {
        find_row(s, group)->exclusive = false;

        return;
    }

    // a row's own colour, #RRGGBBAA
    void tint(
        int  _id,
        rgba _colour
    )
    {
        find_row(s, _id)->colour = _colour;

        return;
    }

    // a template of the defaults, switched on or off by its name
    void use_template(
        const std::string& _name,
        bool               _on
    )
    {
        for (macro_template& each : s.gen.templates)
        {
            if (each.name == _name)
            {
                each.enabled = _on;
            }
        }

        return;
    }

    void segment(
        const std::string& _name,
        int                _lsb,
        int                _msb,
        bool               _collapsed
    )
    {
        column_segment made;
        made.name      = _name;
        made.lsb       = _lsb;
        made.msb       = _msb;
        made.collapsed = _collapsed;
        s.segments.push_back(made);

        return;
    }
};

sheet
uart_cr()
{
    make m;
    m.s = make_sheet("uart_cr", 32);
    m.s.prefix = "UART_CR_";
    m.s.probe  = 0x00360107u;

    m.open("Enables", "", "Switches for the UART's two halves.");
    m.bit("EN", 0, "enables the UART");
    m.bit("TXEN", 1, "enables the transmitter");
    m.bit("RXEN", 2, "enables the receiver");
    m.bit("LOOPBACK", 3, "routes the transmitter back into the receiver");

    m.open("Format", "", "The frame: parity, stop bits and data bits.");

    const int parity = m.range("PARITY", 4, 2, "the parity mode");
    m.value("PARITY_NONE", parity, 0, encoding::unsigned_binary,
            "no parity bit");
    m.value("PARITY_EVEN", parity, 2, encoding::unsigned_binary,
            "even parity");
    m.value("PARITY_ODD", parity, 3, encoding::unsigned_binary,
            "odd parity");

    const int stop = m.range("STOP", 6, 1, "the number of stop bits, less "
                             "one");
    m.value("STOP_1", stop, 0, encoding::unsigned_binary, "one stop bit");
    m.value("STOP_2", stop, 1, encoding::unsigned_binary, "two stop bits");

    const int data = m.range("DATA", 8, 2, "the number of data bits, less "
                             "five");
    m.value("DATA_7", data, 2, encoding::unsigned_binary, "seven data bits");
    m.value("DATA_8", data, 3, encoding::unsigned_binary, "eight data bits");

    m.open("Baud", "", "The baud-rate divisor.");

    const int divisor = m.range("DIV", 16, 12, "the baud-rate divisor");
    m.value("DIV_115200", divisor, 54, encoding::unsigned_binary,
            "115200 baud from a 100 MHz clock");

    m.open("Presets", "", "Whole-register values, built from the above.");
    m.expr("DEFAULT", "EN | TXEN | RXEN | DATA_8 | PARITY_NONE | STOP_1",
           "8N1, both halves on");
    m.expr("ENABLES", "GENMASK(2, 0)", "every enable bit");
    m.macro("IS_ENABLED", "x", "(((x) & {EN}) != 0)",
            "whether a register value has the UART enabled");

    m.segment("enables", 0, 3, false);
    m.segment("format", 4, 11, false);
    m.segment("baud", 16, 27, false);
    m.segment("reserved", 28, 31, true);
    m.s.gen.file_name = "uart_cr.h";

    return m.s;
}

sheet
rv32i_itype()
{
    make m;
    m.s = make_sheet("rv32i_itype", 32);
    m.s.prefix = "RV_I_";
    m.s.probe  = 0xFFF00093u;   // addi x1, x0, -1

    m.open("Fields", "", "Where an I-type instruction keeps its parts.");

    const int opcode = m.range("OPCODE", 0, 7, "the major opcode");
    m.range("RD", 7, 5, "the destination register");
    const int funct3 = m.range("FUNCT3", 12, 3, "the minor opcode");
    m.range("RS1", 15, 5, "the source register");
    const int imm = m.range("IMM", 20, 12,
                            "the immediate, two's complement");

    m.open("Opcodes", "", "");
    m.value("OP_IMM", opcode, 0x13, encoding::unsigned_binary,
            "register-immediate arithmetic");
    m.value("LOAD", opcode, 0x03, encoding::unsigned_binary, "loads");
    m.value("JALR", opcode, 0x67, encoding::unsigned_binary,
            "jump and link register");

    m.open("Functions", "", "");
    m.value("ADDI", funct3, 0, encoding::unsigned_binary, "add");
    m.value("SLTI", funct3, 2, encoding::unsigned_binary, "set if less");
    m.value("XORI", funct3, 4, encoding::unsigned_binary, "exclusive or");
    m.value("ORI", funct3, 6, encoding::unsigned_binary, "or");
    m.value("ANDI", funct3, 7, encoding::unsigned_binary, "and");

    m.open("Immediates", "", "Signed immediates: the field holds -2048 "
           "to 2047.");
    m.value("IMM_MINUS_ONE", imm, -1, encoding::twos_complement,
            "minus one: every bit set");
    m.value("IMM_MIN", imm, -2048, encoding::twos_complement,
            "the most negative immediate");
    m.value("IMM_MAX", imm, 2047, encoding::twos_complement,
            "the most positive immediate");

    m.open("Encodings", "", "");
    m.expr("NOP", "OP_IMM | ADDI", "addi x0, x0, 0");
    m.macro("ITYPE", "op, rd, rs1, imm",
            "((op) | (((rd) & 0x1Fu) << 7) | (((rs1) & 0x1Fu) << 15) | "
            "(((imm) & 0xFFFu) << 20))",
            "an I-type instruction from its parts");

    m.segment("opcode", 0, 6, false);
    m.segment("rd", 7, 11, false);
    m.segment("funct3", 12, 14, false);
    m.segment("rs1", 15, 19, false);
    m.segment("imm", 20, 31, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "rv32i_itype.h";

    return m.s;
}

sheet
posix_mode()
{
    make m;
    m.s = make_sheet("posix_mode", 16);
    m.s.prefix    = "MODE_";
    m.s.probe     = 0755;
    m.s.gen.base  = radix::oct;
    m.s.grid.major_every   = 3;
    m.s.grid.auto_segments = 0;

    m.open("Owner", "OWNER_", "");
    m.bit("R", 8, "the owner may read");
    m.bit("W", 7, "the owner may write");
    m.bit("X", 6, "the owner may execute");

    m.open("Group", "GROUP_", "");
    m.bit("R", 5, "the group may read");
    m.bit("W", 4, "the group may write");
    m.bit("X", 3, "the group may execute");

    m.open("Other", "OTHER_", "");
    m.bit("R", 2, "others may read");
    m.bit("W", 1, "others may write");
    m.bit("X", 0, "others may execute");

    m.open("Special", "", "");
    m.bit("SETUID", 11, "run as the file's owner");
    m.bit("SETGID", 10, "run as the file's group");
    m.bit("STICKY", 9, "only owners may delete");

    m.open("Common", "", "");
    m.expr("RWX_OWNER", "MODE_OWNER_R | MODE_OWNER_W | MODE_OWNER_X",
           "the owner may do anything");
    m.literal("DEFAULT_DIR", 0755, "rwxr-xr-x");
    m.literal("DEFAULT_FILE", 0644, "rw-r--r--");

    m.segment("other", 0, 2, false);
    m.segment("group", 3, 5, false);
    m.segment("owner", 6, 8, false);
    m.segment("special", 9, 11, false);
    m.segment("type", 12, 15, true);
    m.s.gen.file_name = "posix_mode.h";

    return m.s;
}


// ------------------------------------------------------------- hardware

/*
usb_request_type
  One byte, and every bit of it in a field: the first byte of a USB control
transfer's setup packet says which way its data goes, who defines the
request, and what it is addressed to.
*/
sheet
usb_request_type()
{
    make m;
    m.s = make_sheet("usb_request_type", 8);
    m.s.prefix = "USB_RT_";
    m.s.probe  = 0xA1u;   // a class request to an interface, device to host

    m.open("Fields", "", "bmRequestType, as section 9.3 of USB 2.0 lays it "
           "out.");

    const int dir   = m.range("DIR", 7, 1, "which way the data stage goes");
    const int type  = m.range("TYPE", 5, 2, "who defines the request");
    const int recip = m.range("RECIPIENT", 0, 5,
                              "what the request is addressed to");

    m.open("Direction", "DIR_", "");
    m.number("OUT", dir, 0, "host to device");
    m.number("IN", dir, 1, "device to host");

    m.open("Type", "TYPE_", "");
    m.number("STANDARD", type, 0, "defined by the USB specification");
    m.number("CLASS", type, 1, "defined by a device class: HID, CDC, mass "
             "storage");
    m.number("VENDOR", type, 2, "defined by the device's vendor");
    m.number("RESERVED", type, 3, "reserved");

    m.open("Recipient", "RECIP_", "");
    m.number("DEVICE", recip, 0, "the device");
    m.number("INTERFACE", recip, 1, "one of its interfaces");
    m.number("ENDPOINT", recip, 2, "one of its endpoints");
    m.number("OTHER", recip, 3, "something else");

    m.open("Requests", "", "The bytes common requests begin with.");
    m.expr("STANDARD_IN",
           "USB_RT_DIR_IN | USB_RT_TYPE_STANDARD | USB_RT_RECIP_DEVICE",
           "GET_DESCRIPTOR, GET_STATUS, GET_CONFIGURATION");
    m.expr("STANDARD_OUT",
           "USB_RT_DIR_OUT | USB_RT_TYPE_STANDARD | USB_RT_RECIP_DEVICE",
           "SET_ADDRESS, SET_CONFIGURATION");
    m.expr("CLASS_IN",
           "USB_RT_DIR_IN | USB_RT_TYPE_CLASS | USB_RT_RECIP_INTERFACE",
           "a HID GET_REPORT");
    m.expr("CLASS_OUT",
           "USB_RT_DIR_OUT | USB_RT_TYPE_CLASS | USB_RT_RECIP_INTERFACE",
           "a HID SET_REPORT, a CDC SET_LINE_CODING");
    m.expr("VENDOR_IN",
           "USB_RT_DIR_IN | USB_RT_TYPE_VENDOR | USB_RT_RECIP_DEVICE",
           "a vendor's request, answered by the device");
    m.expr("VENDOR_OUT",
           "USB_RT_DIR_OUT | USB_RT_TYPE_VENDOR | USB_RT_RECIP_DEVICE",
           "a vendor's request, with data for the device");

    m.segment("recipient", 0, 4, false);
    m.segment("type", 5, 6, false);
    m.segment("dir", 7, 7, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "usb_request_type.h";

    return m.s;
}

/*
lm75_temperature
  A signed field in a 16-bit register: an LM75 reads out the temperature as
nine bits of two's complement, in half degrees, at the top of the word.
*/
sheet
lm75_temperature()
{
    make m;
    m.s = make_sheet("lm75_temperature", 16);
    m.s.prefix = "LM75_";
    m.s.probe  = 0x1900u;   // +25 C

    m.open("Register", "", "The temperature register, as it is read, most "
           "significant byte first: nine bits of two's complement in half "
           "degrees Celsius, and seven bits of zero below them.");

    const int temp = m.range("TEMP", 7, 9,
                             "the temperature in half degrees: -256 to 255 "
                             "fit, -110 to 250 are measured");

    m.open("Temperatures", "", "The datasheet's examples, in half degrees.");
    m.value("PLUS_125C", temp, 250, encoding::twos_complement,
            "+125 C, the top of its range");
    m.value("PLUS_25C", temp, 50, encoding::twos_complement, "+25 C");
    m.value("PLUS_HALF", temp, 1, encoding::twos_complement,
            "+0.5 C, one step");
    m.value("ZERO", temp, 0, encoding::twos_complement, "0 C");
    m.value("MINUS_HALF", temp, -1, encoding::twos_complement,
            "-0.5 C: every bit of the field set");
    m.value("MINUS_25C", temp, -50, encoding::twos_complement, "-25 C");
    m.value("MINUS_55C", temp, -110, encoding::twos_complement,
            "-55 C, the bottom of its range");

    m.open("Conversion", "", "");
    m.macro("HALF_DEGREES", "reg", "((int)(short)(reg) / 128)",
            "the temperature in half degrees from the register as read: "
            "the sign carried in by a short, the zeros divided away");

    m.segment("unused", 0, 6, true);
    m.segment("temperature", 7, 15, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "lm75_temperature.h";

    return m.s;
}

/*
x86_eflags
  A sparse word: the flags of x86's EFLAGS register lie scattered through
its low 22 bits, with a two-bit privilege level among them, one reserved bit
that always reads as one, and ten reserved bits above.
*/
sheet
x86_eflags()
{
    make m;
    m.s = make_sheet("x86_eflags", 32);
    m.s.prefix = "EFLAGS_";
    m.s.probe  = 0x00000246u;   // IF, ZF, PF and the fixed bit

    m.open("Status", "", "What arithmetic leaves behind.");
    m.bit("CF", 0, "carry, or borrow, out of the top bit");
    m.bit("PF", 2, "parity: the low byte holds an even number of ones");
    m.bit("AF", 4, "auxiliary carry, out of bit 3: for BCD");
    m.bit("ZF", 6, "the result was zero");
    m.bit("SF", 7, "the result's sign bit");
    m.bit("OF", 11, "signed overflow");

    m.open("Control", "", "");
    m.bit("DF", 10, "direction: string instructions count down");

    m.open("System", "", "The operating system's.");
    m.bit("TF", 8, "trap: single-step");
    m.bit("IF", 9, "maskable interrupts enabled");

    const int iopl = m.range("IOPL", 12, 2,
                             "the I/O privilege level, 0 to 3");

    m.bit("NT", 14, "nested task");
    m.bit("RF", 16, "resume: no debug fault for one instruction");
    m.bit("VM", 17, "virtual-8086 mode");
    m.bit("AC", 18, "alignment check");
    m.bit("VIF", 19, "virtual interrupt flag");
    m.bit("VIP", 20, "virtual interrupt pending");
    m.bit("ID", 21, "settable only where the CPUID instruction exists");

    m.open("Privilege", "IOPL_", "");
    m.number("RING0", iopl, 0, "only the kernel may do I/O");
    m.number("RING3", iopl, 3, "every ring may");

    m.open("Reserved", "", "");
    m.bit("FIXED", 1, "reserved, and always reads as one");

    m.open("Values", "", "");
    m.expr("ARITHMETIC",
           "EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | "
           "EFLAGS_OF",
           "the flags arithmetic sets");
    m.literal("RESET", 0x00000002u, "after a reset: the fixed bit alone");
    m.expr("USER", "EFLAGS_FIXED | EFLAGS_IF",
           "a user process starts with interrupts on");

    m.segment("reserved", 22, 31, true);
    m.s.gen.file_name = "x86_eflags.h";

    return m.s;
}

/*
x86_64_pte
  Eight bytes: an x86-64 page-table entry keeps its flags at both ends and a
40-bit physical frame number between them, collapsed here to one column.
*/
sheet
x86_64_pte()
{
    make m;
    m.s = make_sheet("x86_64_pte", 64);
    m.s.prefix     = "PTE_";
    m.s.probe      = 0x8000000123456067ull;   // user data, not executable
    m.s.gen.suffix = int_suffix::ull;

    m.open("Flags", "", "The low twelve bits of a 4 KiB page's entry.");
    m.bit("P", 0, "present: the rest means something only when set");
    m.bit("RW", 1, "writable");
    m.bit("US", 2, "user: reachable from ring 3");
    m.bit("PWT", 3, "write-through");
    m.bit("PCD", 4, "cache disabled");
    m.bit("A", 5, "accessed: the processor sets it");
    m.bit("D", 6, "dirty: written since it was cleared");
    m.bit("PAT", 7, "with PCD and PWT, picks a page attribute");
    m.bit("G", 8, "global: kept in the TLB across address spaces");

    m.open("Frame and the rest", "", "");
    m.range("AVL", 9, 3, "ignored by the processor: the system's own");

    const int frame = m.range("ADDR", 12, 40,
                              "the physical frame: bits 12 to 51 of its "
                              "address");

    m.range("AVL_HIGH", 52, 7, "ignored as well");
    m.range("PKEY", 59, 4, "the protection key, 0 to 15, with CR4.PKE");
    m.bit("XD", 63, "execute disable, with EFER.NXE");
    m.tint(frame, 0x8D99AEFFu);

    m.open("Entries", "", "Some entries' flags, as a kernel sets them.");
    m.expr("KERNEL_DATA",
           "PTE_P | PTE_RW | PTE_A | PTE_D | PTE_G | PTE_XD",
           "the kernel's data: writable, global, never executed");
    m.expr("USER_CODE", "PTE_P | PTE_US | PTE_A",
           "a program's code: read and executed, not written");
    m.expr("USER_DATA",
           "PTE_P | PTE_RW | PTE_US | PTE_A | PTE_D | PTE_XD",
           "a program's data: read and written, not executed");

    m.open("Building", "", "");
    m.macro("FRAME_OF", "pte", "((pte) & {ADDR})",
            "the physical address an entry maps");
    m.macro("MAKE", "phys, flags", "(((phys) & {ADDR}) | (flags))",
            "an entry for a page-aligned physical address");

    m.segment("flags", 0, 11, false);
    m.segment("frame", 12, 51, true);
    m.segment("high", 52, 63, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "x86_64_pte.h";

    return m.s;
}

/*
pci_config_header
  Sixty-four bytes: the configuration header every PCI function has, read
as sixteen 32-bit registers. At 512 bits it is wider than any integer, so
its header keeps it in an array of those registers -- word n the one at
byte 4n -- and names each field and bit by its place: its byte offset times
eight, and its bit within. Those are linux/pci_regs.h's offsets and masks,
which the build compares with that header where it has it.
*/
sheet
pci_config_header()
{
    make m;
    m.s = make_sheet("pci_config_header", 512);
    m.s.prefix   = "PCI_CFG_";
    m.s.msb_left = false;   // byte 0 first, as the space is read

    // the probe: a network controller, as a driver might find it
    wide probe;

    probe.put(0, 16, 0x8086u);        // the vendor
    probe.put(16, 16, 0x100Eu);       // the device
    probe.put(32, 16, 0x0007u);       // I/O, memory, bus master
    probe.put(48, 16, 0x0010u);       // a capability list
    probe.put(64, 8, 0x03u);          // the revision
    probe.put(72, 24, 0x020000u);     // a network controller: Ethernet
    probe.put(128, 32, 0xFEBC0000u);  // BAR0: 32-bit memory
    probe.put(160, 32, 0x0000C001u);  // BAR1: I/O
    probe.put(352, 16, 0x8086u);      // the board's vendor
    probe.put(368, 16, 0x001Eu);      // the board
    probe.put(416, 8, 0xDCu);         // the first capability
    probe.put(480, 8, 0x0Bu);         // routed to line 11
    probe.put(488, 8, 0x01u);         // INTA#
    m.s.probe = probe;

    m.open("Registers", "", "The header's fields, each where the "
           "configuration space keeps it: its first bit is its byte offset "
           "times eight.");

    const int vendor = m.range("VENDOR_ID", 0, 16,
                               "the vendor, as the PCI-SIG assigned it");
    m.number("VENDOR_NONE", vendor, 0xFFFF,
             "what a read returns where no function answers");
    m.number("VENDOR_INTEL", vendor, 0x8086, "Intel");
    m.range("DEVICE_ID", 16, 16, "the device, as its vendor numbers it");
    m.range("COMMAND", 32, 16, "what the function may do: the Command "
            "group's bits");
    m.range("STATUS", 48, 16, "what has happened: the Status group's bits");
    m.range("REVISION_ID", 64, 8, "the device's revision");
    m.range("CLASS_PROG", 72, 8, "the programming interface, within its "
            "subclass");
    m.range("CLASS_SUB", 80, 8, "the subclass, within its class");

    const int base = m.range("CLASS_BASE", 88, 8,
                             "the class: what the function is");
    m.number("CLASS_STORAGE", base, 0x01, "a mass-storage controller");
    m.number("CLASS_NETWORK", base, 0x02, "a network controller");
    m.number("CLASS_DISPLAY", base, 0x03, "a display controller");
    m.number("CLASS_MULTIMEDIA", base, 0x04, "a multimedia controller");
    m.number("CLASS_MEMORY", base, 0x05, "a memory controller");
    m.number("CLASS_BRIDGE", base, 0x06, "a bridge");
    m.number("CLASS_SERIAL", base, 0x0C,
             "a serial-bus controller: USB, SMBus and the like");
    m.range("CACHE_LINE_SIZE", 96, 8, "the cache line, in 32-bit words");
    m.range("LATENCY_TIMER", 104, 8,
            "how long the function may keep the bus, in clocks");
    m.range("HEADER_TYPE", 112, 8, "the layout of the rest, and whether "
            "the device has more functions");
    m.range("BIST", 120, 8, "the built-in self-test");

    for (int i = 0; i < 6; ++i)
    {
        const std::string n = std::to_string(i);

        m.range("BAR" + n, 128 + 32 * i, 32,
                "base address register " + n + ": where the system put "
                "one of the function's windows");
    }

    m.range("CARDBUS_CIS", 320, 32,
            "where a CardBus card's information structure is");
    m.range("SUBSYSTEM_VENDOR_ID", 352, 16,
            "the vendor of the board the device is on");
    m.range("SUBSYSTEM_ID", 368, 16, "the board, as that vendor numbers it");
    m.range("ROM_ADDRESS", 384, 32,
            "the expansion ROM's address, and its enable");
    m.range("CAPABILITY_LIST", 416, 8,
            "the first capability's offset, when STATUS_CAP_LIST is set");
    m.range("RESERVED_35", 424, 24, "reserved");
    m.range("RESERVED_38", 448, 32, "reserved");
    m.range("INTERRUPT_LINE", 480, 8,
            "the interrupt line the system routed the function to");

    const int pin = m.range("INTERRUPT_PIN", 488, 8,
                            "the interrupt pin the function raises");
    m.number("PIN_NONE", pin, 0, "no interrupt pin");
    m.number("PIN_INTA", pin, 1, "INTA#");
    m.number("PIN_INTB", pin, 2, "INTB#");
    m.number("PIN_INTC", pin, 3, "INTC#");
    m.number("PIN_INTD", pin, 4, "INTD#");
    m.range("MIN_GNT", 496, 8,
            "the burst the function needs, in quarters of a microsecond");
    m.range("MAX_LAT", 504, 8,
            "how often it needs the bus, in quarters of a microsecond");

    m.open("Command", "CMD_", "COMMAND's bits: each lets the function do "
           "something.");
    m.bit("IO", 32, "it answers in I/O space");
    m.bit("MEMORY", 33, "it answers in memory space");
    m.bit("MASTER", 34, "it may master the bus: DMA");
    m.bit("SPECIAL", 35, "it watches special cycles");
    m.bit("INVALIDATE", 36, "it may use memory write and invalidate");
    m.bit("VGA_PALETTE", 37, "it snoops VGA palette writes");
    m.bit("PARITY", 38, "it reports parity errors");
    m.bit("WAIT", 39, "address and data stepping, long obsolete");
    m.bit("SERR", 40, "it may drive SERR#");
    m.bit("FAST_BACK", 41, "it may write back to back, fast");
    m.bit("INTX_DISABLE", 42, "it may not raise INTx#: message interrupts "
          "only");

    m.open("Status", "STATUS_", "STATUS's bits. The errors among them are "
           "cleared by writing them as 1.");
    m.bit("IMM_READY", 48, "it is ready at once after a reset");
    m.bit("INTERRUPT", 51, "an INTx# interrupt is pending");
    m.bit("CAP_LIST", 52, "it has a capability list, at CAPABILITY_LIST");
    m.bit("66MHZ", 53, "it runs at 66 MHz");
    m.bit("UDF", 54, "user-definable features, long obsolete");
    m.bit("FAST_BACK", 55, "it accepts fast back-to-back transactions");
    m.bit("PARITY", 56, "as a master, it saw a data parity error");

    const int devsel = m.range("DEVSEL", 57, 2,
                               "how fast it claims a transaction");
    m.number("DEVSEL_FAST", devsel, 0, "fast");
    m.number("DEVSEL_MEDIUM", devsel, 1, "medium");
    m.number("DEVSEL_SLOW", devsel, 2, "slow");
    m.bit("SIG_TARGET_ABORT", 59, "it ended a transaction with a target "
          "abort");
    m.bit("REC_TARGET_ABORT", 60, "a transaction it mastered ended in a "
          "target abort");
    m.bit("REC_MASTER_ABORT", 61, "a transaction it mastered ended in a "
          "master abort");
    m.bit("SIG_SYSTEM_ERROR", 62, "it drove SERR#");
    m.bit("DETECTED_PARITY", 63, "it saw a parity error");

    m.open("Header type", "HEADER_", "HEADER_TYPE's parts.");

    const int layout = m.range("LAYOUT", 112, 7,
                               "the layout of bytes 10h to 3Fh");
    m.number("NORMAL", layout, 0, "a device's: this one");
    m.number("BRIDGE", layout, 1, "a PCI-to-PCI bridge's");
    m.number("CARDBUS", layout, 2, "a CardBus bridge's");
    m.bit("MFD", 119, "the device has functions besides function 0");

    m.open("Self-test", "BIST_", "BIST's parts.");
    m.range("CODE", 120, 4, "the self-test's result: 0 when it passed");
    m.bit("START", 126, "starts the self-test; the device clears it when "
          "it is done");
    m.bit("CAPABLE", 127, "the device has a self-test");

    m.open("BAR0", "BAR0_", "A base address register's parts, as BAR0 "
           "has them: BAR1 to BAR5 have the same. A memory BAR and an I/O "
           "BAR read their bits differently, so the parts overlap.");
    m.shared();
    m.bit("SPACE_IO", 128, "1 for I/O space, 0 for memory");

    const int type = m.range("MEM_TYPE", 129, 2, "how wide a memory BAR's "
                             "address is");
    m.number("MEM_TYPE_32", type, 0, "32 bits");
    m.number("MEM_TYPE_1M", type, 1, "below 1 MiB, long obsolete");
    m.number("MEM_TYPE_64", type, 2, "64 bits, with BAR1 the high half");
    m.bit("MEM_PREFETCH", 131, "reading has no side effects: it may be "
          "prefetched");
    m.range("MEM_ADDRESS", 132, 28, "a memory BAR's address, from its "
            "bit 4");
    m.range("IO_ADDRESS", 130, 30, "an I/O BAR's address, from its bit 2");

    m.open("Expansion ROM", "ROM_", "ROM_ADDRESS's parts.");
    m.bit("ENABLE", 384, "the ROM answers at its address");
    m.range("BASE", 395, 21, "the ROM's address, from its bit 11");

    m.open("Presets", "", "Sets of bits a driver writes, and a way from a "
           "place to the configuration space.");
    m.expr("ENABLE", "PCI_CFG_CMD_IO | PCI_CFG_CMD_MEMORY | "
           "PCI_CFG_CMD_MASTER",
           "what a driver sets in COMMAND to turn the function on");
    m.expr("ERRORS", "PCI_CFG_STATUS_PARITY | PCI_CFG_STATUS_SIG_TARGET_ABORT"
           " | PCI_CFG_STATUS_REC_TARGET_ABORT | "
           "PCI_CFG_STATUS_REC_MASTER_ABORT | "
           "PCI_CFG_STATUS_SIG_SYSTEM_ERROR | PCI_CFG_STATUS_DETECTED_PARITY",
           "STATUS's errors: written back as 1s, they clear");
    m.expr("RESERVED", "PCI_CFG_RESERVED_35 | PCI_CFG_RESERVED_38",
           "every reserved bit");
    m.macro("BYTE", "n", "((n) / 8)",
            "the configuration-space byte bit n is in: BYTE(COMMAND) is 4");

    // a register's bits by their word and mask too, and the tests
    m.use_template("{NAME}_WORD", true);
    m.use_template("{NAME}_MASK", true);
    m.use_template("{NAME}_IS_SET", true);
    m.use_template("{NAME}_IS", true);

    const char* const names[16] = { "ids", "command", "class", "header",
                                    "BAR0", "BAR1", "BAR2", "BAR3", "BAR4",
                                    "BAR5", "CIS", "subsystem", "ROM",
                                    "capabilities", "reserved",
                                    "interrupt" };

    for (int i = 0; i < 16; ++i)
    {
        m.segment(names[i], 32 * i, 32 * i + 31,
                  ( (i >= 6) && (i <= 10) ) || (i == 14));
    }

    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "pci_config_header.h";

    return m.s;
}

// -------------------------------------------------------------- formats

/*
ieee754_single
  A float, bit for bit: a sign, an exponent stored as offset binary with a
bias of 127, and a fraction whose leading 1 is implied.
*/
sheet
ieee754_single()
{
    make m;
    m.s = make_sheet("ieee754_single", 32);
    m.s.prefix = "F32_";
    m.s.probe  = 0x40490FDBu;   // pi

    m.open("Fields", "", "An IEEE 754 binary32: C's and C++'s float.");
    m.bit("SIGN", 31, "set for negative numbers, and for -0");

    const int exponent = m.range("EXPONENT", 23, 8,
                                 "the power of two, plus 127");

    m.range("FRACTION", 0, 23, "the bits after the binary point");

    m.open("Exponents", "EXP_", "Offset binary with a bias of 127: the field "
           "holds the exponent plus 127.");
    m.value("SUBNORMAL", exponent, -127, encoding::offset_binary,
            "all zeros: zero, or a number too small to be normal", 127);
    m.value("MIN", exponent, -126, encoding::offset_binary,
            "the smallest normal exponent: 2 to the -126", 127);
    m.value("ZERO", exponent, 0, encoding::offset_binary,
            "2 to the 0: the numbers from 1 up to 2", 127);
    m.value("MAX", exponent, 127, encoding::offset_binary,
            "the largest finite exponent", 127);
    m.value("SPECIAL", exponent, 128, encoding::offset_binary,
            "all ones: infinity, or not a number", 127);

    m.open("NaN", "", "");
    m.bit("QUIET", 22, "the fraction's top bit: in a NaN, set if it is "
          "quiet, clear if it signals");

    m.open("Numbers", "", "Some floats, bit for bit.");
    m.literal("ONE", 0x3F800000u, "1.0f");
    m.literal("HALF", 0x3F000000u, "0.5f");
    m.literal("MINUS_TWO", 0xC0000000u, "-2.0f");
    m.literal("PI", 0x40490FDBu, "3.14159274f, the float nearest pi");
    m.literal("MAX", 0x7F7FFFFFu, "FLT_MAX, about 3.4e38");
    m.literal("MIN_NORMAL", 0x00800000u, "FLT_MIN, about 1.18e-38");
    m.literal("MIN_SUBNORMAL", 0x00000001u,
              "the smallest positive float, about 1.4e-45");
    m.literal("NEG_ZERO", 0x80000000u,
              "-0.0f: equal to 0.0f, and not the same bits");
    m.literal("INF", 0x7F800000u, "positive infinity");
    m.literal("NEG_INF", 0xFF800000u, "negative infinity");
    m.literal("QNAN", 0x7FC00000u, "the quiet NaN compilers make");

    m.open("Masks", "", "");
    m.expr("MAGNITUDE", "F32_EXPONENT | F32_FRACTION",
           "every bit but the sign: fabsf, as a mask");

    m.segment("fraction", 0, 22, false);
    m.segment("exponent", 23, 30, false);
    m.segment("sign", 31, 31, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "ieee754_single.h";

    return m.s;
}

/*
rgb565
  A dense 16-bit pixel: five bits of red, six of green and five of blue,
each channel, level and colour drawn in the colour it is.
*/
sheet
rgb565()
{
    make m;
    m.s = make_sheet("rgb565", 16);
    m.s.prefix = "RGB565_";
    m.s.probe  = 0xFD20u;   // orange

    m.open("Channels", "", "The eye is most sensitive to green, so green "
           "gets the sixth bit.");

    const int r = m.range("R", 11, 5, "red, 0 to 31");
    const int g = m.range("G", 5, 6, "green, 0 to 63");
    const int b = m.range("B", 0, 5, "blue, 0 to 31");

    m.tint(r, 0xE5484DFFu);
    m.tint(g, 0x46A758FFu);
    m.tint(b, 0x3E63DDFFu);

    // each level and each colour drawn in the colour it is
    m.open("Levels", "", "");
    m.tint(m.number("R_MAX", r, 31, "red, full"), 0xE5484DFFu);
    m.tint(m.number("G_MAX", g, 63, "green, full"), 0x46A758FFu);
    m.tint(m.number("B_MAX", b, 31, "blue, full"), 0x3E63DDFFu);
    m.tint(m.number("R_HALF", r, 16, "red, half"), 0xA03236FFu);
    m.tint(m.number("G_HALF", g, 32, "green, half"), 0x31753EFFu);
    m.tint(m.number("B_HALF", b, 16, "blue, half"), 0x2B459BFFu);

    m.open("Colours", "", "");
    m.tint(m.literal("BLACK", 0x0000u, "every channel off"), 0x000000FFu);
    m.tint(m.expr("WHITE", "RGB565_R_MAX | RGB565_G_MAX | RGB565_B_MAX",
                  "every channel full"),
           0xEDEDEDFFu);
    m.tint(m.expr("RED", "RGB565_R_MAX", "red"), 0xE5484DFFu);
    m.tint(m.expr("GREEN", "RGB565_G_MAX", "green"), 0x46A758FFu);
    m.tint(m.expr("BLUE", "RGB565_B_MAX", "blue"), 0x3E63DDFFu);
    m.tint(m.expr("YELLOW", "RGB565_R_MAX | RGB565_G_MAX", "red and green"),
           0xF5D90AFFu);
    m.tint(m.expr("CYAN", "RGB565_G_MAX | RGB565_B_MAX", "green and blue"),
           0x00C2D7FFu);
    m.tint(m.expr("MAGENTA", "RGB565_R_MAX | RGB565_B_MAX", "red and blue"),
           0xD6409FFFu);
    m.tint(m.expr("GREY", "RGB565_R_HALF | RGB565_G_HALF | RGB565_B_HALF",
                  "half of each"),
           0x8B8D98FFu);

    m.open("Packing", "", "");
    m.macro("PACK", "r, g, b",
            "((((r) & 0xF8u) << 8) | (((g) & 0xFCu) << 3) | "
            "(((b) & 0xF8u) >> 3))",
            "a pixel from 8-bit red, green and blue, their low bits dropped");

    m.segment("blue", 0, 4, false);
    m.segment("green", 5, 10, false);
    m.segment("red", 11, 15, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "rgb565.h";

    return m.s;
}

// -------------------------------------------------------------- Windows

/*
win32_hresult
  Windows' HRESULT: a severity bit that is the sign bit of a signed LONG,
four flags, an 11-bit facility and a 16-bit code -- with the codes built
from those parts, as winerror.h builds them.
*/
sheet
win32_hresult()
{
    make m;
    m.s = make_sheet("win32_hresult", 32);
    m.s.prefix = "WIN_";
    m.s.probe  = 0x80070005u;   // E_ACCESSDENIED

    m.open("Fields", "HR_", "An HRESULT, as winerror.h and [MS-ERREF] lay "
           "it out. It is a signed LONG, and the severity bit is its sign: "
           "FAILED(hr) is hr < 0.");
    m.bit("SEVERITY", 31, "S: set on failure");
    m.bit("RESERVED", 30, "R: reserved, but for an NTSTATUS mapped in");
    m.bit("CUSTOMER", 29, "C: set in codes a customer defines, rather than "
          "Microsoft");
    m.bit("NTSTATUS", 28, "N: set when the value maps an NTSTATUS");
    m.bit("X", 27, "X: reserved, and zero");

    const int facility = m.range("FACILITY", 16, 11,
                                 "the system that defines the code");

    m.range("CODE", 0, 16, "the code, within its facility");

    m.open("Facilities", "FACILITY_", "");
    m.number("NULL", facility, 0, "general codes: E_FAIL, E_NOTIMPL");
    m.number("RPC", facility, 1, "remote procedure calls");
    m.number("DISPATCH", facility, 2, "IDispatch: Automation");
    m.number("STORAGE", facility, 3, "structured storage");
    m.number("ITF", facility, 4, "defined by the interface that returns it");
    m.number("WIN32", facility, 7, "a Win32 error code");
    m.number("WINDOWS", facility, 8, "Windows");
    m.number("SECURITY", facility, 9, "security");
    m.number("CONTROL", facility, 10, "ActiveX controls");

    m.open("Codes", "", "Common HRESULTs: the general failures, and Win32 "
           "errors carried in FACILITY_WIN32.");
    m.literal("S_OK", 0x00000000u, "success");
    m.literal("S_FALSE", 0x00000001u, "success, with a negative answer");
    m.expr("E_NOTIMPL", "WIN_HR_SEVERITY | 0x4001", "not implemented");
    m.expr("E_NOINTERFACE", "WIN_HR_SEVERITY | 0x4002",
           "no such interface");
    m.expr("E_POINTER", "WIN_HR_SEVERITY | 0x4003", "an invalid pointer");
    m.expr("E_ABORT", "WIN_HR_SEVERITY | 0x4004", "the operation was "
           "aborted");
    m.expr("E_FAIL", "WIN_HR_SEVERITY | 0x4005", "unspecified failure");
    m.expr("E_UNEXPECTED", "WIN_HR_SEVERITY | 0xFFFF", "catastrophic "
           "failure");
    m.expr("E_ACCESSDENIED", "WIN_HR_SEVERITY | WIN_FACILITY_WIN32 | 5",
           "ERROR_ACCESS_DENIED");
    m.expr("E_HANDLE", "WIN_HR_SEVERITY | WIN_FACILITY_WIN32 | 6",
           "ERROR_INVALID_HANDLE");
    m.expr("E_OUTOFMEMORY", "WIN_HR_SEVERITY | WIN_FACILITY_WIN32 | 14",
           "ERROR_OUTOFMEMORY");
    m.expr("E_INVALIDARG", "WIN_HR_SEVERITY | WIN_FACILITY_WIN32 | 87",
           "ERROR_INVALID_PARAMETER");

    m.open("Macros", "", "");
    m.macro("MAKE_HRESULT", "sev, fac, code",
            "((((sev) & 1u) << 31) | (((fac) & 0x7FFu) << 16) | "
            "((code) & 0xFFFFu))",
            "an HRESULT from its parts, as MAKE_HRESULT makes one");
    m.macro("HRESULT_FROM_WIN32", "err",
            "((err) == 0 ? 0u : (((err) & 0xFFFFu) | {WIN_FACILITY_WIN32} | "
            "{WIN_HR_SEVERITY}))",
            "a Win32 error code as an HRESULT; winerror.h's also passes "
            "through what is already one");
    m.macro("FAILED", "hr", "(((hr) & {WIN_HR_SEVERITY}) != 0)",
            "whether hr is a failure: its sign bit");
    m.macro("SUCCEEDED", "hr", "(((hr) & {WIN_HR_SEVERITY}) == 0)",
            "whether hr is a success");

    m.segment("code", 0, 15, false);
    m.segment("facility", 16, 26, false);
    m.segment("flags", 27, 31, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "win32_hresult.h";

    return m.s;
}

/*
win32_window_style
  A window's style: every bit of the high half is a style, two of them under
two names that mean different things on different windows, and the low
half belongs to the window's class.
*/
sheet
win32_window_style()
{
    make m;
    m.s = make_sheet("win32_window_style", 32);
    m.s.prefix = "WIN_";
    m.s.probe  = 0x10CF0000u;   // WS_OVERLAPPEDWINDOW | WS_VISIBLE

    m.open("Styles", "WS_", "A window's style, as CreateWindowEx takes it "
           "and winuser.h names it: the high sixteen bits are every "
           "window's, the low sixteen its class's own -- a button's BS_*, "
           "an edit control's ES_*.");
    m.literal("OVERLAPPED", 0x00000000u, "no bits: an overlapped window");
    m.bit("POPUP", 31, "a pop-up window");
    m.bit("CHILD", 30, "a child window: no menu bar");
    m.bit("MINIMIZE", 29, "starts minimized");
    m.bit("VISIBLE", 28, "starts visible");
    m.bit("DISABLED", 27, "starts disabled");
    m.bit("CLIPSIBLINGS", 26, "its siblings are clipped out of its "
          "painting");
    m.bit("CLIPCHILDREN", 25, "its children are clipped out");
    m.bit("MAXIMIZE", 24, "starts maximized");
    m.bit("BORDER", 23, "a thin border");
    m.bit("DLGFRAME", 22, "a dialog box's frame");
    m.bit("VSCROLL", 21, "a vertical scroll bar");
    m.bit("HSCROLL", 20, "a horizontal scroll bar");
    m.bit("SYSMENU", 19, "a window menu on its title bar");
    m.bit("THICKFRAME", 18, "a sizing border");
    m.bit("GROUP", 17, "a control that begins a group");
    m.bit("TABSTOP", 16, "a control the Tab key stops at");

    m.open("Title bar", "WS_", "Two bits under second names: a window with "
           "a title bar has no tab order to join.");
    m.shared();
    m.bit("MINIMIZEBOX", 17, "a minimize button: WS_GROUP's bit");
    m.bit("MAXIMIZEBOX", 16, "a maximize button: WS_TABSTOP's bit");

    m.open("Combinations", "WS_", "");
    m.expr("CAPTION", "WIN_WS_BORDER | WIN_WS_DLGFRAME",
           "a title bar: a border and a dialog frame together");
    m.expr("OVERLAPPEDWINDOW",
           "WIN_WS_OVERLAPPED | WIN_WS_CAPTION | WIN_WS_SYSMENU | "
           "WIN_WS_THICKFRAME | WIN_WS_MINIMIZEBOX | WIN_WS_MAXIMIZEBOX",
           "an application's main window");
    m.expr("POPUPWINDOW", "WIN_WS_POPUP | WIN_WS_BORDER | WIN_WS_SYSMENU",
           "a pop-up with a border and a window menu");

    m.open("Older names", "WS_", "Kept by winuser.h from 16-bit Windows.");
    m.expr("TILED", "WIN_WS_OVERLAPPED", "WS_OVERLAPPED");
    m.expr("ICONIC", "WIN_WS_MINIMIZE", "WS_MINIMIZE");
    m.expr("SIZEBOX", "WIN_WS_THICKFRAME", "WS_THICKFRAME");
    m.expr("TILEDWINDOW", "WIN_WS_OVERLAPPEDWINDOW", "WS_OVERLAPPEDWINDOW");
    m.expr("CHILDWINDOW", "WIN_WS_CHILD", "WS_CHILD");

    m.segment("class styles", 0, 15, true);
    m.segment("window styles", 16, 31, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "win32_window_style.h";

    return m.s;
}

// ---------------------------------------------------------------- macOS

/*
macos_macho_flags
  A dense word of flags: a Mach-O header's, 29 of its 32 bits named.
*/
sheet
macos_macho_flags()
{
    make m;
    m.s = make_sheet("macos_macho_flags", 32);
    m.s.prefix = "MACHO_";
    m.s.probe  = 0x00200085u;   // a typical executable's

    m.open("Linking", "MH_", "The flags field of struct mach_header, as "
           "mach-o/loader.h names it.");
    m.bit("NOUNDEFS", 0, "no undefined references");
    m.bit("INCRLINK", 1, "the output of an incremental link");
    m.bit("DYLDLINK", 2, "input to the dynamic linker");
    m.bit("BINDATLOAD", 3, "undefined references bound at load time");
    m.bit("PREBOUND", 4, "dynamic references prebound");
    m.bit("SPLIT_SEGS", 5, "read-only and writable segments split");
    m.bit("LAZY_INIT", 6, "obsolete");
    m.bit("TWOLEVEL", 7, "two-level namespace bindings");
    m.bit("FORCE_FLAT", 8, "every image forced into a flat namespace");
    m.bit("NOMULTIDEFS", 9, "no symbol defined twice among its "
          "sub-images");
    m.bit("NOFIXPREBINDING", 10, "dyld is not to fix its prebinding");
    m.bit("PREBINDABLE", 11, "not prebound, but could be");
    m.bit("ALLMODSBOUND", 12, "binds to every two-level module");
    m.bit("SUBSECTIONS_VIA_SYMBOLS", 13, "sections may be divided at "
          "symbols, for dead-code stripping");
    m.bit("CANONICAL", 14, "canonicalized by unprebinding");
    m.bit("WEAK_DEFINES", 15, "defines weak symbols");
    m.bit("BINDS_TO_WEAK", 16, "uses weak symbols");

    m.open("Security", "MH_", "");
    m.bit("ALLOW_STACK_EXECUTION", 17, "its stacks may execute");
    m.bit("ROOT_SAFE", 18, "safe in a process with a root uid");
    m.bit("SETUID_SAFE", 19, "safe in a setuid process");
    m.bit("PIE", 21, "loaded at a random address");
    m.bit("NO_HEAP_EXECUTION", 24, "its heap may not execute");

    m.open("Libraries", "MH_", "");
    m.bit("NO_REEXPORTED_DYLIBS", 20, "a dylib that re-exports none");
    m.bit("DEAD_STRIPPABLE_DYLIB", 22, "a dylib not linked if nothing "
          "uses it");
    m.bit("HAS_TLV_DESCRIPTORS", 23, "has thread-local variables");
    m.bit("APP_EXTENSION_SAFE", 25, "linked for an app extension");
    m.bit("NLIST_OUTOFSYNC_WITH_DYLDINFO", 26, "its symbol table and dyld "
          "info disagree");
    m.bit("SIM_SUPPORT", 27, "built for the simulator, too");
    m.bit("DYLIB_IN_CACHE", 31, "a dylib in the shared cache");

    m.open("Typical", "", "What otool -h shows for most files.");
    m.expr("EXECUTABLE",
           "MACHO_MH_NOUNDEFS | MACHO_MH_DYLDLINK | MACHO_MH_TWOLEVEL | "
           "MACHO_MH_PIE",
           "an executable");
    m.expr("DYLIB",
           "MACHO_MH_NOUNDEFS | MACHO_MH_DYLDLINK | MACHO_MH_TWOLEVEL | "
           "MACHO_MH_NO_REEXPORTED_DYLIBS",
           "a dynamic library");

    m.s.gen.file_name = "macos_macho_flags.h";

    return m.s;
}

/*
macos_cpu_type
  A signed int: a cpu_type_t keeps its ABI's bits in the top byte and its
processor family below, and CPU_TYPE_ANY is -1.
*/
sheet
macos_cpu_type()
{
    make m;
    m.s = make_sheet("macos_cpu_type", 32);
    m.s.prefix = "MACH_";
    m.s.probe  = 0x0100000Cu;   // CPU_TYPE_ARM64
    m.s.grid.abbreviate_unused = true;

    m.open("Fields", "CPU_", "A cpu_type_t, as mach/machine.h lays it out: "
           "a signed int, the ABI's capability bits in its top byte and the "
           "processor family below them.");

    const int arch   = m.range("ARCH_MASK", 24, 8,
                               "the ABI's capability bits");
    const int family = m.range("FAMILY", 0, 24, "the processor family");

    m.open("ABIs", "CPU_ARCH_", "");
    m.number("ABI64", arch, 1, "a 64-bit ABI");
    m.number("ABI64_32", arch, 2, "64-bit hardware, 32-bit pointers: the "
             "Apple Watch's");

    m.open("Types", "CPU_TYPE_", "");
    m.negative("ANY", -1, "any processor: (cpu_type_t) -1");
    m.number("VAX", family, 1, "VAX");
    m.number("MC680x0", family, 6, "Motorola 68000");
    m.number("X86", family, 7, "x86");
    m.number("MC98000", family, 10, "Motorola 98000");
    m.number("HPPA", family, 11, "HP PA-RISC");
    m.number("ARM", family, 12, "Arm");
    m.number("MC88000", family, 13, "Motorola 88000");
    m.number("SPARC", family, 14, "SPARC");
    m.number("I860", family, 15, "Intel i860");
    m.number("POWERPC", family, 18, "PowerPC");
    m.expr("I386", "MACH_CPU_TYPE_X86", "the older name for x86");
    m.expr("X86_64", "MACH_CPU_TYPE_X86 | MACH_CPU_ARCH_ABI64",
           "Intel Macs");
    m.expr("ARM64", "MACH_CPU_TYPE_ARM | MACH_CPU_ARCH_ABI64",
           "Apple silicon, iPhones and iPads");
    m.expr("ARM64_32", "MACH_CPU_TYPE_ARM | MACH_CPU_ARCH_ABI64_32",
           "the Apple Watch");
    m.expr("POWERPC64", "MACH_CPU_TYPE_POWERPC | MACH_CPU_ARCH_ABI64",
           "the Power Mac G5");

    m.open("Subtypes", "CPU_SUBTYPE_", "A cpu_subtype_t has the same shape: "
           "feature bits on top, the subtype below -- and its top bit means "
           "one thing for x86-64 and another for arm64e.");
    m.shared();
    m.range("MASK", 24, 8, "the feature bits");
    m.bit("LIB64", 31, "for x86-64: 64-bit libraries");
    m.bit("PTRAUTH_ABI", 31, "for arm64e: pointer authentication, with a "
          "versioned ABI");
    m.literal("X86_64_ALL", 3u, "any x86-64");
    m.literal("X86_64_H", 8u, "Haswell's feature subset");
    m.literal("ARM64_ALL", 0u, "any arm64");
    m.literal("ARM64E", 2u, "arm64e: pointer authentication");

    m.segment("family", 0, 23, false);
    m.segment("ABI", 24, 31, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "macos_cpu_type.h";

    return m.s;
}

// ---------------------------------------------------------------- Linux

/*
linux_ioctl
  An ioctl number: a direction, the argument's size, the driver's letter
and the command's number, packed by the _IOC macros -- here as fields,
macros that pack them, and commands both old and new.
*/
sheet
linux_ioctl()
{
    make m;
    m.s = make_sheet("linux_ioctl", 32);
    m.s.prefix = "LINUX_";
    m.s.probe  = 0x80044501u;   // EVIOCGVERSION

    m.open("Fields", "IOC_", "An ioctl number, as asm-generic/ioctl.h packs "
           "it for x86, Arm and RISC-V; Alpha, MIPS, PowerPC and SPARC give "
           "the direction three bits and the size thirteen.");

    m.range("NR", 0, 8, "the command's number");

    const int type = m.range("TYPE", 8, 8,
                             "the driver's letter: 'T' for terminals, 'E' "
                             "for input devices");

    m.range("SIZE", 16, 14, "the argument's size in bytes");

    const int dir = m.range("DIR", 30, 2, "which way the argument goes");

    m.open("Directions", "IOC_DIR_", "In place: _IOC_READ is 2, unshifted.");
    m.number("NONE", dir, 0, "no argument");
    m.number("WRITE", dir, 1, "user to kernel: the argument is written");
    m.number("READ", dir, 2, "kernel to user: the argument is read back");
    m.number("READ_WRITE", dir, 3, "both");

    m.open("Drivers", "IOC_TYPE_", "Some drivers' letters, in place.");
    m.number("BLOCK", type, 0x12, "block devices");
    m.number("EVDEV", type, 'E', "input devices");
    m.number("RANDOM", type, 'R', "/dev/random");
    m.number("TTY", type, 'T', "terminals, and TUN/TAP");

    m.open("Packing", "", "The _IOC family: types for sizes, as the "
           "kernel's take them.");
    m.macro("IOC", "dir, type, nr, size",
            "((((dir) & 3u) << 30) | (((size) & 0x3FFFu) << 16) | "
            "(((type) & 0xFFu) << 8) | ((nr) & 0xFFu))",
            "a number from its parts, as _IOC packs one");
    m.macro("IO", "type, nr", "{IOC}(0u, (type), (nr), 0u)",
            "a command without an argument");
    m.macro("IOR", "type, nr, size", "{IOC}(2u, (type), (nr), sizeof(size))",
            "a command that reads a size back");
    m.macro("IOW", "type, nr, size", "{IOC}(1u, (type), (nr), sizeof(size))",
            "a command that writes a size");
    m.macro("IOWR", "type, nr, size",
            "{IOC}(3u, (type), (nr), sizeof(size))",
            "a command that does both");

    m.open("Commands", "", "Old commands predate the packing, and fill only "
           "its type and number.");
    m.literal("FIONREAD", 0x541Bu, "bytes waiting to be read");
    m.literal("TIOCGWINSZ", 0x5413u, "a terminal's window size");
    m.literal("BLKFLSBUF", 0x1261u, "_IO(0x12, 97): flush a block device");
    m.literal("EVIOCGVERSION", 0x80044501u,
              "_IOR('E', 0x01, int): an input driver's version");
    m.literal("EVIOCGRAB", 0x40044590u,
              "_IOW('E', 0x90, int): grab an input device");
    m.literal("TUNSETIFF", 0x400454CAu,
              "_IOW('T', 202, int): attach to a TUN or TAP interface");
    m.literal("RNDGETENTCNT", 0x80045200u,
              "_IOR('R', 0x00, int): the entropy count");

    m.segment("nr", 0, 7, false);
    m.segment("type", 8, 15, false);
    m.segment("size", 16, 29, false);
    m.segment("dir", 30, 31, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "linux_ioctl.h";

    return m.s;
}

/*
linux_epoll_events
  A sparse word: epoll's readiness events sit in the low fourteen bits and
its modes in the top four, with nothing between.
*/
sheet
linux_epoll_events()
{
    make m;
    m.s = make_sheet("linux_epoll_events", 32);
    m.s.prefix = "LINUX_";
    m.s.probe  = 0x80002001u;   // EPOLLIN | EPOLLRDHUP | EPOLLET
    m.s.grid.abbreviate_unused = true;

    m.open("Events", "EPOLL", "The events field of struct epoll_event, as "
           "sys/epoll.h names its bits.");
    m.bit("IN", 0, "readable");
    m.bit("PRI", 1, "urgent data: out-of-band, or a priority event");
    m.bit("OUT", 2, "writable");
    m.bit("ERR", 3, "an error: reported whether asked for or not");
    m.bit("HUP", 4, "hung up: reported whether asked for or not");
    m.bit("RDNORM", 6, "normal data readable");
    m.bit("RDBAND", 7, "priority data readable");
    m.bit("WRNORM", 8, "normal data writable");
    m.bit("WRBAND", 9, "priority data writable");
    m.bit("MSG", 10, "unused");
    m.bit("RDHUP", 13, "the peer shut down its writing half");

    m.open("Modes", "EPOLL", "Not events: how they are reported.");
    m.bit("EXCLUSIVE", 28, "wake one waiter, not every one");
    m.bit("WAKEUP", 29, "hold a wakeup source while it is ready");
    m.bit("ONESHOT", 30, "report once, then wait to be rearmed");
    m.bit("ET", 31, "edge-triggered: report changes, not states");

    m.open("Common", "", "");
    m.expr("ALWAYS", "LINUX_EPOLLERR | LINUX_EPOLLHUP",
           "what epoll reports unasked");
    m.expr("READ_EDGE", "LINUX_EPOLLIN | LINUX_EPOLLRDHUP | LINUX_EPOLLET",
           "a socket's reads, edge-triggered");

    m.s.gen.file_name = "linux_epoll_events.h";

    return m.s;
}

/*
linux_capabilities
  Eight bytes, the low 41 bits dense: a Linux capability set, a bit for
each capability, numbered as linux/capability.h numbers them.
*/
sheet
linux_capabilities()
{
    make m;
    m.s = make_sheet("linux_capabilities", 64);
    m.s.prefix     = "LINUX_CAP_";
    m.s.probe      = 0x00000000A80425FBull;   // a container's defaults
    m.s.gen.suffix = int_suffix::ull;
    m.s.grid.abbreviate_unused = true;
    m.use_template("{NAME}_BIT", true);

    m.open("Files", "", "A capability set: a bit for each capability, the "
           "bit numbered as linux/capability.h numbers the capability -- "
           "its _BIT is the kernel's CAP_ number.");
    m.bit("CHOWN", 0, "change a file's owner");
    m.bit("DAC_OVERRIDE", 1, "pass file permission checks");
    m.bit("DAC_READ_SEARCH", 2, "pass read and search checks");
    m.bit("FOWNER", 3, "act as a file's owner");
    m.bit("FSETID", 4, "keep set-user-ID bits through a change");
    m.bit("LINUX_IMMUTABLE", 9, "set immutable and append-only flags");
    m.bit("MKNOD", 27, "make device files");
    m.bit("LEASE", 28, "take leases on files");
    m.bit("SETFCAP", 31, "set file capabilities");

    m.open("Processes", "", "");
    m.bit("KILL", 5, "signal any process");
    m.bit("SETGID", 6, "change group IDs");
    m.bit("SETUID", 7, "change user IDs");
    m.bit("SETPCAP", 8, "change capability sets");
    m.bit("SYS_PTRACE", 19, "trace any process");
    m.bit("SYS_NICE", 23, "raise priorities");
    m.bit("SYS_RESOURCE", 24, "exceed resource limits");
    m.bit("CHECKPOINT_RESTORE", 40, "checkpoint and restore");

    m.open("Network", "", "");
    m.bit("NET_BIND_SERVICE", 10, "bind ports below 1024");
    m.bit("NET_BROADCAST", 11, "broadcast and multicast");
    m.bit("NET_ADMIN", 12, "administer the network");
    m.bit("NET_RAW", 13, "use raw and packet sockets");

    m.open("IPC", "", "");
    m.bit("IPC_LOCK", 14, "lock memory");
    m.bit("IPC_OWNER", 15, "pass IPC permission checks");

    m.open("System", "", "");
    m.bit("SYS_MODULE", 16, "load kernel modules");
    m.bit("SYS_RAWIO", 17, "raw I/O");
    m.bit("SYS_CHROOT", 18, "chroot");
    m.bit("SYS_PACCT", 20, "process accounting");
    m.bit("SYS_ADMIN", 21, "most of system administration");
    m.bit("SYS_BOOT", 22, "reboot");
    m.bit("SYS_TIME", 25, "set the clock");
    m.bit("SYS_TTY_CONFIG", 26, "configure terminals");
    m.bit("SYSLOG", 34, "the kernel's log");
    m.bit("WAKE_ALARM", 35, "wake the system");
    m.bit("BLOCK_SUSPEND", 36, "keep the system from suspending");

    m.open("Audit and security", "", "");
    m.bit("AUDIT_WRITE", 29, "write the audit log");
    m.bit("AUDIT_CONTROL", 30, "configure auditing");
    m.bit("MAC_OVERRIDE", 32, "override mandatory access control");
    m.bit("MAC_ADMIN", 33, "configure mandatory access control");
    m.bit("AUDIT_READ", 37, "read the audit log");

    m.open("Tracing", "", "");
    m.bit("PERFMON", 38, "performance monitoring");
    m.bit("BPF", 39, "BPF programs and maps");

    m.open("Sets", "", "");
    m.expr("ALL", "GENMASK(40, 0)", "every capability: CAP_LAST_CAP is 40");
    m.expr("CONTAINER_DEFAULT",
           "LINUX_CAP_CHOWN | LINUX_CAP_DAC_OVERRIDE | LINUX_CAP_FSETID | "
           "LINUX_CAP_FOWNER | LINUX_CAP_MKNOD | LINUX_CAP_NET_RAW | "
           "LINUX_CAP_SETGID | LINUX_CAP_SETUID | LINUX_CAP_SETFCAP | "
           "LINUX_CAP_SETPCAP | LINUX_CAP_NET_BIND_SERVICE | "
           "LINUX_CAP_SYS_CHROOT | LINUX_CAP_KILL | LINUX_CAP_AUDIT_WRITE",
           "what Docker gives a container");

    m.s.gen.file_name = "linux_capabilities.h";

    return m.s;
}

// -------------------------------------------------------------- FreeBSD

/*
freebsd_file_flags
  Two halves: a FreeBSD file's flags, the owner's in the low sixteen bits
and the superuser's in the high.
*/
sheet
freebsd_file_flags()
{
    make m;
    m.s = make_sheet("freebsd_file_flags", 32);
    m.s.prefix = "FBSD_";
    m.s.probe  = 0x00020000u;   // SF_IMMUTABLE: schg

    m.open("Halves", "", "st_flags, as sys/stat.h names it and chflags(1) "
           "sets it: the owner may change the low half, only the superuser "
           "the high.");
    m.range("UF_SETTABLE", 0, 16, "what a file's owner may change");
    m.range("SF_SETTABLE", 16, 16, "what only the superuser may change");

    m.open("Owner", "UF_", "");
    m.bit("NODUMP", 0, "nodump: left out of dumps");
    m.bit("IMMUTABLE", 1, "uchg: may not be changed");
    m.bit("APPEND", 2, "uappnd: may only be appended to");
    m.bit("OPAQUE", 3, "opaque: a directory union mounts see no further "
          "than");
    m.bit("NOUNLINK", 4, "uunlnk: may not be removed or renamed");
    m.bit("SYSTEM", 7, "the Windows system bit");
    m.bit("SPARSE", 8, "sparse");
    m.bit("OFFLINE", 9, "offline");
    m.bit("REPARSE", 10, "the Windows reparse-point bit");
    m.bit("ARCHIVE", 11, "needs archiving");
    m.bit("READONLY", 12, "the Windows read-only bit");
    m.bit("NOCACHE", 13, "not to be cached, for NFSv4");
    m.bit("HIDDEN", 15, "hidden");

    m.open("Superuser", "SF_", "");
    m.bit("ARCHIVED", 16, "arch: archived");
    m.bit("IMMUTABLE", 17, "schg: may not be changed, even by root");
    m.bit("APPEND", 18, "sappnd: may only be appended to");
    m.bit("NOUNLINK", 20, "sunlnk: may not be removed or renamed");
    m.bit("SNAPSHOT", 21, "a snapshot's inode");

    m.open("Either", "", "");
    m.expr("ANY_IMMUTABLE", "FBSD_UF_IMMUTABLE | FBSD_SF_IMMUTABLE",
           "immutable, by either flag");
    m.expr("ANY_APPEND", "FBSD_UF_APPEND | FBSD_SF_APPEND",
           "append-only, by either flag");

    m.segment("owner", 0, 15, false);
    m.segment("superuser", 16, 31, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "freebsd_file_flags.h";

    return m.s;
}

/*
freebsd_kevent
  Two 16-bit fields of struct kevent in one sheet: its filter is a short,
and every filter a negative one; its flags are an unsigned short.
*/
sheet
freebsd_kevent()
{
    make m;
    m.s = make_sheet("freebsd_kevent", 16);
    m.s.prefix = "FBSD_";
    m.s.probe  = 0x0011u;   // EV_ADD | EV_ONESHOT

    m.open("Filters", "EVFILT_", "struct kevent's filter: a short, and every "
           "filter negative, as sys/event.h numbers them in FreeBSD 15.");
    m.negative("READ", -1, "a descriptor is readable");
    m.negative("WRITE", -2, "a descriptor is writable");
    m.negative("AIO", -3, "an asynchronous I/O request");
    m.negative("VNODE", -4, "a file changed");
    m.negative("PROC", -5, "a process changed");
    m.negative("SIGNAL", -6, "a signal arrived");
    m.negative("TIMER", -7, "a timer");
    m.negative("PROCDESC", -8, "a process descriptor");
    m.negative("FS", -9, "a file system event");
    m.negative("LIO", -10, "a list of I/O requests");
    m.negative("USER", -11, "an event the program triggers");
    m.negative("SENDFILE", -12, "a sendfile request");
    m.negative("EMPTY", -13, "a socket's send buffer is empty");
    m.negative("JAIL", -14, "a jail, from FreeBSD 15");
    m.negative("JAILDESC", -15, "a jail descriptor, from FreeBSD 15");
    m.literal("SYSCOUNT", 15u, "how many filters there are");

    m.open("Actions", "EV_", "struct kevent's flags: an unsigned short.");
    m.bit("ADD", 0, "add the event, and enable it");
    m.bit("DELETE", 1, "delete the event");
    m.bit("ENABLE", 2, "enable it");
    m.bit("DISABLE", 3, "disable it, unreported");
    m.bit("FORCEONESHOT", 8, "make it one-shot, and trigger it");
    m.bit("KEEPUDATA", 9, "keep its udata as it is");

    m.open("Flags", "EV_", "");
    m.bit("ONESHOT", 4, "report it once");
    m.bit("CLEAR", 5, "clear its state once reported");
    m.bit("RECEIPT", 6, "report an EV_ERROR receipt, even on success");
    m.bit("DISPATCH", 7, "disable it once reported");

    m.open("System", "EV_", "Reserved for the system; two names share a "
           "bit.");
    m.shared();
    m.range("SYSFLAGS", 12, 4, "the bits the system keeps");
    m.bit("DROP", 12, "the note is to be dropped");
    m.bit("FLAG1", 13, "a flag of the filter's own");
    m.bit("FLAG2", 14, "another, EV_ERROR's bit");
    m.bit("EOF", 15, "returned: end of file");
    m.bit("ERROR", 14, "returned: an error, and data holds its errno");

    m.segment("actions and flags", 0, 9, false);
    m.segment("system", 12, 15, false);
    m.s.grid.auto_segments = 0;
    m.s.gen.file_name      = "freebsd_kevent.h";

    return m.s;
}

// from_json
//   a large example: its bitfield, as the build embedded it, read as any
// bitfield is. The tests read every example clean; one that would not read
// is an empty sheet named for what went wrong, rather than none.
template<std::size_t Size>
sheet
from_json(
    const unsigned char (&_bytes)[Size]
)
{
    json_read read =
        read_json_sheet(std::string_view(reinterpret_cast<const char*>(_bytes),
                                         Size));

    if (!read.ok)
    {
        sheet broken;
        broken.title = "unreadable example: " + read.error;

        return broken;
    }

    return std::move(read.result);
}

// win32_hresults
//   every HRESULT MinGW-w64's winerror.h defines -- 1,376 codes, each a
// literal, in a group for its facility -- under the fields that make them:
// a thousand rows and more (tools/winerror_to_bitfield.py).
sheet
win32_hresults()
{
    return from_json(LARGE_WIN32_HRESULTS);
}

// x86_cpuid
//   ten of CPUID's feature registers end to end, 320 bits: word n of the
// header's array is the nth register, the MPX address-width adjust a field
// among the flags, and x86-64's levels sets of them
// (tools/x86_cpuid_bitfield.py).
sheet
x86_cpuid()
{
    return from_json(LARGE_X86_CPUID);
}

// entry
//   struct: an example, and the function that makes it.
struct entry
{
    example_info info;
    sheet      (*made)();
};

// the_examples
//   every example, in the File menu's order within each shelf. The first
// three keep their places: the tests open them by number.
const std::vector<entry>&
the_examples()
{
    static const std::vector<entry> table = {
        { { "uart_cr", "UART control register", "Hardware" }, uart_cr },
        { { "rv32i_itype", "RISC-V I-type instruction", "Formats" },
          rv32i_itype },
        { { "posix_mode", "File mode bits (st_mode)", "POSIX" },
          posix_mode },
        { { "usb_request_type", "USB bmRequestType", "Hardware" },
          usb_request_type },
        { { "lm75_temperature", "LM75 temperature register", "Hardware" },
          lm75_temperature },
        { { "x86_eflags", "x86 EFLAGS", "Hardware" }, x86_eflags },
        { { "x86_64_pte", "x86-64 page-table entry", "Hardware" },
          x86_64_pte },
        { { "ieee754_single", "IEEE 754 single (float)", "Formats" },
          ieee754_single },
        { { "rgb565", "RGB565 pixel", "Formats" }, rgb565 },
        { { "win32_hresult", "HRESULT (winerror.h)", "Windows" },
          win32_hresult },
        { { "win32_window_style", "Window styles (winuser.h)", "Windows" },
          win32_window_style },
        { { "macos_macho_flags", "Mach-O header flags (mach-o/loader.h)",
            "macOS" },
          macos_macho_flags },
        { { "macos_cpu_type", "CPU types (mach/machine.h)", "macOS" },
          macos_cpu_type },
        { { "linux_ioctl", "ioctl numbers (asm-generic/ioctl.h)", "Linux" },
          linux_ioctl },
        { { "linux_epoll_events", "epoll events (sys/epoll.h)", "Linux" },
          linux_epoll_events },
        { { "linux_capabilities", "Capabilities (linux/capability.h)",
            "Linux" },
          linux_capabilities },
        { { "freebsd_file_flags", "File flags (sys/stat.h)", "FreeBSD" },
          freebsd_file_flags },
        { { "freebsd_kevent", "kevent filters and flags (sys/event.h)",
            "FreeBSD" },
          freebsd_kevent },
        { { "pci_config_header", "PCI configuration header (512 bits)",
            "Hardware" },
          pci_config_header },
        { { "win32_hresults", "Every HRESULT in winerror.h (1,376 codes)",
            "Large" },
          win32_hresults },
        { { "x86_cpuid", "CPUID feature flags (320 bits)", "Large" },
          x86_cpuid }
    };

    return table;
}

}  // anonymous namespace


const std::vector<example_info>&
example_list()
{
    static const std::vector<example_info> list = []()
    {
        std::vector<example_info> made;

        for (const entry& each : the_examples())
        {
            made.push_back(each.info);
            made.back().width = each.made().width;
        }

        return made;
    }();

    return list;
}

std::vector<std::string>
example_names()
{
    std::vector<std::string> names;

    for (const example_info& each : example_list())
    {
        names.push_back(each.name);
    }

    return names;
}

sheet
make_example(
    const std::string& _name
)
{
    for (const entry& each : the_examples())
    {
        if (each.info.name == _name)
        {
            return each.made();
        }
    }

    return uart_cr();
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
