/*******************************************************************************
* djinterp [net]                                           tcp_tests_sa_native.c
*
* The tcp suite's platform layer: what its tests ask of the system -- a
* pause, a clock, a watchdog, a socket's flags, options and limits, native
* error codes, the machine's name, files -- written once for BSD sockets
* and once for Winsock.
*   The tests call only tcp.h and these, so the suite runs wherever tcp.c
* has a backend: Windows builds it with MinGW-w64, and Wine runs it where no
* Windows is to hand. Each helper is chosen whole, by the backend env_tcp.h
* detects; none is split by #if.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_sa_native.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./tcp_tests_sa.h"  // corresponding header
// std
#include <stdio.h>  // snprintf

#if (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK)
    // windows
    #include <winsock2.h>  // getsockopt, gethostname, ioctlsocket, WSA codes
    #include <windows.h>   // Sleep, GetTickCount64, CreateThread, handles
#else
    // posix
    #include <errno.h>        // errno, EAGAIN, EBADF, ECONNREFUSED, EPIPE
    #include <fcntl.h>        // fcntl, FD_CLOEXEC, O_NONBLOCK
    #include <netinet/in.h>   // IPPROTO_TCP
    #include <netinet/tcp.h>  // TCP_NODELAY
    #include <sys/socket.h>   // getsockopt, SO_RCVTIMEO, SO_SNDTIMEO
    #include <sys/stat.h>     // stat
    #include <sys/time.h>     // struct timeval
    #include <time.h>         // clock_gettime, nanosleep
    #include <unistd.h>       // alarm, close, dup, gethostname, getpid
#endif


#if (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK)

/*
d_tests_tcp_watch_seconds
  The watchdog's time, for its thread to read.
*/
static unsigned d_tests_tcp_watch_seconds = 0u;

/*
d_tests_tcp_pause_ms
  Sleeps for at least the time given.
*/
void
d_tests_tcp_pause_ms(
    long _milliseconds
)
{
    Sleep((DWORD)_milliseconds);

    return;
}

/*
d_tests_tcp_now_ms
  Milliseconds since the first reading, from the monotonic tick count; the
origin keeps the value within a long, which Windows holds in 32 bits.
*/
long
d_tests_tcp_now_ms(void)
{
    static ULONGLONG origin = 0u;
    const ULONGLONG  now    = GetTickCount64();

    // the first reading is the origin
    if (origin == 0u)
    {
        origin = now;
    }

    return (long)(now - origin);
}

/*
d_tests_tcp_watchdog_wait
  The watchdog's thread: ends the process when its time runs out, as
alarm's signal would.
*/
static DWORD WINAPI
d_tests_tcp_watchdog_wait(
    LPVOID _unused
)
{
    (void)_unused;
    Sleep((DWORD)(d_tests_tcp_watch_seconds * 1000u));
    ExitProcess(3u);

    return 0u;
}

/*
d_tests_tcp_watchdog
  Starts a thread that ends the process after `_seconds`, for a run that
hangs: Windows has no alarm.
*/
void
d_tests_tcp_watchdog(
    unsigned _seconds
)
{
    d_tests_tcp_watch_seconds = _seconds;

    const HANDLE thread = CreateThread(NULL,
                                       0u,
                                       d_tests_tcp_watchdog_wait,
                                       NULL,
                                       0u,
                                       NULL);

    // the thread runs on without its handle
    if (thread != NULL)
    {
        (void)CloseHandle(thread);
    }

    return;
}

/*
d_tests_tcp_descriptor_closed
  A socket getsockopt calls no socket is closed.
*/
bool
d_tests_tcp_descriptor_closed(
    d_tcp_socket _socket
)
{
    int type = 0;
    int size = (int)sizeof(type);

    return ( (getsockopt(_socket,
                         SOL_SOCKET,
                         SO_TYPE,
                         (char*)&type,
                         &size) != 0) &&
             (WSAGetLastError() == WSAENOTSOCK) );
}

/*
d_tests_tcp_inheritable
  The socket itself, marked inheritable: a Winsock handle has no dup()
whose copy starts inheritable.
*/
d_tcp_socket
d_tests_tcp_inheritable(
    d_tcp_socket _socket
)
{
    return (SetHandleInformation((HANDLE)_socket,
                                 HANDLE_FLAG_INHERIT,
                                 HANDLE_FLAG_INHERIT))
               ? _socket
               : D_TCP_SOCKET_INVALID;
}

/*
d_tests_tcp_inherited
  Whether a child process would inherit the socket's handle.
*/
bool
d_tests_tcp_inherited(
    d_tcp_socket _socket
)
{
    DWORD flags = 0u;

    return ( (GetHandleInformation((HANDLE)_socket,
                                   &flags)) &&
             ((flags & HANDLE_FLAG_INHERIT) != 0u) );
}

