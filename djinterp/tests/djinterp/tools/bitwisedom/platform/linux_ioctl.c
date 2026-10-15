/*******************************************************************************
* bitwisedom [tests]                                               linux_ioctl.c
*
* The ioctl example against the kernel's headers: the fields, the
* directions, the commands and the _IOC family, compared with the
* platform's own at compile time. Built where the build targets
* Linux.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/linux_ioctl.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint32_t
#include <sys/ioctl.h>           // _IOC, FIONREAD, TIOCGWINSZ
#include <linux/fs.h>            // BLKFLSBUF
#include <linux/if_tun.h>        // TUNSETIFF
#include <linux/input.h>         // EVIOCGVERSION, EVIOCGRAB
#include <linux/random.h>        // RNDGETENTCNT
#include "linux_ioctl.h"         // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 32 unsigned bits both are.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint32_t)(_ours) == (uint32_t)(_theirs),                   \
                   #_ours " is " #_theirs)


// the fields: the kernel's masks are unshifted, the example's in place
SAME(LINUX_IOC_NR, (unsigned long)_IOC_NRMASK << _IOC_NRSHIFT);
SAME(LINUX_IOC_TYPE, (unsigned long)_IOC_TYPEMASK << _IOC_TYPESHIFT);
SAME(LINUX_IOC_SIZE, (unsigned long)_IOC_SIZEMASK << _IOC_SIZESHIFT);
SAME(LINUX_IOC_DIR, (unsigned long)_IOC_DIRMASK << _IOC_DIRSHIFT);
SAME(LINUX_IOC_NR_SHIFT, _IOC_NRSHIFT);
SAME(LINUX_IOC_TYPE_SHIFT, _IOC_TYPESHIFT);
SAME(LINUX_IOC_SIZE_SHIFT, _IOC_SIZESHIFT);
SAME(LINUX_IOC_DIR_SHIFT, _IOC_DIRSHIFT);
SAME(LINUX_IOC_NR_WIDTH, _IOC_NRBITS);
SAME(LINUX_IOC_TYPE_WIDTH, _IOC_TYPEBITS);
SAME(LINUX_IOC_SIZE_WIDTH, _IOC_SIZEBITS);
SAME(LINUX_IOC_DIR_WIDTH, _IOC_DIRBITS);

// the directions
SAME(LINUX_IOC_DIR_NONE, (unsigned long)_IOC_NONE << _IOC_DIRSHIFT);
SAME(LINUX_IOC_DIR_WRITE, (unsigned long)_IOC_WRITE << _IOC_DIRSHIFT);
SAME(LINUX_IOC_DIR_READ, (unsigned long)_IOC_READ << _IOC_DIRSHIFT);
SAME(LINUX_IOC_DIR_READ_WRITE,
     (unsigned long)(_IOC_READ | _IOC_WRITE) << _IOC_DIRSHIFT);

// the drivers' letters, in place, as each driver's own commands carry them
SAME(LINUX_IOC_TYPE_BLOCK, (unsigned long)_IOC_TYPE(BLKFLSBUF) << 8);
SAME(LINUX_IOC_TYPE_EVDEV, (unsigned long)_IOC_TYPE(EVIOCGVERSION) << 8);
SAME(LINUX_IOC_TYPE_RANDOM, (unsigned long)_IOC_TYPE(RNDGETENTCNT) << 8);
SAME(LINUX_IOC_TYPE_TTY, (unsigned long)_IOC_TYPE(TIOCGWINSZ) << 8);
SAME(LINUX_IOC_TYPE_TTY, (unsigned long)_IOC_TYPE(TUNSETIFF) << 8);

// the commands
SAME(LINUX_FIONREAD, FIONREAD);
SAME(LINUX_TIOCGWINSZ, TIOCGWINSZ);
SAME(LINUX_BLKFLSBUF, BLKFLSBUF);
SAME(LINUX_EVIOCGVERSION, EVIOCGVERSION);
SAME(LINUX_EVIOCGRAB, EVIOCGRAB);
SAME(LINUX_TUNSETIFF, TUNSETIFF);
SAME(LINUX_RNDGETENTCNT, RNDGETENTCNT);

// the packing macros, against the kernel's
SAME(LINUX_IO(0x12, 97), _IO(0x12, 97));
SAME(LINUX_IOR('E', 0x01, int), _IOR('E', 0x01, int));
SAME(LINUX_IOW('T', 202, int), _IOW('T', 202, int));
SAME(LINUX_IOWR('f', 0x42, long), _IOWR('f', 0x42, long));
SAME(LINUX_IOC(LINUX_IOC_DIR_READ >> 30, 'R', 0, 4),
     _IOC(_IOC_READ, 'R', 0, 4));
