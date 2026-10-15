/*******************************************************************************
* bitwisedom [tests]                                        linux_epoll_events.c
*
* The epoll example against sys/epoll.h: every event and mode,
* compared with the platform's own at compile time. Built where the
* build targets Linux.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/linux_epoll_events.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint32_t
#include <sys/epoll.h>           // EPOLLIN, ...
#include "linux_epoll_events.h"  // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 32 unsigned bits both are.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint32_t)(_ours) == (uint32_t)(_theirs),                   \
                   #_ours " is " #_theirs)


// every event, and every mode
SAME(LINUX_EPOLLIN, EPOLLIN);
SAME(LINUX_EPOLLPRI, EPOLLPRI);
SAME(LINUX_EPOLLOUT, EPOLLOUT);
SAME(LINUX_EPOLLERR, EPOLLERR);
SAME(LINUX_EPOLLHUP, EPOLLHUP);
SAME(LINUX_EPOLLRDNORM, EPOLLRDNORM);
SAME(LINUX_EPOLLRDBAND, EPOLLRDBAND);
SAME(LINUX_EPOLLWRNORM, EPOLLWRNORM);
SAME(LINUX_EPOLLWRBAND, EPOLLWRBAND);
SAME(LINUX_EPOLLMSG, EPOLLMSG);
SAME(LINUX_EPOLLRDHUP, EPOLLRDHUP);
SAME(LINUX_EPOLLWAKEUP, EPOLLWAKEUP);
SAME(LINUX_EPOLLONESHOT, EPOLLONESHOT);
SAME(LINUX_EPOLLET, EPOLLET);

// EPOLLEXCLUSIVE arrived with Linux 4.5
#if defined(EPOLLEXCLUSIVE)
SAME(LINUX_EPOLLEXCLUSIVE, EPOLLEXCLUSIVE);
#endif
