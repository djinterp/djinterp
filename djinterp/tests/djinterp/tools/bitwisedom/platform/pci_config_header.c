/*******************************************************************************
* bitwisedom [tests]                                         pci_config_header.c
*
* The PCI example against linux/pci_regs.h: every register where Linux puts
* it, every bit and field as the mask Linux gives it in its register, and
* every named value, compared at compile time. Built where the build
* targets Linux. The example names a place by its bit in the whole header,
* and Linux by a byte offset and a mask in the register there: a register's
* first bit is its offset times eight.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/pci_config_header.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/

#include <linux/pci_regs.h>     // PCI_VENDOR_ID, PCI_COMMAND_IO, ...
#include "pci_config_header.h"  // the example, as bitwisedom-gen wrote it


// AT
//   a register is where Linux puts it.
#define AT(_ours, _offset)                                                     \
    _Static_assert((_ours) == (_offset) * 8, #_ours " is at " #_offset)

// MASK_IN
//   _width bits from _lsb, as their mask in the register at _offset.
#define MASK_IN(_lsb, _width, _offset)                                         \
    (((1ul << (_width)) - 1ul) << ((_lsb) - (_offset) * 8))

// BIT
//   a bit is the mask Linux gives it in its register.
#define BIT(_ours, _offset, _theirs)                                           \
    _Static_assert(MASK_IN(_ours, 1, _offset) ==                               \
                       ((unsigned long)(_theirs) & 0xFFFFFFFFul),              \
                   #_ours " is " #_theirs)

// FIELD
//   a field's bits are the mask Linux gives them in its register.
#define FIELD(_ours, _offset, _theirs)                                         \
    _Static_assert(MASK_IN(_ours, _ours##_WIDTH, _offset) ==                   \
                       ((unsigned long)(_theirs) & 0xFFFFFFFFul),              \
                   #_ours " is " #_theirs)

// VALUE
//   a named value, put in its field, is Linux's value in the register.
#define VALUE(_ours, _field, _offset, _theirs)                                 \
    _Static_assert(((unsigned long)(_ours) << ((_field) - (_offset) * 8)) ==   \
                       (unsigned long)(_theirs),                               \
                   #_ours " is " #_theirs)


// the header: 64 bytes, in its words
_Static_assert(PCI_CFG_WORDS * PCI_CFG_WORD_BITS == 64 * 8,
               "the header is 64 bytes");
#if defined(PCI_STD_HEADER_SIZEOF)
_Static_assert(PCI_CFG_WORDS * PCI_CFG_WORD_BITS == PCI_STD_HEADER_SIZEOF * 8,
               "the header is PCI_STD_HEADER_SIZEOF bytes");
#endif
#if defined(PCI_STD_NUM_BARS)
_Static_assert((PCI_CFG_BAR5 - PCI_CFG_BAR0) / 32 + 1 == PCI_STD_NUM_BARS,
               "there are PCI_STD_NUM_BARS BARs");
#endif

// every register, and the macro that gives a place's byte
AT(PCI_CFG_VENDOR_ID, PCI_VENDOR_ID);
AT(PCI_CFG_DEVICE_ID, PCI_DEVICE_ID);
AT(PCI_CFG_COMMAND, PCI_COMMAND);
AT(PCI_CFG_STATUS, PCI_STATUS);
AT(PCI_CFG_REVISION_ID, PCI_REVISION_ID);
AT(PCI_CFG_CLASS_PROG, PCI_CLASS_PROG);
AT(PCI_CFG_CLASS_SUB, PCI_CLASS_DEVICE);
AT(PCI_CFG_CACHE_LINE_SIZE, PCI_CACHE_LINE_SIZE);
AT(PCI_CFG_LATENCY_TIMER, PCI_LATENCY_TIMER);
AT(PCI_CFG_HEADER_TYPE, PCI_HEADER_TYPE);
AT(PCI_CFG_BIST, PCI_BIST);
AT(PCI_CFG_BAR0, PCI_BASE_ADDRESS_0);
AT(PCI_CFG_BAR1, PCI_BASE_ADDRESS_1);
AT(PCI_CFG_BAR2, PCI_BASE_ADDRESS_2);
AT(PCI_CFG_BAR3, PCI_BASE_ADDRESS_3);
AT(PCI_CFG_BAR4, PCI_BASE_ADDRESS_4);
AT(PCI_CFG_BAR5, PCI_BASE_ADDRESS_5);
AT(PCI_CFG_CARDBUS_CIS, PCI_CARDBUS_CIS);
AT(PCI_CFG_SUBSYSTEM_VENDOR_ID, PCI_SUBSYSTEM_VENDOR_ID);
AT(PCI_CFG_SUBSYSTEM_ID, PCI_SUBSYSTEM_ID);
AT(PCI_CFG_ROM_ADDRESS, PCI_ROM_ADDRESS);
AT(PCI_CFG_CAPABILITY_LIST, PCI_CAPABILITY_LIST);
AT(PCI_CFG_INTERRUPT_LINE, PCI_INTERRUPT_LINE);
AT(PCI_CFG_INTERRUPT_PIN, PCI_INTERRUPT_PIN);
AT(PCI_CFG_MIN_GNT, PCI_MIN_GNT);
AT(PCI_CFG_MAX_LAT, PCI_MAX_LAT);
_Static_assert(PCI_CFG_BYTE(PCI_CFG_COMMAND) == PCI_COMMAND,
               "BYTE(COMMAND) is PCI_COMMAND");

// the command register
BIT(PCI_CFG_CMD_IO, PCI_COMMAND, PCI_COMMAND_IO);
BIT(PCI_CFG_CMD_MEMORY, PCI_COMMAND, PCI_COMMAND_MEMORY);
BIT(PCI_CFG_CMD_MASTER, PCI_COMMAND, PCI_COMMAND_MASTER);
BIT(PCI_CFG_CMD_SPECIAL, PCI_COMMAND, PCI_COMMAND_SPECIAL);
BIT(PCI_CFG_CMD_INVALIDATE, PCI_COMMAND, PCI_COMMAND_INVALIDATE);
BIT(PCI_CFG_CMD_VGA_PALETTE, PCI_COMMAND, PCI_COMMAND_VGA_PALETTE);
BIT(PCI_CFG_CMD_PARITY, PCI_COMMAND, PCI_COMMAND_PARITY);
BIT(PCI_CFG_CMD_WAIT, PCI_COMMAND, PCI_COMMAND_WAIT);
BIT(PCI_CFG_CMD_SERR, PCI_COMMAND, PCI_COMMAND_SERR);
BIT(PCI_CFG_CMD_FAST_BACK, PCI_COMMAND, PCI_COMMAND_FAST_BACK);
BIT(PCI_CFG_CMD_INTX_DISABLE, PCI_COMMAND, PCI_COMMAND_INTX_DISABLE);

// the status register; Immediate Readiness came to Linux later than the rest
#if defined(PCI_STATUS_IMM_READY)
BIT(PCI_CFG_STATUS_IMM_READY, PCI_STATUS, PCI_STATUS_IMM_READY);
#endif
BIT(PCI_CFG_STATUS_INTERRUPT, PCI_STATUS, PCI_STATUS_INTERRUPT);
BIT(PCI_CFG_STATUS_CAP_LIST, PCI_STATUS, PCI_STATUS_CAP_LIST);
BIT(PCI_CFG_STATUS_66MHZ, PCI_STATUS, PCI_STATUS_66MHZ);
BIT(PCI_CFG_STATUS_UDF, PCI_STATUS, PCI_STATUS_UDF);
BIT(PCI_CFG_STATUS_FAST_BACK, PCI_STATUS, PCI_STATUS_FAST_BACK);
BIT(PCI_CFG_STATUS_PARITY, PCI_STATUS, PCI_STATUS_PARITY);
FIELD(PCI_CFG_STATUS_DEVSEL, PCI_STATUS, PCI_STATUS_DEVSEL_MASK);
VALUE(PCI_CFG_STATUS_DEVSEL_FAST, PCI_CFG_STATUS_DEVSEL, PCI_STATUS,
      PCI_STATUS_DEVSEL_FAST);
VALUE(PCI_CFG_STATUS_DEVSEL_MEDIUM, PCI_CFG_STATUS_DEVSEL, PCI_STATUS,
      PCI_STATUS_DEVSEL_MEDIUM);
VALUE(PCI_CFG_STATUS_DEVSEL_SLOW, PCI_CFG_STATUS_DEVSEL, PCI_STATUS,
      PCI_STATUS_DEVSEL_SLOW);
BIT(PCI_CFG_STATUS_SIG_TARGET_ABORT, PCI_STATUS, PCI_STATUS_SIG_TARGET_ABORT);
BIT(PCI_CFG_STATUS_REC_TARGET_ABORT, PCI_STATUS, PCI_STATUS_REC_TARGET_ABORT);
BIT(PCI_CFG_STATUS_REC_MASTER_ABORT, PCI_STATUS, PCI_STATUS_REC_MASTER_ABORT);
BIT(PCI_CFG_STATUS_SIG_SYSTEM_ERROR, PCI_STATUS, PCI_STATUS_SIG_SYSTEM_ERROR);
BIT(PCI_CFG_STATUS_DETECTED_PARITY, PCI_STATUS, PCI_STATUS_DETECTED_PARITY);

// the header type, and the self-test
FIELD(PCI_CFG_HEADER_LAYOUT, PCI_HEADER_TYPE, PCI_HEADER_TYPE_MASK);
VALUE(PCI_CFG_HEADER_NORMAL, PCI_CFG_HEADER_LAYOUT, PCI_HEADER_TYPE,
      PCI_HEADER_TYPE_NORMAL);
VALUE(PCI_CFG_HEADER_BRIDGE, PCI_CFG_HEADER_LAYOUT, PCI_HEADER_TYPE,
      PCI_HEADER_TYPE_BRIDGE);
VALUE(PCI_CFG_HEADER_CARDBUS, PCI_CFG_HEADER_LAYOUT, PCI_HEADER_TYPE,
      PCI_HEADER_TYPE_CARDBUS);
BIT(PCI_CFG_HEADER_MFD, PCI_HEADER_TYPE, PCI_HEADER_TYPE_MFD);
FIELD(PCI_CFG_BIST_CODE, PCI_BIST, PCI_BIST_CODE_MASK);
BIT(PCI_CFG_BIST_START, PCI_BIST, PCI_BIST_START);
BIT(PCI_CFG_BIST_CAPABLE, PCI_BIST, PCI_BIST_CAPABLE);

// a base address register's parts, and the expansion ROM's
BIT(PCI_CFG_BAR0_SPACE_IO, PCI_BASE_ADDRESS_0, PCI_BASE_ADDRESS_SPACE_IO);
FIELD(PCI_CFG_BAR0_MEM_TYPE, PCI_BASE_ADDRESS_0,
      PCI_BASE_ADDRESS_MEM_TYPE_MASK);
VALUE(PCI_CFG_BAR0_MEM_TYPE_32, PCI_CFG_BAR0_MEM_TYPE, PCI_BASE_ADDRESS_0,
      PCI_BASE_ADDRESS_MEM_TYPE_32);
VALUE(PCI_CFG_BAR0_MEM_TYPE_1M, PCI_CFG_BAR0_MEM_TYPE, PCI_BASE_ADDRESS_0,
      PCI_BASE_ADDRESS_MEM_TYPE_1M);
VALUE(PCI_CFG_BAR0_MEM_TYPE_64, PCI_CFG_BAR0_MEM_TYPE, PCI_BASE_ADDRESS_0,
      PCI_BASE_ADDRESS_MEM_TYPE_64);
BIT(PCI_CFG_BAR0_MEM_PREFETCH, PCI_BASE_ADDRESS_0,
    PCI_BASE_ADDRESS_MEM_PREFETCH);
FIELD(PCI_CFG_BAR0_MEM_ADDRESS, PCI_BASE_ADDRESS_0,
      PCI_BASE_ADDRESS_MEM_MASK);
FIELD(PCI_CFG_BAR0_IO_ADDRESS, PCI_BASE_ADDRESS_0, PCI_BASE_ADDRESS_IO_MASK);
BIT(PCI_CFG_ROM_ENABLE, PCI_ROM_ADDRESS, PCI_ROM_ADDRESS_ENABLE);
FIELD(PCI_CFG_ROM_BASE, PCI_ROM_ADDRESS, PCI_ROM_ADDRESS_MASK);