/*
d_tests_tcp_drop
  Nothing: d_tests_tcp_inheritable's copy is the socket itself, which its
connection closes.
*/
void
d_tests_tcp_drop(
    d_tcp_socket _copy
)
{
    (void)_copy;

    return;
}

/*
d_tests_tcp_no_delay
  TCP_NODELAY as the socket reports it.
*/
bool
d_tests_tcp_no_delay(
    d_tcp_socket _socket,
    bool*        _on
)
{
    BOOL value = FALSE;
    int  size  = (int)sizeof(value);

    const bool fetched = (getsockopt(_socket,
                                     IPPROTO_TCP,
                                     TCP_NODELAY,
                                     (char*)&value,
                                     &size) == 0);

    *_on = ( (fetched) &&
             (value != FALSE) );

    return fetched;
}

/*
d_tests_tcp_nonblocking
  Puts the socket in non-blocking mode, as FIONBIO does.
*/
bool
d_tests_tcp_nonblocking(
    d_tcp_socket _socket
)
{
    u_long on = 1u;

    return (ioctlsocket(_socket,
                        FIONBIO,
                        &on) == 0);
}

/*
d_tests_tcp_timeout_ms
  A SO_RCVTIMEO or SO_SNDTIMEO limit as the socket reports it: Winsock holds
milliseconds where BSD sockets hold a timeval.
*/
long
d_tests_tcp_timeout_ms(
    d_tcp_socket _socket,
    bool         _receive
)
{
    DWORD held = 0u;
    int   size = (int)sizeof(held);

    // an unreadable limit holds nothing known
    if (getsockopt(_socket,
                   SOL_SOCKET,
                   (_receive) ? SO_RCVTIMEO : SO_SNDTIMEO,
                   (char*)&held,
                   &size) != 0)
    {
        return -1L;
    }

    return (long)held;
}

/*
d_tests_tcp_native_codes
  Winsock's codes for a call that would block, a refused connection, and a
write after the connection's own shutdown.
*/
struct d_tests_tcp_codes
d_tests_tcp_native_codes(void)
{
    const struct d_tests_tcp_codes codes = { WSAEWOULDBLOCK,
                                             WSAECONNREFUSED,
                                             WSAESHUTDOWN };

    return codes;
}

/*
d_tests_tcp_system_name
  The machine's name as gethostname gives it, Winsock started around the
call so that its count of starts is left as it was.
*/
bool
d_tests_tcp_system_name(
    char*  _buffer,
    size_t _capacity
)
{
    WSADATA    data    = { 0 };
    const bool started = (WSAStartup(MAKEWORD(2,
                                              2),
                                     &data) == 0);
    const bool named   = ( (started) &&
                           (gethostname(_buffer,
                                        (int)_capacity) == 0) );

    // every start is balanced by a cleanup
    if (started)
    {
        (void)WSACleanup();
    }

    return named;
}

/*
d_tests_tcp_unix_path
  A unix-domain path unique to this process and tag, in the temporary
folder. Winsock's AF_UNIX is not used, so the tests use it only to see it
refused.
*/
bool
d_tests_tcp_unix_path(
    char*       _buffer,
    size_t      _capacity,
    const char* _tag
)
{
    char        folder[MAX_PATH + 1] = { 0 };
    const DWORD length               = GetTempPathA((DWORD)sizeof(folder),
                                                    folder);
    const int   written              =
        ( (length > 0u) &&
          (length < sizeof(folder)) )
            ? snprintf(_buffer,
                       _capacity,
                       "%sdjinterp_tcp_%lu_%s.sock",
                       folder,
                       (unsigned long)GetCurrentProcessId(),
                       _tag)
            : -1;

    return ( (written > 0) &&
             ((size_t)written < _capacity) );
}

/*
d_tests_tcp_path_exists
  Whether anything is at a path.
*/
bool
d_tests_tcp_path_exists(
    const char* _path
)
{
    return (GetFileAttributesA(_path) != INVALID_FILE_ATTRIBUTES);
}

#else

/*
d_tests_tcp_pause_ms
  Sleeps, resuming after signals, for at least the time given.
*/
void
d_tests_tcp_pause_ms(
    long _milliseconds
)
{
    struct timespec left = { .tv_sec  = _milliseconds / 1000L,
                             .tv_nsec = (_milliseconds % 1000L) * 1000000L };

    // a signal cuts the sleep short, leaving the rest in left
    while ( (nanosleep(&left,
                       &left) != 0) &&
            (errno == EINTR) )
    {
        continue;
    }

    return;
}

/*
d_tests_tcp_now_ms
  A monotonic clock in milliseconds, for measuring how long a wait took.
*/
long
d_tests_tcp_now_ms(void)
{
    struct timespec now = { .tv_sec = 0 };

    (void)clock_gettime(CLOCK_MONOTONIC,
                        &now);

    return (long)now.tv_sec * 1000L + now.tv_nsec / 1000000L;
}

