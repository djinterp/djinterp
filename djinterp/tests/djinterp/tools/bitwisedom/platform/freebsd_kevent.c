/*******************************************************************************
* bitwisedom [tests]                                            freebsd_kevent.c
*
* The kevent example against sys/event.h: the filters, as the
* negative numbers they are, and the flags, compared with the
* platform's own at compile time. Built where the build targets
* FreeBSD.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/freebsd_kevent.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint16_t
#include <sys/types.h>           // what sys/event.h needs
#include <sys/event.h>           // EVFILT_READ, EV_ADD, ...
#include "freebsd_kevent.h"      // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 16 unsigned bits both are.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint16_t)(_ours) == (uint16_t)(_theirs),                   \
                   #_ours " is " #_theirs)

// SAME_SIGNED
//   ours is theirs, compared as the signed numbers both are.
#define SAME_SIGNED(_ours, _theirs)                                            \
    _Static_assert((long long)(_ours) == (long long)(_theirs),                \
                   #_ours " is " #_theirs)


// the filters: negative shorts
SAME_SIGNED(FBSD_EVFILT_READ, EVFILT_READ);
SAME_SIGNED(FBSD_EVFILT_WRITE, EVFILT_WRITE);
SAME_SIGNED(FBSD_EVFILT_AIO, EVFILT_AIO);
SAME_SIGNED(FBSD_EVFILT_VNODE, EVFILT_VNODE);
SAME_SIGNED(FBSD_EVFILT_PROC, EVFILT_PROC);
SAME_SIGNED(FBSD_EVFILT_SIGNAL, EVFILT_SIGNAL);
SAME_SIGNED(FBSD_EVFILT_TIMER, EVFILT_TIMER);
SAME_SIGNED(FBSD_EVFILT_PROCDESC, EVFILT_PROCDESC);
SAME_SIGNED(FBSD_EVFILT_FS, EVFILT_FS);
SAME_SIGNED(FBSD_EVFILT_LIO, EVFILT_LIO);
SAME_SIGNED(FBSD_EVFILT_USER, EVFILT_USER);
SAME_SIGNED(FBSD_EVFILT_SENDFILE, EVFILT_SENDFILE);
SAME_SIGNED(FBSD_EVFILT_EMPTY, EVFILT_EMPTY);

// FreeBSD 15's jail filters, and the count that includes them
#if defined(EVFILT_JAIL)
SAME_SIGNED(FBSD_EVFILT_JAIL, EVFILT_JAIL);
SAME_SIGNED(FBSD_EVFILT_JAILDESC, EVFILT_JAILDESC);
SAME_SIGNED(FBSD_EVFILT_SYSCOUNT, EVFILT_SYSCOUNT);
#endif

// the flags
SAME(FBSD_EV_ADD, EV_ADD);
SAME(FBSD_EV_DELETE, EV_DELETE);
SAME(FBSD_EV_ENABLE, EV_ENABLE);
SAME(FBSD_EV_DISABLE, EV_DISABLE);
SAME(FBSD_EV_FORCEONESHOT, EV_FORCEONESHOT);
SAME(FBSD_EV_KEEPUDATA, EV_KEEPUDATA);
SAME(FBSD_EV_ONESHOT, EV_ONESHOT);
SAME(FBSD_EV_CLEAR, EV_CLEAR);
SAME(FBSD_EV_RECEIPT, EV_RECEIPT);
SAME(FBSD_EV_DISPATCH, EV_DISPATCH);
SAME(FBSD_EV_SYSFLAGS, EV_SYSFLAGS);
SAME(FBSD_EV_DROP, EV_DROP);
SAME(FBSD_EV_FLAG1, EV_FLAG1);
SAME(FBSD_EV_FLAG2, EV_FLAG2);
SAME(FBSD_EV_EOF, EV_EOF);
SAME(FBSD_EV_ERROR, EV_ERROR);