/*
d_tests_tcp_watchdog
  Arms alarm, whose signal ends a run that hangs.
*/
void
d_tests_tcp_watchdog(
    unsigned _seconds
)
{
    (void)alarm(_seconds);

    return;
}

/*
d_tests_tcp_descriptor_closed
  A descriptor fcntl calls bad is closed.
*/
bool
d_tests_tcp_descriptor_closed(
    d_tcp_socket _socket
)
{
    return ( (fcntl((int)_socket,
                    F_GETFD) < 0) &&
             (errno == EBADF) );
}

/*
d_tests_tcp_inheritable
  A dup() of the socket, which starts without FD_CLOEXEC whatever the
original carries.
*/
d_tcp_socket
d_tests_tcp_inheritable(
    d_tcp_socket _socket
)
{
    const int copy = dup((int)_socket);

    return (copy >= 0) ? (d_tcp_socket)copy : D_TCP_SOCKET_INVALID;
}

/*
d_tests_tcp_inherited
  Whether the descriptor lacks FD_CLOEXEC, so that a child would inherit it.
*/
bool
d_tests_tcp_inherited(
    d_tcp_socket _socket
)
{
    const int flags = fcntl((int)_socket,
                            F_GETFD);

    return ( (flags >= 0) &&
             ((flags & FD_CLOEXEC) == 0) );
}

/*
d_tests_tcp_drop
  Closes d_tests_tcp_inheritable's copy.
*/
void
d_tests_tcp_drop(
    d_tcp_socket _copy
)
{
    // only a copy that was made
    if (_copy != D_TCP_SOCKET_INVALID)
    {
        (void)close((int)_copy);
    }

    return;
}

/*
d_tests_tcp_no_delay
  TCP_NODELAY as the socket reports it.
*/
bool
d_tests_tcp_no_delay(
    d_tcp_socket _socket,
    bool*        _on
)
{
    int       value = 0;
    socklen_t size  = (socklen_t)sizeof(value);

    const bool fetched = (getsockopt((int)_socket,
                                     IPPROTO_TCP,
                                     TCP_NODELAY,
                                     &value,
                                     &size) == 0);

    *_on = ( (fetched) &&
             (value != 0) );

    return fetched;
}

/*
d_tests_tcp_nonblocking
  Adds O_NONBLOCK to the descriptor's status flags.
*/
bool
d_tests_tcp_nonblocking(
    d_tcp_socket _socket
)
{
    const int flags = fcntl((int)_socket,
                            F_GETFL);

    return ( (flags >= 0) &&
             (fcntl((int)_socket,
                    F_SETFL,
                    flags | O_NONBLOCK) == 0) );
}

/*
d_tests_tcp_timeout_ms
  A SO_RCVTIMEO or SO_SNDTIMEO limit as the socket reports it, from the
timeval BSD sockets hold.
*/
long
d_tests_tcp_timeout_ms(
    d_tcp_socket _socket,
    bool         _receive
)
{
    struct timeval held = { .tv_sec = 0 };
    socklen_t      size = (socklen_t)sizeof(held);

    // an unreadable limit holds nothing known
    if (getsockopt((int)_socket,
                   SOL_SOCKET,
                   (_receive) ? SO_RCVTIMEO : SO_SNDTIMEO,
                   &held,
                   &size) != 0)
    {
        return -1L;
    }

    return (long)held.tv_sec * 1000L + (long)held.tv_usec / 1000L;
}

/*
d_tests_tcp_native_codes
  errno's codes for a call that would block, a refused connection, and a
write after the connection's own shutdown.
*/
struct d_tests_tcp_codes
d_tests_tcp_native_codes(void)
{
    const struct d_tests_tcp_codes codes = { EAGAIN,
                                             ECONNREFUSED,
                                             EPIPE };

    return codes;
}

/*
d_tests_tcp_system_name
  The machine's name as gethostname gives it, held to the buffer and
terminated.
*/
bool
d_tests_tcp_system_name(
    char*  _buffer,
    size_t _capacity
)
{
    // room for a name and its terminator
    if (_capacity < 2u)
    {
        return false;
    }

    const bool named = (gethostname(_buffer,
                                    _capacity - 1u) == 0);

    _buffer[_capacity - 1u] = '\0';

    return named;
}

/*
d_tests_tcp_unix_path
  A unix-domain path unique to this process and tag, under /tmp.
*/
bool
d_tests_tcp_unix_path(
    char*       _buffer,
    size_t      _capacity,
    const char* _tag
)
{
    const int written = snprintf(_buffer,
                                 _capacity,
                                 "/tmp/djinterp_tcp_%ld_%s.sock",
                                 (long)getpid(),
                                 _tag);

    return ( (written > 0) &&
             ((size_t)written < _capacity) );
}

/*
d_tests_tcp_path_exists
  Whether anything is at a path.
*/
bool
d_tests_tcp_path_exists(
    const char* _path
)
{
    struct stat status = { 0 };

    return (stat(_path,
                 &status) == 0);
}

#endif
